//#include "esp_https_server.h"
#include "spiffs_file_server.h"
#include "websockets.h"

static const char *TAG = "SPIFFS_SERVER";

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
    char uri[FILE_PATH_MAX];
    const char *uri_in = strcmp(req->uri, "/") == 0 ? "/index.html" : req->uri;
    strlcpy(uri, uri_in, sizeof(uri));

    char *q = strchr(uri, '?');
    if (q) *q = '\0';  // Strip query string (e.g. ?v=2)

    // Log client IP using sockaddr_storage
int sockfd = httpd_req_to_sockfd(req);
struct sockaddr_storage addr;
socklen_t addr_len = sizeof(addr);
if (getpeername(sockfd, (struct sockaddr *)&addr, &addr_len) == 0) {
    char ip_str[INET6_ADDRSTRLEN] = {0};

    if (addr.ss_family == AF_INET) {
        struct sockaddr_in *addr4 = (struct sockaddr_in *)&addr;
        inet_ntop(AF_INET, &addr4->sin_addr, ip_str, sizeof(ip_str));
    } else if (addr.ss_family == AF_INET6) {
        struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)&addr;
        inet_ntop(AF_INET6, &addr6->sin6_addr, ip_str, sizeof(ip_str));

        // Trim "::FFFF:" prefix if it's an IPv4-mapped IPv6 address
        const char *mapped_prefix = "::ffff:";
        if (strncasecmp(ip_str, mapped_prefix, strlen(mapped_prefix)) == 0) {
            memmove(ip_str, ip_str + strlen(mapped_prefix), strlen(ip_str) - strlen(mapped_prefix) + 1);
        }
    } else {
        strncpy(ip_str, "UnknownAF", sizeof(ip_str));
    }

    ESP_LOGI(TAG, "Client [%s] requested file: %s", ip_str, uri);
} else {
    ESP_LOGW(TAG, "Could not retrieve client IP for %s", uri);
}

    char filepath[FILE_PATH_MAX];
    strlcpy(filepath, BASE_PATH, sizeof(filepath));
    strlcat(filepath, uri, sizeof(filepath));

    FILE *file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGW(TAG, "File not found: %s", filepath);
        snprintf(filepath, sizeof(filepath), BASE_PATH "/404.html");
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
    } else {
        char chunk[1024];
        size_t chunksize;
        while ((chunksize = fread(chunk, 1, sizeof(chunk), file)) > 0) {
            httpd_resp_send_chunk(req, chunk, chunksize);
        }
        fclose(file);
        httpd_resp_send_chunk(req, NULL, 0);
    }
    return ESP_OK;
}

httpd_handle_t  start_spiffs_webserver(httpd_handle_t *server) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.uri_match_fn = httpd_uri_match_wildcard;

    esp_err_t ret = httpd_start(server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server: %s", esp_err_to_name(ret));
        return NULL;
    }
    ESP_LOGI(TAG, "Web server started");

    register_ws_handler(*server);
/*       
    static bool ws_registered = false;
    if (!ws_registered) {
        ws_registered = true;
    }
*/ 
    httpd_uri_t file_uri = {
        .uri      = "/*",
        .method   = HTTP_GET,
        .handler  = file_get_handler,
        .user_ctx = NULL
    };

    httpd_register_uri_handler(*server, &file_uri);

    return *server;
}

esp_err_t init_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount or format filesystem (%s)", esp_err_to_name(ret));
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "SPIFFS total: %d, used: %d", total, used);
    }
    return ret;
}
