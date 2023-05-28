#pragma once

#define WIFI_SSID_LEN          32
#define WIFI_PASS_LEN          64

typedef enum {
    APP_MODE_AP = 0,
    APP_MODE_STA = 1
} app_mode_t;

typedef struct {
    app_mode_t mode;
    const char *net_adapter_name;
    const char *command_arg;    /* -command | -c */
    const char *config_file;    /* --file   | -f */
    const char *rtos_type;
    const char *flash_size;
    char dual_core;
    char debug_level;           /* --debug  | -d */
    char interface;             /* 0: jtag 1: swd*/
    char wifi_ssid[WIFI_SSID_LEN];
    char wifi_pass[WIFI_PASS_LEN];
    char my_ip[16];
    const char **target_list;
    uint16_t target_count;
    uint16_t selected_target_index;
    const char **rtos_list;
    uint8_t rtos_count;
    uint16_t selected_rtos_index;
} app_params_t;

extern app_params_t g_app_params;
