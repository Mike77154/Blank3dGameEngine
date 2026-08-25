#ifndef BMP_FILTERS_H
#define BMP_FILTERS_H

#include "../include/bmp/bmp_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bmp_metadata.h"

/* Filtros sobre un buffer RGBA32 ya decodificado */

BMP_EXPORT void bmp_filter_flip_vertical_rgba32(bmp_u8 *pixels,
                                     bmp_u32 width,
                                     bmp_u32 height,
                                     bmp_u32 stride);

BMP_EXPORT void bmp_filter_invert_rgba32(bmp_u8 *pixels,
                              bmp_u32 width,
                              bmp_u32 height,
                              bmp_u32 stride);

BMP_EXPORT void bmp_filter_grayscale_rgba32(bmp_u8 *pixels,
                                 bmp_u32 width,
                                 bmp_u32 height,
                                 bmp_u32 stride);

#ifdef __cplusplus
}
#endif

#endif /* BMP_FILTERS_H */
