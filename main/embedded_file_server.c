//#include "esp_https_server.h"
#include "esp_http_server.h"
#include "esp_log.h"

static const char *TAG = "WEB_SERVER";

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

void start_embedded_webserver(httpd_handle_t *server) {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;

    esp_err_t ret = httpd_start(server, &config);
/*
    httpd_ssl_config_t conf = HTTPD_SSL_CONFIG_DEFAULT();
    conf.httpd.server_port = 443;
    conf.servercert = server_crt_start;
    conf.servercert_len = server_crt_end - server_crt_start;
    conf.prvtkey_pem = server_key_start;
    conf.prvtkey_len = server_key_end - server_key_start;

    esp_err_t ret = httpd_ssl_start(server, &conf);
*/

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


    httpd_register_uri_handler(*server, &uri_index);
    httpd_register_uri_handler(*server, &uri_css);
    httpd_register_uri_handler(*server, &uri_js);
    httpd_register_uri_handler(*server, &uri_favicon);
}
