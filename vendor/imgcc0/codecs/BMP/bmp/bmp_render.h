#ifndef BMP_RENDER_H
#define BMP_RENDER_H

#include "../include/bmp/bmp_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bmp_metadata.h"

/* Callback genérico para pintar un pixel RGBA en tu consola */
typedef void (*bmp_put_pixel_fn)(void *user,
                                 bmp_u32 x,
                                 bmp_u32 y,
                                 bmp_u8 r,
                                 bmp_u8 g,
                                 bmp_u8 b,
                                 bmp_u8 a);

/* Renderiza un buffer RGBA32 ya decodificado a un callback */
BMP_EXPORT void bmp_render_rgba32_to_callback(const bmp_u8 *pixels,
                                   bmp_u32 width,
                                   bmp_u32 height,
                                   bmp_u32 stride,
                                   bmp_put_pixel_fn put_pixel,
                                   void *user_data);

#ifdef __cplusplus
}
#endif

#endif /* BMP_RENDER_H */
