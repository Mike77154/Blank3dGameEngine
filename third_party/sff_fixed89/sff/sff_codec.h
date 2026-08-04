/* sff_codec.h - dependency injection hooks for PNG or other image backends */
#ifndef SFF_CODEC_H
#define SFF_CODEC_H

#include "sff_types.h"

/* Generic read-at interface. Core SFF parser has no stdio dependency. */
typedef int (*SffReadAtFn)(void *user, sff_u32 offset, void *dst, sff_u32 len);

typedef struct SffIo {
    SffReadAtFn read_at;
    void *user;
    sff_u32 size;
} SffIo;

/* Codec callbacks should return:
   1  -> success
   0  -> failure / corrupt / unsupported input
*/
typedef int (*SffDecodePngIndexedFn)(void *user,
                                     const sff_u8 *png_bytes, sff_u32 png_len,
                                     sff_u8 *out_pixels, sff_u32 out_pixels_size,
                                     sff_u16 *out_w, sff_u16 *out_h,
                                     sff_u8 out_pal_rgb[768], int *out_has_palette);

typedef int (*SffDecodePngRgbaFn)(void *user,
                                  const sff_u8 *png_bytes, sff_u32 png_len,
                                  sff_u8 *out_rgba, sff_u32 out_rgba_size,
                                  sff_u16 *out_w, sff_u16 *out_h);

typedef struct SffImageCodec {
    SffDecodePngIndexedFn decode_png_indexed;
    SffDecodePngRgbaFn    decode_png_rgba;
    void *user;
} SffImageCodec;

#endif /* SFF_CODEC_H */
