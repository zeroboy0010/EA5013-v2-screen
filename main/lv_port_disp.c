/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1
// add this 
#define CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM 1
#define CONFIG_EXAMPLE_DOUBLE_FB 0
/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include "lvgl.h"

/*********************
 *      DEFINES
 *********************/
static const char *TAG = "lv_port_disp";

esp_lcd_panel_handle_t panel_handle = NULL;
/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  STATIC VARIABLES
 **********************/
// we use two semaphores to sync the VSYNC event and the LVGL task, to avoid potential tearing effect
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
SemaphoreHandle_t sem_vsync_end;
SemaphoreHandle_t sem_gui_ready;
#endif

/**********************
 *      MACROS
 **********************/
static bool rgb_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data);
static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map);

/**
 * @brief Clear LCD screen with specified color (line by line)
 *
 * @param panel LCD panel handle
 * @param color Color to fill (16-bit RGB565 format)
 * @return esp_err_t ESP_OK on success, ESP_FAIL on memory allocation failure
 */

esp_err_t lcd_reconfigure_pclk(uint32_t new_pclk_hz) {
    if (panel_handle != NULL) {
        // Delete existing panel instance
        esp_lcd_panel_del(panel_handle);
        panel_handle = NULL;
    }

    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .psram_trans_align = 64,
#if CONFIG_EXAMPLE_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 10 * LCD_WIDTH,
#endif
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = GPIO_NUM_NC,
        .pclk_gpio_num = GPIO_LCD_PCLK,
        .vsync_gpio_num = GPIO_LCD_VSYNC,
        .hsync_gpio_num = GPIO_LCD_HSYNC,
        .de_gpio_num = GPIO_LCD_DE,
        .data_gpio_nums = {
            GPIO_LCD_B0,
            GPIO_LCD_B1,
            GPIO_LCD_B2,
            GPIO_LCD_B3,
            GPIO_LCD_B4,
            GPIO_LCD_G0,
            GPIO_LCD_G1,
            GPIO_LCD_G2,
            GPIO_LCD_G3,
            GPIO_LCD_G4,
            GPIO_LCD_G5,
            GPIO_LCD_R0,
            GPIO_LCD_R1,
            GPIO_LCD_R2,
            GPIO_LCD_R3,
            GPIO_LCD_R4,
        },
        .timings = {
            .h_res = LCD_WIDTH, .v_res = LCD_HEIGHT,
    // The following parameters should refer to LCD spec
#if ((LCD_5r0_800x480==1)||(LCD_4r3_800x480==1))
            .pclk_hz = 15 * 1000 * 1000,
            .hsync_back_porch = 42,
            .hsync_front_porch = 20,
            .hsync_pulse_width = 1,
            .vsync_back_porch = 12,
            .vsync_front_porch = 4,
            .vsync_pulse_width = 10,
#elif(LCD_7r0_800x480==1) 
            .pclk_hz = 15 * 1000 * 1000,
            .hsync_back_porch = 32,
            .hsync_front_porch = 10,
            .hsync_pulse_width = 16,
            .vsync_back_porch = 12,
            .vsync_front_porch = 15,
            .vsync_pulse_width = 3,            
#elif(LCD_4r0_480x480==1) 
            .pclk_hz = 10 * 1000 * 1000,
            .hsync_back_porch = 32,
            .hsync_front_porch = 10,
            .hsync_pulse_width = 16,
            .vsync_back_porch = 12,
            .vsync_front_porch = 15,
            .vsync_pulse_width = 3,
            
#elif(LCD_5r0_480x854==1) //EP5008S
            .pclk_hz = new_pclk_hz,
            .hsync_back_porch = 30,
            .hsync_front_porch = 12,
            .hsync_pulse_width = 6,
            .vsync_back_porch = 30,
            .vsync_front_porch = 12,
            .vsync_pulse_width = 1,

#elif(LCD_4r3_480x272==1) 
            .pclk_hz = 10 * 1000 * 1000,
            .hsync_back_porch = 43,
            .hsync_front_porch = 75,
            .hsync_pulse_width = 4,
            .vsync_back_porch = 12,
            .vsync_front_porch = 8,
            .vsync_pulse_width = 4,
#endif

#if ((LCD_4r0_480x480==1)||(LCD_5r0_480x854==1))
            .flags.pclk_active_neg = false,
#else
            .flags.pclk_active_neg = true,
#endif
        },
        .flags.fb_in_psram = true,
#if CONFIG_EXAMPLE_DOUBLE_FB
        .flags.double_fb = true,
#endif
    };
    
    // Initialize new panel with updated config
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));
    
    // Register event callbacks 
    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = rgb_on_vsync_event,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, NULL));

    // Reset and init panel
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    return ESP_OK;
}


/**
 * @brief Change LCD pixel clock frequency while running
 * 
 * @param new_pclk_hz New pixel clock frequency in Hz
 * @return esp_err_t ESP_OK on success
 */
esp_err_t lcd_change_pclk(uint32_t new_pclk_hz)
{
    ESP_LOGI(TAG, "Changing PCLK to %lu Hz", new_pclk_hz);
    
    // Suspend LVGL to avoid drawing during reconfiguration
    lv_disp_t *disp = lv_disp_get_default();
    if (!disp) {
        ESP_LOGE(TAG, "No display registered");
        return ESP_FAIL;
    }
    
    // Get current driver
    lv_disp_drv_t *drv = disp->driver;
    
    // Stop LVGL timer-based operations temporarily
    bool was_enabled = lv_disp_is_invalidation_enabled(disp);
    if (was_enabled) {
        lv_disp_enable_invalidation(disp, false);
    }
    
    // Reconfigure LCD with new pixel clock
    esp_err_t ret = lcd_reconfigure_pclk(new_pclk_hz);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to reconfigure LCD pixel clock");
        // Re-enable LVGL operations
        if (was_enabled) {
            lv_disp_enable_invalidation(disp, true);
        }
        return ret;
    }
    
    // Update display driver with new panel handle
    drv->user_data = panel_handle;
    
    // Re-enable LVGL operations if they were enabled before
    if (was_enabled) {
        lv_disp_enable_invalidation(disp, true);
    }
    
    // Force a full refresh
    lv_obj_invalidate(lv_scr_act());
    
    ESP_LOGI(TAG, "PCLK changed successfully to %lu Hz", new_pclk_hz);
    return ESP_OK;
}

esp_err_t lcd_clear(uint16_t color)
{
    uint16_t *buffer = heap_caps_malloc(LCD_WIDTH * sizeof(uint16_t), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (NULL == buffer) {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return ESP_FAIL;
    } else {
        for (uint16_t i = 0; i < LCD_WIDTH; i++) {
            buffer[i] = color;
        }
        for (int y = 0; y < LCD_HEIGHT; y++) {
            esp_lcd_panel_draw_bitmap(panel_handle, 0, y, LCD_WIDTH, y+1, buffer);
        }
        heap_caps_free(buffer);
    }
    return ESP_OK;
}

/**
 * @brief Clear LCD screen faster by drawing multiple lines at once
 *
 * @param panel LCD panel handle
 * @param color Color to fill (16-bit RGB565 format)
 * @return esp_err_t ESP_OK on success, ESP_FAIL on memory allocation failure
 */
esp_err_t lcd_clear_fast(uint16_t color)
{
    uint16_t fact = 68;
    uint16_t *buffer = heap_caps_malloc(LCD_WIDTH * fact * sizeof(uint16_t), MALLOC_CAP_SPIRAM);
    if (NULL == buffer) {
        ESP_LOGE(TAG, "Memory for bitmap is not enough");
        return ESP_FAIL;
    } else {
        for (uint16_t i = 0; i < LCD_WIDTH * fact; i++) {
            buffer[i] = color;
        }
        for (int y = 0; y < LCD_HEIGHT; y += fact) {
            esp_lcd_panel_draw_bitmap(panel_handle, 0, y, LCD_WIDTH, y + fact, buffer);
        }
        heap_caps_free(buffer);
    }
    return ESP_OK;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_port_disp_init(uint32_t pclk_hz_value)
{
    static lv_disp_drv_t disp_drv; // contains callback functions

#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    /* Create semaphores */
    sem_vsync_end = xSemaphoreCreateBinary();
    assert(sem_vsync_end);
    sem_gui_ready = xSemaphoreCreateBinary();
    assert(sem_gui_ready);
#endif

    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << GPIO_LCD_BL};
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    ESP_LOGI(TAG, "Install RGB LCD panel driver");
    
    esp_lcd_rgb_panel_config_t panel_config = {
        .data_width = 16, // RGB565 in parallel mode, thus 16bit in width
        .psram_trans_align = 64,
#if CONFIG_EXAMPLE_USE_BOUNCE_BUFFER
        .bounce_buffer_size_px = 10 * LCD_WIDTH,
#endif
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .disp_gpio_num = GPIO_NUM_NC,
        .pclk_gpio_num = GPIO_LCD_PCLK,
        .vsync_gpio_num = GPIO_LCD_VSYNC,
        .hsync_gpio_num = GPIO_LCD_HSYNC,
        .de_gpio_num = GPIO_LCD_DE,
        .data_gpio_nums = {
            GPIO_LCD_B0,
            GPIO_LCD_B1,
            GPIO_LCD_B2,
            GPIO_LCD_B3,
            GPIO_LCD_B4,
            GPIO_LCD_G0,
            GPIO_LCD_G1,
            GPIO_LCD_G2,
            GPIO_LCD_G3,
            GPIO_LCD_G4,
            GPIO_LCD_G5,
            GPIO_LCD_R0,
            GPIO_LCD_R1,
            GPIO_LCD_R2,
            GPIO_LCD_R3,
            GPIO_LCD_R4,
        },
        .timings = {
            .h_res = LCD_WIDTH, .v_res = LCD_HEIGHT,
    // The following parameters should refer to LCD spec
#if ((LCD_5r0_800x480==1)||(LCD_4r3_800x480==1))
            .pclk_hz = 15 * 1000 * 1000,
            .hsync_back_porch = 42,
            .hsync_front_porch = 20,
            .hsync_pulse_width = 1,
            .vsync_back_porch = 12,
            .vsync_front_porch = 4,
            .vsync_pulse_width = 10,
#elif(LCD_7r0_800x480==1) 
            .pclk_hz = 15 * 1000 * 1000,
            .hsync_back_porch = 32,
            .hsync_front_porch = 10,
            .hsync_pulse_width = 16,
            .vsync_back_porch = 12,
            .vsync_front_porch = 15,
            .vsync_pulse_width = 3,            
#elif(LCD_4r0_480x480==1) 
            .pclk_hz = 10 * 1000 * 1000,
            .hsync_back_porch = 32,
            .hsync_front_porch = 10,
            .hsync_pulse_width = 16,
            .vsync_back_porch = 12,
            .vsync_front_porch = 15,
            .vsync_pulse_width = 3,
            
#elif(LCD_5r0_480x854==1) //EP5008S
            .pclk_hz = pclk_hz_value,
            .hsync_back_porch = 30,
            .hsync_front_porch = 12,
            .hsync_pulse_width = 6,
            .vsync_back_porch = 30,
            .vsync_front_porch = 12,
            .vsync_pulse_width = 1,

#elif(LCD_4r3_480x272==1) 
            .pclk_hz = 10 * 1000 * 1000,
            .hsync_back_porch = 43,
            .hsync_front_porch = 75,
            .hsync_pulse_width = 4,
            .vsync_back_porch = 12,
            .vsync_front_porch = 8,
            .vsync_pulse_width = 4,
#endif

#if ((LCD_4r0_480x480==1)||(LCD_5r0_480x854==1))
            .flags.pclk_active_neg = false,
#else
            .flags.pclk_active_neg = true,
#endif
        },
        .flags.fb_in_psram = true,
#if CONFIG_EXAMPLE_DOUBLE_FB
        .flags.double_fb = true,
#endif
    };
    ESP_ERROR_CHECK(esp_lcd_new_rgb_panel(&panel_config, &panel_handle));

    esp_lcd_rgb_panel_event_callbacks_t cbs = {
        .on_vsync = rgb_on_vsync_event,
    };
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_register_event_callbacks(panel_handle, &cbs, &disp_drv));

    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));

    ESP_LOGI(TAG, "Turn on LCD backlight");
    gpio_set_level(GPIO_LCD_BL, 1);
    ESP_LOGI(TAG, "LCD resolution: %dx%d", LCD_WIDTH, LCD_HEIGHT);

    static lv_disp_draw_buf_t disp_buf;
    void *buf1 = NULL;
    void *buf2 = NULL;
#if CONFIG_EXAMPLE_DOUBLE_FB
    ESP_LOGI(TAG, "Use frame buffers as LVGL draw buffers");
    ESP_ERROR_CHECK(esp_lcd_rgb_panel_get_frame_buffer(panel_handle, 2, &buf1, &buf2));
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_WIDTH * LCD_HEIGHT);
#else
    ESP_LOGI(TAG, "Allocate separate LVGL draw buffers from PSRAM");
    uint16_t fact = LCD_HEIGHT;
    buf1 = heap_caps_malloc(LCD_WIDTH * fact * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf1);
#if 1
    buf2 = heap_caps_malloc(LCD_WIDTH * fact * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    assert(buf2);
    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, LCD_WIDTH * fact);
#else
    lv_disp_draw_buf_init(&disp_buf, buf1, NULL, LCD_WIDTH * fact);
#endif
#endif

    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_WIDTH;
    disp_drv.ver_res = LCD_HEIGHT;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &disp_buf;
    disp_drv.user_data = panel_handle;

    // Enable software rotation
    disp_drv.sw_rotate = 1;
    disp_drv.rotated = LV_DISP_ROT_270;

#if CONFIG_EXAMPLE_DOUBLE_FB
    disp_drv.full_refresh = true;
#endif

    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/
static bool rgb_on_vsync_event(esp_lcd_panel_handle_t panel, const esp_lcd_rgb_panel_event_data_t *event_data, void *user_data)
{
    BaseType_t high_task_awoken = pdFALSE;
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    if (xSemaphoreTakeFromISR(sem_gui_ready, &high_task_awoken) == pdTRUE)
    {
        xSemaphoreGiveFromISR(sem_vsync_end, &high_task_awoken);
    }
#endif
    return high_task_awoken == pdTRUE;
}

static void lvgl_flush_cb(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_lcd_panel_handle_t panel_handle = (esp_lcd_panel_handle_t)drv->user_data;
    int offsetx1 = area->x1;
    int offsetx2 = area->x2;
    int offsety1 = area->y1;
    int offsety2 = area->y2;
    
#if CONFIG_EXAMPLE_AVOID_TEAR_EFFECT_WITH_SEM
    BaseType_t res;
    // Give semaphore to signal RGB panel that GUI is ready to send a new frame
    xSemaphoreGive(sem_gui_ready);
    
    // Wait for vsync with increased timeout to accommodate WiFi operations
    res = xSemaphoreTake(sem_vsync_end, pdMS_TO_TICKS(2000));
    if (res != pdTRUE) {
        ESP_LOGW("LVGL_DISP", "Timeout waiting for VSYNC - proceeding anyway");
        // Force a short delay to avoid rendering conflicts
        vTaskDelay(pdMS_TO_TICKS(10));
    }
#endif

    // Draw the bitmap
    esp_lcd_panel_draw_bitmap(panel_handle, offsetx1, offsety1, offsetx2 + 1, offsety2 + 1, color_map);
    
    // Notify LVGL that flush is done
    lv_disp_flush_ready(drv);
}

static void lv_tick_inc_cb(void *data)
{
    uint32_t tick_inc_period_ms = *((uint32_t *)data);
    lv_tick_inc(tick_inc_period_ms);
}

esp_err_t lv_port_tick_init(void)
{
    static const uint32_t tick_inc_period_ms = 2;
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = lv_tick_inc_cb,
        .arg = &tick_inc_period_ms,
        .name = "lvgl_tick",
    };

    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, tick_inc_period_ms * 1000));

    return ESP_OK;
}

uint8_t IMG_4143_map[];

void lcd_draw_picture_test()
{
    // Assuming same dimensions: 120x39
    const uint16_t width = 480;
    const uint16_t height = 854;
    
    // Calculate the size needed for 1-bit data
    // Each byte contains 8 pixels, so we divide width by 8 and round up
    const uint16_t bytes_per_row = (width + 7) / 8;
    
    // Allocate buffer for the converted image
    uint16_t *pixels = heap_caps_malloc(width * height * sizeof(uint16_t), MALLOC_CAP_8BIT | MALLOC_CAP_INTERNAL);
    
    // Convert 1-bit alpha to 16-bit color
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            // Calculate byte position and bit position
            uint16_t byte_pos = y * bytes_per_row + (x / 8);
            uint8_t bit_pos = 7 - (x % 8);  // MSB first
            
            // Get the bit value from the alpha array
            uint8_t alpha = (IMG_4143_map[byte_pos] >> bit_pos) & 0x01;
            
            // Convert to 16-bit color: 0 for black, 0xFFFF for white
            pixels[y * width + x] = alpha ? 0xFFFF : 0x0000;
        }
    }

    // Draw the bitmap
    esp_lcd_panel_draw_bitmap(panel_handle, 100, 100, 100+width, 100+height, pixels);
    
    // Free the allocated memory
    heap_caps_free(pixels);
}



#else
typedef int keep_pedantic_happy;
#endif