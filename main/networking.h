#ifndef __NETWORKING_H__
#define __NETWORKING_H__

#include "web_server.h"

esp_err_t networking_init(void);
void networking_reset_connection_config(void);

#define SSID_LENGTH                 (32 + 1)
#define PASSWORD_LENGTH             (64 + 1)
#define SSID_KEY                    "ssid"
#define PASSWORD_KEY                "pass"
#define WIFI_MODE_KEY               "wifi_mode"
#define WIFI_AP_MODE                '0'
#define WIFI_STA_MODE               '1'

#if CONFIG_UI_ENABLE
#define ESP_OOCD_ERROR_CHECK(x, log_tag, req) do {                                                         \
        esp_err_t err_rc_ = (x);                                                                           \
        if (unlikely(err_rc_ != ESP_OK)) {                                                                 \
            ESP_LOGE(log_tag, "Failed at %s(%s:%d)", __FILE__, __FUNCTION__, __LINE__);                    \
            if (req != NULL) {                                                                             \
                web_server_alert(req, __FILE__, __LINE__);                                                 \
            }                                                                                              \
            ui_show_error(__FILE__, __LINE__);                                                             \
            abort();                                                                                       \
        }                                                                                                  \
    } while(0)
#else
#define ESP_OOCD_ERROR_CHECK(x, log_tag, req) do {                                                         \
        esp_err_t err_rc_ = (x);                                                                           \
        if (unlikely(err_rc_ != ESP_OK)) {                                                                 \
            ESP_LOGE(log_tag, "Failed at %s(%s:%d)", __FILE__, __FUNCTION__, __LINE__);                    \
            if (req != NULL) {                                                                             \
                web_server_alert(req, __FILE__, __LINE__);                                                 \
            }                                                                                              \
            abort();                                                                                       \
        }                                                                                                  \
    } while(0)

#endif

#endif
