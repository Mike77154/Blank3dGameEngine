#include "bmp_render.h"

void bmp_render_rgba32_to_callback(const bmp_u8 *pixels,
                                   bmp_u32 width,
                                   bmp_u32 height,
                                   bmp_u32 stride,
                                   bmp_put_pixel_fn put_pixel,
                                   void *user_data)
{
    bmp_u32 y, x;

    if (!pixels || !put_pixel) return;

    for (y = 0; y < height; ++y) {
        const bmp_u8 *row = pixels + y * stride;
        for (x = 0; x < width; ++x) {
            const bmp_u8 *p = row + x * 4U;
            put_pixel(user_data, x, y, p[0], p[1], p[2], p[3]);
        }
    }
}
