#ifndef W2F_H
#define W2F_H

#include "w2f_config.h"
#include "w2f_types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define W2F_OK 0
#define W2F_ERR_NULL_ARG -1
#define W2F_ERR_TRUNCATED -2
#define W2F_ERR_BAD_SIGNATURE -3
#define W2F_ERR_BAD_LENGTH -4
#define W2F_ERR_TOO_MANY_TABLES -5
#define W2F_ERR_BAD_BASE128 -6
#define W2F_ERR_UNSUPPORTED_COLLECTION -7
#define W2F_ERR_UNSUPPORTED_TRANSFORM -8
#define W2F_ERR_SIZE_OVERFLOW -9
#define W2F_ERR_OUTPUT_TOO_SMALL -10
#define W2F_ERR_SCRATCH_TOO_SMALL -11
#define W2F_ERR_BROTLI_UNAVAILABLE -12
#define W2F_ERR_BROTLI_FAILED -13
#define W2F_ERR_BAD_TABLE_STREAM -14
#define W2F_ERR_BAD_TABLE_ORDER -15
#define W2F_ERR_BAD_HEAD -16
#define W2F_ERR_INTERNAL -17
#define W2F_ERR_BAD_GLYF_TRANSFORM -18
#define W2F_ERR_BAD_LOCA_TRANSFORM -19
#define W2F_ERR_BAD_HMTX_TRANSFORM -20

const char *w2f_error_name(int code);

int w2f_decode_woff2_to_sfnt(const W2F_U8 *woff2,
                             W2F_U32 woff2_len,
                             W2F_U8 *sfnt_out,
                             W2F_U32 sfnt_cap,
                             W2F_U32 *sfnt_len,
                             W2F_U8 *scratch,
                             W2F_U32 scratch_cap,
                             W2F_BrotliDecodeFn brotli_decode,
                             W2F_DecoderInfo *info_out);

int w2f_probe_woff2(const W2F_U8 *woff2,
                    W2F_U32 woff2_len,
                    W2F_DecoderInfo *info_out,
                    W2F_TableRec *tables,
                    W2F_U16 table_cap);

int w2f_brotli_stub_decode(const W2F_U8 *src,
                           W2F_U32 src_len,
                           W2F_U8 *dst,
                           W2F_U32 dst_cap,
                           W2F_U32 *dst_len);

#ifdef W2F_USE_GOOGLE_BROTLI
int w2f_brotli_google_static_decode(const W2F_U8 *src,
                                    W2F_U32 src_len,
                                    W2F_U8 *dst,
                                    W2F_U32 dst_cap,
                                    W2F_U32 *dst_len);
#endif

#ifdef __cplusplus
}
#endif

#endif
