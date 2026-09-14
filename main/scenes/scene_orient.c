/*
 * 场景：方向标定。
 *
 * 用于确定观察者与面板坐标系的关系。四角使用不同的颜色与字母，
 * 即使观察方向旋转 90 或 180 度，也能通过"我看到哪个颜色在哪一角"确定朝向；
 * 两条轴线带箭头，标明面板坐标系的正方向（+X 向右，+Y 向下）。
 *
 * 面板坐标系（原生，竖屏）：
 *   (0,0) 在 A 角，(319,479) 在 D 角
 */

#include "bsp_display.h"
#include "gfx.h"
#include "scenes.h"

#define CORNER 48
#define AXIS   8

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT, gfx_panel_color(0x10, 0x18, 0x40));

    const uint16_t black = gfx_panel_color(0x00, 0x00, 0x00);
    const uint16_t white = gfx_panel_color(0xFF, 0xFF, 0xFF);

    /* 四角色块 */
    scene_fill_abs(canvas, band_y, 0, 0, CORNER, CORNER, white);
    scene_fill_abs(canvas, band_y, BSP_DISPLAY_WIDTH - CORNER, 0, CORNER, CORNER, gfx_panel_color(0xFF, 0xFF, 0x00));
    scene_fill_abs(canvas, band_y, 0, BSP_DISPLAY_HEIGHT - CORNER, CORNER, CORNER, gfx_panel_color(0x00, 0xFF, 0xFF));
    scene_fill_abs(canvas, band_y, BSP_DISPLAY_WIDTH - CORNER, BSP_DISPLAY_HEIGHT - CORNER, CORNER, CORNER,
                   gfx_panel_color(0x00, 0x00, 0xFF));

    /* 角标字母：不依赖朝向即可向观察者指认 */
    scene_text_abs(canvas, band_y, 18, 16, "A", 2, black);
    scene_text_abs(canvas, band_y, BSP_DISPLAY_WIDTH - 30, 16, "B", 2, black);
    scene_text_abs(canvas, band_y, 18, BSP_DISPLAY_HEIGHT - 32, "C", 2, black);
    scene_text_abs(canvas, band_y, BSP_DISPLAY_WIDTH - 30, BSP_DISPLAY_HEIGHT - 32, "D", 2, white);

    /* 坐标轴：X 轴红色指向右，Y 轴绿色指向下 */
    const int cx = BSP_DISPLAY_WIDTH / 2;
    const int cy = BSP_DISPLAY_HEIGHT / 2;
    const uint16_t red = gfx_panel_color(0xFF, 0x00, 0x00);
    const uint16_t green = gfx_panel_color(0x00, 0xFF, 0x00);

    scene_fill_abs(canvas, band_y, 0, cy - AXIS / 2, BSP_DISPLAY_WIDTH, AXIS, red);
    scene_fill_abs(canvas, band_y, cx - AXIS / 2, 0, AXIS, BSP_DISPLAY_HEIGHT, green);

    /* 箭头：沿正方向逐行收窄的三角 */
    const int head = 24;
    for (int d = -head; d <= head; d++) {
        const int len = head - (d < 0 ? -d : d);
        scene_fill_abs(canvas, band_y, BSP_DISPLAY_WIDTH - head - 4, cy + d, len, 1, red);
        scene_fill_abs(canvas, band_y, cx + d, BSP_DISPLAY_HEIGHT - head - 4, 1, len, green);
    }

    scene_text_abs(canvas, band_y, BSP_DISPLAY_WIDTH - 40, cy - AXIS / 2 - 20, "X+", 2, red);
    scene_text_abs(canvas, band_y, cx + AXIS, BSP_DISPLAY_HEIGHT - 40, "Y+", 2, green);
}

const demo_scene_t scene_orient = {
    .name = "orient",
    .description = "方向标定：A 白 / B 黄 / C 青 / D 蓝，红轴为 +X，绿轴为 +Y",
    .animated = false,
    .interval_ms = 0,
    .enter = NULL,
    .render_band = render_band,
    .frame = NULL,
};
