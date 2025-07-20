#include "nvs_utils.h"

#define TAG "nvs_utils"
#define NVS_NAMESPACE "nvs"

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

esp_err_t check_nvs(const char* name, bool* var) 
{
    esp_err_t ret_flag = read_bool_from_nvs(name, var);
    if(ret_flag != ESP_OK) {
        if(ret_flag == ESP_ERR_NVS_NOT_FOUND) {
            *var = false;
            ret_flag = write_uint8_to_nvs(name, *var);
            ESP_LOGI(TAG,"%s not found, set to %d", name, *var);
        } else {
            ESP_LOGE(TAG,"%s read_uint8_from_nvs error: %d",name, ret_flag);
            return ret_flag;
        }
    }
    return ret_flag;
}
