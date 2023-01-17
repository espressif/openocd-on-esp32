#ifndef __STORAGE_H__
#define __STORAGE_H__

#define OOCD_F_PARAM_KEY            "file"
#define OOCD_C_PARAM_KEY            "command"
#define OOCD_D_PARAM_KEY            "debug"

esp_err_t storage_init_filesystem(void);
esp_err_t storage_nvs_write(const char *key, const char *value, size_t size);
esp_err_t storage_nvs_read(const char *key, char *value, size_t size);
esp_err_t storage_nvs_erase_key(const char *key);
size_t storage_nvs_get_value_length(const char *key);
bool storage_nvs_is_key_exist(const char *key);

#endif
