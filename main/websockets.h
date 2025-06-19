// websockets.h
#ifndef WEBSOCKETS_H
#define WEBSOCKETS_H

#include "esp_http_server.h"

esp_err_t ws_handler(httpd_req_t *req);
esp_err_t register_ws_handler(httpd_handle_t server);

#endif
