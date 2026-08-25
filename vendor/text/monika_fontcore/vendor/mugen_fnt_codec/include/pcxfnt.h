#ifndef MFT_PCXFNT_H
#define MFT_PCXFNT_H

#include "mfont_types.h"

#ifdef __cplusplus
extern "C" {
#endif

mft_status pcxfnt_decode(mft_image *out,
                         const mft_u8 *pcx,
                         mft_u32 pcx_size);

mft_status pcxfnt_encode(mft_u8 *dst,
                         mft_u32 *dst_size,
                         const mft_image *image);

#ifdef __cplusplus
}
#endif

#endif
