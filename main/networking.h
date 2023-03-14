#ifndef __NETWORKING_H__
#define __NETWORKING_H__

esp_err_t networking_init(void);
void reset_connection_config(void);

#define SSID_LENGTH                 (32 + 1)
#define PASSWORD_LENGTH             (64 + 1)
#define SSID_KEY                    "ssid"
#define PASSWORD_KEY                "pass"
#define WIFI_MODE_KEY               "wifi_mode"
#define WIFI_AP_MODE                '0'
#define WIFI_STA_MODE               '1'

#endif
