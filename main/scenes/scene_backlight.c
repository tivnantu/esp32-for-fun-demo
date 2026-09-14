/*
 * 场景：背光调光。
 *
 * 亮度在 5% 与 100% 之间往复，画面上同步显示当前占空比。
 * 用于确认 IO41 的 PWM 通路与低占空比下是否闪烁。
 */

#include <stdio.h>

#include "bsp_display.h"
#include "bsp_backlight.h"
#include "gfx.h"
#include "scenes.h"

#define LEVEL_MIN    5
#define LEVEL_MAX    100
#define LEVEL_STEP   5
#define BAR_TOP      (BSP_DISPLAY_HEIGHT / 2 + 40)
#define BAR_HEIGHT   32

static int s_level = LEVEL_MAX;
static int s_direction = -1;

static void enter(void)
{
    s_level = LEVEL_MAX;
    s_direction = -1;
    ESP_ERROR_CHECK(bsp_backlight_set_percent(s_level));
}

static void leave(void)
{
    /* 还原满亮度：低亮度若残留到后续场景，会干扰对其余画面的判读。 */
    ESP_ERROR_CHECK(bsp_backlight_set_percent(LEVEL_MAX));
}

static void frame(void)
{
    s_level += s_direction * LEVEL_STEP;
    if (s_level <= LEVEL_MIN) {
        s_level = LEVEL_MIN;
        s_direction = 1;
    } else if (s_level >= LEVEL_MAX) {
        s_level = LEVEL_MAX;
        s_direction = -1;
    }
    ESP_ERROR_CHECK(bsp_backlight_set_percent(s_level));
}

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    const uint16_t fg = gfx_panel_color(0xFF, 0xFF, 0xFF);

    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT, gfx_panel_color(0x00, 0x00, 0x00));

    scene_text_centered(canvas, band_y, BSP_DISPLAY_WIDTH, 80, "BACKLIGHT", 3, fg);

    char line[24];
    snprintf(line, sizeof(line), "%d %%", s_level);
    scene_text_centered(canvas, band_y, BSP_DISPLAY_WIDTH, 160, line, 3, fg);

    const int filled = BSP_DISPLAY_WIDTH * s_level / 100;
    scene_fill_abs(canvas, band_y, 0, BAR_TOP, BSP_DISPLAY_WIDTH, BAR_HEIGHT, gfx_panel_color(0x30, 0x30, 0x30));
    scene_fill_abs(canvas, band_y, 0, BAR_TOP, filled, BAR_HEIGHT, fg);

    scene_text_centered(canvas, band_y, BSP_DISPLAY_WIDTH, BAR_TOP + BAR_HEIGHT + 24, "PWM via LEDC", 2, fg);
}

const demo_scene_t scene_backlight = {
    .name = "backlight",
    .description = "背光调光：5% 到 100% 往复",
    .animated = true,
    .interval_ms = 60,
    .enter = enter,
    .exit = leave,
    .render_band = render_band,
    .frame = frame,
};
