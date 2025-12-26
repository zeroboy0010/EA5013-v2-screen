#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialize screen/backlight control.
 * Safe to call multiple times.
 */
void screen_control_init(void);

/**
 * Turn the screen on/off.
 * Currently implemented by toggling LCD backlight GPIO.
 */
void screen_set_power(bool on);

/** Get the last requested screen power state. */
bool screen_get_power(void);

#ifdef __cplusplus
}
#endif
