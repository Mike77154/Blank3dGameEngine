#ifndef MFT_FNTPNG_H
#define MFT_FNTPNG_H

#include "mfont_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fntpng_workspace_tag {
    mft_u8 idat[MFT_CFG_MAX_PNG_IDAT];
    mft_u8 filtered[MFT_CFG_MAX_FILTERED];
    mft_u8 row_prev[MFT_CFG_MAX_SCANLINE];
    mft_u8 row_cur[MFT_CFG_MAX_SCANLINE];
} fntpng_workspace;

mft_status fntpng_decode(mft_image *out,
                         fntpng_workspace *ws,
                         const mft_u8 *png,
                         mft_u32 png_size);

mft_status fntpng_encode(mft_u8 *dst,
                         mft_u32 *dst_size,
                         fntpng_workspace *ws,
                         const mft_image *image);

#ifdef __cplusplus
}
#endif

#endif
