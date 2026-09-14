/*
 * 演示场景接口。
 *
 * 渲染模型：整个面板按横带渲染。应用层为每个场景顺序提供若干横带，
 * 场景只需回答"这一带画什么"，不接触面板尺寸以外的任何硬件细节。
 *
 *   render_band  渲染一个横带，可为 NULL（表示本场景不绘制）
 *   frame        每帧调用一次，用于推进动画状态，可为 NULL
 *   enter        进入场景时调用，可为 NULL
 *   exit         离开场景时调用，可为 NULL
 *   animated     false：进入时渲染一次；true：每 interval_ms 重绘
 *
 * 状态推进与绘制分离：frame() 只改状态，render_band() 只读状态并绘制。
 *
 * enter/exit 用于建立与还原场景改动的**硬件状态**（灯色、背光亮度、
 * 音频输出）。凡是 enter 中改动且不会自行恢复的硬件状态，都必须在 exit
 * 中还原，否则离开该场景后残留会一直生效并影响后续场景的判断。
 *
 * 契约：render_band() 须完整覆盖该带的全部像素。应用层不预填背景，
 * 未覆盖处将保留上一帧内容。
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "gfx.h"

typedef struct {
    const char *name;
    const char *description;
    bool animated;
    uint32_t interval_ms;
    void (*enter)(void);
    void (*exit)(void);
    void (*render_band)(gfx_canvas_t *canvas, int band_y);
    void (*frame)(void);
} demo_scene_t;

/*
 * 场景按绝对面板坐标作画，由本辅助换算到带内坐标。
 * 应用层始终以整宽推送（x=0，宽度为面板宽），两者均为 4 的倍数，
 * 因此 4 像素对齐约束在推送边界处一次性满足，场景代码无需关心。
 */

/* 判断绝对行区间 [y, y + h) 是否与当前带相交。canvas->height 即带高。 */
static inline bool scene_visible(const gfx_canvas_t *canvas, int band_y, int y, int h)
{
    return (y < band_y + canvas->height) && ((y + h) > band_y);
}

/*
 * 下列辅助是场景唯一的作画入口，可见性判定集中在内部，场景无需自行判断，
 * 也不会漏判。否则每个带都会重算整屏元素，开销随（带数 × 元素数）增长。
 */

static inline void scene_fill_abs(gfx_canvas_t *canvas, int band_y, int x, int y, int w, int h, uint16_t color)
{
    if (!scene_visible(canvas, band_y, y, h)) {
        return;
    }
    gfx_fill_rect(canvas, x, y - band_y, w, h, color);
}

static inline void scene_text_abs(gfx_canvas_t *canvas, int band_y, int x, int y, const char *text, int scale,
                                  uint16_t color)
{
    if (!scene_visible(canvas, band_y, y, gfx_text_height(scale))) {
        return;
    }
    gfx_draw_text(canvas, x, y - band_y, text, scale, color);
}

/* 水平居中绘制。 */
static inline void scene_text_centered(gfx_canvas_t *canvas, int band_y, int panel_width, int y, const char *text,
                                       int scale, uint16_t color)
{
    const int x = (panel_width - gfx_text_width(text, scale)) / 2;
    scene_text_abs(canvas, band_y, x, y, text, scale, color);
}

extern const demo_scene_t scene_orient;
extern const demo_scene_t scene_selftest;
extern const demo_scene_t scene_solid;
extern const demo_scene_t scene_pattern;
extern const demo_scene_t scene_edge;
extern const demo_scene_t scene_text;
extern const demo_scene_t scene_scroll;
extern const demo_scene_t scene_backlight;

#if CONFIG_BSP_ENABLE_TOUCH
extern const demo_scene_t scene_touch;
#endif

#if CONFIG_BSP_ENABLE_RGB
extern const demo_scene_t scene_rgb;
#endif

#if CONFIG_BSP_ENABLE_AUDIO
extern const demo_scene_t scene_audio;
#endif

#if CONFIG_BSP_ENABLE_BATTERY || CONFIG_BSP_ENABLE_SDCARD
extern const demo_scene_t scene_status;
#endif
