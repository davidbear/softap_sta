#ifndef EMBEDDED_FILE_SERVER_H
#define EMBEDDED_FILE_SERVER_H

#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

void start_embedded_webserver(httpd_handle_t *server);

#ifdef __cplusplus
}
#endif

#endif // EMBEDDED_FILE_SERVER_H
