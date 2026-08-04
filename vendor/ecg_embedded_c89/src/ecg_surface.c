#include "ecg_surface.h"

ECG_Color ecg_color_make(ECG_U8 r, ECG_U8 g, ECG_U8 b)
{
    ECG_Color color;

    color.r = r;
    color.g = g;
    color.b = b;
    return color;
}

ECG_Color ecg_color_fade_step(ECG_Color base, ECG_Color gradient, unsigned int step)
{
    ECG_Color out;
    unsigned long sub_r;
    unsigned long sub_g;
    unsigned long sub_b;

    sub_r = ((unsigned long)gradient.r) * (unsigned long)step;
    sub_g = ((unsigned long)gradient.g) * (unsigned long)step;
    sub_b = ((unsigned long)gradient.b) * (unsigned long)step;

    out.r = sub_r >= (unsigned long)base.r ? (ECG_U8)0 : (ECG_U8)((unsigned long)base.r - sub_r);
    out.g = sub_g >= (unsigned long)base.g ? (ECG_U8)0 : (ECG_U8)((unsigned long)base.g - sub_g);
    out.b = sub_b >= (unsigned long)base.b ? (ECG_U8)0 : (ECG_U8)((unsigned long)base.b - sub_b);
    return out;
}

ECG_Color ecg_color_scale(ECG_Color base, unsigned int numerator, unsigned int denominator)
{
    ECG_Color out;

    if (denominator == 0u) {
        out.r = 0u;
        out.g = 0u;
        out.b = 0u;
        return out;
    }

    out.r = (ECG_U8)((((unsigned int)base.r) * numerator) / denominator);
    out.g = (ECG_U8)((((unsigned int)base.g) * numerator) / denominator);
    out.b = (ECG_U8)((((unsigned int)base.b) * numerator) / denominator);
    return out;
}

int ecg_surface_is_valid(const ECG_Surface *surface)
{
    if (surface == (const ECG_Surface *)0) {
        return 0;
    }
    if (surface->pixels == (ECG_Color *)0) {
        return 0;
    }
    if (surface->width == 0u || surface->height == 0u || surface->stride == 0u) {
        return 0;
    }
    if (surface->stride < surface->width) {
        return 0;
    }
    return 1;
}

ECG_Status ecg_surface_clear(ECG_Surface *surface, ECG_Color color)
{
    unsigned int y;
    unsigned int x;
    ECG_Color *row;

    if (!ecg_surface_is_valid(surface)) {
        return ECG_STATUS_NULL;
    }

    for (y = 0u; y < surface->height; y++) {
        row = surface->pixels + ((unsigned long)y * (unsigned long)surface->stride);
        for (x = 0u; x < surface->width; x++) {
            row[x] = color;
        }
    }

    return ECG_STATUS_OK;
}

void ecg_put_pixel(ECG_Surface *surface, int x, int y, ECG_Color color)
{
    ECG_Color *pixel;

    if (!ecg_surface_is_valid(surface)) {
        return;
    }
    if (x < 0 || y < 0) {
        return;
    }
    if ((unsigned int)x >= surface->width || (unsigned int)y >= surface->height) {
        return;
    }

    pixel = surface->pixels + ((unsigned long)(unsigned int)y * (unsigned long)surface->stride) + (unsigned int)x;
    *pixel = color;
}

void ecg_put_pixel_additive(ECG_Surface *surface, int x, int y,
                            ECG_Color color)
{
    ECG_Color *pixel;
    unsigned int r;
    unsigned int g;
    unsigned int b;

    if (!ecg_surface_is_valid(surface)) {
        return;
    }
    if (x < 0 || y < 0) {
        return;
    }
    if ((unsigned int)x >= surface->width ||
        (unsigned int)y >= surface->height) {
        return;
    }

    pixel = surface->pixels +
            ((unsigned long)(unsigned int)y *
             (unsigned long)surface->stride) + (unsigned int)x;

    r = (unsigned int)pixel->r + (unsigned int)color.r;
    g = (unsigned int)pixel->g + (unsigned int)color.g;
    b = (unsigned int)pixel->b + (unsigned int)color.b;
    pixel->r = (ECG_U8)(r > 255u ? 255u : r);
    pixel->g = (ECG_U8)(g > 255u ? 255u : g);
    pixel->b = (ECG_U8)(b > 255u ? 255u : b);
}


void ecg_put_pixel_lighten(ECG_Surface *surface, int x, int y,
                           ECG_Color color)
{
    ECG_Color *pixel;

    if (!ecg_surface_is_valid(surface)) {
        return;
    }
    if (x < 0 || y < 0) {
        return;
    }
    if ((unsigned int)x >= surface->width ||
        (unsigned int)y >= surface->height) {
        return;
    }

    pixel = surface->pixels +
            ((unsigned long)(unsigned int)y *
             (unsigned long)surface->stride) + (unsigned int)x;
    if (color.r > pixel->r) pixel->r = color.r;
    if (color.g > pixel->g) pixel->g = color.g;
    if (color.b > pixel->b) pixel->b = color.b;
}

ECG_Status ecg_fill_rect(ECG_Surface *surface, int x, int y,
                         unsigned int width, unsigned int height,
                         ECG_Color color)
{
    unsigned int yy;
    unsigned int xx;
    int py;
    int px;

    if (!ecg_surface_is_valid(surface)) {
        return ECG_STATUS_NULL;
    }
    if (width == 0u || height == 0u) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    for (yy = 0u; yy < height; yy++) {
        py = y + (int)yy;
        for (xx = 0u; xx < width; xx++) {
            px = x + (int)xx;
            ecg_put_pixel(surface, px, py, color);
        }
    }

    return ECG_STATUS_OK;
}

ECG_Status ecg_draw_hline(ECG_Surface *surface, int x0, int x1, int y,
                          ECG_Color color)
{
    int x;
    int tmp;

    if (!ecg_surface_is_valid(surface)) {
        return ECG_STATUS_NULL;
    }

    if (x0 > x1) {
        tmp = x0;
        x0 = x1;
        x1 = tmp;
    }

    for (x = x0; x <= x1; x++) {
        ecg_put_pixel(surface, x, y, color);
    }

    return ECG_STATUS_OK;
}

ECG_Status ecg_draw_vline(ECG_Surface *surface, int x, int y0, int y1,
                          ECG_Color color)
{
    int y;
    int tmp;

    if (!ecg_surface_is_valid(surface)) {
        return ECG_STATUS_NULL;
    }

    if (y0 > y1) {
        tmp = y0;
        y0 = y1;
        y1 = tmp;
    }

    for (y = y0; y <= y1; y++) {
        ecg_put_pixel(surface, x, y, color);
    }

    return ECG_STATUS_OK;
}

ECG_Status ecg_draw_rect_outline(ECG_Surface *surface, int x, int y,
                                 unsigned int width, unsigned int height,
                                 ECG_Color color)
{
    int right;
    int bottom;

    if (!ecg_surface_is_valid(surface)) {
        return ECG_STATUS_NULL;
    }
    if (width == 0u || height == 0u) {
        return ECG_STATUS_BAD_ARGUMENT;
    }

    right = x + (int)width - 1;
    bottom = y + (int)height - 1;

    (void)ecg_draw_hline(surface, x, right, y, color);
    (void)ecg_draw_hline(surface, x, right, bottom, color);
    (void)ecg_draw_vline(surface, x, y, bottom, color);
    (void)ecg_draw_vline(surface, right, y, bottom, color);

    return ECG_STATUS_OK;
}
