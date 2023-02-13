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
#if CONFIG_CONN_WEB_SERVER
const char connection_type  = '0';
#include "network/web_server.h"
#endif
#if CONFIG_CONN_PROV
const char connection_type = '1';
#include "network/provisioning.h"
#endif
#if CONFIG_CONN_MANUAL
const char connection_type = '2';
#include "network/manual_config.h"
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

    free(argv[4]);
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

static void wait_for_connection(TickType_t timeout)
{
    /* Wait a maximum of timeout value for OOCD_START_FLAG to be set within
    the event group. Clear the bits before exiting. */
    EventBits_t bits = xEventGroupWaitBits(
                           s_event_group,      /* The event group being tested */
                           OOCD_START_FLAG,    /* The bit within the event group to wait for */
                           pdTRUE,             /* OOCD_START_FLAG should be cleared before returning */
                           pdFALSE,            /* Don't wait for bit, either bit will do */
                           timeout);           /* Wait a maximum of 5 mins for either bit to be set */

    if (bits == 0) {
        ESP_LOGE(TAG, "WiFi credentials has not entered in %d seconds. Restarting...", (timeout / 1000));
        esp_restart();
    }
}

#if CONFIG_CONN_PROV || CONFIG_CONN_WEB_SERVER
static esp_err_t app_param_init(void)
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

    return ESP_OK;
}
#endif /* CONFIG_CONN_PROV || CONFIG_CONN_WEB_SERVER */

esp_err_t check_connection_config(void)
{
    esp_err_t ret = ESP_OK;
    char val = 0;
    if (!storage_nvs_is_key_exist(CONNECTION_TYPE_KEY)) {
        ESP_RETURN_ON_ERROR(storage_nvs_write(CONNECTION_TYPE_KEY, &connection_type, 1), TAG, "Failed at %s:%d", __FILE__, __LINE__);
    } else {
        ret = storage_nvs_read(CONNECTION_TYPE_KEY, &val, 1);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read connection input method");
            return ESP_FAIL;
        }
        if (val != connection_type) {
#if CONFIG_CONN_WEB_SERVER || CONFIG_CONN_PROV
            reset_connection_config();
#endif
            ESP_RETURN_ON_ERROR(storage_nvs_write(CONNECTION_TYPE_KEY, &connection_type, 1), TAG, "Failed at %s:%d", __FILE__, __LINE__);
            ESP_LOGW(TAG, "Old WiFi connection method cleanup done. Restarting...");
            esp_restart();
        }
    }
    return ESP_OK;
}

void app_main(void)
{
    idf_init();
    ESP_ERROR_CHECK(check_connection_config());
#if CONFIG_CONN_WEB_SERVER
    ESP_ERROR_CHECK(app_param_init());
    ESP_ERROR_CHECK(web_server_init());
#elif CONFIG_CONN_PROV
    ESP_ERROR_CHECK(app_param_init());
    ESP_ERROR_CHECK(provisioning_init());
#else
    ESP_ERROR_CHECK(manual_config_init());
#endif

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
