#include <stdio.h>
#include "w2f.h"

#define TAG(a,b,c,d) ((((W2F_U32)(a)) << 24) | (((W2F_U32)(b)) << 16) | (((W2F_U32)(c)) << 8) | ((W2F_U32)(d)))

static W2F_U8 g_in[512];
static W2F_U8 g_scratch[512];
static W2F_U8 g_out[768];

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

static W2F_U16 rd16(const W2F_U8 *p)
{
    return (W2F_U16)(((W2F_U16)p[0] << 8) | (W2F_U16)p[1]);
}

static W2F_U32 rd32(const W2F_U8 *p)
{
    return (((W2F_U32)p[0]) << 24) | (((W2F_U32)p[1]) << 16) | (((W2F_U32)p[2]) << 8) | ((W2F_U32)p[3]);
}

static int copy_decode(const W2F_U8 *src, W2F_U32 src_len, W2F_U8 *dst, W2F_U32 dst_cap, W2F_U32 *dst_len)
{
    W2F_U32 i;
    if (src_len > dst_cap) return W2F_ERR_SCRATCH_TOO_SMALL;
    for (i = 0u; i < src_len; ++i) dst[i] = src[i];
    *dst_len = src_len;
    return W2F_OK;
}

static W2F_U32 find_table_offset(const W2F_U8 *sfnt, W2F_U16 num_tables, W2F_U32 tag, W2F_U32 *len_out)
{
    W2F_U16 i;
    W2F_U32 rec;
    for (i = 0u; i < num_tables; ++i) {
        rec = 12u + ((W2F_U32)i) * 16u;
        if (rd32(sfnt + rec) == tag) {
            *len_out = rd32(sfnt + rec + 12u);
            return rd32(sfnt + rec + 8u);
        }
    }
    *len_out = 0u;
    return 0u;
}

int main(void)
{
    W2F_U32 p;
    W2F_U32 total_len;
    W2F_U32 out_len;
    W2F_U32 hmtx_off;
    W2F_U32 hmtx_len;
    int rc;
    unsigned i;

    for (i = 0u; i < sizeof(g_in); ++i) g_in[i] = 0u;

    wr32(g_in + 0, TAG('w','O','F','2'));
    wr32(g_in + 4, 0x00010000ul);
    wr16(g_in + 12, 6u);
    wr16(g_in + 14, 0u);
    wr32(g_in + 16, 236u);
    wr32(g_in + 20, 115u);
    wr16(g_in + 24, 1u);
    wr16(g_in + 26, 0u);

    p = 48u;
    g_in[p++] = 1u;   g_in[p++] = 54u;  /* head, null */
    g_in[p++] = 4u;   g_in[p++] = 6u;   /* maxp, null */
    g_in[p++] = 2u;   g_in[p++] = 36u;  /* hhea, null */
    g_in[p++] = 202u; g_in[p++] = 10u;  /* glyf, null transform v3 */
    g_in[p++] = 203u; g_in[p++] = 6u;   /* loca, null transform v3 */
    g_in[p++] = 67u;  g_in[p++] = 6u; g_in[p++] = 3u; /* hmtx v1: orig 6, transformed 3 */

    /* head[54], indexToLocFormat short at offset 50. */
    p += 54u;

    /* maxp[6], numGlyphs = 2 */
    wr16(g_in + p + 4u, 2u);
    p += 6u;

    /* hhea[36], numberOfHMetrics = 1 */
    wr16(g_in + p + 34u, 1u);
    p += 36u;

    /* glyf[10], glyph 0 has xMin=5; glyph 1 is empty by loca. */
    wr16(g_in + p + 0u, 1u);
    wr16(g_in + p + 2u, 5u);
    wr16(g_in + p + 4u, 0u);
    wr16(g_in + p + 6u, 10u);
    wr16(g_in + p + 8u, 20u);
    p += 10u;

    /* loca[6] short offsets: 0, 10, 10 encoded divided by 2. */
    wr16(g_in + p + 0u, 0u);
    wr16(g_in + p + 2u, 5u);
    wr16(g_in + p + 4u, 5u);
    p += 6u;

    /* transformed hmtx: flags reconstruct both lsb arrays, advanceWidth[0] = 1000. */
    g_in[p++] = 3u;
    wr16(g_in + p, 1000u);
    p += 2u;

    total_len = p;
    wr32(g_in + 8, total_len);

    rc = w2f_decode_woff2_to_sfnt(g_in, total_len, g_out, sizeof(g_out), &out_len,
                                  g_scratch, sizeof(g_scratch), copy_decode, 0);
    if (rc != W2F_OK) {
        printf("FAIL hmtx decode: %s\n", w2f_error_name(rc));
        return 1;
    }

    hmtx_off = find_table_offset(g_out, 6u, TAG('h','m','t','x'), &hmtx_len);
    if (hmtx_off == 0u || hmtx_len != 6u) {
        printf("FAIL hmtx lookup off=%lu len=%lu\n", (unsigned long)hmtx_off, (unsigned long)hmtx_len);
        return 2;
    }
    if (rd16(g_out + hmtx_off + 0u) != 1000u) return 3;
    if (rd16(g_out + hmtx_off + 2u) != 5u) return 4;
    if (rd16(g_out + hmtx_off + 4u) != 0u) return 5;

    printf("PASS hmtx transform v1 synthetic WOFF2\n");
    return 0;
}
