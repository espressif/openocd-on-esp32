
#pragma once

#include <stdint.h>
#include <stdbool.h>

#if CONFIG_UI_ENABLE

#include "lvgl.h"

extern lv_obj_t *g_ui_qr_screen;
extern lv_obj_t *g_ui_openocd_screen;
extern lv_obj_t *g_ui_info_screen;
extern lv_obj_t *g_ui_target_dropdown;
extern lv_obj_t *g_ui_rtos_dropdown;
extern lv_obj_t *g_ui_debug_level_dropdown;
extern lv_obj_t *g_ui_interface_dropdown;
extern lv_obj_t *g_ui_flash_checkbox;
extern lv_obj_t *g_ui_dual_core_checkbox;

void ui_init(void);
void ui_show_openocd_screen(void);
void ui_show_qr_screen(void);
void ui_update_target_list(uint16_t selected_inx);
void ui_update_rtos_list(uint16_t selected_inx);
void ui_update_debug_level_dropdown(uint16_t selected_inx);
void ui_update_flash_checkbox(bool checked);
void ui_update_dual_core_checkbox(bool checked);
void ui_update_interface_dropdown(uint16_t selected_inx);
void ui_show_info_screen(const char *text);
void ui_update_ip_ssid_info(const char *ip, const char *ssid);
void ui_update_ip_info(const char *ip);

#else

__attribute__((weak)) void ui_init(void) {}
__attribute__((weak)) void ui_show_openocd_screen(void) {}
__attribute__((weak)) void ui_show_qr_screen(void) {}
__attribute__((weak)) void ui_update_target_list(uint16_t selected_inx) {}
__attribute__((weak)) void ui_update_rtos_list(uint16_t selected_inx) {}
__attribute__((weak)) void ui_update_debug_level_dropdown(uint16_t selected_inx) {}
__attribute__((weak)) void ui_update_flash_checkbox(bool checked) {}
__attribute__((weak)) void ui_update_dual_core_checkbox(bool checked) {}
__attribute__((weak)) void ui_update_interface_dropdown(uint16_t selected_inx) {}
__attribute__((weak)) void ui_show_info_screen(const char *text) {}
__attribute__((weak)) void ui_update_ip_ssid_info(const char *ip, const char *ssid) {}
__attribute__((weak)) void ui_update_ip_info(const char *ip) {}

#endif
