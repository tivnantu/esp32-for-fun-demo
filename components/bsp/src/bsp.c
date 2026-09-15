#include "bsp.h"

#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "bsp";

/*
 * 可选子系统允许在硬件上不存在（未插卡、未接喇叭、未焊编码器）。
 * 初始化失败只降级该项功能，不阻断启动。
 */
#if CONFIG_BSP_ENABLE_TOUCH || CONFIG_BSP_ENABLE_RGB || CONFIG_BSP_ENABLE_AUDIO || CONFIG_BSP_ENABLE_BATTERY || \
    CONFIG_BSP_ENABLE_SDCARD
static void init_optional(const char *name, esp_err_t (*init_fn)(void))
{
    const esp_err_t err = init_fn();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "%s 不可用（%s），相关功能与场景将不可用", name, esp_err_to_name(err));
    }
}
#endif

esp_err_t bsp_init(void)
{
    /* 必需子系统：失败即中止。 */
    ESP_RETURN_ON_ERROR(bsp_display_init(), TAG, "显示初始化失败");
    ESP_RETURN_ON_ERROR(bsp_backlight_init(), TAG, "背光初始化失败");
    ESP_RETURN_ON_ERROR(bsp_key_init(), TAG, "按键初始化失败");

#if CONFIG_BSP_ENABLE_I2C_BUS
    /* I2C 总线为触摸与音频共用，属于必需项。 */
    ESP_RETURN_ON_ERROR(bsp_i2c_init(), TAG, "I2C 总线初始化失败");
#endif

    /* 可选子系统：失败仅告警。 */
#if CONFIG_BSP_ENABLE_TOUCH
    init_optional("触摸", bsp_touch_init);
#endif

#if CONFIG_BSP_ENABLE_RGB
    init_optional("RGB 指示灯", bsp_rgb_init);
#endif

#if CONFIG_BSP_ENABLE_AUDIO
    init_optional("音频", bsp_audio_init);
#endif

#if CONFIG_BSP_ENABLE_BATTERY
    init_optional("电池检测", bsp_battery_init);
#endif

#if CONFIG_BSP_ENABLE_SDCARD
    init_optional("MicroSD", bsp_sdcard_init);
#endif

    return ESP_OK;
}
