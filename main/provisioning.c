#include "provisioning.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"
#include "nvs_utils.h"
//#include <iostream.h>
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"
//#include "qrcode.h"
//#include <cstring>

#define MAX_RETRY_NUM 5

struct AppContext {
    const char* tag;
};

static struct AppContext app_ctx = {
    .tag = "ProvisioningApp"
};

static const char* TAG = "PROV";

static void wifi_prov_handler(void *user_data, wifi_prov_cb_event_t event, void *event_data)
{
    switch(event)
    {
        case WIFI_PROV_START :
            ESP_LOGI(TAG,"WIFI_PROV_START");
            break;
        case WIFI_PROV_CRED_RECV :
            ESP_LOGI(TAG,"cred: ssid: %s pass: %s", 
                ((wifi_sta_config_t *)event_data)->ssid, 
                ((wifi_sta_config_t *)event_data)->password);
            break;
        case WIFI_PROV_CRED_SUCCESS :
            ESP_LOGI(TAG,"Provision Success");
            wifi_prov_mgr_stop_provisioning();
            ESP_LOGI(TAG,"After wifi_prov_mgr_stop_provisioning");
            vTaskDelay(pdMS_TO_TICKS(5000));
            esp_wifi_disconnect();
            ESP_LOGI(TAG,"After esp_wifi_disconnect");
            esp_system_abort("Leaving Provisioning");
            esp_restart();
            break;
        case WIFI_PROV_CRED_FAIL :
            ESP_LOGE(TAG,"WIFI_PROV_CRED_FAIL");
            esp_system_abort("Failed Provisioning");
            esp_restart();
            break;
        case WIFI_PROV_END :
            ESP_LOGI(TAG,"Provisioning has ended");
            wifi_prov_mgr_wait();
            wifi_prov_mgr_deinit();
            break;
        default: break;
    };
}

static void wifi_event_handler(void* event_handler_arg,
                                    esp_event_base_t event_base,
                                    int32_t event_id,
                                    void* event_data)
{
    static int retry_cnt = 0;
    if(event_base == WIFI_EVENT)
    {
        switch(event_id)
        {
            case WIFI_EVENT_STA_START :
                ESP_LOGI(TAG, "ESP32 STARTING");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED :
                ESP_LOGE(TAG, "ESP32 Disconnected, retrying");
                retry_cnt++;
                if(retry_cnt < MAX_RETRY_NUM) {
                    esp_wifi_connect();
                }
                else ESP_LOGE(TAG,"Connection error");
                break;
            default : break;
        }
    }
    else if (event_base == IP_EVENT) 
    {
        if(event_id == IP_EVENT_STA_GOT_IP) 
        {
            ip_event_got_ip_t* event = (ip_event_got_ip_t*)event_data;
            ESP_LOGI(TAG,"ip address = " IPSTR,IP2STR(&event->ip_info.ip));
            esp_event_post(WIFI_PROV_EVENT, WIFI_PROV_CRED_SUCCESS, NULL, 0, portMAX_DELAY);
        }
    }
}

void wifi_hw_init(void)
{
    nvs_flash_init();
    esp_netif_init();
    esp_event_loop_create_default();
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    esp_event_handler_instance_register(WIFI_EVENT,
                                        ESP_EVENT_ANY_ID,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_any_id);
    esp_event_handler_instance_register(IP_EVENT,
                                        IP_EVENT_STA_GOT_IP,
                                        &wifi_event_handler,
                                        NULL,
                                        &instance_got_ip);
    esp_netif_t *handle = esp_netif_create_default_wifi_sta();
    assert(handle);
    esp_netif_set_hostname(handle, "clr_clk");
    esp_netif_create_default_wifi_ap();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    esp_wifi_start();
    const char* old_hostname; 
    esp_netif_get_hostname(handle, &old_hostname);
    ESP_LOGI(TAG, "name = %s",old_hostname);
}

static void prov_start(void)
{
    wifi_prov_mgr_config_t cfg = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = {},
        .app_event_handler = {
            .event_cb = wifi_prov_handler,
            .user_data = &app_ctx
        },
//        .wifi_prov_conn_cfg = 0
    };

    wifi_prov_mgr_init(cfg);
    bool is_provisioned = 0;
    wifi_prov_mgr_is_provisioned(&is_provisioned);
    wifi_prov_mgr_disable_auto_stop(100);
    if(is_provisioned)
    {
        ESP_LOGI(TAG,"Already provisioned");
        ESP_ERROR_CHECK(wifi_prov_mgr_reset_provisioning());
    }
    wifi_prov_mgr_disable_auto_stop(100);
    wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_1, "color1234", "ESP32S3-PROV", NULL);
}

void start_provisioning(void) {
    wifi_hw_init();
    prov_start();
    vTaskDelay(portMAX_DELAY );  // forever
}

void init_provisioning(void) {
    wifi_prov_mgr_config_t config = {
        .scheme = wifi_prov_scheme_softap,
        .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE,
        .app_event_handler = {
            .event_cb = wifi_prov_handler,
            .user_data = NULL
        }
    };
//    config.wifi_prov_conn_cfg = {};

    ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
}

