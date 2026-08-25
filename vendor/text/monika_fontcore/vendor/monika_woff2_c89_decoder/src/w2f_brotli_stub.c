#include "w2f.h"

int w2f_brotli_stub_decode(const W2F_U8 *src,
                           W2F_U32 src_len,
                           W2F_U8 *dst,
                           W2F_U32 dst_cap,
                           W2F_U32 *dst_len)
{
    (void)src;
    (void)src_len;
    (void)dst;
    (void)dst_cap;
    if (dst_len != 0) *dst_len = 0u;
    return W2F_ERR_BROTLI_UNAVAILABLE;
}
