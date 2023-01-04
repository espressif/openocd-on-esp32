#include <sys/unistd.h>
#include <string.h>
#include <sys/param.h>
#include "esp_err.h"
#include "esp_http_server.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "lwip/inet.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_log.h"
#include "storage.h"
#include "esp_check.h"

#define ESP_AP_SSID                 "ESP-OCD"
#define ESP_AP_PASS                 "esp-ocd-pass"
#define ESP_AP_MAX_CON              1
#define MAX_WIFI_CONN_RETRY         5
#define SSID_LENGTH                 32
#define PASSWORD_LENGTH             64

static const char default_config[] = "target/esp32.cfg";
static char credentials_delimiter = '+';
static const char *TAG = "wifi";
static int s_retry_num = 0;

static esp_err_t init_connect_sta(char *credentials, int length);
static esp_err_t init_softap(void);
static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
static esp_err_t send_response(httpd_req_t *req, const char *response_msg);

static esp_err_t common_handler(httpd_req_t *req, int recv_size, const char *response_msg)
{
    if (!req)
        return ESP_FAIL;

    int ret = httpd_req_recv(req, req->user_ctx, recv_size);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }
    return send_response(req, response_msg);
}

static esp_err_t reset_credentials_handler(httpd_req_t *req)
{
    const char resp[] = "Credentials are resetting";
    int ret = send_response(req, resp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error on reset response %d", ret);
        return ret;
    }
    ESP_LOGW(TAG, "Credentials are resetting");
    ESP_ERROR_CHECK(erase_key_nvs(CREDENTIALS_KEY));
    esp_restart();
    return ESP_OK;
}

static esp_err_t save_credentials_handler(httpd_req_t *req)
{
    const char resp[] = "Connecting to given credentials";
    size_t recv_size = MIN(req->content_len, credentials_length - 1);
    /* ssid + password + delimiter + NULL */
    char buff[credentials_length];
    buff[recv_size] = 0;
    req->user_ctx = buff;

    int ret = common_handler(req, recv_size, resp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to handle save credentials request");
        return ESP_FAIL;
    }
    /* Writing credentials into NVS including NULL and restart the system */
    ret = write_nvs(CREDENTIALS_KEY, req->user_ctx, credentials_length);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "Failed to save credentials (%d)", ret);
    esp_restart();
    return ESP_OK;
}

static esp_err_t set_openocd_config_handler(httpd_req_t *req)
{
    const char resp[] = "Changing config";
    size_t recv_size = req->content_len;
    /* Create an array sizeof content length + NULL and set last index as NULL */
    char buff[recv_size + 1];
    buff[recv_size] = 0;
    req->user_ctx = buff;
    int ret = common_handler(req, recv_size, resp);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to handle save config request");
        return ESP_FAIL;
    }
    /* Writing config string into NVS including NULL */
    ret = write_nvs(CONFIG_KEY, req->user_ctx, recv_size + 1);
    if (ret != ESP_OK)
        ESP_LOGE(TAG, "Failed to save config file name");
    else
        ESP_LOGI(TAG, "Config file saved. Restarting device to run OpenOCD with given config.");
    vTaskDelay(200 / portTICK_PERIOD_MS);
    esp_restart();
    return ESP_OK;
}

static esp_err_t main_page_handler(httpd_req_t *req)
{
    /* Get handle to embedded file upload script */
    extern const unsigned char upload_script_start[] asm("_binary_web_page_html_start");
    extern const unsigned char upload_script_end[]   asm("_binary_web_page_html_end");
    const size_t upload_script_size = (upload_script_end - upload_script_start);

    /* Add file upload form and script which on execution sends a POST request to /upload */
    httpd_resp_send_chunk(req, (const char *)upload_script_start, upload_script_size);

    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t start_server()
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    /* Running web server on other core to get better response time */
    config.core_id = 1;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }

    /* URI handler to get main page */
    httpd_uri_t main_page = {
        .uri       = "/",
        .method    = HTTP_GET,
        .handler   = main_page_handler,
        .user_ctx  = NULL
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &main_page), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    /* URI handler to send credentials */
    httpd_uri_t save_credentials = {
        .uri       = "/save_credentials",
        .method    = HTTP_POST,
        .handler   = save_credentials_handler,
        .user_ctx  = NULL
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &save_credentials), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    /* URI handler to set OpenOCD parameters */
    httpd_uri_t set_openocd_config = {
        .uri       = "/set_openocd_config",
        .method    = HTTP_POST,
        .handler   = set_openocd_config_handler,
        .user_ctx  = NULL
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &set_openocd_config), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    /* URI handler to reset credentials */
    httpd_uri_t reset_credentials = {
        .uri       = "/reset_credentials",
        .method    = HTTP_POST,
        .handler   = reset_credentials_handler,
        .user_ctx  = NULL
    };
    ESP_RETURN_ON_ERROR(httpd_register_uri_handler(server, &reset_credentials), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    return ESP_OK;
}

static esp_err_t get_credentials(char **ssid, char **pass, char *content)
{
    if (!content) {
        ESP_LOGE(TAG, "Given content info is NULL");
        return ESP_FAIL;
    }

    /* Expected string format is: ssid+password */
    char *pass_str = strchr(content, credentials_delimiter);
    char *ssid_str = strtok(content, &credentials_delimiter);

    if (!ssid_str) {
        ESP_LOGE(TAG, "WiFi name is NULL");
        return ESP_FAIL;
    }

    *ssid = ssid_str;
    *pass = pass_str + 1;
    return ESP_OK;
}

static esp_err_t send_response(httpd_req_t *req, const char *response_msg)
{
    esp_err_t ret = httpd_resp_send(req, response_msg, HTTPD_RESP_USE_STRLEN);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    return ret;
}

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_WIFI_CONN_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else {
            s_retry_num = 0;
            /* If connection fails, delete the credentials and restart the system */
            erase_key_nvs(CREDENTIALS_KEY);
            esp_restart();
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
    }
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *)event_data;
        ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
    }
}

static esp_err_t init_connect_sta(char *credentials, int length)
{
    if (!credentials || length > credentials_length) {
        ESP_LOGE(TAG, "Invalid credentials");
        return ESP_FAIL;
    }
    char *ssid, *pass;
    char tmp_credentials[credentials_length];
    strncpy(tmp_credentials, credentials, length);
    ESP_RETURN_ON_ERROR(get_credentials(&ssid, &pass, tmp_credentials), TAG, "Error on parsing credentials");

    static bool sta_if_created = false;
    if (!sta_if_created) {
        esp_netif_create_default_wifi_sta();
        sta_if_created = true;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id),
                                                        TAG,
                                                        "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip),
                                                        TAG,
                                                        "Failed at %s:%d", __FILE__, __LINE__);

    wifi_config_t wifi_config = { 0 };
	strcpy((char *)wifi_config.sta.ssid, ssid);
	strcpy((char *)wifi_config.sta.password, pass);

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    ESP_LOGI(TAG, "wifi_init_sta finished.");
    return ESP_OK;
}

static esp_err_t init_softap(void)
{
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    ESP_RETURN_ON_ERROR(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL),
                                                        TAG,
                                                        "Failed at %s:%d", __FILE__, __LINE__);

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = ESP_AP_SSID,
            .ssid_len = strlen(ESP_AP_SSID),
            .password = ESP_AP_PASS,
            .max_connection = ESP_AP_MAX_CON,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        },
    };
    if (strlen(ESP_AP_PASS) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_AP), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_AP, &wifi_config), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s", ESP_AP_SSID, ESP_AP_PASS);
    return ESP_OK;
}

esp_err_t setup_network(char **config_str)
{
    /* ssid + password + delimiter + NULL */
    static char credentials[SSID_LENGTH + PASSWORD_LENGTH + 1 + 1];
    size_t config_length = strlen(default_config) + 1;

    ESP_ERROR_CHECK(start_server());

    /* First read length of the config string */
    int ret_nvs = read_nvs(CONFIG_KEY, NULL, &config_length);

    char *config_string = (char *)calloc(config_length, sizeof(char));
    if (!config_string) {
        ESP_LOGE(TAG, "Failed create config_string");
        return ESP_FAIL;
    }
    ret_nvs = read_nvs(CONFIG_KEY, config_string, &config_length);
    if (ret_nvs != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get config file name, default config will be used");
        strncpy(config_string, default_config, config_length);
    }

    /* Check that, credentials set before, if not set, init WiFi as access point */
    ret_nvs = read_nvs(CREDENTIALS_KEY, credentials, &credentials_length);
    if (ret_nvs == ESP_OK)
        ESP_ERROR_CHECK(init_connect_sta(credentials, credentials_length));
    else
        ESP_ERROR_CHECK(init_softap());

    *config_str = config_string;
    return ESP_OK;
}
