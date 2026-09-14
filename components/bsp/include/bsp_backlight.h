/*
 * 背光接口。
 *
 * 背光控制引脚为高电平点亮，上电默认熄灭，故初始化后须显式点亮。
 * PWM 调光由 LEDC 提供。
 */

#pragma once

#include "esp_err.h"

#include <stdbool.h>

/* 初始化背光 GPIO 与 LEDC 通道。不点亮背光。 */
esp_err_t bsp_backlight_init(void);

/* 点亮或熄灭。点亮时恢复最近一次设定的亮度。 */
esp_err_t bsp_backlight_set(bool on);

/* 按百分比设定亮度，0 为熄灭，100 为最大。 */
esp_err_t bsp_backlight_set_percent(int percent);
