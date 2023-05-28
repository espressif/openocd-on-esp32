#pragma once

#include <stdbool.h>
#include "network_adapter.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIFI_AP_SSSID      "esp-openocd"
#define WIFI_AP_PASSW      ""

typedef enum {
    NETWORK_STATE_UNKNOWN,
    NETWORK_STATE_AP_INITED,
    NETWORK_STATE_CONNECTED,
    NETWORK_STATE_DISCONNECTED,
} network_states_t;

struct network {
    struct network_adapter *adapter;   /* wifi-sta, wifi-ap, eth, gsm */
    char *name;
    bool init;
    void *http_handle;
    network_states_t state;
    void *private_config;   /* pointer to specific config data ie.apn for gsm */
};

struct network_init_config {
    const char *adapter_name;
    const char *ssid;
    const char *pass;
    void *http_handle;
};

esp_err_t network_start(struct network_init_config *config);
esp_err_t network_get_sta_credentials(char *ssid, char *pass);
esp_err_t network_get_my_ip(char *ip);

#ifdef __cplusplus
}
#endif
