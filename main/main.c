// https://www.eya-display.com/

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_system.h"

#include "nvs_flash.h"

#include "lvgl.h"
#include "lv_port_disp.h"
#include "lv_port_indev.h"
// #include "lv_port_fs.h"
#include "lv_demos.h"

#include "clock_screen.h"
#include "clock_config.h"
#include "wifi_config.h"
#include "time_sync.h"
#include "gold_price.h"

static const char *TAG = "https://www.eya-display.com/";
void lvgl_driver_init() // 初始化液晶驱动
{
    ESP_ERROR_CHECK(bsp_i2c_init(I2C_NUM_0, 1000000));
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    lv_port_tick_init();
}
void lv_tick_task(void *arg)// LVGL 时钟任务
{
    while (1)
    {
        vTaskDelay((20) / portTICK_PERIOD_MS);
        lv_task_handler();
    }
}


void Touch_IO_RST(void)
{

#if(GT911==1)
	//touch reset pin  低电平复位
	gpio_reset_pin(39);
	gpio_reset_pin(40);
	gpio_pullup_en(40);
	gpio_pullup_en(39);

	gpio_set_direction(39, GPIO_MODE_OUTPUT);
	gpio_set_direction(40, GPIO_MODE_OUTPUT);
	gpio_set_level(40, 1);
	gpio_set_level(39, 1);
	ESP_LOGI(TAG, "io39 set_high");
	vTaskDelay(pdMS_TO_TICKS(50));
	gpio_pulldown_en(39);
	gpio_pulldown_en(40);
	gpio_set_level(39, 0);
	gpio_set_level(40, 0);
	ESP_LOGI(TAG, "io39 set_low");

	vTaskDelay(pdMS_TO_TICKS(20));
	gpio_set_level(40, 1);
	vTaskDelay(pdMS_TO_TICKS(20));
	gpio_pulldown_en(39);
	gpio_set_level(39, 0);
	//gpio_reset_pin(39);
	//gpio_set_direction(39, GPIO_MODE_INPUT);//中断脚没有用上，这只是用来配置地址
	
#elif(CST3240==1)

    gpio_reset_pin(39);
	gpio_reset_pin(40);
	
	gpio_set_direction( GPIO_NUM_40, GPIO_MODE_OUTPUT);//RST SET PORT OUTPUT
	gpio_set_level(40, 0);        //RST RESET IO
	vTaskDelay(pdMS_TO_TICKS(50));//DELAY 50ms 
	gpio_set_level(40, 1);        //SET RESET IO
	vTaskDelay(pdMS_TO_TICKS(10));//DELAY 10ms 	
#endif

}

// Clock update task
void clock_update_task(void *arg)
{
    while (1) {
        clock_screen_update();
        vTaskDelay(pdMS_TO_TICKS(CLOCK_UPDATE_INTERVAL_MS));
    }
}

// Main LVGL task
void lvgl_task(void *arg)
{
    // Initialize touch and hardware
    Touch_IO_RST();
    lvgl_driver_init();
    
    ESP_LOGI(TAG, "LVGL initialized");
    
    // Create clock screen
    clock_screen_load();
    
    // Create clock update task
    xTaskCreate(clock_update_task, "clock_update", 4096, NULL, 5, NULL);
    
    // Create LVGL tick task
    xTaskCreate(lv_tick_task, "lv_tick_task", 4096, NULL, 1, NULL);
    
    // Main LVGL loop
    while (1) {
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
    
    // Initialize WiFi
    wifi_init();
    
    // Initialize and synchronize time
    time_sync_init();
    
    // Initialize gold price fetcher
    gold_price_init();
    
    // Start LVGL task
    xTaskCreate(lvgl_task, "lvgl_task", 8192, NULL, 5, NULL);
    
    // Start gold price update task
    gold_price_start_task();
    
    ESP_LOGI(TAG, "Clock application started");
}
