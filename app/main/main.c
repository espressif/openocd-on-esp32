#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_vfs_dev.h"
#include "driver/uart.h"
#include "esp_spiffs.h"

#include "jim.h"
#include "openocd.h"

static void initialize_console(void)
{
    /* Drain stdout before reconfiguring it */
    fflush(stdout);
    fsync(fileno(stdout));

    /* Disable buffering on stdin */
    setvbuf(stdin, NULL, _IONBF, 0);

    /* Configure line endings */
    esp_vfs_dev_uart_set_rx_line_endings(ESP_LINE_ENDINGS_LF);
    esp_vfs_dev_uart_set_tx_line_endings(ESP_LINE_ENDINGS_CRLF);

    /* Configure UART. Note that REF_TICK is used so that the baud rate remains
     * correct while APB frequency is changing in light sleep mode.
     */
    const uart_config_t uart_config = {
            .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .source_clk = UART_SCLK_REF_TICK,
    };
    /* Install UART driver for interrupt-driven reads and writes */
    ESP_ERROR_CHECK( uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM,
            256, 0, 0, NULL, 0) );
    ESP_ERROR_CHECK( uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &uart_config) );

    /* Tell VFS to use UART driver */
    esp_vfs_dev_uart_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
}

static const char *TAG = "main";

static void JimPrintErrorMessage(Jim_Interp *interp)
{
    Jim_MakeErrorMessage(interp);
    fprintf(stderr, "%s\n", Jim_String(Jim_GetResult(interp)));
}

extern int Jim_initjimshInit(Jim_Interp *interp);

static void JimSetArgv(Jim_Interp *interp, int argc, char *const argv[])
{
    int n;
    Jim_Obj *listObj = Jim_NewListObj(interp, NULL, 0);


    for (n = 0; n < argc; n++) {
        Jim_Obj *obj = Jim_NewStringObj(interp, argv[n], -1);

        Jim_ListAppendElement(interp, listObj, obj);
    }

    Jim_SetVariableStr(interp, "argv", listObj);
    Jim_SetVariableStr(interp, "argc", Jim_NewIntObj(interp, argc));
}

void run_jimsh(void)
{
    int retcode;
    Jim_Interp *interp;
    /* Create and initialize the interpreter */
    interp = Jim_CreateInterp();
    Jim_RegisterCoreCommands(interp);

    /* Register static extensions */
    if (Jim_InitStaticExtensions(interp) != JIM_OK) {
        JimPrintErrorMessage(interp);
    }

    Jim_SetVariableStrWithStr(interp, "jim::argv0", "jimsh");
    Jim_SetVariableStrWithStr(interp, JIM_INTERACTIVE, "1");
    retcode = Jim_initjimshInit(interp);

    if (retcode == JIM_ERR) {
        JimPrintErrorMessage(interp);
    }
    if (retcode != JIM_EXIT) {
        JimSetArgv(interp, 0, NULL);
        retcode = Jim_InteractivePrompt(interp);
    }
    if (retcode == JIM_EXIT) {
        retcode = Jim_GetExitCode(interp);
    }
    else if (retcode == JIM_ERR) {
        retcode = 1;
    }
    else {
        retcode = 0;
    }
    Jim_FreeInterp(interp);
    ESP_LOGI(TAG, "jimtcl has finished, exit code %d", retcode);
}

void mount_storage(void)
{
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/data",
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = false
    };

    // Use settings defined above to initialize and mount SPIFFS filesystem.
    // Note: esp_vfs_spiffs_register is an all-in-one convenience function.
    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
}

void run_openocd(void)
{
    setenv("OPENOCD_SCRIPTS", "/data", 1);
    const char* argv[] = {
        "openocd",
        "-f", "/data/target/esp32.cfg"
    };
    int argc = sizeof(argv)/sizeof(argv[0]);
    int ret = openocd_main(argc, (char**) argv);
    ESP_LOGI(TAG, "opencod has finished, exit code %d", ret);
}

void app_main(void)
{
    initialize_console();
    mount_storage();
    ESP_LOGI(TAG, "Setting up...");
    
    // run_jimsh();

    run_openocd();

    vTaskDelay(1000/portTICK_PERIOD_MS);
    esp_restart();
}
