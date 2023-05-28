#include "esp_log.h"
#include "esp_system.h"

#include "lvgl.h"

#include "ui.h"
#include "storage.h"
#include "types.h"

static const char *TAG = "ui-events";

void ui_event_qr_screen(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_GESTURE &&  lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_LEFT) {
        lv_scr_load_anim(g_ui_openocd_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
    }
}

void ui_event_info_screen(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_GESTURE &&  lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_LEFT) {
        lv_scr_load_anim(g_ui_openocd_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 200, 0, false);
    }
}

void ui_event_openocd_screen(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_GESTURE &&  lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT) {
        if (g_app_params.mode == APP_MODE_STA) {
            lv_scr_load_anim(g_ui_info_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
        } else {
            lv_scr_load_anim(g_ui_qr_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 200, 0, false);
        }
    }
}

void ui_event_reset_button(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        storage_erase_all();
        esp_restart();
    }
}

void ui_event_run_button(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        /* save selected target */
        uint16_t selected_index = lv_dropdown_get_selected(g_ui_target_dropdown);
        ESP_LOGI(TAG, "save selected target: %s", g_app_params.target_list[selected_index]);
        storage_erase_key(OOCD_CFG_FILE_KEY);
        storage_write(OOCD_CFG_FILE_KEY,
                      g_app_params.target_list[selected_index],
                      strlen(g_app_params.target_list[selected_index]));

        /* save RTOS type */
        selected_index = lv_dropdown_get_selected(g_ui_rtos_dropdown);
        ESP_LOGI(TAG, "save selected rtos: %s", g_app_params.rtos_list[selected_index]);
        storage_write(OOCD_RTOS_TYPE_KEY,
                      g_app_params.rtos_list[selected_index],
                      strlen(g_app_params.rtos_list[selected_index]));

        /* save Interface type */
        selected_index = lv_dropdown_get_selected(g_ui_interface_dropdown);
        ESP_LOGI(TAG, "save selected interface: %s", selected_index == 0 ? "jtag" : "swd");
        storage_write(OOCD_INTERFACE_KEY,  (const char *)&selected_index, 1);

        /* save Flash Support */
        const char *flash_size = lv_obj_get_state(g_ui_flash_checkbox) & LV_STATE_CHECKED ? "auto" : "0";
        ESP_LOGI(TAG, "save selected flash support: %s", flash_size);
        storage_write(OOCD_FLASH_SUPPORT_KEY, flash_size, strlen(flash_size));

        /* save Dual Core Support */
        const char *dual_core = lv_obj_get_state(g_ui_dual_core_checkbox) & LV_STATE_CHECKED ? "3" : "1";
        ESP_LOGI(TAG, "save selected dual core support: %s", dual_core);
        storage_write(OOCD_DUAL_CORE_KEY, dual_core, strlen(dual_core));

        /* save debug_level */
        selected_index = lv_dropdown_get_selected(g_ui_debug_level_dropdown);
        char debug_level = selected_index + '1';
        ESP_LOGI(TAG, "save selected debug_level: %c", debug_level);
        storage_write(OOCD_DBG_LEVEL_KEY, &debug_level, 1);
        esp_restart();
    }
}
