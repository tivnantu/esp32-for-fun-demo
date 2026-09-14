/*
 * 显示接口。
 *
 * 调用方不接触 ST77922 的任何细节：QSPI 命令编码、初始化序列、字节序、
 * 4 像素对齐均由本组件内部处理。
 *
 * 像素格式为面板序 RGB565，由 gfx_panel_color() 生成（见 gfx 组件）。
 */

#pragma once

#include "esp_err.h"

#include <stdint.h>

/* 面板几何（像素）。编译期常量，可用于静态缓冲区尺寸。 */
#define BSP_DISPLAY_WIDTH  320
#define BSP_DISPLAY_HEIGHT 480

/* 运行时查询，便于调用方不依赖宏。 */
int bsp_display_width(void);
int bsp_display_height(void);

/*
 * 初始化 QSPI 总线、面板 IO 与 ST77922 驱动。
 * 不点亮背光 —— 背光由 bsp_backlight_* 独立控制。
 */
esp_err_t bsp_display_init(void);

/*
 * 将面板序像素推送到指定矩形。
 *
 * 坐标须满足 ST77922 双栅极的 4 像素对齐要求（见规格书第 7 节）：
 * x 与 x + w 均须为 4 的倍数。不满足时返回 ESP_ERR_INVALID_ARG。
 */
esp_err_t bsp_display_flush(int x, int y, int w, int h, const uint16_t *pixels);

/* 推送整屏。pixels 须含 width * height 个像素。 */
esp_err_t bsp_display_flush_all(const uint16_t *pixels);
