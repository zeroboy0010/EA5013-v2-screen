#ifndef GOLD_PRICE_H
#define GOLD_PRICE_H

#include <stdbool.h>

typedef struct {
    float price;
    float change;
    float change_percent;
    bool valid;
} gold_price_data_t;

/**
 * @brief Initialize gold price fetcher
 * 
 * Sets up HTTP client and prepares for API calls
 */
void gold_price_init(void);

/**
 * @brief Fetch current gold price from API
 * 
 * @param data Pointer to structure to store the fetched data
 * @return true if fetch was successful, false otherwise
 */
bool gold_price_fetch(gold_price_data_t *data);

/**
 * @brief Start gold price update task
 * 
 * Creates a task that periodically fetches and updates gold price
 */
void gold_price_start_task(void);

#endif // GOLD_PRICE_H
