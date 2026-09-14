/*
 * 场景：纯色循环。
 *
 * 全屏切换单一颜色，用于目视确认无残影、无反色异常、无花屏。
 * 白色与黑色交替可暴露反色设置错误；三原色可暴露字节序错误。
 */

#include <stddef.h>
#include <stdint.h>

#include "bsp_display.h"
#include "gfx.h"
#include "scenes.h"

static const struct {
    const char *name;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} kColors[] = {
    {"WHITE", 0xFF, 0xFF, 0xFF},
    {"RED", 0xFF, 0x00, 0x00},
    {"GREEN", 0x00, 0xFF, 0x00},
    {"BLUE", 0x00, 0x00, 0xFF},
    {"BLACK", 0x00, 0x00, 0x00},
    {"NAVY", 0x10, 0x18, 0x40},
};

static size_t s_index;

static void frame(void)
{
    s_index = (s_index + 1) % (sizeof(kColors) / sizeof(kColors[0]));
}

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    const uint16_t color = gfx_panel_color(kColors[s_index].r, kColors[s_index].g, kColors[s_index].b);
    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT, color);

    /* 文字用补色，保证在任意底色上可读。 */
    const uint16_t text = gfx_panel_color((uint8_t)~kColors[s_index].r, (uint8_t)~kColors[s_index].g,
                                          (uint8_t)~kColors[s_index].b);
    scene_text_centered(canvas, band_y, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT / 2 - 12, kColors[s_index].name, 3, text);
}

const demo_scene_t scene_solid = {
    .name = "solid",
    .description = "纯色循环：反色与字节序的目视判据",
    .animated = true,
    .interval_ms = 700,
    .enter = NULL,
    .render_band = render_band,
    .frame = frame,
};
