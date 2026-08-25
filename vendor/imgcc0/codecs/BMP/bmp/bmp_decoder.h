#ifndef BMP_DECODER_H
#define BMP_DECODER_H

#include "../include/bmp/bmp_export.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "bmp_parser.h"

BMP_EXPORT int bmp_calc_rgba32_buffer_size(const bmp_image *img,
                                bmp_u32 *out_size);

BMP_EXPORT int bmp_decode_to_rgba32(const bmp_image *img,
                         bmp_u8 *out_rgba,
                         bmp_u32 out_stride);
BMP_EXPORT int bmp_decode_to_rgba32_with_limits(const bmp_image *img,
                                     const bmp_limits *limits,
                                     bmp_u8 *out_rgba,
                                     bmp_u32 out_stride);

#ifdef __cplusplus
}
#endif

#endif /* BMP_DECODER_H */
