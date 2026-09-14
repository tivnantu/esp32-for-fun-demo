/*
 * 场景：RGB 指示灯。
 *
 * 板上一颗 WS2812B，依次显示红、绿、蓝、白、灭。
 * 该场景仅在 CONFIG_BSP_ENABLE_RGB 打开时编译。
 */

#include <stdio.h>

#include "bsp_display.h"
#include "bsp_rgb.h"
#include "gfx.h"
#include "scenes.h"

typedef struct {
    const char *name;
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} rgb_step_t;

/* WS2812B 单色满亮对应分量 255；此处逐档递增以便看清通道顺序。 */
static const rgb_step_t kSteps[] = {
    {"红", 255, 0, 0},
    {"绿", 0, 255, 0},
    {"蓝", 0, 0, 255},
    {"白", 255, 255, 255},
    {"灭", 0, 0, 0},
};
#define STEP_COUNT (sizeof(kSteps) / sizeof(kSteps[0]))

/* 每档持续帧数。interval_ms 为 100，故每档约 900 ms。 */
#define STEP_FRAMES 9

static size_t s_step;
static uint32_t s_frames_in_step;

static void apply_step(void)
{
    const rgb_step_t *step = &kSteps[s_step];
    bsp_rgb_set(0, step->red, step->green, step->blue);
}

static void enter(void)
{
    s_step = 0;
    s_frames_in_step = 0;
    apply_step();
}

static void leave(void)
{
    /* 熄灭灯珠：灯珠会保持最后一次设置的颜色，不主动熄灭会一直亮着。 */
    bsp_rgb_clear();
}

static void frame(void)
{
    if (++s_frames_in_step < STEP_FRAMES) {
        return;
    }
    s_frames_in_step = 0;
    s_step = (s_step + 1) % STEP_COUNT;
    apply_step();
}

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    const uint16_t bg = gfx_panel_color(0x10, 0x10, 0x14);
    const uint16_t fg = gfx_panel_color(0xFF, 0xFF, 0xFF);
    const uint16_t dim = gfx_panel_color(0x80, 0x80, 0x90);
    const rgb_step_t *step = &kSteps[s_step];

    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT, bg);

    scene_text_abs(canvas, band_y, 8, 16, "RGB LED", 3, fg);
    scene_text_abs(canvas, band_y, 8, 56, "XL-5050RGBC-WS2812B  GPIO40", 1, dim);

    char line[48];
    snprintf(line, sizeof(line), "now: %s", step->name);
    scene_text_abs(canvas, band_y, 8, 120, line, 3, fg);

    snprintf(line, sizeof(line), "R %3u  G %3u  B %3u", step->red, step->green, step->blue);
    scene_text_abs(canvas, band_y, 8, 170, line, 2, dim);

    /* 屏幕上的色块用同一组分量，便于与灯珠逐色比对。 */
    const uint16_t swatch = gfx_panel_color(step->red, step->green, step->blue);
    scene_fill_abs(canvas, band_y, 8, 220, 304, 200, swatch);

    scene_text_abs(canvas, band_y, 8, BSP_DISPLAY_HEIGHT - 24, "compare with the LED on the board", 1, dim);
}

const demo_scene_t scene_rgb = {
    .name = "rgb",
    .description = "RGB 指示灯：红绿蓝白依次循环",
    .animated = true,
    .interval_ms = 100,
    .enter = enter,
    .exit = leave,
    .render_band = render_band,
    .frame = frame,
};
