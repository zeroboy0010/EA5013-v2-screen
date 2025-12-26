#include "screen_control.h"

#include "driver/gpio.h"
#include "esp_log.h"

#include "bsp_board.h"  // GPIO_LCD_BL

static const char *TAG = "screen_control";

static bool s_inited = false;
static bool s_power_on = true;

void screen_control_init(void)
{
    if (s_inited) {
        return;
    }

    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << GPIO_LCD_BL,
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    s_inited = true;
    screen_set_power(s_power_on);
}

void screen_set_power(bool on)
{
    s_power_on = on;

    if (!s_inited) {
        // Defer actual GPIO write until initialized.
        return;
    }

    gpio_set_level(GPIO_LCD_BL, on ? 1 : 0);
    ESP_LOGI(TAG, "Screen power %s (backlight GPIO %d)", on ? "ON" : "OFF", (int)GPIO_LCD_BL);
}

bool screen_get_power(void)
{
    return s_power_on;
}
