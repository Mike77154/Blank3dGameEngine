#ifndef WOFF1_INFLATE_H
#define WOFF1_INFLATE_H

#include "woff1_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum woff1_inflate_result_e {
    WOFF1_INF_OK = 0,
    WOFF1_INF_NEED_INPUT = -1,
    WOFF1_INF_OUTPUT_FULL = -2,
    WOFF1_INF_BAD_ZLIB_HEADER = -3,
    WOFF1_INF_UNSUPPORTED_ZLIB = -4,
    WOFF1_INF_BAD_BLOCK_TYPE = -5,
    WOFF1_INF_BAD_STORED_BLOCK = -6,
    WOFF1_INF_BAD_HUFFMAN = -7,
    WOFF1_INF_BAD_DISTANCE = -8,
    WOFF1_INF_BAD_LENGTH = -9,
    WOFF1_INF_BAD_ADLER32 = -10,
    WOFF1_INF_TRAILING_DATA = -11
} woff1_inflate_result;

int woff1_inflate_zlib(const woff1_u8 *src, woff1_u32 src_len,
                       woff1_u8 *dst, woff1_u32 dst_cap,
                       woff1_u32 *dst_len, int strict_trailing);

const char *woff1_inflate_error_string(int code);
woff1_u32 woff1_adler32(const woff1_u8 *buf, woff1_u32 len);

#ifdef __cplusplus
}
#endif

#endif
