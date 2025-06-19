#ifndef WEBSOCKETS_H
#define WEBSOCKETS_H

#define CONFIG_HTTPD_WS_SUPPORT
#include "esp_http_server.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_http_server.h"
#include <string.h>
#include <time.h>

#include "esp_log.h"

esp_err_t ws_handler(httpd_req_t *req);
esp_err_t register_ws_handler(httpd_handle_t server);

#endif // WEBSOCKETS_H
