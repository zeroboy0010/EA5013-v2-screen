#ifndef CLOCK_CONFIG_H
#define CLOCK_CONFIG_H

// WiFi Configuration
// Update these with your WiFi credentials
#define DEFAULT_WIFI_SSID "BVC Staff"
#define DEFAULT_WIFI_PASSWORD "BVC@staff"

// Time Configuration
// Set your timezone offset from UTC
// Examples:
// UTC+8 (Asia/Shanghai): "CST-8"
// UTC-5 (US Eastern): "EST5EDT,M3.2.0,M11.1.0"
// UTC+0 (London): "GMT0BST,M3.5.0/1,M10.5.0"
#define TIMEZONE_CONFIG "CST-7"

// NTP Server Configuration
#define NTP_SERVER "pool.ntp.org"
#define NTP_SERVER_BACKUP "time.nist.gov"

// Display Configuration
#define CLOCK_UPDATE_INTERVAL_MS 1000  // Update every second
#define MAX_WIFI_RETRY_COUNT 5

// Colors (in hex format)
#define CLOCK_BG_COLOR 0x001122        // Dark blue background
#define TIME_TEXT_COLOR 0xFFFFFF       // White time text
#define DATE_TEXT_COLOR 0xCCCCCC       // Light gray date text
#define WIFI_CONNECTED_COLOR 0x00AA00  // Green for connected
#define WIFI_DISCONNECTED_COLOR 0xAA0000 // Red for disconnected
#define ACCENT_LINE_COLOR 0x444444     // Dark gray accent lines

#endif // CLOCK_CONFIG_H