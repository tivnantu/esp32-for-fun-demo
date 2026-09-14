/*
 * 场景：测试图样。
 *
 * 构图：四个内缩区块，块间以背景色留白分隔。区块不与屏幕边缘或彼此直接
 * 相接，因此不存在两种高对比内容硬碰硬形成的边界 —— 硬边界在实机上容易
 * 被误读为"溢出"或"发虚"。
 *
 *   区块 1  8 条竖彩带     色彩还原、反色、字节序
 *   区块 2  16 级灰阶阶梯  灰度线性；RGB565 只有 5/6 位，连续渐变必然呈现
 *                          约 32 级台阶，故此处刻意做成阶梯而非渐变
 *   区块 3  4 像素对齐条纹 列寻址与对齐；任一族列错位都会破坏条纹规律
 *   区块 4  8 像素棋盘     像素映射与局部寻址
 *
 * 列寻址的完整性由区块 3 承担，不再依赖灰阶的连续性。
 */

#include <stddef.h>
#include <stdint.h>

#include "bsp_display.h"
#include "gfx.h"
#include "scenes.h"

#define MARGIN      8
#define CONTENT_X   MARGIN
#define CONTENT_W   (BSP_DISPLAY_WIDTH - 2 * MARGIN) /* 304 */
#define GAP         8

#define BARS_Y      8
#define BARS_H      224

#define GRAY_Y      (BARS_Y + BARS_H + GAP)  /* 240 */
#define GRAY_STEPS  16
#define GRAY_H      64

#define STRIPE_Y    (GRAY_Y + GRAY_H + GAP)  /* 312 */
#define STRIPE_H    64
#define STRIPE_W    4

#define CHECK_Y     (STRIPE_Y + STRIPE_H + GAP)                    /* 384 */
#define CHECK_H     (BSP_DISPLAY_HEIGHT - CHECK_Y - MARGIN)        /* 88 */
#define CHECK_CELL  8

static const uint8_t kBars[8][3] = {
    {0xFF, 0xFF, 0xFF}, {0xFF, 0xFF, 0x00}, {0x00, 0xFF, 0xFF}, {0x00, 0xFF, 0x00},
    {0xFF, 0x00, 0xFF}, {0xFF, 0x00, 0x00}, {0x00, 0x00, 0xFF}, {0x00, 0x00, 0x00},
};

static void render_bands(gfx_canvas_t *canvas, int band_y)
{
    const int bar_width = CONTENT_W / 8;

    for (int i = 0; i < 8; i++) {
        scene_fill_abs(canvas, band_y, CONTENT_X + i * bar_width, BARS_Y, bar_width, BARS_H,
                       gfx_panel_color(kBars[i][0], kBars[i][1], kBars[i][2]));
    }
}

static void render_grays(gfx_canvas_t *canvas, int band_y)
{
    if (!scene_visible(canvas, band_y, GRAY_Y, GRAY_H)) {
        return;
    }

    const int step_width = CONTENT_W / GRAY_STEPS;
    for (int i = 0; i < GRAY_STEPS; i++) {
        const uint8_t v = (uint8_t)(i * 255 / (GRAY_STEPS - 1));
        gfx_fill_rect(canvas, CONTENT_X + i * step_width, GRAY_Y - band_y, step_width, GRAY_H,
                      gfx_panel_color(v, v, v));
    }
}

static void render_stripes(gfx_canvas_t *canvas, int band_y)
{
    if (!scene_visible(canvas, band_y, STRIPE_Y, STRIPE_H)) {
        return;
    }

    const uint16_t white = gfx_panel_color(0xFF, 0xFF, 0xFF);
    const uint16_t black = gfx_panel_color(0x00, 0x00, 0x00);

    for (int x = 0; x < CONTENT_W; x += STRIPE_W) {
        const bool on = ((x / STRIPE_W) % 2) == 0;
        gfx_fill_rect(canvas, CONTENT_X + x, STRIPE_Y - band_y, STRIPE_W, STRIPE_H, on ? white : black);
    }
}

static void render_checker(gfx_canvas_t *canvas, int band_y)
{
    if (!scene_visible(canvas, band_y, CHECK_Y, CHECK_H)) {
        return;
    }

    const uint16_t white = gfx_panel_color(0xFF, 0xFF, 0xFF);
    const uint16_t black = gfx_panel_color(0x00, 0x00, 0x00);

    for (int y = 0; y < CHECK_H; y += CHECK_CELL) {
        for (int x = 0; x < CONTENT_W; x += CHECK_CELL) {
            const bool on = (((x / CHECK_CELL) + (y / CHECK_CELL)) % 2) == 0;
            gfx_fill_rect(canvas, CONTENT_X + x, CHECK_Y + y - band_y, CHECK_CELL, CHECK_CELL, on ? white : black);
        }
    }
}

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT,
                   gfx_panel_color(0x10, 0x18, 0x40));

    render_bands(canvas, band_y);
    render_grays(canvas, band_y);
    render_stripes(canvas, band_y);
    render_checker(canvas, band_y);

    scene_text_centered(canvas, band_y, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT - MARGIN, "ST77922 QSPI 320x480", 1,
                        gfx_panel_color(0x80, 0x90, 0xB0));
}

const demo_scene_t scene_pattern = {
    .name = "pattern",
    .description = "测试图样：彩带、灰阶阶梯、对齐条纹、像素棋盘",
    .animated = false,
    .interval_ms = 0,
    .enter = NULL,
    .render_band = render_band,
    .frame = NULL,
};
