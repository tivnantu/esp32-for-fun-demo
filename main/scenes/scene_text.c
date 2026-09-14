/*
 * 场景：文本渲染。
 *
 * 演示 8x8 位图字体的多倍放大，以及 4 像素对齐下的居中计算。
 * 居中块宽 264 像素置于 320 像素面板时，起止为 28 与 292，均为 4 的倍数。
 */

#include <stdio.h>

#include "bsp_display.h"
#include "gfx.h"
#include "scenes.h"

#define MESSAGE "Hello World"

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    const uint16_t bg = gfx_panel_color(0x10, 0x18, 0x40);
    const uint16_t fg = gfx_panel_color(0xFF, 0xFF, 0xFF);
    const uint16_t dim = gfx_panel_color(0x80, 0x90, 0xB0);

    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT, bg);

    int y = 16;
    scene_text_abs(canvas, band_y, 8, y, "scale 1  ABCDEFGHIJKLM", 1, fg);
    y += 24;
    scene_text_abs(canvas, band_y, 8, y, "scale 2  ABCDEFGH", 2, fg);
    y += 32;
    scene_text_abs(canvas, band_y, 8, y, "scale 3", 3, fg);
    y += 40;
    scene_text_abs(canvas, band_y, 8, y, "scale 4", 4, fg);
    y += 56;

    scene_text_centered(canvas, band_y, BSP_DISPLAY_WIDTH, y, MESSAGE, 3, fg);
    y += 40;

    /* 居中的列范围须满足 4 像素对齐，否则推送会被拒绝。 */
    const int width = gfx_text_width(MESSAGE, 3);
    const int x0 = (BSP_DISPLAY_WIDTH - width) / 2;
    const int x1 = x0 + width;

    char line[32];
    snprintf(line, sizeof(line), "x %d..%d", x0, x1);
    scene_text_centered(canvas, band_y, BSP_DISPLAY_WIDTH, y, line, 2, dim);
    y += 28;
    scene_text_centered(canvas, band_y, BSP_DISPLAY_WIDTH, y, "align4 ok", 2, dim);

    scene_text_abs(canvas, band_y, 8, BSP_DISPLAY_HEIGHT - 24, "8x8 bitmap font, integer scaling", 1, dim);
}

const demo_scene_t scene_text = {
    .name = "text",
    .description = "文本渲染与 4 像素对齐的居中计算",
    .animated = false,
    .interval_ms = 0,
    .enter = NULL,
    .render_band = render_band,
    .frame = NULL,
};
