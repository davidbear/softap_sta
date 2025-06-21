// timeserver.c
#include "timeserver.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "TIMESERVER";

static time_t base_epoch = 0;
static int32_t time_offset = 0;
static int64_t last_sync_us = 0;
static bool sta_connected = false;

void timeserver_init(void) {
    base_epoch = 0;
    time_offset = 0;
    last_sync_us = esp_timer_get_time();
}

void timeserver_set_epoch_ms(int64_t epoch_ms) {
    base_epoch = (time_t)(epoch_ms / 1000);
    int64_t ms_remainder = epoch_ms % 1000;
    last_sync_us = esp_timer_get_time() - (ms_remainder * 1000);
    ESP_LOGI(TAG, "Epoch updated (ms): %lld", (long long)epoch_ms);
}

void timeserver_set_offset(int32_t offset_seconds) {
    time_offset = offset_seconds;
    ESP_LOGI(TAG, "Offset updated: %ld", offset_seconds);
}

void timeserver_sync(void) {
    last_sync_us = esp_timer_get_time();
}

const char* timeserver_get_time_str(void) {
    static char time_str[32];
    int64_t elapsed = (esp_timer_get_time() - last_sync_us) / 1000000;
    time_t now = base_epoch + elapsed + time_offset;
    struct tm tm_info;
    localtime_r(&now, &tm_info);
    strftime(time_str, sizeof(time_str), "time:%H:%M:%S", &tm_info);
    return time_str;
}

bool timeserver_should_accept_time(void) {
    return !sta_connected;
}

void timeserver_sta_set_connected(bool connected) {
    sta_connected = connected;
    ESP_LOGI(TAG, "STA connection %s", connected ? "established" : "lost");
}

time_t timeserver_get_epoch(void) {
    return base_epoch;
}

int32_t timeserver_get_offset(void) {
    return time_offset;
}
