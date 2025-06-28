#include "timekeeper.h"

const char *TAG = "SNTP_SYNC";

#define TIME_SYNC_INTERVAL_S (CONFIG_TIME_SYNC_INTERVAL_H * 60 * 60) // 5 hours
#define MAX_DRIFT_SECONDS    2             // Maximum drift tolerated before adjustment

// Optional: Set preferred SNTP server
#define SNTP_SERVER "pool.ntp.org"

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized. UNIX time: %lld", tv->tv_sec);
}

void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_set_sync_mode(SNTP_SYNC_MODE_IMMED);  // Smooth time sync (small adjustments)
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_setservername(0, SNTP_SERVER);         // You can set multiple servers if desired

    esp_sntp_init();
}

void start_periodic_sntp(void)
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;

    initialize_sntp();

    // Set sync interval to 5 hours
    sntp_set_sync_interval(TIME_SYNC_INTERVAL_S * 1000); // milliseconds
}

// Optional utility to check if drift is within threshold
bool is_drift_too_large(struct timeval *old_time, struct timeval *new_time)
{
    time_t diff = llabs(new_time->tv_sec - old_time->tv_sec);
    return diff > MAX_DRIFT_SECONDS;
}
