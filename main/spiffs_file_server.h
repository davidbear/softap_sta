#ifndef SPIFFS_FILE_SERVER_H
#define SPIFFS_FILE_SERVER_H

#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include <string.h>
#include "esp_vfs.h"

#ifdef __cplusplus
extern "C" {
#endif

void start_spiffs_webserver(httpd_handle_t *server);
void init_spiffs(void);

#ifdef __cplusplus
}
#endif

#endif // SPIFFS_FILE_SERVER_H
