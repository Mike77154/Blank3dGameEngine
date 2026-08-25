#ifndef IMGCC0_ZRAGF_BRIDGE_H_INCLUDED
#define IMGCC0_ZRAGF_BRIDGE_H_INCLUDED

#include "png_decoder.h"

int imgcc0_zragf_png_decompress(png_u8 *dest, png_u32 *dest_len,
                                const png_u8 *src, png_u32 src_len);

#endif
