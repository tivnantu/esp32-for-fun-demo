/*
 * gfx 宿主侧单元测试。
 *
 * 不依赖 ESP-IDF、不依赖目标硬件，在本机编译执行。
 * 运行方式：scripts/host-tests.sh
 */

#include <stdio.h>
#include <string.h>

#include "gfx.h"

static int g_checks;
static int g_failures;

#define CHECK(cond)                                                     \
    do {                                                                \
        g_checks++;                                                     \
        if (!(cond)) {                                                  \
            g_failures++;                                               \
            printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);      \
        }                                                               \
    } while (0)

#define CHECK_EQ(actual, expected)                                                              \
    do {                                                                                        \
        g_checks++;                                                                             \
        unsigned long a_ = (unsigned long)(actual);                                             \
        unsigned long e_ = (unsigned long)(expected);                                           \
        if (a_ != e_) {                                                                         \
            g_failures++;                                                                       \
            printf("FAIL %s:%d  %s == 0x%lX，期望 0x%lX\n", __FILE__, __LINE__, #actual, a_, e_); \
        }                                                                                       \
    } while (0)

/* ---- 色彩 ---- */

static void test_color_conversion(void)
{
    CHECK_EQ(gfx_rgb565(0x00, 0x00, 0x00), 0x0000);
    CHECK_EQ(gfx_rgb565(0xFF, 0xFF, 0xFF), 0xFFFF);
    CHECK_EQ(gfx_rgb565(0xFF, 0x00, 0x00), 0xF800);
    CHECK_EQ(gfx_rgb565(0x00, 0xFF, 0x00), 0x07E0);
    CHECK_EQ(gfx_rgb565(0x00, 0x00, 0xFF), 0x001F);

    /* 规格书 03-display-st77922.md 第 6.2 节所用示例值。 */
    CHECK_EQ(gfx_rgb565(0x10, 0x18, 0x40), 0x10C8);
}

static void test_byte_swap(void)
{
    CHECK_EQ(gfx_swap16(0x0000), 0x0000);
    CHECK_EQ(gfx_swap16(0xFFFF), 0xFFFF);
    CHECK_EQ(gfx_swap16(0x10C8), 0xC810);
    CHECK_EQ(gfx_swap16(gfx_swap16(0x1234)), 0x1234);
}

/*
 * 核心断言：面板按高字节先接收 RGB565，而小端主机按内存序输出低字节先，
 * 因此写入画布的值必须是字节交换后的结果。
 *
 * 判据来自规格书 03-display-st77922.md 第 6.2 节：
 *   深蓝 0x10C8 未交换时被面板解析为 0xC810，呈现品红；
 *   交换后正确呈现深蓝。
 */
static void test_panel_color_order(void)
{
    CHECK_EQ(gfx_panel_color(0x10, 0x18, 0x40), 0xC810);
    CHECK_EQ(gfx_panel_color(0x00, 0x00, 0x00), 0x0000);
    CHECK_EQ(gfx_panel_color(0xFF, 0xFF, 0xFF), 0xFFFF);

    /* 白色与对称灰阶在交换前后一致，故该缺陷只表现为彩色偏移。 */
    CHECK_EQ(gfx_panel_color(0xFF, 0xFF, 0xFF), gfx_rgb565(0xFF, 0xFF, 0xFF));

    /* 交换确实发生了，否则上面的断言不可能同时成立。 */
    CHECK(gfx_panel_color(0x10, 0x18, 0x40) != gfx_rgb565(0x10, 0x18, 0x40));
}

/* ---- 4 像素对齐 ---- */

static void test_align4(void)
{
    CHECK(gfx_is_align4(0));
    CHECK(gfx_is_align4(4));
    CHECK(!gfx_is_align4(3));

    CHECK_EQ(gfx_align4_floor(28), 28);
    CHECK_EQ(gfx_align4_floor(29), 28);
    CHECK_EQ(gfx_align4_floor(31), 28);
    CHECK_EQ(gfx_align4_ceil(29), 32);
    CHECK_EQ(gfx_align4_ceil(28), 28);

    /* 负数向下取整。 */
    CHECK_EQ(gfx_align4_floor(-1), -4);
    CHECK_EQ(gfx_align4_ceil(-1), 0);
}

/* ---- 矩形裁剪 ---- */

static void test_clip(void)
{
    static uint16_t pixels[16 * 8];
    gfx_canvas_t canvas = gfx_canvas_tight(pixels, 16, 8);

    int x = 4, y = 2, w = 4, h = 4;
    CHECK(gfx_clip_rect(&canvas, &x, &y, &w, &h));
    CHECK_EQ(x, 4);
    CHECK_EQ(y, 2);
    CHECK_EQ(w, 4);
    CHECK_EQ(h, 4);

    x = -3; y = -3; w = 5; h = 5;
    CHECK(gfx_clip_rect(&canvas, &x, &y, &w, &h));
    CHECK_EQ(x, 0);
    CHECK_EQ(y, 0);
    CHECK_EQ(w, 2);
    CHECK_EQ(h, 2);

    x = 100; y = 100; w = 4; h = 4;
    CHECK(!gfx_clip_rect(&canvas, &x, &y, &w, &h));

    x = 0; y = 0; w = 0; h = 4;
    CHECK(!gfx_clip_rect(&canvas, &x, &y, &w, &h));
}

/* ---- 绘制 ---- */

static void test_fill(void)
{
    static uint16_t pixels[16 * 8];
    gfx_canvas_t canvas = gfx_canvas_tight(pixels, 16, 8);

    gfx_fill(&canvas, 0xAAAA);
    for (int i = 0; i < 16 * 8; i++) {
        CHECK_EQ(pixels[i], 0xAAAA);
    }

    gfx_fill_rect(&canvas, 2, 2, 3, 2, 0xBBBB);
    CHECK_EQ(pixels[2 * 16 + 2], 0xBBBB);
    CHECK_EQ(pixels[3 * 16 + 4], 0xBBBB);
    /* 矩形外未被改写 */
    CHECK_EQ(pixels[2 * 16 + 1], 0xAAAA);
    CHECK_EQ(pixels[4 * 16 + 2], 0xAAAA);
    CHECK_EQ(pixels[2 * 16 + 5], 0xAAAA);

    gfx_hline(&canvas, 0, 7, 16, 0xCCCC);
    CHECK_EQ(pixels[7 * 16 + 15], 0xCCCC);

    gfx_vline(&canvas, 0, 0, 8, 0xDDDD);
    CHECK_EQ(pixels[0], 0xDDDD);
    CHECK_EQ(pixels[7 * 16], 0xDDDD);
}

static void test_text_metrics(void)
{
    CHECK_EQ(gfx_glyph_size(1), 8);
    CHECK_EQ(gfx_glyph_size(3), 24);
    CHECK_EQ(gfx_text_height(2), 16);
    CHECK_EQ(gfx_text_width("Hello World", 3), 11 * 24);
    CHECK_EQ(gfx_text_width("", 1), 0);
    CHECK_EQ(gfx_text_width(NULL, 1), 0);
    CHECK_EQ(gfx_glyph_size(0), 0);
}

/*
 * 字形渲染契约。
 *
 * 断言针对渲染约定本身，而不是整张字形表：位 0 为最左列、字节 0 为顶行、
 * 每行只取低 7 位。位序或行序若被反转，下列断言必然失败。
 *
 * 取值来自字形表中 'L'（U+004C）的位图：
 *   { 0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00 }
 * 以及空格（U+0020）的全零位图。
 */
static void test_glyph_bit_order(void)
{
    static uint16_t pixels[16 * 16];

    struct probe {
        int x;
        int y;
        int lit;
        const char *what;
    };
    const struct probe probes[] = {
        /* 行 6 = 0x7F：低 7 位全置，位 7 为空。 */
        {0, 6, 1, "行 6 位 0 = 最左列"},
        {6, 6, 1, "行 6 位 6"},
        {7, 6, 0, "行 6 位 7 不越读"},
        /* 行 1 = 0x06：只有位 1、2 置。 */
        {0, 1, 0, "行 1 位 0"},
        {1, 1, 1, "行 1 位 1"},
        {2, 1, 1, "行 1 位 2"},
        {3, 1, 0, "行 1 位 3"},
        /* 行 0 有墨、行 7 全空：字节顺序自顶向下。 */
        {0, 0, 1, "行 0 位 0"},
        {0, 7, 0, "行 7 为空"},
    };

    for (size_t i = 0; i < sizeof(probes) / sizeof(probes[0]); i++) {
        gfx_canvas_t canvas = gfx_canvas_tight(pixels, 16, 16);
        gfx_fill(&canvas, 0x0000);
        gfx_draw_glyph(&canvas, 0, 0, 'L', 1, 0xFFFF);
        CHECK_EQ(pixels[(size_t)probes[i].y * 16 + (size_t)probes[i].x], probes[i].lit ? 0xFFFF : 0x0000);
    }

    /* 空格无墨迹。 */
    {
        gfx_canvas_t canvas = gfx_canvas_tight(pixels, 16, 16);
        gfx_fill(&canvas, 0x0000);
        gfx_draw_glyph(&canvas, 0, 0, ' ', 1, 0xFFFF);
        int lit = 0;
        for (int i = 0; i < 16 * 16; i++) {
            if (pixels[i] != 0x0000) {
                lit++;
            }
        }
        CHECK_EQ(lit, 0);
    }

    /* 放大 N 倍时，同一字形像素占 NxN 个画布像素：行 6 在 y=18，列 0..2 在 x=18..20。 */
    {
        enum { S = 32 };
        static uint16_t big[S * S];
        gfx_canvas_t canvas = gfx_canvas_tight(big, S, S);
        gfx_fill(&canvas, 0x0000);
        gfx_draw_glyph(&canvas, 0, 0, 'L', 3, 0xFFFF);
        CHECK_EQ(big[18 * S + 0], 0xFFFF);
        CHECK_EQ(big[18 * S + 18], 0xFFFF);
        CHECK_EQ(big[18 * S + 21], 0x0000);
    }
}

static void test_text_draws_pixels(void)
{
    static uint16_t pixels[64 * 32];
    gfx_canvas_t canvas = gfx_canvas_tight(pixels, 64, 32);

    gfx_fill(&canvas, 0x0000);
    gfx_draw_text(&canvas, 0, 0, "A", 3, 0xFFFF);

    int lit = 0;
    for (int i = 0; i < 64 * 32; i++) {
        if (pixels[i] == 0xFFFF) {
            lit++;
        }
    }
    CHECK(lit > 0);

    /* 放大 3 倍后每个字形像素占 9 个画布像素，亮点数须为 9 的倍数。 */
    CHECK_EQ(lit % 9, 0);
}

/* 相邻字形不同、且落在各自的步进位：若步进或取模有误，两者会重合。 */
static void test_glyphs_differ(void)
{
    enum { W = 32, H = 8 };
    static uint16_t a[W * H];
    static uint16_t b[W * H];

    gfx_canvas_t ca = gfx_canvas_tight(a, W, H);
    gfx_canvas_t cb = gfx_canvas_tight(b, W, H);
    gfx_fill(&ca, 0x0000);
    gfx_fill(&cb, 0x0000);
    gfx_draw_glyph(&ca, 0, 0, 'A', 1, 0xFFFF);
    gfx_draw_glyph(&cb, 8, 0, 'A', 1, 0xFFFF);

    int same = 1;
    for (int i = 0; i < W * H; i++) {
        if (a[i] != b[i]) {
            same = 0;
            break;
        }
    }
    /* 步进 8 像素，同一字形后移一列后不可能与原地完全相同。 */
    CHECK_EQ(same, 0);

    /* 非 ASCII 字节按低 7 位取字形，不得越界。 */
    gfx_canvas_t cc = gfx_canvas_tight(a, W, H);
    gfx_fill(&cc, 0x0000);
    gfx_draw_glyph(&cc, 0, 0, (char)0xE7, 1, 0xFFFF);
    gfx_canvas_t cd = gfx_canvas_tight(b, W, H);
    gfx_fill(&cd, 0x0000);
    gfx_draw_glyph(&cd, 0, 0, (char)0x67, 1, 0xFFFF);
    int equal = 1;
    for (int i = 0; i < W * H; i++) {
        if (a[i] != b[i]) {
            equal = 0;
            break;
        }
    }
    CHECK_EQ(equal, 1);
}

/*
 * 跨行寻址：stride 大于 width 时，第 r 行须落在 r * stride 处，
 * 且行尾的填充区不得被写入。
 */
static void test_strided_canvas(void)
{
    enum { W = 8, H = 3, STRIDE = 12 };
    static uint16_t buffer[STRIDE * H];
    for (int i = 0; i < STRIDE * H; i++) {
        buffer[i] = 0x0000;
    }

    gfx_canvas_t canvas = gfx_canvas_strided(buffer, W, H, STRIDE);
    gfx_fill_rect(&canvas, 0, 0, W, H, 0xBEEF);

    for (int r = 0; r < H; r++) {
        for (int c = 0; c < STRIDE; c++) {
            CHECK_EQ(buffer[r * STRIDE + c], c < W ? 0xBEEF : 0x0000);
        }
    }
}

/* stride < width 违反契约，须拒绝绘制而不是错行写入。 */
static void test_stride_contract(void)
{
    static uint16_t buffer[16];
    for (int i = 0; i < 16; i++) {
        buffer[i] = 0x0000;
    }

    gfx_canvas_t bad = gfx_canvas_strided(buffer, 8, 2, 4);
    gfx_fill(&bad, 0xFFFF);
    gfx_fill_rect(&bad, 0, 0, 8, 2, 0xFFFF);
    gfx_draw_text(&bad, 0, 0, "A", 1, 0xFFFF);

    for (int i = 0; i < 16; i++) {
        CHECK_EQ(buffer[i], 0x0000);
    }
}

/*
 * 内存安全：画布四周布置哨兵像素，任何越界写入都会破坏哨兵。
 * 这是绘制原语最基本的契约。
 */
static void test_no_out_of_bounds_writes(void)
{
    enum { W = 16, H = 8, G = 2, STRIDE = W + 2 * G, ROWS = H + 2 * G };
    static uint16_t buffer[STRIDE * ROWS];
    const uint16_t sentinel = 0xDEAD;

    for (size_t i = 0; i < sizeof(buffer) / sizeof(buffer[0]); i++) {
        buffer[i] = sentinel;
    }

    /* 画布位于缓冲区中，四周各留 G 像素哨兵。 */
    gfx_canvas_t canvas = gfx_canvas_strided(buffer + G * STRIDE + G, W, H, STRIDE);

    gfx_fill_rect(&canvas, -5, -5, 3, 3, 0x1111);
    gfx_fill_rect(&canvas, W - 1, H - 1, 10, 10, 0x2222);
    gfx_fill_rect(&canvas, -100, -100, 1000, 1000, 0x3333);
    gfx_draw_text(&canvas, -6, -6, "Hello World", 3, 0x4444);
    gfx_draw_text(&canvas, W - 2, H - 2, "X", 4, 0x5555);
    gfx_hline(&canvas, -10, 0, 40, 0x6666);
    gfx_vline(&canvas, 0, -10, 40, 0x7777);

    for (int r = 0; r < ROWS; r++) {
        for (int c = 0; c < STRIDE; c++) {
            const bool inside = (r >= G && r < G + H && c >= G && c < G + W);
            if (inside) {
                continue;
            }
            CHECK_EQ(buffer[r * STRIDE + c], sentinel);
        }
    }
}

/* ---- 空指针防御 ---- */

static void test_null_arguments(void)
{
    gfx_canvas_t null_pixels = gfx_canvas_tight(NULL, 4, 4);
    gfx_fill(&null_pixels, 0xFFFF);
    gfx_fill_rect(&null_pixels, 0, 0, 4, 4, 0xFFFF);
    gfx_draw_text(&null_pixels, 0, 0, "x", 1, 0xFFFF);

    static uint16_t pixels[4];
    gfx_canvas_t ok = gfx_canvas_tight(pixels, 2, 2);
    gfx_fill_rect(&ok, 0, 0, 2, 2, 0xFFFF);
    gfx_fill_rect(NULL, 0, 0, 2, 2, 0xFFFF);
    gfx_fill(NULL, 0xFFFF);
    gfx_draw_text(&ok, 0, 0, NULL, 2, 0xFFFF);
    gfx_draw_glyph(&ok, 0, 0, 'A', 0, 0xFFFF);
    gfx_draw_glyph(NULL, 0, 0, 'A', 1, 0xFFFF);

    CHECK(true); /* 以上调用不得崩溃 */
}

int main(void)
{
    test_color_conversion();
    test_byte_swap();
    test_panel_color_order();
    test_align4();
    test_clip();
    test_fill();
    test_text_metrics();
    test_glyph_bit_order();
    test_text_draws_pixels();
    test_glyphs_differ();
    test_strided_canvas();
    test_stride_contract();
    test_no_out_of_bounds_writes();
    test_null_arguments();

    printf("gfx host tests: %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
