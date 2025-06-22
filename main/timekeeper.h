#ifndef TIMEKEEPER_H
#define TIMEKEEPER_H
#include "esp_sntp.h"
#include "esp_log.h"
#include "esp_event.h"
#include "esp_netif_sntp.h"
#include "esp_wifi.h"
#include "esp_system.h"
#include <time.h>
#include <sys/time.h>

#ifdef __cplusplus
extern "C" {
#endif

void start_periodic_sntp(void);

#ifdef __cplusplus
}
#endif

#endif // TIMEKEEPER_H
