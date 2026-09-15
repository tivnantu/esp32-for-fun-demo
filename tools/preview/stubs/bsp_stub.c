/*
 * 宿主侧替代实现。
 *
 * 场景在 enter()/frame()/exit() 中调用背光接口。宿主侧没有硬件，这里不产生
 * 任何设备行为，只把调用记录下来，供 preview 断言状态机的效果与还原动作。
 */

#include "bsp_stub.h"

#include "bsp_backlight.h"

static int s_percent_calls;
static int s_last_percent = -1;
static int s_set_calls;

esp_err_t bsp_backlight_set_percent(int percent)
{
    s_percent_calls++;
    s_last_percent = percent;
    return ESP_OK;
}

esp_err_t bsp_backlight_set(bool on)
{
    (void)on;
    s_set_calls++;
    return ESP_OK;
}

int stub_backlight_percent_calls(void)
{
    return s_percent_calls;
}

int stub_backlight_last_percent(void)
{
    return s_last_percent;
}

int stub_backlight_set_calls(void)
{
    return s_set_calls;
}

void stub_backlight_reset(void)
{
    s_percent_calls = 0;
    s_last_percent = -1;
    s_set_calls = 0;
}
