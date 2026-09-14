/*
 * 宿主侧编译场景所需的最小替代头。
 *
 * 场景源文件仅通过 bsp_display.h 间接引入 esp_err.h，用于函数返回类型；
 * 在宿主机上不需要其定义。本文件使场景代码与 gfx 一样可以脱离 ESP-IDF
 * 编译，从而支持离线渲染校验。
 */

#pragma once

typedef int esp_err_t;

#define ESP_OK 0

/* 宿主侧无中止语义，退化为求值。 */
#define ESP_ERROR_CHECK(expr) ((void)(expr))
