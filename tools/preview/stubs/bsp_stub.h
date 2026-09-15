/*
 * 宿主侧桩件的观测接口。
 *
 * 场景的 enter/exit 钩子会改动硬件状态。宿主侧没有真实硬件，桩件把调用
 * 记录下来，供校验代码断言"离开场景时确实还原了状态"。
 */

#pragma once

/* 最近一次 bsp_backlight_set_percent() 的参数；从未调用时返回 -1。 */
int stub_backlight_last_percent(void);

/* 清零记录。 */
void stub_backlight_reset(void);
