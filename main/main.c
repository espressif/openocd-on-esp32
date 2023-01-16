#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_spiffs.h"
#include "nvs_flash.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"

#include "storage.h"
#include "networking.h"
#include "openocd.h"

static const char *TAG = "main";

static void initialize_console(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);
    /* Configure line endings */
    esp_vfs_dev_uart_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_LF);
    esp_vfs_dev_uart_port_set_tx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CRLF);

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
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
}

void run_openocd(char *config_str)
{
    if (!config_str) {
        ESP_LOGE(TAG, "Config file is NULL");
        return;
    }

    setenv("OPENOCD_SCRIPTS", "/data", 1);
    const char* argv[] = {
        "openocd",
        "-c", "bindto 0.0.0.0",
        "-f", "interface/esp32_gpio.cfg",
        "-f", config_str,
        "-c", "init; reset halt;",
        "-d2"
    };
    int argc = sizeof(argv)/sizeof(argv[0]);
    int ret = openocd_main(argc, (char**) argv);
    ESP_LOGI(TAG, "openocd has finished, exit code %d", ret);
}

esp_err_t initialize_wifi(char **config_str)
{
    ESP_LOGI(TAG, "Initializing Wi-Fi");
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    return setup_network(config_str);
}

void app_main(void)
{
    char *config_str;
    ESP_LOGI(TAG, "Setting up...");
    ESP_ERROR_CHECK(nvs_flash_init());
    initialize_console();
    ESP_ERROR_CHECK(mount_storage());
    ESP_ERROR_CHECK(initialize_wifi(&config_str));
    if (config_str) {
        run_openocd(config_str);
        free(config_str);
    }
    ESP_LOGW(TAG, "OpenOCD has finished. You can either reset the board or send the config file from the web interface to restart device");
    while (true) {
        ESP_LOGI(TAG, "Infinite loop");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
}
