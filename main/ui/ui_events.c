// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.2.1
// LVGL VERSION: 8.3.4
// PROJECT: SquareLine_Project
#include "ui.h"
#include "../storage.h"
#include "esp_log.h"
#include "esp_system.h"

#define DUAL_CORE_KEY       "dual_core"
#define FLASH_SUPPORT_KEY   "flash_support"
#define RTOS_KEY            "rtos"
#define TARGET_PATH_PRELIM  "target/"

static const char *TAG = "ui";

static char *s_targets_str;
static bool s_dual_core_option = true;
static bool s_flash_support_option = true;
static int s_debug_level_value = 2;
static int s_rtos_type_value = 0;
static int s_selected_target = 0;

static char *get_target_name_from_index(int index);
static int get_index_from_target_name(char *target_name);
static void single_core_ui_setup();
static void dual_core_ui_setup();
static bool is_single_core(char *target_name);

void reset_button_handler(lv_event_t *e)
{
    ESP_ERROR_CHECK(storage_nvs_erase_everything());
    esp_restart();
}

void target_select_dropdown_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    s_selected_target = lv_dropdown_get_selected(obj);

    char *target_name = get_target_name_from_index(s_selected_target);
    if (!target_name) {
        ESP_LOGE(TAG, "Error happened during target name fetch operation");
        assert(false);
    }
    if (is_single_core(target_name)) {
        single_core_ui_setup();
    } else {
        dual_core_ui_setup();
    }
    free(target_name);
}

void run_openocd_button_handler(lv_event_t *e)
{
    char *target_name = get_target_name_from_index(s_selected_target);
    if (!target_name) {
        ESP_LOGE(TAG, "Error happened during target name fetch operation");
        assert(false);
    }
    char *file_param = (char *)calloc(strlen(TARGET_PATH_PRELIM) + strlen(target_name) + 1, sizeof(char));
    if (!file_param) {
        ESP_LOGE(TAG, "Allocation error");
        assert(false);
    }
    strcpy(file_param, TARGET_PATH_PRELIM);
    strcpy(file_param + strlen(TARGET_PATH_PRELIM), target_name);

    int ret = storage_nvs_write(OOCD_F_PARAM_KEY, file_param, strlen(file_param));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save --file, -f parameter");
    }
    free(file_param);

    char d_param_str[4] = {0};
    snprintf(d_param_str, sizeof(d_param_str), "-d%d", (int)s_debug_level_value);
    ret = storage_nvs_write(OOCD_D_PARAM_KEY, d_param_str, strlen(d_param_str));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save --debug, -d parameter");
    }

    char c_param[75] = {0};
    snprintf(c_param, sizeof(c_param), "set ESP_FLASH_SIZE %s; set ESP_RTOS %s; ", (s_flash_support_option == false ? "0" : "auto"),
             (s_rtos_type_value == 0 ? "FreeRTOS" : s_rtos_type_value == 1 ? "NuttX" : "none"));

    if (!is_single_core(target_name)) {
        if (strcmp(target_name, "esp32s3.cfg") == 0) {
            snprintf(c_param + strlen(c_param), sizeof(c_param) - strlen(c_param), "set %s %s; ", ("ESP32_S3_ONLYCPU"), (s_dual_core_option == false ? "1" : "2"));
        } else {
            snprintf(c_param + strlen(c_param), sizeof(c_param) - strlen(c_param), "set %s %s; ", ("ESP32_ONLYCPU"), (s_dual_core_option == false ? "1" : "3"));
        }
    }
    free(target_name);

    ret = storage_nvs_write(OOCD_C_PARAM_KEY, c_param, strlen(c_param));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save --command, -c parameter");
    }

    char option = s_dual_core_option == true ? '1' : '0';
    ret = storage_nvs_write(DUAL_CORE_KEY, &option, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save dual core option");
    }

    option = s_flash_support_option == true ? '1' : '0';
    ret = storage_nvs_write(FLASH_SUPPORT_KEY, &option, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save flash support option");
    }

    option = s_rtos_type_value;
    ret = storage_nvs_write(RTOS_KEY, &option, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save rtos option");
    }

    esp_restart();
}

void rtos_support_dropdown_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    s_rtos_type_value = lv_dropdown_get_selected(obj);
}

void flash_support_checkbox_handler(lv_event_t *e)
{
    s_flash_support_option = !s_flash_support_option;
}

void dual_core_checkbox_handler(lv_event_t *e)
{
    s_dual_core_option = !s_dual_core_option;
}

void debug_level_dropdown_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    int index = lv_dropdown_get_selected(obj);
    s_debug_level_value = index + 2;
}

static void single_core_ui_setup()
{
    bsp_display_lock(0);
    lv_obj_clear_state(ui_dualcorecheckbox, LV_STATE_CHECKED);
    lv_obj_add_state(ui_dualcorecheckbox, LV_STATE_DISABLED);
    s_dual_core_option = false;
    bsp_display_unlock();
}

static void dual_core_ui_setup()
{
    bsp_display_lock(0);
    lv_obj_clear_state(ui_dualcorecheckbox, LV_STATE_DISABLED);
    lv_obj_add_state(ui_dualcorecheckbox, LV_STATE_CHECKED);
    s_dual_core_option = true;
    bsp_display_unlock();
}

static bool is_single_core(char *target_name)
{
    const char *single_core_targets[] = {"esp32s2.cfg", "esp32c3.cfg", "esp32c2.cfg", "esp32-solo-1.cfg"};

    for (int i = 0; i < sizeof(single_core_targets) / sizeof(char *); i++) {
        if (strcmp(single_core_targets[i], target_name) == 0) {
            return true;
        }
    }
    return false;
}

void ui_load_connection_screen()
{
    bsp_display_lock(0);
    lv_disp_load_scr(ui_ConnectionScreen);
    bsp_display_unlock();
}

void ui_load_config_screen()
{
    bsp_display_lock(0);
    lv_disp_load_scr(ui_ConfigScreen);
    lv_obj_remove_event_cb(ui_ConfigScreen, ui_event_ConfigScreen);
    lv_label_set_text(ui_leftarrowtext, "");
    lv_obj_clean(ui_ConnectionScreen);
    bsp_display_unlock();
}

void ui_print_qr(char *data)
{
    bsp_display_lock(0);
    lv_qrcode_update(ui_qrlabel, data, strlen(data));
    bsp_display_unlock();
}

void ui_clear_screen(void)
{
    bsp_display_lock(0);
    lv_obj_clean(lv_scr_act());
    bsp_display_unlock();
}

static char *get_target_names()
{
    s_targets_str = storage_get_config_files();
    return s_targets_str;
}

static char *get_target_name_from_index(int index)
{
    char *targets = (char *)calloc(strlen(s_targets_str) + 1, sizeof(char));
    if (!targets) {
        ESP_LOGE(TAG, "Allocation error");
        return NULL;
    }

    strcpy(targets, s_targets_str);

    char delimiter = '\n';
    int i = 0;
    char *str = strtok(targets, &delimiter);

    while (i != index) {
        str = strtok (NULL, &delimiter);
        i++;
    }

    char *ret_str = (char *)calloc(strlen(str) + 1, sizeof(char));
    if (!ret_str) {
        free(targets);
        ESP_LOGE(TAG, "Allocation error");
        return NULL;
    }

    strcpy(ret_str, str);
    free(targets);

    return ret_str;
}

static int get_index_from_target_name(char *target_name)
{
    char *targets = (char *)calloc(strlen(s_targets_str) + 1, sizeof(char));
    if (!targets) {
        ESP_LOGE(TAG, "Allocation error");
        return -1;
    }
    int i = 0;
    char *delimiter = "\n";
    strcpy(targets, s_targets_str);
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

static void set_previous_values()
{
    char *param;
    int ret_nvs = storage_nvs_read_param(OOCD_F_PARAM_KEY, &param);
    if (ret_nvs == ESP_OK) {
        int index = get_index_from_target_name(param + strlen(TARGET_PATH_PRELIM));
        if (index != -1) {
            lv_dropdown_set_selected(ui_targetselectdropdown, index);
            s_selected_target = index;
            if (is_single_core(param + strlen(TARGET_PATH_PRELIM))) {
                single_core_ui_setup();
            } else {
                dual_core_ui_setup();
            }
        }
        free(param);
    }

    char option;
    ret_nvs = storage_nvs_read(DUAL_CORE_KEY, &option, 1);
    if (ret_nvs == ESP_OK) {
        s_dual_core_option = true;
        if (option == '0') {
            lv_obj_clear_state(ui_dualcorecheckbox, LV_STATE_CHECKED);
            lv_obj_add_state(ui_dualcorecheckbox, LV_STATE_DEFAULT);
            s_dual_core_option = false;
        }
    }

    ret_nvs = storage_nvs_read(FLASH_SUPPORT_KEY, &option, 1);
    if (ret_nvs == ESP_OK) {
        s_flash_support_option = true;
        if (option == '0') {
            lv_obj_clear_state(ui_flashsupportcheckbox, LV_STATE_CHECKED);
            lv_obj_add_state(ui_flashsupportcheckbox, LV_STATE_DEFAULT);
            s_flash_support_option = false;
        }
    }

    ret_nvs = storage_nvs_read(RTOS_KEY, &option, 1);
    if (ret_nvs == ESP_OK) {
        lv_dropdown_set_selected(ui_rtosdropdown, option);
        s_rtos_type_value = option;
    }

    ret_nvs = storage_nvs_read_param(OOCD_D_PARAM_KEY, &param);
    if (ret_nvs == ESP_OK) {
        int debug_lvl_val = param[strlen(param) - 1] - 48 - 2;
        lv_dropdown_set_selected(ui_debuglevelodropdown, debug_lvl_val);
        s_debug_level_value = debug_lvl_val + 2;
    }
}

void init_ui(void)
{
    bsp_i2c_init();
    bsp_display_start();
    bsp_display_backlight_on();
    bsp_display_lock(0);

    ui_init();
    lv_dropdown_set_options(ui_targetselectdropdown, get_target_names());
    set_previous_values();
    bsp_display_unlock();
}
