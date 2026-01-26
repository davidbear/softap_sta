#include "nvs_utils.h"

#define TAG "nvs_utils"
#define NVS_NAMESPACE "nvs"

/*
*   nvs_set_i8, nvs_set_u8, nvs_set_i16, nvs_set_u16, nvs_set_i32, nvs_set_u32
*/

esp_err_t write_nvs_integer(nvs_type_t type, const char *key, uint32_t value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    switch (type) {
        case NVS_TYPE_I8:   err = nvs_set_i8(handle, key, (int8_t)value); break;
        case NVS_TYPE_U8:   err = nvs_set_u8(handle, key, (uint8_t)value); break;
        case NVS_TYPE_I16:  err = nvs_set_i16(handle, key, (int16_t)value); break;
        case NVS_TYPE_U16:  err = nvs_set_u16(handle, key, (uint16_t)value); break;
        case NVS_TYPE_I32:  err = nvs_set_i32(handle, key, (int32_t)value); break;
        case NVS_TYPE_U32:  err = nvs_set_u32(handle, key, (uint32_t)value); break;
        default: err = ESP_ERR_INVALID_ARG; break;
    }
    ESP_LOGI(TAG, "key: %s; value: %d; err: %s", key, value,  esp_err_to_name(err));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err;
}

esp_err_t read_nvs_integer(nvs_type_t type, const char *key, uint32_t *value) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    uint32_t old_val = *value;
    int8_t i8;
    uint8_t u8;
    int16_t i16;
    uint16_t u16;
    switch (type) {
        case NVS_TYPE_I8:
            err = nvs_get_i8(handle, key, &i8);
            *value = (int32_t)i8;
            break;
        case NVS_TYPE_U8:
            err = nvs_get_u8(handle, key, &u8);
            *value = (uint32_t)u8;
            break;
        case NVS_TYPE_I16:  
            err = nvs_get_i16(handle, key, &i16);
            *value = (int32_t)i16;
            break;
        case NVS_TYPE_U16:  
            err = nvs_get_u16(handle, key, &u16);
            *value = (uint32_t)u16;
            break;
        case NVS_TYPE_I32:  
            err = nvs_get_i32(handle, key, (int32_t *)value); 
            break;
        case NVS_TYPE_U32:  
            err = nvs_get_u32(handle, key, value); 
            break;
        default:
            err = ESP_ERR_INVALID_ARG;
            break;
    }

    switch (err) {
        case ESP_OK:
            ESP_LOGI(TAG, "Read value: %u", *value);
            nvs_close(handle);
            break;
        case ESP_ERR_NVS_NOT_FOUND:
            *value = old_val;
            ESP_LOGW(TAG, "The value is not initialized yet! Initializing to %d", (int)*value);
            nvs_close(handle);
            err = write_nvs_integer(type, key, *value);
            break;
        default:
            *value = old_val;
            ESP_LOGE(TAG, "Error (%s) reading value!", esp_err_to_name(err));
            return err;
    }
    return err;
}

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

esp_err_t write_int8_to_nvs(const char *key, uint8_t value) {
    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return err;
    }

    err = nvs_set_i8(nvs_handle, key, value);
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
