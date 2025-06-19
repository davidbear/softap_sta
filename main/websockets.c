#include "websockets.h"

static const char *TAG = "WEBSOCKETS";

static bool led_state = false;  // Toggle state for demo

esp_err_t ws_handler(httpd_req_t *req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "WebSocket handshake initiated");
        return ESP_OK;  // Let the server handle the handshake
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
            ESP_LOGE(TAG, "Failed to allocate memory for frame");
            return ESP_ERR_NO_MEM;
        }

        ret = httpd_ws_recv_frame(req, &frame, frame.len);
        if (ret == ESP_OK) {
            frame.payload[frame.len] = '\0'; // Null-terminate
            ESP_LOGI(TAG, "Received WS message: %s", (char *)frame.payload);
            // TODO: parse "toggle", "time:", etc. here
        }

        free(frame.payload);
    }

    return ESP_OK;
}

esp_err_t ws_on_message(httpd_handle_t server, int sockfd, httpd_ws_frame_t *frame) {
    char *msg = (char *)malloc(frame->len + 1);
    if (!msg) return ESP_ERR_NO_MEM;

    memcpy(msg, frame->payload, frame->len);
    msg[frame->len] = 0;

    ESP_LOGI(TAG, "Received WS: %s", msg);

    if (strncmp(msg, "toggle", 6) == 0) {
        led_state = !led_state;
        const char *toggle_msg = led_state ? "T1" : "T0";
        httpd_ws_send_frame_async(server, sockfd, &(httpd_ws_frame_t){
            .final = true, .type = HTTPD_WS_TYPE_TEXT,
            .payload = (uint8_t *)toggle_msg, .len = strlen(toggle_msg)
        });

        // Send current local time
        time_t now;
        struct tm timeinfo;
        char time_str[16];
        time(&now);
        localtime_r(&now, &timeinfo);
        strftime(time_str, sizeof(time_str), "time:%H:%M:%S", &timeinfo);

        httpd_ws_send_frame_async(server, sockfd, &(httpd_ws_frame_t){
            .final = true, .type = HTTPD_WS_TYPE_TEXT,
            .payload = (uint8_t *)time_str, .len = strlen(time_str)
        });
    }

    // Future: handle `timeoff:` and `time:` if you want to use it for time sync logging

    free(msg);
    return ESP_OK;
}

esp_err_t register_ws_handler(httpd_handle_t server) {
    static bool registered = false;
    if (registered) return ESP_OK;

    static const httpd_uri_t ws_uri = {
        .uri      = "/ws",
        .method   = HTTP_GET,
        .handler  = ws_handler,
        .user_ctx = NULL,
        .is_websocket = true,
        .handle_ws_control_frames = false,
//        .recv_wait_timeout = 10,
        .supported_subprotocol = NULL
    };

    esp_err_t err = httpd_register_uri_handler(server, &ws_uri);
    if (err == ESP_OK) {
        registered = true;
        ESP_LOGI(TAG, "WebSocket URI handler registered");

    } else {
        ESP_LOGE(TAG, "Failed to register WS handler: %s", esp_err_to_name(err));
    }
    return err;
}
