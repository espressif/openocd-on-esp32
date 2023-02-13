#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#define OOCD_START_FLAG         (1 << 0)
extern EventGroupHandle_t s_event_group;

esp_err_t web_server_init(void);
void reset_connection_config(void);

#endif
