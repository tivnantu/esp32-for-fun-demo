/*
 * 场景：音频输出。
 *
 * 依次播放 C 大调音阶并循环，验证 I2S → ES8311 → 功放 → 喇叭整条通路。
 *
 * 播放为阻塞式：本场景每帧占用约 300 ms 写 I2S。画面内容静态，
 * 因此不额外引入任务与同步；退出场景即停止输出。
 *
 * 该场景仅在 CONFIG_BSP_ENABLE_AUDIO 打开时编译。
 */

#include <stdio.h>

#include "bsp_audio.h"
#include "bsp_display.h"
#include "gfx.h"
#include "scenes.h"

#define TONE_DURATION_MS 300

static const struct {
    const char *name;
    uint32_t freq_hz;
} kNotes[] = {
    {"C4", 262}, {"D4", 294}, {"E4", 330}, {"F4", 349},
    {"G4", 392}, {"A4", 440}, {"B4", 494}, {"C5", 523},
};
#define NOTE_COUNT (sizeof(kNotes) / sizeof(kNotes[0]))

static size_t s_note;
static bool s_failed;

static void enter(void)
{
    s_note = 0;
    s_failed = false;
    if (bsp_audio_init() != ESP_OK) {
        s_failed = true;
        return;
    }
    bsp_audio_set_mute(false);
}

static void leave(void)
{
    /* 静音，避免离开后仍在发声。 */
    bsp_audio_set_mute(true);
}

static void frame(void)
{
    if (!bsp_audio_is_ready()) {
        s_failed = true;
        return;
    }
    if (bsp_audio_play_tone(kNotes[s_note].freq_hz, TONE_DURATION_MS) != ESP_OK) {
        s_failed = true;
        return;
    }
    s_note = (s_note + 1) % NOTE_COUNT;
}

static void render_band(gfx_canvas_t *canvas, int band_y)
{
    const uint16_t bg = gfx_panel_color(0x0A, 0x14, 0x0A);
    const uint16_t fg = gfx_panel_color(0xFF, 0xFF, 0xFF);
    const uint16_t accent = gfx_panel_color(0x40, 0xE0, 0x60);
    const uint16_t dim = gfx_panel_color(0x70, 0x90, 0x70);

    scene_fill_abs(canvas, band_y, 0, 0, BSP_DISPLAY_WIDTH, BSP_DISPLAY_HEIGHT, bg);

    scene_text_abs(canvas, band_y, 8, 16, "AUDIO", 3, fg);
    scene_text_abs(canvas, band_y, 8, 56, "I2S -> ES8311 -> FM8002E", 1, dim);

    char line[48];
    if (s_failed) {
        scene_text_abs(canvas, band_y, 8, 130, "codec unavailable", 2, dim);
        scene_text_abs(canvas, band_y, 8, 170, "check log / wiring", 1, dim);
    } else {
        snprintf(line, sizeof(line), "%s  %u Hz", kNotes[s_note].name, (unsigned)kNotes[s_note].freq_hz);
        scene_text_abs(canvas, band_y, 8, 130, line, 3, accent);
        scene_text_abs(canvas, band_y, 8, 180, "C major scale, looping", 1, dim);
    }

    /* 音量条：与 bsp_audio_init 中设定的 70% 对应。 */
    scene_text_abs(canvas, band_y, 8, 240, "volume", 1, dim);
    scene_fill_abs(canvas, band_y, 8, 256, 304, 12, gfx_panel_color(0x20, 0x30, 0x20));
    scene_fill_abs(canvas, band_y, 8, 256, 304 * 70 / 100, 12, accent);

    scene_text_abs(canvas, band_y, 8, BSP_DISPLAY_HEIGHT - 24, "listen to the speaker", 1, dim);
}

const demo_scene_t scene_audio = {
    .name = "audio",
    .description = "音频：播放 C 大调音阶，验证喇叭通路",
    .animated = true,
    .interval_ms = 400,
    .enter = enter,
    .exit = leave,
    .render_band = render_band,
    .frame = frame,
};
