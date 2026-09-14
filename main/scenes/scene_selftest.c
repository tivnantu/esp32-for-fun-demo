/*
 * 场景：自检画面。
 *
 * 目的：一屏之内同时验证反色、色彩、4 像素对齐与文本渲染。
 * 静止画面，便于拍照留档。
 */

#include "bsp_display.h"
#include "gfx.h"
#include "scenes.h"

#define BLOCK_TOP     56
#define BLOCK_HEIGHT  56
#define STRIPE_TOP    (BLOCK_TOP + 5 * BLOCK_HEIGHT + 8)
#define STRIPE_HEIGHT 16
#define INFO_TOP      (STRIPE_TOP + STRIPE_HEIGHT + 8)

static const struct {
    const char *name;
    uint8_t r;
    uint8_t g;
    uint8_t b;
    bool dark_text;
} kBlocks[] = {
    {"WHITE", 0xFF, 0xFF, 0xFF, true},
    {"RED", 0xFF, 0x00, 0x00, false},
    {"GREEN", 0x00, 0xFF, 0x00, true},
    {"BLUE", 0x00, 0x00, 0xFF, false},
    {"BLACK", 0x00, 0x00, 0x00, false},
};

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    const uint16_t bg = gfx_panel_color(0x10, 0x18, 0x40);
    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT, bg);

    /* 标题区 */
    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, 48, gfx_panel_color(0x00, 0x00, 0x00));
    scene_text_centered(canvas, band_y, BSP_DISPLAY_WIDTH, 16, "esp32-for-fun-demo", 2, gfx_panel_color(0xFF, 0xFF, 0xFF));

    /* 色块：验证反色与字节序。深蓝若呈现品红，即为字节序未交换。 */
    for (size_t i = 0; i < sizeof(kBlocks) / sizeof(kBlocks[0]); i++) {
        const int top = BLOCK_TOP + (int)i * BLOCK_HEIGHT;
        const uint16_t color = gfx_panel_color(kBlocks[i].r, kBlocks[i].g, kBlocks[i].b);
        const uint16_t text = kBlocks[i].dark_text ? gfx_panel_color(0x00, 0x00, 0x00)
                                                   : gfx_panel_color(0xFF, 0xFF, 0xFF);
        scene_fill_abs(canvas, band_y, 0, top, BSP_DISPLAY_WIDTH, BLOCK_HEIGHT, color);
        scene_text_abs(canvas, band_y, 8, top + BLOCK_HEIGHT / 2 - 8, kBlocks[i].name, 2, text);
    }

    /* 4 像素条纹：验证列对齐。条纹应边界锐利、间距均匀，无半宽或错位。 */
    if (scene_visible(canvas, band_y, STRIPE_TOP, STRIPE_HEIGHT)) {
        for (int x = 0; x < BSP_DISPLAY_WIDTH; x += 4) {
            const bool on = ((x / 4) % 2) == 0;
            const uint16_t color = on ? gfx_panel_color(0xFF, 0xFF, 0xFF) : gfx_panel_color(0x00, 0x00, 0x00);
            scene_fill_abs(canvas, band_y, x, STRIPE_TOP, 4, STRIPE_HEIGHT, color);
        }
    }

    /* 信息区 */
    scene_text_abs(canvas, band_y, 8, INFO_TOP, "ST77922 QSPI 320x480", 1, gfx_panel_color(0xFF, 0xFF, 0xFF));
    scene_text_abs(canvas, band_y, 8, INFO_TOP + 16, "RST tied to EN", 1, gfx_panel_color(0xFF, 0xFF, 0xFF));
    scene_text_abs(canvas, band_y, 8, INFO_TOP + 32, "SWAP=1 INVERT=1", 1, gfx_panel_color(0xFF, 0xFF, 0xFF));
}

const demo_scene_t scene_selftest = {
    .name = "selftest",
    .description = "自检画面：反色、色彩、4 像素对齐、文本",
    .animated = false,
    .interval_ms = 0,
    .enter = NULL,
    .render_band = render_band,
    .frame = NULL,
};
