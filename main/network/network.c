#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "network.h"
#include "network_adapter.h"

static const char *TAG = "network";

static struct network *s_network = NULL;

static struct network_adapter *network_adapters[] = {
    &wifista_adapter,
    &wifiap_adapter,
    &wifiprov_adapter,
    NULL
};

static esp_err_t network_create(struct network_init_config *config)
{
    if (!config->adapter_name) {
        ESP_LOGE(TAG, "Network adapter name is NULL!");
        return ESP_ERR_INVALID_ARG;
    }

    if (s_network) {
        ESP_LOGW(TAG, "network has already created..");
        return ESP_OK;
    }

    int8_t network_adapter_index = -1;
    for (size_t i = 0; network_adapters[i]; i++) {
        if (!strcmp(config->adapter_name, network_adapters[i]->name)) {
            network_adapter_index = i;
            break;
        }
    }
    if (network_adapter_index < 0) {
        ESP_LOGE(TAG, "Network adapter is not found!");
        return ESP_FAIL;
    }

    struct network *network = (struct network *)calloc(1, sizeof(*network));
    if (!network) {
        ESP_LOGE(TAG, "Couldn't allocate a memory for network!");
        return ESP_FAIL;
    }

    network->adapter = network_adapters[network_adapter_index];
    network->state = NETWORK_STATE_UNKNOWN;

    if (network->adapter->create) {
        esp_err_t err = network->adapter->create(network);
        if (err != ESP_OK) {
            goto _exit_failure;
        }
        ESP_LOGI(TAG, "network type (%s) created", network->adapter->name);
    }
    network->name = strdup(network_adapters[network_adapter_index]->name);
    network->http_handle = config->http_handle;

    /* configure connection specific params */
    if (network->adapter->configure) {
        if (network->adapter->configure(network, config) != ESP_OK) {
            goto _exit_failure;
        }
    }

    s_network = network;

    return ESP_OK;

_exit_failure:
    free(network->name);
    free(network->private_config);
    free(network);

    s_network = NULL;

    ESP_LOGE(TAG, "Unable to create a network!");

    return ESP_FAIL;
}

static esp_err_t network_init(void)
{
    if (!s_network) {
        return ESP_FAIL;
    }

    if (s_network->init) {
        ESP_LOGW(TAG, "network has already initalized..");
        return ESP_OK;
    }

    assert(s_network->adapter->init);

    esp_err_t status = s_network->adapter->init(s_network);

    if (status == ESP_OK) {
        s_network->init = true;
    }

    return status;
}

static esp_err_t network_connect(void)
{
    if (!s_network) {
        return ESP_FAIL;
    }

    if (s_network->state == NETWORK_STATE_CONNECTED) {
        ESP_LOGW(TAG, "network is in connected state");
        return ESP_OK;
    }

    assert(s_network->adapter->connect);

    esp_err_t status = s_network->adapter->connect(s_network);

    if (status == ESP_OK) {
        s_network->state = NETWORK_STATE_CONNECTED;
    }

    return status;
}

static esp_err_t __attribute__((unused)) network_disconnect(void)
{
    if (!s_network) {
        return ESP_FAIL;
    }

    if (s_network->state == NETWORK_STATE_DISCONNECTED) {
        ESP_LOGW(TAG, "network is in disconnected state");
        return ESP_OK;
    }

    assert(s_network->adapter->disconnect);

    return s_network->adapter->disconnect(s_network);
}

static inline bool network_is_connected(void)
{
    return s_network && (s_network->state == NETWORK_STATE_CONNECTED);
}

static inline network_states_t network_state(void)
{
    if (!s_network) {
        return NETWORK_STATE_UNKNOWN;
    }

    return s_network->state;
}

static esp_err_t __attribute__((unused)) network_poll(void)
{
    if (!s_network || !s_network->adapter->poll) {
        return ESP_FAIL;
    }

    s_network->adapter->poll(s_network);

    return ESP_OK;
}

esp_err_t network_start(struct network_init_config *config)
{
    esp_err_t status = network_create(config);
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "network couldn't created!");
        return ESP_FAIL;
    }

    status = network_init();
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "network couldn't started!");
        return ESP_FAIL;
    }

    status = network_connect();
    if (status != ESP_OK) {
        ESP_LOGE(TAG, "Failed to connect.");
        return ESP_FAIL;
    }

    return network_is_connected() ? ESP_OK : ESP_FAIL;
}

esp_err_t network_get_sta_credentials(char *ssid, char *pass)
{
    struct adapter_private *adap_prv = s_network->private_config;

    strcpy(ssid, adap_prv->ssid);
    strcpy(pass, adap_prv->pass);
    return ESP_OK;
}

esp_err_t network_get_my_ip(char *ip)
{
    struct adapter_private *adap_prv = s_network->private_config;

    strcpy(ip, adap_prv->my_ip);

    return ESP_OK;
}
