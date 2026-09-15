/*
 * 场景：状态汇总。
 *
 * 显示电池电压与 MicroSD 挂载情况。两者均可缺省：缺省时显示为不可用，
 * 不影响本场景其余内容。
 *
 * 该场景仅在 CONFIG_BSP_ENABLE_BATTERY 或 CONFIG_BSP_ENABLE_SDCARD 打开时编译。
 */

#include <stdio.h>

#include "bsp_display.h"
#include "gfx.h"
#include "scenes.h"

#if CONFIG_BSP_ENABLE_BATTERY
#include "bsp_battery.h"
#endif

#if CONFIG_BSP_ENABLE_SDCARD
#include "bsp_sdcard.h"
#endif

/* 连续采样次数上限，用于在图上留下近期波动。 */
#define HISTORY_LEN 60
#define HISTORY_TOP 260
#define HISTORY_HEIGHT 80

static int s_history[HISTORY_LEN];
static size_t s_history_count;
static size_t s_history_head;

static int s_battery_mv;

static void frame(void)
{
#if CONFIG_BSP_ENABLE_BATTERY
    s_battery_mv = bsp_battery_read_mv_or_zero();
    s_history[s_history_head] = s_battery_mv;
    s_history_head = (s_history_head + 1) % HISTORY_LEN;
    if (s_history_count < HISTORY_LEN) {
        s_history_count++;
    }
#endif
}

static void enter(void)
{
    s_history_count = 0;
    s_history_head = 0;
    s_battery_mv = 0;
    frame();
}

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    const uint16_t bg = gfx_panel_color(0x10, 0x10, 0x14);
    const uint16_t fg = gfx_panel_color(0xFF, 0xFF, 0xFF);
    const uint16_t accent = gfx_panel_color(0x40, 0xC0, 0xFF);
    const uint16_t dim = gfx_panel_color(0x80, 0x80, 0x90);

    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT, bg);

    scene_text_abs(canvas, band_y, 8, 16, "STATUS", 3, fg);

    char line[64];

#if CONFIG_BSP_ENABLE_BATTERY
    scene_text_abs(canvas, band_y, 8, 80, "battery  ADC1_CH7 / IO8  2:1 divider", 1, dim);
    scene_text_abs(canvas, band_y, 8, 96, "USB power, no cell: reads system rail", 1, dim);
    if (s_battery_mv > 0) {
        snprintf(line, sizeof(line), "%d.%02d V", s_battery_mv / 1000, (s_battery_mv % 1000) / 10);
        scene_text_abs(canvas, band_y, 8, 104, line, 3, accent);
    } else {
        scene_text_abs(canvas, band_y, 8, 104, "unavailable", 3, dim);
    }

    /* 波动曲线。纵轴固定覆盖 3.0 V 至 4.4 V。 */
    scene_fill_abs(canvas, band_y, 8, HISTORY_TOP, 304, 1, dim);
    scene_fill_abs(canvas, band_y, 8, HISTORY_TOP + HISTORY_HEIGHT, 304, 1, dim);
    for (size_t i = 0; i < s_history_count; i++) {
        const size_t index = (s_history_head + HISTORY_LEN - s_history_count + i) % HISTORY_LEN;
        int mv = s_history[index];
        if (mv < 3000) {
            mv = 3000;
        } else if (mv > 4400) {
            mv = 4400;
        }
        const int y = HISTORY_TOP + HISTORY_HEIGHT - ((mv - 3000) * HISTORY_HEIGHT) / 1400;
        scene_fill_abs(canvas, band_y, 8 + (int)i * (304 / HISTORY_LEN), y, 4, 2, accent);
    }
#else
    scene_text_abs(canvas, band_y, 8, 80, "battery  disabled in menuconfig", 1, dim);
#endif

#if CONFIG_BSP_ENABLE_SDCARD
    scene_text_abs(canvas, band_y, 8, 380, "microSD  SDIO 4-bit  CLK IO5  CMD IO4", 1, dim);
    bsp_sdcard_describe(line, sizeof(line));
    if (bsp_sdcard_is_mounted()) {
        scene_text_abs(canvas, band_y, 8, 400, line, 2, accent);
    } else {
        scene_text_abs(canvas, band_y, 8, 400, "not mounted", 2, dim);
        scene_text_abs(canvas, band_y, 8, 424, "insert a card, then power cycle", 1, dim);
    }
#else
    scene_text_abs(canvas, band_y, 8, 380, "microSD  disabled in menuconfig", 1, dim);
#endif
}

const demo_scene_t scene_status = {
    .name = "status",
    .description = "状态：电池电压与 MicroSD 挂载情况",
    .animated = true,
    .interval_ms = 200,
    .enter = enter,
    .render_band = render_band,
    .frame = frame,
};
