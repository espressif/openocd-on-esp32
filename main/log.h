#ifndef __LOG_H__
#define __LOG_H__

#include "web_server.h"

#if CONFIG_DEBUG_BUILD
#define OOCD_RETURN_ON_ERROR(x, fmt, ...)   ESP_RETURN_ON_ERROR((x), "%s:%d: "#fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define OOCD_LOGE(tag, fmt, ...)            ESP_LOGE(tag, "%s:%d: " fmt, __FILE__, __LINE__, ##__VA_ARGS__)
#define WEB_SERVER_ALERT(req, msg)          web_server_alert(req, msg, __FILE__, __LINE__)
#define UI_SHOW_ERROR(msg)                  ui_show_error(msg, __FILE__, __LINE__)
#else
#define OOCD_RETURN_ON_ERROR(x, fmt, ...)   ESP_RETURN_ON_ERROR((x), fmt, ##__VA_ARGS__)
#define OOCD_LOGE(tag, fmt, ...)            ESP_LOGE(tag, fmt, ##__VA_ARGS__)
#define WEB_SERVER_ALERT(req, msg)          web_server_alert(req, msg, NULL, 0)
#define UI_SHOW_ERROR(msg)                  ui_show_error(msg, NULL, 0)
#endif

#if CONFIG_UI_ENABLE
#define ESP_OOCD_ERROR_CHECK(x, req, msg) do {                                                             \
        esp_err_t err_rc_ = (x);                                                                           \
        if (unlikely(err_rc_ != ESP_OK)) {                                                                 \
            OOCD_LOGE(TAG, msg);                                                                           \
            if (req != NULL) {                                                                             \
                WEB_SERVER_ALERT(req, msg);                                                                \
            }                                                                                              \
            UI_SHOW_ERROR(msg);                                                                            \
            abort();                                                                                       \
        }                                                                                                  \
    } while(0)
#else
#define ESP_OOCD_ERROR_CHECK(x, req, msg) do {                                                             \
        esp_err_t err_rc_ = (x);                                                                           \
        if (unlikely(err_rc_ != ESP_OK)) {                                                                 \
            OOCD_LOGE(TAG, msg);                                                                           \
            if (req != NULL) {                                                                             \
                WEB_SERVER_ALERT(req, msg);                                                                \
            }                                                                                              \
            abort();                                                                                       \
        }                                                                                                  \
    } while(0)
#endif

#endif
