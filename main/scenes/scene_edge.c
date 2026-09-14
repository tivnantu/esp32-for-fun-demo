/*
 * 场景：边缘诊断。
 *
 * 目的：区分两种"接缝"来源。
 *
 *   A. 分带推送引入的接缝 —— 只在推送边界（y 为 32 的整数倍）出现
 *   B. 面板自身在强对比水平边缘上的串扰 —— 与推送边界无关
 *
 * 做法：在若干条 y 位置上放置黑白硬边，其中一半正好落在推送边界上，
 * 另一半刻意落在边界之间。每条边用红色短标与文字标注其 y 值。
 *
 * 判读：
 *   仅落在推送边界的那些边发虚  → 是 A，属驱动使用问题
 *   两类边都发虚                → 是 B，属面板特性
 */

#include <stdio.h>

#include "bsp_display.h"
#include "gfx.h"
#include "scenes.h"

/* 前三个为 32 的整数倍（推送边界），后三个不是。 */
static const int kEdges[] = {32, 96, 160, 100, 200, 300};
static const bool kOnBandBoundary[] = {true, true, true, false, false, false};
#define EDGE_COUNT (sizeof(kEdges) / sizeof(kEdges[0]))

#define MARKER_WIDTH 44

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    const uint16_t white = gfx_panel_color(0xFF, 0xFF, 0xFF);
    const uint16_t black = gfx_panel_color(0x00, 0x00, 0x00);
    const uint16_t marker = gfx_panel_color(0xFF, 0x00, 0x00);

    int top = 0;
    bool bright = false;

    for (size_t i = 0; i < EDGE_COUNT; i++) {
        const int bottom = kEdges[i];
        if (bottom > top) {
            scene_fill_abs(canvas, band_y, 0, top, BSP_DISPLAY_WIDTH, bottom - top, bright ? white : black);
        }
        top = bottom;
        bright = !bright;
    }
    if (top < BSP_DISPLAY_HEIGHT) {
        scene_fill_abs(canvas, band_y, 0, top, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT - top, bright ? white : black);
    }

    /* 每条边上放红色短标，并标注 y 值与是否落在推送边界。 */
    for (size_t i = 0; i < EDGE_COUNT; i++) {
        const int y = kEdges[i];
        scene_fill_abs(canvas, band_y, 0, y - 1, MARKER_WIDTH, 2, marker);

        char label[24];
        snprintf(label, sizeof(label), "y=%d %s", y, kOnBandBoundary[i] ? "BND" : "mid");
        scene_text_abs(canvas, band_y, MARKER_WIDTH + 4, y - 4, label, 1, marker);
    }
}

const demo_scene_t scene_edge = {
    .name = "edge",
    .description = "边缘诊断：区分推送边界接缝与面板串扰",
    .animated = false,
    .interval_ms = 0,
    .enter = NULL,
    .render_band = render_band,
    .frame = NULL,
};
