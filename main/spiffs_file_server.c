//#include "esp_https_server.h"
#include "spiffs_file_server.h"
#include "control_led.h"
#include "nvs_utils.h"

static const char *TAG = "SPIffs";
static bool ws_registered = false;

extern httpd_handle_t server;
extern const ip4_addr_t ap_ip_address;
extern bool led_state;
extern bool conn_state;
extern bool sta_state;
extern esp_netif_t *g_esp_netif_ap;

struct async_resp_arg {
    httpd_handle_t hd;
    int fd;
};

#define FILE_PATH_MAX (ESP_VFS_PATH_MAX + 128)
#define BASE_PATH "/spiffs"

static const char *get_content_type(const char *filename) {
    if (strstr(filename, ".html")) return "text/html";
    if (strstr(filename, ".css")) return "text/css";
    if (strstr(filename, ".js")) return "application/javascript";
    if (strstr(filename, ".ico")) return "image/x-icon";
    if (strstr(filename, ".png")) return "image/png";
    if (strstr(filename, ".jpg")) return "image/jpeg";
    if (strstr(filename, ".svg")) return "image/svg+xml";
    return "text/plain";
}

static esp_err_t file_get_handler(httpd_req_t *req) {
    const char *uri = strcmp(req->uri, "/") == 0 ? "/index.html" : req->uri;
    char *term = strrchr(uri,'?');
    if(term != NULL) {
        ESP_LOGI(TAG, "Truncating: %s", uri);
        *term='\0';
    }

    ESP_LOGI(TAG,"Serving: %s",uri);
    char filepath[FILE_PATH_MAX];
    strlcpy(filepath, BASE_PATH, sizeof(filepath));
    strlcat(filepath, uri, sizeof(filepath));
    FILE *file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGW(TAG, "File not found: %s", filepath);
        // Try to serve 404.html if it exists
        snprintf(filepath, sizeof(filepath), BASE_PATH"/404.html");
        file = fopen(filepath, "r");
        if (!file) {
            const char *not_found_msg = "<html><body><h1>404 - File Not Found</h1></body></html>";
            httpd_resp_set_status(req, "404 Not Found");
            httpd_resp_set_type(req, "text/html");
            httpd_resp_send(req, not_found_msg, strlen(not_found_msg));
            return ESP_OK;
        }
        httpd_resp_set_status(req, "404 Not Found");
    }

    fseek(file, 0, SEEK_END);
    size_t filesize = ftell(file);
    rewind(file);

    httpd_resp_set_type(req, get_content_type(filepath));
    httpd_resp_set_hdr(req, "Cache-Control", "max-age=3600");
    char len_str[16];
    snprintf(len_str, sizeof(len_str), "%d", (int)filesize);
    httpd_resp_set_hdr(req, "Content-Length", len_str);

    if (filesize < 8192) {
        char *buf = malloc(filesize);
        if (!buf) {
            fclose(file);
            return ESP_ERR_NO_MEM;
        }
        fread(buf, 1, filesize, file);
        fclose(file);
        httpd_resp_send(req, buf, filesize);
        free(buf);
        return ESP_OK;
    } else {
        char chunk[1024];
        size_t chunksize;
        while ((chunksize = fread(chunk, 1, sizeof(chunk), file)) > 0) {
            httpd_resp_send_chunk(req, chunk, chunksize);
        }
        fclose(file);
        httpd_resp_send_chunk(req, NULL, 0);
        return ESP_OK;
    }
}

static void make_send_packet(void *arg, char *buff) 
{
    httpd_ws_frame_t ws_pkt;
    struct async_resp_arg *resp_arg = arg;
    httpd_handle_t hd = resp_arg->hd;

    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t *)buff;
    ws_pkt.len = strlen(buff);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ESP_LOGI(TAG, "ws_pkt.payload = '%s'", ws_pkt.payload);

    static size_t max_clients = CONFIG_LWIP_MAX_LISTENING_TCP;
    size_t fds = max_clients;
    int client_fds[max_clients];

    esp_err_t ret = httpd_get_client_list(server, &fds, client_fds);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG,"httpd_get_client_list failed");
        return;
    }

    for (int i = 0; i < fds; i++) {
        int client_info = httpd_ws_get_fd_info(server, client_fds[i]);
        if (client_info == HTTPD_WS_CLIENT_WEBSOCKET) {
            httpd_ws_send_frame_async(hd, client_fds[i], &ws_pkt);
        }
    }
}

static void ws_async_send(void *arg)
{
    ESP_LOGI(TAG,"ws_async_send");
//    httpd_ws_frame_t ws_pkt;
//    struct async_resp_arg *resp_arg = arg;
//    httpd_handle_t hd = resp_arg->hd;
//    int fd = resp_arg->fd;

//    gpio_set_level(LED_PIN, led_state);
    blink_led();
    
    char buff[40];

    memset(buff, 0, sizeof(buff));
    sprintf(buff, "T%d",led_state);
    make_send_packet(arg, buff);

    memset(buff, 0, sizeof(buff));
    sprintf(buff, "conn:%d",conn_state);
    make_send_packet(arg, buff);

    memset(buff, 0, sizeof(buff));
    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long js_time = tv.tv_sec*1000+tv.tv_usec/1000;
    sprintf(buff, "time:%lld",js_time);

    make_send_packet(arg, buff);
    free(arg);
}

static esp_err_t trigger_async_send(httpd_handle_t handle, httpd_req_t *req)
{
    ESP_LOGD(TAG,"trigger_async_send");
    struct async_resp_arg *resp_arg = malloc(sizeof(struct async_resp_arg));
    resp_arg->hd = req->handle;
    resp_arg->fd = httpd_req_to_sockfd(req);
    return httpd_queue_work(handle, ws_async_send, resp_arg);
}

void set_conn_state(bool new_state) {
    conn_state = new_state;
    if (conn_state) {
        esp_netif_napt_enable(g_esp_netif_ap);
        esp_err_t ret_flag = write_uint8_to_nvs("conn_state", conn_state);
        if (ret_flag != ESP_OK) {
            ESP_LOGE(TAG,"conn_state write_uint8_from_nvs error: %d",ret_flag);
        }
        ESP_LOGI("CONN_CTRL", "conn_state ON: Internet sharing enabled");
    } else {
        esp_netif_napt_disable(g_esp_netif_ap);
        esp_err_t ret_flag = write_uint8_to_nvs("conn_state", conn_state);
        if (ret_flag != ESP_OK) {
            ESP_LOGE(TAG,"conn_state write_uint8_from_nvs error: %d",ret_flag);
        }
        ESP_LOGI("CONN_CTRL", "conn_state OFF: Internet sharing disabled");
    }
}

esp_err_t handle_ws_req(httpd_req_t *req)
{
    ESP_LOGD(TAG, "Entered handle_ws_req");

    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");

        struct sockaddr_in6 addr6;
        socklen_t len = sizeof(addr6);
        getpeername(httpd_req_to_sockfd(req), (struct sockaddr *)&addr6, &len);

        char ip_str[INET6_ADDRSTRLEN] = {0};
        struct in_addr ipv4 = {.s_addr = 0};

        if (IN6_IS_ADDR_V4MAPPED(&addr6.sin6_addr)) {
            memcpy(&ipv4, &addr6.sin6_addr.s6_addr[12], sizeof(ipv4));
            inet_ntop(AF_INET, &ipv4, ip_str, sizeof(ip_str));
        } else {
            inet_ntop(AF_INET6, &addr6.sin6_addr, ip_str, sizeof(ip_str));
        }

        char ap_status[5] = "AP:0";
        if (strstr(ip_str, "192.168.5.") != NULL) {
            ap_status[3] = '1';
        }

        ESP_LOGI(TAG, "Client IP: %s, AP: %c", ip_str, ap_status[3]);

        httpd_ws_frame_t ip_pkt = {
            .payload = (uint8_t *)ap_status,
            .len = strlen(ap_status),
            .type = HTTPD_WS_TYPE_TEXT,
            .final = true
        };
        return httpd_ws_send_frame(req, &ip_pkt);
    }

    httpd_ws_frame_t ws_pkt = { .type = HTTPD_WS_TYPE_TEXT };
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get frame length: %d", ret);
        return ret;
    }

    uint8_t *buf = NULL;
    if (ws_pkt.len > 0) {
        buf = calloc(1, ws_pkt.len + 1);
        if (!buf) {
            ESP_LOGE(TAG, "Memory allocation for buf failed");
            return ESP_ERR_NO_MEM;
        }

        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to receive frame: %d", ret);
            goto cleanup;
        }

        ESP_LOGI(TAG, "Received message: %s", (char *)ws_pkt.payload);

        const char *payload = (char *)ws_pkt.payload;
        if (strcmp(payload, "toggle") == 0) {
            led_state = !led_state;
            ret = trigger_async_send(req->handle, req);
            goto cleanup;
        }

        if (strncmp(payload, "box:", 4) == 0) {
            ESP_LOGI(TAG,"sta_state = %d", sta_state);
            if(!sta_state) {
                ret = trigger_async_send(req->handle, req);
                goto cleanup;
            }
            int my_conn_state = strtod(payload + 4, NULL);
            set_conn_state(my_conn_state);
            ESP_LOGI(TAG,"conn_state = %d", conn_state);
            ret = trigger_async_send(req->handle, req);
            goto cleanup;
        }

        if (strcmp(payload, "update") == 0) {
            ret = trigger_async_send(req->handle, req);
            goto cleanup;
        }

        if (strncmp(payload, "timeoff:", 8) == 0) {
            int my_offset = strtod(payload + 8, NULL);
            if(abs(my_offset) < 6000) {
                int my_zone = my_offset / 60;
                char buf_zone[32];
                sprintf(buf_zone, "GMT%c%02d:%02d",my_zone>=0?'+':'-',abs(my_zone),
                    my_zone*60-my_offset);
                ESP_LOGI(TAG, "TZ: %s", buf_zone);
                setenv("TZ", buf_zone, 1);
                tzset();
            } else {
                char buf_zone[12] = "invalid TZ";
                ESP_LOGI(TAG, "TZ: %s", buf_zone);
                setenv("TZ", "GMT+00:00", 1);
                tzset();
            }
            goto cleanup;
        }

        if (strncmp(payload, "time:", 5) == 0) {
            if(sta_state) goto cleanup;
            long long my_time = strtoll(payload + 5, NULL, 10);
            struct timeval tv = {
                .tv_sec = (time_t)(my_time / 1000LL),
                .tv_usec = (suseconds_t)(my_time % 1000) * 1000
            };
            settimeofday(&tv, NULL);

            time_t now;
            struct tm timeinfo;
            char strftime_buf[64];
            time(&now);
            localtime_r(&now, &timeinfo);
            strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
            ESP_LOGI(TAG, "The current date/time is: %s", strftime_buf);
            goto cleanup;
        }
        if (strcmp(payload, "provision:yes") == 0) {
            ESP_LOGI(TAG, "Provisioning confirmed via WebSocket");
            esp_err_t ret_flag = write_uint8_to_nvs("do_prov", true);
            if (ret_flag != ESP_OK) {
                ESP_LOGE(TAG,"do_prov write_uint8_from_nvs error: %d",ret_flag);
            }
            const char *ack_msg = "modal:close";  // Tells client to close modal and return
            httpd_ws_frame_t ws_response = {
                .final = true,
                .fragmented = false,
                .len = strlen(ack_msg),
                .payload = (uint8_t *)ack_msg,
                .type = HTTPD_WS_TYPE_TEXT,
            };
            ret = httpd_ws_send_frame(req, &ws_response);
            free(buf);
            esp_restart();
        }
    }

cleanup:
    free(buf);
    return ret;
}

httpd_handle_t start_spiffs_webserver(httpd_handle_t *server) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.uri_match_fn = httpd_uri_match_wildcard;

#if defined(CONFIG_HTTPD_WS_SUPPORT) && defined(CONFIG_IDF_TARGET_ESP32)
    config.enable_websocket = true;
    ESP_LOGI(TAG, "WebSocket support enabled in config");
#else
    ESP_LOGW(TAG, "WebSocket support not enabled in SDK");
#endif

    esp_err_t ret = httpd_start(server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server: %s", esp_err_to_name(ret));
        return NULL;
    }

    httpd_uri_t ws_uri = {
        .uri = "/ws",
        .method = HTTP_GET,
        .handler = handle_ws_req,
        .user_ctx = NULL,
        .is_websocket = true};

    if (!ws_registered) {
        ESP_ERROR_CHECK(httpd_register_uri_handler(*server, &ws_uri));
        ws_registered = true;
    }

    httpd_uri_t file_uri = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = file_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(*server, &file_uri);

    ESP_LOGI(TAG, "Web server started");
    return *server;
}

esp_err_t init_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
      .base_path = BASE_PATH,
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount or format filesystem: %s", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition info");
        return ret;
    } 
    ESP_LOGI(TAG, "SPIFFS total: %d, used: %d", total, used);
    return ESP_OK;
}
