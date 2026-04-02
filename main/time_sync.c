#include "time_sync.h"
#include "clock_config.h"
#include "esp_log.h"
#include "esp_sntp.h"
#include <time.h>
#include <sys/time.h>

static const char *TAG = "TimeSync";

void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized with NTP server");
}

void time_sync_init(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    
    // Set timezone for Cambodia (UTC+7)
    setenv("TZ", TIMEZONE, 1);
    tzset();
    
    // Initialize SNTP
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER_1);
    esp_sntp_setservername(1, NTP_SERVER_2);
    esp_sntp_setservername(2, NTP_SERVER_3);
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
    
    ESP_LOGI(TAG, "Waiting for time synchronization...");
    
    // Wait for time to be synchronized (up to 10 seconds)
    int retry = 0;
    const int retry_count = 20;
    while (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
    
    if (retry >= retry_count) {
        ESP_LOGW(TAG, "Failed to synchronize time with NTP server");
    } else {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        char strftime_buf[64];
        strftime(strftime_buf, sizeof(strftime_buf), "%c", &timeinfo);
        ESP_LOGI(TAG, "Current time: %s", strftime_buf);
    }
}

bool time_is_synced(void)
{
    return (esp_sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED);
}

void time_get_current(struct tm *timeinfo)
{
    time_t now;
    time(&now);
    localtime_r(&now, timeinfo);
}
