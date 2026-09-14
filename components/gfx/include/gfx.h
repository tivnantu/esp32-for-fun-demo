/*
 * gfx —— 与硬件无关的绘制原语。
 *
 * 设计约束：
 *   1. 纯函数。输入输出均为调用方提供的数据，本组件不分配内存、不接触外设、
 *      不依赖 ESP-IDF。因此可在宿主机上直接编译与测试。
 *   2. 颜色以「面板序」表示（高字节在前），由 gfx_panel_color() 生成。
 *      面板序的必要性见规格书 03-display-st77922.md 第 6 节。
 *   3. 所有矩形操作自行裁剪，越界不写入。
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* 画布视图。
 * pixels 指向左上角像素；stride 为每行像素数，须 >= width。
 * 紧密排列的缓冲区取 stride == width；子矩形视图取宿主缓冲区的行宽
 * （分带缓冲、大帧缓冲的窗口均属后者）。 */
typedef struct {
    uint16_t *pixels;
    int width;
    int height;
    int stride;
} gfx_canvas_t;

/* 构造紧密排列的画布视图（stride == width）。 */
static inline gfx_canvas_t gfx_canvas_tight(uint16_t *pixels, int width, int height)
{
    gfx_canvas_t canvas = { .pixels = pixels, .width = width, .height = height, .stride = width };
    return canvas;
}

/* 构造子矩形画布视图。stride 为宿主缓冲区每行像素数。 */
static inline gfx_canvas_t gfx_canvas_strided(uint16_t *pixels, int width, int height, int stride)
{
    gfx_canvas_t canvas = { .pixels = pixels, .width = width, .height = height, .stride = stride };
    return canvas;
}

/* ---- 色彩 ---- */

/* 由 8 位分量构造 RGB565。结果为主机序，用于校验与计算。 */
uint16_t gfx_rgb565(uint8_t r, uint8_t g, uint8_t b);

/* 交换 16 位值的两个字节。 */
uint16_t gfx_swap16(uint16_t value);

/* 由 8 位分量构造可直接写入画布的面板序颜色。 */
uint16_t gfx_panel_color(uint8_t r, uint8_t g, uint8_t b);

/* ---- 画布操作（颜色参数须为面板序）---- */

void gfx_fill(gfx_canvas_t *canvas, uint16_t color);
void gfx_fill_rect(gfx_canvas_t *canvas, int x, int y, int w, int h, uint16_t color);
void gfx_hline(gfx_canvas_t *canvas, int x, int y, int w, uint16_t color);
void gfx_vline(gfx_canvas_t *canvas, int x, int y, int h, uint16_t color);

/* ---- 字形与文本 ---- */

/* 字形单元边长（像素）。8x8 位图字体，scale 为整数放大倍数。 */
int gfx_glyph_size(int scale);
int gfx_text_width(const char *text, int scale);
int gfx_text_height(int scale);

void gfx_draw_glyph(gfx_canvas_t *canvas, int x, int y, char ch, int scale, uint16_t color);
void gfx_draw_text(gfx_canvas_t *canvas, int x, int y, const char *text, int scale, uint16_t color);

/* ---- 4 像素对齐 ----
 * ST77922 为双栅极驱动，像素数须为 4 的倍数，列地址起止亦须 4 对齐。
 * 见规格书 03-display-st77922.md 第 7 节。 */
bool gfx_is_align4(int value);
int gfx_align4_floor(int value);
int gfx_align4_ceil(int value);

/* 把矩形裁剪到画布内。返回 false 表示完全落在画布外。 */
bool gfx_clip_rect(const gfx_canvas_t *canvas, int *x, int *y, int *w, int *h);
