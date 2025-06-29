/*
 * This code is based on a number of example files
 */

/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 * 
 *  WiFi softAP & station Example
*/
#include "softap_sta.h"
#include "spiffs_file_server.h"
#include "control_led.h"
#include "timekeeper.h"
#include "provisioning.h"
#include "nvs_utils.h"


static const char *TAG_AP = "WiFi SoftAP";
static const char *TAG_STA = "WiFi Sta";

static int s_retry_num = 0;
httpd_handle_t server = NULL;
bool led_state = false;
bool conn_state = true;
bool sta_state = false;
bool do_prov = false;
bool provisioned = false;

// Global AP netif pointer (for use in handlers)
esp_netif_t *g_esp_netif_ap = NULL;

/* FreeRTOS event group to signal when we are connected/disconnected */
static EventGroupHandle_t s_wifi_event_group;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT)
    {
        switch (event_id)
        {
        case WIFI_EVENT_AP_STACONNECTED:
            wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *)event_data;
            ESP_LOGI(TAG_AP, "Station " MACSTR " joined, AID=%d",
                     MAC2STR(event->mac), event->aid);
            break;
        case WIFI_EVENT_STA_START:
            esp_wifi_connect();
            ESP_LOGI(TAG_STA, "Station started");
            break;
        case WIFI_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG_STA, "STA disconnected. Stopping SNTP.");
            sta_state = false;
            esp_sntp_stop();
            // NEW: Ensure NAPT is off
            if (esp_netif_napt_disable(g_esp_netif_ap) == ESP_OK)
            {
                ESP_LOGI(TAG_STA, "NAPT disabled after STA disconnect");
            }
        }
    }
    else if (event_base == IP_EVENT)
    {
        switch (event_id)
        {
        case IP_EVENT_STA_GOT_IP:
            ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
            ESP_LOGI(TAG_STA, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
            s_retry_num = 0;
            sta_state = true;
            start_periodic_sntp();
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
            // NEW: NAT control logic
            if (conn_state)
            {
                if (esp_netif_napt_enable(g_esp_netif_ap) == ESP_OK)
                {
                    ESP_LOGI(TAG_STA, "NAPT enabled due to conn_state");
                }
                else
                {
                    ESP_LOGW(TAG_STA, "Failed to enable NAPT");
                }
            }
            else
            {
                if (esp_netif_napt_disable(g_esp_netif_ap) == ESP_OK)
                {
                    ESP_LOGI(TAG_STA, "NAPT disabled due to conn_state");
                }
                else
                {
                    ESP_LOGW(TAG_STA, "Failed to disable NAPT");
                }
            }
        }
    }
}

/* Initialize soft AP */
esp_netif_t *wifi_init_softap(void)
{
    const ip4_addr_t ap_ip_address = {
        .addr = PP_HTONL(LWIP_MAKEU32(1, 5, 168, 192)) 
    };

    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();
    esp_netif_ip_info_t ip_info;
    esp_netif_set_ip4_addr(&ip_info.ip, //192, 168, 5, 1);
        ip4_addr4(&ap_ip_address), 
        ip4_addr3(&ap_ip_address), 
        ip4_addr2(&ap_ip_address), 
        ip4_addr1(&ap_ip_address));
    esp_netif_set_ip4_addr(&ip_info.gw, //192, 168, 5, 1);
        ip4_addr4(&ap_ip_address), 
        ip4_addr3(&ap_ip_address), 
        ip4_addr2(&ap_ip_address), 
        ip4_addr1(&ap_ip_address));

    esp_netif_set_ip4_addr(&ip_info.netmask, 255, 255, 255, 0);
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(esp_netif_ap));
    ESP_ERROR_CHECK(esp_netif_set_ip_info(esp_netif_ap, &ip_info));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(esp_netif_ap));

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_AP_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_AP_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_AP_PASSWD,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };

    if (strlen(EXAMPLE_ESP_WIFI_AP_PASSWD) == 0) {
        wifi_ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI(TAG_AP, "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_AP_SSID, EXAMPLE_ESP_WIFI_AP_PASSWD, EXAMPLE_ESP_WIFI_CHANNEL);

    return esp_netif_ap;
}

/* Initialize wifi station */
esp_netif_t *wifi_init_sta(void)
{
    esp_netif_t *esp_netif_sta = esp_netif_create_default_wifi_sta();
    assert(esp_netif_sta);
    esp_netif_set_hostname(esp_netif_sta, "clr_clk");

    init_provisioning();

    wifi_config_t wifi_sta_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_STA_SSID,
            .password = EXAMPLE_ESP_WIFI_STA_PASSWD,
            .scan_method = WIFI_ALL_CHANNEL_SCAN,
            .failure_retry_cnt = EXAMPLE_ESP_MAXIMUM_RETRY,
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
        },
    };

    ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
    if(provisioned) {
        memset(&wifi_sta_config, 0, sizeof(wifi_config_t));
        esp_err_t get_config_ret = esp_wifi_get_config (WIFI_IF_STA, &wifi_sta_config);
        if(get_config_ret != 0) {
            ESP_LOGE(TAG_STA,"Error getting provisions -> %d", get_config_ret);
        }
        ESP_LOGI(TAG_STA,"Provisioned SSID: %s",wifi_sta_config.sta.ssid);
        ESP_LOGI(TAG_STA,"Provisioned password: %s",wifi_sta_config.sta.password);
    }
    else {
        ESP_LOGI(TAG_STA,"Not Provisioned using defaults");
    }

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_sta_config) );

    ESP_LOGI(TAG_STA, "wifi_init_sta finished.");

    return esp_netif_sta;
}

void softap_set_dns_addr(esp_netif_t *esp_netif_ap,esp_netif_t *esp_netif_sta)
{
    esp_netif_dns_info_t dns;
    esp_netif_get_dns_info(esp_netif_sta,ESP_NETIF_DNS_MAIN,&dns);
    uint8_t dhcps_offer_option = DHCPS_OFFER_DNS;
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_stop(esp_netif_ap));
    ESP_ERROR_CHECK(esp_netif_dhcps_option(esp_netif_ap, ESP_NETIF_OP_SET, ESP_NETIF_DOMAIN_NAME_SERVER, &dhcps_offer_option, sizeof(dhcps_offer_option)));
    ESP_ERROR_CHECK(esp_netif_set_dns_info(esp_netif_ap, ESP_NETIF_DNS_MAIN, &dns));
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_netif_dhcps_start(esp_netif_ap));
}

void start_mdns_service(void) {
    ESP_ERROR_CHECK(mdns_init());
    ESP_ERROR_CHECK(mdns_hostname_set("esp32s3"));
    ESP_ERROR_CHECK(mdns_instance_name_set("ESP32-S3 Web Server"));

    // Optional: Register HTTP service on port 80 or 443
    ESP_ERROR_CHECK(mdns_service_add("ESP Web", "_http", "_tcp", 80, NULL, 0));
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    esp_err_t ret_flag = read_bool_from_nvs("do_prov", &do_prov);
    if(ret_flag != ESP_OK) {
        if(ret_flag == ESP_ERR_NVS_NOT_FOUND) {
            do_prov = false;
            ret_flag = write_uint8_to_nvs("do_prov", do_prov);
            ESP_LOGI(TAG_AP,"do_prov not found, set to %d", do_prov);
        } else {
            ESP_LOGE(TAG_AP,"read_uint8_from_nvs error: %d",ret_flag);
            return;
        }
    }
    ret_flag = read_bool_from_nvs("conn_state", &conn_state);
    if(ret_flag == ESP_ERR_NVS_NOT_FOUND ) {
        conn_state = true;
        ret_flag = write_uint8_to_nvs("conn_state", conn_state);
    } else if (ret_flag != ESP_OK) {
        ESP_LOGE(TAG_AP,"read/write_uint8_from_nvs error: %d",ret_flag);
        return;
    }
    ESP_LOGI(TAG_STA,"do_prov: %d, conn_state: %d",do_prov, conn_state);

    if(do_prov) {
        ESP_LOGI(TAG_AP,"Provisioning Begins...");
        ret_flag = write_uint8_to_nvs("do_prov", false);
        for(int i=10;i>0;--i) {
            vTaskDelay(pdMS_TO_TICKS(1000));  // 1 second
            printf("%d\n",i);
        }
        start_provisioning();
        esp_restart();
    }

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* Initialize event group */
    s_wifi_event_group = xEventGroupCreate();

    /* Register Event handler */
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                    ESP_EVENT_ANY_ID,
                    &wifi_event_handler,
                    NULL,
                    NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                    IP_EVENT_STA_GOT_IP,
                    &wifi_event_handler,
                    NULL,
                    NULL));

    /*Initialize WiFi */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));

    /* Initialize AP */
    ESP_LOGI(TAG_AP, "ESP_WIFI_MODE_AP");
//    esp_netif_t *esp_netif_ap = wifi_init_softap();

    // Assign the return to global var here (INSIDE function)
    g_esp_netif_ap = wifi_init_softap();

    /* Initialize STA */
    ESP_LOGI(TAG_STA, "ESP_WIFI_MODE_STA");
    esp_netif_t *esp_netif_sta = wifi_init_sta();

    /* Start WiFi */
    ESP_ERROR_CHECK(esp_wifi_start() );
    const char* old_hostname; 
    esp_netif_get_hostname(esp_netif_sta, &old_hostname);
    ESP_LOGI(TAG_STA, "name = %s",old_hostname);

//  Start spiffs web server unconditionally
    ESP_ERROR_CHECK(init_spiffs());
    server = start_spiffs_webserver(&server);
    if(!server) {
        ESP_LOGE(TAG_AP,"Failed to start server");
        return;
    }

    // Configure LEDs 
    configure_led();
    ESP_LOGI(TAG_AP,"Server started and LED configured");

    /*
     * Wait until either the connection is established (WIFI_CONNECTED_BIT) or
     * connection failed for the maximum number of re-tries (WIFI_FAIL_BIT).
     * The bits are set by event_handler() (see above)
     */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           pdMS_TO_TICKS(10000));

    /* xEventGroupWaitBits() returns the bits before the call returned,
     * hence we can test which event actually happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG_STA, "Connected to STA network, setting DNS...");
        softap_set_dns_addr(g_esp_netif_ap, esp_netif_sta);
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGW(TAG_STA, "Failed to connect to STA. Continuing in SoftAP-only mode.");
    }
    else
    {
        ESP_LOGW(TAG_STA, "No STA event received in timeout. Continuing anyway.");
    }
    /* Set sta as the default interface */
    esp_netif_set_default_netif(esp_netif_sta);

    /* Enable napt on the AP netif */
    if (conn_state) {
        if (esp_netif_napt_enable(g_esp_netif_ap) != ESP_OK) {
            ESP_LOGE(TAG_STA, "NAPT not enabled on the netif: %p", g_esp_netif_ap);
        }
    } else {
        ESP_LOGI(TAG_STA,"Not reconnecting to internet");
    }

    start_mdns_service();;
}
