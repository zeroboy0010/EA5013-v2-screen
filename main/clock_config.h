#ifndef CLOCK_CONFIG_H
#define CLOCK_CONFIG_H

// Display Colors (24-bit hex)
#define CLOCK_BG_COLOR           0x000000  // Black background
#define TIME_TEXT_COLOR          0xFFFFFF  // White for time
#define DATE_TEXT_COLOR          0xCCCCCC  // Light gray for date
#define WIFI_CONNECTED_COLOR     0x2DFF22  // Green for connected
#define WIFI_DISCONNECTED_COLOR  0xFF2222  // Red for disconnected

// Update Intervals (milliseconds)
#define CLOCK_UPDATE_INTERVAL_MS 1000      // Update clock every 1 second
#define WIFI_CHECK_INTERVAL_MS   5000      // Check WiFi status every 5 seconds
#define GOLD_UPDATE_INTERVAL_MS  60000     // Update gold price every 60 seconds

// Time Configuration for Cambodia
#define TIMEZONE                 "ICT-7"   // Cambodia timezone (UTC+7)
#define NTP_SERVER_1             "pool.ntp.org"
#define NTP_SERVER_2             "time.google.com"
#define NTP_SERVER_3             "time.cloudflare.com"

// Display Settings
#define TIME_FONT_SIZE           48        // Large font for time
#define DATE_FONT_SIZE           20        // Medium font for date
#define STATUS_FONT_SIZE         14        // Small font for status
#define PRICE_FONT_SIZE          16        // Font for gold price

#endif // CLOCK_CONFIG_H
