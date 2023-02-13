#ifndef __MANUAL_CONFIG_H__
#define __MANUAL_CONFIG_H__

#define OOCD_START_FLAG         (1 << 0)
extern EventGroupHandle_t s_event_group;

esp_err_t manual_config_init(void);

#endif
