#include <unistd.h>

#include "esp_log.h"

#include "lvgl.h"
#include "bsp/esp-bsp.h"

#include "ui.h"
#include "ui_events.h"
#include "types.h"

static const char *TAG = "ui";

#define PROV_QR_VERSION         "v1"
#define PROV_TRANSPORT_SOFTAP   "softap"

// lvgl global objects that we might need them to delete, update etc..
lv_obj_t *g_ui_qr_screen;
lv_obj_t *g_ui_qr_label;
lv_obj_t *g_ui_ip_info_text;
lv_obj_t *g_ui_ssid_info_text;
lv_obj_t *g_ui_openocd_screen;
lv_obj_t *g_ui_info_screen;
lv_obj_t *g_ui_ip_label;
lv_obj_t *g_ui_info_text_area;
lv_obj_t *g_ui_target_dropdown;
lv_obj_t *g_ui_rtos_dropdown;
lv_obj_t *g_ui_debug_level_dropdown;
lv_obj_t *g_ui_interface_dropdown;
lv_obj_t *g_ui_flash_checkbox;
lv_obj_t *g_ui_dual_core_checkbox;

void ui_show_qr_screen(void)
{
    char qr_payload[150] = {0};

    snprintf(qr_payload, sizeof(qr_payload), "{\"ver\":\"%s\",\"name\":\"%s\"" \
             ",\"pop\":\"%s\",\"transport\":\"%s\"}",
             PROV_QR_VERSION, g_app_params.wifi_ssid, "abcd1234", PROV_TRANSPORT_SOFTAP);

    ESP_LOGI(TAG, "qr payload: %s", qr_payload);

    lv_qrcode_update(g_ui_qr_label, qr_payload, strlen(qr_payload));

    bsp_display_lock(0);
    lv_disp_load_scr(g_ui_qr_screen);
    bsp_display_unlock();
}

void ui_show_openocd_screen(void)
{
    bsp_display_lock(0);
    lv_disp_load_scr(g_ui_openocd_screen);
    bsp_display_unlock();
}

static void ui_init_qr_screen(void)
{
    if (!g_ui_qr_screen) {
        g_ui_qr_screen = lv_obj_create(NULL);
        lv_obj_clear_flag(g_ui_qr_screen, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ui_qr_header_label = lv_obj_create(g_ui_qr_screen);
        lv_obj_set_width(ui_qr_header_label, 140);
        lv_obj_set_height(ui_qr_header_label, 25);
        lv_obj_set_x(ui_qr_header_label, -85);
        lv_obj_set_y(ui_qr_header_label, -107);
        lv_obj_set_align(ui_qr_header_label, LV_ALIGN_CENTER);
        lv_obj_clear_flag(ui_qr_header_label, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ui_qr_header_text = lv_label_create(ui_qr_header_label);
        lv_obj_set_width(ui_qr_header_text, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_qr_header_text, LV_SIZE_CONTENT);
        lv_obj_set_align(ui_qr_header_text, LV_ALIGN_CENTER);
        lv_label_set_text(ui_qr_header_text, "ESP Provisioning");
        lv_obj_set_style_text_font(ui_qr_header_text, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        g_ui_ip_info_text = lv_label_create(g_ui_qr_screen);
        lv_obj_set_width( g_ui_ip_info_text, LV_SIZE_CONTENT);
        lv_obj_set_height( g_ui_ip_info_text, LV_SIZE_CONTENT);
        lv_obj_set_x( g_ui_ip_info_text, 81 );
        lv_obj_set_y( g_ui_ip_info_text, -111);
        lv_obj_set_align( g_ui_ip_info_text, LV_ALIGN_CENTER );
        lv_obj_set_style_text_font(g_ui_ip_info_text, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

        g_ui_ssid_info_text = lv_label_create(g_ui_qr_screen);
        lv_obj_set_width( g_ui_ssid_info_text, LV_SIZE_CONTENT);
        lv_obj_set_height( g_ui_ssid_info_text, LV_SIZE_CONTENT);
        lv_obj_set_x( g_ui_ssid_info_text, 89);
        lv_obj_set_y( g_ui_ssid_info_text, -100);
        lv_obj_set_align( g_ui_ssid_info_text, LV_ALIGN_CENTER );
        lv_obj_set_style_text_font(g_ui_ssid_info_text, &lv_font_montserrat_10, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_qr_body_label = lv_obj_create(g_ui_qr_screen);
        lv_obj_set_width(ui_qr_body_label, 312);
        lv_obj_set_height(ui_qr_body_label, 54);
        lv_obj_set_x(ui_qr_body_label, 0);
        lv_obj_set_y(ui_qr_body_label, -66);
        lv_obj_set_align(ui_qr_body_label, LV_ALIGN_CENTER);
        lv_obj_clear_flag(ui_qr_body_label, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ui_qr_body_text = lv_label_create(ui_qr_body_label);
        lv_obj_set_align(ui_qr_body_text, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_qr_body_text, LV_FLEX_FLOW_COLUMN);
        lv_obj_set_flex_align(ui_qr_body_text, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_label_set_text(ui_qr_body_text,
                          "Please scan this code in \"ESP SoftAP Prov\" app to \nconnect a WiFi. "
                          "If you don't have app, you can \ndownload it from App Store or Google Play");
        lv_obj_set_style_text_align(ui_qr_body_text, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(ui_qr_body_text, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

        g_ui_qr_label = lv_qrcode_create(g_ui_qr_screen, 155, lv_color_hex(0x00), lv_color_hex(0xffffff));
        lv_obj_set_width(g_ui_qr_label, 155);
        lv_obj_set_height(g_ui_qr_label, 155);
        lv_obj_set_x(g_ui_qr_label, 0);
        lv_obj_set_y(g_ui_qr_label, 39);
        lv_obj_set_align(g_ui_qr_label, LV_ALIGN_CENTER);
        lv_obj_clear_flag(g_ui_qr_label, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ui_slide_right_text = lv_label_create(g_ui_qr_screen);
        lv_obj_set_width(ui_slide_right_text, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_slide_right_text, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_slide_right_text, 137 );
        lv_obj_set_y(ui_slide_right_text, 105 );
        lv_obj_set_align(ui_slide_right_text, LV_ALIGN_CENTER );
        lv_label_set_text(ui_slide_right_text, ">>>");
        lv_obj_set_style_text_font(ui_slide_right_text, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_add_event_cb(g_ui_qr_screen, ui_event_qr_screen, LV_EVENT_ALL, NULL);
    }
}

static void ui_init_openocd_screen(void)
{
    if (!g_ui_openocd_screen) {
        g_ui_openocd_screen = lv_obj_create(NULL);
        lv_obj_clear_flag(g_ui_openocd_screen, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ui_slide_left_text = lv_label_create(g_ui_openocd_screen);
        lv_obj_set_width(ui_slide_left_text, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_slide_left_text, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_slide_left_text, -143);
        lv_obj_set_y(ui_slide_left_text, -107);
        lv_obj_set_align(ui_slide_left_text, LV_ALIGN_CENTER);
        lv_label_set_text(ui_slide_left_text, "<<<");
        lv_obj_set_style_text_font(ui_slide_left_text, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_header_label = lv_obj_create(g_ui_openocd_screen);
        lv_obj_set_width(ui_header_label, 211);
        lv_obj_set_height(ui_header_label, 31);
        lv_obj_set_x( ui_header_label, 4 );
        lv_obj_set_y( ui_header_label, 5 );
        lv_obj_set_align(ui_header_label, LV_ALIGN_TOP_MID);
        lv_obj_clear_flag(ui_header_label, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ui_header_text = lv_label_create(ui_header_label);
        lv_obj_set_width(ui_header_text, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_header_text, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_header_text, 1);
        lv_obj_set_y(ui_header_text, 1);
        lv_obj_set_align(ui_header_text, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_header_text, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(ui_header_text, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_label_set_text(ui_header_text, "OpenOCD Configuration");
        lv_obj_set_style_text_align(ui_header_text, LV_TEXT_ALIGN_AUTO, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(ui_header_text, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_targets_label = lv_obj_create(g_ui_openocd_screen);
        lv_obj_set_width(ui_targets_label, 311);
        lv_obj_set_height(ui_targets_label, 45);
        lv_obj_set_x(ui_targets_label, -1);
        lv_obj_set_y(ui_targets_label, -56);
        lv_obj_set_align(ui_targets_label, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_targets_label, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(ui_targets_label, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(ui_targets_label, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ui_target_label = lv_label_create(ui_targets_label);
        lv_obj_set_width(ui_target_label, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_target_label, LV_SIZE_CONTENT);
        lv_obj_set_align(ui_target_label, LV_ALIGN_CENTER);
        lv_label_set_text(ui_target_label, "Target");

        g_ui_target_dropdown = lv_dropdown_create(ui_targets_label);
        lv_obj_set_width(g_ui_target_dropdown, 225);
        lv_obj_set_height(g_ui_target_dropdown, LV_SIZE_CONTENT);
        lv_obj_set_x(g_ui_target_dropdown, 18);
        lv_obj_set_y(g_ui_target_dropdown, 2);
        lv_obj_set_align(g_ui_target_dropdown, LV_ALIGN_CENTER);
        lv_obj_add_flag(g_ui_target_dropdown, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_set_style_text_color(g_ui_target_dropdown, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
        lv_obj_set_style_text_opa(g_ui_target_dropdown, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(g_ui_target_dropdown, lv_color_hex(0x0092D4), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(g_ui_target_dropdown, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_reset_button = lv_btn_create(g_ui_openocd_screen);
        lv_obj_set_width(ui_reset_button, 120);
        lv_obj_set_height(ui_reset_button, 38);
        lv_obj_set_x(ui_reset_button, -96);
        lv_obj_set_y(ui_reset_button, 98);
        lv_obj_set_align(ui_reset_button, LV_ALIGN_CENTER);
        lv_obj_add_flag(ui_reset_button, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_clear_flag(ui_reset_button, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(ui_reset_button, lv_color_hex(0xF42727), LV_PART_MAIN | LV_STATE_DEFAULT );
        lv_obj_set_style_bg_opa(ui_reset_button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_reset_label = lv_label_create(ui_reset_button);
        lv_obj_set_width(ui_reset_label, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_reset_label, LV_SIZE_CONTENT);
        lv_obj_set_align(ui_reset_label, LV_ALIGN_CENTER);
        lv_label_set_text(ui_reset_label, "Reset Settings");
        lv_obj_set_style_text_font(ui_reset_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_run_button = lv_btn_create(g_ui_openocd_screen);
        lv_obj_set_width(ui_run_button, 140);
        lv_obj_set_height(ui_run_button, 38);
        lv_obj_set_x(ui_run_button, 84);
        lv_obj_set_y(ui_run_button, 99);
        lv_obj_set_align(ui_run_button, LV_ALIGN_CENTER);
        lv_obj_add_flag(ui_run_button, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_clear_flag(ui_run_button, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(ui_run_button, lv_color_hex(0x053C67), LV_PART_MAIN | LV_STATE_DEFAULT );
        lv_obj_set_style_bg_opa(ui_run_button, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_run_label = lv_label_create(ui_run_button);
        lv_obj_set_width(ui_run_label, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_run_label, LV_SIZE_CONTENT);
        lv_obj_set_align(ui_run_label, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_run_label, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(ui_run_label, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_label_set_text(ui_run_label, "Restart OpenOCD");
        lv_obj_set_style_text_font(ui_run_label, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_rtos_panel = lv_obj_create(g_ui_openocd_screen);
        lv_obj_set_width(ui_rtos_panel, 155);
        lv_obj_set_height(ui_rtos_panel, 42);
        lv_obj_set_x(ui_rtos_panel, -79);
        lv_obj_set_y(ui_rtos_panel, 55);
        lv_obj_set_align(ui_rtos_panel, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_rtos_panel, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(ui_rtos_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(ui_rtos_panel, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ui_rtos_label = lv_label_create(ui_rtos_panel);
        lv_obj_set_width(ui_rtos_label, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_rtos_label, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_rtos_label, -37);
        lv_obj_set_y(ui_rtos_label, 2);
        lv_obj_set_align(ui_rtos_label, LV_ALIGN_CENTER);
        lv_label_set_text(ui_rtos_label, "RTOS");
        lv_obj_set_style_text_font(ui_rtos_label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

        g_ui_rtos_dropdown = lv_dropdown_create(ui_rtos_panel);
        lv_obj_set_width(g_ui_rtos_dropdown, 96);
        lv_obj_set_height(g_ui_rtos_dropdown, LV_SIZE_CONTENT);
        lv_obj_set_x(g_ui_rtos_dropdown, -8);
        lv_obj_set_y(g_ui_rtos_dropdown, -1);
        lv_obj_set_align(g_ui_rtos_dropdown, LV_ALIGN_CENTER);
        lv_obj_add_flag(g_ui_rtos_dropdown, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_set_style_text_font(g_ui_rtos_dropdown, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_color(g_ui_rtos_dropdown, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
        lv_obj_set_style_text_opa(g_ui_rtos_dropdown, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(g_ui_rtos_dropdown, lv_color_hex(0x0092D4), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(g_ui_rtos_dropdown, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_flash_label = lv_obj_create(g_ui_openocd_screen);
        lv_obj_set_width(ui_flash_label, 155);
        lv_obj_set_height(ui_flash_label, 30);
        lv_obj_set_x(ui_flash_label, 77);
        lv_obj_set_y(ui_flash_label, -17);
        lv_obj_set_align(ui_flash_label, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_flash_label, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(ui_flash_label, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(ui_flash_label, LV_OBJ_FLAG_SCROLLABLE);

        g_ui_flash_checkbox = lv_checkbox_create(ui_flash_label);
        lv_checkbox_set_text(g_ui_flash_checkbox, "Flash Support");
        lv_obj_set_width(g_ui_flash_checkbox, LV_SIZE_CONTENT);
        lv_obj_set_height(g_ui_flash_checkbox, LV_SIZE_CONTENT);
        lv_obj_set_align(g_ui_flash_checkbox, LV_ALIGN_CENTER);
        lv_obj_add_flag(g_ui_flash_checkbox, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

        lv_obj_t *ui_dual_core_panel = lv_obj_create(g_ui_openocd_screen);
        lv_obj_set_width(ui_dual_core_panel, 155);
        lv_obj_set_height(ui_dual_core_panel, 30);
        lv_obj_set_x(ui_dual_core_panel, 77);
        lv_obj_set_y(ui_dual_core_panel, 16);
        lv_obj_set_align(ui_dual_core_panel, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_dual_core_panel, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(ui_dual_core_panel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(ui_dual_core_panel, LV_OBJ_FLAG_SCROLLABLE);

        g_ui_dual_core_checkbox = lv_checkbox_create(ui_dual_core_panel);
        lv_checkbox_set_text(g_ui_dual_core_checkbox, "Dual Core");
        lv_obj_set_width(g_ui_dual_core_checkbox, LV_SIZE_CONTENT);
        lv_obj_set_height(g_ui_dual_core_checkbox, LV_SIZE_CONTENT);
        lv_obj_set_x(g_ui_dual_core_checkbox, -87);
        lv_obj_set_y(g_ui_dual_core_checkbox, -4);
        lv_obj_set_align(g_ui_dual_core_checkbox, LV_ALIGN_CENTER);
        lv_obj_add_flag(g_ui_dual_core_checkbox, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

        lv_obj_t *ui_debug_level_panel = lv_obj_create(g_ui_openocd_screen);
        lv_obj_set_width(ui_debug_level_panel, 155);
        lv_obj_set_height(ui_debug_level_panel, 42);
        lv_obj_set_x(ui_debug_level_panel, 77);
        lv_obj_set_y(ui_debug_level_panel, 55);
        lv_obj_set_align(ui_debug_level_panel, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_debug_level_panel, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(ui_debug_level_panel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(ui_debug_level_panel, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ui_interface_panel = lv_obj_create(g_ui_openocd_screen);
        lv_obj_set_width(ui_interface_panel, 155);
        lv_obj_set_height(ui_interface_panel, 64);
        lv_obj_set_x(ui_interface_panel, -79);
        lv_obj_set_y(ui_interface_panel, 0);
        lv_obj_set_align(ui_interface_panel, LV_ALIGN_CENTER);
        lv_obj_set_flex_flow(ui_interface_panel, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(ui_interface_panel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
        lv_obj_clear_flag(ui_interface_panel, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t *ui_interface_label = lv_label_create(ui_interface_panel);
        lv_obj_set_width(ui_interface_label, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_interface_label, LV_SIZE_CONTENT);
        lv_obj_set_align(ui_interface_label, LV_ALIGN_CENTER);
        lv_label_set_text(ui_interface_label, "Interface");

        g_ui_interface_dropdown = lv_dropdown_create(ui_interface_panel);
        lv_obj_set_width(g_ui_interface_dropdown, 71);
        lv_obj_set_height(g_ui_interface_dropdown, LV_SIZE_CONTENT);
        lv_obj_set_align(g_ui_interface_dropdown, LV_ALIGN_CENTER);
        lv_obj_add_flag(g_ui_interface_dropdown, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_set_style_text_color(g_ui_interface_dropdown, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
        lv_obj_set_style_text_opa(g_ui_interface_dropdown, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(g_ui_interface_dropdown, lv_color_hex(0x0092D4), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(g_ui_interface_dropdown, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_debug_level_label = lv_label_create(ui_debug_level_panel);
        lv_obj_set_width(ui_debug_level_label, LV_SIZE_CONTENT);
        lv_obj_set_height(ui_debug_level_label, LV_SIZE_CONTENT);
        lv_obj_set_x(ui_debug_level_label, -37);
        lv_obj_set_y(ui_debug_level_label, 2);
        lv_obj_set_align(ui_debug_level_label, LV_ALIGN_CENTER);
        lv_label_set_text(ui_debug_level_label, "Debug Level");
        lv_obj_set_style_text_font(ui_debug_level_label, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

        g_ui_debug_level_dropdown = lv_dropdown_create(ui_debug_level_panel);
        lv_obj_set_width(g_ui_debug_level_dropdown, 45);
        lv_obj_set_height(g_ui_debug_level_dropdown, LV_SIZE_CONTENT);
        lv_obj_set_align(g_ui_debug_level_dropdown, LV_ALIGN_CENTER);
        lv_obj_add_flag(g_ui_debug_level_dropdown, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_set_style_text_color(g_ui_debug_level_dropdown, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT );
        lv_obj_set_style_text_opa(g_ui_debug_level_dropdown, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_color(g_ui_debug_level_dropdown, lv_color_hex(0x0092D4), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_bg_opa(g_ui_debug_level_dropdown, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_add_event_cb(ui_reset_button, ui_event_reset_button, LV_EVENT_CLICKED, NULL);
        lv_obj_add_event_cb(ui_run_button, ui_event_run_button, LV_EVENT_CLICKED, NULL);

        lv_obj_add_event_cb(g_ui_openocd_screen, ui_event_openocd_screen, LV_EVENT_ALL, NULL);
    }
}

void ui_update_target_list(uint16_t selected_inx)
{
    lv_dropdown_clear_options(g_ui_target_dropdown);
    if (g_app_params.target_count > 0) {
        for (size_t i = 0; i < g_app_params.target_count - 1; ++i) {
            lv_dropdown_add_option(g_ui_target_dropdown, g_app_params.target_list[i], i);
        }
        lv_dropdown_add_option(g_ui_target_dropdown, g_app_params.target_list[g_app_params.target_count - 1], LV_DROPDOWN_POS_LAST);
        lv_dropdown_set_selected(g_ui_target_dropdown, selected_inx);
    }
}

void ui_update_rtos_list(uint16_t selected_inx)
{
    lv_dropdown_clear_options(g_ui_rtos_dropdown);
    if (g_app_params.rtos_count > 0) {
        for (size_t i = 0; i < g_app_params.rtos_count - 1; ++i) {
            lv_dropdown_add_option(g_ui_rtos_dropdown, g_app_params.rtos_list[i], i);
        }
        lv_dropdown_add_option(g_ui_rtos_dropdown, g_app_params.rtos_list[g_app_params.rtos_count - 1], LV_DROPDOWN_POS_LAST);
        lv_dropdown_set_selected(g_ui_rtos_dropdown, selected_inx);
    }
}

void ui_update_debug_level_dropdown(uint16_t selected_inx)
{
    lv_dropdown_clear_options(g_ui_debug_level_dropdown);
    lv_dropdown_set_options(g_ui_debug_level_dropdown, "1\n2\n3\n4");
    lv_dropdown_set_selected(g_ui_debug_level_dropdown, selected_inx);
}

void ui_update_interface_dropdown(uint16_t selected_inx)
{
    lv_dropdown_clear_options(g_ui_interface_dropdown);
    lv_dropdown_set_options(g_ui_interface_dropdown, "jtag\nswd");
    lv_dropdown_set_selected(g_ui_interface_dropdown, selected_inx);
}

void ui_update_flash_checkbox(bool checked)
{
    if (checked) {
        lv_obj_add_state(g_ui_flash_checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(g_ui_flash_checkbox, LV_STATE_CHECKED);
    }
}

void ui_update_dual_core_checkbox(bool checked)
{
    if (checked) {
        lv_obj_add_state(g_ui_dual_core_checkbox, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(g_ui_dual_core_checkbox, LV_STATE_CHECKED);
    }
}

void ui_update_ip_ssid_info(const char *ip, const char *ssid)
{
    char temp[64] = {0};
    sprintf(temp, "web server ip : %s", ip);
    lv_label_set_text(g_ui_ip_info_text, temp);
    sprintf(temp, "wifi ssid : %s", ssid);
    lv_label_set_text(g_ui_ssid_info_text, temp);
}

void ui_update_ip_info(const char *ip)
{
    char temp[64] = {0};
    sprintf(temp, "web server ip : %s", ip);
    lv_label_set_text(g_ui_ip_label, temp);
}

static void ui_info_screen_init(void)
{
    if (!g_ui_info_screen) {
        g_ui_info_screen = lv_obj_create(NULL);
        lv_obj_clear_flag(g_ui_info_screen, LV_OBJ_FLAG_SCROLLABLE );
        lv_obj_set_style_text_font(g_ui_info_screen, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

        g_ui_info_text_area = lv_textarea_create(g_ui_info_screen);
        lv_obj_set_width(g_ui_info_text_area, 305);
        lv_obj_set_height(g_ui_info_text_area, LV_SIZE_CONTENT);
        lv_obj_set_x(g_ui_info_text_area, -1 );
        lv_obj_set_y(g_ui_info_text_area, -9 );
        lv_obj_set_align(g_ui_info_text_area, LV_ALIGN_CENTER );
        lv_obj_clear_flag( g_ui_info_text_area, LV_OBJ_FLAG_CLICKABLE );
        lv_obj_set_style_text_font(g_ui_info_text_area, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_t *ui_slide_right_text = lv_label_create(g_ui_info_screen);
        lv_obj_set_width( ui_slide_right_text, LV_SIZE_CONTENT);
        lv_obj_set_height( ui_slide_right_text, LV_SIZE_CONTENT);
        lv_obj_set_x( ui_slide_right_text, 138 );
        lv_obj_set_y( ui_slide_right_text, -104 );
        lv_obj_set_align( ui_slide_right_text, LV_ALIGN_CENTER );
        lv_label_set_text(ui_slide_right_text, ">>>");
        lv_obj_set_style_text_font(ui_slide_right_text, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

        g_ui_ip_label = lv_label_create(g_ui_info_screen);
        lv_obj_set_width( g_ui_ip_label, LV_SIZE_CONTENT);
        lv_obj_set_height( g_ui_ip_label, LV_SIZE_CONTENT);
        lv_obj_set_x( g_ui_ip_label, -49 );
        lv_obj_set_y( g_ui_ip_label, -104 );
        lv_obj_set_align( g_ui_ip_label, LV_ALIGN_CENTER );
        lv_label_set_text(g_ui_ip_label, "");
        lv_obj_set_style_text_align(g_ui_ip_label, LV_TEXT_ALIGN_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_text_font(g_ui_ip_label, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);

        lv_obj_add_event_cb(g_ui_info_screen, ui_event_info_screen, LV_EVENT_ALL, NULL);
    }
}

void ui_show_info_screen(const char *text)
{
    ESP_LOGI(TAG, "%s", text);
    lv_textarea_set_text(g_ui_info_text_area, text);
    bsp_display_lock(0);
    lv_disp_load_scr(g_ui_info_screen);
    bsp_display_unlock();
}

void ui_init(void)
{
    bsp_i2c_init();
    bsp_display_start();
    bsp_display_backlight_on();

    lv_disp_t *disp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                        false, LV_FONT_DEFAULT);
    lv_disp_set_theme(disp, theme);

    ui_init_qr_screen();
    ui_init_openocd_screen();
    ui_info_screen_init();
}
