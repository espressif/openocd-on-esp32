#ifndef __NETWORKING_H__
#define __NETWORKING_H__

esp_err_t networking_wait_for_connection(TickType_t timeout);
esp_err_t networking_start_server(void);
esp_err_t networking_init_sta_mode(void);
esp_err_t networking_init_ap_mode(void);
esp_err_t networking_set_wifi_mode(char new_mode);
esp_err_t networking_get_wifi_mode(char *mode);
esp_err_t networking_init_wifi_mode(void);
esp_err_t networking_init_connection(void);

#endif
