#include "bsp_key.h"

#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "bsp_key";

#define KEY_ACTIVE_LEVEL 0
#define KEY_DEBOUNCE_MS  20

static bool s_initialized;
static bool s_was_down;

esp_err_t bsp_key_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    const gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << CONFIG_BSP_BOOT_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&cfg), TAG, "按键 GPIO 配置失败");

    s_was_down = false;
    s_initialized = true;
    ESP_LOGI(TAG, "就绪：GPIO%d", CONFIG_BSP_BOOT_GPIO);
    return ESP_OK;
}

bool bsp_key_take_press(void)
{
    if (!s_initialized) {
        return false;
    }

    const bool down = gpio_get_level(CONFIG_BSP_BOOT_GPIO) == KEY_ACTIVE_LEVEL;
    bool pressed = false;

    if (down && !s_was_down) {
        vTaskDelay(pdMS_TO_TICKS(KEY_DEBOUNCE_MS));
        pressed = gpio_get_level(CONFIG_BSP_BOOT_GPIO) == KEY_ACTIVE_LEVEL;
    }
    s_was_down = down;
    return pressed;
}
