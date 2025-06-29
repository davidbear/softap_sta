#ifndef SPIFFS_FILE_SERVER_H
#define SPIFFS_FILE_SERVER_H

#include "esp_http_server.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include <string.h>
#include "esp_vfs.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <lwip/inet.h>
#if IP_NAPT
#include "lwip/lwip_napt.h"
#endif
#include "esp_netif.h"


#ifdef __cplusplus
extern "C" {
#endif

httpd_handle_t start_spiffs_webserver(httpd_handle_t *server);
esp_err_t init_spiffs(void);

#ifdef __cplusplus
}
#endif

#endif // SPIFFS_FILE_SERVER_H
