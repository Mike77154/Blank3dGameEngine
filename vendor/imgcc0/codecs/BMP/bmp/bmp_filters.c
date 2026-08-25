#include "bmp_filters.h"

void bmp_filter_flip_vertical_rgba32(bmp_u8 *pixels,
                                     bmp_u32 width,
                                     bmp_u32 height,
                                     bmp_u32 stride)
{
    bmp_u32 y;
    bmp_u32 half = height / 2U;

    for (y = 0; y < half; ++y) {
        bmp_u8 *row_top = pixels + y * stride;
        bmp_u8 *row_bot = pixels + (height - 1U - y) * stride;
        bmp_u32 x;
        for (x = 0; x < width * 4U; ++x) {
            bmp_u8 tmp = row_top[x];
            row_top[x] = row_bot[x];
            row_bot[x] = tmp;
        }
    }
}

void bmp_filter_invert_rgba32(bmp_u8 *pixels,
                              bmp_u32 width,
                              bmp_u32 height,
                              bmp_u32 stride)
{
    bmp_u32 y, x;

    for (y = 0; y < height; ++y) {
        bmp_u8 *row = pixels + y * stride;
        for (x = 0; x < width; ++x) {
            bmp_u8 *p = row + x * 4U;
            p[0] = (bmp_u8)(255U - p[0]); /* R */
            p[1] = (bmp_u8)(255U - p[1]); /* G */
            p[2] = (bmp_u8)(255U - p[2]); /* B */
            /* A se mantiene */
        }
    }
}

void bmp_filter_grayscale_rgba32(bmp_u8 *pixels,
                                 bmp_u32 width,
                                 bmp_u32 height,
                                 bmp_u32 stride)
{
    bmp_u32 y, x;

    for (y = 0; y < height; ++y) {
        bmp_u8 *row = pixels + y * stride;
        for (x = 0; x < width; ++x) {
            bmp_u8 *p = row + x * 4U;
            bmp_u8 r = p[0];
            bmp_u8 g = p[1];
            bmp_u8 b = p[2];
            /* aprox luminancia 0.3R + 0.59G + 0.11B usando enteros */
            bmp_u32 gray = (bmp_u32)r * 30U +
                           (bmp_u32)g * 59U +
                           (bmp_u32)b * 11U;
            gray /= 100U;
            p[0] = p[1] = p[2] = (bmp_u8)gray;
        }
    }
}
