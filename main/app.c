/*
 * 演示应用。
 *
 * 职责：
 *   - 启动自检与耗时测量（对应规格书 07-open-items.md 第 3 节的验收判据）
 *   - 场景调度：BOOT 按键或串口命令切换
 *   - 横带渲染：为场景提供画布，逐带推送到面板
 *
 * 本层不含显示硬件细节：QSPI 时序、初始化序列、字节序、像素对齐
 * 均由 bsp 与 gfx 组件承担。
 */

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "bsp.h"
#include "gfx.h"
#include "scenes.h"

static const char *TAG = "demo";

/* 横带渲染参数。480 为 32 的整数倍，故不存在残带。 */
#define BAND_LINES 32
#define BAND_COUNT (BSP_DISPLAY_HEIGHT / BAND_LINES)

_Static_assert(BSP_DISPLAY_HEIGHT % BAND_LINES == 0, "面板高度须为带高的整数倍");
_Static_assert(BSP_DISPLAY_WIDTH % 4 == 0, "面板宽度须为 4 的倍数（ST77922 双栅极）");

static uint16_t s_band[BSP_DISPLAY_WIDTH * BAND_LINES];

static const demo_scene_t *const s_scenes[] = {
    &scene_orient,
    &scene_selftest,
    &scene_solid,
    &scene_pattern,
    &scene_edge,
    &scene_text,
    &scene_scroll,
    &scene_backlight,
#if CONFIG_BSP_ENABLE_TOUCH
    &scene_touch,
#endif
#if CONFIG_BSP_ENABLE_RGB
    &scene_rgb,
#endif
#if CONFIG_BSP_ENABLE_AUDIO
    &scene_audio,
#endif
#if CONFIG_BSP_ENABLE_STATUS_SCENE
    &scene_status,
#endif
};
#define SCENE_COUNT (sizeof(s_scenes) / sizeof(s_scenes[0]))

static int s_scene_index;
static const demo_scene_t *s_scene;
static int64_t s_next_frame_us;

static volatile char s_command;

/* ---- 渲染 ---- */

static void render_full(void)
{
    if (s_scene->render_band == NULL) {
        return;
    }

    /* 分解计时：CPU 绘制 与 推送入队（含队列满时的阻塞）分开统计，
     * 否则无法判断瓶颈在算法还是在总线。 */
    int64_t draw_us = 0;
    int64_t flush_us = 0;

    for (int i = 0; i < BAND_COUNT; i++) {
        const int y = i * BAND_LINES;
        gfx_canvas_t canvas = gfx_canvas_strided(s_band, BSP_DISPLAY_WIDTH, BAND_LINES, BSP_DISPLAY_WIDTH);

        const int64_t t0 = esp_timer_get_time();
        s_scene->render_band(&canvas, y);
        const int64_t t1 = esp_timer_get_time();

        ESP_ERROR_CHECK(bsp_display_flush(0, y, BSP_DISPLAY_WIDTH, BAND_LINES, s_band));

        const int64_t t2 = esp_timer_get_time();
        draw_us += t1 - t0;
        flush_us += t2 - t1;
    }

    ESP_LOGI(TAG, "整屏 %d 带：绘制 %lld ms，推送 %lld ms，合计 %lld ms", BAND_COUNT, draw_us / 1000, flush_us / 1000,
             (draw_us + flush_us) / 1000);
}

static void print_scene_list(void)
{
    printf("\n场景列表：\n");
    for (size_t i = 0; i < SCENE_COUNT; i++) {
        printf("  %zu  %-12s %s\n", i + 1, s_scenes[i]->name, s_scenes[i]->description);
    }
}

static void print_help(void)
{
    printf("\n命令：\n");
    printf("  n  下一场景\n");
    printf("  p  上一场景\n");
    printf("  l  列出场景\n");
    printf("  h  显示本帮助\n");
    printf("也可按 BOOT 键切换场景。\n\n");
}

static void switch_to(int index)
{
    /* 先让上一个场景还原它改动的硬件状态，再切换。 */
    if (s_scene != NULL && s_scene->exit != NULL) {
        s_scene->exit();
    }

    const int count = (int)SCENE_COUNT;
    s_scene_index = ((index % count) + count) % count;
    s_scene = s_scenes[s_scene_index];

    ESP_LOGI(TAG, "[%d/%d] %s —— %s", s_scene_index + 1, count, s_scene->name, s_scene->description);

    if (s_scene->enter != NULL) {
        s_scene->enter();
    }

    render_full();

    s_next_frame_us = esp_timer_get_time() + (int64_t)s_scene->interval_ms * 1000;
}

/* ---- 输入 ---- */

static void poll_key(void)
{
    if (bsp_key_take_press()) {
        switch_to(s_scene_index + 1);
    }
}

static void poll_command(void)
{
    const char command = s_command;
    if (command == 0) {
        return;
    }
    s_command = 0;

    switch (command) {
    case 'n':
        switch_to(s_scene_index + 1);
        break;
    case 'p':
        switch_to(s_scene_index - 1);
        break;
    case 'l':
        print_scene_list();
        break;
    default:
        print_help();
        break;
    }
}

static void console_task(void *arg)
{
    (void)arg;
    while (true) {
        const int c = fgetc(stdin);
        if (c == EOF) {
            vTaskDelay(pdMS_TO_TICKS(20));
            continue;
        }
        if (c == 'n' || c == 'p' || c == 'l' || c == 'h') {
            s_command = (char)c;
        }
    }
}

/* ---- 启动 ---- */

void app_main(void)
{
    ESP_LOGI(TAG, "esp32-for-fun-demo 启动");

    /*
     * 屏初始化单独计时并以验收判据的形式报告。
     *
     * 不能整个 bsp_init() 计时后当作屏初始化耗时：bsp_init() 还包含可选
     * 子系统的初始化，其中触摸有两段固定 100 ms 复位延时，MicroSD 在无卡
     * 时也要走一次挂载尝试。在全子系统配置下这些会把总耗时推过 300 ms，
     * 使日志出现一条看似违反判据的记录，而屏本身是达标的。
     * bsp_display_init() 幂等，故此处先单独调用一次。
     */
    const int64_t display_start = esp_timer_get_time();
    ESP_ERROR_CHECK(bsp_display_init());
    const int64_t display_us = esp_timer_get_time() - display_start;
    ESP_LOGI(TAG, "屏初始化耗时 %lld ms（验收判据 ≤300 ms）", display_us / 1000);

    const int64_t init_start = esp_timer_get_time();
    ESP_ERROR_CHECK(bsp_init());
    const int64_t init_us = esp_timer_get_time() - init_start;
    ESP_LOGI(TAG, "其余子系统初始化耗时 %lld ms", (init_us - display_us) / 1000);

    ESP_ERROR_CHECK(bsp_backlight_set(true));

    xTaskCreate(console_task, "console", 3072, NULL, 4, NULL);

    print_scene_list();
    print_help();

    switch_to(0);

    while (true) {
        if (s_scene->animated) {
            const int64_t now = esp_timer_get_time();
            if (now >= s_next_frame_us) {
                if (s_scene->frame != NULL) {
                    s_scene->frame();
                }
                render_full();
                s_next_frame_us = now + (int64_t)s_scene->interval_ms * 1000;
            }
        }

        poll_key();
        poll_command();

        /*
         * 轮询周期。此处必须保证至少延时 1 个 tick：
         * 默认 tick 频率为 100 Hz，pdMS_TO_TICKS(5) 等于 0，会导致本任务
         * 不再让出 CPU、空闲任务饿死并触发任务看门狗。
         * 1 个 tick 在默认频率下为 10 ms，对按键与串口输入足够。
         */
        vTaskDelay(1);
    }
}
