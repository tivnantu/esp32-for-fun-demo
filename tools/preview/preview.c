/*
 * 宿主侧场景预览与校验。
 *
 * 在宿主机上按与设备相同的横带模型渲染场景，输出一份 24 位 BMP 图，并对
 * 关键像素做数值断言。用途：
 *   1. 无需硬件即可检查场景的视觉正确性
 *   2. 以数值方式验证场景是否完整覆盖每个带（未覆盖处保留哨兵值）
 *
 * 用法：preview <场景名> [输出.bmp]
 * 场景名：selftest | solid | pattern | text | scroll | backlight
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bsp_display.h"
#include "bsp_stub.h"
#include "gfx.h"
#include "scenes.h"

/* 与 scene_text.c 的 MESSAGE 一致。该宏是场景的私有定义，此处按值对齐。 */
#define MESSAGE_FOR_CHECK "Hello World"

#define BAND_LINES 32
#define BAND_COUNT (BSP_DISPLAY_HEIGHT / BAND_LINES)
#define SENTINEL   0xDEAD

static uint16_t s_screen[BSP_DISPLAY_WIDTH * BSP_DISPLAY_HEIGHT];
static uint16_t s_band[BSP_DISPLAY_WIDTH * BAND_LINES];

static int s_failures;

static void render_scene(const demo_scene_t *scene)
{
    for (int i = 0; i < BAND_COUNT; i++) {
        const int y = i * BAND_LINES;

        /* 预填哨兵：场景未覆盖之处会保留哨兵值，可据此发现漏画。 */
        for (int k = 0; k < BSP_DISPLAY_WIDTH * BAND_LINES; k++) {
            s_band[k] = SENTINEL;
        }

        gfx_canvas_t canvas = gfx_canvas_strided(s_band, BSP_DISPLAY_WIDTH, BAND_LINES, BSP_DISPLAY_WIDTH);
        scene->render_band(&canvas, y);

        memcpy(&s_screen[y * BSP_DISPLAY_WIDTH], s_band, sizeof(s_band));
    }
}

static uint16_t pixel(int x, int y)
{
    return s_screen[y * BSP_DISPLAY_WIDTH + x];
}

static int expect(const char *label, int x, int y, uint16_t expected)
{
    const uint16_t actual = pixel(x, y);
    const int ok = actual == expected;
    if (!ok) {
        s_failures++;
    }
    printf("  %s  %-26s (%3d,%3d) 实际 0x%04X  期望 0x%04X\n", ok ? "OK  " : "FAIL", label, x, y, actual, expected);
    return ok;
}

static void check_covered(const char *scene_name)
{
    int uncovered = 0;
    for (int i = 0; i < BSP_DISPLAY_WIDTH * BSP_DISPLAY_HEIGHT; i++) {
        if (s_screen[i] == SENTINEL) {
            uncovered++;
        }
    }
    printf("  %s  %-26s 未覆盖像素 %d\n", uncovered == 0 ? "OK  " : "FAIL", "整屏覆盖", uncovered);
    if (uncovered != 0) {
        s_failures++;
    }
    (void)scene_name;
}

/* 把画面按色相归类，用于无图像环境下的目视核对。 */
static char classify(uint16_t panel_value)
{
    const uint16_t v = gfx_swap16(panel_value);
    const int r = (v >> 11) & 0x1F;
    const int g = (v >> 5) & 0x3F;
    const int b = v & 0x1F;

    const bool rh = r > 20, gh = g > 40, bh = b > 20;
    const bool rl = r < 10, gl = g < 20, bl = b < 10;

    if (rh && gh && bh) {
        return 'W';
    }
    if (rh && gh && bl) {
        return 'Y';
    }
    if (rl && gh && bh) {
        return 'C';
    }
    if (rl && gh && bl) {
        return 'G';
    }
    if (rh && gl && bh) {
        return 'M';
    }
    if (rh && gl && bl) {
        return 'R';
    }
    if (rl && gl && bh) {
        return 'B';
    }
    if (rl && gl && bl) {
        return 'K';
    }
    return '.';
}

/*
 * 字符图：每个字符代表 CELL_W x CELL_H 像素块，取块中心采样。
 * 图例：W 白 Y 黄 C 青 G 绿 M 品红 R 红 B 蓝 K 黑 . 其余
 */
static void print_ascii(void)
{
    const int cell_w = 8;
    const int cell_h = 16;

    printf("字符图（每字符 %dx%d 像素，取中心采样）：\n     ", cell_w, cell_h);
    for (int x = 0; x < BSP_DISPLAY_WIDTH; x += cell_w * 10) {
        printf("%-10d", x);
    }
    printf("\n");

    for (int y = 0; y < BSP_DISPLAY_HEIGHT; y += cell_h) {
        printf("%4d ", y);
        for (int x = 0; x < BSP_DISPLAY_WIDTH; x += cell_w) {
            putchar(classify(pixel(x + cell_w / 2, y + cell_h / 2)));
        }
        putchar('\n');
    }
}

static void put_le16(uint8_t *p, unsigned value)
{
    p[0] = (uint8_t)(value & 0xFF);
    p[1] = (uint8_t)((value >> 8) & 0xFF);
}

static void put_le32(uint8_t *p, unsigned long value)
{
    p[0] = (uint8_t)(value & 0xFF);
    p[1] = (uint8_t)((value >> 8) & 0xFF);
    p[2] = (uint8_t)((value >> 16) & 0xFF);
    p[3] = (uint8_t)((value >> 24) & 0xFF);
}

/* 输出 24 位未压缩 BMP。画布中的值已按面板序交换，此处换回并展开为 888。 */
static void write_bmp(const char *path)
{
    const int w = BSP_DISPLAY_WIDTH;
    const int h = BSP_DISPLAY_HEIGHT;
    const int row_bytes = ((w * 3) + 3) & ~3;
    const int image_size = row_bytes * h;

    uint8_t *row = malloc((size_t)row_bytes);
    FILE *fp = fopen(path, "wb");
    if (row == NULL || fp == NULL) {
        fprintf(stderr, "无法写出 %s\n", path);
        free(row);
        if (fp != NULL) {
            fclose(fp);
        }
        return;
    }

    uint8_t header[54] = {0};
    header[0] = 'B';
    header[1] = 'M';
    put_le32(&header[2], (unsigned long)(54 + image_size));
    put_le32(&header[10], 54);
    put_le32(&header[14], 40);
    put_le32(&header[18], (unsigned long)w);
    put_le32(&header[22], (unsigned long)h);
    put_le16(&header[26], 1);
    put_le16(&header[28], 24);
    put_le32(&header[34], (unsigned long)image_size);
    put_le32(&header[38], 2835);
    put_le32(&header[42], 2835);
    fwrite(header, 1, sizeof(header), fp);

    for (int y = h - 1; y >= 0; y--) {
        memset(row, 0, (size_t)row_bytes);
        for (int x = 0; x < w; x++) {
            const uint16_t v = gfx_swap16(s_screen[y * w + x]);
            const uint8_t r5 = (uint8_t)((v >> 11) & 0x1F);
            const uint8_t g6 = (uint8_t)((v >> 5) & 0x3F);
            const uint8_t b5 = (uint8_t)(v & 0x1F);
            row[x * 3 + 0] = (uint8_t)((b5 << 3) | (b5 >> 2));
            row[x * 3 + 1] = (uint8_t)((g6 << 2) | (g6 >> 4));
            row[x * 3 + 2] = (uint8_t)((r5 << 3) | (r5 >> 2));
        }
        fwrite(row, 1, (size_t)row_bytes, fp);
    }

    fclose(fp);
    free(row);
    printf("已写出 %s（%dx%d，24 位 BMP）\n", path, w, h);
}

static void check_pattern(void)
{
    /* 与 scene_pattern.c 的布局常量一致。 */
    enum { MARGIN = 8, CONTENT_X = MARGIN, CONTENT_W = BSP_DISPLAY_WIDTH - 2 * MARGIN, GAP = 8 };
    enum { BARS_Y = 8, BARS_H = 224 };
    enum { GRAY_Y = BARS_Y + BARS_H + GAP, GRAY_STEPS = 16, GRAY_H = 64 };
    enum { STRIPE_Y = GRAY_Y + GRAY_H + GAP, STRIPE_H = 64, STRIPE_W = 4 };
    enum { CHECK_Y = STRIPE_Y + STRIPE_H + GAP, CHECK_CELL = 8 };

    const uint16_t navy = gfx_panel_color(0x10, 0x18, 0x40);
    const uint16_t white = gfx_panel_color(0xFF, 0xFF, 0xFF);
    const uint16_t black = gfx_panel_color(0x00, 0x00, 0x00);

    static const struct {
        const char *name;
        uint8_t r;
        uint8_t g;
        uint8_t b;
    } bars[] = {
        {"bar0 白", 0xFF, 0xFF, 0xFF}, {"bar1 黄", 0xFF, 0xFF, 0x00}, {"bar2 青", 0x00, 0xFF, 0xFF},
        {"bar3 绿", 0x00, 0xFF, 0x00}, {"bar4 品红", 0xFF, 0x00, 0xFF}, {"bar5 红", 0xFF, 0x00, 0x00},
        {"bar6 蓝", 0x00, 0x00, 0xFF}, {"bar7 黑", 0x00, 0x00, 0x00},
    };

    /* 留白：四边与块间必须为背景色，区块不得触及屏幕边缘。 */
    printf("pattern 留白（背景色应为深藏青）：\n");
    expect("左上角 (0,0)", 0, 0, navy);
    expect("右上角 (319,0)", BSP_DISPLAY_WIDTH - 1, 0, navy);
    expect("左下角 (0,479)", 0, BSP_DISPLAY_HEIGHT - 1, navy);
    expect("右下角 (319,479)", BSP_DISPLAY_WIDTH - 1, BSP_DISPLAY_HEIGHT - 1, navy);
    expect("彩带与灰阶之间", 4, (BARS_Y + BARS_H + GAP / 2), navy);
    expect("灰阶与条纹之间", 4, (GRAY_Y + GRAY_H + GAP / 2), navy);
    expect("条纹与棋盘之间", 4, (STRIPE_Y + STRIPE_H + GAP / 2), navy);

    /* 区块 1：彩带。 */
    printf("pattern 区块 1 彩带（采样行 y=100）：\n");
    {
        const int bar_width = CONTENT_W / 8;
        for (size_t i = 0; i < sizeof(bars) / sizeof(bars[0]); i++) {
            const int x = CONTENT_X + (int)i * bar_width + bar_width / 2;
            expect(bars[i].name, x, 100, gfx_panel_color(bars[i].r, bars[i].g, bars[i].b));
        }
    }

    /* 区块 2：灰阶阶梯。每级内部取值一致，级间跃变。 */
    printf("pattern 区块 2 灰阶阶梯（采样行 y=272）：\n");
    {
        const int step_width = CONTENT_W / GRAY_STEPS;
        int errors = 0;
        for (int i = 0; i < GRAY_STEPS; i++) {
            const uint8_t v = (uint8_t)(i * 255 / (GRAY_STEPS - 1));
            const int x = CONTENT_X + i * step_width + step_width / 2;
            if (pixel(x, 272) != gfx_panel_color(v, v, v)) {
                errors++;
            }
        }
        printf("  %s  %d 级灰阶\n", errors == 0 ? "OK  " : "FAIL", GRAY_STEPS);
        if (errors != 0) {
            s_failures++;
        }
    }

    /* 区块 3：对齐条纹。奇数族与偶数族必须严格交替。 */
    printf("pattern 区块 3 对齐条纹（采样行 y=344）：\n");
    {
        int errors = 0;
        for (int x = 0; x < CONTENT_W; x += STRIPE_W) {
            const uint16_t want = ((x / STRIPE_W) % 2) == 0 ? white : black;
            if (pixel(CONTENT_X + x, 344) != want || pixel(CONTENT_X + x + STRIPE_W - 1, 344) != want) {
                errors++;
            }
        }
        printf("  %s  %d 族条纹两端取值一致\n", errors == 0 ? "OK  " : "FAIL", CONTENT_W / STRIPE_W);
        if (errors != 0) {
            s_failures++;
        }
    }

    /* 区块 4：棋盘。相邻格必须相反。 */
    printf("pattern 区块 4 棋盘（采样行 y=384 与 y=392）：\n");
    {
        int errors = 0;
        for (int x = 0; x < CONTENT_W; x += CHECK_CELL) {
            const bool row0_on = ((x / CHECK_CELL) + 0) % 2 == 0;
            const bool row1_on = ((x / CHECK_CELL) + 1) % 2 == 0;
            if (pixel(CONTENT_X + x + 1, CHECK_Y + 1) != (row0_on ? white : black)) {
                errors++;
            }
            if (pixel(CONTENT_X + x + 1, CHECK_Y + CHECK_CELL + 1) != (row1_on ? white : black)) {
                errors++;
            }
        }
        printf("  %s  %d 格，相邻行反相\n", errors == 0 ? "OK  " : "FAIL", CONTENT_W / CHECK_CELL);
        if (errors != 0) {
            s_failures++;
        }
    }
}

/* ---------- 场景不变量 ---------- */

/*
 * edge：逐条验证硬边存在且交替相位正确。
 *
 * 这个检查针对一个真实的失效模式：kEdges 若未按 y 升序，相邻间隔会变成
 * 负高度而被跳过，交替相位随之错开，但整屏仍被涂满——只做覆盖校验发现不了。
 */
static void check_edge(void)
{
    enum { MARKER_WIDTH = 44 };
    static const int edges[] = {32, 96, 100, 160, 200, 300};
    const size_t count = sizeof(edges) / sizeof(edges[0]);
    const uint16_t white = gfx_panel_color(0xFF, 0xFF, 0xFF);
    const uint16_t black = gfx_panel_color(0x00, 0x00, 0x00);
    const uint16_t marker = gfx_panel_color(0xFF, 0x00, 0x00);

    /*
     * 探针列取在短标与文字标签之外：短标占 x<44，标签自 x=48 起、
     * 每个字符 8 像素、最多 9 字符，故 x≥128 必然干净。
     */
    const int probe_x = 200;

    printf("edge 硬边（探针列 x=%d）：\n", probe_x);
    int missing = 0;
    for (size_t i = 0; i < count; i++) {
        if (pixel(probe_x, edges[i] - 1) == pixel(probe_x, edges[i])) {
            printf("  FAIL  y=%d 上下同色，硬边不存在\n", edges[i]);
            missing++;
        }
    }
    printf("  %s  %zu 条硬边均存在\n", missing == 0 ? "OK  " : "FAIL", count);
    if (missing != 0) {
        s_failures++;
    }

    /* 交替相位：首段为黑，每越过一条边翻转一次。 */
    printf("edge 交替相位：\n");
    int phase = 0;
    int top = 0;
    bool bright = false;
    for (size_t i = 0; i <= count; i++) {
        const int bottom = (i < count) ? edges[i] : BSP_DISPLAY_HEIGHT;
        if (bottom > top) {
            const uint16_t want = bright ? white : black;
            if (pixel(probe_x, top + (bottom - top) / 2) != want) {
                phase++;
            }
            top = bottom;
        }
        bright = !bright;
    }
    printf("  %s  %zu 段相位与交替顺序一致\n", phase == 0 ? "OK  " : "FAIL", count + 1);
    if (phase != 0) {
        s_failures++;
    }

    /* 每条边应有红色短标，且在标注位置有文字。 */
    int markers = 0;
    for (size_t i = 0; i < count; i++) {
        if (pixel(MARKER_WIDTH / 2, edges[i] - 1) != marker) {
            markers++;
        }
    }
    printf("  %s  %zu 条红色短标\n", markers == 0 ? "OK  " : "FAIL", count);
    if (markers != 0) {
        s_failures++;
    }
}

/* orient：四角色块与两条坐标轴的方向。 */
static void check_orient(void)
{
    enum { CORNER = 48 };
    const uint16_t white = gfx_panel_color(0xFF, 0xFF, 0xFF);
    const uint16_t red = gfx_panel_color(0xFF, 0x00, 0x00);
    const uint16_t green = gfx_panel_color(0x00, 0xFF, 0x00);

    const int cx = BSP_DISPLAY_WIDTH / 2;
    const int cy = BSP_DISPLAY_HEIGHT / 2;
    /* 采样点取在角块内部、但避开角标字母的绘制范围。 */
    const int in = CORNER - 8;

    printf("orient 角块（避开角标字母）：\n");
    expect("左上 A 白", in, in, white);
    expect("右上 B 黄", BSP_DISPLAY_WIDTH - 1 - in, in, gfx_panel_color(0xFF, 0xFF, 0x00));
    expect("左下 C 青", in, BSP_DISPLAY_HEIGHT - 1 - in, gfx_panel_color(0x00, 0xFF, 0xFF));
    expect("右下 D 蓝", BSP_DISPLAY_WIDTH - 1 - in, BSP_DISPLAY_HEIGHT - 1 - in, gfx_panel_color(0x00, 0x00, 0xFF));

    printf("orient 坐标轴：\n");
    expect("X 轴在 y=240 为红", 64, cy, red);
    expect("Y 轴在 x=160 为绿", cx, 64, green);

    /*
     * 箭头沿正方向逐行收窄，因此必须在收窄段上采样，并同时验证其外侧
     * 不再是箭头色。只采轴线所在行列无法区分"有箭头"与"只有轴线"：
     * 两条轴都贯穿全屏，其行列上的任何点都是轴色。
     *
     * 偏移 8 像素处箭头长度为 16：X 箭头覆盖 x 292..307（行 cy-8），
     * Y 箭头覆盖 y 452..467（列 cx-8）。采样点同时避开角块与文字标注。
     */
    const uint16_t navy = gfx_panel_color(0x10, 0x18, 0x40);
    expect("X 箭头收窄段内", 294, cy - 8, red);
    expect("X 箭头收窄段外", 310, cy - 8, navy);
    expect("Y 箭头收窄段内", cx - 8, 456, green);
    expect("Y 箭头收窄段外", cx - 8, 470, navy);
}

/* text：居中块的列范围必须与 4 像素对齐的计算一致。 */
static void check_text(void)
{
    const uint16_t bg = gfx_panel_color(0x10, 0x18, 0x40);
    const uint16_t fg = gfx_panel_color(0xFF, 0xFF, 0xFF);

    /* 与 scene_text.c 的布局一致：居中行 y=168，scale 3。 */
    enum { CENTER_Y = 168, SCALE = 3 };
    const int width = gfx_text_width(MESSAGE_FOR_CHECK, SCALE);
    const int x0 = (BSP_DISPLAY_WIDTH - width) / 2;
    const int x1 = x0 + width;

    printf("text 居中（行 %d，scale %d）：\n", CENTER_Y, SCALE);

    int outside = 0;
    for (int x = 0; x < x0; x++) {
        if (pixel(x, CENTER_Y + gfx_glyph_size(SCALE) / 2) != bg) {
            outside++;
        }
    }
    printf("  %s  居中块左侧 %d 列无墨迹\n", outside == 0 ? "OK  " : "FAIL", x0);
    if (outside != 0) {
        s_failures++;
    }

    expect("居中块首列有墨迹", x0, CENTER_Y + 1, fg);

    const int align_ok = (x0 % 4 == 0) && (x1 % 4 == 0);
    printf("  %s  列范围 %d..%d 均为 4 的倍数\n", align_ok ? "OK  " : "FAIL", x0, x1);
    if (!align_ok) {
        s_failures++;
    }
}

/*
 * 帧推进：同一场景在 0 帧与若干帧之后的画面必须不同。
 *
 * 这条检查覆盖 frame() 状态机：若状态不推进、或推进后不反映到画面，
 * 动画场景就是死的。
 */
static uint64_t render_fingerprint(const demo_scene_t *scene, int frames)
{
    if (scene->enter != NULL) {
        scene->enter();
    }
    for (int i = 0; i < frames; i++) {
        if (scene->frame != NULL) {
            scene->frame();
        }
    }
    render_scene(scene);

    uint64_t hash = 1469598103934665603ULL;
    for (int i = 0; i < BSP_DISPLAY_WIDTH * BSP_DISPLAY_HEIGHT; i++) {
        hash ^= s_screen[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static void check_frame_advances(const demo_scene_t *scene)
{
    /*
     * 帧数取 1 与 13。不能用 24 这类可能与状态机周期成整数倍的帧数：
     * 纯色场景的颜色数若为 8，推进 24 帧恰好回到原值，画面相同。
     * 1 与 13 对 5、8、120、200 这些常见周期都不同余于 0。
     */
    static const int probes[] = {1, 13};
    const uint64_t at_zero = render_fingerprint(scene, 0);

    for (size_t i = 0; i < sizeof(probes) / sizeof(probes[0]); i++) {
        const int n = probes[i];
        const uint64_t at_n = render_fingerprint(scene, n);
        const int ok = at_zero != at_n;
        printf("  %s  %-26s 0 帧与 %d 帧画面%s\n", ok ? "OK  " : "FAIL", "帧推进", n, ok ? "不同" : "相同");
        if (!ok) {
            s_failures++;
        }
    }
}

/* exit：离开场景时必须还原它改动的硬件状态。 */
static void check_exit_restores(const demo_scene_t *scene)
{
    if (scene->exit == NULL) {
        printf("  --    退出还原                      该场景无 exit 钩子\n");
        return;
    }
    stub_backlight_reset();
    scene->exit();

    const int last = stub_backlight_last_percent();
    const int ok = (last == 100);
    printf("  %s  %-26s 退出后背光 %d%%\n", ok ? "OK  " : "FAIL", "退出还原背光", last);
    if (!ok) {
        s_failures++;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        fprintf(stderr,
                "用法：preview <场景名> [输出.bmp] [帧数]\n"
                "  场景名：orient | selftest | solid | pattern | edge | text | scroll | backlight\n"
                "  帧数  ：先推进多少次 frame() 再渲染，用于检查动画状态机，缺省 0\n");
        return 2;
    }

    const char *name = argv[1];
    const demo_scene_t *scene = NULL;

    if (strcmp(name, "orient") == 0) {
        scene = &scene_orient;
    } else if (strcmp(name, "selftest") == 0) {
        scene = &scene_selftest;
    } else if (strcmp(name, "solid") == 0) {
        scene = &scene_solid;
    } else if (strcmp(name, "pattern") == 0) {
        scene = &scene_pattern;
    } else if (strcmp(name, "edge") == 0) {
        scene = &scene_edge;
    } else if (strcmp(name, "text") == 0) {
        scene = &scene_text;
    } else if (strcmp(name, "scroll") == 0) {
        scene = &scene_scroll;
    } else if (strcmp(name, "backlight") == 0) {
        scene = &scene_backlight;
    } else {
        fprintf(stderr, "未知场景：%s\n", name);
        return 2;
    }

    const int frames = (argc >= 4) ? atoi(argv[3]) : 0;

    if (scene->enter != NULL) {
        scene->enter();
    }
    for (int i = 0; i < frames; i++) {
        if (scene->frame != NULL) {
            scene->frame();
        }
    }
    memset(s_screen, 0, sizeof(s_screen));
    render_scene(scene);

    printf("== %s（帧数 %d）==\n", name, frames);

    /* 通用：整屏必须被场景完整覆盖。 */
    check_covered(name);

    if (scene->animated) {
        check_frame_advances(scene);
        /* 上面的调用会推进状态，重新渲染出用于输出的画面。 */
        if (scene->enter != NULL) {
            scene->enter();
        }
        for (int i = 0; i < frames; i++) {
            if (scene->frame != NULL) {
                scene->frame();
            }
        }
        render_scene(scene);
    }

    if (strcmp(name, "pattern") == 0) {
        check_pattern();
    } else if (strcmp(name, "edge") == 0) {
        check_edge();
    } else if (strcmp(name, "orient") == 0) {
        check_orient();
    } else if (strcmp(name, "text") == 0) {
        check_text();
    }

    /* 离开场景必须还原硬件状态。 */
    check_exit_restores(scene);
    if (scene->enter != NULL) {
        scene->enter();
    }
    for (int i = 0; i < frames; i++) {
        if (scene->frame != NULL) {
            scene->frame();
        }
    }
    render_scene(scene);

    print_ascii();

    if (argc >= 3) {
        write_bmp(argv[2]);
    }

    printf("%s：%d 项失败\n", name, s_failures);
    return s_failures == 0 ? 0 : 1;
}
