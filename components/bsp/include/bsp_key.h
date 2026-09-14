/*
 * BOOT 按键接口。
 *
 * 按键为低有效、外部 10 kΩ 上拉。本模块负责去抖与边沿检测，
 * 应用层只需询问"是否发生了一次按下"。
 *
 * 注意：GPIO0 同时是启动配置（strapping）引脚。上电或复位时被拉低
 * 会进入下载模式，其余时间可作普通输入。见规格书 01-hardware.md 第 2.9 节。
 */

#pragma once

#include "esp_err.h"

#include <stdbool.h>

esp_err_t bsp_key_init(void);

/* 返回 true 表示自上次调用以来发生了一次按下。已去抖。 */
bool bsp_key_take_press(void);
