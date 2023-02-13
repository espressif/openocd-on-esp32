#ifndef __PROVISIONING_H__
#define __PROVISIONING_H__

#define OOCD_START_FLAG         (1 << 0)
extern EventGroupHandle_t s_event_group;

esp_err_t provisioning_init(void);
void reset_connection_config(void);

#endif
