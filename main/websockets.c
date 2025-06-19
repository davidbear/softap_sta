#include "websockets.h"
#include "esp_log.h"
#include <time.h>

static const char *TAG = "WEBSOCKETS";
static bool led_state = false;

esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket handshake initiated");
        return ESP_OK;
    }

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

        frame.payload[frame.len] = '\0';
        char *msg = (char *)frame.payload;
        ESP_LOGI(TAG, "Received WS message: %s", msg);

        if (strncmp(msg, "toggle", 6) == 0) {
            led_state = !led_state;
            const char *toggle_msg = led_state ? "T1" : "T0";
            httpd_ws_frame_t resp = {
                .final = true,
                .type = HTTPD_WS_TYPE_TEXT,
                .payload = (uint8_t *)toggle_msg,
                .len = strlen(toggle_msg)
            };
            httpd_ws_send_frame(req, &resp);

            time_t now;
            struct tm timeinfo;
            char time_str[32];
            time(&now);
            localtime_r(&now, &timeinfo);
            strftime(time_str, sizeof(time_str), "time:%H:%M:%S", &timeinfo);

            httpd_ws_frame_t time_resp = {
                .final = true,
                .type = HTTPD_WS_TYPE_TEXT,
                .payload = (uint8_t *)time_str,
                .len = strlen(time_str)
            };
            httpd_ws_send_frame(req, &time_resp);
        }

        // Future: parse "time:", "timeoff:"
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