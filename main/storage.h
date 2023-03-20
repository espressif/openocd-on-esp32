#ifndef __STORAGE_H__
#define __STORAGE_H__

#define OOCD_F_PARAM_KEY            "file"
#define OOCD_C_PARAM_KEY            "command"
#define OOCD_D_PARAM_KEY            "debug"
#define CONNECTION_TYPE_KEY         "connection"
#define DUAL_CORE_KEY               "dual_core"
#define FLASH_SUPPORT_KEY           "flash_support"
#define RTOS_KEY                    "rtos"
#define CONFIG_FILES_PATH           "/data/target/"
#define TARGET_PATH_PRELIM          "target/"

struct esp_oocd_config {
    bool dual_core;
    bool flash_support;
    int rtos;
    int target_index;
    int debug_level_index;
};
extern struct esp_oocd_config oocd_config;

esp_err_t storage_get_openocd_config(void);
esp_err_t storage_init_filesystem(void);
esp_err_t storage_nvs_write(const char *key, const char *value, size_t size);
esp_err_t storage_nvs_read(const char *key, char *value, size_t size);
esp_err_t storage_nvs_erase_key(const char *key);
esp_err_t storage_nvs_erase_everything(void);
esp_err_t storage_nvs_read_param(char *key, char **value);
esp_err_t storage_save_credentials(const char *ssid, const char *pass);
esp_err_t storage_get_target_path(const char *target_name, char **path);
esp_err_t storage_get_target_name_from_index(int index, char **target_name);
size_t storage_nvs_get_value_length(const char *key);
bool storage_nvs_is_key_exist(const char *key);
char *storage_get_config_files(char *path, bool filter, int *file_count);
int storage_get_file_size(const char *file_name);
bool storage_is_target_single_core(char *target_name);
char *storage_get_target_names(bool update);

#endif
