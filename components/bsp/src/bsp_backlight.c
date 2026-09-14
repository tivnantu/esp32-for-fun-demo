#include "bsp_backlight.h"

#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "bsp_backlight";

#define BL_LEDC_MODE    LEDC_LOW_SPEED_MODE
#define BL_LEDC_TIMER   LEDC_TIMER_0
#define BL_LEDC_CHANNEL LEDC_CHANNEL_0

/*
 * 满量程取 2^位宽 - 1，避开占空比等于计数器回绕值的边界。
 * 对 10 位即 1023，与理论满量程相差 1/1024，不可见。
 */
#define BL_DUTY_FULL ((1u << CONFIG_BSP_BACKLIGHT_PWM_RESOLUTION_BITS) - 1u)

/* 背光控制：高电平点亮，上电默认熄灭。见规格书 02-electrical-and-timing.md 第 7 节。 */
#define BL_ON_LEVEL 1

static bool s_initialized;
static bool s_on;
static int s_level = 100; /* 由 bsp_backlight_set_percent() 设定，熄灭时保留 */

static esp_err_t apply_duty(int percent)
{
    const uint32_t duty = (uint32_t)(((uint64_t)BL_DUTY_FULL * (uint32_t)percent) / 100u);
    ESP_RETURN_ON_ERROR(ledc_set_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL, duty), TAG, "占空比设置失败");
    return ledc_update_duty(BL_LEDC_MODE, BL_LEDC_CHANNEL);
}

esp_err_t bsp_backlight_init(void)
{
    if (s_initialized) {
        return ESP_OK;
    }

    const gpio_config_t gpio_cfg = {
        .pin_bit_mask = 1ULL << CONFIG_BSP_BACKLIGHT_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&gpio_cfg), TAG, "背光 GPIO 配置失败");
    ESP_RETURN_ON_ERROR(gpio_set_level(CONFIG_BSP_BACKLIGHT_GPIO, !BL_ON_LEVEL), TAG, "背光置灭失败");

    const ledc_timer_config_t timer_config = {
        .speed_mode = BL_LEDC_MODE,
        .duty_resolution = (ledc_timer_bit_t)CONFIG_BSP_BACKLIGHT_PWM_RESOLUTION_BITS,
        .timer_num = BL_LEDC_TIMER,
        .freq_hz = CONFIG_BSP_BACKLIGHT_PWM_FREQ_HZ,
        .clk_cfg = LEDC_AUTO_CLK,
    };
    ESP_RETURN_ON_ERROR(ledc_timer_config(&timer_config), TAG, "LEDC 定时器配置失败");

    const ledc_channel_config_t channel_config = {
        .gpio_num = CONFIG_BSP_BACKLIGHT_GPIO,
        .speed_mode = BL_LEDC_MODE,
        .channel = BL_LEDC_CHANNEL,
        .intr_type = LEDC_INTR_DISABLE,
        .timer_sel = BL_LEDC_TIMER,
        .duty = 0,
        .hpoint = 0,
    };
    ESP_RETURN_ON_ERROR(ledc_channel_config(&channel_config), TAG, "LEDC 通道配置失败");

    s_on = false;
    s_initialized = true;
    ESP_LOGI(TAG, "就绪：GPIO%d，%d Hz，%d 位", CONFIG_BSP_BACKLIGHT_GPIO, CONFIG_BSP_BACKLIGHT_PWM_FREQ_HZ,
             CONFIG_BSP_BACKLIGHT_PWM_RESOLUTION_BITS);
    return ESP_OK;
}

esp_err_t bsp_backlight_set_percent(int percent)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    if (percent < 0) {
        percent = 0;
    } else if (percent > 100) {
        percent = 100;
    }

    if (percent > 0) {
        s_level = percent;
        s_on = true;
    } else {
        s_on = false;
    }
    return apply_duty(s_on ? s_level : 0);
}

esp_err_t bsp_backlight_set(bool on)
{
    if (!s_initialized) {
        return ESP_ERR_INVALID_STATE;
    }
    s_on = on;
    return apply_duty(on ? s_level : 0);
}
