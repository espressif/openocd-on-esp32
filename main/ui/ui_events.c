// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.2.1
// LVGL VERSION: 8.3.4
// PROJECT: SquareLine_Project
#include "ui.h"
#include "../storage.h"
#include "esp_log.h"
#include "esp_system.h"
#include "../networking.h"
#include "../log.h"

static const char *TAG = "ui";

static void single_core_ui_setup();
static void dual_core_ui_setup();

void reset_button_handler(lv_event_t *e)
{
    ESP_OOCD_ERROR_CHECK(storage_nvs_erase_everything(), NULL, "Failed to erase NVS");
    esp_restart();
}

void target_select_dropdown_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    oocd_config.target_index = lv_dropdown_get_selected(obj);

    char *target_name;
    ESP_OOCD_ERROR_CHECK(storage_get_target_name_from_index(oocd_config.target_index, &target_name), NULL, "Failed to get target index");
    if (storage_is_target_single_core(target_name)) {
        single_core_ui_setup();
    } else {
        dual_core_ui_setup();
    }
    free(target_name);
}

void run_openocd_button_handler(lv_event_t *e)
{
    char *target_name;
    ESP_OOCD_ERROR_CHECK(storage_get_target_name_from_index(oocd_config.target_index, &target_name), NULL, "Failed to get target index");
    char *file_param = NULL;
    ESP_OOCD_ERROR_CHECK(storage_get_target_path(target_name, &file_param), NULL, "Failed to get target path");

    int ret = storage_nvs_write(OOCD_F_PARAM_KEY, file_param, strlen(file_param));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save --file, -f parameter");
    }
    free(file_param);

    char d_param_str[4] = {0};
    snprintf(d_param_str, sizeof(d_param_str), "-d%d", (int)oocd_config.debug_level_index + 2);
    ret = storage_nvs_write(OOCD_D_PARAM_KEY, d_param_str, strlen(d_param_str));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save --debug, -d parameter");
    }

    char c_param[75] = {0};
    snprintf(c_param, sizeof(c_param), "set ESP_FLASH_SIZE %s; set ESP_RTOS %s; ", (oocd_config.flash_support == false ? "0" : "auto"),
             (oocd_config.rtos == 0 ? "FreeRTOS" : oocd_config.rtos == 1 ? "NuttX" : "none"));

    if (!storage_is_target_single_core(target_name)) {
        if (strcmp(target_name, "esp32s3.cfg") == 0) {
            snprintf(c_param + strlen(c_param), sizeof(c_param) - strlen(c_param), "set %s %s; ", ("ESP32_S3_ONLYCPU"), (oocd_config.dual_core == false ? "1" : "2"));
        } else {
            snprintf(c_param + strlen(c_param), sizeof(c_param) - strlen(c_param), "set %s %s; ", ("ESP32_ONLYCPU"), (oocd_config.dual_core == false ? "1" : "3"));
        }
    }
    free(target_name);

    ret = storage_nvs_write(OOCD_C_PARAM_KEY, c_param, strlen(c_param));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save --command, -c parameter");
    }

    char option = oocd_config.dual_core == true ? '1' : '0';
    ret = storage_nvs_write(DUAL_CORE_KEY, &option, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save dual core option");
    }

    option = oocd_config.flash_support == true ? '1' : '0';
    ret = storage_nvs_write(FLASH_SUPPORT_KEY, &option, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save flash support option");
    }

    option = oocd_config.rtos;
    ret = storage_nvs_write(RTOS_KEY, &option, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save rtos option");
    }

    esp_restart();
}

void rtos_support_dropdown_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    oocd_config.rtos = lv_dropdown_get_selected(obj);
}

void flash_support_checkbox_handler(lv_event_t *e)
{
    oocd_config.flash_support = !oocd_config.flash_support;
}

void dual_core_checkbox_handler(lv_event_t *e)
{
    oocd_config.dual_core = !oocd_config.dual_core;
}

void debug_level_dropdown_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    oocd_config.debug_level_index = lv_dropdown_get_selected(obj);
}

static void single_core_ui_setup(void)
{
    bsp_display_lock(0);
    lv_obj_clear_state(ui_dualcorecheckbox, LV_STATE_CHECKED);
    lv_obj_add_state(ui_dualcorecheckbox, LV_STATE_DISABLED);
    oocd_config.dual_core = false;
    bsp_display_unlock();
}

static void dual_core_ui_setup(void)
{
    bsp_display_lock(0);
    lv_obj_clear_state(ui_dualcorecheckbox, LV_STATE_DISABLED);
    bsp_display_unlock();
}

void ui_load_connection_screen(void)
{
    bsp_display_lock(0);
    lv_disp_load_scr(ui_ConnectionScreen);
    bsp_display_unlock();
}

void ui_load_config_screen(void)
{
    bsp_display_lock(0);
    lv_disp_load_scr(ui_ConfigScreen);
    lv_obj_remove_event_cb(ui_ConfigScreen, ui_event_ConfigScreen);
    lv_label_set_text(ui_leftarrowtext, "");
    lv_obj_clean(ui_ConnectionScreen);
    bsp_display_unlock();
}

void ui_load_message(const char *title, const char *text)
{
    bsp_display_lock(0);
    if (ui_messagebox != NULL) {
        lv_msgbox_close(lv_obj_get_parent(ui_messagebox));
    }
    ui_messagebox = lv_msgbox_create(NULL, title, text, NULL, true);
    lv_obj_center(ui_messagebox);
    bsp_display_unlock();
    vTaskDelay(1000 / portTICK_PERIOD_MS);
}

void ui_show_error(const char *msg, const char *file, int line)
{
    if (file) {
        // if source file info is available (debug build)
        char text[128] = {0};
        snprintf(text, sizeof(text), "%s!\n(%s:%d)", msg, file, line);
        ui_load_message("Error", text);
        return;
    }
    ui_load_message("Error", msg);
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

void ui_set_oocd_config(void)
{
    lv_dropdown_set_selected(ui_targetselectdropdown, oocd_config.target_index);
    char *target_name;
    storage_get_target_name_from_index(oocd_config.target_index, &target_name);
    if (storage_is_target_single_core(target_name)) {
        single_core_ui_setup();
    } else {
        dual_core_ui_setup();
    }

    if (!oocd_config.dual_core) {
        lv_obj_clear_state(ui_dualcorecheckbox, LV_STATE_CHECKED);
        lv_obj_add_state(ui_dualcorecheckbox, LV_STATE_DEFAULT);
    }

    if (!oocd_config.flash_support) {
        lv_obj_clear_state(ui_flashsupportcheckbox, LV_STATE_CHECKED);
        lv_obj_add_state(ui_flashsupportcheckbox, LV_STATE_DEFAULT);
    }

    lv_dropdown_set_selected(ui_rtosdropdown, oocd_config.rtos);
    lv_dropdown_set_selected(ui_debuglevelodropdown, oocd_config.debug_level_index);
}

void ui_set_target_menu(void)
{
    lv_dropdown_set_options(ui_targetselectdropdown, storage_get_target_names(true));
}
