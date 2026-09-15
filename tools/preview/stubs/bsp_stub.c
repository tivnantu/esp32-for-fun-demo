/*
 * 宿主侧替代实现。
 *
 * 场景在 enter()/frame()/exit() 中调用背光接口。宿主侧没有硬件，这里不产生
 * 任何设备行为，只记录最近一次设置值，供 preview 断言"离开场景时确实还原了
 * 硬件状态"。
 */

#include "bsp_stub.h"

#include "bsp_backlight.h"

static int s_last_percent = -1;

esp_err_t bsp_backlight_set_percent(int percent)
{
    s_last_percent = percent;
    return ESP_OK;
}

esp_err_t bsp_backlight_set(bool on)
{
    (void)on;
    return ESP_OK;
}

int stub_backlight_last_percent(void)
{
    return s_last_percent;
}

void stub_backlight_reset(void)
{
    s_last_percent = -1;
}
