/*
 * 触摸接口。
 *
 * 控制器集成于 ST77922 内部，经 I2C 访问，从地址 0x55。
 * 协议依据厂商 ST77922_TOUCH 库（资料包 1-示例程序_Demo/Arduino/
 * Install libraries/ST77922_TOUCH）。
 *
 * 坐标原点为面板原生左上角，与显示坐标一致（均为 320x480 竖屏）。
 */

#pragma once

#include "esp_err.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define BSP_TOUCH_MAX_POINTS 10

typedef struct {
    uint16_t x;
    uint16_t y;
} bsp_touch_point_t;

typedef struct {
    /* 控制器报告数据已更新（含抬起事件）。为 false 时其余字段无意义。 */
    bool updated;
    /* 有效触点数，0 表示已无触点。 */
    size_t count;
    bsp_touch_point_t points[BSP_TOUCH_MAX_POINTS];
} bsp_touch_state_t;

esp_err_t bsp_touch_init(void);

/* 控制器是否已初始化并就绪。 */
bool bsp_touch_is_ready(void);

/* 查询一次触摸状态。不做缓存，每次均访问控制器。 */
esp_err_t bsp_touch_poll(bsp_touch_state_t *state);
