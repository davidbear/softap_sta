#ifndef NVS_UTILS_H
#define NVS_UTILS_H

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"

typedef enum {
    AP_PASSTHROUGH,         // Indicates that folks logged into the AP can see the internet
    DO_PROVISION,     // Start provisioning on reset.
} flag_t;

/*
typedef enum  {
    NVS_TYPE_I8,
    NVS_TYPE_U8,
    NVS_TYPE_I16,
    NVS_TYPE_U16,
    NVS_TYPE_I32,
    NVS_TYPE_U32
} nvs_var_type_t;
*/

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t write_nvs_integer(nvs_type_t type, const char *key, uint32_t value);
esp_err_t read_nvs_integer(nvs_type_t type, const char *key, uint32_t *value);
esp_err_t read_uint8_from_nvs(const char *key, uint8_t *value);
esp_err_t read_bool_from_nvs(const char *key, bool *value);
esp_err_t write_uint8_to_nvs(const char *key, uint8_t value);
esp_err_t check_nvs(const char* name, bool* var);

#ifdef __cplusplus
}
#endif

#endif  //  NVS_UTILS_H