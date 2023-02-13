#include "sdkconfig.h"

#if CONFIG_CONN_MANUAL
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

#include "storage.h"
#include "manual_config.h"

#define MAX_WIFI_CONN_RETRY         5

static const char *TAG = "manual_config";
static int s_retry_num = 0;
EventGroupHandle_t s_event_group;

static void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    wifi_event_ap_staconnected_t *event;
    if (event_base == WIFI_EVENT) {
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
            xEventGroupSetBits(s_event_group, OOCD_START_FLAG);
            break;
        default:
            break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_event_group, OOCD_START_FLAG);
    }
}

static esp_err_t init_sta_mode(void)
{
    char ssid[] = CONFIG_ESP_WIFI_SSID;
    char password[] = CONFIG_ESP_WIFI_PASSWORD;

    s_event_group = xEventGroupCreate();
    if (!s_event_group) {
        ESP_LOGE(TAG, "Event group did not create.");
        return ESP_FAIL;
    }

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
    strcpy((char *)wifi_config.sta.password, password);

    ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "Failed at %s:%d", __FILE__, __LINE__);

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    return ESP_OK;
}

static esp_err_t param_init(void)
{
    esp_err_t ret = ESP_OK;

    ret = storage_nvs_write(OOCD_F_PARAM_KEY, CONFIG_OPENOCD_F_PARAM, strlen(CONFIG_OPENOCD_F_PARAM));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save default --file, -f command");
        return ESP_FAIL;
    }

    ret = storage_nvs_write(OOCD_C_PARAM_KEY, CONFIG_OPENOCD_C_PARAM, strlen(CONFIG_OPENOCD_C_PARAM));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save default --command, -c command");
        return ESP_FAIL;
    }

    char d_param_str[4] = {0};
    sprintf(d_param_str, "-d%c", CONFIG_OPENOCD_D_PARAM + 48);
    ret = storage_nvs_write(OOCD_D_PARAM_KEY, d_param_str, strlen(d_param_str));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save default --debug, -d command");
        return ESP_FAIL;
    }

    return ESP_OK;
}

esp_err_t manual_config_init(void)
{
    ESP_RETURN_ON_ERROR(param_init(), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    ESP_RETURN_ON_ERROR(init_sta_mode(), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    return ESP_OK;
}
#endif /* CONFIG_CONN_MANUAL */
