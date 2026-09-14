#include "bsp_rgb.h"

#include "esp_check.h"
#include "esp_log.h"
#include "led_strip.h"

static const char *TAG = "bsp_rgb";

/* WS2812B 的位时序由 led_strip 组件按 10 MHz 分辨率生成。 */
#define RGB_RMT_RESOLUTION_HZ (10 * 1000 * 1000)

static led_strip_handle_t s_strip;

esp_err_t bsp_rgb_init(void)
{
    if (s_strip != NULL) {
        return ESP_OK;
    }

    const led_strip_config_t strip_config = {
        .strip_gpio_num = CONFIG_BSP_RGB_GPIO,
        .max_leds = BSP_RGB_LED_COUNT,
        .led_model = LED_MODEL_WS2812,
        .flags = {.invert_out = false},
    };
    const led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = RGB_RMT_RESOLUTION_HZ,
        .flags = {.with_dma = false},
    };
    ESP_RETURN_ON_ERROR(led_strip_new_rmt_device(&strip_config, &rmt_config, &s_strip), TAG, "灯带设备创建失败");
    ESP_RETURN_ON_ERROR(led_strip_clear(s_strip), TAG, "初始熄灭失败");

    ESP_LOGI(TAG, "就绪：GPIO%d，%d 颗灯珠", CONFIG_BSP_RGB_GPIO, BSP_RGB_LED_COUNT);
    return ESP_OK;
}

esp_err_t bsp_rgb_set(int index, uint8_t red, uint8_t green, uint8_t blue)
{
    if (s_strip == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    if (index < 0 || index >= BSP_RGB_LED_COUNT) {
        return ESP_ERR_INVALID_ARG;
    }
    ESP_RETURN_ON_ERROR(led_strip_set_pixel(s_strip, (uint32_t)index, red, green, blue), TAG, "设置颜色失败");
    return led_strip_refresh(s_strip);
}

esp_err_t bsp_rgb_clear(void)
{
    if (s_strip == NULL) {
        return ESP_ERR_INVALID_STATE;
    }
    return led_strip_clear(s_strip);
}
