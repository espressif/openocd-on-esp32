#include <unistd.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <stdbool.h>
#include <cJSON.h>

#include "esp_log.h"
#include "esp_check.h"

#include "storage.h"
#include "networking.h"
#include "web_server.h"
#include "ui/ui.h"
#include "log.h"

#define FAVICON_URI "/favicon.ico"
static const char *TAG = "web_server";

void web_server_alert(httpd_req_t *req, const char *msg, const char *file, int line)
{
    if (file) {
        // if sourcefile info is available (debug build)
        char text[128] = {0};
        snprintf(text, sizeof(text), "%s:%d %s", file, line, msg);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, text);
    } else {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg);
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
}

void web_server_add_option_into_dropdown(httpd_req_t *req, char *element_id, char *target_name)
{
    httpd_resp_sendstr_chunk(req, "<script>");
    httpd_resp_sendstr_chunk(req, "var dropdown = document.getElementById(\"");
    httpd_resp_sendstr_chunk(req, element_id);
    httpd_resp_sendstr_chunk(req, "\");");

    httpd_resp_sendstr_chunk(req, "var option = new Option(\"");
    httpd_resp_sendstr_chunk(req, target_name);
    httpd_resp_sendstr_chunk(req, "\",\"");
    char *file_param = NULL;
    ESP_OOCD_ERROR_CHECK(storage_get_target_path(target_name, &file_param), req, "Failed to get target path");
    httpd_resp_sendstr_chunk(req, file_param);
    httpd_resp_sendstr_chunk(req, "\");");
    httpd_resp_sendstr_chunk(req, "dropdown.add(option);");
    httpd_resp_sendstr_chunk(req, "</script>");
    free(file_param);
}

void web_server_set_checkbox(httpd_req_t *req, char *element_id, bool state)
{
    httpd_resp_sendstr_chunk(req, "<script>");
    httpd_resp_sendstr_chunk(req, "document.getElementById(\"");
    httpd_resp_sendstr_chunk(req, element_id);
    httpd_resp_sendstr_chunk(req, "\").checked = ");
    if (state) {
        httpd_resp_sendstr_chunk(req, "true;");
    } else {
        httpd_resp_sendstr_chunk(req, "false;");
    }
    httpd_resp_sendstr_chunk(req, "</script>");
}

void web_server_set_dropdown(httpd_req_t *req, char *element_id, int index)
{
    httpd_resp_sendstr_chunk(req, "<script>");
    httpd_resp_sendstr_chunk(req, "document.getElementById(\"");
    httpd_resp_sendstr_chunk(req, element_id);

    httpd_resp_sendstr_chunk(req, "\")");

    httpd_resp_sendstr_chunk(req, ".selectedIndex = ");
    char str_index[3] = {0};
    snprintf(str_index, sizeof(str_index), "%d", index);
    httpd_resp_sendstr_chunk(req, str_index);
    httpd_resp_sendstr_chunk(req, "</script>");
}

static esp_err_t send_response(httpd_req_t *req, const char *response_msg)
{
    esp_err_t ret = httpd_resp_send(req, response_msg, HTTPD_RESP_USE_STRLEN);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    return ret;
}

static esp_err_t common_handler(httpd_req_t *req, int recv_size, const char *response_msg)
{
    if (!req) {
        return ESP_FAIL;
    }

    int ret = httpd_req_recv(req, req->user_ctx, recv_size);
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }
    return send_response(req, response_msg);
}

static esp_err_t reset_credentials_handler(httpd_req_t *req)
{
    ESP_OOCD_ERROR_CHECK(storage_nvs_erase_key(SSID_KEY), req, "Failed to erase SSID");
    ESP_OOCD_ERROR_CHECK(storage_nvs_erase_key(PASSWORD_KEY), req, "Failed to erase password");
    ESP_OOCD_ERROR_CHECK(storage_nvs_erase_key(WIFI_MODE_KEY), req, "Failed to erase WiFi mode");

    int ret = send_response(req, "The credentials are being reset");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error on reset response %d", ret);
        return ret;
    }

    esp_restart();

    return ESP_OK;
}

static esp_err_t save_credentials_handler(httpd_req_t *req)
{
    /* ssid + password + delimiter */
    const int credentials_length = SSID_LENGTH + PASSWORD_LENGTH + 1;
    size_t recv_size = MIN(req->content_len, credentials_length);
    char buff[SSID_LENGTH + PASSWORD_LENGTH + 1] = {0};
    req->user_ctx = buff;

    esp_err_t ret = common_handler(req, recv_size, "The credentials are being saved");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to handle save credentials request");
        return ESP_FAIL;
    }

    cJSON *json = cJSON_ParseWithLength(buff, recv_size + 1);
    if (!json) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "Error before: %s\n", error_ptr);
        }
        return ESP_FAIL;
    }

    cJSON *ssid = cJSON_GetObjectItem(json, SSID_KEY);
    cJSON *pass = cJSON_GetObjectItem(json, PASSWORD_KEY);
    if (!pass || !ssid || !ssid->valuestring) {
        ESP_LOGE(TAG, "WiFi name is NULL");
        return ESP_FAIL;
    }
    OOCD_RETURN_ON_ERROR(storage_save_credentials(ssid->valuestring, pass->valuestring), TAG, "Error on saving credentials");
    cJSON_Delete(json);
    esp_restart();

    return ESP_OK;
}

static esp_err_t set_openocd_config_handler(httpd_req_t *req)
{
    size_t recv_size = req->content_len;
    /* Create an array sizeof content length + NULL and set last index as NULL */
    char buff[recv_size + 1]; buff[recv_size] = 0;

    req->user_ctx = buff;
    int ret = common_handler(req, recv_size, "OpenOCD config is being changed");
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to handle save config request");
        return ESP_FAIL;
    }

    storage_nvs_erase_key(OOCD_F_PARAM_KEY);
    storage_nvs_erase_key(OOCD_C_PARAM_KEY);
    storage_nvs_erase_key(OOCD_D_PARAM_KEY);

    cJSON *json = cJSON_ParseWithLength(buff, recv_size + 1);
    if (!json) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "Error before: %s\n", error_ptr);
        }
        return ESP_FAIL;
    }

    const char *keys[] = {OOCD_C_PARAM_KEY, OOCD_D_PARAM_KEY, OOCD_F_PARAM_KEY, DUAL_CORE_KEY, FLASH_SUPPORT_KEY, RTOS_KEY};
    for (int i = 0; i < sizeof(keys) / sizeof(char *); i++) {
        cJSON *param = cJSON_GetObjectItem(json, keys[i]);
        if (param != NULL && param->valuestring != NULL) {
            ret = storage_nvs_write(keys[i], param->valuestring, strlen(param->valuestring));
            if (ret != ESP_OK) {
                ESP_LOGE(TAG, "Failed to save %s parameter", keys[i]);
            }
        }
    }

    cJSON *ssid = cJSON_GetObjectItem(json, SSID_KEY);
    cJSON *pass = cJSON_GetObjectItem(json, PASSWORD_KEY);
    if (pass && ssid && ssid->valuestring) {
        ESP_LOGI(TAG, "Saving WiFi crendentials");
        OOCD_RETURN_ON_ERROR(storage_save_credentials(ssid->valuestring, pass->valuestring), TAG, "Error on saving credentials");
    }
    cJSON_Delete(json);

    vTaskDelay(200 / portTICK_PERIOD_MS);
    esp_restart();

    return ESP_OK;
}

static void add_targets(httpd_req_t *req)
{
    char *delimiter = "\n";
    char *tmp_target_list = storage_get_target_names(false);
    char *tmp_targets_str = calloc(strlen(tmp_target_list) + 1, sizeof(char));
    if (!tmp_targets_str) {
        ESP_LOGE(TAG, "Allocation error");
    } else {
        sprintf(tmp_targets_str, "%s", tmp_target_list);
        char *each_config_file = strtok(tmp_targets_str, delimiter);
        while (each_config_file != NULL)  {
            web_server_add_option_into_dropdown(req, "fCommand", each_config_file);
            each_config_file = strtok(NULL, delimiter);
        }
        free(tmp_targets_str);
    }
}

static void list_files(httpd_req_t *req)
{
    /* Send file-list table definition and column labels */
    int total_size = 0;
    int total_count = 0;
    char *config_files = storage_get_config_files(CONFIG_FILES_PATH, false, &total_count);
    if (!config_files) {
        ESP_LOGW(TAG, "Failed to fetch config file names");
    } else {
        char *delimiter = "\n";
        char *each_config_file = strtok(config_files, delimiter);

        httpd_resp_sendstr_chunk(req,
                                 "<table class=\"fixed\" border=\"1\">"
                                 "<col width=\"500px\"/><col width=\"300px\"/><col width=\"100px\"/>"
                                 "<thead><tr><th>Name</th><th>Size (Bytes)</th><th>Delete</th></tr></thead>"
                                 "<tbody>");

        char str[9] = {0};
        while (each_config_file != NULL)  {

            httpd_resp_sendstr_chunk(req, "<tr><td><a href=\"");
            httpd_resp_sendstr_chunk(req, req->uri);
            httpd_resp_sendstr_chunk(req, each_config_file);
            httpd_resp_sendstr_chunk(req, "\">");
            httpd_resp_sendstr_chunk(req, each_config_file);
            httpd_resp_sendstr_chunk(req, "</a></td><td>");

            int size = storage_get_file_size(each_config_file);
            total_size += size;
            snprintf(str, sizeof(str), "%d", size);

            httpd_resp_sendstr_chunk(req, str);

            httpd_resp_sendstr_chunk(req, "</td><td align=\"center\">");
            httpd_resp_sendstr_chunk(req, "<form method=\"post\" action=\"/delete");
            httpd_resp_sendstr_chunk(req, req->uri);
            httpd_resp_sendstr_chunk(req, each_config_file);
            httpd_resp_sendstr_chunk(req, "\"><button type=\"submit\">Delete</button></form>");
            httpd_resp_sendstr_chunk(req, "</td></tr>\n");

            each_config_file = strtok(NULL, delimiter);
        }
        free(config_files);
        httpd_resp_sendstr_chunk(req, "</tbody><tfoot>");
        httpd_resp_sendstr_chunk(req, "<tr><td>");
        httpd_resp_sendstr_chunk(req, "Total count: ");
        snprintf(str, sizeof(str), "%d", total_count);
        httpd_resp_sendstr_chunk(req, str);
        httpd_resp_sendstr_chunk(req, "</td><td>");
        snprintf(str, sizeof(str), "%d", total_size);
        httpd_resp_sendstr_chunk(req, str);
        httpd_resp_sendstr_chunk(req, "</td><td>");

        /* Finish the file list table */
        httpd_resp_sendstr_chunk(req, "</td></tfoot></table>");
    }
}

static esp_err_t load_main_page(httpd_req_t *req)
{
    /* Get handle to embedded file upload script */
    extern const unsigned char upload_script_start[] asm("_binary_web_page_html_start");
    extern const unsigned char upload_script_end[]   asm("_binary_web_page_html_end");
    const size_t upload_script_size = (upload_script_end - upload_script_start);

    /* Add file upload form and script which on execution sends a POST request to /upload */
    httpd_resp_send_chunk(req, (const char *)upload_script_start, upload_script_size);

    add_targets(req);
    list_files(req);
    web_server_set_dropdown(req, "fCommand", oocd_config.target_index);
    web_server_set_checkbox(req, "dualCore", oocd_config.dual_core);
    web_server_set_checkbox(req, "flashSupport", oocd_config.flash_support);
    web_server_set_dropdown(req, "rtos", oocd_config.rtos);
    web_server_set_dropdown(req, "dCommand", oocd_config.debug_level_index);
    /* Run the file param changes manually in first load */
    httpd_resp_sendstr_chunk(req, "<script>fileParamOnChange()</script>");

    /* Send remaining chunk of HTML file to complete it */
    httpd_resp_sendstr_chunk(req, "</html>");
    /* Send empty chunk to signal HTTP response completion */
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}

static esp_err_t get_path_from_uri(httpd_req_t *req, const char *uri, char **ret_file_path)
{
    char *filename = strstr(req->uri, uri);
    if (!filename) {
        ESP_LOGE(TAG, "String not found");
        return ESP_FAIL;
    }
    filename = (char *)req->uri + strlen(uri);

    if (filename[strlen(filename) - 1] == '/') {
        ESP_LOGE(TAG, "Invalid filename : %s", filename);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Invalid filename");
        return ESP_FAIL;
    }

    char *filepath = calloc(strlen(CONFIG_FILES_PATH) + strlen(filename) + 1, sizeof(char));
    strcpy(filepath, CONFIG_FILES_PATH);
    strcpy(filepath + strlen(CONFIG_FILES_PATH), filename);

    *ret_file_path = filepath;
    return ESP_OK;
}

static esp_err_t main_page_handler(httpd_req_t *req)
{
    if (req->uri[strlen(req->uri) - 1] == '/') {
        return load_main_page(req);
    } else if (strcmp(req->uri, FAVICON_URI) == 0) {
        return ESP_OK;
    }

    const char *main_uri = "/";
    char *filepath = NULL;
    OOCD_RETURN_ON_ERROR(get_path_from_uri(req, main_uri, &filepath), TAG, "Failed get path from URI '%s'!", main_uri);
    if (!filepath) {
        ESP_LOGE(TAG, "Allocation error");
        return ESP_FAIL;
    }
    char *filename = filepath + strlen(CONFIG_FILES_PATH);

    FILE *fd = fopen(filepath, "r");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to read existing file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to read existing file");
        free(filepath);
        return ESP_FAIL;
    }

    struct stat file_stat;
    stat(filepath, &file_stat);
    ESP_LOGI(TAG, "Sending file : %s (%ld bytes)...", filename, file_stat.st_size);
    httpd_resp_set_type(req, "text/plain");
    free(filepath);
    const int buffer_size = 1024;
    char chunk[buffer_size];
    size_t chunksize;
    do {
        chunksize = fread(chunk, 1, buffer_size, fd);

        if (chunksize > 0) {
            if (httpd_resp_send_chunk(req, chunk, chunksize) != ESP_OK) {
                fclose(fd);
                ESP_LOGE(TAG, "File sending failed!");
                httpd_resp_sendstr_chunk(req, NULL);
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to send file");
                return ESP_FAIL;
            }
        }
    } while (chunksize != 0);

    fclose(fd);
    ESP_LOGI(TAG, "File sending complete");
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t upload_post_handler(httpd_req_t *req)
{
    const char *upload_uri = "/upload/";
    char *filepath = NULL;
    OOCD_RETURN_ON_ERROR(get_path_from_uri(req, upload_uri, &filepath), TAG, "Failed get path from URI '%s'!", upload_uri);
    if (!filepath) {
        ESP_LOGE(TAG, "Allocation error");
        return ESP_FAIL;
    }
    char *filename = filepath + strlen(CONFIG_FILES_PATH);

    FILE *fd = fopen(filepath, "w");
    if (!fd) {
        ESP_LOGE(TAG, "Failed to create file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to create file");
        free(filepath);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "Receiving file : %s...", filename);

    const int buffer_size = 1024;
    char buf[buffer_size];

    int received;
    int remaining = req->content_len;

    while (remaining > 0) {
        ESP_LOGI(TAG, "Remaining size : %d", remaining);
        if ((received = httpd_req_recv(req, buf, MIN(remaining, buffer_size))) <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;
            }
            fclose(fd);
            unlink(filepath);
            free(filepath);
            ESP_LOGE(TAG, "File reception failed!");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to receive file");
            return ESP_FAIL;
        }
        if (received && (received != fwrite(buf, 1, received, fd))) {
            fclose(fd);
            unlink(filepath);
            free(filepath);
            ESP_LOGE(TAG, "File write failed!");
            httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to write file to storage");
            return ESP_FAIL;
        }
        remaining -= received;
    }

    oocd_config.target_index = 0;
    free(filepath);
    fclose(fd);
    ESP_LOGI(TAG, "File reception complete");
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "File uploaded successfully");
#if CONFIG_UI_ENABLE
    ui_set_target_menu();
#else
    storage_get_target_names(true);
#endif /* CONFIG_UI_ENABLE */
    return ESP_OK;
}

static esp_err_t delete_post_handler(httpd_req_t *req)
{
    const char *delete_uri = "/delete/";
    char *filepath = NULL;
    OOCD_RETURN_ON_ERROR(get_path_from_uri(req, delete_uri, &filepath), TAG, "Failed get path from URI '%s'!", delete_uri);
    if (!filepath) {
        ESP_LOGE(TAG, "Allocation error");
        return ESP_FAIL;
    }
    char *filename = filepath + strlen(CONFIG_FILES_PATH);

    struct stat file_stat;
    if (stat(filepath, &file_stat) == -1) {
        ESP_LOGE(TAG, "File does not exist : %s", filename);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File does not exist");
        free(filepath);
        return ESP_FAIL;
    }

    oocd_config.target_index = 0;
    ESP_LOGI(TAG, "Deleting file : %s", filename);
    unlink(filepath);
    free(filepath);
    httpd_resp_set_status(req, "303 See Other");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "File deleted successfully");
#if CONFIG_UI_ENABLE
    ui_set_target_menu();
#else
    storage_get_target_names(true);
#endif /* CONFIG_UI_ENABLE */
    return ESP_OK;
}

esp_err_t web_server_start(httpd_handle_t *server)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    /* Running web server on other core to get better response time */
    config.core_id = 1;
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 16;

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);
    if (httpd_start(server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start file server!");
        return ESP_FAIL;
    }
    /* URI handler to get main page */
    httpd_uri_t main_page = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = main_page_handler,
        .user_ctx  = NULL
    };
    OOCD_RETURN_ON_ERROR(httpd_register_uri_handler(*server, &main_page), TAG, "Failed to register %s uri handler", main_page.uri);

    /* URI handler for uploading files to server */
    httpd_uri_t file_upload = {
        .uri       = "/upload/*",
        .method    = HTTP_POST,
        .handler   = upload_post_handler,
        .user_ctx  = NULL
    };
    OOCD_RETURN_ON_ERROR(httpd_register_uri_handler(*server, &file_upload), TAG, "Failed to register %s uri handler", file_upload.uri);

    /* URI handler for deleting files from server */
    httpd_uri_t file_delete = {
        .uri       = "/delete/*",
        .method    = HTTP_POST,
        .handler   = delete_post_handler,
        .user_ctx  = NULL
    };
    OOCD_RETURN_ON_ERROR(httpd_register_uri_handler(*server, &file_delete), TAG, "Failed to register %s uri handler", file_delete.uri);

    /* URI handler to send credentials */
    httpd_uri_t save_credentials = {
        .uri       = "/save_credentials",
        .method    = HTTP_POST,
        .handler   = save_credentials_handler,
        .user_ctx  = NULL
    };
    OOCD_RETURN_ON_ERROR(httpd_register_uri_handler(*server, &save_credentials), TAG, "Failed to register %s uri handler", save_credentials.uri);

    /* URI handler to set OpenOCD parameters */
    httpd_uri_t set_openocd_config = {
        .uri       = "/set_openocd_config",
        .method    = HTTP_POST,
        .handler   = set_openocd_config_handler,
        .user_ctx  = NULL
    };
    OOCD_RETURN_ON_ERROR(httpd_register_uri_handler(*server, &set_openocd_config), TAG, "Failed to register %s uri handler", set_openocd_config.uri);

    /* URI handler to reset credentials */
    httpd_uri_t reset_credentials = {
        .uri       = "/reset_credentials",
        .method    = HTTP_POST,
        .handler   = reset_credentials_handler,
        .user_ctx  = NULL
    };
    OOCD_RETURN_ON_ERROR(httpd_register_uri_handler(*server, &reset_credentials), TAG, "Failed to register %s uri handler", reset_credentials.uri);

    return ESP_OK;
}
