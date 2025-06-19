//#include "esp_https_server.h"
#include "spiffs_file_server.h"

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
    const char *uri = strcmp(req->uri, "/") == 0 ? "/index.html" : req->uri;

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

static esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WS handshake initiated");
        return ESP_OK; // WebSocket handshake handled internally by the server
    }
    return ESP_FAIL;
}

void start_spiffs_webserver(httpd_handle_t *server) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 8192;
    config.uri_match_fn = httpd_uri_match_wildcard;

    esp_err_t ret = httpd_start(server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "Web server started");

    httpd_uri_t file_uri = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = file_get_handler,
        .user_ctx  = NULL
    };
    httpd_register_uri_handler(*server, &file_uri);

    httpd_uri_t ws_uri = {
        .uri       = "/ws",
        .method    = HTTP_GET,
        .handler   = ws_handler,
        .user_ctx  = NULL,
//        .is_websocket = true
    };
     httpd_register_uri_handler(*server, &static_uri);

    if (!ws_registered) {
        httpd_register_uri_handler(*server, &ws_uri);
        ws_registered = true;
    }

    ESP_LOGI(TAG, "Web server started");
    return *server;
}

void init_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
      .base_path = BASE_PATH,
      .partition_label = NULL,
      .max_files = 5,
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to mount or format filesystem: %s", esp_err_to_name(ret));
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition info");
    } else {
        ESP_LOGI(TAG, "SPIFFS total: %d, used: %d", total, used);
    }
}


extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
extern const uint8_t style_css_start[]  asm("_binary_style_css_start");
extern const uint8_t style_css_end[]    asm("_binary_style_css_end");
extern const uint8_t index_js_start[]   asm("_binary_index_js_start");
extern const uint8_t index_js_end[]     asm("_binary_index_js_end");
extern const uint8_t favicon_ico_start[] asm("_binary_favicon_ico_start");
extern const uint8_t favicon_ico_end[]   asm("_binary_favicon_ico_end");

//extern const uint8_t server_crt_start[] asm("_binary_server_crt_start"); //is cert necessary here?
//extern const uint8_t server_crt_end[]   asm("_binary_server_crt_end");
//extern const uint8_t server_key_start[] asm("_binary_server_key_start");
//extern const uint8_t server_key_end[]   asm("_binary_server_key_end");


static esp_err_t send_embedded(httpd_req_t *req, const uint8_t *start, const uint8_t *end, const char *type) {
    httpd_resp_set_type(req, type);
    return httpd_resp_send(req, (const char *) start, end - start);
}

static esp_err_t index_handler(httpd_req_t *req) {
/*
    char *buff = malloc(index_html_end - index_html_start + 1);
    strncpy(buff,(char *)index_html_start,index_html_end-index_html_start);
    buff[index_html_end-index_html_start]='\0';
*/
    ESP_LOGI(TAG, "Serving index.html");
    return send_embedded(req, index_html_start, index_html_end, "text/html; charset=utf-8");
}

static esp_err_t css_handler(httpd_req_t *req) {
/*
    char *buff = malloc(style_css_end - style_css_start + 1);
    strncpy(buff,(char *)style_css_start,style_css_end-style_css_start);
    buff[style_css_end-style_css_start]='\0';
*/
    ESP_LOGI(TAG, "Serving style.css");
    return send_embedded(req, style_css_start, style_css_end, "text/css; charset=utf-8");
}

static esp_err_t js_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Serving index.js");
    return send_embedded(req, index_js_start, index_js_end, "application/javascript; charset=utf-8");
}

static esp_err_t favicon_handler(httpd_req_t * req)
{
    ESP_LOGI(TAG, "Serving favicon.ico");
    httpd_resp_set_type(req, "image/x-icon");
    return httpd_resp_send(req, (const char *)favicon_ico_start,
                            favicon_ico_end - favicon_ico_start);
}

/*
static esp_err_t not_found_handler(httpd_req_t *req) {
    ESP_LOGW(TAG, "404 Not Found: %s", req->uri);
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "URI not found");
    return ESP_OK;
}
*/

static esp_err_t catch_all_handler(httpd_req_t *req) {
    ESP_LOGW(TAG, "Catch-all: URI=%s, Method=%d", req->uri, req->method);

    // Log headers
    char value[128];
    if (httpd_req_get_hdr_value_str(req, "User-Agent", value, sizeof(value)) == ESP_OK)
        ESP_LOGW(TAG, "User-Agent: %s", value);
    if (httpd_req_get_hdr_value_str(req, "Content-Type", value, sizeof(value)) == ESP_OK)
        ESP_LOGW(TAG, "Content-Type: %s", value);

    // If POST, log body
    if (req->method == HTTP_POST && req->content_len > 0) {
        char *buf = malloc(req->content_len + 1);
        if (!buf) return ESP_ERR_NO_MEM;

        int received = httpd_req_recv(req, buf, req->content_len);
        if (received > 0) {
            buf[received] = '\0';
            ESP_LOGW(TAG, "Body: %s", buf);
        }
        free(buf);
    }

    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Resource not found");
    return ESP_OK;
}

void start_embedded_webserver(httpd_handle_t *server) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    config.stack_size = 8192;               // Increase stack size
    config.recv_wait_timeout = 10;          // Increase receive timeout
    config.send_wait_timeout = 10;          // Increase send timeout
    config.max_uri_handlers = 10;           // Increase if many endpoints
    config.uri_match_fn = httpd_uri_match_wildcard; // Support wildcards for URI fallback

    esp_err_t ret = httpd_start(server, &config);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start server! Error: %s", esp_err_to_name(ret));
        return;
    }

    ESP_LOGI(TAG, "Web server started");

    httpd_uri_t uri_index = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = index_handler,
        .user_ctx = NULL
    };

    httpd_uri_t uri_css = {
        .uri = "/style.css",
        .method = HTTP_GET,
        .handler = css_handler,
        .user_ctx = NULL
    };

    httpd_uri_t uri_js = {
        .uri = "/index.js",
        .method = HTTP_GET,
        .handler = js_handler,
        .user_ctx = NULL};

    httpd_uri_t uri_favicon = {
        .uri = "/favicon.ico",
        .method = HTTP_GET,
        .handler = favicon_handler,
        .user_ctx = NULL};

    httpd_uri_t uri_404 = {
        .uri       = "/*",
        .method    = HTTP_GET,
        .handler   = catch_all_handler,
        .user_ctx  = NULL
    };

    httpd_uri_t catch_all_post = {
        .uri       = "/*",
        .method    = HTTP_POST,  // can't be wildcard, so you'll need to copy this per method
        .handler   = catch_all_handler,
        .user_ctx  = NULL
    };

    httpd_register_uri_handler(*server, &uri_index);
    httpd_register_uri_handler(*server, &uri_css);
    httpd_register_uri_handler(*server, &uri_js);
    httpd_register_uri_handler(*server, &uri_favicon);
    httpd_register_uri_handler(*server, &uri_404);
//    httpd_register_uri_handler(*server, &catch_all);
    httpd_register_uri_handler(*server, &catch_all_post);

}
