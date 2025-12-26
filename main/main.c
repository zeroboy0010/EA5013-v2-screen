#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
#include <dirent.h>          // Added for DIR/opendir/readdir/closedir

#include "esp_spiffs.h"
#include "esp_heap_caps.h"
#include "clock_config.h"

// External declaration for simson image
// LV_IMG_DECLARE(simson);

static const char *TAG = "digital_clock";
static const char *LOG_SPIFFS = "spiffs";

// WiFi credentials - modify these in clock_config.h
#define WIFI_SSID DEFAULT_WIFI_SSID
#define WIFI_PASSWORD DEFAULT_WIFI_PASSWORD

// WiFi event group
static EventGroupHandle_t s_wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

// Clock UI elements
static lv_obj_t *time_label;
static lv_obj_t *date_label;
static lv_obj_t *wifi_status_label;
static lv_obj_t *clock_screen;

// Task handles
TaskHandle_t lvgl_task_handle = NULL;
TaskHandle_t clock_update_task_handle = NULL;

// WiFi connection status
static bool wifi_connected = false;
static int wifi_retry_num = 0;
#define MAX_RETRY_COUNT MAX_WIFI_RETRY_COUNT

// WiFi event handler
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (wifi_retry_num < MAX_RETRY_COUNT) {
            esp_wifi_connect();
            wifi_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        wifi_connected = false;
        ESP_LOGI(TAG, "Connect to the AP failed");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP:" IPSTR, IP2STR(&event->ip_info.ip));
        wifi_retry_num = 0;
        wifi_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

// Initialize WiFi
void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s", WIFI_SSID);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s", WIFI_SSID);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

// SNTP time sync notification callback
void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Notification of a time synchronization event");
}

// Initialize SNTP for time synchronization
void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, NTP_SERVER);
    esp_sntp_setservername(1, NTP_SERVER_BACKUP);
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
}

// Get formatted time string
void get_time_string(char *buffer, size_t size)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    strftime(buffer, size, "%H:%M:%S", &timeinfo);
}

// Get formatted date string
void get_date_string(char *buffer, size_t size)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    
    strftime(buffer, size, "%A, %B %d, %Y", &timeinfo);
}

// Initialize SPIFFS
void init_spiffs(void) {
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(LOG_SPIFFS, "Failed to mount or format SPIFFS");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(LOG_SPIFFS, "Failed to find SPIFFS partition");
        } else {
            ESP_LOGE(LOG_SPIFFS, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(NULL, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(LOG_SPIFFS, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(LOG_SPIFFS, "SPIFFS Partition Size: total: %zu, used: %zu", total, used);
    }
}
void list_files_in_spiffs(void) {
    const char* base_path = "/spiffs";
    DIR* dir = opendir(base_path);
    if (dir == NULL) {
        printf("Failed to open directory: %s\n", base_path);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        printf("File: %s\n", entry->d_name);
    }

    closedir(dir);
}
// Initialize LVGL hardware
void lvgl_hardware_init()
{
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 400000));
    lv_init();
    
    // Use consistent pixel clock speed
    uint32_t pclk = 10 * 1000 * 1000;  // 10 MHz
    lv_port_disp_init(pclk);
    ESP_LOGI("LCD PCLK", "Using PCLK: %lu MHz", pclk / 1000000);
    
    lv_port_indev_init();
    lv_port_tick_init();
    // lv_port_fs_init(); // Initialize file system support for GIF
}

// Touch reset function
void touch_io_reset(void)
{
#if(GT911==1)
    // GT911 touch reset sequence
    gpio_reset_pin(39);
    gpio_reset_pin(40);
    gpio_pullup_en(40);
    gpio_pullup_en(39);

    gpio_set_direction(39, GPIO_MODE_OUTPUT);
    gpio_set_direction(40, GPIO_MODE_OUTPUT);
    gpio_set_level(40, 1);
    gpio_set_level(39, 1);
    ESP_LOGI(TAG, "Touch reset high");
    vTaskDelay(pdMS_TO_TICKS(50));
    
    gpio_pulldown_en(39);
    gpio_pulldown_en(40);
    gpio_set_level(39, 0);
    gpio_set_level(40, 0);
    ESP_LOGI(TAG, "Touch reset low");

    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_set_level(40, 1);
    vTaskDelay(pdMS_TO_TICKS(20));
    gpio_pulldown_en(39);
    gpio_set_level(39, 0);
    
#elif(CST3240==1)
    // CST3240 touch reset sequence
    gpio_reset_pin(39);
    gpio_reset_pin(40);
    
    gpio_set_direction(GPIO_NUM_40, GPIO_MODE_OUTPUT);
    gpio_set_level(40, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    gpio_set_level(40, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
#endif
}

// Create landscape digital clock screen
void create_clock_screen(void)
{
    // Create a new screen
    clock_screen = lv_obj_create(NULL);
    
    // Set background color to dark blue/black
    lv_obj_set_style_bg_color(clock_screen, lv_color_hex(CLOCK_BG_COLOR), 0);
    
    // Create main container for clock elements
    lv_obj_t *main_container = lv_obj_create(clock_screen);
    lv_obj_set_size(main_container, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_style_bg_opa(main_container, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_opa(main_container, LV_OPA_TRANSP, 0);
    lv_obj_center(main_container);
    
    // Create time label (large font)
    time_label = lv_label_create(main_container);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, 0);
    lv_obj_set_style_text_color(time_label, lv_color_hex(TIME_TEXT_COLOR), 0);
    lv_label_set_text(time_label, "00:00:00");
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -40);
    
    // Create date label (medium font)
    date_label = lv_label_create(main_container);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_20, 0);
    lv_obj_set_style_text_color(date_label, lv_color_hex(DATE_TEXT_COLOR), 0);
    lv_label_set_text(date_label, "Loading...");
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 20);
    
    // // Create simson image below the clock
    // lv_obj_t *simson_img_ = lv_gif_create(main_container);
    // lv_gif_set_src(simson_img_, &simson);
    // lv_obj_set_size(simson_img_, 320, 320);  // Set the specified size
    // lv_obj_align(simson_img_, LV_ALIGN_CENTER, 0, 250);  // Position below the date label
    
    // Create WiFi status indicator
    wifi_status_label = lv_label_create(main_container);
    lv_obj_set_style_text_font(wifi_status_label, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(0x888888), 0);
    lv_label_set_text(wifi_status_label, "WiFi: Connecting...");
    lv_obj_align(wifi_status_label, LV_ALIGN_TOP_RIGHT, -10, 10);
    
    // Create decorative elements
    lv_obj_t *line1 = lv_line_create(main_container);
    static lv_point_t line_points1[] = {{50, 0}, {854 - 100, 0}};
    lv_line_set_points(line1, line_points1, 2);
    lv_obj_set_style_line_color(line1, lv_color_hex(ACCENT_LINE_COLOR), 0);
    lv_obj_set_style_line_width(line1, 2, 0);
    lv_obj_align(line1, LV_ALIGN_CENTER, 0, -10);



    // Load the screen
    lv_scr_load(clock_screen);
    
    ESP_LOGI(TAG, "Clock screen created and loaded");
}

// Update clock display
void update_clock_display(void)
{
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
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(WIFI_CONNECTED_COLOR), 0);
    } else {
        snprintf(wifi_str, sizeof(wifi_str), "WiFi: Disconnected");
        lv_obj_set_style_text_color(wifi_status_label, lv_color_hex(WIFI_DISCONNECTED_COLOR), 0);
    }
    lv_label_set_text(wifi_status_label, wifi_str);
}

// Clock update task
void clock_update_task(void *arg)
{
    while (1) {
        update_clock_display();
        vTaskDelay(pdMS_TO_TICKS(CLOCK_UPDATE_INTERVAL_MS)); // Update based on config
    }
}

// Main LVGL task
void lvgl_task(void *arg)
{
    // Initialize touch and hardware
    touch_io_reset();
    lvgl_hardware_init();
    ESP_LOGI(TAG, "LVGL initialized");

    // Create and display clock screen
    create_clock_screen();
    
    // Start clock update task
    xTaskCreate(clock_update_task, "clock_update", 1024*4, NULL, 3, &clock_update_task_handle);

    // Main LVGL loop
    while (1)
    {
        lv_task_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void app_main(void)
{
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Initialize SPIFFS
    init_spiffs();
    list_files_in_spiffs();
    
    // Initialize WiFi
    ESP_LOGI(TAG, "Initializing WiFi...");
    wifi_init_sta();
    
    // Wait a moment for WiFi to stabilize
    vTaskDelay(pdMS_TO_TICKS(2000));
    
    // Initialize SNTP for time synchronization
    initialize_sntp();
    
    // Set timezone (adjust in clock_config.h)
    setenv("TZ", TIMEZONE_CONFIG, 1);
    tzset();
    
    // Wait for time to be set
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2023 - 1900) && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }
    
    // Create LVGL task on Core 1
    xTaskCreate(lvgl_task, "lvgl_task", 1024*80, NULL, 4, &lvgl_task_handle);
    
    // Log memory info
    ESP_LOGI("MEM", "Internal RAM free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
    ESP_LOGI("MEM", "PSRAM free: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
    
    ESP_LOGI(TAG, "Digital Clock Application started");
}
