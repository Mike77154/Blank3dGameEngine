#ifndef PNG89_ZLIB89_H
#define PNG89_ZLIB89_H

#ifdef __cplusplus
extern "C" {
#endif

#include "png89.h"
#include "../zlib89/zlib89.h"

int png89_zlib89_get_requirements(const png89_u8 *data, png89_u32 size, png89_u32 flags,
                                  Png89Info *out_info, Png89Requirements *out_req);

int png89_zlib89_decode_indexed8(const png89_u8 *data, png89_u32 size,
                                 png89_u32 flags,
                                 Zlib89Scratch *inflate_scratch,
                                 png89_u8 *work, png89_u32 work_size,
                                 png89_u8 *out_pixels, png89_u32 out_pixels_size,
                                 png89_u8 out_pal_rgb[768], int *out_has_palette,
                                 Png89Info *out_info);

int png89_zlib89_decode_rgba8888(const png89_u8 *data, png89_u32 size,
                                 png89_u32 flags,
                                 Zlib89Scratch *inflate_scratch,
                                 png89_u8 *work, png89_u32 work_size,
                                 png89_u8 *out_rgba, png89_u32 out_rgba_size,
                                 Png89Info *out_info);

#ifdef __cplusplus
}
#endif

#endif
