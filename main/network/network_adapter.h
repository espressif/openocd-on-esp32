#pragma once

#include <esp_err.h>

#ifdef __cplusplus
extern "C" {
#endif

struct network;
struct network_init_config;

struct adapter_private {
    char *ssid;
    char *pass;
    char *my_ip;
    unsigned char max_retry;
};

struct network_adapter {
    const char *name; /* wifista, wifiprov, wifiap */
    esp_err_t (*init)(struct network *net);
    esp_err_t (*create)(struct network *net);
    esp_err_t (*poll)(struct network *net);
    esp_err_t (*connect)(struct network *net);
    esp_err_t (*disconnect)(struct network *net);
    esp_err_t (*deinit)(struct network *net);
    esp_err_t (*configure)(struct network *net, struct network_init_config *config);
};

extern struct network_adapter wifista_adapter;
extern struct network_adapter wifiap_adapter;
extern struct network_adapter wifiprov_adapter;

#ifdef __cplusplus
}
#endif
