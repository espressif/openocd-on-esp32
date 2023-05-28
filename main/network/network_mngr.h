#pragma once

typedef enum {
    NETWORK_MNGR_CONNECTED,
    NETWORK_MNGR_DISCONNECTED,
    NETWORK_MNGR_STATUS_NOT_CHANGED,
    NETWORK_MNGR_ERROR
} network_mngr_states_t;

esp_err_t network_mngr_init(void);
esp_err_t network_mngr_init_ap(const char *ssid, const char *pass);
esp_err_t network_mngr_connect_ap(unsigned int max_retry);
esp_err_t network_mngr_init_sta(const char *ssid, const char *pass);
esp_err_t network_mngr_connect_sta(unsigned int max_retry);
esp_err_t network_mngr_init_prov(const char *ssid, const char *pass, const void *http_handle);
esp_err_t network_mngr_connect_prov(unsigned int max_retry);
network_mngr_states_t network_mngr_state(unsigned int timeout);
esp_err_t network_mngr_get_sta_credentials(char **ssid, char **pass);
esp_err_t network_mngr_get_sta_ip(char **ip);
esp_err_t network_mngr_get_ap_ip(char **ip);
