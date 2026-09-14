/*
 * 宿主侧替代实现。
 *
 * 背光场景在 frame() 中调用 bsp_backlight_set_percent()。预览工具只调用
 * enter() 与 render_band()，不执行 frame()，因此这里只需提供符号即可，
 * 不产生任何设备行为。
 */

#include "bsp_backlight.h"

esp_err_t bsp_backlight_set_percent(int percent)
{
    (void)percent;
    return ESP_OK;
}

esp_err_t bsp_backlight_set(bool on)
{
    (void)on;
    return ESP_OK;
}
