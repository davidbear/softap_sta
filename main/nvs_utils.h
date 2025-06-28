#ifndef NVS_UTILS_H
#define NVS_UTILS_H

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

typedef enum {
    AP_PASSTHROUGH,         // Indicates that folks logged into the AP can see the internet
    DO_PROVISION,     // Start provisioning on reset.
} flag_t;

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t read_uint8_from_nvs(const char *key, uint8_t *value);
esp_err_t write_uint8_to_nvs(const char *key, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif  //  NVS_UTILS_H