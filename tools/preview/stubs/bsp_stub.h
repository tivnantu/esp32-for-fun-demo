/*
 * 宿主侧桩件的观测接口。
 *
 * 场景的 enter/exit 钩子会改动硬件状态。宿主侧没有真实硬件，桩件把调用
 * 记录下来，供校验代码断言"离开场景时确实还原了状态"。
 */

#pragma once

#include <stdbool.h>

/* 从上次清零以来，bsp_backlight_set_percent() 被调用的次数。 */
int stub_backlight_percent_calls(void);

/* 最近一次 bsp_backlight_set_percent() 的参数；从未调用时返回 -1。 */
int stub_backlight_last_percent(void);

/* 从上次清零以来，bsp_backlight_set() 被调用的次数。 */
int stub_backlight_set_calls(void);

/* 清零全部记录。 */
void stub_backlight_reset(void);
