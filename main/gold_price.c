#include "gold_price.h"
#include "clock_screen.h"
#include "clock_config.h"
#include "wifi_config.h"
#include "esp_log.h"
#include "esp_http_client.h"
#include "esp_tls.h"
#include "esp_crt_bundle.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "GoldPrice";

// Using Metals-API.com (Free tier: 50 requests/month)
// You can also use other APIs like GoldAPI.io, MetalpriceAPI.com
#define GOLD_API_URL "https://www.goldapi.io/api/XAU/USD"
#define API_KEY "goldapi-YOUR_API_KEY_HERE"  // Get free API key from goldapi.io

// Alternative free API (no key needed, but less reliable)
#define ALT_API_URL "https://api.metals.live/v1/spot/gold"

static char response_buffer[2048];
static int response_len = 0;

// HTTP event handler
static esp_err_t http_event_handler(esp_http_client_event_t *evt)
{
    switch(evt->event_id) {
        case HTTP_EVENT_ON_DATA:
            if (response_len + evt->data_len < sizeof(response_buffer)) {
                memcpy(response_buffer + response_len, evt->data, evt->data_len);
                response_len += evt->data_len;
                response_buffer[response_len] = 0;
            }
            break;
        default:
            break;
    }
    return ESP_OK;
}

void gold_price_init(void)
{
    ESP_LOGI(TAG, "Gold price fetcher initialized");
}

bool gold_price_fetch(gold_price_data_t *data)
{
    if (!wifi_is_connected()) {
        ESP_LOGW(TAG, "WiFi not connected, skipping gold price fetch");
        return false;
    }

    // Reset response buffer
    response_len = 0;
    memset(response_buffer, 0, sizeof(response_buffer));

    // Configure HTTP client
    esp_http_client_config_t config = {
        .url = ALT_API_URL,  // Using alternative free API
        .event_handler = http_event_handler,
        .timeout_ms = 10000,
        .crt_bundle_attach = esp_crt_bundle_attach,  // Enable certificate verification
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    
    // Add API key header if using GoldAPI.io
    // esp_http_client_set_header(client, "x-access-token", API_KEY);
    
    esp_err_t err = esp_http_client_perform(client);
    
    if (err == ESP_OK) {
        int status_code = esp_http_client_get_status_code(client);
        ESP_LOGI(TAG, "HTTP Status = %d, content_length = %d", 
                 status_code, response_len);
        
        if (status_code == 200) {
            // Parse JSON response
            cJSON *json = cJSON_Parse(response_buffer);
            if (json != NULL) {
                // Parse response based on API structure
                // For metals.live API:
                cJSON *price_item = cJSON_GetObjectItem(json, "price");
                cJSON *change_item = cJSON_GetObjectItem(json, "ch");
                cJSON *change_percent_item = cJSON_GetObjectItem(json, "chp");
                
                if (price_item && cJSON_IsNumber(price_item)) {
                    data->price = price_item->valuedouble;
                    data->change = change_item ? change_item->valuedouble : 0.0;
                    data->change_percent = change_percent_item ? change_percent_item->valuedouble : 0.0;
                    data->valid = true;
                    
                    ESP_LOGI(TAG, "Gold Price: $%.2f, Change: %.2f (%.2f%%)", 
                             data->price, data->change, data->change_percent);
                    
                    cJSON_Delete(json);
                    esp_http_client_cleanup(client);
                    return true;
                }
                
                // Try alternative JSON structure (for goldapi.io)
                cJSON *price_obj = cJSON_GetObjectItem(json, "price");
                if (price_obj && cJSON_IsNumber(price_obj)) {
                    data->price = price_obj->valuedouble;
                    
                    cJSON *prev_close = cJSON_GetObjectItem(json, "prev_close_price");
                    if (prev_close && cJSON_IsNumber(prev_close)) {
                        data->change = data->price - prev_close->valuedouble;
                        data->change_percent = (data->change / prev_close->valuedouble) * 100.0;
                    } else {
                        data->change = 0.0;
                        data->change_percent = 0.0;
                    }
                    
                    data->valid = true;
                    
                    ESP_LOGI(TAG, "Gold Price: $%.2f, Change: %.2f (%.2f%%)", 
                             data->price, data->change, data->change_percent);
                    
                    cJSON_Delete(json);
                    esp_http_client_cleanup(client);
                    return true;
                }
                
                cJSON_Delete(json);
            } else {
                ESP_LOGE(TAG, "Failed to parse JSON: %s", response_buffer);
            }
        } else {
            ESP_LOGW(TAG, "HTTP request failed with status %d", status_code);
        }
    } else {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(err));
    }
    
    esp_http_client_cleanup(client);
    return false;
}

// Task to periodically fetch gold price
static void gold_price_task(void *arg)
{
    gold_price_data_t price_data;
    
    // Wait for WiFi to connect
    vTaskDelay(pdMS_TO_TICKS(5000));
    
    while (1) {
        if (gold_price_fetch(&price_data)) {
            // Update the display with new price
            clock_screen_update_gold_price(price_data.price, 
                                          price_data.change, 
                                          price_data.change_percent);
        } else {
            ESP_LOGW(TAG, "Failed to fetch gold price");
        }
        
        // Wait for next update (as defined in config)
        vTaskDelay(pdMS_TO_TICKS(GOLD_UPDATE_INTERVAL_MS));
    }
}

void gold_price_start_task(void)
{
    xTaskCreate(gold_price_task, "gold_price", 8192, NULL, 4, NULL);
    ESP_LOGI(TAG, "Gold price update task started");
}
