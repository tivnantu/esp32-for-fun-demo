#include "gfx.h"

#include <string.h>

#include "font8x8_basic.h"

/* 8x8 位图字体，字形单元为正方形。 */
#define GLYPH_PIXELS 8

uint16_t gfx_rgb565(uint8_t r, uint8_t g, uint8_t b)
{
    return (uint16_t)((((uint16_t)r & 0xF8) << 8) | (((uint16_t)g & 0xFC) << 3) | (((uint16_t)b & 0xF8) >> 3));
}

uint16_t gfx_swap16(uint16_t value)
{
    return (uint16_t)((value >> 8) | (value << 8));
}

uint16_t gfx_panel_color(uint8_t r, uint8_t g, uint8_t b)
{
    return gfx_swap16(gfx_rgb565(r, g, b));
}

bool gfx_clip_rect(const gfx_canvas_t *canvas, int *x, int *y, int *w, int *h)
{
    if (canvas == NULL || canvas->pixels == NULL || x == NULL || y == NULL || w == NULL || h == NULL) {
        return false;
    }
    if (*w <= 0 || *h <= 0) {
        return false;
    }

    int x0 = *x;
    int y0 = *y;
    int x1 = x0 + *w;
    int y1 = y0 + *h;

    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 > canvas->width) {
        x1 = canvas->width;
    }
    if (y1 > canvas->height) {
        y1 = canvas->height;
    }
    if (x0 >= x1 || y0 >= y1) {
        return false;
    }

    *x = x0;
    *y = y0;
    *w = x1 - x0;
    *h = y1 - y0;
    return true;
}

void gfx_fill_rect(gfx_canvas_t *canvas, int x, int y, int w, int h, uint16_t color)
{
    if (canvas == NULL || canvas->pixels == NULL) {
        return;
    }
    /* 契约：stride >= width。不满足时拒绝绘制，避免行错位导致的越界。 */
    if (canvas->stride < canvas->width) {
        return;
    }
    if (!gfx_clip_rect(canvas, &x, &y, &w, &h)) {
        return;
    }

    for (int row = 0; row < h; row++) {
        uint16_t *dst = canvas->pixels + (size_t)(y + row) * (size_t)canvas->stride + (size_t)x;
        for (int col = 0; col < w; col++) {
            dst[col] = color;
        }
    }
}

void gfx_fill(gfx_canvas_t *canvas, uint16_t color)
{
    if (canvas == NULL || canvas->pixels == NULL) {
        return;
    }
    if (canvas->stride < canvas->width) {
        return;
    }
    gfx_fill_rect(canvas, 0, 0, canvas->width, canvas->height, color);
}

void gfx_hline(gfx_canvas_t *canvas, int x, int y, int w, uint16_t color)
{
    gfx_fill_rect(canvas, x, y, w, 1, color);
}

void gfx_vline(gfx_canvas_t *canvas, int x, int y, int h, uint16_t color)
{
    gfx_fill_rect(canvas, x, y, 1, h, color);
}

int gfx_glyph_size(int scale)
{
    return scale < 1 ? 0 : GLYPH_PIXELS * scale;
}

int gfx_text_width(const char *text, int scale)
{
    if (text == NULL) {
        return 0;
    }
    return (int)strlen(text) * gfx_glyph_size(scale);
}

int gfx_text_height(int scale)
{
    return gfx_glyph_size(scale);
}

void gfx_draw_glyph(gfx_canvas_t *canvas, int x, int y, char ch, int scale, uint16_t color)
{
    if (canvas == NULL || canvas->pixels == NULL || scale < 1) {
        return;
    }
    if (canvas->stride < canvas->width) {
        return;
    }

    const int size = GLYPH_PIXELS * scale;
    int bx = x;
    int by = y;
    int bw = size;
    int bh = size;
    if (!gfx_clip_rect(canvas, &bx, &by, &bw, &bh)) {
        return;
    }
    const int right = bx + bw;
    const int bottom = by + bh;

    /* 位 0 为最左列。高位字符无字形，按 7 位取模，与位图字体的定义域一致。 */
    const uint8_t *glyph = font8x8_basic[(uint8_t)ch & 0x7F];

    /* 直接按跨度写入，不经 gfx_fill_rect —— 后者对每个字形像素都会
     * 重复一次裁剪与调用开销，是文本渲染的主要成本。 */
    for (int gy = 0; gy < GLYPH_PIXELS; gy++) {
        const int py0 = y + gy * scale;
        if (py0 >= bottom || (py0 + scale) <= by) {
            continue;
        }
        const int cy0 = py0 > by ? py0 : by;
        const int cy1 = (py0 + scale) < bottom ? (py0 + scale) : bottom;

        const uint8_t row = glyph[gy];
        for (int gx = 0; gx < GLYPH_PIXELS; gx++) {
            if ((row & (1u << gx)) == 0) {
                continue;
            }
            const int px0 = x + gx * scale;
            if (px0 >= right || (px0 + scale) <= bx) {
                continue;
            }
            const int cx0 = px0 > bx ? px0 : bx;
            const int cx1 = (px0 + scale) < right ? (px0 + scale) : right;
            const int span = cx1 - cx0;

            for (int py = cy0; py < cy1; py++) {
                uint16_t *dst = canvas->pixels + (size_t)py * (size_t)canvas->stride + (size_t)cx0;
                for (int i = 0; i < span; i++) {
                    dst[i] = color;
                }
            }
        }
    }
}

void gfx_draw_text(gfx_canvas_t *canvas, int x, int y, const char *text, int scale, uint16_t color)
{
    if (text == NULL) {
        return;
    }

    const int advance = gfx_glyph_size(scale);
    for (int i = 0; text[i] != '\0'; i++) {
        gfx_draw_glyph(canvas, x + i * advance, y, text[i], scale, color);
    }
}

bool gfx_is_align4(int value)
{
    return (value & 3) == 0;
}

int gfx_align4_floor(int value)
{
    return value & ~3;
}

int gfx_align4_ceil(int value)
{
    return (value + 3) & ~3;
}
