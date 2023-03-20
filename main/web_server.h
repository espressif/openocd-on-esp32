#ifndef __WEB_SERVER_H__
#define __WEB_SERVER_H__

#include "esp_http_server.h"

esp_err_t web_server_start(httpd_handle_t *server);
void web_server_alert(httpd_req_t *req, const char *file, int line);

#endif
