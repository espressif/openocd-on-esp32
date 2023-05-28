#pragma once

#include "esp_log.h"
#include "esp_http_server.h"

esp_err_t web_server_start(httpd_handle_t *http_handle);
