#include "sdkconfig.h"

#include <string.h>
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_http_server.h"
#include "wifi_provisioning/manager.h"
#include <wifi_provisioning/scheme_softap.h>

#include "storage.h"
#include "networking.h"
#include "ui/ui.h"

#define CONN_DONE_FLAG              (1 << 0)
#define MAX_WIFI_CONN_RETRY         5
#define ESP_AP_SSID                 "ESP32-OOCD"
#define ESP_AP_PASS                 "esp32pass"
#define ESP_AP_MAX_CON              1
#define SSID_LENGTH                 (32 + 1)
#define PASSWORD_LENGTH             (64 + 1)
#define PROV_QR_VERSION             "v1"
#define PROV_TRANSPORT_SOFTAP       "softap"
#define QRCODE_BASE_URL             "https://espressif.github.io/esp-jumpstart/qrcode.html"

static const char *TAG = "networking";
static int s_retry_num = 0;
EventGroupHandle_t s_event_group;

static esp_err_t get_credentials(char **ssid, char **pass, char *content);

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_event_ap_staconnected_t *event;
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG, "Received Wi-Fi credentials"
                     "\n\tSSID     : %s\n\tPassword : %s",
                     (const char *) wifi_sta_cfg->ssid,
                     (const char *) wifi_sta_cfg->password);
            break;
        }
        case WIFI_PROV_CRED_FAIL: {
            wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
            ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                     "\n\tPlease reset to factory and retry provisioning",
                     (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                     "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
            s_retry_num++;
            if (s_retry_num >= MAX_WIFI_CONN_RETRY) {
                ESP_LOGI(TAG, "Failed to connect with provisioned AP, reseting provisioned credentials");
                wifi_prov_mgr_reset_sm_state_on_failure();
                s_retry_num = 0;
            }
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG, "Provisioning successful");
            s_retry_num = 0;
            break;
        case WIFI_PROV_END:
            /* De-initialize manager once provisioning is finished */
            wifi_prov_mgr_deinit();
            break;
        default:
            break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            if (s_retry_num < MAX_WIFI_CONN_RETRY) {
                esp_wifi_connect();
                s_retry_num++;
                ESP_LOGI(TAG, "retry to connect to the AP");
            } else {
                s_retry_num = 0;
#if CONFIG_CONN_PROV
                wifi_prov_mgr_reset_provisioning();
#else
                storage_nvs_erase_key(SSID_KEY);
                storage_nvs_erase_key(PASSWORD_KEY);
                storage_nvs_erase_key(WIFI_MODE_KEY);
#endif
                esp_restart();
            }
            ESP_LOGI(TAG, "connect to the AP fail");
            break;
        case WIFI_EVENT_AP_STACONNECTED:
            event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(TAG, "station " MACSTR " join, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        case WIFI_EVENT_AP_STADISCONNECTED:
            event = (wifi_event_ap_stadisconnected_t *)event_data;
            ESP_LOGI(TAG, "station " MACSTR " leave, AID=%d", MAC2STR(event->mac), event->aid);
            break;
        case WIFI_EVENT_AP_START:
            xEventGroupSetBits(s_event_group, CONN_DONE_FLAG);
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_event_group, CONN_DONE_FLAG);
#if CONFIG_CONN_PROV
        ui_clear_screen();
        ui_load_config_screen();
#endif
    }
}

static esp_err_t set_wifi_mode(char new_mode)
{
    esp_err_t ret = storage_nvs_write(WIFI_MODE_KEY, &new_mode, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save WiFi mode");
    }

    return ret;
}

static esp_err_t get_wifi_mode(char *mode)
{
    esp_err_t ret = storage_nvs_read(WIFI_MODE_KEY, mode, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi mode");
    }
    return ret;
}

static esp_err_t init_wifi_mode(void)
{
    if (!storage_nvs_is_key_exist(WIFI_MODE_KEY)) {
        return set_wifi_mode(WIFI_AP_MODE);
    }

    return ESP_OK;
}

static esp_err_t init_ap_mode(void)
{
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

static esp_err_t init_sta_mode(void)
{
    char ssid[SSID_LENGTH] = {0};
    char password[PASSWORD_LENGTH] = {0};

    /* Check that, credentials set before, if not set, init WiFi as access point */
    esp_err_t ret = storage_nvs_read(PASSWORD_KEY, password, PASSWORD_LENGTH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi password");
    }
    ret = storage_nvs_read(SSID_KEY, ssid, SSID_LENGTH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get WiFi SSID. Trying access point mode");
        ret = set_wifi_mode(WIFI_AP_MODE);
        if (ret != ESP_OK) {
            return ESP_FAIL;
        }
        esp_restart();
    }

    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT,
                        ESP_EVENT_ANY_ID,
                        &event_handler,
                        NULL),
                        TAG,
                        "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT,
                        IP_EVENT_STA_GOT_IP,
                        &event_handler,
                        NULL),
                        TAG,
                        "Failed at %s:%d", __FILE__, __LINE__);

    wifi_config_t wifi_config = { 0 };
    memcpy(wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, password, sizeof(wifi_config.sta.password));


    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    return ESP_OK;
}

static esp_err_t init_connection(void)
{
    char wifi_mode;
    esp_err_t ret_nvs = get_wifi_mode(&wifi_mode);

    if (ret_nvs == ESP_OK && wifi_mode == WIFI_STA_MODE) {
        return init_sta_mode();
    }
    return init_ap_mode();
}

static esp_err_t send_response(httpd_req_t *req, const char *response_msg)
{
    esp_err_t ret = httpd_resp_send(req, response_msg, HTTPD_RESP_USE_STRLEN);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    return ret;
}

static esp_err_t common_handler(httpd_req_t *req, int recv_size, const char *response_msg)
{
    if (!req) {
        return ESP_FAIL;
    }

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
    int ret = send_response(req, "The credentials are being reset");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error on reset response %d", ret);
        return ret;
    }

    ESP_ERROR_CHECK(storage_nvs_erase_key(SSID_KEY));
    ESP_ERROR_CHECK(storage_nvs_erase_key(PASSWORD_KEY));
    ESP_ERROR_CHECK(storage_nvs_erase_key(WIFI_MODE_KEY));

    esp_restart();

    return ESP_OK;
}

static esp_err_t save_credentials_handler(httpd_req_t *req)
{
    /* ssid + password + delimiter */
    const int credentials_length = SSID_LENGTH + PASSWORD_LENGTH + 1;
    size_t recv_size = MIN(req->content_len, credentials_length);
    char buff[SSID_LENGTH + PASSWORD_LENGTH + 1] = {0};
    req->user_ctx = buff;

    esp_err_t ret = common_handler(req, recv_size, "The credentials are being saved");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to handle save credentials request");
        return ESP_FAIL;
    }

    char *ssid, *pass;

    ESP_RETURN_ON_ERROR(get_credentials(&ssid, &pass, buff), TAG, "Error on parsing credentials");

    ret = storage_nvs_write(SSID_KEY, ssid, SSID_LENGTH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save SSID (%d)", ret);
    }

    ret = storage_nvs_write(PASSWORD_KEY, pass, PASSWORD_LENGTH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save WiFi password (%d)", ret);
    }

    set_wifi_mode(WIFI_STA_MODE);

    esp_restart();

    return ESP_OK;
}

static esp_err_t set_openocd_config_handler(httpd_req_t *req)
{
    size_t recv_size = req->content_len;
    /* Create an array sizeof content length + NULL and set last index as NULL */
    char buff[recv_size + 1]; buff[recv_size] = 0;

    req->user_ctx = buff;
    int ret = common_handler(req, recv_size, "OpenOCD config is being changed");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to handle save config request");
        return ESP_FAIL;
    }

    storage_nvs_erase_key(OOCD_F_PARAM_KEY);
    storage_nvs_erase_key(OOCD_C_PARAM_KEY);
    storage_nvs_erase_key(OOCD_D_PARAM_KEY);

    char delim = '&';
    char *tmp_str = strtok(buff, &delim);
    while (tmp_str != NULL) {
        if (strcmp(tmp_str, "-f") == 0 || strcmp(tmp_str, "--file") == 0) {
            char *param = strtok(NULL, &delim);
            if (param != NULL) {
                ESP_LOGI(TAG, "Saving -f param:[%s]", param);
                ret = storage_nvs_write(OOCD_F_PARAM_KEY, param, strlen(param));
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to save --file, -f parameter");
                }
            }
        } else if (strcmp(tmp_str, "-c") == 0 || strcmp(tmp_str, "--command") == 0) {
            char *param = strtok(NULL, &delim);
            if (param != NULL) {
                ESP_LOGI(TAG, "Saving -c param:[%s]", param);
                ret = storage_nvs_write(OOCD_C_PARAM_KEY, param, strlen(param));
                if (ret != ESP_OK) {
                    ESP_LOGE(TAG, "Failed to save --command, -c parameter");
                }
            }
        } else if (strstr(tmp_str, "-d") != NULL || strstr(tmp_str, "--debug") != NULL) {
            ESP_LOGI(TAG, "Saving -d param:[%s]", tmp_str);
            ret = storage_nvs_write(OOCD_D_PARAM_KEY, tmp_str, strlen(tmp_str));
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to save config --debug, -d parameter");
            }
        } else {
            ESP_LOGE(TAG, "Invalid parameter flag");
            return ESP_FAIL;
        }
        tmp_str = strtok(NULL, &delim);
    }

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

static esp_err_t get_credentials(char **ssid, char **pass, char *content)
{
    if (!content) {
        ESP_LOGE(TAG, "Given content info is NULL");
        return ESP_FAIL;
    }
    /* Expected string format is: ssid+password */
    char delim = '+';
    char *pass_str = strchr(content, delim);
    char *ssid_str = strtok(content, &delim);

    if (!ssid_str) {
        ESP_LOGE(TAG, "WiFi name is NULL");
        return ESP_FAIL;
    }
    *ssid = ssid_str;
    *pass = pass_str + 1;

    return ESP_OK;
}

static esp_err_t start_server(void)
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

esp_err_t web_server_init(void)
{
    ESP_RETURN_ON_ERROR(init_wifi_mode(), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(init_connection(), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(start_server(), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    return ESP_OK;
}

static void wifi_prov_print_qr(const char *name, const char *pop, const char *transport)
{
    if (!name || !transport) {
        ESP_LOGW(TAG, "Cannot generate QR code payload. Data missing.");
        return;
    }
    char payload[150] = {0};
    if (pop) {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                 ",\"pop\":\"%s\",\"transport\":\"%s\"}",
                 PROV_QR_VERSION, name, pop, transport);
    } else {
        snprintf(payload, sizeof(payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
                 ",\"transport\":\"%s\"}",
                 PROV_QR_VERSION, name, transport);
    }
    ui_load_connection_screen();
    ui_print_qr(payload);
    ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, payload);
}

esp_err_t provisioning_init(void)
{
    init_ui();
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    bool provisioned = false;
    ESP_RETURN_ON_ERROR(wifi_prov_mgr_is_provisioned(&provisioned), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    if (!provisioned) {
        ESP_LOGI(TAG, "Starting provisioning");

        wifi_prov_mgr_config_t config = {
            .scheme = wifi_prov_scheme_softap,
            .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
        };
        ESP_RETURN_ON_ERROR(wifi_prov_mgr_init(config), TAG, "Failed at %s:%d", __FILE__, __LINE__);

        wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
        wifi_prov_security1_params_t *sec_params = "abcd1234";

        /* Wi-Fi password */
        const char *service_key = NULL;
        ESP_RETURN_ON_ERROR(wifi_prov_mgr_start_provisioning(security, (const void *) sec_params, ESP_AP_SSID, service_key), TAG, "Failed at %s:%d", __FILE__, __LINE__);
        /* Print QR code for provisioning */
        wifi_prov_print_qr(ESP_AP_SSID, (char *)sec_params, PROV_TRANSPORT_SOFTAP);
    } else {
        ESP_LOGI(TAG, "Already provisioned, starting Wi-Fi STA");
        /* Start Wi-Fi station */
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed at %s:%d", __FILE__, __LINE__);
        ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    }
    return ESP_OK;
}

void reset_connection_config(void)
{
    ESP_ERROR_CHECK(storage_nvs_erase_everything());
}

static void wait_for_connection(TickType_t timeout)
{
    /* Wait a maximum of timeout value for CONN_DONE_FLAG to be set within
    the event group. Clear the bits before exiting. */
    EventBits_t bits = xEventGroupWaitBits(
                           s_event_group,      /* The event group being tested */
                           CONN_DONE_FLAG,     /* The bit within the event group to wait for */
                           pdTRUE,             /* CONN_DONE_FLAG should be cleared before returning */
                           pdFALSE,            /* Don't wait for bit, either bit will do */
                           timeout);           /* Wait a maximum of 5 mins for either bit to be set */

    if (bits == 0) {
        ESP_LOGE(TAG, "WiFi credentials has not entered in %d seconds. Restarting...", (timeout / 1000));
        esp_restart();
    }
}

esp_err_t networking_init(void)
{
    s_event_group = xEventGroupCreate();
    if (!s_event_group) {
        ESP_LOGE(TAG, "Event group did not create.");
        return ESP_FAIL;
    }

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Failed at %s:%d", __FILE__, __LINE__);


#if CONFIG_CONN_PROV
    ESP_RETURN_ON_ERROR(provisioning_init(), TAG, "Failed at %s:%d", __FILE__, __LINE__);
#else
    ESP_RETURN_ON_ERROR(web_server_init(), TAG, "Failed at %s:%d", __FILE__, __LINE__);
#endif

    wait_for_connection(30000 / portTICK_PERIOD_MS);

    return ESP_OK;
}
