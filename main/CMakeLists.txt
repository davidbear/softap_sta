# Components
idf_component_register(SRCS "softap_sta.c" "embedded_file_server.c"
        INCLUDE_DIRS "."
        REQUIRES 
            esp_http_server 
            esp_https_server 
            esp_wifi 
            nvs_flash 
            mdns
        EMBED_FILES
            cert/server.crt
            cert/server.key
            web/index.html
            web/style.css
            web/index.js
            web/favicon.ico
)

# Embed web assets
#target_add_binary_data(${COMPONENT_TARGET} "web/index.html" BINARY)
#target_add_binary_data(${COMPONENT_TARGET} "web/style.css" BINARY)
#target_add_binary_data(${COMPONENT_TARGET} "web/index.js" BINARY)
#target_add_binary_data(${COMPONENT_TARGET} "web/favicon.ico" BINARY)
