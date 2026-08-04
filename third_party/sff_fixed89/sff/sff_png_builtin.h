#ifndef SFF_PNG_BUILTIN_H
#define SFF_PNG_BUILTIN_H

#include "sff_types.h"

int sff_png_builtin_decode_indexed(const sff_u8 *blob, sff_u32 blob_len,
                                   int tolerant,
                                   sff_u8 *work_buf, sff_u32 work_buf_size,
                                   sff_u8 *out_pixels, sff_u32 out_pixels_size,
                                   sff_u16 *out_w, sff_u16 *out_h,
                                   sff_u8 out_pal_rgb[768], int *out_has_palette);

int sff_png_builtin_decode_rgba(const sff_u8 *blob, sff_u32 blob_len,
                                int tolerant,
                                sff_u8 *work_buf, sff_u32 work_buf_size,
                                sff_u8 *out_rgba, sff_u32 out_rgba_size,
                                sff_u16 *out_w, sff_u16 *out_h);

#endif
