#include "sdkconfig.h"

#include <string.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_err.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_check.h"
#include "wifi_provisioning/manager.h"
#include <wifi_provisioning/scheme_softap.h>

#include "storage.h"
#include "networking.h"
#include "ui/ui.h"
#include "web_server.h"

#define CONN_DONE_FLAG              (1 << 0)
#define MAX_WIFI_CONN_RETRY         5
#define ESP_AP_SSID                 "ESP32-OOCD"
#define ESP_AP_PASS                 "esp32pass"
#define ESP_AP_MAX_CON              1
#define PROV_QR_VERSION             "v1"
#define PROV_TRANSPORT_SOFTAP       "softap"
#define QRCODE_BASE_URL             "https://espressif.github.io/esp-jumpstart/qrcode.html"

static const char *TAG = "networking";
static int s_retry_num = 0;
httpd_handle_t server = NULL;
EventGroupHandle_t s_event_group;

#if CONFIG_UI_ENABLE
static esp_err_t provisioning_init(void);
#endif /* CONFIG_UI_ENABLE */

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
            storage_save_credentials((const char *) wifi_sta_cfg->ssid, (const char *) wifi_sta_cfg->password);
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
                storage_nvs_erase_key(SSID_KEY);
                storage_nvs_erase_key(PASSWORD_KEY);
                storage_nvs_erase_key(WIFI_MODE_KEY);
                s_retry_num = 0;
                esp_restart();
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
                storage_nvs_erase_key(SSID_KEY);
                storage_nvs_erase_key(PASSWORD_KEY);
                storage_nvs_erase_key(WIFI_MODE_KEY);
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
        char ip_addr[16] = {0};
        snprintf(ip_addr, sizeof(ip_addr), IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "got ip: %s", ip_addr);
        s_retry_num = 0;
        xEventGroupSetBits(s_event_group, CONN_DONE_FLAG);
#if CONFIG_UI_ENABLE
        ui_clear_screen();
        ui_load_config_screen();
        ui_load_message("Web Server IP address:", ip_addr);
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
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&cfg), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    esp_err_t ret_nvs = get_wifi_mode(&wifi_mode);

    if (ret_nvs == ESP_OK && wifi_mode == WIFI_STA_MODE) {
        esp_netif_create_default_wifi_sta();
        return init_sta_mode();
    }
    esp_netif_create_default_wifi_ap();
#if CONFIG_UI_ENABLE
    esp_netif_create_default_wifi_sta();
    return provisioning_init();
#endif /* CONFIG_UI_ENABLE */
    return init_ap_mode();
}

#if CONFIG_UI_ENABLE
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
    ui_load_message("Web Server IP address:", "192.168.4.1");
    ESP_LOGI(TAG, "If QR code is not visible, copy paste the below URL in a browser.\n%s?data=%s", QRCODE_BASE_URL, payload);
}

static esp_err_t provisioning_init(void)
{
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    ESP_LOGI(TAG, "Starting provisioning");

    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };
    ESP_RETURN_ON_ERROR(wifi_prov_mgr_init(config), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    wifi_prov_scheme_softap_set_httpd_handle((void *)&server);

    wifi_prov_security_t security = WIFI_PROV_SECURITY_1;
    wifi_prov_security1_params_t *sec_params = "abcd1234";

    /* Wi-Fi password */
    const char *service_key = NULL;
    ESP_RETURN_ON_ERROR(wifi_prov_mgr_start_provisioning(security, (const void *) sec_params, ESP_AP_SSID, service_key), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    /* Print QR code for provisioning */
    wifi_prov_print_qr(ESP_AP_SSID, (char *)sec_params, PROV_TRANSPORT_SOFTAP);

    return ESP_OK;
}
#endif /* CONFIG_UI_ENABLE */

void networking_reset_connection_config(void)
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

    storage_get_target_names(true);
    storage_get_openocd_config();
#if CONFIG_UI_ENABLE
    ui_init();
#endif /* CONFIG_UI_ENABLE */
    ESP_RETURN_ON_ERROR(init_wifi_mode(), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(web_server_start(&server), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(init_connection(), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    wait_for_connection(30000 / portTICK_PERIOD_MS);

    return ESP_OK;
}
