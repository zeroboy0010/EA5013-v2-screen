#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <stdbool.h>

// WiFi Configuration
// Change these to match your WiFi network
#define WIFI_SSID               "BVC Staff"
#define WIFI_PASSWORD           "BVC@staff"

// WiFi Connection Settings
#define WIFI_MAXIMUM_RETRY      5          // Maximum retry attempts
#define WIFI_CONNECTED_BIT      BIT0
#define WIFI_FAIL_BIT           BIT1

// WiFi Status
extern bool wifi_connected;

// Function declarations
void wifi_init(void);
bool wifi_is_connected(void);
void wifi_reconnect(void);

#endif // WIFI_CONFIG_H
