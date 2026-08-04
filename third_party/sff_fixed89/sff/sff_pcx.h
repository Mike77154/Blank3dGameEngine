#ifndef SFF_PCX_H
#define SFF_PCX_H

#include "sff_types.h"

int sff_pcx_is_8bpp_rle(const sff_u8 *raw, sff_u32 raw_len);
int sff_pcx_peek_dims(const sff_u8 *raw, sff_u32 raw_len, sff_u16 *out_w, sff_u16 *out_h);
int sff_pcx_has_palette(const sff_u8 *raw, sff_u32 raw_len);
int sff_pcx_extract_palette_rgb(const sff_u8 *raw, sff_u32 raw_len, sff_u8 out_pal_rgb[768]);

/* Decodes 8bpp PCX into caller-owned output. out_pixels_size must be >= w*h. */
int sff_pcx_decode_8bpp_into(const sff_u8 *raw, sff_u32 raw_len,
                             sff_u8 *out_pixels, sff_u32 out_pixels_size,
                             sff_u16 *out_w, sff_u16 *out_h,
                             sff_u8 out_pal_rgb[768], int *out_has_pal);

#endif /* SFF_PCX_H */
