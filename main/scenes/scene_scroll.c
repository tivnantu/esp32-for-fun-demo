/*
 * 场景：滚动动画。
 *
 * 每帧整屏重绘（15 个横带，共 307,200 字节）。用于目视评估刷新是否流畅，
 * 并通过串口日志中的整屏渲染入队耗时判断带宽是否达到预期。
 *
 * 规格书 02-electrical-and-timing.md 第 2 节给出整屏填充的验收界值。
 */

#include <stdio.h>

#include "bsp_display.h"
#include "gfx.h"
#include "scenes.h"

#define BAR_HEIGHT       48
#define BAR_PERIOD       120 /* 往返一个周期的帧数 */
#define PROGRESS_PERIOD  200
#define PROGRESS_TOP     (BSP_DISPLAY_HEIGHT - 40)
#define PROGRESS_HEIGHT  12

static uint32_t s_tick;

static void frame(void)
{
    s_tick++;
}

static int bar_position(void)
{
    const int travel = BSP_DISPLAY_HEIGHT - BAR_HEIGHT;
    const int half = BAR_PERIOD / 2;
    const int phase = (int)(s_tick % BAR_PERIOD);

    return phase < half ? (phase * travel) / half : ((BAR_PERIOD - phase) * travel) / half;
}

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT, gfx_panel_color(0x10, 0x18, 0x40));

    scene_fill_abs(canvas, band_y, 0, bar_position(), BSP_DISPLAY_WIDTH, BAR_HEIGHT,
                   gfx_panel_color(0x00, 0xC0, 0xFF));

    const int progress = (int)(s_tick % PROGRESS_PERIOD);
    const int filled = BSP_DISPLAY_WIDTH * progress / PROGRESS_PERIOD;
    scene_fill_abs(canvas, band_y, 0, PROGRESS_TOP, BSP_DISPLAY_WIDTH, PROGRESS_HEIGHT,
                   gfx_panel_color(0x30, 0x30, 0x30));
    scene_fill_abs(canvas, band_y, 0, PROGRESS_TOP, filled, PROGRESS_HEIGHT, gfx_panel_color(0x00, 0xFF, 0x00));

    char line[24];
    snprintf(line, sizeof(line), "frame %lu", (unsigned long)s_tick);
    scene_text_centered(canvas, band_y, BSP_DISPLAY_WIDTH, 4, line, 1, gfx_panel_color(0xFF, 0xFF, 0xFF));
}

const demo_scene_t scene_scroll = {
    .name = "scroll",
    .description = "滚动动画：每帧整屏重绘，评估带宽与流畅度",
    .animated = true,
    .interval_ms = 30,
    .enter = NULL,
    .render_band = render_band,
    .frame = frame,
};
