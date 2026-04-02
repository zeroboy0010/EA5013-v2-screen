#ifndef CLOCK_SCREEN_H
#define CLOCK_SCREEN_H

#include "lvgl.h"
#include <stdbool.h>

/**
 * @brief Create the clock screen
 * 
 * Creates all UI elements for the clock display including:
 * - Time display (HH:MM:SS)
 * - Date display
 * - WiFi status indicator
 * - Gold price display
 */
void clock_screen_create(void);

/**
 * @brief Update clock screen display
 * 
 * Updates time, date, and WiFi status on the screen
 * Should be called periodically (e.g., every second)
 */
void clock_screen_update(void);

/**
 * @brief Load and display the clock screen
 * 
 * Creates the screen if it doesn't exist and loads it
 */
void clock_screen_load(void);

/**
 * @brief Delete the clock screen and free resources
 */
void clock_screen_delete(void);

/**
 * @brief Update gold price display
 * 
 * @param price Current gold price
 * @param change Price change
 * @param change_percent Percentage change
 */
void clock_screen_update_gold_price(float price, float change, float change_percent);

#endif // CLOCK_SCREEN_H