#include "sdkconfig.h"

#include <unistd.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_check.h"
#include "driver/uart.h"

#include "types.h"
#include "storage.h"
#include "network.h"
#include "web_server.h"
#include "ui.h"
#include "openocd.h"

#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 3, 0)
#include "driver/uart_vfs.h"
#define vfs_dev_port_set_rx_line_endings uart_vfs_dev_port_set_rx_line_endings
#define vfs_dev_port_set_tx_line_endings uart_vfs_dev_port_set_tx_line_endings
#define vfs_dev_uart_use_driver uart_vfs_dev_use_driver
#else
#include "esp_vfs_dev.h"
#define vfs_dev_port_set_rx_line_endings esp_vfs_dev_uart_port_set_rx_line_endings
#define vfs_dev_port_set_tx_line_endings esp_vfs_dev_uart_port_set_tx_line_endings
#define vfs_dev_uart_use_driver esp_vfs_dev_uart_use_driver
#endif

static const char *TAG = "main";

app_params_t g_app_params;

static void init_console(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);
    /* Configure line endings */
    vfs_dev_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_LF);
    vfs_dev_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
        .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
#if CONFIG_IDF_TARGET_ESP32 || CONFIG_IDF_TARGET_ESP32S2
        .source_clk = UART_SCLK_REF_TICK,
#else
        .source_clk = UART_SCLK_XTAL,
#endif
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config));

    /* Tell VFS to use UART driver */
    vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
}

static bool is_espressif_target(const char *cfg_file)
{
    const char *prefix = "esp32";
    size_t prefix_length = strlen(prefix);

    if (strlen(cfg_file) < prefix_length) {
        return false;
    }

    if (strncmp(cfg_file, prefix, prefix_length) == 0) {
        return true;
    }

    return false;
}

void run_openocd(void)
{
    setenv("OPENOCD_SCRIPTS", "/data", 1);
    const char *argv[20] = {
        "openocd",
        "-c", "bindto 0.0.0.0; set ESP_IDF_HOST 1"
    };
    /* Constant OpenOCD parameters takes 3 index */
    int argc = 3;

    char iface[32] = {0};
    sprintf(iface, "interface/esp_gpio_%s.cfg", g_app_params.interface == 0 ? "jtag" : "swd");
    argv[argc++] = "-f";
    argv[argc++] = iface;

    char config[64] = {0};
    sprintf(config, "target/%s", g_app_params.config_file);

    char command[128] = {0};
    if (is_espressif_target(g_app_params.config_file)) {
        argv[argc++] = "-c";
        sprintf(command, "set ESP_FLASH_SIZE %s; set ESP_RTOS %s; set ESP_ONLYCPU %c",
                g_app_params.flash_size, g_app_params.rtos_type, g_app_params.dual_core);
        argv[argc++] = command;
    }
    if (strlen(g_app_params.command_arg)) {
        argv[argc++] = "-c";
        argv[argc++] = g_app_params.command_arg;
    }

    /* target config must be set after extra commands */
    argv[argc++] = "-f";
    argv[argc++] = config;

    char debug_level[8] = {0};
    sprintf(debug_level, "-d%c", g_app_params.debug_level);
    argv[argc++] = debug_level;

    int ret = openocd_main(argc, (char **)argv);

    ESP_LOGI(TAG, "OpenOCD should not return! ret(%d)", ret);
}

void load_openocd_params(void)
{
    char *read_param = NULL;

    esp_err_t err = storage_alloc_and_read(OOCD_CFG_FILE_KEY, &read_param);
    if (err != ESP_OK || !read_param) {
        g_app_params.config_file = CONFIG_OPENOCD_TARGET_CONFIG_FILE;
    } else {
        g_app_params.config_file = read_param;
    }

    err = storage_read(OOCD_INTERFACE_KEY, &g_app_params.interface, 1);
    if (err != ESP_OK) {
        g_app_params.interface = CONFIG_OPENOCD_INTERFACE;
    }
    ui_update_interface_dropdown(g_app_params.interface);

    read_param = NULL;
    err = storage_alloc_and_read(OOCD_RTOS_TYPE_KEY, &read_param);
    if (err != ESP_OK || !read_param) {
        g_app_params.rtos_type = CONFIG_ESP_RTOS;
    } else {
        g_app_params.rtos_type = read_param;
    }

    read_param = NULL;
    err = storage_alloc_and_read(OOCD_CMD_LINE_ARGS_KEY, &read_param);
    if (err != ESP_OK || !read_param) {
        if (!is_espressif_target(g_app_params.config_file)) {
            g_app_params.command_arg = CONFIG_OPENOCD_CUSTOM_COMMAND;
        } else {
            g_app_params.command_arg = "";
        }
    } else {
        g_app_params.command_arg = read_param;
    }

    read_param = NULL;
    err = storage_alloc_and_read(OOCD_FLASH_SUPPORT_KEY, &read_param);
    if (err != ESP_OK || !read_param) {
        g_app_params.flash_size = CONFIG_ESP_FLASH_SIZE;
    } else {
        g_app_params.flash_size = read_param;
    }
    if (!strcmp(g_app_params.flash_size, "0")) {
        ui_update_flash_checkbox(false);
    } else {
        ui_update_flash_checkbox(true);
    }

    err = storage_read(OOCD_DUAL_CORE_KEY, &g_app_params.dual_core, 1);
    if (err != ESP_OK) {
        g_app_params.dual_core = CONFIG_ESP_ONLYCPU + '0';
    }
    if (g_app_params.dual_core == '1') {
        ui_update_dual_core_checkbox(false);
    } else {
        ui_update_dual_core_checkbox(true);
    }

    err = storage_read(OOCD_DBG_LEVEL_KEY, &g_app_params.debug_level, 1);
    if (err != ESP_OK) {
        g_app_params.debug_level = CONFIG_OPENOCD_DEBUG_LEVEL + '0';
    }
    ui_update_debug_level_dropdown(g_app_params.debug_level - '0' - 1);

    ESP_LOGI(TAG, "config file (%s)", g_app_params.config_file);
    ESP_LOGI(TAG, "rtos type (%s)", g_app_params.rtos_type);
    ESP_LOGI(TAG, "flash size (%s)", g_app_params.flash_size);
    ESP_LOGI(TAG, "dual core (%c)", g_app_params.dual_core);
    ESP_LOGI(TAG, "interface (%s)", g_app_params.interface == 0 ? "jtag" : "swd");
    ESP_LOGI(TAG, "command arg (%s)", g_app_params.command_arg);
    ESP_LOGI(TAG, "debug_level (%c)", g_app_params.debug_level);
}

/*
 * If there is no wifi settings stored into nvs, we will look at the menuconfig.
 * If menuconfig still has default values, we will start AP mode
 * If menuconfig has user settings, we will start STA mode
*/
void load_network_params(void)
{
    size_t ssid_len = storage_get_value_length(WIFI_SSID_KEY);

    ESP_LOGI(TAG, "ssid_len:%d", ssid_len);

    if (ssid_len == 0) {
        ESP_LOGW(TAG, "Failed to get WiFi SSID len from nvs.");
        if (!strcmp(CONFIG_ESP_WIFI_SSID, WIFI_AP_SSSID)) {
            ESP_LOGW(TAG, "Default values read from menuconfig. AP mode will be activated!");
            g_app_params.mode = APP_MODE_AP;
            strcpy(g_app_params.wifi_ssid, WIFI_AP_SSSID);
            strcpy(g_app_params.wifi_pass, WIFI_AP_PASSW);
        } else {
            ESP_LOGW(TAG, "User values read from menuconfig. STA mode will be activated!");
            g_app_params.mode = APP_MODE_STA;
            strcpy(g_app_params.wifi_ssid, CONFIG_ESP_WIFI_SSID);
            strcpy(g_app_params.wifi_pass, CONFIG_ESP_WIFI_PASSWORD);
        }
    } else {
        g_app_params.mode = APP_MODE_STA;
        esp_err_t err = storage_read(WIFI_SSID_KEY, g_app_params.wifi_ssid, ssid_len);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Failed to read WiFi ssid");
        } else {
            size_t pass_len = storage_get_value_length(WIFI_PASS_KEY);
            if (pass_len == 0) {
                ESP_LOGW(TAG, "Failed to get WiFi password len");
            } else {
                err = storage_read(WIFI_PASS_KEY, g_app_params.wifi_pass, pass_len);
                if (err != ESP_OK) {
                    ESP_LOGW(TAG, "Failed to read WiFi password");
                }
            }
        }
    }

    if (g_app_params.mode == APP_MODE_AP) {
        g_app_params.net_adapter_name = "wifiap";
    } else if (g_app_params.mode == APP_MODE_STA) {
        g_app_params.net_adapter_name = "wifista";
    }

    ESP_LOGI(TAG, "app mode (%s)", g_app_params.mode == APP_MODE_AP ? "Access Point" : "Station");
    ESP_LOGI(TAG, "wifi ssid (%s)", g_app_params.wifi_ssid);
    ESP_LOGI(TAG, "wifi pass (%s)", g_app_params.wifi_pass);
}

void init_idf_components(void)
{
    ESP_LOGI(TAG, "Setting up...");
    init_console();
    ESP_ERROR_CHECK(storage_init_filesystem());
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

void app_main(void)
{
    init_idf_components();
    ui_init();
    load_openocd_params();
    load_network_params();

    if (storage_update_target_struct() != ESP_OK) {
        ui_show_info_screen("Target list can not be read. Please check tcl-lite directory");
        goto _wait;
    }
    storage_update_rtos_struct();

#if CONFIG_UI_ENABLE
    if (g_app_params.mode == APP_MODE_AP) {
        g_app_params.net_adapter_name = "wifiprov";
        ui_show_qr_screen();
    } else {
        ui_show_info_screen("Waiting for the network connection...");
    }
#endif

    /* We may need http_handle in the next steps. So starting here makes sense. */
    httpd_handle_t http_handle;
    if (web_server_start(&http_handle) != ESP_OK) {
        ui_show_info_screen("Web server couldn't be started!");
        goto _wait;
    }

    struct network_init_config config = {
        .adapter_name = g_app_params.net_adapter_name,
        .ssid = g_app_params.wifi_ssid,
        .pass = g_app_params.wifi_pass,
        .http_handle = &http_handle
    };

    if (network_start(&config) == ESP_OK) {

        if (!strcmp(config.adapter_name, "wifiprov")) {
            network_get_sta_credentials(g_app_params.wifi_ssid, g_app_params.wifi_pass);
            storage_write(WIFI_SSID_KEY, g_app_params.wifi_ssid, strlen(g_app_params.wifi_ssid));
            storage_write(WIFI_PASS_KEY, g_app_params.wifi_pass, strlen(g_app_params.wifi_pass));

            ui_show_info_screen("Wifi credentials have been changed. Restarting in 2 seconds...");
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            esp_restart();
        }

        network_get_my_ip(g_app_params.my_ip);
        ui_update_ip_info(g_app_params.my_ip);

        ui_show_info_screen("OpenOCD has been launched.");
        run_openocd();
    } else {
        ui_show_info_screen("Network connection can not be establised. Please check your wifi credentials!");
        goto _wait;
    }

    ESP_LOGW(TAG, "You can reset the board or send the new config from the web interface");

    ui_show_info_screen("OpenOCD stopped running or couldn't communicate with the target. Please check your configuration parameters.");

_wait:
    while (true) {
        ESP_LOGI(TAG, "Waiting for the new config...");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
    esp_restart();
}
