#pragma once

#include "esp_err.h"

/* OpenOCD params */
#define OOCD_CFG_FILE_KEY           "file"
#define OOCD_RTOS_TYPE_KEY          "rtos"
#define OOCD_DUAL_CORE_KEY          "smp"
#define OOCD_FLASH_SUPPORT_KEY      "flash"
#define OOCD_INTERFACE_KEY          "interface"
#define OOCD_CMD_LINE_ARGS_KEY      "command"
#define OOCD_DBG_LEVEL_KEY          "debug"

/* Network params */
#define WIFI_SSID_KEY               "ssid"
#define WIFI_PASS_KEY               "pass"

#define CFG_FILE_PATH               "/data/target/"

esp_err_t storage_init_filesystem(void);
esp_err_t storage_write(const char *key, const char *value, size_t len);
esp_err_t storage_read(const char *key, char *value, size_t len);
esp_err_t storage_erase_key(const char *key);
size_t storage_get_value_length(const char *key);
bool storage_is_key_exist(const char *key);
esp_err_t storage_erase_all(void);
esp_err_t storage_alloc_and_read(char *key, char **value);
esp_err_t storage_update_target_struct(void);
esp_err_t storage_update_rtos_struct(void);
