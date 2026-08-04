#include "zlib89.h"
#include <string.h>

typedef struct Zlib89BitReader {
    const zlib89_u8 *src;
    zlib89_u32 size;
    zlib89_u32 pos;
    zlib89_u32 hold;
    unsigned bits;
} Zlib89BitReader;

typedef struct Zlib89InflateState {
    Zlib89BitReader br;
    zlib89_u8 *dst;
    zlib89_u32 dst_size;
    zlib89_u32 out_pos;
    Zlib89Scratch *scratch;
} Zlib89InflateState;

static const zlib89_u16 zlib89_len_base[29] = {
    3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u,
    11u, 13u, 15u, 17u,
    19u, 23u, 27u, 31u,
    35u, 43u, 51u, 59u,
    67u, 83u, 99u, 115u,
    131u, 163u, 195u, 227u,
    258u
};

static const zlib89_u8 zlib89_len_extra[29] = {
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u,
    1u, 1u, 1u, 1u,
    2u, 2u, 2u, 2u,
    3u, 3u, 3u, 3u,
    4u, 4u, 4u, 4u,
    5u, 5u, 5u, 5u,
    0u
};

static const zlib89_u16 zlib89_dist_base[30] = {
    1u, 2u, 3u, 4u, 5u, 7u, 9u, 13u, 17u, 25u,
    33u, 49u, 65u, 97u, 129u, 193u, 257u, 385u, 513u, 769u,
    1025u, 1537u, 2049u, 3073u, 4097u, 6145u, 8193u, 12289u,
    16385u, 24577u
};

static const zlib89_u8 zlib89_dist_extra[30] = {
    0u, 0u, 0u, 0u, 1u, 1u, 2u, 2u, 3u, 3u,
    4u, 4u, 5u, 5u, 6u, 6u, 7u, 7u, 8u, 8u,
    9u, 9u, 10u, 10u, 11u, 11u, 12u, 12u, 13u, 13u
};

static const zlib89_u8 zlib89_cl_order[19] = {
    16u, 17u, 18u, 0u, 8u, 7u, 9u, 6u, 10u, 5u, 11u, 4u, 12u, 3u, 13u, 2u, 14u, 1u, 15u
};

static int zlib89_add_u32(zlib89_u32 a, zlib89_u32 b, zlib89_u32 *out)
{
    if (!out) return 0;
    if (a > (zlib89_u32)(~(zlib89_u32)0) - b) return 0;
    *out = a + b;
    return 1;
}

static zlib89_u32 zlib89_adler32(const zlib89_u8 *buf, zlib89_u32 len)
{
    zlib89_u32 s1 = 1u;
    zlib89_u32 s2 = 0u;
    zlib89_u32 i;
    for (i = 0u; i < len; ++i) {
        s1 += (zlib89_u32)buf[i];
        if (s1 >= 65521u) s1 -= 65521u;
        s2 += s1;
        s2 %= 65521u;
    }
    return (s2 << 16) | s1;
}

static void zlib89_br_init(Zlib89BitReader *br, const zlib89_u8 *src, zlib89_u32 size)
{
    br->src = src;
    br->size = size;
    br->pos = 0u;
    br->hold = 0u;
    br->bits = 0u;
}

static int zlib89_br_ensure(Zlib89BitReader *br, unsigned need)
{
    while (br->bits < need) {
        if (br->pos >= br->size) return 0;
        br->hold |= (zlib89_u32)br->src[br->pos++] << br->bits;
        br->bits += 8u;
    }
    return 1;
}

static int zlib89_br_read(Zlib89BitReader *br, unsigned nbits, zlib89_u32 *out)
{
    zlib89_u32 mask;
    if (!out) return 0;
    if (nbits > 24u) return 0;
    if (!zlib89_br_ensure(br, nbits)) return 0;
    if (nbits == 32u) mask = 0xFFFFFFFFul;
    else if (nbits == 0u) mask = 0u;
    else mask = ((zlib89_u32)1u << nbits) - 1u;
    *out = br->hold & mask;
    br->hold >>= nbits;
    br->bits -= nbits;
    return 1;
}

static int zlib89_br_read_bit(Zlib89BitReader *br, zlib89_u32 *out)
{
    return zlib89_br_read(br, 1u, out);
}

static void zlib89_br_align_byte(Zlib89BitReader *br)
{
    unsigned drop;
    drop = br->bits & 7u;
    br->hold >>= drop;
    br->bits -= drop;
}

static int zlib89_br_read_u16le(Zlib89BitReader *br, zlib89_u16 *out)
{
    zlib89_u32 lo;
    zlib89_u32 hi;
    if (!out) return 0;
    zlib89_br_align_byte(br);
    if (!zlib89_br_read(br, 8u, &lo)) return 0;
    if (!zlib89_br_read(br, 8u, &hi)) return 0;
    *out = (zlib89_u16)(lo | (hi << 8));
    return 1;
}

static zlib89_u32 zlib89_rd_be32(const zlib89_u8 *p)
{
    return ((zlib89_u32)p[0] << 24) | ((zlib89_u32)p[1] << 16) | ((zlib89_u32)p[2] << 8) | (zlib89_u32)p[3];
}

static zlib89_u32 zlib89_rev_bits(zlib89_u32 code, unsigned len)
{
    zlib89_u32 out = 0u;
    unsigned i;
    for (i = 0u; i < len; ++i) {
        out = (out << 1) | (code & 1u);
        code >>= 1;
    }
    return out;
}

static void zlib89_huff_init(Zlib89Huff *h)
{
    zlib89_u32 i;
    if (!h) return;
    for (i = 0u; i < ZLIB89_MAX_TREE_NODES; ++i) {
        h->left[i] = -1;
        h->right[i] = -1;
        h->sym[i] = -1;
    }
    h->nodes_used = 1;
    h->used_symbols = 0;
}

static int zlib89_huff_new_node(Zlib89Huff *h)
{
    int idx;
    if (!h) return -1;
    if ((zlib89_u32)h->nodes_used >= ZLIB89_MAX_TREE_NODES) return -1;
    idx = (int)h->nodes_used;
    h->left[idx] = -1;
    h->right[idx] = -1;
    h->sym[idx] = -1;
    ++h->nodes_used;
    return idx;
}

static int zlib89_huff_insert(Zlib89Huff *h, zlib89_u32 rev_code, unsigned len, int sym)
{
    int node;
    unsigned i;
    node = 0;
    for (i = 0u; i < len; ++i) {
        short *child;
        int next;
        if ((rev_code >> i) & 1u) child = &h->right[node];
        else child = &h->left[node];
        if (i + 1u == len) {
            if (*child != -1) return 0;
            next = zlib89_huff_new_node(h);
            if (next < 0) return 0;
            h->sym[next] = (short)sym;
            *child = next;
            return 1;
        }
        if (*child == -1) {
            next = zlib89_huff_new_node(h);
            if (next < 0) return 0;
            *child = next;
        }
        node = *child;
        if (h->sym[node] >= 0) return 0;
    }
    return 0;
}

static int zlib89_huff_build(Zlib89Huff *h, const zlib89_u8 *lengths, zlib89_u32 count)
{
    unsigned bl_count[ZLIB89_MAX_BITS + 1u];
    unsigned next_code[ZLIB89_MAX_BITS + 1u];
    unsigned bits;
    unsigned code;
    zlib89_u32 i;
    int left;
    int used;

    if (!h || !lengths) return ZLIB89_EINVAL;
    zlib89_huff_init(h);

    for (bits = 0u; bits <= ZLIB89_MAX_BITS; ++bits) bl_count[bits] = 0u;
    used = 0;
    for (i = 0u; i < count; ++i) {
        zlib89_u8 len;
        len = lengths[i];
        if (len > ZLIB89_MAX_BITS) return ZLIB89_EFORMAT;
        if (len != 0u) {
            ++bl_count[len];
            ++used;
        }
    }
    h->used_symbols = (short)used;
    if (used == 0) return ZLIB89_OK;

    left = 1;
    for (bits = 1u; bits <= ZLIB89_MAX_BITS; ++bits) {
        left = (left << 1) - (int)bl_count[bits];
        if (left < 0) return ZLIB89_EFORMAT;
    }

    next_code[0] = 0u;
    code = 0u;
    for (bits = 1u; bits <= ZLIB89_MAX_BITS; ++bits) {
        code = (code + bl_count[bits - 1u]) << 1;
        next_code[bits] = code;
    }

    for (i = 0u; i < count; ++i) {
        zlib89_u8 len;
        zlib89_u32 canon;
        zlib89_u32 rev;
        len = lengths[i];
        if (len == 0u) continue;
        canon = (zlib89_u32)next_code[len]++;
        rev = zlib89_rev_bits(canon, (unsigned)len);
        if (!zlib89_huff_insert(h, rev, (unsigned)len, (int)i)) return ZLIB89_EFORMAT;
    }

    return ZLIB89_OK;
}

static int zlib89_huff_decode(Zlib89BitReader *br, const Zlib89Huff *h, unsigned *out_sym)
{
    int node;
    zlib89_u32 bit;
    if (!br || !h || !out_sym) return ZLIB89_EINVAL;
    if (h->used_symbols == 0) return ZLIB89_EFORMAT;
    node = 0;
    for (;;) {
        if (!zlib89_br_read_bit(br, &bit)) return ZLIB89_ETRUNCATED;
        node = bit ? h->right[node] : h->left[node];
        if (node < 0) return ZLIB89_EFORMAT;
        if (h->sym[node] >= 0) {
            *out_sym = (unsigned)h->sym[node];
            return ZLIB89_OK;
        }
    }
}

static int zlib89_emit_byte(Zlib89InflateState *st, zlib89_u8 b)
{
    if (!st) return ZLIB89_EINVAL;
    if (st->out_pos >= st->dst_size) return ZLIB89_ENOSPC;
    st->dst[st->out_pos++] = b;
    return ZLIB89_OK;
}

static int zlib89_copy_match(Zlib89InflateState *st, zlib89_u32 dist, zlib89_u32 len)
{
    zlib89_u32 i;
    if (!st) return ZLIB89_EINVAL;
    if (dist == 0u || dist > st->out_pos) return ZLIB89_EFORMAT;
    if (len > st->dst_size - st->out_pos) return ZLIB89_ENOSPC;
    for (i = 0u; i < len; ++i) {
        st->dst[st->out_pos] = st->dst[st->out_pos - dist];
        ++st->out_pos;
    }
    return ZLIB89_OK;
}

static int zlib89_build_fixed_trees(Zlib89Scratch *scratch)
{
    zlib89_u32 i;
    int rc;
    if (!scratch) return ZLIB89_EINVAL;
    for (i = 0u; i < ZLIB89_MAX_LITLEN_SYMS; ++i) {
        if (i <= 143u) scratch->ll_lengths[i] = 8u;
        else if (i <= 255u) scratch->ll_lengths[i] = 9u;
        else if (i <= 279u) scratch->ll_lengths[i] = 7u;
        else scratch->ll_lengths[i] = 8u;
    }
    for (i = 0u; i < ZLIB89_MAX_DIST_SYMS; ++i) scratch->dist_lengths[i] = 5u;
    rc = zlib89_huff_build(&scratch->litlen, scratch->ll_lengths, ZLIB89_MAX_LITLEN_SYMS);
    if (rc != ZLIB89_OK) return rc;
    rc = zlib89_huff_build(&scratch->dist, scratch->dist_lengths, ZLIB89_MAX_DIST_SYMS);
    if (rc != ZLIB89_OK) return rc;
    return ZLIB89_OK;
}

static int zlib89_read_dynamic_trees(Zlib89InflateState *st)
{
    zlib89_u32 hlit;
    zlib89_u32 hdist;
    zlib89_u32 hclen;
    zlib89_u32 i;
    zlib89_u32 total;
    zlib89_u32 idx;
    int rc;

    if (!st || !st->scratch) return ZLIB89_EINVAL;
    if (!zlib89_br_read(&st->br, 5u, &hlit)) return ZLIB89_ETRUNCATED;
    if (!zlib89_br_read(&st->br, 5u, &hdist)) return ZLIB89_ETRUNCATED;
    if (!zlib89_br_read(&st->br, 4u, &hclen)) return ZLIB89_ETRUNCATED;
    hlit += 257u;
    hdist += 1u;
    hclen += 4u;
    if (hlit > ZLIB89_MAX_LITLEN_SYMS || hdist > ZLIB89_MAX_DIST_SYMS) return ZLIB89_EFORMAT;

    for (i = 0u; i < ZLIB89_MAX_CL_SYMS; ++i) st->scratch->code_lengths[i] = 0u;
    for (i = 0u; i < hclen; ++i) {
        zlib89_u32 v;
        if (!zlib89_br_read(&st->br, 3u, &v)) return ZLIB89_ETRUNCATED;
        st->scratch->code_lengths[zlib89_cl_order[i]] = (zlib89_u8)v;
    }
    rc = zlib89_huff_build(&st->scratch->code, st->scratch->code_lengths, ZLIB89_MAX_CL_SYMS);
    if (rc != ZLIB89_OK) return rc;

    total = hlit + hdist;
    if (total > (ZLIB89_MAX_LITLEN_SYMS + ZLIB89_MAX_DIST_SYMS)) return ZLIB89_EFORMAT;
    for (i = 0u; i < ZLIB89_MAX_LITLEN_SYMS; ++i) st->scratch->ll_lengths[i] = 0u;
    for (i = 0u; i < ZLIB89_MAX_DIST_SYMS; ++i) st->scratch->dist_lengths[i] = 0u;

    idx = 0u;
    while (idx < total) {
        unsigned sym;
        rc = zlib89_huff_decode(&st->br, &st->scratch->code, &sym);
        if (rc != ZLIB89_OK) return rc;
        if (sym <= 15u) {
            if (idx < hlit) st->scratch->ll_lengths[idx] = (zlib89_u8)sym;
            else st->scratch->dist_lengths[idx - hlit] = (zlib89_u8)sym;
            ++idx;
        } else if (sym == 16u) {
            zlib89_u32 extra;
            zlib89_u8 prev;
            zlib89_u32 rep;
            if (idx == 0u) return ZLIB89_EFORMAT;
            if (!zlib89_br_read(&st->br, 2u, &extra)) return ZLIB89_ETRUNCATED;
            rep = extra + 3u;
            if (idx > hlit) prev = st->scratch->dist_lengths[idx - hlit - 1u];
            else if (idx == hlit) prev = st->scratch->ll_lengths[hlit - 1u];
            else prev = st->scratch->ll_lengths[idx - 1u];
            while (rep--) {
                if (idx >= total) return ZLIB89_EFORMAT;
                if (idx < hlit) st->scratch->ll_lengths[idx] = prev;
                else st->scratch->dist_lengths[idx - hlit] = prev;
                ++idx;
            }
        } else if (sym == 17u) {
            zlib89_u32 extra;
            zlib89_u32 rep;
            if (!zlib89_br_read(&st->br, 3u, &extra)) return ZLIB89_ETRUNCATED;
            rep = extra + 3u;
            while (rep--) {
                if (idx >= total) return ZLIB89_EFORMAT;
                if (idx < hlit) st->scratch->ll_lengths[idx] = 0u;
                else st->scratch->dist_lengths[idx - hlit] = 0u;
                ++idx;
            }
        } else if (sym == 18u) {
            zlib89_u32 extra;
            zlib89_u32 rep;
            if (!zlib89_br_read(&st->br, 7u, &extra)) return ZLIB89_ETRUNCATED;
            rep = extra + 11u;
            while (rep--) {
                if (idx >= total) return ZLIB89_EFORMAT;
                if (idx < hlit) st->scratch->ll_lengths[idx] = 0u;
                else st->scratch->dist_lengths[idx - hlit] = 0u;
                ++idx;
            }
        } else {
            return ZLIB89_EFORMAT;
        }
    }

    rc = zlib89_huff_build(&st->scratch->litlen, st->scratch->ll_lengths, hlit);
    if (rc != ZLIB89_OK) return rc;
    rc = zlib89_huff_build(&st->scratch->dist, st->scratch->dist_lengths, hdist);
    if (rc != ZLIB89_OK) return rc;
    return ZLIB89_OK;
}

static int zlib89_decode_compressed_block(Zlib89InflateState *st)
{
    int rc;
    if (!st || !st->scratch) return ZLIB89_EINVAL;
    for (;;) {
        unsigned sym;
        rc = zlib89_huff_decode(&st->br, &st->scratch->litlen, &sym);
        if (rc != ZLIB89_OK) return rc;
        if (sym < 256u) {
            rc = zlib89_emit_byte(st, (zlib89_u8)sym);
            if (rc != ZLIB89_OK) return rc;
        } else if (sym == 256u) {
            return ZLIB89_OK;
        } else if (sym <= 285u) {
            zlib89_u32 len;
            zlib89_u32 dist;
            zlib89_u32 extra;
            unsigned dsym;
            sym -= 257u;
            len = (zlib89_u32)zlib89_len_base[sym];
            extra = (zlib89_u32)zlib89_len_extra[sym];
            if (extra != 0u) {
                zlib89_u32 v;
                if (!zlib89_br_read(&st->br, (unsigned)extra, &v)) return ZLIB89_ETRUNCATED;
                if (!zlib89_add_u32(len, v, &len)) return ZLIB89_ERANGE;
            }
            if (st->scratch->dist.used_symbols == 0) return ZLIB89_EFORMAT;
            rc = zlib89_huff_decode(&st->br, &st->scratch->dist, &dsym);
            if (rc != ZLIB89_OK) return rc;
            if (dsym >= 30u) return ZLIB89_EFORMAT;
            dist = (zlib89_u32)zlib89_dist_base[dsym];
            extra = (zlib89_u32)zlib89_dist_extra[dsym];
            if (extra != 0u) {
                zlib89_u32 v2;
                if (!zlib89_br_read(&st->br, (unsigned)extra, &v2)) return ZLIB89_ETRUNCATED;
                if (!zlib89_add_u32(dist, v2, &dist)) return ZLIB89_ERANGE;
            }
            rc = zlib89_copy_match(st, dist, len);
            if (rc != ZLIB89_OK) return rc;
        } else {
            return ZLIB89_EFORMAT;
        }
    }
}

static int zlib89_decode_stored_block(Zlib89InflateState *st)
{
    zlib89_u16 len;
    zlib89_u16 nlen;
    zlib89_u32 i;
    zlib89_u32 b;
    if (!st) return ZLIB89_EINVAL;
    if (!zlib89_br_read_u16le(&st->br, &len)) return ZLIB89_ETRUNCATED;
    if (!zlib89_br_read_u16le(&st->br, &nlen)) return ZLIB89_ETRUNCATED;
    if ((zlib89_u16)(len ^ 0xFFFFu) != nlen) return ZLIB89_EFORMAT;
    if ((zlib89_u32)len > st->dst_size - st->out_pos) return ZLIB89_ENOSPC;
    for (i = 0u; i < (zlib89_u32)len; ++i) {
        if (!zlib89_br_read(&st->br, 8u, &b)) return ZLIB89_ETRUNCATED;
        st->dst[st->out_pos++] = (zlib89_u8)b;
    }
    return ZLIB89_OK;
}

static int zlib89_inflate_deflate(Zlib89InflateState *st)
{
    int final_block;
    int rc;
    if (!st || !st->scratch) return ZLIB89_EINVAL;
    final_block = 0;
    while (!final_block) {
        zlib89_u32 final_bit;
        zlib89_u32 btype;
        if (!zlib89_br_read(&st->br, 1u, &final_bit)) return ZLIB89_ETRUNCATED;
        if (!zlib89_br_read(&st->br, 2u, &btype)) return ZLIB89_ETRUNCATED;
        final_block = (int)final_bit;
        if (btype == 0u) {
            rc = zlib89_decode_stored_block(st);
            if (rc != ZLIB89_OK) return rc;
        } else if (btype == 1u) {
            rc = zlib89_build_fixed_trees(st->scratch);
            if (rc != ZLIB89_OK) return rc;
            rc = zlib89_decode_compressed_block(st);
            if (rc != ZLIB89_OK) return rc;
        } else if (btype == 2u) {
            rc = zlib89_read_dynamic_trees(st);
            if (rc != ZLIB89_OK) return rc;
            rc = zlib89_decode_compressed_block(st);
            if (rc != ZLIB89_OK) return rc;
        } else {
            return ZLIB89_EFORMAT;
        }
    }
    return ZLIB89_OK;
}

int zlib89_inflate_raw(const zlib89_u8 *src, zlib89_u32 src_size,
                       zlib89_u8 *dst, zlib89_u32 dst_size,
                       Zlib89Scratch *scratch,
                       zlib89_u32 *out_written)
{
    Zlib89InflateState st;
    int rc;

    if (out_written) *out_written = 0u;
    if (!src || !dst || !scratch) return ZLIB89_EINVAL;

    zlib89_br_init(&st.br, src, src_size);
    st.dst = dst;
    st.dst_size = dst_size;
    st.out_pos = 0u;
    st.scratch = scratch;

    rc = zlib89_inflate_deflate(&st);
    if (rc != ZLIB89_OK) return rc;
    if (out_written) *out_written = st.out_pos;
    return ZLIB89_OK;
}

int zlib89_inflate_zlib(const zlib89_u8 *src, zlib89_u32 src_size,
                        zlib89_u8 *dst, zlib89_u32 dst_size,
                        zlib89_u32 flags,
                        Zlib89Scratch *scratch,
                        zlib89_u32 *out_written,
                        Zlib89Info *out_info)
{
    Zlib89InflateState st;
    int rc;
    zlib89_u8 cmf;
    zlib89_u8 flg;
    zlib89_u16 hdr;
    zlib89_u32 expected;
    zlib89_u32 actual;

    if (out_written) *out_written = 0u;
    if (out_info) memset(out_info, 0, sizeof(*out_info));
    if (!src || !dst || !scratch) return ZLIB89_EINVAL;
    if (src_size < 6u) return ZLIB89_ETRUNCATED;

    cmf = src[0];
    flg = src[1];
    hdr = (zlib89_u16)(((zlib89_u16)cmf << 8) | (zlib89_u16)flg);
    if ((hdr % 31u) != 0u) return ZLIB89_EFORMAT;
    if ((cmf & 0x0Fu) != 8u) return ZLIB89_EUNSUPPORTED;
    if ((cmf >> 4) > 7u) return ZLIB89_EUNSUPPORTED;
    if ((flg & 0x20u) != 0u) return ZLIB89_EDICT;

    zlib89_br_init(&st.br, src + 2u, src_size - 6u);
    st.dst = dst;
    st.dst_size = dst_size;
    st.out_pos = 0u;
    st.scratch = scratch;

    rc = zlib89_inflate_deflate(&st);
    if (rc != ZLIB89_OK) return rc;

    expected = zlib89_rd_be32(src + src_size - 4u);
    actual = zlib89_adler32(dst, st.out_pos);
    if (!(flags & ZLIB89_FLAG_IGNORE_ADLER) && expected != actual) return ZLIB89_EADLER;

    if (!(flags & ZLIB89_FLAG_ALLOW_TRAILING)) {
        if (st.br.pos != st.br.size) return ZLIB89_EFORMAT;
    }

    if (out_written) *out_written = st.out_pos;
    if (out_info) {
        out_info->cmf = cmf;
        out_info->flg = flg;
        out_info->total_in = src_size;
        out_info->total_out = st.out_pos;
        out_info->adler_expected = expected;
        out_info->adler_actual = actual;
        out_info->used_dict = 0;
    }
    return ZLIB89_OK;
}
