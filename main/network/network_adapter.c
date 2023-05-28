/*
    Generic wifi adapter layer. Supports ap, sta mode and provisioning
*/
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"

#include "network_adapter.h"
#include "network_mngr.h"
#include "network.h"
#include "ui.h"

#define ADAPTER_MAX_RETRY_CNT   10

static const char *TAG = "network-adapter";

static esp_err_t adapter_create(struct network *network)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);

    struct adapter_private *adap_prv = calloc(1, sizeof(*adap_prv));
    if (!adap_prv) {
        return ESP_FAIL;
    }
    network->private_config = adap_prv;

    return ESP_OK;
}

static esp_err_t adapter_wifi_configure(struct network *network, struct network_init_config *config)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);

    struct adapter_private *adap_prv = network->private_config;

    adap_prv->ssid = strdup(config->ssid);
    adap_prv->pass = strdup(config->pass);
    adap_prv->max_retry = ADAPTER_MAX_RETRY_CNT;

    if (!adap_prv->ssid) {
        ESP_LOGE(TAG, "ssid can't be null!");
        goto err;
    }

    ESP_LOGI(TAG, "ssid (%s)", adap_prv->ssid);
    ESP_LOGI(TAG, "pass (%s)", adap_prv->pass);

    return ESP_OK;

err:
    free(adap_prv->pass);
    free(adap_prv->ssid);
    return ESP_FAIL;
}

static esp_err_t adapter_poll(struct network *network)
{
    network_mngr_states_t net_state = network_mngr_state(0);

    ESP_LOGI(TAG, "%s state(%d)", __FUNCTION__, net_state);

    switch (net_state) {
    case NETWORK_MNGR_CONNECTED:
        network->state = NETWORK_STATE_CONNECTED;
        ESP_LOGI(TAG, "%s state is connected", network->name);
        break;
    case NETWORK_MNGR_DISCONNECTED:
        network->state = NETWORK_STATE_DISCONNECTED;
        ESP_LOGI(TAG, "%s state is disconnected", network->name);
        break;
    case NETWORK_MNGR_STATUS_NOT_CHANGED:
        ESP_LOGI(TAG, "%s state is not changed (%d)", network->name, network->state);
        break;
    default:
        network->state = NETWORK_STATE_UNKNOWN;
        ESP_LOGI(TAG, "%s state is unknown", network->name);
        break;
    }

    return ESP_OK;
}

static esp_err_t wifi_ap_init(struct network *network)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);

    struct adapter_private *adap_prv = network->private_config;

    if (network_mngr_init() != ESP_OK) {
        ESP_LOGE(TAG, "network mngr init failed!");
        return ESP_FAIL;
    }

    if (network_mngr_init_ap(adap_prv->ssid, adap_prv->pass) != ESP_OK) {
        ESP_LOGE(TAG, "wifi-ap init failed!");
        return ESP_FAIL;
    }

    network->state = NETWORK_STATE_AP_INITED;

    return ESP_OK;
}

static esp_err_t wifi_ap_connect(struct network *network)
{
    struct adapter_private *adap_prv = network->private_config;

    if (network->state == NETWORK_STATE_AP_INITED) {
        free(adap_prv->my_ip);
        network_mngr_get_ap_ip(&adap_prv->my_ip);
    }

    return ESP_OK;
}

static esp_err_t wifi_sta_init(struct network *network)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);

    struct adapter_private *adap_prv = network->private_config;

    if (network_mngr_init() != ESP_OK) {
        ESP_LOGE(TAG, "network mngr init failed!");
        return ESP_FAIL;
    }

    if (network_mngr_init_sta(adap_prv->ssid, adap_prv->pass) != ESP_OK) {
        ESP_LOGE(TAG, "network mngr sta init failed!");
        return ESP_FAIL;
    }

    return ESP_OK;
}

static esp_err_t wifi_sta_connect(struct network *network)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);

    struct adapter_private *adap_prv = network->private_config;

    esp_err_t status = network_mngr_connect_sta(adap_prv->max_retry);

    if (status == ESP_OK) {
        free(adap_prv->my_ip);
        network_mngr_get_sta_ip(&adap_prv->my_ip);
    }

    return status;
}

static esp_err_t wifi_prov_init(struct network *network)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);

    struct adapter_private *adap_prv = network->private_config;

    if (network_mngr_init() != ESP_OK) {
        ESP_LOGE(TAG, "network mngr init failed!");
        return ESP_FAIL;
    }

    if (network_mngr_init_prov(adap_prv->ssid, NULL, network->http_handle) != ESP_OK) {
        ESP_LOGE(TAG, "network mngr prov init failed!");
        return ESP_FAIL;
    }

    network_mngr_get_ap_ip(&adap_prv->my_ip);
    ui_update_ip_ssid_info(adap_prv->my_ip, adap_prv->ssid);

    return ESP_OK;
}

static esp_err_t wifi_prov_connect(struct network *network)
{
    ESP_LOGI(TAG, "%s", __FUNCTION__);

    struct adapter_private *adap_prv = network->private_config;

    esp_err_t status = network_mngr_connect_prov(adap_prv->max_retry);

    if (status == ESP_OK) {
        free(adap_prv->ssid);
        free(adap_prv->pass);
        free(adap_prv->my_ip);
        network_mngr_get_sta_credentials(&adap_prv->ssid, &adap_prv->pass);
        network_mngr_get_sta_ip(&adap_prv->my_ip);
    }

    return status;
}

struct network_adapter wifiap_adapter = {
    .name = "wifiap",
    .create = adapter_create,
    .init = wifi_ap_init,
    .configure = adapter_wifi_configure,
    .connect = wifi_ap_connect,
    .poll = NULL,
    .disconnect = NULL,
    .deinit = NULL,
};

struct network_adapter wifista_adapter = {
    .name = "wifista",
    .create = adapter_create,
    .init = wifi_sta_init,
    .configure = adapter_wifi_configure,
    .connect = wifi_sta_connect,
    .poll = adapter_poll,
    .deinit = NULL,
    .disconnect = NULL,
};

struct network_adapter wifiprov_adapter = {
    .name = "wifiprov",
    .create = adapter_create,
    .init = wifi_prov_init,
    .configure = adapter_wifi_configure,
    .connect = wifi_prov_connect,
    .poll = adapter_poll,
    .deinit = NULL,
    .disconnect = NULL,
};
