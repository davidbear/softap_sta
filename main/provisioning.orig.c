#include "provisioning.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"
#include "nvs_utils.h"

#define TAG_PROV "PROV"

extern bool provisioned;
extern bool do_prov;

static void provisioning_event_handler(void *ctx, wifi_prov_cb_event_t event, void *event_data) {
    switch (event) {
        case WIFI_PROV_START:
            ESP_LOGI(TAG_PROV, "Provisioning started");
            break;
        case WIFI_PROV_CRED_RECV: {
            wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
            ESP_LOGI(TAG_PROV, "Received SSID: %s", wifi_sta_cfg->ssid);
            break;
        }
        case WIFI_PROV_CRED_SUCCESS:
            ESP_LOGI(TAG_PROV, "Provisioning successful");
            break;
        case WIFI_PROV_CRED_FAIL:
            ESP_LOGW(TAG_PROV, "Provisioning failed");
//            wifi_prov_mgr_reset_sm_state_on_failure();
            break;
        case WIFI_PROV_END:
            ESP_LOGI(TAG_PROV, "Provisioning ended");
            do_prov = true;
            ESP_ERROR_CHECK(write_uint8_to_nvs("do_prov", do_prov));
            wifi_prov_mgr_deinit();
            esp_restart();
            break;
        default:
            break;
    }
}

void init_provisioning(void) {
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
        .app_event_handler = {
            .event_cb = provisioning_event_handler,
            .user_data = NULL
        }
    };
//    config.wifi_prov_conn_cfg = {};

    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
}

void start_provisioning(void) {

    ESP_LOGI(TAG_PROV, "Starting provisioning...");
    const char *service_name = "ESP32-PROV";
    const char *service_key  = "color1234"; 

    ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, service_key, service_name, NULL));
    ESP_LOGI(TAG_PROV, "Provisioning started, waiting for client...");
    vTaskDelay(pdMS_TO_TICKS(10000));  // 10 seconds
}

void trigger_provisioning_via_websocket(void) {
    do_prov = !do_prov;
    if(write_uint8_to_nvs("do_prov", do_prov) == ESP_OK) {
        ESP_LOGI(TAG_PROV, "Set do_prov flag. Restarting...");
        vTaskDelay(pdMS_TO_TICKS(100));
        esp_restart();
    }
}
