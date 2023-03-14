#include "esp_log.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "esp_check.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "freertos/event_groups.h"

#include "sdkconfig.h"

#include "storage.h"
#include "openocd.h"
#include "networking.h"

#if CONFIG_CONN_WEB_SERVER
static const char s_connection_type  = '0';
#endif
#if CONFIG_CONN_PROV
static const char s_connection_type = '1';
#endif
#if CONFIG_CONN_MANUAL
static const char s_connection_type = '2';
#endif

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

void run_openocd()
{
    setenv("OPENOCD_SCRIPTS", "/data", 1);
    char *argv[10] = {
        "openocd",
        "-c", "bindto 0.0.0.0",
    };
    /* Constant OpenOCD parameters takes 3 index */
    int argc = 3;

    char *param = NULL;
    int ret_nvs = storage_nvs_read_param(OOCD_C_PARAM_KEY, &param);
    if (ret_nvs != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get --command, -c parameter");
    } else if (param != NULL) {
        argv[argc++] = "-c";
        argv[argc++] = param;
    }

    argv[argc++] = "-f";
    argv[argc++] = "interface/esp32_gpio.cfg";

    argv[argc++] = "-f";
    ret_nvs = storage_nvs_read_param(OOCD_F_PARAM_KEY, &argv[argc]);
    if (ret_nvs != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get --file, -f parameter.");
        return;
    }
    if (argv[argc++] == NULL) {
        ESP_LOGE(TAG, "File parameter is NULL");
        return;
    }

    ret_nvs = storage_nvs_read_param(OOCD_D_PARAM_KEY, &param);
    if (ret_nvs != ESP_OK) {
        ESP_LOGW(TAG, "Failed to get --debug, -d parameter");
    } else if (param != NULL) {
        argv[argc++] = param;
    }

    int ret = openocd_main(argc, (char **)argv);
    ESP_LOGI(TAG, "openocd has finished, exit code %d", ret);

    /* Free the OpenOCD command parameter */
    free(argv[4]);
    if (argc > 6) {
        /* Free the OpenOCD file parameter */
        free(argv[8]);
    }
    if (argc > 8) {
        /* Free the OpenOCD debug parameter */
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

#if CONFIG_CONN_PROV || CONFIG_CONN_WEB_SERVER
static esp_err_t app_param_init(void)
{
    esp_err_t ret = ESP_OK;

    if (!storage_nvs_is_key_exist(OOCD_F_PARAM_KEY)) {
        const char *default_f_param = "target/esp32.cfg";
        ret = storage_nvs_write(OOCD_F_PARAM_KEY, default_f_param, strlen(default_f_param));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to save default --file, -f command");
            return ESP_FAIL;
        }
    }

    return ESP_OK;
}
#else
static esp_err_t app_param_init(void)
{
    esp_err_t ret = ESP_OK;

    ret = storage_nvs_write(SSID_KEY, CONFIG_ESP_WIFI_SSID, SSID_LENGTH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save ssid");
        return ESP_FAIL;
    }

    ret = storage_nvs_write(PASSWORD_KEY, CONFIG_ESP_WIFI_PASSWORD, PASSWORD_LENGTH);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save password");
        return ESP_FAIL;
    }

    char wifi_mode = WIFI_STA_MODE;
    ret = storage_nvs_write(WIFI_MODE_KEY, &wifi_mode, 1);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save WiFi mode");
    }

    ret = storage_nvs_write(OOCD_F_PARAM_KEY, CONFIG_OPENOCD_F_PARAM, strlen(CONFIG_OPENOCD_F_PARAM));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save default --file, -f command");
        return ESP_FAIL;
    }

    ret = storage_nvs_write(OOCD_C_PARAM_KEY, CONFIG_OPENOCD_C_PARAM, strlen(CONFIG_OPENOCD_C_PARAM));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save default --command, -c command");
        return ESP_FAIL;
    }

    char d_param_str[4] = {0};
    snprintf(d_param_str, sizeof(d_param_str), "-d%d", CONFIG_OPENOCD_D_PARAM);
    ret = storage_nvs_write(OOCD_D_PARAM_KEY, d_param_str, strlen(d_param_str));
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save default --debug, -d command");
        return ESP_FAIL;
    }

    return ESP_OK;
}
#endif /* CONFIG_CONN_PROV || CONFIG_CONN_WEB_SERVER  */

esp_err_t check_connection_config(void)
{
    if (storage_nvs_is_key_exist(CONNECTION_TYPE_KEY)) {
        char val = 0;
        esp_err_t ret = storage_nvs_read(CONNECTION_TYPE_KEY, &val, 1);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read connection input method");
            return ESP_FAIL;
        }
        if (val == s_connection_type) {
            return ESP_OK;
        } else {
            reset_connection_config();
            ESP_LOGW(TAG, "Old WiFi connection method cleanup done. Restarting...");
            esp_restart();
        }
    }
    ESP_RETURN_ON_ERROR(storage_nvs_write(CONNECTION_TYPE_KEY, &s_connection_type, 1), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    return ESP_OK;
}

void app_main(void)
{
    idf_init();
    ESP_ERROR_CHECK(check_connection_config());
    ESP_ERROR_CHECK(app_param_init());
    ESP_ERROR_CHECK(networking_init());

    run_openocd();
    ESP_LOGW(TAG, "OpenOCD has finished. "
             "You can either reset the board or send the config file from the web interface to restart device");

    while (true) {
        ESP_LOGI(TAG, "Infinite loop");
        vTaskDelay(3000 / portTICK_PERIOD_MS);
    }
    esp_restart();
}
