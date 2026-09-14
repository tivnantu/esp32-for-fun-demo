/*
 * 场景：触摸。
 *
 * 显示触摸点位置与坐标。背景为 40 像素网格，便于判断坐标是否与显示一致。
 *
 * 该场景仅在 CONFIG_BSP_ENABLE_TOUCH 打开时编译。
 */

#include <stdio.h>

#include "bsp_display.h"
#include "bsp_touch.h"
#include "gfx.h"
#include "scenes.h"

#define GRID_STEP  40
#define CROSS_HALF 12

/* 仅在控制器报告更新时替换，因此抬起后画面清空、按住时跟随。 */
static bsp_touch_state_t s_state;

static void frame(void)
{
    bsp_touch_state_t state;
    if (bsp_touch_poll(&state) == ESP_OK && state.updated) {
        s_state = state;
    }
}

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    const uint16_t bg = gfx_panel_color(0x08, 0x0C, 0x20);
    const uint16_t grid = gfx_panel_color(0x20, 0x30, 0x50);
    const uint16_t fg = gfx_panel_color(0xFF, 0xFF, 0xFF);
    const uint16_t mark = gfx_panel_color(0x00, 0xE0, 0x40);

    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT, bg);

    if (scene_visible(canvas, band_y, 0, BSP_DISPLAY_HEIGHT)) {
        for (int x = 0; x < BSP_DISPLAY_WIDTH; x += GRID_STEP) {
            scene_fill_abs(canvas, band_y, x, 0, 1, BSP_DISPLAY_HEIGHT, grid);
        }
        for (int y = 0; y < BSP_DISPLAY_HEIGHT; y += GRID_STEP) {
            scene_fill_abs(canvas, band_y, 0, y, BSP_DISPLAY_WIDTH, 1, grid);
        }
    }

    char line[32];
    snprintf(line, sizeof(line), "touches %u", (unsigned)s_state.count);
    scene_text_abs(canvas, band_y, 8, 8, line, 2, fg);
    scene_text_abs(canvas, band_y, 8, BSP_DISPLAY_HEIGHT - 16, "touch the panel", 1, grid);

    for (size_t i = 0; i < s_state.count; i++) {
        int cx = s_state.points[i].x;
        int cy = s_state.points[i].y;
        if (cx >= BSP_DISPLAY_WIDTH) {
            cx = BSP_DISPLAY_WIDTH - 1;
        }
        if (cy >= BSP_DISPLAY_HEIGHT) {
            cy = BSP_DISPLAY_HEIGHT - 1;
        }

        scene_fill_abs(canvas, band_y, cx - CROSS_HALF, cy, 2 * CROSS_HALF + 1, 1, mark);
        scene_fill_abs(canvas, band_y, cx, cy - CROSS_HALF, 1, 2 * CROSS_HALF + 1, mark);

        snprintf(line, sizeof(line), "%u (%d,%d)", (unsigned)i, cx, cy);
        scene_text_abs(canvas, band_y, cx + CROSS_HALF + 4, cy - 4, line, 1, mark);
    }
}

const demo_scene_t scene_touch = {
    .name = "touch",
    .description = "触摸：触点位置与坐标，背景为 40 像素网格",
    .animated = true,
    .interval_ms = 50,
    .enter = NULL,
    .render_band = render_band,
    .frame = frame,
};
