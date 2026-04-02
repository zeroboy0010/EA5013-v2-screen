#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <stdbool.h>
#include <time.h>

/**
 * @brief Initialize SNTP time synchronization for Cambodia timezone
 * 
 * Sets up NTP servers and Cambodia timezone (UTC+7)
 * Waits for initial time synchronization
 */
void time_sync_init(void);

/**
 * @brief Check if time has been synchronized
 * 
 * @return true if time is synced with NTP server
 * @return false if not yet synced
 */
bool time_is_synced(void);

/**
 * @brief Get current time in Cambodia timezone
 * 
 * @param timeinfo Pointer to tm structure to fill with current time
 */
void time_get_current(struct tm *timeinfo);

#endif // TIME_SYNC_H
