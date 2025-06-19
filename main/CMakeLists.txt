# Components
set(SRCS
    spiffs_file_server.c
    softap_sta.c
    websockets.c
)

idf_component_register(SRCS ${SRCS}
        INCLUDE_DIRS "."
        REQUIRES 
            esp_http_server 
            esp_https_server 
            esp_wifi 
            nvs_flash 
            mdns
            spiffs
            esp_timer
        EMBED_FILES
            cert/server.crt
            cert/server.key
            web/index.html
            web/style.css
            web/index.js
            web/favicon.ico
)

spiffs_create_partition_image(storage ../main/spiffs FLASH_IN_PROJECT)
# Embed web assets
#target_add_binary_data(${COMPONENT_TARGET} "web/index.html" BINARY)  "embedded_file_server.c"
#target_add_binary_data(${COMPONENT_TARGET} "web/style.css" BINARY)
#target_add_binary_data(${COMPONENT_TARGET} "web/index.js" BINARY)
#target_add_binary_data(${COMPONENT_TARGET} "web/favicon.ico" BINARY)
