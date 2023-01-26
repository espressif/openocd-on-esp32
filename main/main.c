#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"

#include "storage.h"
#include "networking.h"
#include "openocd.h"

static const char *TAG = "main";

static void init_console(void)
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

esp_err_t read_oocd_param(char *key, char **value)
{
    if (!key || !value) {
        return ESP_ERR_INVALID_ARG;
    }

    /* First read length of the config string */
    size_t len = storage_nvs_get_value_length(key);
    if (len == 0) {
        return ESP_FAIL;
    }

    /* allocate memory for the null terminated string */
    char *ptr = (char *)calloc(len + 1, sizeof(char));
    if (!ptr) {
        ESP_LOGE(TAG, "Failed to allocate memory");
        return ESP_FAIL;
    }

    esp_err_t ret_nvs = storage_nvs_read(key, ptr, len);
    if (ret_nvs != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get %s command", key);
        free(ptr);
        return ESP_FAIL;
    }

    *value = ptr;

    return ESP_OK;
}

void run_openocd()
{
    setenv("OPENOCD_SCRIPTS", "/data", 1);
    char *argv[10] = {
        "openocd",
        "-c", "bindto 0.0.0.0",
        "-f", "interface/esp32_gpio.cfg"
    };
    /* Constant OpenOCD parameters takes 5 index */
    int argc = 5;

    argv[argc++] = "-f";
    int ret_nvs = read_oocd_param(OOCD_F_PARAM_KEY, &argv[argc]);
    if (ret_nvs != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get --file, -f parameter.");
        return;
    }
    if (argv[argc++] == NULL) {
        ESP_LOGE(TAG, "File parameter is NULL");
        return;
    }

    char *param = NULL;
    ret_nvs = read_oocd_param(OOCD_C_PARAM_KEY, &param);
    if (ret_nvs != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get --command, -c parameter");
    } else if (param != NULL) {
        argv[argc++] = "-c";
        argv[argc++] = param;
    }

    ret_nvs = read_oocd_param(OOCD_D_PARAM_KEY, &param);
    if (ret_nvs != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get --debug, -d parameter");
    } else if (param != NULL) {
        argv[argc++] = param;
    }

    int ret = openocd_main(argc, (char **)argv);
    ESP_LOGI(TAG, "openocd has finished, exit code %d", ret);

    free(argv[6]);
    if (argc > 6) {
        free(argv[8]);
    }
    if (argc > 8) {
        free(argv[9]);
    }
}

void idf_init(void)
{
    ESP_LOGI(TAG, "Setting up...");
    init_console();
    ESP_ERROR_CHECK(storage_init_filesystem());
    ESP_ERROR_CHECK(nvs_flash_init());
    esp_netif_init();
    ESP_ERROR_CHECK(esp_event_loop_create_default());
}

esp_err_t app_param_init(void)
{
    esp_err_t ret = ESP_OK;
    const char *default_f_param = "target/esp32.cfg";

    if (!storage_nvs_is_key_exist(OOCD_F_PARAM_KEY)) {
        ret = storage_nvs_write(OOCD_F_PARAM_KEY, default_f_param, strlen(default_f_param));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to save default --file, -f command");
            return ESP_FAIL;
        }
    }

    return networking_init_wifi_mode();
}

esp_err_t wait_for_connection(TickType_t timeout)
{
    if (networking_wait_for_connection(timeout) != ESP_OK) {
        ESP_LOGE(TAG, "WiFi credentials has not entered in %d seconds. Restarting...", (timeout / 1000));
        esp_restart();
    }
    return ESP_OK;
}

void app_main(void)
{
    idf_init();
    ESP_ERROR_CHECK(app_param_init());
    ESP_ERROR_CHECK(networking_init_connection());
    ESP_ERROR_CHECK(networking_start_server());
    wait_for_connection(30000 / portTICK_PERIOD_MS);

    run_openocd();
    ESP_LOGW(TAG, "OpenOCD has finished. "
             "You can either reset the board or send the config file from the web interface to restart device");

    while (true) {
        ESP_LOGI(TAG, "Infinite loop");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
    esp_restart();
}
