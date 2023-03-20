// SquareLine LVGL GENERATED FILE
// EDITOR VERSION: SquareLine Studio 1.2.1
// LVGL VERSION: 8.3.4
// PROJECT: SquareLine_Project

#include "ui.h"

///////////////////// VARIABLES ////////////////////
void ui_event_ConnectionScreen(lv_event_t *e);
lv_obj_t *ui_ConnectionScreen;
lv_obj_t *ui_connectionheaderlabel;
lv_obj_t *ui_connectionheadertext;
lv_obj_t *ui_explanationlabel;
lv_obj_t *ui_explanationtext;
lv_obj_t *ui_qrlabel;
lv_obj_t *ui_rightarrowtext;
void ui_event_ConfigScreen(lv_event_t *e);
lv_obj_t *ui_ConfigScreen;
lv_obj_t *ui_leftarrowtext;
lv_obj_t *ui_configscreenheaderlabel;
lv_obj_t *ui_configscreenheadertext;
lv_obj_t *ui_targetslabel;
lv_obj_t *ui_targetselectlabel;
void ui_event_targetselectdropdown(lv_event_t *e);
lv_obj_t *ui_targetselectdropdown;
void ui_event_resetbutton(lv_event_t *e);
lv_obj_t *ui_resetbutton;
lv_obj_t *ui_resetlabel;
void ui_event_runbutton(lv_event_t *e);
lv_obj_t *ui_runbutton;
lv_obj_t *ui_runlabel;
lv_obj_t *ui_rtospanel;
lv_obj_t *ui_rtossupportlabel;
void ui_event_rtosdropdown(lv_event_t *e);
lv_obj_t *ui_rtosdropdown;
lv_obj_t *ui_flashpabel;
void ui_event_flashsupportcheckbox(lv_event_t *e);
lv_obj_t *ui_flashsupportcheckbox;
lv_obj_t *ui_dualcorepanel;
void ui_event_dualcorecheckbox(lv_event_t *e);
lv_obj_t *ui_dualcorecheckbox;
lv_obj_t *ui_debuglevelpanel;
lv_obj_t *ui_debuglevellabel;
void ui_event_debuglevelodropdown(lv_event_t *e);
lv_obj_t *ui_debuglevelodropdown;
lv_obj_t *ui_messagebox = NULL;

///////////////////// TEST LVGL SETTINGS ////////////////////
#if LV_COLOR_DEPTH != 16
#error "LV_COLOR_DEPTH should be 16bit to match SquareLine Studio's settings"
#endif
#if LV_COLOR_16_SWAP !=1
#error "LV_COLOR_16_SWAP should be 1 to match SquareLine Studio's settings"
#endif

///////////////////// ANIMATIONS ////////////////////

///////////////////// FUNCTIONS ////////////////////
void ui_event_ConnectionScreen(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_GESTURE &&  lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_LEFT) {
        lv_scr_load_anim(ui_ConfigScreen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, false);
    }
}

void ui_event_ConfigScreen(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_GESTURE &&  lv_indev_get_gesture_dir(lv_indev_get_act()) == LV_DIR_RIGHT) {
        lv_scr_load_anim(ui_ConnectionScreen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);

    }
}

void ui_event_targetselectdropdown(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED) {
        target_select_dropdown_handler(e);
    }
}
void ui_event_resetbutton(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        reset_button_handler(e);
    }
}
void ui_event_runbutton(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        run_openocd_button_handler(e);
    }
}
void ui_event_rtosdropdown(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED) {
        rtos_support_dropdown_handler(e);
    }
}
void ui_event_flashsupportcheckbox(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        flash_support_checkbox_handler(e);
    }
}
void ui_event_dualcorecheckbox(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_CLICKED) {
        dual_core_checkbox_handler(e);
    }
}
void ui_event_debuglevelodropdown(lv_event_t *e)
{
    lv_event_code_t event_code = lv_event_get_code(e);
    if (event_code == LV_EVENT_VALUE_CHANGED) {
        debug_level_dropdown_handler(e);
    }
}

///////////////////// SCREENS ////////////////////
void ui_ConnectionScreen_screen_init(void)
{
    ui_ConnectionScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_ConnectionScreen, LV_OBJ_FLAG_SCROLLABLE);

    ui_connectionheaderlabel = lv_obj_create(ui_ConnectionScreen);
    lv_obj_set_width(ui_connectionheaderlabel, 190);
    lv_obj_set_height(ui_connectionheaderlabel, 25);
    lv_obj_set_x(ui_connectionheaderlabel, 0);
    lv_obj_set_y(ui_connectionheaderlabel, -107);
    lv_obj_set_align(ui_connectionheaderlabel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_connectionheaderlabel, LV_OBJ_FLAG_SCROLLABLE);

    ui_connectionheadertext = lv_label_create(ui_connectionheaderlabel);
    lv_obj_set_width(ui_connectionheadertext, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_connectionheadertext, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_connectionheadertext, LV_ALIGN_CENTER);
    lv_label_set_text(ui_connectionheadertext, "ESP Provisioning");
    lv_obj_set_style_text_font(ui_connectionheadertext, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_explanationlabel = lv_obj_create(ui_ConnectionScreen);
    lv_obj_set_width(ui_explanationlabel, 312);
    lv_obj_set_height(ui_explanationlabel, 54);
    lv_obj_set_x(ui_explanationlabel, 0);
    lv_obj_set_y(ui_explanationlabel, -66);
    lv_obj_set_align(ui_explanationlabel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_explanationlabel, LV_OBJ_FLAG_SCROLLABLE);

    ui_explanationtext = lv_label_create(ui_explanationlabel);
    lv_obj_set_align(ui_explanationtext, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_explanationtext, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(ui_explanationtext, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_label_set_text(ui_explanationtext,
                      "Please scan this code in \"ESP SoftAP Prov\" app to \nconnect a WiFi. If you don't have app, you can \ndownload it from App Store or Google Play");
    lv_obj_set_style_text_align(ui_explanationtext, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_explanationtext, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_qrlabel = lv_qrcode_create(ui_ConnectionScreen, 155, lv_color_hex(0x00), lv_color_hex(0xffffff));
    lv_obj_set_width(ui_qrlabel, 155);
    lv_obj_set_height(ui_qrlabel, 155);
    lv_obj_set_x(ui_qrlabel, 0);
    lv_obj_set_y(ui_qrlabel, 39);
    lv_obj_set_align(ui_qrlabel, LV_ALIGN_CENTER);
    lv_obj_clear_flag(ui_qrlabel, LV_OBJ_FLAG_SCROLLABLE);

    ui_rightarrowtext = lv_label_create(ui_ConnectionScreen);
    lv_obj_set_width(ui_rightarrowtext, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_rightarrowtext, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_rightarrowtext, 144);
    lv_obj_set_y(ui_rightarrowtext, -107);
    lv_obj_set_align(ui_rightarrowtext, LV_ALIGN_CENTER);
    lv_label_set_text(ui_rightarrowtext, ">");
    lv_obj_set_style_text_font(ui_rightarrowtext, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_ConnectionScreen, ui_event_ConnectionScreen, LV_EVENT_ALL, NULL);
}

void ui_ConfigScreen_screen_init(void)
{
    ui_ConfigScreen = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_ConfigScreen, LV_OBJ_FLAG_SCROLLABLE);

    ui_leftarrowtext = lv_label_create(ui_ConfigScreen);
    lv_obj_set_width(ui_leftarrowtext, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_leftarrowtext, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_leftarrowtext, -145);
    lv_obj_set_y(ui_leftarrowtext, -107);
    lv_obj_set_align(ui_leftarrowtext, LV_ALIGN_CENTER);
    lv_label_set_text(ui_leftarrowtext, "<");
    lv_obj_set_style_text_font(ui_leftarrowtext, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_configscreenheaderlabel = lv_obj_create(ui_ConfigScreen);
    lv_obj_set_width(ui_configscreenheaderlabel, 243);
    lv_obj_set_height(ui_configscreenheaderlabel, 40);
    lv_obj_set_align(ui_configscreenheaderlabel, LV_ALIGN_TOP_MID);
    lv_obj_clear_flag(ui_configscreenheaderlabel, LV_OBJ_FLAG_SCROLLABLE);

    ui_configscreenheadertext = lv_label_create(ui_configscreenheaderlabel);
    lv_obj_set_width(ui_configscreenheadertext, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_configscreenheadertext, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_configscreenheadertext, 3);
    lv_obj_set_y(ui_configscreenheadertext, -1);
    lv_obj_set_align(ui_configscreenheadertext, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_configscreenheadertext, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_configscreenheadertext, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_label_set_text(ui_configscreenheadertext, "ESP-OOCD Configration");
    lv_obj_set_style_text_font(ui_configscreenheadertext, &lv_font_montserrat_18, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_targetslabel = lv_obj_create(ui_ConfigScreen);
    lv_obj_set_width(ui_targetslabel, 311);
    lv_obj_set_height(ui_targetslabel, 50);
    lv_obj_set_x(ui_targetslabel, -1);
    lv_obj_set_y(ui_targetslabel, -54);
    lv_obj_set_align(ui_targetslabel, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_targetslabel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_targetslabel, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(ui_targetslabel, LV_OBJ_FLAG_SCROLLABLE);

    ui_resetbutton = lv_btn_create(ui_ConfigScreen);
    lv_obj_set_width(ui_resetbutton, 110);
    lv_obj_set_height(ui_resetbutton, 38);
    lv_obj_set_x(ui_resetbutton, -106);
    lv_obj_set_y(ui_resetbutton, 98);
    lv_obj_set_align(ui_resetbutton, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_resetbutton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_resetbutton, LV_OBJ_FLAG_SCROLLABLE);

    ui_targetselectlabel = lv_label_create(ui_targetslabel);
    lv_obj_set_width(ui_targetselectlabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_targetselectlabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_targetselectlabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_targetselectlabel, "Target");

    ui_targetselectdropdown = lv_dropdown_create(ui_targetslabel);
    lv_obj_set_width(ui_targetselectdropdown, 200);
    lv_obj_set_height(ui_targetselectdropdown, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_targetselectdropdown, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_targetselectdropdown, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_text_color(ui_targetselectdropdown, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_targetselectdropdown, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(ui_targetselectdropdown, lv_color_hex(0x0092D4), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_targetselectdropdown, 255, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_text_color(ui_targetselectdropdown, lv_color_hex(0xFFFFFF), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_targetselectdropdown, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    ui_resetlabel = lv_label_create(ui_resetbutton);
    lv_obj_set_width(ui_resetlabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_resetlabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_resetlabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_resetlabel, "Factory Reset");
    lv_obj_set_style_text_font(ui_resetlabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_runbutton = lv_btn_create(ui_ConfigScreen);
    lv_obj_set_width(ui_runbutton, 110);
    lv_obj_set_height(ui_runbutton, 38);
    lv_obj_set_x(ui_runbutton, 104);
    lv_obj_set_y(ui_runbutton, 99);
    lv_obj_set_align(ui_runbutton, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_runbutton, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_clear_flag(ui_runbutton, LV_OBJ_FLAG_SCROLLABLE);

    ui_runlabel = lv_label_create(ui_runbutton);
    lv_obj_set_width(ui_runlabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_runlabel, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_runlabel, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_runlabel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_runlabel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_label_set_text(ui_runlabel, "Run OpenOCD");
    lv_obj_set_style_text_font(ui_runlabel, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_rtospanel = lv_obj_create(ui_ConfigScreen);
    lv_obj_set_width(ui_rtospanel, 155);
    lv_obj_set_height(ui_rtospanel, 50);
    lv_obj_set_x(ui_rtospanel, -79);
    lv_obj_set_y(ui_rtospanel, 50);
    lv_obj_set_align(ui_rtospanel, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_rtospanel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_rtospanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(ui_rtospanel, LV_OBJ_FLAG_SCROLLABLE);

    ui_rtossupportlabel = lv_label_create(ui_rtospanel);
    lv_obj_set_width(ui_rtossupportlabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_rtossupportlabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_rtossupportlabel, -37);
    lv_obj_set_y(ui_rtossupportlabel, 2);
    lv_obj_set_align(ui_rtossupportlabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_rtossupportlabel, "RTOS");
    lv_obj_set_style_text_font(ui_rtossupportlabel, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_rtosdropdown = lv_dropdown_create(ui_rtospanel);
    lv_dropdown_set_options(ui_rtosdropdown, "FreeRTOS\nNuttX\nNone");
    lv_obj_set_width(ui_rtosdropdown, 96);
    lv_obj_set_height(ui_rtosdropdown, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_rtosdropdown, -8);
    lv_obj_set_y(ui_rtosdropdown, -1);
    lv_obj_set_align(ui_rtosdropdown, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_rtosdropdown, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_text_font(ui_rtosdropdown, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_flashpabel = lv_obj_create(ui_ConfigScreen);
    lv_obj_set_width(ui_flashpabel, 155);
    lv_obj_set_height(ui_flashpabel, 50);
    lv_obj_set_x(ui_flashpabel, 77);
    lv_obj_set_y(ui_flashpabel, -3);
    lv_obj_set_align(ui_flashpabel, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_flashpabel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_flashpabel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(ui_flashpabel, LV_OBJ_FLAG_SCROLLABLE);

    ui_flashsupportcheckbox = lv_checkbox_create(ui_flashpabel);
    lv_checkbox_set_text(ui_flashsupportcheckbox, "Flash Support");
    lv_obj_set_width(ui_flashsupportcheckbox, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_flashsupportcheckbox, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_flashsupportcheckbox, LV_ALIGN_CENTER);
    lv_obj_add_state(ui_flashsupportcheckbox, LV_STATE_CHECKED);
    lv_obj_add_flag(ui_flashsupportcheckbox, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    ui_dualcorepanel = lv_obj_create(ui_ConfigScreen);
    lv_obj_set_width(ui_dualcorepanel, 155);
    lv_obj_set_height(ui_dualcorepanel, 50);
    lv_obj_set_x(ui_dualcorepanel, -79);
    lv_obj_set_y(ui_dualcorepanel, -3);
    lv_obj_set_align(ui_dualcorepanel, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_dualcorepanel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_dualcorepanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(ui_dualcorepanel, LV_OBJ_FLAG_SCROLLABLE);

    ui_dualcorecheckbox = lv_checkbox_create(ui_dualcorepanel);
    lv_checkbox_set_text(ui_dualcorecheckbox, "Dual Core");
    lv_obj_set_width(ui_dualcorecheckbox, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_dualcorecheckbox, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_dualcorecheckbox, -87);
    lv_obj_set_y(ui_dualcorecheckbox, -4);
    lv_obj_set_align(ui_dualcorecheckbox, LV_ALIGN_CENTER);
    lv_obj_add_state(ui_dualcorecheckbox, LV_STATE_CHECKED);
    lv_obj_add_flag(ui_dualcorecheckbox, LV_OBJ_FLAG_SCROLL_ON_FOCUS);

    ui_debuglevelpanel = lv_obj_create(ui_ConfigScreen);
    lv_obj_set_width(ui_debuglevelpanel, 155);
    lv_obj_set_height(ui_debuglevelpanel, 50);
    lv_obj_set_x(ui_debuglevelpanel, 77);
    lv_obj_set_y(ui_debuglevelpanel, 50);
    lv_obj_set_align(ui_debuglevelpanel, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_debuglevelpanel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_debuglevelpanel, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_clear_flag(ui_debuglevelpanel, LV_OBJ_FLAG_SCROLLABLE);

    ui_debuglevellabel = lv_label_create(ui_debuglevelpanel);
    lv_obj_set_width(ui_debuglevellabel, LV_SIZE_CONTENT);
    lv_obj_set_height(ui_debuglevellabel, LV_SIZE_CONTENT);
    lv_obj_set_x(ui_debuglevellabel, -37);
    lv_obj_set_y(ui_debuglevellabel, 2);
    lv_obj_set_align(ui_debuglevellabel, LV_ALIGN_CENTER);
    lv_label_set_text(ui_debuglevellabel, "Debug Level");
    lv_obj_set_style_text_font(ui_debuglevellabel, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_debuglevelodropdown = lv_dropdown_create(ui_debuglevelpanel);
    lv_dropdown_set_options(ui_debuglevelodropdown, "2\n3\n4");
    lv_obj_set_width(ui_debuglevelodropdown, 45);
    lv_obj_set_height(ui_debuglevelodropdown, LV_SIZE_CONTENT);
    lv_obj_set_align(ui_debuglevelodropdown, LV_ALIGN_CENTER);
    lv_obj_add_flag(ui_debuglevelodropdown, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_text_font(ui_debuglevelodropdown, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_add_event_cb(ui_targetselectdropdown, ui_event_targetselectdropdown, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_resetbutton, ui_event_resetbutton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_runbutton, ui_event_runbutton, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_rtosdropdown, ui_event_rtosdropdown, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_flashsupportcheckbox, ui_event_flashsupportcheckbox, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_dualcorecheckbox, ui_event_dualcorecheckbox, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_debuglevelodropdown, ui_event_debuglevelodropdown, LV_EVENT_ALL, NULL);
    lv_obj_add_event_cb(ui_ConfigScreen, ui_event_ConfigScreen, LV_EVENT_ALL, NULL);
}

void ui_widget_init(void)
{
    lv_disp_t *disp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(disp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED),
                        false, LV_FONT_DEFAULT);

    ui_ConnectionScreen_screen_init();
    ui_ConfigScreen_screen_init();

    lv_disp_set_theme(disp, theme);
}

void ui_init(void)
{
    bsp_display_lock(0);
    ui_widget_init();
    ui_set_target_menu();
    ui_set_oocd_config();
    bsp_display_unlock();
}

void ui_hw_init(void)
{
    bsp_i2c_init();
    bsp_display_start();
    bsp_display_backlight_on();
}
