#include "websockets.h"
#include "timeserver.h"
#include "esp_log.h"
#include <time.h>

static const char *TAG = "WEBSOCKETS";
static bool led_state = false;

esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket handshake initiated");
        return ESP_OK; // httpd_ws_handshake(req);
    }

    ESP_LOGI(TAG, "WebSocket frame incoming");
    httpd_ws_frame_t frame = {
        .final = true,
        .fragmented = false,
        .type = HTTPD_WS_TYPE_TEXT,
        .payload = NULL,
        .len = 0
    };

    esp_err_t ret = httpd_ws_recv_frame(req, &frame, 0);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get frame length: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "WebSocket validating frame");
    if (frame.len > 0) {
        frame.payload = malloc(frame.len + 1);
        if (!frame.payload) {
            ESP_LOGE(TAG, "Memory allocation failed");
            return ESP_ERR_NO_MEM;
        }

        ret = httpd_ws_recv_frame(req, &frame, frame.len);
        if (ret != ESP_OK) {
            free(frame.payload);
            return ret;
        }

        ESP_LOGI(TAG, "WebSocket set payload to zero");
        frame.payload[frame.len] = '\0';
        char *msg = (char *)frame.payload;
        ESP_LOGI(TAG, "Received WS message: %s", msg);

        if (strncmp(msg, "toggle", 6) == 0) {
            ESP_LOGI(TAG, "WebSocket toggle");
            led_state = !led_state;
            const char *toggle_msg = led_state ? "T1" : "T0";
            httpd_ws_frame_t resp = {
                .final = true,
                .type = HTTPD_WS_TYPE_TEXT,
                .payload = (uint8_t *)toggle_msg,
                .len = strlen(toggle_msg)
            };
            httpd_ws_send_frame(req, &resp);

            const char* time_str = timeserver_get_time_str();

            httpd_ws_frame_t time_resp = {
                .final = true,
                .type = HTTPD_WS_TYPE_TEXT,
                .payload = (uint8_t *)time_str,
                .len = strlen(time_str)
            };
            httpd_ws_send_frame(req, &time_resp);
            ESP_LOGI(TAG, "WebSocket toggle done");
        }

        if (strncmp(msg, "timeoff:", 8) == 0) {
            ESP_LOGI(TAG, "WebSocket timeoff");
            int offset = atoi(msg + 8);
            timeserver_set_offset(offset);
            ESP_LOGI(TAG, "WebSocket timeoff done");
        } else if (strncmp(msg, "time:", 5) == 0 ) {  //&& timeserver_should_accept_time()
            int64_t epoch_ms = atoll(msg + 5);
            timeserver_set_epoch_ms(epoch_ms);
            timeserver_sync();
            ESP_LOGI(TAG, "WebSocket time done");
        }
        free(frame.payload);
    }

    return ESP_OK;
}

esp_err_t register_ws_handler(httpd_handle_t server) {
    static const httpd_uri_t ws_uri = {
        .uri      = "/ws",
        .method   = HTTP_GET,
        .handler  = ws_handler,
        .user_ctx = NULL
    };
    return httpd_register_uri_handler(server, &ws_uri);
}