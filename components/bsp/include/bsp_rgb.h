/*
 * RGB 指示灯。
 *
 * 器件为 XL-5050RGBC-WS2812B（单线、内置控制 IC，5 V 供电），
 * 数据引脚 IO40。经 Espressif led_strip 组件驱动，不手写 WS2812B 时序。
 *
 * 板上为单颗灯珠。
 */

#pragma once

#include "esp_err.h"

#include <stdint.h>

/* 板上灯珠数量。 */
#define BSP_RGB_LED_COUNT 1

esp_err_t bsp_rgb_init(void);

/* 设置第 index 颗灯珠的颜色，并立即刷新。 */
esp_err_t bsp_rgb_set(int index, uint8_t red, uint8_t green, uint8_t blue);

/* 熄灭全部灯珠。 */
esp_err_t bsp_rgb_clear(void);
