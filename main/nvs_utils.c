#include "nvs_utils.h"

#define TAG "nvs_utils"
#define NVS_NAMESPACE "storage"

esp_err_t write_uint8_to_nvs(const char *key, uint8_t value) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_u8(nvs_handle, key, value);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write value (%s)!", esp_err_to_name(err));
        nvs_close(nvs_handle);
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit changes (%s)!", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t read_uint8_from_nvs(const char *key, uint8_t *value) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    err = nvs_get_u8(nvs_handle, key, value);
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "Read value: %u", *value);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG, "The value is not initialized yet!");
            break;
        default:
            ESP_LOGE(TAG, "Error (%s) reading value!", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}

esp_err_t read_bool_from_nvs(const char *key, bool *value) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }
    uint8_t val8;
    err = nvs_get_u8(nvs_handle, key, &val8);
    *value = val8;
    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "Read value: %u", *value);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            ESP_LOGW(TAG, "The value is not initialized yet!");
            break;
        default:
            ESP_LOGE(TAG, "Error (%s) reading value!", esp_err_to_name(err));
    }

    nvs_close(nvs_handle);
    return err;
}
