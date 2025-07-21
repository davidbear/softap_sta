# Components
idf_component_register(
        SRCS 
            "spiffs_file_server.c" 
            "softap_sta.c" 
            "control_led.c"
            "timekeeper.c"
            "provisioning.c"
            "nvs_utils.c"
            "lights.c"
        INCLUDE_DIRS "."
        REQUIRES 
            esp_http_server 
            esp_wifi 
            nvs_flash 
            mdns
            spiffs
            wifi_provisioning
#            espressif__qrcode 
            driver
)

spiffs_create_partition_image(storage ../main/spiffs FLASH_IN_PROJECT)
