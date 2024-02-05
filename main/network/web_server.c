#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/unistd.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_http_server.h"

#include <cJSON.h>

#include "web_server.h"
#include "storage.h"
#include "ui.h"
#include "types.h"

#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif

static const char *TAG = "web-server";

esp_err_t website_handler(httpd_req_t *req)
{
    extern const unsigned char website_html_start[] asm("_binary_website_html_start");
    extern const unsigned char website_html_end[]   asm("_binary_website_html_end");
    const size_t website_html_size = (website_html_end - website_html_start);

    return httpd_resp_send(req, (const char *)website_html_start, website_html_size);
}

esp_err_t esp_logo_handler(httpd_req_t *req)
{
    extern const unsigned char esp_logo_svg_start[] asm("_binary_esp_logo_svg_start");
    extern const unsigned char esp_logo_svg_end[]   asm("_binary_esp_logo_svg_end");
    const size_t esp_logo_size = (esp_logo_svg_end - esp_logo_svg_start);

    httpd_resp_set_type(req, "image/svg+xml");

    return httpd_resp_send(req, (const char *)esp_logo_svg_start, esp_logo_size);
}

esp_err_t favicon_handler(httpd_req_t *req)
{
    extern const unsigned char favicon_ico_start[] asm("_binary_favicon_ico_start");
    extern const unsigned char favicon_ico_end[]   asm("_binary_favicon_ico_end");
    const size_t favicon_ico_size = (favicon_ico_end - favicon_ico_start);

    httpd_resp_set_type(req, "image/x-icon");

    return httpd_resp_send(req, (const char *)favicon_ico_start, favicon_ico_size);
}

static cJSON *set_handler_start(httpd_req_t *req, char *json_buf, uint32_t json_buf_sz)
{
    memset(json_buf, 0x00, json_buf_sz);

    /* Read data received in the request */
    int ret = httpd_req_recv(req, json_buf, json_buf_sz);
    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            httpd_resp_send_408(req);
        }
        return NULL;
    }

    ESP_LOGI(TAG, "json : %s", json_buf);

    cJSON *root = cJSON_ParseWithLength(json_buf, json_buf_sz);
    if (root == NULL) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Json parse error");
        return NULL;
    }
    return root;
}

esp_err_t set_credentials_handler(httpd_req_t *req)
{
    char json[req->content_len];

    cJSON *root = set_handler_start(req, json, sizeof(json));
    if (!root) {
        return ESP_FAIL;
    }

    cJSON *ssid = cJSON_GetObjectItem(root, "ssid");
    cJSON *pass = cJSON_GetObjectItem(root, "pass");
    if (!pass || !ssid || !ssid->valuestring) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Json parse error");
        return ESP_FAIL;
    }

    httpd_resp_sendstr(req, "Wifi credentials were set successfully");

    storage_write(WIFI_SSID_KEY, ssid->valuestring, strlen(ssid->valuestring));
    storage_write(WIFI_PASS_KEY, pass->valuestring, strlen(pass->valuestring));
    cJSON_Delete(root);

    ui_show_info_screen("Wifi credentials have been changed. Restarting in 2 seconds...");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();

    return  ESP_OK;
}

esp_err_t set_openocd_config_handler(httpd_req_t *req)
{
    char json[req->content_len];

    cJSON *root = set_handler_start(req, json, sizeof(json));
    if (!root) {
        return ESP_FAIL;
    }

    cJSON *target = cJSON_GetObjectItem(root, "target");
    cJSON *interface = cJSON_GetObjectItem(root, "interface");
    cJSON *rtos = cJSON_GetObjectItem(root, "rtos");
    cJSON *debug = cJSON_GetObjectItem(root, "debug");
    cJSON *dualCore = cJSON_GetObjectItem(root, "dualCore");
    cJSON *flash = cJSON_GetObjectItem(root, "flash");
    cJSON *cParam = cJSON_GetObjectItem(root, "cParam");

    // Print the values
    ESP_LOGI(TAG, "Target: %s", target->valuestring);
    ESP_LOGI(TAG, "Interface: %s", interface->valuestring[0] == '0' ? "jtag" : "swd");
    ESP_LOGI(TAG, "RTOS: %s", rtos->valuestring);
    ESP_LOGI(TAG, "Debug: %s", debug->valuestring);
    ESP_LOGI(TAG, "Dual Core: %s", dualCore->type == cJSON_True ? "true" : "false");
    ESP_LOGI(TAG, "Flash: %s", flash->type == cJSON_True ? "true" : "false");
    ESP_LOGI(TAG, "C Param: %s", cParam->valuestring);

    storage_write(OOCD_CFG_FILE_KEY, target->valuestring, strlen(target->valuestring));
    storage_write(OOCD_CMD_LINE_ARGS_KEY, cParam->valuestring, strlen(cParam->valuestring));
    storage_write(OOCD_RTOS_TYPE_KEY, rtos->valuestring, strlen(rtos->valuestring));
    storage_write(OOCD_INTERFACE_KEY, interface->valuestring, 1);
    storage_write(OOCD_DBG_LEVEL_KEY, debug->valuestring, 1);

    if (dualCore->type == cJSON_True) {
        storage_write(OOCD_DUAL_CORE_KEY, "3", 1);
    } else {
        storage_write(OOCD_DUAL_CORE_KEY, "1", 1);
    }

    if (flash->type == cJSON_True) {
        storage_write(OOCD_FLASH_SUPPORT_KEY, "auto", 4);
    } else {
        storage_write(OOCD_FLASH_SUPPORT_KEY, "0", 1);
    }

    // Cleanup
    cJSON_Delete(root);

    httpd_resp_sendstr(req, "OpenOCD config is set successfully");

    ui_show_info_screen("OpenOCD parameters have been changed. Restarting in 2 seconds...");
    vTaskDelay(2000 / portTICK_PERIOD_MS);
    esp_restart();

    return ESP_OK;
}

esp_err_t get_openocd_config_handler(httpd_req_t *req)
{
    // Create the config list array
    cJSON *config_list = cJSON_CreateArray();
    for (size_t i = 0; i < g_app_params.target_count; ++i) {
        cJSON_AddItemToArray(config_list, cJSON_CreateString(g_app_params.target_list[i]));
    }

    // Create the rtos list array
    cJSON *rtos_list = cJSON_CreateArray();
    for (size_t i = 0; i < g_app_params.rtos_count; ++i) {
        cJSON_AddItemToArray(rtos_list, cJSON_CreateString(g_app_params.rtos_list[i]));
    }

    // Create the JSON object and add the arrays as properties
    cJSON *json_object = cJSON_CreateObject();
    cJSON_AddItemToObject(json_object, "configList", config_list);
    cJSON_AddItemToObject(json_object, "rtosList", rtos_list);

    // Convert the JSON object to a string
    char *json_string = cJSON_PrintUnformatted(json_object);

    ESP_LOGD(TAG, "openocd_config json : %s", json_string);

    cJSON_Delete(json_object);

    esp_err_t err = httpd_resp_send(req, json_string, HTTPD_RESP_USE_STRLEN);

    free(json_string);

    return err;
}

static void get_filename_from_path(const char *path, char *filename)
{
    const char *last_slash = strrchr(path, '/');
    if (last_slash) {
        strcpy(filename, last_slash + 1);
    } else {
        strcpy(filename, path);
    }
}

static esp_err_t file_upload_handler(httpd_req_t *req)
{
    struct stat file_stat;
    char filepath[128] = CFG_FILE_PATH;

    get_filename_from_path(req->uri, filepath + strlen(CFG_FILE_PATH));

    if (stat(filepath, &file_stat) == 0) {
        ESP_LOGE(TAG, "File already exists : %s", filepath);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File already exists!");
        return ESP_FAIL;
    }

    // Open the file for writing
    FILE *fp = fopen(filepath, "w");
    if (fp == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to open file");
        return ESP_FAIL;
    }

    char buf[1024];
    size_t remaining = req->content_len;
    const uint8_t poll_period = 10; /* ms */
    uint16_t try_count = 1000 / poll_period; /* 1 second timeout */

    while (try_count) {

        ESP_LOGI(TAG, "Remaining size : %d", remaining);

        // Read a chunk of data from the request
        int received = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)));
        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                /* Retry if timeout occurred */
                continue;
            }

            /* In case of unrecoverable error, close and delete the unfinished file*/
            fclose(fp);
            unlink(filepath);

            ESP_LOGE(TAG, "File reception failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }

        /* Write buffer content to file on storage */
        if (received != fwrite(buf, 1, received, fp)) {
            /* Couldn't write everything to file! Storage may be full? */
            fclose(fp);
            unlink(filepath);

            ESP_LOGE(TAG, "File write failed!");
            /* Respond with 500 Internal Server Error */
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
            return ESP_FAIL;
        }

        /* Keep track of remaining size of the file left to be uploaded */
        remaining -= received;
        if (!remaining) {
            break;
        }

        /* If still there is remaining data, something might be wrong. */
        try_count--;
        vTaskDelay(pdMS_TO_TICKS(poll_period));
    }

    // Close the file
    fclose(fp);
    ESP_LOGI(TAG, "File received: %s", filepath);

    storage_update_target_struct();

    // Send the response indicating success
    httpd_resp_sendstr(req, "File uploaded successfully");

    return ESP_OK;
}

static esp_err_t file_delete_handler(httpd_req_t *req)
{
    char filepath[128] = {0};
    char filename[64] = {0};

    get_filename_from_path(req->uri, filename);

    strcpy(filepath, CFG_FILE_PATH);
    strcat(filepath, filename);

    ESP_LOGI(TAG, "filepath:%s", filepath);

    struct stat file_stat;

    if (stat(filepath, &file_stat) != 0) {
        ESP_LOGE(TAG, "File not exists : %s", filepath);
        /* Respond with 400 Bad Request */
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File not exists!");
        return ESP_FAIL;
    }

    unlink(filepath);

    storage_update_target_struct();

    // Send the response indicating success
    httpd_resp_sendstr(req, "File deleted successfully");

    return ESP_OK;

}

httpd_uri_t uri_get_main_page = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = website_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_get_logo = {
    .uri = "/esp_logo.svg",
    .method = HTTP_GET,
    .handler = esp_logo_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_get_favicon = {
    .uri = "/favicon.ico",
    .method = HTTP_GET,
    .handler = favicon_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_set_credentials = {
    .uri = "/set_credentials",
    .method = HTTP_POST,
    .handler = set_credentials_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_set_openocd_config = {
    .uri = "/set_openocd_config",
    .method = HTTP_POST,
    .handler = set_openocd_config_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_get_openocd_config = {
    .uri = "/get_openocd_config",
    .method = HTTP_GET,
    .handler = get_openocd_config_handler,
    .user_ctx = NULL
};

httpd_uri_t uri_file_upload = {
    .uri       = "/upload/*",
    .method    = HTTP_POST,
    .handler   = file_upload_handler,
    .user_ctx  = NULL
};

httpd_uri_t uri_file_delete = {
    .uri       = "/delete/*",
    .method    = HTTP_POST,
    .handler   = file_delete_handler,
    .user_ctx  = NULL
};

esp_err_t web_server_start(httpd_handle_t *http_handle)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    /* Running web server on other core to get better response time */
    config.core_id = 1;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 16;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(http_handle, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start web server!");
        return ESP_FAIL;
    }

    httpd_register_uri_handler(*http_handle, &uri_get_main_page);
    httpd_register_uri_handler(*http_handle, &uri_get_logo);
    httpd_register_uri_handler(*http_handle, &uri_get_favicon);
    httpd_register_uri_handler(*http_handle, &uri_set_credentials);
    httpd_register_uri_handler(*http_handle, &uri_set_openocd_config);
    httpd_register_uri_handler(*http_handle, &uri_get_openocd_config);
    httpd_register_uri_handler(*http_handle, &uri_file_upload);
    httpd_register_uri_handler(*http_handle, &uri_file_delete);

    return ESP_OK;
}
