#include "w2f.h"

#define W2F_TAG(a,b,c,d) ((((W2F_U32)(a)) << 24) | (((W2F_U32)(b)) << 16) | (((W2F_U32)(c)) << 8) | ((W2F_U32)(d)))
#define W2F_WOF2 W2F_TAG('w','O','F','2')
#define W2F_TTCF W2F_TAG('t','t','c','f')
#define W2F_HEAD W2F_TAG('h','e','a','d')
#define W2F_HHEA W2F_TAG('h','h','e','a')
#define W2F_HMTX W2F_TAG('h','m','t','x')
#define W2F_MAXP W2F_TAG('m','a','x','p')
#define W2F_GLYF W2F_TAG('g','l','y','f')
#define W2F_LOCA W2F_TAG('l','o','c','a')

#define W2F_TT_ON_CURVE 0x01u
#define W2F_TT_X_SHORT  0x02u
#define W2F_TT_Y_SHORT  0x04u
#define W2F_TT_REPEAT   0x08u
#define W2F_TT_X_SAME   0x10u
#define W2F_TT_Y_SAME   0x20u
#define W2F_TT_OVERLAP  0x40u

#define W2F_COMP_ARG_WORDS   0x0001u
#define W2F_COMP_MORE        0x0020u
#define W2F_COMP_SCALE       0x0008u
#define W2F_COMP_XY_SCALE    0x0040u
#define W2F_COMP_2X2         0x0080u
#define W2F_COMP_INSTR       0x0100u

static const W2F_U32 w2f_known_tags[63] = {
    W2F_TAG('c','m','a','p'), W2F_TAG('h','e','a','d'), W2F_TAG('h','h','e','a'), W2F_TAG('h','m','t','x'),
    W2F_TAG('m','a','x','p'), W2F_TAG('n','a','m','e'), W2F_TAG('O','S','/','2'), W2F_TAG('p','o','s','t'),
    W2F_TAG('c','v','t',' '), W2F_TAG('f','p','g','m'), W2F_TAG('g','l','y','f'), W2F_TAG('l','o','c','a'),
    W2F_TAG('p','r','e','p'), W2F_TAG('C','F','F',' '), W2F_TAG('V','O','R','G'), W2F_TAG('E','B','D','T'),
    W2F_TAG('E','B','L','C'), W2F_TAG('g','a','s','p'), W2F_TAG('h','d','m','x'), W2F_TAG('k','e','r','n'),
    W2F_TAG('L','T','S','H'), W2F_TAG('P','C','L','T'), W2F_TAG('V','D','M','X'), W2F_TAG('v','h','e','a'),
    W2F_TAG('v','m','t','x'), W2F_TAG('B','A','S','E'), W2F_TAG('G','D','E','F'), W2F_TAG('G','P','O','S'),
    W2F_TAG('G','S','U','B'), W2F_TAG('E','B','S','C'), W2F_TAG('J','S','T','F'), W2F_TAG('M','A','T','H'),
    W2F_TAG('C','B','D','T'), W2F_TAG('C','B','L','C'), W2F_TAG('C','O','L','R'), W2F_TAG('C','P','A','L'),
    W2F_TAG('S','V','G',' '), W2F_TAG('s','b','i','x'), W2F_TAG('a','c','n','t'), W2F_TAG('a','v','a','r'),
    W2F_TAG('b','d','a','t'), W2F_TAG('b','l','o','c'), W2F_TAG('b','s','l','n'), W2F_TAG('c','v','a','r'),
    W2F_TAG('f','d','s','c'), W2F_TAG('f','e','a','t'), W2F_TAG('f','m','t','x'), W2F_TAG('f','v','a','r'),
    W2F_TAG('g','v','a','r'), W2F_TAG('h','s','t','y'), W2F_TAG('j','u','s','t'), W2F_TAG('l','c','a','r'),
    W2F_TAG('m','o','r','t'), W2F_TAG('m','o','r','x'), W2F_TAG('o','p','b','d'), W2F_TAG('p','r','o','p'),
    W2F_TAG('t','r','a','k'), W2F_TAG('Z','a','p','f'), W2F_TAG('S','i','l','f'), W2F_TAG('G','l','a','t'),
    W2F_TAG('G','l','o','c'), W2F_TAG('F','e','a','t'), W2F_TAG('S','i','l','l')
};

typedef struct W2F_Stream_ {
    const W2F_U8 *p;
    W2F_U32 len;
    W2F_U32 pos;
} W2F_Stream;

typedef struct W2F_GlyfCtx_ {
    W2F_Stream n_contour;
    W2F_Stream n_points;
    W2F_Stream flags;
    W2F_Stream glyph;
    W2F_Stream composite;
    W2F_Stream bbox;
    W2F_Stream instr;
    const W2F_U8 *bbox_bitmap;
    const W2F_U8 *overlap_bitmap;
    W2F_U32 bitmap_len;
    W2F_U16 num_glyphs;
    W2F_U16 index_format;
    W2F_U16 option_flags;
} W2F_GlyfCtx;

static W2F_U32 w2f_round4(W2F_U32 v)
{
    return (W2F_U32)((v + 3u) & ~((W2F_U32)3u));
}

static int w2f_add_overflow(W2F_U32 a, W2F_U32 b, W2F_U32 *out)
{
    W2F_U32 r;
    r = a + b;
    if (r < a) return 1;
    *out = r;
    return 0;
}

static W2F_U16 w2f_rd16(const W2F_U8 *p)
{
    return (W2F_U16)(((W2F_U16)p[0] << 8) | (W2F_U16)p[1]);
}

static W2F_U32 w2f_rd32(const W2F_U8 *p)
{
    return (((W2F_U32)p[0]) << 24) | (((W2F_U32)p[1]) << 16) | (((W2F_U32)p[2]) << 8) | ((W2F_U32)p[3]);
}

static W2F_S16 w2f_rd_s16(const W2F_U8 *p)
{
    return (W2F_S16)w2f_rd16(p);
}

static void w2f_wr16(W2F_U8 *p, W2F_U16 v)
{
    p[0] = (W2F_U8)((v >> 8) & 255u);
    p[1] = (W2F_U8)(v & 255u);
}

static void w2f_wr_s16(W2F_U8 *p, W2F_S16 v)
{
    w2f_wr16(p, (W2F_U16)v);
}

static void w2f_wr32(W2F_U8 *p, W2F_U32 v)
{
    p[0] = (W2F_U8)((v >> 24) & 255u);
    p[1] = (W2F_U8)((v >> 16) & 255u);
    p[2] = (W2F_U8)((v >> 8) & 255u);
    p[3] = (W2F_U8)(v & 255u);
}

static void w2f_zero(W2F_U8 *p, W2F_U32 n)
{
    W2F_U32 i;
    for (i = 0u; i < n; ++i) p[i] = 0u;
}

static void w2f_copy(W2F_U8 *dst, const W2F_U8 *src, W2F_U32 n)
{
    W2F_U32 i;
    for (i = 0u; i < n; ++i) dst[i] = src[i];
}

static int w2f_read_base128(const W2F_U8 *buf, W2F_U32 len, W2F_U32 *pos, W2F_U32 *value)
{
    W2F_U32 accum;
    W2F_U32 i;
    W2F_U8 b;

    if (buf == 0 || pos == 0 || value == 0) return W2F_ERR_NULL_ARG;
    accum = 0u;
    for (i = 0u; i < 5u; ++i) {
        if (*pos >= len) return W2F_ERR_TRUNCATED;
        b = buf[*pos];
        *pos = *pos + 1u;
        if (i == 0u && b == 0x80u) return W2F_ERR_BAD_BASE128;
        if ((accum & 0xFE000000ul) != 0u) return W2F_ERR_BAD_BASE128;
        accum = (W2F_U32)((accum << 7) | ((W2F_U32)b & 0x7fu));
        if ((b & 0x80u) == 0u) {
            *value = accum;
            return W2F_OK;
        }
    }
    return W2F_ERR_BAD_BASE128;
}

static int w2f_stream_u8(W2F_Stream *s, W2F_U8 *v)
{
    if (s == 0 || v == 0) return W2F_ERR_NULL_ARG;
    if (s->pos >= s->len) return W2F_ERR_BAD_GLYF_TRANSFORM;
    *v = s->p[s->pos];
    s->pos = s->pos + 1u;
    return W2F_OK;
}

static int w2f_stream_u16(W2F_Stream *s, W2F_U16 *v)
{
    if (s == 0 || v == 0) return W2F_ERR_NULL_ARG;
    if (s->pos + 2u > s->len) return W2F_ERR_BAD_GLYF_TRANSFORM;
    *v = w2f_rd16(s->p + s->pos);
    s->pos = s->pos + 2u;
    return W2F_OK;
}

static int w2f_stream_s16(W2F_Stream *s, W2F_S16 *v)
{
    W2F_U16 u;
    int rc;
    rc = w2f_stream_u16(s, &u);
    if (rc != W2F_OK) return rc;
    *v = (W2F_S16)u;
    return W2F_OK;
}

static int w2f_stream_255u16(W2F_Stream *s, W2F_U16 *v)
{
    W2F_U8 code;
    W2F_U8 b0;
    W2F_U8 b1;
    int rc;

    rc = w2f_stream_u8(s, &code);
    if (rc != W2F_OK) return rc;
    if (code == 253u) {
        rc = w2f_stream_u8(s, &b0);
        if (rc != W2F_OK) return rc;
        rc = w2f_stream_u8(s, &b1);
        if (rc != W2F_OK) return rc;
        *v = (W2F_U16)((((W2F_U16)b0) << 8) | (W2F_U16)b1);
    } else if (code == 255u) {
        rc = w2f_stream_u8(s, &b0);
        if (rc != W2F_OK) return rc;
        *v = (W2F_U16)((W2F_U16)b0 + 253u);
    } else if (code == 254u) {
        rc = w2f_stream_u8(s, &b0);
        if (rc != W2F_OK) return rc;
        *v = (W2F_U16)((W2F_U16)b0 + 506u);
    } else {
        *v = (W2F_U16)code;
    }
    return W2F_OK;
}

static int w2f_table_needs_transform_len(W2F_U32 tag, W2F_U8 tv)
{
    if (tag == W2F_GLYF || tag == W2F_LOCA) return tv != 3u;
    return tv != 0u;
}

static int w2f_transform_version_supported(W2F_U32 tag, W2F_U8 tv)
{
    if (tag == W2F_GLYF || tag == W2F_LOCA) return (tv == 0u || tv == 3u);
    if (tag == W2F_HMTX) return (tv == 0u || tv == 1u);
    return tv == 0u;
}

static W2F_U32 w2f_table_checksum(const W2F_U8 *data, W2F_U32 len)
{
    W2F_U32 sum;
    W2F_U32 i;
    W2F_U32 rounded;
    W2F_U32 word;
    W2F_U8 b0, b1, b2, b3;

    sum = 0u;
    rounded = w2f_round4(len);
    for (i = 0u; i < rounded; i += 4u) {
        b0 = (i < len) ? data[i] : 0u;
        b1 = ((i + 1u) < len) ? data[i + 1u] : 0u;
        b2 = ((i + 2u) < len) ? data[i + 2u] : 0u;
        b3 = ((i + 3u) < len) ? data[i + 3u] : 0u;
        word = (((W2F_U32)b0) << 24) | (((W2F_U32)b1) << 16) | (((W2F_U32)b2) << 8) | ((W2F_U32)b3);
        sum += word;
    }
    return sum;
}

static void w2f_sort_indices_by_tag(const W2F_TableRec *tables, W2F_U16 n, W2F_U16 *idx)
{
    W2F_U16 i, j, key;
    for (i = 0u; i < n; ++i) idx[i] = i;
    for (i = 1u; i < n; ++i) {
        key = idx[i];
        j = i;
        while (j > 0u && tables[idx[j - 1u]].tag > tables[key].tag) {
            idx[j] = idx[j - 1u];
            --j;
        }
        idx[j] = key;
    }
}

static void w2f_sfnt_search_params(W2F_U16 num_tables, W2F_U16 *search_range, W2F_U16 *entry_selector, W2F_U16 *range_shift)
{
    W2F_U16 max_pow2;
    W2F_U16 selector;

    max_pow2 = 1u;
    selector = 0u;
    while ((W2F_U16)(max_pow2 << 1) != 0u && (W2F_U16)(max_pow2 << 1) <= num_tables) {
        max_pow2 = (W2F_U16)(max_pow2 << 1);
        ++selector;
    }
    *search_range = (W2F_U16)(max_pow2 * 16u);
    *entry_selector = selector;
    *range_shift = (W2F_U16)((num_tables * 16u) - *search_range);
}

static int w2f_i32_to_s16(W2F_S32 v, W2F_S16 *out)
{
    if (v < -32768L || v > 32767L) return W2F_ERR_BAD_GLYF_TRANSFORM;
    *out = (W2F_S16)v;
    return W2F_OK;
}

static W2F_S32 w2f_apply_sign(W2F_S32 v, int positive)
{
    if (v == 0) return 0;
    return positive ? v : -v;
}

static int w2f_decode_triplet(W2F_U8 flag, W2F_Stream *glyph, W2F_S16 *dx, W2F_S16 *dy)
{
    W2F_U8 idx;
    W2F_U8 b0;
    W2F_U8 b1;
    W2F_U8 b2;
    W2F_U8 b3;
    W2F_U32 t;
    W2F_S32 x;
    W2F_S32 y;
    int rc;
    int sx;
    int sy;
    W2F_U32 q;
    W2F_U32 group;
    W2F_U32 inner;

    idx = (W2F_U8)(flag & 0x7fu);
    x = 0;
    y = 0;

    if (idx <= 9u) {
        rc = w2f_stream_u8(glyph, &b0);
        if (rc != W2F_OK) return rc;
        y = (W2F_S32)b0 + (W2F_S32)(((W2F_U32)idx / 2u) * 256u);
        y = w2f_apply_sign(y, (idx & 1u) != 0u);
    } else if (idx <= 19u) {
        rc = w2f_stream_u8(glyph, &b0);
        if (rc != W2F_OK) return rc;
        q = (W2F_U32)(idx - 10u);
        x = (W2F_S32)b0 + (W2F_S32)((q / 2u) * 256u);
        x = w2f_apply_sign(x, (q & 1u) != 0u);
    } else if (idx <= 83u) {
        rc = w2f_stream_u8(glyph, &b0);
        if (rc != W2F_OK) return rc;
        q = (W2F_U32)(idx - 20u);
        group = q / 16u;
        inner = q & 15u;
        sx = (inner & 1u) != 0u;
        sy = (inner & 2u) != 0u;
        x = (W2F_S32)((W2F_U32)(b0 >> 4) + 1u + group * 16u);
        y = (W2F_S32)((W2F_U32)(b0 & 15u) + 1u + (inner / 4u) * 16u);
        x = w2f_apply_sign(x, sx);
        y = w2f_apply_sign(y, sy);
    } else if (idx <= 119u) {
        rc = w2f_stream_u8(glyph, &b0);
        if (rc != W2F_OK) return rc;
        rc = w2f_stream_u8(glyph, &b1);
        if (rc != W2F_OK) return rc;
        q = (W2F_U32)(idx - 84u);
        group = q / 12u;
        inner = q % 12u;
        sx = (inner & 1u) != 0u;
        sy = (inner & 2u) != 0u;
        x = (W2F_S32)((W2F_U32)b0 + 1u + group * 256u);
        y = (W2F_S32)((W2F_U32)b1 + 1u + (inner / 4u) * 256u);
        x = w2f_apply_sign(x, sx);
        y = w2f_apply_sign(y, sy);
    } else if (idx <= 123u) {
        rc = w2f_stream_u8(glyph, &b0);
        if (rc != W2F_OK) return rc;
        rc = w2f_stream_u8(glyph, &b1);
        if (rc != W2F_OK) return rc;
        rc = w2f_stream_u8(glyph, &b2);
        if (rc != W2F_OK) return rc;
        t = (((W2F_U32)b0) << 16) | (((W2F_U32)b1) << 8) | (W2F_U32)b2;
        x = (W2F_S32)((t >> 12) & 0xfffu);
        y = (W2F_S32)(t & 0xfffu);
        sx = ((idx - 120u) & 1u) != 0u;
        sy = ((idx - 120u) & 2u) != 0u;
        x = w2f_apply_sign(x, sx);
        y = w2f_apply_sign(y, sy);
    } else {
        rc = w2f_stream_u8(glyph, &b0);
        if (rc != W2F_OK) return rc;
        rc = w2f_stream_u8(glyph, &b1);
        if (rc != W2F_OK) return rc;
        rc = w2f_stream_u8(glyph, &b2);
        if (rc != W2F_OK) return rc;
        rc = w2f_stream_u8(glyph, &b3);
        if (rc != W2F_OK) return rc;
        x = (W2F_S32)((((W2F_U32)b0) << 8) | (W2F_U32)b1);
        y = (W2F_S32)((((W2F_U32)b2) << 8) | (W2F_U32)b3);
        sx = ((idx - 124u) & 1u) != 0u;
        sy = ((idx - 124u) & 2u) != 0u;
        x = w2f_apply_sign(x, sx);
        y = w2f_apply_sign(y, sy);
    }

    rc = w2f_i32_to_s16(x, dx);
    if (rc != W2F_OK) return rc;
    rc = w2f_i32_to_s16(y, dy);
    if (rc != W2F_OK) return rc;
    return W2F_OK;
}

static W2F_U32 w2f_coord_encoded_size(W2F_S16 d)
{
    W2F_S32 v;
    v = (W2F_S32)d;
    if (v == 0) return 0u;
    if (v > 0 && v <= 255L) return 1u;
    if (v < 0 && -v <= 255L) return 1u;
    return 2u;
}

static W2F_U8 w2f_coord_flag_x(W2F_S16 d)
{
    W2F_S32 v;
    v = (W2F_S32)d;
    if (v == 0) return W2F_TT_X_SAME;
    if (v > 0 && v <= 255L) return (W2F_U8)(W2F_TT_X_SHORT | W2F_TT_X_SAME);
    if (v < 0 && -v <= 255L) return W2F_TT_X_SHORT;
    return 0u;
}

static W2F_U8 w2f_coord_flag_y(W2F_S16 d)
{
    W2F_S32 v;
    v = (W2F_S32)d;
    if (v == 0) return W2F_TT_Y_SAME;
    if (v > 0 && v <= 255L) return (W2F_U8)(W2F_TT_Y_SHORT | W2F_TT_Y_SAME);
    if (v < 0 && -v <= 255L) return W2F_TT_Y_SHORT;
    return 0u;
}

static void w2f_write_coord(W2F_U8 *out, W2F_U32 *pos, W2F_S16 d)
{
    W2F_S32 v;
    v = (W2F_S32)d;
    if (v == 0) return;
    if (v > 0 && v <= 255L) {
        out[*pos] = (W2F_U8)v;
        *pos = *pos + 1u;
    } else if (v < 0 && -v <= 255L) {
        out[*pos] = (W2F_U8)(-v);
        *pos = *pos + 1u;
    } else {
        w2f_wr_s16(out + *pos, d);
        *pos = *pos + 2u;
    }
}

static int w2f_bitmap_get(const W2F_U8 *bitmap, W2F_U32 bitmap_len, W2F_U16 index)
{
    W2F_U32 byte_index;
    W2F_U8 mask;
    byte_index = ((W2F_U32)index) >> 3;
    if (bitmap == 0 || byte_index >= bitmap_len) return 0;
    mask = (W2F_U8)(0x80u >> (index & 7u));
    return (bitmap[byte_index] & mask) != 0u;
}

static int w2f_loca_write(W2F_U8 *loca, W2F_U32 loca_len, W2F_U16 index_format, W2F_U16 glyph, W2F_U32 off)
{
    W2F_U32 p;
    if (loca == 0) return W2F_OK;
    if (index_format == 0u) {
        if ((off & 1u) != 0u) return W2F_ERR_BAD_LOCA_TRANSFORM;
        if ((off >> 1) > 65535u) return W2F_ERR_BAD_LOCA_TRANSFORM;
        p = ((W2F_U32)glyph) * 2u;
        if (p + 2u > loca_len) return W2F_ERR_BAD_LOCA_TRANSFORM;
        w2f_wr16(loca + p, (W2F_U16)(off >> 1));
    } else if (index_format == 1u) {
        p = ((W2F_U32)glyph) * 4u;
        if (p + 4u > loca_len) return W2F_ERR_BAD_LOCA_TRANSFORM;
        w2f_wr32(loca + p, off);
    } else {
        return W2F_ERR_BAD_LOCA_TRANSFORM;
    }
    return W2F_OK;
}


static int w2f_find_table_index(W2F_TableRec *tables, W2F_U16 n, W2F_U32 tag, W2F_U16 *idx)
{
    W2F_U16 i;
    if (tables == 0 || idx == 0) return W2F_ERR_NULL_ARG;
    for (i = 0u; i < n; ++i) {
        if (tables[i].tag == tag) {
            *idx = i;
            return W2F_OK;
        }
    }
    return W2F_ERR_BAD_TABLE_STREAM;
}

static int w2f_read_metric_counts_from_stream(W2F_TableRec *tables,
                                              W2F_U16 n,
                                              const W2F_U8 *table_block,
                                              W2F_U32 table_block_len,
                                              W2F_U16 *num_glyphs,
                                              W2F_U16 *num_hmetrics)
{
    W2F_U16 maxp_i;
    W2F_U16 hhea_i;
    W2F_TableRec *maxp;
    W2F_TableRec *hhea;
    int rc;

    if (tables == 0 || table_block == 0 || num_glyphs == 0 || num_hmetrics == 0) return W2F_ERR_NULL_ARG;
    rc = w2f_find_table_index(tables, n, W2F_MAXP, &maxp_i);
    if (rc != W2F_OK) return W2F_ERR_BAD_HMTX_TRANSFORM;
    rc = w2f_find_table_index(tables, n, W2F_HHEA, &hhea_i);
    if (rc != W2F_OK) return W2F_ERR_BAD_HMTX_TRANSFORM;
    maxp = &tables[maxp_i];
    hhea = &tables[hhea_i];
    if (maxp->transform_version != 0u || hhea->transform_version != 0u) return W2F_ERR_BAD_HMTX_TRANSFORM;
    if (maxp->transform_len < 6u || hhea->transform_len < 36u) return W2F_ERR_BAD_HMTX_TRANSFORM;
    if (maxp->src_off + maxp->transform_len < maxp->src_off) return W2F_ERR_SIZE_OVERFLOW;
    if (hhea->src_off + hhea->transform_len < hhea->src_off) return W2F_ERR_SIZE_OVERFLOW;
    if (maxp->src_off + maxp->transform_len > table_block_len) return W2F_ERR_BAD_HMTX_TRANSFORM;
    if (hhea->src_off + hhea->transform_len > table_block_len) return W2F_ERR_BAD_HMTX_TRANSFORM;

    *num_glyphs = w2f_rd16(table_block + maxp->src_off + 4u);
    *num_hmetrics = w2f_rd16(table_block + hhea->src_off + 34u);
    if (*num_glyphs == 0u || *num_hmetrics == 0u) return W2F_ERR_BAD_HMTX_TRANSFORM;
    if (*num_hmetrics > *num_glyphs) return W2F_ERR_BAD_HMTX_TRANSFORM;
    return W2F_OK;
}

static int w2f_infer_loca_format(W2F_U32 loca_len, W2F_U16 num_glyphs, W2F_U16 *index_format)
{
    W2F_U32 count;
    if (index_format == 0) return W2F_ERR_NULL_ARG;
    count = (W2F_U32)num_glyphs + 1u;
    if (loca_len == count * 2u) {
        *index_format = 0u;
        return W2F_OK;
    }
    if (loca_len == count * 4u) {
        *index_format = 1u;
        return W2F_OK;
    }
    return W2F_ERR_BAD_LOCA_TRANSFORM;
}

static int w2f_loca_read_offset(const W2F_U8 *loca,
                                W2F_U32 loca_len,
                                W2F_U16 index_format,
                                W2F_U16 glyph,
                                W2F_U32 *off)
{
    W2F_U32 p;
    if (loca == 0 || off == 0) return W2F_ERR_NULL_ARG;
    if (index_format == 0u) {
        p = ((W2F_U32)glyph) * 2u;
        if (p + 2u > loca_len) return W2F_ERR_BAD_LOCA_TRANSFORM;
        *off = ((W2F_U32)w2f_rd16(loca + p)) << 1;
    } else if (index_format == 1u) {
        p = ((W2F_U32)glyph) * 4u;
        if (p + 4u > loca_len) return W2F_ERR_BAD_LOCA_TRANSFORM;
        *off = w2f_rd32(loca + p);
    } else {
        return W2F_ERR_BAD_LOCA_TRANSFORM;
    }
    return W2F_OK;
}

static int w2f_glyf_xmin_for_glyph(const W2F_U8 *glyf,
                                   W2F_U32 glyf_len,
                                   const W2F_U8 *loca,
                                   W2F_U32 loca_len,
                                   W2F_U16 index_format,
                                   W2F_U16 num_glyphs,
                                   W2F_U16 glyph,
                                   W2F_S16 *xmin)
{
    W2F_U32 off0;
    W2F_U32 off1;
    int rc;
    if (glyph >= num_glyphs) return W2F_ERR_BAD_HMTX_TRANSFORM;
    if (glyf == 0 || loca == 0 || xmin == 0) return W2F_ERR_NULL_ARG;
    rc = w2f_loca_read_offset(loca, loca_len, index_format, glyph, &off0);
    if (rc != W2F_OK) return rc;
    rc = w2f_loca_read_offset(loca, loca_len, index_format, (W2F_U16)(glyph + 1u), &off1);
    if (rc != W2F_OK) return rc;
    if (off1 < off0 || off1 > glyf_len) return W2F_ERR_BAD_HMTX_TRANSFORM;
    if (off0 == off1) {
        *xmin = 0;
        return W2F_OK;
    }
    if (off1 - off0 < 10u) return W2F_ERR_BAD_HMTX_TRANSFORM;
    *xmin = w2f_rd_s16(glyf + off0 + 2u);
    return W2F_OK;
}

static int w2f_check_hmtx_transform(const W2F_U8 *in,
                                    W2F_U32 in_len,
                                    W2F_U32 orig_len,
                                    W2F_U16 num_glyphs,
                                    W2F_U16 num_hmetrics)
{
    W2F_U8 flags;
    W2F_U32 expected_in;
    W2F_U32 expected_out;
    W2F_U32 mono_count;

    if (in == 0) return W2F_ERR_NULL_ARG;
    if (num_hmetrics == 0u || num_hmetrics > num_glyphs) return W2F_ERR_BAD_HMTX_TRANSFORM;
    if (in_len < 1u) return W2F_ERR_BAD_HMTX_TRANSFORM;
    flags = in[0];
    if ((flags & 0xfcu) != 0u) return W2F_ERR_BAD_HMTX_TRANSFORM;
    if ((flags & 0x03u) == 0u) return W2F_ERR_BAD_HMTX_TRANSFORM;

    mono_count = (W2F_U32)num_glyphs - (W2F_U32)num_hmetrics;
    expected_out = ((W2F_U32)num_hmetrics) * 4u + mono_count * 2u;
    if (orig_len != expected_out) return W2F_ERR_BAD_HMTX_TRANSFORM;

    expected_in = 1u + ((W2F_U32)num_hmetrics) * 2u;
    if ((flags & 0x01u) == 0u) {
        if (w2f_add_overflow(expected_in, ((W2F_U32)num_hmetrics) * 2u, &expected_in)) return W2F_ERR_SIZE_OVERFLOW;
    }
    if ((flags & 0x02u) == 0u) {
        if (w2f_add_overflow(expected_in, mono_count * 2u, &expected_in)) return W2F_ERR_SIZE_OVERFLOW;
    }
    if (in_len != expected_in) return W2F_ERR_BAD_HMTX_TRANSFORM;
    return W2F_OK;
}

static int w2f_decode_hmtx_transform(const W2F_U8 *in,
                                     W2F_U32 in_len,
                                     W2F_U8 *out,
                                     W2F_U32 out_cap,
                                     W2F_U16 num_glyphs,
                                     W2F_U16 num_hmetrics,
                                     const W2F_U8 *glyf,
                                     W2F_U32 glyf_len,
                                     const W2F_U8 *loca,
                                     W2F_U32 loca_len,
                                     W2F_U16 loca_format,
                                     W2F_U32 *out_len)
{
    W2F_U8 flags;
    W2F_U32 p_adv;
    W2F_U32 p_lsb;
    W2F_U32 p_mono;
    W2F_U32 q;
    W2F_U32 expected_out;
    W2F_U32 mono_count;
    W2F_U16 g;
    W2F_S16 xmin;
    int rc;

    if (out == 0 || out_len == 0) return W2F_ERR_NULL_ARG;
    rc = w2f_check_hmtx_transform(in, in_len, out_cap, num_glyphs, num_hmetrics);
    if (rc != W2F_OK) return rc;

    flags = in[0];
    mono_count = (W2F_U32)num_glyphs - (W2F_U32)num_hmetrics;
    expected_out = ((W2F_U32)num_hmetrics) * 4u + mono_count * 2u;
    if (out_cap < expected_out) return W2F_ERR_OUTPUT_TOO_SMALL;

    p_adv = 1u;
    p_lsb = p_adv + ((W2F_U32)num_hmetrics) * 2u;
    p_mono = p_lsb;
    if ((flags & 0x01u) == 0u) p_mono += ((W2F_U32)num_hmetrics) * 2u;

    q = 0u;
    for (g = 0u; g < num_hmetrics; ++g) {
        w2f_wr16(out + q, w2f_rd16(in + p_adv + ((W2F_U32)g) * 2u));
        q += 2u;
        if ((flags & 0x01u) != 0u) {
            rc = w2f_glyf_xmin_for_glyph(glyf, glyf_len, loca, loca_len, loca_format, num_glyphs, g, &xmin);
            if (rc != W2F_OK) return rc;
            w2f_wr_s16(out + q, xmin);
        } else {
            w2f_wr16(out + q, w2f_rd16(in + p_lsb + ((W2F_U32)g) * 2u));
        }
        q += 2u;
    }

    for (g = num_hmetrics; g < num_glyphs; ++g) {
        if ((flags & 0x02u) != 0u) {
            rc = w2f_glyf_xmin_for_glyph(glyf, glyf_len, loca, loca_len, loca_format, num_glyphs, g, &xmin);
            if (rc != W2F_OK) return rc;
            w2f_wr_s16(out + q, xmin);
        } else {
            w2f_wr16(out + q, w2f_rd16(in + p_mono + (((W2F_U32)g - (W2F_U32)num_hmetrics) * 2u)));
        }
        q += 2u;
    }

    if (q != expected_out) return W2F_ERR_INTERNAL;
    *out_len = expected_out;
    return W2F_OK;
}

static int w2f_scan_simple(W2F_GlyfCtx *ctx,
                           W2F_U16 glyph_index,
                           W2F_S16 n_contours,
                           W2F_U32 *glyph_size,
                           W2F_S16 *bbox_out,
                           W2F_U32 *n_points_out,
                           W2F_U32 *x_size_out,
                           W2F_U32 *y_size_out,
                           W2F_U16 *instr_len_out)
{
    W2F_U16 c;
    W2F_U16 count;
    W2F_U32 n_points;
    W2F_U32 i;
    W2F_U8 f;
    W2F_S16 dx;
    W2F_S16 dy;
    W2F_S32 x;
    W2F_S32 y;
    W2F_S32 xmin;
    W2F_S32 ymin;
    W2F_S32 xmax;
    W2F_S32 ymax;
    W2F_U32 x_size;
    W2F_U32 y_size;
    W2F_U16 instr_len;
    int rc;
    int have_bbox;
    int explicit_bbox;
    W2F_S16 bb;
    W2F_U32 tmp;

    n_points = 0u;
    for (c = 0u; c < (W2F_U16)n_contours; ++c) {
        rc = w2f_stream_255u16(&ctx->n_points, &count);
        if (rc != W2F_OK) return rc;
        if (count == 0u) return W2F_ERR_BAD_GLYF_TRANSFORM;
        if (w2f_add_overflow(n_points, (W2F_U32)count, &n_points)) return W2F_ERR_SIZE_OVERFLOW;
        if (n_points > 65535u) return W2F_ERR_BAD_GLYF_TRANSFORM;
    }

    x = 0;
    y = 0;
    xmin = 0;
    ymin = 0;
    xmax = 0;
    ymax = 0;
    x_size = 0u;
    y_size = 0u;
    have_bbox = 0;
    for (i = 0u; i < n_points; ++i) {
        rc = w2f_stream_u8(&ctx->flags, &f);
        if (rc != W2F_OK) return rc;
        rc = w2f_decode_triplet(f, &ctx->glyph, &dx, &dy);
        if (rc != W2F_OK) return rc;
        x += (W2F_S32)dx;
        y += (W2F_S32)dy;
        if (!have_bbox) {
            xmin = x; xmax = x; ymin = y; ymax = y; have_bbox = 1;
        } else {
            if (x < xmin) xmin = x;
            if (x > xmax) xmax = x;
            if (y < ymin) ymin = y;
            if (y > ymax) ymax = y;
        }
        x_size += w2f_coord_encoded_size(dx);
        y_size += w2f_coord_encoded_size(dy);
    }

    rc = w2f_stream_255u16(&ctx->glyph, &instr_len);
    if (rc != W2F_OK) return rc;
    if (ctx->instr.pos + (W2F_U32)instr_len > ctx->instr.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
    ctx->instr.pos += (W2F_U32)instr_len;

    explicit_bbox = w2f_bitmap_get(ctx->bbox_bitmap, ctx->bitmap_len, glyph_index);
    if (explicit_bbox) {
        rc = w2f_stream_s16(&ctx->bbox, &bb); if (rc != W2F_OK) return rc; bbox_out[0] = bb;
        rc = w2f_stream_s16(&ctx->bbox, &bb); if (rc != W2F_OK) return rc; bbox_out[1] = bb;
        rc = w2f_stream_s16(&ctx->bbox, &bb); if (rc != W2F_OK) return rc; bbox_out[2] = bb;
        rc = w2f_stream_s16(&ctx->bbox, &bb); if (rc != W2F_OK) return rc; bbox_out[3] = bb;
    } else {
        rc = w2f_i32_to_s16(xmin, &bbox_out[0]); if (rc != W2F_OK) return rc;
        rc = w2f_i32_to_s16(ymin, &bbox_out[1]); if (rc != W2F_OK) return rc;
        rc = w2f_i32_to_s16(xmax, &bbox_out[2]); if (rc != W2F_OK) return rc;
        rc = w2f_i32_to_s16(ymax, &bbox_out[3]); if (rc != W2F_OK) return rc;
    }

    tmp = 10u;
    if (w2f_add_overflow(tmp, ((W2F_U32)n_contours) * 2u, &tmp)) return W2F_ERR_SIZE_OVERFLOW;
    if (w2f_add_overflow(tmp, 2u, &tmp)) return W2F_ERR_SIZE_OVERFLOW;
    if (w2f_add_overflow(tmp, (W2F_U32)instr_len, &tmp)) return W2F_ERR_SIZE_OVERFLOW;
    if (w2f_add_overflow(tmp, n_points, &tmp)) return W2F_ERR_SIZE_OVERFLOW;
    if (w2f_add_overflow(tmp, x_size, &tmp)) return W2F_ERR_SIZE_OVERFLOW;
    if (w2f_add_overflow(tmp, y_size, &tmp)) return W2F_ERR_SIZE_OVERFLOW;

    *glyph_size = tmp;
    *n_points_out = n_points;
    *x_size_out = x_size;
    *y_size_out = y_size;
    *instr_len_out = instr_len;
    return W2F_OK;
}

static int w2f_emit_simple(W2F_GlyfCtx *ctx,
                           W2F_U8 *out,
                           W2F_U32 out_cap,
                           W2F_U32 out_off,
                           W2F_U16 glyph_index,
                           W2F_S16 n_contours,
                           W2F_U32 *glyph_size_out)
{
    W2F_U32 np0;
    W2F_U32 fl0;
    W2F_U32 gy0;
    W2F_U32 in0;
    W2F_U32 bb0;
    W2F_U32 np1;
    W2F_U32 fl1;
    W2F_U32 gy1;
    W2F_U32 in1;
    W2F_U32 bb1;
    W2F_U32 glyph_size;
    W2F_U32 n_points;
    W2F_U32 x_size;
    W2F_U32 y_size;
    W2F_U16 instr_len;
    W2F_S16 bbox[4];
    W2F_U32 p;
    W2F_U32 flags_pos;
    W2F_U32 x_pos;
    W2F_U32 y_pos;
    W2F_U32 i;
    W2F_U16 c;
    W2F_U16 count;
    W2F_U32 accum;
    W2F_U8 wf;
    W2F_U8 tf;
    W2F_S16 dx;
    W2F_S16 dy;
    W2F_U16 instr_len2;
    int rc;

    np0 = ctx->n_points.pos;
    fl0 = ctx->flags.pos;
    gy0 = ctx->glyph.pos;
    in0 = ctx->instr.pos;
    bb0 = ctx->bbox.pos;

    rc = w2f_scan_simple(ctx, glyph_index, n_contours, &glyph_size, bbox, &n_points, &x_size, &y_size, &instr_len);
    if (rc != W2F_OK) return rc;

    np1 = ctx->n_points.pos;
    fl1 = ctx->flags.pos;
    gy1 = ctx->glyph.pos;
    in1 = ctx->instr.pos;
    bb1 = ctx->bbox.pos;

    if (out != 0) {
        if (out_off + glyph_size > out_cap || out_off + glyph_size < out_off) return W2F_ERR_OUTPUT_TOO_SMALL;

        ctx->n_points.pos = np0;
        ctx->flags.pos = fl0;
        ctx->glyph.pos = gy0;
        ctx->instr.pos = in0;
        ctx->bbox.pos = bb0;

        w2f_wr_s16(out + out_off + 0u, n_contours);
        w2f_wr_s16(out + out_off + 2u, bbox[0]);
        w2f_wr_s16(out + out_off + 4u, bbox[1]);
        w2f_wr_s16(out + out_off + 6u, bbox[2]);
        w2f_wr_s16(out + out_off + 8u, bbox[3]);

        p = out_off + 10u;
        accum = 0u;
        for (c = 0u; c < (W2F_U16)n_contours; ++c) {
            rc = w2f_stream_255u16(&ctx->n_points, &count);
            if (rc != W2F_OK) return rc;
            accum += (W2F_U32)count;
            w2f_wr16(out + p, (W2F_U16)(accum - 1u));
            p += 2u;
        }

        gy0 = ctx->glyph.pos;
        fl0 = ctx->flags.pos;
        for (i = 0u; i < n_points; ++i) {
            rc = w2f_stream_u8(&ctx->flags, &wf);
            if (rc != W2F_OK) return rc;
            rc = w2f_decode_triplet(wf, &ctx->glyph, &dx, &dy);
            if (rc != W2F_OK) return rc;
        }
        rc = w2f_stream_255u16(&ctx->glyph, &instr_len2);
        if (rc != W2F_OK) return rc;
        if (instr_len2 != instr_len) return W2F_ERR_BAD_GLYF_TRANSFORM;

        w2f_wr16(out + p, instr_len);
        p += 2u;
        if (ctx->instr.pos + (W2F_U32)instr_len > ctx->instr.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
        w2f_copy(out + p, ctx->instr.p + ctx->instr.pos, (W2F_U32)instr_len);
        ctx->instr.pos += (W2F_U32)instr_len;
        p += (W2F_U32)instr_len;

        flags_pos = p;
        x_pos = flags_pos + n_points;
        y_pos = x_pos + x_size;

        ctx->glyph.pos = gy0;
        ctx->flags.pos = fl0;
        for (i = 0u; i < n_points; ++i) {
            rc = w2f_stream_u8(&ctx->flags, &wf);
            if (rc != W2F_OK) return rc;
            rc = w2f_decode_triplet(wf, &ctx->glyph, &dx, &dy);
            if (rc != W2F_OK) return rc;
            tf = 0u;
            if ((wf & 0x80u) == 0u) tf = (W2F_U8)(tf | W2F_TT_ON_CURVE);
            tf = (W2F_U8)(tf | w2f_coord_flag_x(dx) | w2f_coord_flag_y(dy));
            if (i == 0u && w2f_bitmap_get(ctx->overlap_bitmap, ctx->bitmap_len, glyph_index)) {
                tf = (W2F_U8)(tf | W2F_TT_OVERLAP);
            }
            out[flags_pos + i] = tf;
            w2f_write_coord(out, &x_pos, dx);
            w2f_write_coord(out, &y_pos, dy);
        }
        if (y_pos != out_off + glyph_size) return W2F_ERR_INTERNAL;
    }

    ctx->n_points.pos = np1;
    ctx->flags.pos = fl1;
    ctx->glyph.pos = gy1;
    ctx->instr.pos = in1;
    ctx->bbox.pos = bb1;
    *glyph_size_out = glyph_size;
    return W2F_OK;
}

static W2F_U32 w2f_composite_arg_bytes(W2F_U16 flags)
{
    W2F_U32 n;
    n = 2u;
    if ((flags & W2F_COMP_ARG_WORDS) != 0u) n += 4u;
    else n += 2u;
    if ((flags & W2F_COMP_SCALE) != 0u) n += 2u;
    else if ((flags & W2F_COMP_XY_SCALE) != 0u) n += 4u;
    else if ((flags & W2F_COMP_2X2) != 0u) n += 8u;
    return n;
}

static int w2f_scan_composite(W2F_GlyfCtx *ctx,
                              W2F_U16 glyph_index,
                              W2F_U32 *glyph_size,
                              W2F_S16 *bbox,
                              W2F_U16 *instr_len)
{
    int explicit_bbox;
    int have_instr;
    int more;
    W2F_U16 flags;
    W2F_U32 arg_bytes;
    W2F_U32 comp_bytes;
    W2F_S16 bb;
    W2F_U16 ilen;
    int rc;
    W2F_U32 tmp;

    explicit_bbox = w2f_bitmap_get(ctx->bbox_bitmap, ctx->bitmap_len, glyph_index);
    if (!explicit_bbox) return W2F_ERR_BAD_GLYF_TRANSFORM;
    rc = w2f_stream_s16(&ctx->bbox, &bb); if (rc != W2F_OK) return rc; bbox[0] = bb;
    rc = w2f_stream_s16(&ctx->bbox, &bb); if (rc != W2F_OK) return rc; bbox[1] = bb;
    rc = w2f_stream_s16(&ctx->bbox, &bb); if (rc != W2F_OK) return rc; bbox[2] = bb;
    rc = w2f_stream_s16(&ctx->bbox, &bb); if (rc != W2F_OK) return rc; bbox[3] = bb;

    have_instr = 0;
    comp_bytes = 0u;
    more = 1;
    while (more) {
        rc = w2f_stream_u16(&ctx->composite, &flags);
        if (rc != W2F_OK) return rc;
        arg_bytes = w2f_composite_arg_bytes(flags);
        if (ctx->composite.pos + arg_bytes > ctx->composite.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
        ctx->composite.pos += arg_bytes;
        if (w2f_add_overflow(comp_bytes, 2u + arg_bytes, &comp_bytes)) return W2F_ERR_SIZE_OVERFLOW;
        if ((flags & W2F_COMP_INSTR) != 0u) have_instr = 1;
        more = (flags & W2F_COMP_MORE) != 0u;
    }

    ilen = 0u;
    if (have_instr) {
        rc = w2f_stream_255u16(&ctx->glyph, &ilen);
        if (rc != W2F_OK) return rc;
        if (ctx->instr.pos + (W2F_U32)ilen > ctx->instr.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
        ctx->instr.pos += (W2F_U32)ilen;
    }

    tmp = 10u;
    if (w2f_add_overflow(tmp, comp_bytes, &tmp)) return W2F_ERR_SIZE_OVERFLOW;
    if (have_instr) {
        if (w2f_add_overflow(tmp, 2u, &tmp)) return W2F_ERR_SIZE_OVERFLOW;
        if (w2f_add_overflow(tmp, (W2F_U32)ilen, &tmp)) return W2F_ERR_SIZE_OVERFLOW;
    }
    *glyph_size = tmp;
    *instr_len = ilen;
    return W2F_OK;
}

static int w2f_emit_composite(W2F_GlyfCtx *ctx,
                              W2F_U8 *out,
                              W2F_U32 out_cap,
                              W2F_U32 out_off,
                              W2F_U16 glyph_index,
                              W2F_U32 *glyph_size_out)
{
    W2F_U32 comp0;
    W2F_U32 gly0;
    W2F_U32 ins0;
    W2F_U32 bb0;
    W2F_U32 comp1;
    W2F_U32 gly1;
    W2F_U32 ins1;
    W2F_U32 bb1;
    W2F_U32 glyph_size;
    W2F_S16 bbox[4];
    W2F_U16 instr_len;
    W2F_U32 p;
    W2F_U16 flags;
    W2F_U32 arg_bytes;
    int more;
    int have_instr;
    W2F_U16 ilen2;
    int rc;

    comp0 = ctx->composite.pos;
    gly0 = ctx->glyph.pos;
    ins0 = ctx->instr.pos;
    bb0 = ctx->bbox.pos;

    rc = w2f_scan_composite(ctx, glyph_index, &glyph_size, bbox, &instr_len);
    if (rc != W2F_OK) return rc;

    comp1 = ctx->composite.pos;
    gly1 = ctx->glyph.pos;
    ins1 = ctx->instr.pos;
    bb1 = ctx->bbox.pos;

    if (out != 0) {
        if (out_off + glyph_size > out_cap || out_off + glyph_size < out_off) return W2F_ERR_OUTPUT_TOO_SMALL;

        ctx->composite.pos = comp0;
        ctx->glyph.pos = gly0;
        ctx->instr.pos = ins0;
        ctx->bbox.pos = bb0;

        rc = w2f_stream_s16(&ctx->bbox, &bbox[0]); if (rc != W2F_OK) return rc;
        rc = w2f_stream_s16(&ctx->bbox, &bbox[1]); if (rc != W2F_OK) return rc;
        rc = w2f_stream_s16(&ctx->bbox, &bbox[2]); if (rc != W2F_OK) return rc;
        rc = w2f_stream_s16(&ctx->bbox, &bbox[3]); if (rc != W2F_OK) return rc;

        w2f_wr_s16(out + out_off + 0u, (W2F_S16)-1);
        w2f_wr_s16(out + out_off + 2u, bbox[0]);
        w2f_wr_s16(out + out_off + 4u, bbox[1]);
        w2f_wr_s16(out + out_off + 6u, bbox[2]);
        w2f_wr_s16(out + out_off + 8u, bbox[3]);
        p = out_off + 10u;
        more = 1;
        have_instr = 0;
        while (more) {
            rc = w2f_stream_u16(&ctx->composite, &flags);
            if (rc != W2F_OK) return rc;
            arg_bytes = w2f_composite_arg_bytes(flags);
            if (ctx->composite.pos + arg_bytes > ctx->composite.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
            w2f_wr16(out + p, flags);
            p += 2u;
            w2f_copy(out + p, ctx->composite.p + ctx->composite.pos, arg_bytes);
            p += arg_bytes;
            ctx->composite.pos += arg_bytes;
            if ((flags & W2F_COMP_INSTR) != 0u) have_instr = 1;
            more = (flags & W2F_COMP_MORE) != 0u;
        }
        if (have_instr) {
            rc = w2f_stream_255u16(&ctx->glyph, &ilen2);
            if (rc != W2F_OK) return rc;
            if (ilen2 != instr_len) return W2F_ERR_BAD_GLYF_TRANSFORM;
            w2f_wr16(out + p, instr_len);
            p += 2u;
            if (ctx->instr.pos + (W2F_U32)instr_len > ctx->instr.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
            w2f_copy(out + p, ctx->instr.p + ctx->instr.pos, (W2F_U32)instr_len);
            ctx->instr.pos += (W2F_U32)instr_len;
            p += (W2F_U32)instr_len;
        }
        if (p != out_off + glyph_size) return W2F_ERR_INTERNAL;
    }

    ctx->composite.pos = comp1;
    ctx->glyph.pos = gly1;
    ctx->instr.pos = ins1;
    ctx->bbox.pos = bb1;
    *glyph_size_out = glyph_size;
    return W2F_OK;
}

static int w2f_parse_glyf_header(const W2F_U8 *in, W2F_U32 in_len, W2F_GlyfCtx *ctx)
{
    W2F_U32 sizes[7];
    W2F_U32 p;
    W2F_U32 end;
    W2F_U32 bitmap_len;
    W2F_U32 overlap_len;
    W2F_U32 i;

    if (in == 0 || ctx == 0) return W2F_ERR_NULL_ARG;
    if (in_len < 36u) return W2F_ERR_BAD_GLYF_TRANSFORM;
    if (w2f_rd16(in + 0u) != 0u) return W2F_ERR_BAD_GLYF_TRANSFORM;
    ctx->option_flags = w2f_rd16(in + 2u);
    if ((ctx->option_flags & 0xfffeu) != 0u) return W2F_ERR_BAD_GLYF_TRANSFORM;
    ctx->num_glyphs = w2f_rd16(in + 4u);
    ctx->index_format = w2f_rd16(in + 6u);
    if (ctx->index_format > 1u) return W2F_ERR_BAD_GLYF_TRANSFORM;
    for (i = 0u; i < 7u; ++i) sizes[i] = w2f_rd32(in + 8u + i * 4u);
    if (sizes[0] != ((W2F_U32)ctx->num_glyphs) * 2u) return W2F_ERR_BAD_GLYF_TRANSFORM;
    bitmap_len = (((W2F_U32)ctx->num_glyphs + 31u) >> 5) * 4u;
    if (sizes[5] < bitmap_len) return W2F_ERR_BAD_GLYF_TRANSFORM;
    overlap_len = ((ctx->option_flags & 1u) != 0u) ? bitmap_len : 0u;

    p = 36u;
    for (i = 0u; i < 7u; ++i) {
        if (w2f_add_overflow(p, sizes[i], &end)) return W2F_ERR_SIZE_OVERFLOW;
        if (end > in_len) return W2F_ERR_BAD_GLYF_TRANSFORM;
        switch (i) {
            case 0u: ctx->n_contour.p = in + p; ctx->n_contour.len = sizes[i]; ctx->n_contour.pos = 0u; break;
            case 1u: ctx->n_points.p = in + p; ctx->n_points.len = sizes[i]; ctx->n_points.pos = 0u; break;
            case 2u: ctx->flags.p = in + p; ctx->flags.len = sizes[i]; ctx->flags.pos = 0u; break;
            case 3u: ctx->glyph.p = in + p; ctx->glyph.len = sizes[i]; ctx->glyph.pos = 0u; break;
            case 4u: ctx->composite.p = in + p; ctx->composite.len = sizes[i]; ctx->composite.pos = 0u; break;
            case 5u:
                ctx->bbox_bitmap = in + p;
                ctx->bitmap_len = bitmap_len;
                ctx->bbox.p = in + p + bitmap_len;
                ctx->bbox.len = sizes[i] - bitmap_len;
                ctx->bbox.pos = 0u;
                break;
            case 6u: ctx->instr.p = in + p; ctx->instr.len = sizes[i]; ctx->instr.pos = 0u; break;
            default: break;
        }
        p = end;
    }
    if (w2f_add_overflow(p, overlap_len, &end)) return W2F_ERR_SIZE_OVERFLOW;
    if (end != in_len) return W2F_ERR_BAD_GLYF_TRANSFORM;
    ctx->overlap_bitmap = (overlap_len != 0u) ? (in + p) : 0;
    return W2F_OK;
}

static int w2f_decode_glyf_transform(const W2F_U8 *in,
                                     W2F_U32 in_len,
                                     W2F_U8 *glyf_out,
                                     W2F_U32 glyf_cap,
                                     W2F_U32 *glyf_len,
                                     W2F_U8 *loca_out,
                                     W2F_U32 loca_len,
                                     W2F_U32 *loca_len_out,
                                     W2F_U16 *num_glyphs_out,
                                     W2F_U16 *index_format_out)
{
    W2F_GlyfCtx ctx;
    W2F_U16 g;
    W2F_S16 n_contours;
    W2F_U32 off;
    W2F_U32 glyph_size;
    int rc;
    W2F_U32 expected_loca;
    W2F_U32 padded;

    if (glyf_len == 0 || loca_len_out == 0) return W2F_ERR_NULL_ARG;
    rc = w2f_parse_glyf_header(in, in_len, &ctx);
    if (rc != W2F_OK) return rc;

    expected_loca = ((W2F_U32)ctx.num_glyphs + 1u) * ((ctx.index_format == 0u) ? 2u : 4u);
    if (loca_out != 0 && loca_len < expected_loca) return W2F_ERR_BAD_LOCA_TRANSFORM;
    if (num_glyphs_out != 0) *num_glyphs_out = ctx.num_glyphs;
    if (index_format_out != 0) *index_format_out = ctx.index_format;
    *loca_len_out = expected_loca;

    off = 0u;
    for (g = 0u; g < ctx.num_glyphs; ++g) {
        rc = w2f_loca_write(loca_out, loca_len, ctx.index_format, g, off);
        if (rc != W2F_OK) return rc;
        rc = w2f_stream_s16(&ctx.n_contour, &n_contours);
        if (rc != W2F_OK) return rc;
        if (n_contours == 0) {
            if (w2f_bitmap_get(ctx.bbox_bitmap, ctx.bitmap_len, g)) return W2F_ERR_BAD_GLYF_TRANSFORM;
            glyph_size = 0u;
        } else if (n_contours > 0) {
            rc = w2f_emit_simple(&ctx, glyf_out, glyf_cap, off, g, n_contours, &glyph_size);
            if (rc != W2F_OK) return rc;
        } else if (n_contours == (W2F_S16)-1) {
            rc = w2f_emit_composite(&ctx, glyf_out, glyf_cap, off, g, &glyph_size);
            if (rc != W2F_OK) return rc;
        } else {
            return W2F_ERR_BAD_GLYF_TRANSFORM;
        }
        if (w2f_add_overflow(off, glyph_size, &off)) return W2F_ERR_SIZE_OVERFLOW;
        if (ctx.index_format == 0u && (off & 1u) != 0u) {
            if (glyf_out != 0) {
                if (off >= glyf_cap) return W2F_ERR_OUTPUT_TOO_SMALL;
                glyf_out[off] = 0u;
            }
            off += 1u;
        }
    }

    rc = w2f_loca_write(loca_out, loca_len, ctx.index_format, ctx.num_glyphs, off);
    if (rc != W2F_OK) return rc;

    padded = off;
    (void)padded;
    if (ctx.n_contour.pos != ctx.n_contour.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
    if (ctx.n_points.pos != ctx.n_points.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
    if (ctx.flags.pos != ctx.flags.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
    if (ctx.glyph.pos != ctx.glyph.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
    if (ctx.composite.pos != ctx.composite.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
    if (ctx.bbox.pos != ctx.bbox.len) return W2F_ERR_BAD_GLYF_TRANSFORM;
    if (ctx.instr.pos != ctx.instr.len) return W2F_ERR_BAD_GLYF_TRANSFORM;

    *glyf_len = off;
    return W2F_OK;
}

const char *w2f_error_name(int code)
{
    switch (code) {
        case W2F_OK: return "W2F_OK";
        case W2F_ERR_NULL_ARG: return "W2F_ERR_NULL_ARG";
        case W2F_ERR_TRUNCATED: return "W2F_ERR_TRUNCATED";
        case W2F_ERR_BAD_SIGNATURE: return "W2F_ERR_BAD_SIGNATURE";
        case W2F_ERR_BAD_LENGTH: return "W2F_ERR_BAD_LENGTH";
        case W2F_ERR_TOO_MANY_TABLES: return "W2F_ERR_TOO_MANY_TABLES";
        case W2F_ERR_BAD_BASE128: return "W2F_ERR_BAD_BASE128";
        case W2F_ERR_UNSUPPORTED_COLLECTION: return "W2F_ERR_UNSUPPORTED_COLLECTION";
        case W2F_ERR_UNSUPPORTED_TRANSFORM: return "W2F_ERR_UNSUPPORTED_TRANSFORM";
        case W2F_ERR_SIZE_OVERFLOW: return "W2F_ERR_SIZE_OVERFLOW";
        case W2F_ERR_OUTPUT_TOO_SMALL: return "W2F_ERR_OUTPUT_TOO_SMALL";
        case W2F_ERR_SCRATCH_TOO_SMALL: return "W2F_ERR_SCRATCH_TOO_SMALL";
        case W2F_ERR_BROTLI_UNAVAILABLE: return "W2F_ERR_BROTLI_UNAVAILABLE";
        case W2F_ERR_BROTLI_FAILED: return "W2F_ERR_BROTLI_FAILED";
        case W2F_ERR_BAD_TABLE_STREAM: return "W2F_ERR_BAD_TABLE_STREAM";
        case W2F_ERR_BAD_TABLE_ORDER: return "W2F_ERR_BAD_TABLE_ORDER";
        case W2F_ERR_BAD_HEAD: return "W2F_ERR_BAD_HEAD";
        case W2F_ERR_INTERNAL: return "W2F_ERR_INTERNAL";
        case W2F_ERR_BAD_GLYF_TRANSFORM: return "W2F_ERR_BAD_GLYF_TRANSFORM";
        case W2F_ERR_BAD_LOCA_TRANSFORM: return "W2F_ERR_BAD_LOCA_TRANSFORM";
        case W2F_ERR_BAD_HMTX_TRANSFORM: return "W2F_ERR_BAD_HMTX_TRANSFORM";
        default: return "W2F_ERR_UNKNOWN";
    }
}

int w2f_probe_woff2(const W2F_U8 *woff2,
                    W2F_U32 woff2_len,
                    W2F_DecoderInfo *info_out,
                    W2F_TableRec *tables,
                    W2F_U16 table_cap)
{
    W2F_DecoderInfo info;
    W2F_U32 pos;
    W2F_U32 sig;
    W2F_U16 i;
    W2F_U8 flags;
    W2F_U8 tag_index;
    W2F_U8 tv;
    W2F_U32 tag;
    W2F_U32 orig_len;
    W2F_U32 trans_len;
    W2F_U32 stream_total;
    int rc;

    if (woff2 == 0 || info_out == 0) return W2F_ERR_NULL_ARG;
    if (woff2_len < 48u) return W2F_ERR_TRUNCATED;

    sig = w2f_rd32(woff2 + 0u);
    if (sig != W2F_WOF2) return W2F_ERR_BAD_SIGNATURE;

    info.flavor = w2f_rd32(woff2 + 4u);
    info.declared_length = w2f_rd32(woff2 + 8u);
    info.num_tables = w2f_rd16(woff2 + 12u);
    info.total_sfnt_size = w2f_rd32(woff2 + 16u);
    info.total_compressed_size = w2f_rd32(woff2 + 20u);
    info.major_version = w2f_rd16(woff2 + 24u);
    info.minor_version = w2f_rd16(woff2 + 26u);
    info.meta_offset = w2f_rd32(woff2 + 28u);
    info.meta_length = w2f_rd32(woff2 + 32u);
    info.meta_orig_length = w2f_rd32(woff2 + 36u);
    info.priv_offset = w2f_rd32(woff2 + 40u);
    info.priv_length = w2f_rd32(woff2 + 44u);
    info.compressed_offset = 0u;
    info.decompressed_table_bytes = 0u;
    info.is_collection = (W2F_U8)(info.flavor == W2F_TTCF ? 1u : 0u);
    info.has_glyf_loca_transform = 0u;
    info.reserved0 = 0u;
    info.reserved1 = 0u;

    if (info.declared_length != woff2_len) return W2F_ERR_BAD_LENGTH;
    if (info.num_tables == 0u) return W2F_ERR_BAD_LENGTH;
    if (info.num_tables > W2F_MAX_TABLES) return W2F_ERR_TOO_MANY_TABLES;
    if (tables != 0 && info.num_tables > table_cap) return W2F_ERR_TOO_MANY_TABLES;
    if (info.is_collection != 0u) return W2F_ERR_UNSUPPORTED_COLLECTION;

    pos = 48u;
    stream_total = 0u;

    for (i = 0u; i < info.num_tables; ++i) {
        if (pos >= woff2_len) return W2F_ERR_TRUNCATED;
        flags = woff2[pos++];
        tag_index = (W2F_U8)(flags & 0x3fu);
        tv = (W2F_U8)(flags >> 6);
        if (tag_index == 63u) {
            if (pos + 4u > woff2_len) return W2F_ERR_TRUNCATED;
            tag = w2f_rd32(woff2 + pos);
            pos += 4u;
        } else {
            tag = w2f_known_tags[tag_index];
        }

        rc = w2f_read_base128(woff2, woff2_len, &pos, &orig_len);
        if (rc != W2F_OK) return rc;
        trans_len = orig_len;
        if (w2f_table_needs_transform_len(tag, tv)) {
            rc = w2f_read_base128(woff2, woff2_len, &pos, &trans_len);
            if (rc != W2F_OK) return rc;
        }

        if (!w2f_transform_version_supported(tag, tv)) return W2F_ERR_UNSUPPORTED_TRANSFORM;
        if (tag == W2F_LOCA && tv == 0u && trans_len != 0u) return W2F_ERR_BAD_LOCA_TRANSFORM;
        if ((tag == W2F_GLYF || tag == W2F_LOCA) && tv == 0u) info.has_glyf_loca_transform = 1u;

        if (tables != 0) {
            tables[i].tag = tag;
            tables[i].orig_len = orig_len;
            tables[i].transform_len = trans_len;
            tables[i].final_len = orig_len;
            tables[i].src_off = stream_total;
            tables[i].dst_off = 0u;
            tables[i].checksum = 0u;
            tables[i].flags = flags;
            tables[i].transform_version = tv;
            tables[i].is_custom_tag = (W2F_U8)(tag_index == 63u ? 1u : 0u);
            tables[i].reserved = 0u;
        }

        if (w2f_add_overflow(stream_total, trans_len, &stream_total)) return W2F_ERR_SIZE_OVERFLOW;
    }

    info.decompressed_table_bytes = stream_total;
    info.compressed_offset = pos;
    if (w2f_add_overflow(pos, info.total_compressed_size, &stream_total)) return W2F_ERR_SIZE_OVERFLOW;
    if (stream_total > woff2_len) return W2F_ERR_TRUNCATED;

    if (info_out != 0) *info_out = info;
    return W2F_OK;
}

static int w2f_find_transformed_loca(W2F_TableRec *tables, W2F_U16 n, W2F_U16 glyf_i, W2F_U16 *loca_i)
{
    W2F_U16 i;
    for (i = (W2F_U16)(glyf_i + 1u); i < n; ++i) {
        if (tables[i].tag == W2F_LOCA) {
            if (tables[i].transform_version != 0u) return W2F_ERR_BAD_LOCA_TRANSFORM;
            *loca_i = i;
            return W2F_OK;
        }
    }
    return W2F_ERR_BAD_LOCA_TRANSFORM;
}

static int w2f_prepare_final_lengths(const W2F_DecoderInfo *info,
                                     W2F_TableRec *tables,
                                     const W2F_U8 *table_block,
                                     W2F_U32 table_block_len)
{
    W2F_U16 i;
    W2F_U16 li;
    W2F_U32 glyf_len;
    W2F_U32 loca_len;
    W2F_U16 num_glyphs;
    W2F_U16 index_format;
    W2F_U16 num_hmetrics;
    W2F_U16 gi;
    int rc;
    int saw_glyf_x;
    int saw_loca_x;

    (void)info;
    saw_glyf_x = 0;
    saw_loca_x = 0;
    for (i = 0u; i < info->num_tables; ++i) {
        tables[i].final_len = tables[i].orig_len;
        if (tables[i].src_off + tables[i].transform_len < tables[i].src_off) return W2F_ERR_SIZE_OVERFLOW;
        if (tables[i].src_off + tables[i].transform_len > table_block_len) return W2F_ERR_BAD_TABLE_STREAM;
        if (tables[i].tag == W2F_GLYF && tables[i].transform_version == 0u) {
            saw_glyf_x = 1;
            rc = w2f_find_transformed_loca(tables, info->num_tables, i, &li);
            if (rc != W2F_OK) return rc;
            rc = w2f_decode_glyf_transform(table_block + tables[i].src_off,
                                           tables[i].transform_len,
                                           0, 0u, &glyf_len,
                                           0, 0u, &loca_len,
                                           &num_glyphs, &index_format);
            if (rc != W2F_OK) return rc;
            (void)num_glyphs;
            (void)index_format;
            if (tables[li].orig_len != loca_len) return W2F_ERR_BAD_LOCA_TRANSFORM;
            tables[i].final_len = glyf_len;
            tables[li].final_len = loca_len;
        }
        if (tables[i].tag == W2F_LOCA && tables[i].transform_version == 0u) saw_loca_x = 1;
    }
    if (saw_glyf_x != saw_loca_x) return W2F_ERR_BAD_LOCA_TRANSFORM;

    for (i = 0u; i < info->num_tables; ++i) {
        if (tables[i].tag == W2F_HMTX && tables[i].transform_version == 1u) {
            rc = w2f_find_table_index(tables, info->num_tables, W2F_GLYF, &gi);
            if (rc != W2F_OK) return W2F_ERR_BAD_HMTX_TRANSFORM;
            (void)gi;
            rc = w2f_read_metric_counts_from_stream(tables, info->num_tables, table_block, table_block_len, &num_glyphs, &num_hmetrics);
            if (rc != W2F_OK) return rc;
            rc = w2f_check_hmtx_transform(table_block + tables[i].src_off,
                                          tables[i].transform_len,
                                          tables[i].orig_len,
                                          num_glyphs,
                                          num_hmetrics);
            if (rc != W2F_OK) return rc;
            tables[i].final_len = tables[i].orig_len;
        }
    }
    return W2F_OK;
}

static int w2f_build_sfnt(const W2F_DecoderInfo *info,
                          W2F_TableRec *tables,
                          const W2F_U8 *table_block,
                          W2F_U32 table_block_len,
                          W2F_U8 *out,
                          W2F_U32 out_cap,
                          W2F_U32 *out_len)
{
    W2F_U32 needed;
    W2F_U32 data_pos;
    W2F_U32 tmp;
    W2F_U16 i;
    W2F_U16 indices[W2F_MAX_TABLES];
    W2F_U16 search_range, entry_selector, range_shift;
    W2F_U32 head_off;
    int have_head;
    W2F_U32 whole_sum;
    W2F_U32 adjust;
    int rc;
    W2F_U16 li;
    W2F_U32 glyf_len;
    W2F_U32 loca_len;
    W2F_U32 hmtx_len;
    W2F_U16 gi;
    W2F_U16 lci;
    W2F_U16 num_glyphs;
    W2F_U16 num_hmetrics;
    W2F_U16 loca_format;

    if (info == 0 || tables == 0 || table_block == 0 || out == 0 || out_len == 0) return W2F_ERR_NULL_ARG;

    rc = w2f_prepare_final_lengths(info, tables, table_block, table_block_len);
    if (rc != W2F_OK) return rc;

    needed = 12u;
    tmp = ((W2F_U32)info->num_tables) * 16u;
    if (w2f_add_overflow(needed, tmp, &needed)) return W2F_ERR_SIZE_OVERFLOW;
    data_pos = needed;

    for (i = 0u; i < info->num_tables; ++i) {
        tables[i].dst_off = data_pos;
        if (w2f_add_overflow(data_pos, w2f_round4(tables[i].final_len), &data_pos)) return W2F_ERR_SIZE_OVERFLOW;
    }

    needed = data_pos;
    if (out_cap < needed) return W2F_ERR_OUTPUT_TOO_SMALL;

    w2f_zero(out, needed);
    w2f_sfnt_search_params(info->num_tables, &search_range, &entry_selector, &range_shift);
    w2f_wr32(out + 0u, info->flavor);
    w2f_wr16(out + 4u, info->num_tables);
    w2f_wr16(out + 6u, search_range);
    w2f_wr16(out + 8u, entry_selector);
    w2f_wr16(out + 10u, range_shift);

    for (i = 0u; i < info->num_tables; ++i) {
        if (tables[i].tag == W2F_GLYF && tables[i].transform_version == 0u) {
            rc = w2f_find_transformed_loca(tables, info->num_tables, i, &li);
            if (rc != W2F_OK) return rc;
            rc = w2f_decode_glyf_transform(table_block + tables[i].src_off,
                                           tables[i].transform_len,
                                           out + tables[i].dst_off,
                                           tables[i].final_len,
                                           &glyf_len,
                                           out + tables[li].dst_off,
                                           tables[li].final_len,
                                           &loca_len,
                                           0, 0);
            if (rc != W2F_OK) return rc;
            if (glyf_len != tables[i].final_len || loca_len != tables[li].final_len) return W2F_ERR_INTERNAL;
        } else if (tables[i].tag == W2F_LOCA && tables[i].transform_version == 0u) {
        } else if (tables[i].tag == W2F_HMTX && tables[i].transform_version == 1u) {
        } else {
            if (tables[i].transform_len != tables[i].final_len) return W2F_ERR_INTERNAL;
            if (tables[i].src_off + tables[i].transform_len > table_block_len) return W2F_ERR_BAD_TABLE_STREAM;
            w2f_copy(out + tables[i].dst_off, table_block + tables[i].src_off, tables[i].final_len);
        }
    }

    for (i = 0u; i < info->num_tables; ++i) {
        if (tables[i].tag == W2F_HMTX && tables[i].transform_version == 1u) {
            rc = w2f_read_metric_counts_from_stream(tables, info->num_tables, table_block, table_block_len, &num_glyphs, &num_hmetrics);
            if (rc != W2F_OK) return rc;
            rc = w2f_find_table_index(tables, info->num_tables, W2F_GLYF, &gi);
            if (rc != W2F_OK) return W2F_ERR_BAD_HMTX_TRANSFORM;
            rc = w2f_find_table_index(tables, info->num_tables, W2F_LOCA, &lci);
            if (rc != W2F_OK) return W2F_ERR_BAD_HMTX_TRANSFORM;
            rc = w2f_infer_loca_format(tables[lci].final_len, num_glyphs, &loca_format);
            if (rc != W2F_OK) return rc;
            rc = w2f_decode_hmtx_transform(table_block + tables[i].src_off,
                                           tables[i].transform_len,
                                           out + tables[i].dst_off,
                                           tables[i].final_len,
                                           num_glyphs,
                                           num_hmetrics,
                                           out + tables[gi].dst_off,
                                           tables[gi].final_len,
                                           out + tables[lci].dst_off,
                                           tables[lci].final_len,
                                           loca_format,
                                           &hmtx_len);
            if (rc != W2F_OK) return rc;
            if (hmtx_len != tables[i].final_len) return W2F_ERR_INTERNAL;
        }
    }

    head_off = 0u;
    have_head = 0;
    for (i = 0u; i < info->num_tables; ++i) {
        if (tables[i].tag == W2F_HEAD) {
            if (tables[i].final_len < 12u) return W2F_ERR_BAD_HEAD;
            head_off = tables[i].dst_off;
            have_head = 1;
            out[head_off + 8u] = 0u;
            out[head_off + 9u] = 0u;
            out[head_off + 10u] = 0u;
            out[head_off + 11u] = 0u;
        }
    }

    for (i = 0u; i < info->num_tables; ++i) {
        tables[i].checksum = w2f_table_checksum(out + tables[i].dst_off, tables[i].final_len);
    }

    w2f_sort_indices_by_tag(tables, info->num_tables, indices);
    for (i = 0u; i < info->num_tables; ++i) {
        W2F_U16 ti;
        W2F_U32 rec;
        ti = indices[i];
        rec = 12u + ((W2F_U32)i) * 16u;
        w2f_wr32(out + rec + 0u, tables[ti].tag);
        w2f_wr32(out + rec + 4u, tables[ti].checksum);
        w2f_wr32(out + rec + 8u, tables[ti].dst_off);
        w2f_wr32(out + rec + 12u, tables[ti].final_len);
    }

    if (have_head) {
        whole_sum = w2f_table_checksum(out, needed);
        adjust = (W2F_U32)(0xB1B0AFBAul - whole_sum);
        w2f_wr32(out + head_off + 8u, adjust);
    }

    *out_len = needed;
    return W2F_OK;
}

int w2f_decode_woff2_to_sfnt(const W2F_U8 *woff2,
                             W2F_U32 woff2_len,
                             W2F_U8 *sfnt_out,
                             W2F_U32 sfnt_cap,
                             W2F_U32 *sfnt_len,
                             W2F_U8 *scratch,
                             W2F_U32 scratch_cap,
                             W2F_BrotliDecodeFn brotli_decode,
                             W2F_DecoderInfo *info_out)
{
    W2F_DecoderInfo info;
    W2F_TableRec tables[W2F_MAX_TABLES];
    W2F_U32 scratch_len;
    int rc;

    if (woff2 == 0 || sfnt_out == 0 || sfnt_len == 0 || scratch == 0 || brotli_decode == 0) return W2F_ERR_NULL_ARG;
    *sfnt_len = 0u;

    rc = w2f_probe_woff2(woff2, woff2_len, &info, tables, (W2F_U16)W2F_MAX_TABLES);
    if (rc != W2F_OK) return rc;

    if (scratch_cap < info.decompressed_table_bytes) return W2F_ERR_SCRATCH_TOO_SMALL;

    scratch_len = 0u;
    rc = brotli_decode(woff2 + info.compressed_offset,
                       info.total_compressed_size,
                       scratch,
                       scratch_cap,
                       &scratch_len);
    if (rc != W2F_OK) return rc;
    if (scratch_len != info.decompressed_table_bytes) return W2F_ERR_BAD_TABLE_STREAM;

    rc = w2f_build_sfnt(&info, tables, scratch, scratch_len, sfnt_out, sfnt_cap, sfnt_len);
    if (rc != W2F_OK) return rc;

    if (info_out != 0) *info_out = info;
    return W2F_OK;
}
