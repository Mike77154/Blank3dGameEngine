#ifndef MFT_ZLFONT_H
#define MFT_ZLFONT_H

#include "mfont_types.h"

#ifdef __cplusplus
extern "C" {
#endif

mft_u32 zlf_adler32(const mft_u8 *data, mft_u32 size);
mft_u32 zlf_crc32(const mft_u8 *data, mft_u32 size);
mft_u32 zlf_deflate_stored_bound(mft_u32 size);

mft_status zlf_inflate_zlib(mft_u8 *dst,
                            mft_u32 *dst_size,
                            const mft_u8 *src,
                            mft_u32 src_size,
                            int verify_adler);

mft_status zlf_deflate_stored_zlib(mft_u8 *dst,
                                   mft_u32 *dst_size,
                                   const mft_u8 *src,
                                   mft_u32 src_size);

#ifdef __cplusplus
}
#endif

#endif
