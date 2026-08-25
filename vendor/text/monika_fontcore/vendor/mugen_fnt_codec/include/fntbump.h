#ifndef MFT_FNTBUMP_H
#define MFT_FNTBUMP_H

#include "mfont_types.h"

#ifdef __cplusplus
extern "C" {
#endif

mft_status fntbump_decode(mft_image *out,
                          const mft_u8 *bmp,
                          mft_u32 bmp_size);

mft_status fntbump_encode(mft_u8 *dst,
                          mft_u32 *dst_size,
                          const mft_image *image);

#ifdef __cplusplus
}
#endif

#endif
