#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_check.h"
#include "esp_vfs_fat.h"

#include "storage.h"
#include "networking.h"

#define STORAGE_NAMESPACE           "NVS_DATA"

static const char *TAG = "storage";

struct esp_oocd_config oocd_config;
int files_count = 0;

esp_err_t storage_init_filesystem(void)
{
    ESP_LOGI(TAG, "Mounting FAT filesystem");
    // To mount device we need name of device partition, define base_path
    // and allow format partition in case if it is new one and was not formatted before
    const esp_vfs_fat_mount_config_t mount_config = {
        .max_files = 5,
        .format_if_mount_failed = true
    };

    static wl_handle_t wl_handle;
    esp_err_t ret = esp_vfs_fat_spiflash_mount_rw_wl("/data", NULL, &mount_config, &wl_handle);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find FATFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize FATFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_vfs_fat_info("/data", &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
        return ret;
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    return ESP_OK;
}

esp_err_t storage_nvs_write(const char *key, const char *value, size_t size)
{
    nvs_handle_t my_handle;

    ESP_RETURN_ON_ERROR((!key || !value), TAG, "Key or Value is NULL");
    ESP_RETURN_ON_ERROR(nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle), TAG, "Failed to open namespace");
    ESP_RETURN_ON_ERROR(nvs_set_blob(my_handle, key, value, size), TAG, "Failed to set blob \"%s\"", key);
    ESP_RETURN_ON_ERROR(nvs_commit(my_handle), TAG, "Failed save changes");
    nvs_close(my_handle);

    return ESP_OK;
}

esp_err_t storage_nvs_read(const char *key, char *value, size_t size)
{
    nvs_handle_t my_handle;
    size_t n_bytes = size;

    ESP_RETURN_ON_ERROR(!key, TAG, "Key is NULL");
    ESP_RETURN_ON_ERROR(nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle), TAG, "Failed to open namespace");
    ESP_RETURN_ON_ERROR(nvs_get_blob(my_handle, key, value, &n_bytes), TAG, "Failed to get blob \"%s\"", key);
    nvs_close(my_handle);

    if (n_bytes != size) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

int storage_nvs_erase_key(const char *key)
{
    nvs_handle_t my_handle;

    ESP_RETURN_ON_ERROR(!key, TAG, "Key is NULL");
    ESP_RETURN_ON_ERROR(nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle), TAG, "Failed to open namespace");
    ESP_RETURN_ON_ERROR(nvs_erase_key(my_handle, key), TAG, "Failed to erase \"%s\"", key);
    nvs_close(my_handle);

    return ESP_OK;
}

size_t storage_nvs_get_value_length(const char *key)
{
    nvs_handle_t my_handle;
    size_t length;

    if (!key) {
        ESP_LOGE(TAG, "Key is NULL");
        return 0;
    }

    esp_err_t ret = nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &my_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open namespace");
        return 0;
    }

    ret = nvs_get_blob(my_handle, key, NULL, &length);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get blob \"%s\"", key);
        return 0;
    }

    nvs_close(my_handle);

    return length;
}

bool storage_nvs_is_key_exist(const char *key)
{
    return storage_nvs_get_value_length(key) > 0;
}

esp_err_t storage_nvs_erase_everything(void)
{
    return nvs_flash_erase();
}

static bool will_be_filtered(const char *file_name)
{
    const char *filtered_files[] = {"esp_common.cfg", "xtensa-core-esp32"};

    for (int i = 0; i < sizeof(filtered_files) / sizeof(char *); i++) {
        if (strstr(file_name, filtered_files[i]) != NULL) {
            return true;
        }
    }
    return false;
}

char *storage_get_config_files(char *path, bool filter, int *file_count)
{
    if (!path) {
        ESP_LOGE(TAG, "Path value is NULL");
        return NULL;
    }

    DIR *d;
    struct dirent *dir;
    char *str = NULL;

    d = opendir(path);
    if (!d) {
        ESP_LOGE(TAG, "Could not open the directory");
        return NULL;
    }

    while ((dir = readdir(d)) != NULL) {
        if (dir->d_type == DT_DIR) {
            continue;
        }
        if (str == NULL) {
            str = (char *) calloc(sizeof(dir->d_name), sizeof(char));
            if (!str) {
                ESP_LOGE(TAG, "Allocation error during fetch config file names");
                return NULL;
            }
            strcpy(str, dir->d_name);
            (*file_count)++;
        } else {
            if (!filter || !will_be_filtered(dir->d_name)) {
                (*file_count)++;
                int current_length = strlen(dir->d_name) + strlen(str) + 1 + 1;
                if (current_length > sizeof(dir->d_name)) {
                    char *tmp_str = (char *) calloc(current_length, sizeof(char));
                    if (!tmp_str) {
                        ESP_LOGE(TAG, "Allocation error");
                        free(str);
                        closedir(d);
                        return NULL;
                    }
                    strcpy(tmp_str, str);
                    free(str);
                    str = tmp_str;
                }
                str = strcat(str, "\n");
                str = strcat(str, dir->d_name);
            }
        }
    }
    closedir(d);

    return str;
}

esp_err_t storage_nvs_read_param(char *key, char **value)
{
    if (!key || !value) {
        return ESP_ERR_INVALID_ARG;
    }

    /* First read length of the config string */
    size_t len = storage_nvs_get_value_length(key);
    if (len == 0) {
        return ESP_FAIL;
    }

    /* allocate memory for the null terminated string */
    char *ptr = (char *)calloc(len + 1, sizeof(char));
    if (!ptr) {
        ESP_LOGE(TAG, "Failed to allocate memory");
        return ESP_FAIL;
    }

    esp_err_t ret_nvs = storage_nvs_read(key, ptr, len);
    if (ret_nvs != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get %s command", key);
        free(ptr);
        return ESP_FAIL;
    }

    *value = ptr;

    return ESP_OK;
}

int storage_get_file_size(const char *file_name)
{
    int size = -1;

    if (!file_name) {
        ESP_LOGE(TAG, "File name is NULL");
        return size;
    }

    char *path = (char *)calloc(strlen(CONFIG_FILES_PATH) + strlen(file_name) + 1, sizeof(char));
    if (!path) {
        ESP_LOGE(TAG, "Allocation error");
    } else {
        strcpy(path, CONFIG_FILES_PATH);
        strcpy(path + strlen(CONFIG_FILES_PATH), file_name);
        struct stat file_stat;
        if (stat(path, &file_stat) == 0) {
            size = file_stat.st_size;
        }
        free(path);
    }

    return size;
}

esp_err_t storage_save_credentials(const char *ssid, const char *pass)
{
    if (!ssid || !pass) {
        return ESP_ERR_INVALID_ARG;
    }

    int ret = storage_nvs_write(SSID_KEY, ssid, SSID_LENGTH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save SSID (%d)", ret);
        return ret;
    }

    ret = storage_nvs_write(PASSWORD_KEY, pass, PASSWORD_LENGTH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save WiFi password (%d)", ret);
        return ret;
    }

    char tmp = WIFI_STA_MODE;
    ret = storage_nvs_write(WIFI_MODE_KEY, &tmp, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save WiFi password (%d)", ret);
        return ret;
    }
    return ESP_OK;
}

static int get_index_from_target_name(char *target_name)
{
    char *tmp_target_list = storage_get_target_names(false);
    char *targets = (char *)calloc(strlen(tmp_target_list) + 1, sizeof(char));
    if (!targets) {
        ESP_LOGE(TAG, "Allocation error");
        return -1;
    }
    int i = 0;
    char *delimiter = "\n";
    strcpy(targets, tmp_target_list);
    char *str = strtok(targets, delimiter);
    while (str != NULL)  {
        if (strcmp(target_name, str) == 0) {
            free(targets);
            return i;
        }
        str = strtok(NULL, delimiter);
        i++;
    }

    free(targets);
    return -1;
}

esp_err_t storage_get_target_name_from_index(int index, char **target_name)
{
    char *tmp_target_list = storage_get_target_names(false);
    char *targets = (char *)calloc(strlen(tmp_target_list) + 1, sizeof(char));
    if (!targets) {
        ESP_LOGE(TAG, "Allocation error");
        return ESP_FAIL;
    }

    strcpy(targets, tmp_target_list);

    char *delimiter = "\n";
    int i = 0;
    char *str = strtok(targets, delimiter);

    while (i != index) {
        str = strtok (NULL, delimiter);
        i++;
    }

    char *ret_str = (char *)calloc(strlen(str) + 1, sizeof(char));
    if (!ret_str) {
        free(targets);
        ESP_LOGE(TAG, "Allocation error");
        return ESP_FAIL;
    }

    strcpy(ret_str, str);
    free(targets);

    *target_name = ret_str;
    return ESP_OK;
}

bool storage_is_target_single_core(char *target_name)
{
    if (!target_name) {
        ESP_LOGE(TAG, "Target name is NULL");
        return false;
    }

    const char *multi_core_targets[] = {"esp32.cfg", "esp32s3.cfg"};

    for (int i = 0; i < sizeof(multi_core_targets) / sizeof(char *); i++) {
        if (strcmp(multi_core_targets[i], target_name) == 0) {
            return false;
        }
    }
    return true;
}

esp_err_t storage_get_openocd_config(void)
{
    char option;
    oocd_config.dual_core = true;
    oocd_config.flash_support = true;
    oocd_config.rtos = 0;
    oocd_config.target_index = 0;
    oocd_config.debug_level_index = 0;

    int ret_nvs = storage_nvs_read(DUAL_CORE_KEY, &option, 1);
    if (ret_nvs == ESP_OK && option == '0') {
        oocd_config.dual_core = false;
    }

    ret_nvs = storage_nvs_read(FLASH_SUPPORT_KEY, &option, 1);
    if (ret_nvs == ESP_OK && option == '0') {
        oocd_config.flash_support = false;
    }

    ret_nvs = storage_nvs_read(RTOS_KEY, &option, 1);
    if (ret_nvs == ESP_OK) {
        oocd_config.rtos = option;
    }

    char *param;
    ret_nvs = storage_nvs_read_param(OOCD_F_PARAM_KEY, &param);
    if (ret_nvs == ESP_OK) {
        int index = get_index_from_target_name(param + strlen(TARGET_PATH_PRELIM));
        if (index != -1) {
            oocd_config.target_index = index;
        }
    }

    ret_nvs = storage_nvs_read_param(OOCD_D_PARAM_KEY, &param);
    if (ret_nvs == ESP_OK) {
        oocd_config.debug_level_index = param[strlen(param) - 1] - 48 - 2;
    }

    return ESP_OK;
}

char *storage_get_target_names(bool update)
{
    static char *target_list = NULL;
    if (!update) {
        return target_list;
    } else if (target_list != NULL) {
        free(target_list);
    }

    target_list = storage_get_config_files(CONFIG_FILES_PATH, true, &files_count);
    return target_list;
}

esp_err_t storage_get_target_path(const char *target_name, char **path)
{
    char *prelim = TARGET_PATH_PRELIM;
    char *file_param = (char *)calloc(strlen(prelim) + strlen(target_name) + 1, sizeof(char));
    if (!file_param) {
        ESP_LOGE(TAG, "Allocation error");
        return ESP_FAIL;
    }
    strcpy(file_param, prelim);
    strcpy(file_param + strlen(prelim), target_name);
    *path = file_param;
    return ESP_OK;
}
