#include "clock_screen.h"
#include "clock_config.h"
#include "lvgl.h"
#include <time.h>
#include <sys/time.h>
#include <stdio.h>

// Screen objects
static lv_obj_t *clock_screen = NULL;
static lv_obj_t *time_label = NULL;
static lv_obj_t *date_label = NULL;
static lv_obj_t *wifi_label = NULL;
static lv_obj_t *gold_price_label = NULL;

// External variables
extern bool wifi_connected;

// Helper function to get current time string
static void get_time_string(char *buf, size_t buf_size)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buf, buf_size, "%H:%M:%S", &timeinfo);
}

// Helper function to get current date string
static void get_date_string(char *buf, size_t buf_size)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buf, buf_size, "%a %d %b %Y", &timeinfo);
}

void clock_screen_create(void)
{
    // Create main screen
    clock_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(clock_screen, lv_color_hex(CLOCK_BG_COLOR), 0);
    
    // Create time label (large, centered)
    time_label = lv_label_create(clock_screen);
    lv_obj_set_style_text_color(time_label, lv_color_hex(TIME_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_label_set_text(time_label, "00:00:00");
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -50);
    
    // Create date label (below time)
    date_label = lv_label_create(clock_screen);
    lv_obj_set_style_text_color(date_label, lv_color_hex(DATE_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
    lv_label_set_text(date_label, "--- -- --- ----");
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 10);
    
    // Create WiFi status label (top right)
    wifi_label = lv_label_create(clock_screen);
    lv_obj_set_style_text_font(wifi_label, &lv_font_montserrat_14, 0);
    lv_label_set_text(wifi_label, "WiFi: Disconnected");
    lv_obj_align(wifi_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    
    // Create gold price label (bottom)
    gold_price_label = lv_label_create(clock_screen);
    lv_obj_set_style_text_color(gold_price_label, lv_color_hex(TIME_TEXT_COLOR), 0);
    lv_obj_set_style_text_font(gold_price_label, &lv_font_montserrat_16, 0);
    lv_obj_set_style_text_align(gold_price_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(gold_price_label, 
        "XAU/USD - Gold Spot\n\n"
        "4,599.27 +1.85(+0.04%)");
    lv_obj_set_width(gold_price_label, LV_HOR_RES - 40);
    lv_obj_align(gold_price_label, LV_ALIGN_BOTTOM_MID, 0, -20);
}

void clock_screen_update(void)
{
    if (clock_screen == NULL) return;
    
    char time_str[32];
    char date_str[64];
    char wifi_str[32];
    
    // Update time
    get_time_string(time_str, sizeof(time_str));
    lv_label_set_text(time_label, time_str);
    
    // Update date
    get_date_string(date_str, sizeof(date_str));
    lv_label_set_text(date_label, date_str);
    
    // Update WiFi status
    if (wifi_connected) {
        snprintf(wifi_str, sizeof(wifi_str), "WiFi: Connected");
        lv_obj_set_style_text_color(wifi_label, lv_color_hex(WIFI_CONNECTED_COLOR), 0);
    } else {
        snprintf(wifi_str, sizeof(wifi_str), "WiFi: Disconnected");
        lv_obj_set_style_text_color(wifi_label, lv_color_hex(WIFI_DISCONNECTED_COLOR), 0);
    }
    lv_label_set_text(wifi_label, wifi_str);
}

void clock_screen_load(void)
{
    if (clock_screen == NULL) {
        clock_screen_create();
    }
    lv_scr_load(clock_screen);
    clock_screen_update();
}

void clock_screen_delete(void)
{
    if (clock_screen != NULL) {
        lv_obj_del(clock_screen);
        clock_screen = NULL;
        time_label = NULL;
        date_label = NULL;
        wifi_label = NULL;
        gold_price_label = NULL;
    }
}

void clock_screen_update_gold_price(float price, float change, float change_percent)
{
    if (gold_price_label == NULL) return;
    
    char price_str[128];
    char title_line[64] = "XAU/USD - Gold Spot\n\n";
    char price_line[64];
    
    // Format price with comma separator
    int price_int = (int)price;
    int price_dec = (int)((price - price_int) * 100);
    
    snprintf(price_line, sizeof(price_line),
        "%d,%03d.%02d %+.2f(%+.2f%%)",
        price_int / 1000, price_int % 1000, price_dec,
        change, change_percent);
    
    snprintf(price_str, sizeof(price_str), "%s%s", title_line, price_line);
    
    lv_label_set_text(gold_price_label, price_str);
    
    // Set color based on change (entire label color)
    if (change >= 0) {
        lv_obj_set_style_text_color(gold_price_label, lv_color_hex(WIFI_CONNECTED_COLOR), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(gold_price_label, lv_color_hex(WIFI_DISCONNECTED_COLOR), LV_PART_MAIN);
    }
}