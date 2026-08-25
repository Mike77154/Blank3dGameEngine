#ifndef MFT_MFONT_H
#define MFT_MFONT_H

#include "mfont_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MFT_FNT_SIGNATURE_TEXT "ElecbyteFnt"
#define MFT_FNT_SIGNATURE      "ElecbyteFnt\0"
#define MFT_FNT_SIGNATURE_SIZE 12U
#define MFT_FNT_HEADER_SIZE    64UL
#define MFT_FNT_COMMENT_SIZE   32U

typedef char mft_fnt_header_must_match_binary_header[(sizeof(mft_fnt_header) == MFT_FNT_HEADER_SIZE) ? 1 : -1];

const char *mft_status_string(mft_status status);
void mft_image_reset(mft_image *image);
mft_status mft_image_validate(const mft_image *image);

mft_status mft_font_text_parse(mft_font_text *out,
                               const char *text,
                               mft_u32 text_size);

mft_status mft_font_text_write(char *dst,
                               mft_u32 *dst_size,
                               const mft_font_text *font);

mft_status mft_fnt_parse_header(mft_fnt_header *out,
                                const mft_u8 *fnt,
                                mft_u32 fnt_size);

void mft_fnt_comment_to_cstr(char *dst,
                             mft_u32 dst_size,
                             const mft_fnt_header *header);

mft_status mft_fnt_extract(const mft_u8 *fnt,
                           mft_u32 fnt_size,
                           mft_u8 *pcx_out,
                           mft_u32 *pcx_size,
                           char *text_out,
                           mft_u32 *text_size,
                           mft_fnt_header *header_out);

mft_status mft_fnt_pack(mft_u8 *dst,
                        mft_u32 *dst_size,
                        const mft_u8 *pcx,
                        mft_u32 pcx_size,
                        const char *text,
                        mft_u32 text_size,
                        const char *comment,
                        mft_u16 ver_hi,
                        mft_u16 ver_lo);

#ifdef __cplusplus
}
#endif

#endif
