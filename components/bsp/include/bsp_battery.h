/*
 * 电池电压检测。
 *
 * 电池电压经 2:1 分压后接入 IO8（ADC1 通道 7），因此实际电压为读数两倍。
 * 分压网络见规格书 01-hardware.md 第 2.6 节与第 3 节。
 *
 * 注意：ESP32-S3 的 ADC 存在固有的非线性与个体偏差，未经单板标定时
 * 绝对精度有限，本模块以百分比形式给出的电量仅作趋势参考。
 */

#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stdint.h>

esp_err_t bsp_battery_init(void);

/* 读取电池电压，单位毫伏。 */
esp_err_t bsp_battery_read_mv(int *out_mv);

/*
 * 读取电池电压，单位毫伏，失败时返回 0。
 * 供显示类代码在无法处理错误时使用。
 */
int bsp_battery_read_mv_or_zero(void);
