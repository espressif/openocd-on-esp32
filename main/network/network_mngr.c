#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_mac.h"
#include "wifi_provisioning/scheme_softap.h"

#include "network_mngr.h"
#include "ui.h"

#define WIFI_START_TIMEOUT              3000
#define WIFI_DISCONNECT_TIMEOUT         3000
#define WIFI_DEINIT_TIMEOUT             15000

#define WIFI_CONNECTED_BIT              BIT0
#define WIFI_DISCONNECTED_BIT           BIT1
#define WIFI_STARTED_BIT                BIT2
#define WIFI_CRED_RECV_BIT              BIT3
#define WIFI_CRED_FAIL_BIT              BIT4
#define WIFI_PROV_DONE_BIT              BIT5
#define WIFI_PROV_DEINIT_BIT            BIT6

static const char *TAG = "network-mngr";

/* objects may be destroyed upon application request */
static EventGroupHandle_t s_event_group = NULL;
static esp_netif_t *s_sta_netif = NULL;
static esp_netif_t *s_ap_netif = NULL;

static void net_utils_print_ip_info(void *event_data, const char *caption)
{
    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    esp_netif_t *netif = event->esp_netif;
    esp_netif_dns_info_t dns_info;
    esp_netif_ip_info_t ip_info;

    ESP_LOGI(TAG, "%s", caption);
    ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
    esp_netif_get_ip_info(netif, &ip_info);
    ESP_LOGI(TAG, "IP          : " IPSTR, IP2STR(&ip_info.ip));
    ESP_LOGI(TAG, "Gateway     : " IPSTR, IP2STR(&ip_info.gw));
    ESP_LOGI(TAG, "Netmask     : " IPSTR, IP2STR(&ip_info.netmask));
    esp_netif_get_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);
    ESP_LOGI(TAG, "Name Server1: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    esp_netif_get_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
    ESP_LOGI(TAG, "Name Server2: " IPSTR, IP2STR(&dns_info.ip.u_addr.ip4));
    ESP_LOGI(TAG, "~~~~~~~~~~~~~~");
}

static esp_err_t wait_for_event(const EventBits_t event_bit, const uint32_t timeout)
{
    EventBits_t bits = xEventGroupWaitBits(s_event_group,
                                           event_bit,
                                           pdTRUE,
                                           pdTRUE,
                                           pdMS_TO_TICKS(timeout));

    if (bits & event_bit) {
        return ESP_OK;
    }

    ESP_LOGE(TAG, "%s: unknown event (0x%" PRIX32 ") or timeout!", __func__, bits);

    return ESP_FAIL;
}

static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    const wifi_event_t event_type = (wifi_event_t)(event_id);

    ESP_LOGI(TAG, "%s:%d Event ID %" PRIi32, __func__, __LINE__, event_id);

    switch (event_type) {
    case WIFI_EVENT_SCAN_DONE:
        ESP_LOGI(TAG, "%s:%d WIFI_EVENT_SCAN_DONE", __func__, __LINE__);
        break;
    case WIFI_EVENT_STA_START:
    case WIFI_EVENT_AP_START:
        ESP_LOGI(TAG, "%s:%d WIFI_EVENT_START", __func__, __LINE__);
        xEventGroupSetBits(s_event_group, WIFI_STARTED_BIT);
        break;
    case WIFI_EVENT_STA_STOP:
    case WIFI_EVENT_AP_STOP:
        ESP_LOGI(TAG, "%s:%d WIFI_EVENT_STOP", __func__, __LINE__);
        break;
    case WIFI_EVENT_STA_CONNECTED:
    case WIFI_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "%s:%d WIFI_EVENT_CONNECTED", __func__, __LINE__);
        break;
    case WIFI_EVENT_STA_DISCONNECTED:
    case WIFI_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "%s:%d WIFI_EVENT_DISCONNECTED", __func__, __LINE__);
        xEventGroupSetBits(s_event_group, WIFI_DISCONNECTED_BIT);
        break;
    default:
        ESP_LOGW(TAG, "%s:%d Default switch case (%" PRIi32 ")", __func__, __LINE__, event_id);
        break;
    }
}

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    const wifi_event_t event_type = (wifi_event_t)(event_id);

    ESP_LOGI(TAG, "%s:%d Event ID %" PRIi32, __func__, __LINE__, event_id);

    switch (event_type) {
    case IP_EVENT_STA_GOT_IP: {
        ESP_LOGI(TAG, "GOT ip event!!!");
        net_utils_print_ip_info(event_data, "Wifi Connect to the Access Point");
        xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "%s:%d CONNECTED!", __func__, __LINE__);
        break;
    }
    case IP_EVENT_STA_LOST_IP:
        ESP_LOGI(TAG, "%s:%d IP_LOST.WAITING_FOR_NEW_IP", __func__, __LINE__);
        break;
    case IP_EVENT_GOT_IP6: {
        ip_event_got_ip6_t *event = (ip_event_got_ip6_t *)event_data;
        ESP_LOGI(TAG, "Got IPv6 address " IPV6STR, IPV62STR(event->ip6_info.ip));
        xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "%s:%d CONNECTED!", __func__, __LINE__);
        break;
    }
    case IP_EVENT_AP_STAIPASSIGNED:
        xEventGroupSetBits(s_event_group, WIFI_CONNECTED_BIT);
        ESP_LOGI(TAG, "%s:%d IP assigned to a connected station", __func__, __LINE__);
        break;
    default:
        ESP_LOGW(TAG, "%s:%d Default switch case (%" PRIi32 ")", __func__, __LINE__, event_id);
        break;
    }
}

static void prov_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    const wifi_event_t event_type = (wifi_event_t)(event_id);

    ESP_LOGI(TAG, "%s:%d Event ID %" PRIi32, __func__, __LINE__, event_id);

    switch (event_type) {
    case WIFI_PROV_INIT:
        ESP_LOGI(TAG, "%s:%d WIFI_PROV_INIT", __func__, __LINE__);
        break;
    case WIFI_PROV_START:
        ESP_LOGI(TAG, "%s:%d WIFI_PROV_START", __func__, __LINE__);
        break;
    case WIFI_PROV_CRED_RECV: {
        wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
        ESP_LOGI(TAG, "Received Wi-Fi credentials"
                 "\n\tSSID     : %s\n\tPassword : %s",
                 (const char *)wifi_sta_cfg->ssid,
                 (const char *)wifi_sta_cfg->password);
        xEventGroupSetBits(s_event_group, WIFI_CRED_RECV_BIT);
        break;
    }
    case WIFI_PROV_CRED_FAIL: {
        wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
        ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                 "\n\tPlease reset to factory and retry provisioning",
                 (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                 "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
        xEventGroupSetBits(s_event_group, WIFI_CRED_FAIL_BIT);
        break;
    }
    case WIFI_PROV_CRED_SUCCESS:
        ESP_LOGI(TAG, "%s:%d WIFI_PROV_CRED_SUCCESS", __func__, __LINE__);
        xEventGroupSetBits(s_event_group, WIFI_PROV_DONE_BIT);
        ESP_LOGI(TAG, "Provisioning successful");
        break;
    case WIFI_PROV_END:
        ESP_LOGI(TAG, "%s:%d WIFI_PROV_END", __func__, __LINE__);
        wifi_prov_mgr_deinit();
        break;
    case WIFI_PROV_DEINIT:
        ESP_LOGI(TAG, "%s:%d WIFI_PROV_DEINIT_BIT", __func__, __LINE__);
        xEventGroupSetBits(s_event_group, WIFI_PROV_DEINIT_BIT);
        break;
    default:
        ESP_LOGW(TAG, "%s:%d Default switch case (%" PRIi32 ")", __func__, __LINE__, event_id);
        break;
    }
}

static esp_err_t register_default_events(esp_netif_t *netif)
{
    esp_err_t status = esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, netif);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d wifi event register error! (%s)", __func__, __LINE__, esp_err_to_name(status));
        return ESP_FAIL;
    }
    status = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, netif);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d ip event register error! (%s)", __func__, __LINE__, esp_err_to_name(status));
        return ESP_FAIL;
    }

    return ESP_OK;
}

static void network_mngr_show_error(int try, int max_try)
{
    char msg[128] = {0};
    sprintf(msg, "Waiting for the network connection... retry (%d/%d)", try, max_try);
    ui_show_info_screen(msg);
}

esp_err_t network_mngr_init(void)
{
    if (!s_event_group) {
        s_event_group = xEventGroupCreate();
        if (!s_event_group) {
            ESP_LOGE(TAG, "%s:%d event group create error!", __func__, __LINE__);
            return ESP_FAIL;
        }
    }
    return register_default_events(NULL);
}

esp_err_t network_mngr_init_ap(const char *ssid, const char *pass)
{
    esp_err_t status = ESP_FAIL;

    if (!ssid || strlen(ssid) == 0 || !pass) {
        ESP_LOGE(TAG, "%s:%d init param error!", __func__, __LINE__);
        return ESP_FAIL;
    }

    s_ap_netif = esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    status = esp_wifi_init(&cfg);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d esp_wifi_init error! (%s)", __func__, __LINE__, esp_err_to_name(status));
        return ESP_FAIL;
    }

    status = esp_wifi_set_mode(WIFI_MODE_AP);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d wifi set mode error! (%s)", __func__, __LINE__, esp_err_to_name(status));
        return ESP_FAIL;
    }

    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.ap.ssid, ssid, sizeof(wifi_config.ap.ssid));
    memcpy(wifi_config.ap.password, pass, sizeof(wifi_config.ap.password));
    wifi_config.ap.ssid_len = strlen(ssid);
    wifi_config.ap.max_connection = 3;
    wifi_config.ap.authmode = strlen(pass) == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK;

    status = esp_wifi_set_config(WIFI_IF_AP, &wifi_config);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d wifi set config error!", __func__, __LINE__);
        return ESP_FAIL;
    }

    status = esp_wifi_start();
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d wifi start error!", __func__, __LINE__);
        return ESP_FAIL;
    }

    if (wait_for_event(WIFI_STARTED_BIT, WIFI_START_TIMEOUT) == ESP_OK) {
        ESP_LOGI(TAG, "%s:%d Wifi started successfully", __func__, __LINE__);
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t network_mngr_connect_ap(unsigned int max_retry)
{
    network_mngr_states_t state = network_mngr_state(portMAX_DELAY);
    if (state == NETWORK_MNGR_CONNECTED) {
        ESP_LOGI(TAG, "wifi ap client is connected");
        return ESP_OK;
    }
    return ESP_FAIL;
}

esp_err_t network_mngr_init_sta(const char *ssid, const char *pass)
{
    esp_err_t status = ESP_FAIL;

    if (!ssid || strlen(ssid) == 0 || !pass) {
        ESP_LOGE(TAG, "%s:%d ssid or password can't be null!", __func__, __LINE__);
        return ESP_FAIL;
    }

    s_sta_netif = esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    status = esp_wifi_init(&cfg);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d esp_wifi_init error! (%s)", __func__, __LINE__, esp_err_to_name(status));
        goto err;
    }

    status = esp_wifi_set_mode(WIFI_MODE_STA);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d wifi set mode error! (%s)", __func__, __LINE__, esp_err_to_name(status));
        goto err;
    }

    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    memcpy(wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = strlen(pass) == 0 ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK;
    status = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d wifi set config error! (%s)", __func__, __LINE__, esp_err_to_name(status));
        goto err;
    }

    status = esp_wifi_start();
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d wifi start error!", __func__, __LINE__);
        goto err;
    }

    if (wait_for_event(WIFI_STARTED_BIT, WIFI_START_TIMEOUT) == ESP_OK) {
        ESP_LOGI(TAG, "%s:%d Wifi started successfully", __func__, __LINE__);
        return ESP_OK;
    }

err:
    esp_netif_destroy(s_sta_netif);
    s_sta_netif = NULL;
    return ESP_FAIL;
}

esp_err_t network_mngr_connect_sta(unsigned int max_retry)
{
    for (size_t i = 0; i < max_retry; i++) {

        esp_err_t status = esp_wifi_connect();

        if (status != ESP_OK) {
            ESP_LOGE(TAG, "%s:%d wifi connect failed! (%s)", __func__, __LINE__, esp_err_to_name(status));
            network_mngr_show_error(i + 1, max_retry);
            vTaskDelay(pdMS_TO_TICKS(3000));
            continue;
        }

        network_mngr_states_t state = network_mngr_state(portMAX_DELAY);
        if (state == NETWORK_MNGR_CONNECTED) {
            ESP_LOGI(TAG, "wifi sta is connected");
            return ESP_OK;
        } else if (state == NETWORK_MNGR_DISCONNECTED) {
            network_mngr_show_error(i + 1, max_retry);
        }
    }

    return ESP_FAIL;
}

esp_err_t network_mngr_init_prov(const char *ssid, const char *pass, const void *http_handle)
{
    if (!ssid || strlen(ssid) == 0) {
        ESP_LOGE(TAG, "%s:%d ssid can't be null!", __func__, __LINE__);
        return ESP_FAIL;
    }

    s_ap_netif = esp_netif_create_default_wifi_ap();
    s_sta_netif = esp_netif_create_default_wifi_sta();

    esp_err_t status = esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &prov_event_handler, NULL);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d prov event register error! (%s)", __func__, __LINE__, esp_err_to_name(status));
        return ESP_FAIL;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    status = esp_wifi_init(&cfg);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d esp_wifi_init error! (%s)", __func__, __LINE__, esp_err_to_name(status));
        return ESP_FAIL;
    }

    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
    };
    status = wifi_prov_mgr_init(config);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "%s:%d init error! (%s)", __func__, __LINE__, esp_err_to_name(status));
        return ESP_FAIL;
    }

    wifi_prov_scheme_softap_set_httpd_handle((void *)http_handle);

    if (wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, (const void *)"abcd1234", ssid, pass) != ESP_OK) {
        ESP_LOGE(TAG, "wifi mngr prov start failed!");
        return ESP_FAIL;
    }

    if (wait_for_event(WIFI_STARTED_BIT, WIFI_START_TIMEOUT) == ESP_OK) {
        ESP_LOGI(TAG, "%s:%d Wifi prov started successfully", __func__, __LINE__);
        return ESP_OK;
    }

    return ESP_FAIL;
}

esp_err_t network_mngr_connect_prov(unsigned int max_retry)
{
    EventBits_t bits = xEventGroupWaitBits(s_event_group,
                                           WIFI_PROV_DONE_BIT | WIFI_CRED_FAIL_BIT,
                                           pdTRUE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_PROV_DONE_BIT) {
        if (wait_for_event(WIFI_PROV_DEINIT_BIT, WIFI_DEINIT_TIMEOUT) == ESP_OK) {
            ESP_LOGI(TAG, "%s:%d Wifi prov deinit successfully", __func__, __LINE__);
            return NETWORK_MNGR_CONNECTED;
        }
    } else if (bits != 0) {
        ESP_LOGE(TAG, "UNEXPECTED EVENT (0x%" PRIX32 ")", bits);
    }
    return NETWORK_MNGR_ERROR;
}

network_mngr_states_t network_mngr_state(unsigned int timeout)
{
    EventBits_t bits = xEventGroupWaitBits(s_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT,
                                           pdTRUE,
                                           pdFALSE,
                                           timeout);

    if (bits & WIFI_CONNECTED_BIT) {
        return NETWORK_MNGR_CONNECTED;
    } else if (bits & WIFI_DISCONNECTED_BIT) {
        ESP_LOGE(TAG, "Wifi is disconnected!");
        return NETWORK_MNGR_DISCONNECTED;
    } else {
        if (bits != 0) {
            ESP_LOGE(TAG, "UNEXPECTED EVENT (%" PRIX32 ")", bits);
            return NETWORK_MNGR_ERROR;
        }
    }

    return NETWORK_MNGR_STATUS_NOT_CHANGED;
}

esp_err_t network_mngr_get_sta_credentials(char **ssid, char **pass)
{
    wifi_config_t conf;

    if (esp_wifi_get_config(WIFI_IF_STA, &conf) == ESP_OK) {
        *ssid = strdup((const char *)conf.sta.ssid);
        *pass = strdup((const char *)conf.sta.password);
        return ESP_OK;
    }
    return ESP_FAIL;
}

static esp_err_t network_mngr_get_ip(esp_netif_t *netif, char **ip)
{
    esp_netif_ip_info_t ip_info;

    esp_netif_get_ip_info(netif, &ip_info);

    char ip_addr[16] = {0};
    snprintf(ip_addr, sizeof(ip_addr), IPSTR, IP2STR(&ip_info.ip));

    *ip = strdup(ip_addr);

    return ESP_OK;
}

esp_err_t network_mngr_get_sta_ip(char **ip)
{
    return network_mngr_get_ip(s_sta_netif, ip);
}

esp_err_t network_mngr_get_ap_ip(char **ip)
{
    return network_mngr_get_ip(s_ap_netif, ip);
}
