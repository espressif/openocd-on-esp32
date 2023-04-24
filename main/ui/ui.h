// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.2.1
// LVGL VERSION: 8.3.4
// PROJECT: SquareLine_Project

#ifndef _SQUARELINE_PROJECT_UI_H
#define _SQUARELINE_PROJECT_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "lvgl.h"
#include "bsp/esp-bsp.h"

void ui_event_ConnectionScreen(lv_event_t *e);
extern lv_obj_t *ui_ConnectionScreen;
extern lv_obj_t *ui_connectionheaderlabel;
extern lv_obj_t *ui_connectionheadertext;
extern lv_obj_t *ui_explanationlabel;
extern lv_obj_t *ui_explanationtext;
extern lv_obj_t *ui_qrlabel;
extern lv_obj_t *ui_rightarrowtext;
void ui_event_ConfigScreen(lv_event_t *e);
extern lv_obj_t *ui_ConfigScreen;
extern lv_obj_t *ui_leftarrowtext;
extern lv_obj_t *ui_configscreenheaderlabel;
extern lv_obj_t *ui_configscreenheadertext;
extern lv_obj_t *ui_targetslabel;
extern lv_obj_t *ui_targetselectlabel;
void ui_event_targetselectdropdown(lv_event_t *e);
extern lv_obj_t *ui_targetselectdropdown;
void ui_event_resetbutton(lv_event_t *e);
extern lv_obj_t *ui_resetbutton;
extern lv_obj_t *ui_resetlabel;
void ui_event_runbutton(lv_event_t *e);
extern lv_obj_t *ui_runbutton;
extern lv_obj_t *ui_runlabel;
extern lv_obj_t *ui_rtospanel;
extern lv_obj_t *ui_rtossupportlabel;
void ui_event_rtosdropdown(lv_event_t *e);
extern lv_obj_t *ui_rtosdropdown;
extern lv_obj_t *ui_flashpabel;
void ui_event_flashsupportcheckbox(lv_event_t *e);
extern lv_obj_t *ui_flashsupportcheckbox;
extern lv_obj_t *ui_dualcorepanel;
void ui_event_dualcorecheckbox(lv_event_t *e);
extern lv_obj_t *ui_dualcorecheckbox;
extern lv_obj_t *ui_debuglevelpanel;
extern lv_obj_t *ui_debuglevellabel;
void ui_event_debuglevelodropdown(lv_event_t *e);
extern lv_obj_t *ui_debuglevelodropdown;
extern lv_obj_t *ui_messagebox;

void target_select_dropdown_handler(lv_event_t *e);
void reset_button_handler(lv_event_t *e);
void run_openocd_button_handler(lv_event_t *e);
void rtos_support_dropdown_handler(lv_event_t *e);
void flash_support_checkbox_handler(lv_event_t *e);
void dual_core_checkbox_handler(lv_event_t *e);
void debug_level_dropdown_handler(lv_event_t *e);

void ui_set_target_menu(void);
void ui_init(void);
void ui_load_config_screen(void);
void ui_load_connection_screen(void);
void ui_clear_screen(void);
void ui_print_qr(char *data);
void ui_load_message(const char *title, const char *text);
void ui_show_error(const char *msg, const char *file, int line);
void ui_set_oocd_config(void);
void ui_hw_init(void);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
