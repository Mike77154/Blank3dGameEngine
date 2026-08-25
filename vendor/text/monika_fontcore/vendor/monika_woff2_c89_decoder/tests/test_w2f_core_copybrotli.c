#include <stdio.h>
#include "w2f.h"

#define TAG(a,b,c,d) ((((W2F_U32)(a)) << 24) | (((W2F_U32)(b)) << 16) | (((W2F_U32)(c)) << 8) | ((W2F_U32)(d)))

static W2F_U8 g_in[256];
static W2F_U8 g_scratch[256];
static W2F_U8 g_out[512];

static void wr16(W2F_U8 *p, W2F_U16 v)
{
    p[0] = (W2F_U8)((v >> 8) & 255u);
    p[1] = (W2F_U8)(v & 255u);
}

static void wr32(W2F_U8 *p, W2F_U32 v)
{
    p[0] = (W2F_U8)((v >> 24) & 255u);
    p[1] = (W2F_U8)((v >> 16) & 255u);
    p[2] = (W2F_U8)((v >> 8) & 255u);
    p[3] = (W2F_U8)(v & 255u);
}

static int copy_decode(const W2F_U8 *src, W2F_U32 src_len, W2F_U8 *dst, W2F_U32 dst_cap, W2F_U32 *dst_len)
{
    W2F_U32 i;
    if (src_len > dst_cap) return W2F_ERR_SCRATCH_TOO_SMALL;
    for (i = 0u; i < src_len; ++i) dst[i] = src[i];
    *dst_len = src_len;
    return W2F_OK;
}

int main(void)
{
    W2F_U32 p;
    W2F_U32 total_len;
    W2F_U32 out_len;
    int rc;
    W2F_DecoderInfo info;
    unsigned i;

    for (i = 0u; i < sizeof(g_in); ++i) g_in[i] = 0u;

    /* WOFF2 header */
    wr32(g_in + 0, TAG('w','O','F','2'));
    wr32(g_in + 4, 0x00010000ul); /* sfnt version */
    /* length filled later */
    wr16(g_in + 12, 2u);
    wr16(g_in + 14, 0u);
    wr32(g_in + 16, 108u); /* sfnt size: 12+32+56+8 */
    wr32(g_in + 20, 60u);  /* table stream */
    wr16(g_in + 24, 1u);
    wr16(g_in + 26, 0u);

    p = 48u;
    g_in[p++] = 1u;  /* known tag head, transform 0 */
    g_in[p++] = 54u; /* UIntBase128 54 */
    g_in[p++] = 4u;  /* known tag maxp, transform 0 */
    g_in[p++] = 6u;  /* UIntBase128 6 */

    /* fake table block: head[54], maxp[6] */
    for (i = 0u; i < 54u; ++i) g_in[p + i] = (W2F_U8)i;
    p += 54u;
    for (i = 0u; i < 6u; ++i) g_in[p + i] = (W2F_U8)(0x80u + i);
    p += 6u;
    total_len = p;
    wr32(g_in + 8, total_len);

    rc = w2f_decode_woff2_to_sfnt(g_in, total_len, g_out, sizeof(g_out), &out_len,
                                  g_scratch, sizeof(g_scratch), copy_decode, &info);
    if (rc != W2F_OK) {
        printf("FAIL decode: %s\n", w2f_error_name(rc));
        return 1;
    }
    if (out_len != 108u) {
        printf("FAIL out_len %lu\n", (unsigned long)out_len);
        return 2;
    }
    if (g_out[0] != 0u || g_out[1] != 1u || g_out[2] != 0u || g_out[3] != 0u) {
        printf("FAIL sfnt version\n");
        return 3;
    }
    if (g_out[12] != 'h' || g_out[13] != 'e' || g_out[14] != 'a' || g_out[15] != 'd') {
        printf("FAIL sorted record head\n");
        return 4;
    }
    if (g_out[28] != 'm' || g_out[29] != 'a' || g_out[30] != 'x' || g_out[31] != 'p') {
        printf("FAIL sorted record maxp\n");
        return 5;
    }

    printf("PASS core copy-brotli synthetic WOFF2\n");
    return 0;
}
