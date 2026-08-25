#include "woff1_inflate.h"

#define WOFF1_MAX_BITS 15
#define WOFF1_MAX_LITLEN 288
#define WOFF1_MAX_DIST 32
#define WOFF1_MAX_CODELEN 19

struct woff1_bits_s {
    const woff1_u8 *src;
    woff1_u32 size;
    woff1_u32 pos;
    woff1_u32 bitbuf;
    int bitcnt;
};

struct woff1_huff_s {
    int num;
    int max_bits;
    int count[WOFF1_MAX_BITS + 1];
    int code[WOFF1_MAX_LITLEN];
    unsigned short sym[WOFF1_MAX_LITLEN];
    unsigned char len[WOFF1_MAX_LITLEN];
};

static const unsigned short woff1_len_base[29] = {
    3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
    35,43,51,59,67,83,99,115,131,163,195,227,258
};

static const unsigned char woff1_len_extra[29] = {
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
    3,3,3,3,4,4,4,4,5,5,5,5,0
};

static const unsigned short woff1_dist_base[30] = {
    1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
    257,385,513,769,1025,1537,2049,3073,4097,6145,
    8193,12289,16385,24577
};

static const unsigned char woff1_dist_extra[30] = {
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,
    7,7,8,8,9,9,10,10,11,11,12,12,13,13
};

static const unsigned char woff1_cl_order[19] = {
    16,17,18,0,8,7,9,6,10,5,11,4,12,3,13,2,14,1,15
};

static void woff1_bits_init(struct woff1_bits_s *br,
                            const woff1_u8 *src, woff1_u32 size)
{
    br->src = src;
    br->size = size;
    br->pos = 0;
    br->bitbuf = 0;
    br->bitcnt = 0;
}

static int woff1_bits_need(struct woff1_bits_s *br, int need)
{
    while (br->bitcnt < need) {
        if (br->pos >= br->size) {
            return WOFF1_INF_NEED_INPUT;
        }
        br->bitbuf |= ((woff1_u32)br->src[br->pos]) << br->bitcnt;
        br->pos++;
        br->bitcnt += 8;
    }
    return WOFF1_INF_OK;
}

static int woff1_bits_read(struct woff1_bits_s *br, int n, woff1_u32 *out)
{
    woff1_u32 mask;
    int r;

    if (n < 0 || n > 16) {
        return WOFF1_INF_BAD_LENGTH;
    }
    r = woff1_bits_need(br, n);
    if (r != WOFF1_INF_OK) {
        return r;
    }
    if (n == 0) {
        *out = 0;
        return WOFF1_INF_OK;
    }
    mask = (((woff1_u32)1u) << n) - 1u;
    *out = br->bitbuf & mask;
    br->bitbuf >>= n;
    br->bitcnt -= n;
    return WOFF1_INF_OK;
}

static void woff1_bits_align_byte(struct woff1_bits_s *br)
{
    int drop;
    drop = br->bitcnt & 7;
    if (drop != 0) {
        br->bitbuf >>= drop;
        br->bitcnt -= drop;
    }
}

static int woff1_bits_read_byte_aligned(struct woff1_bits_s *br, woff1_u8 *out)
{
    woff1_bits_align_byte(br);
    if (br->pos >= br->size) {
        return WOFF1_INF_NEED_INPUT;
    }
    *out = br->src[br->pos];
    br->pos++;
    return WOFF1_INF_OK;
}



static int woff1_huff_build(struct woff1_huff_s *h,
                            const unsigned char *lengths, int n)
{
    int bl_count[WOFF1_MAX_BITS + 1];
    int next_code[WOFF1_MAX_BITS + 1];
    int bits;
    int code;
    int i;
    int used;
    int len;
    int left;

    if (n <= 0 || n > WOFF1_MAX_LITLEN) {
        return WOFF1_INF_BAD_HUFFMAN;
    }

    for (i = 0; i <= WOFF1_MAX_BITS; ++i) {
        bl_count[i] = 0;
        h->count[i] = 0;
    }
    for (i = 0; i < n; ++i) {
        len = (int)lengths[i];
        if (len < 0 || len > WOFF1_MAX_BITS) {
            return WOFF1_INF_BAD_HUFFMAN;
        }
        if (len != 0) {
            bl_count[len]++;
        }
    }

    left = 1;
    for (bits = 1; bits <= WOFF1_MAX_BITS; ++bits) {
        left <<= 1;
        left -= bl_count[bits];
        if (left < 0) {
            return WOFF1_INF_BAD_HUFFMAN;
        }
    }

    code = 0;
    bl_count[0] = 0;
    for (bits = 1; bits <= WOFF1_MAX_BITS; ++bits) {
        code = (code + bl_count[bits - 1]) << 1;
        next_code[bits] = code;
    }

    used = 0;
    h->max_bits = 0;
    for (i = 0; i < n; ++i) {
        len = (int)lengths[i];
        if (len != 0) {
            h->code[used] = next_code[len];
            h->sym[used] = (unsigned short)i;
            h->len[used] = (unsigned char)len;
            h->count[len]++;
            if (len > h->max_bits) {
                h->max_bits = len;
            }
            next_code[len]++;
            used++;
        }
    }
    h->num = used;
    if (used == 0) {
        return WOFF1_INF_BAD_HUFFMAN;
    }
    return WOFF1_INF_OK;
}

static int woff1_huff_decode(struct woff1_bits_s *br,
                             const struct woff1_huff_s *h,
                             int *sym_out)
{
    int len;
    int i;
    int code;
    woff1_u32 bit;
    int r;

    code = 0;
    for (len = 1; len <= h->max_bits; ++len) {
        r = woff1_bits_read(br, 1, &bit);
        if (r != WOFF1_INF_OK) {
            return r;
        }
        code = (code << 1) | (int)bit;
        if (h->count[len] != 0) {
            for (i = 0; i < h->num; ++i) {
                if ((int)h->len[i] == len && h->code[i] == code) {
                    *sym_out = (int)h->sym[i];
                    return WOFF1_INF_OK;
                }
            }
        }
    }
    return WOFF1_INF_BAD_HUFFMAN;
}

static int woff1_make_fixed(struct woff1_huff_s *litlen,
                            struct woff1_huff_s *dist)
{
    unsigned char lens[WOFF1_MAX_LITLEN];
    unsigned char dlens[WOFF1_MAX_DIST];
    int i;
    int r;

    for (i = 0; i <= 143; ++i) {
        lens[i] = 8;
    }
    for (i = 144; i <= 255; ++i) {
        lens[i] = 9;
    }
    for (i = 256; i <= 279; ++i) {
        lens[i] = 7;
    }
    for (i = 280; i <= 287; ++i) {
        lens[i] = 8;
    }
    for (i = 0; i < WOFF1_MAX_DIST; ++i) {
        dlens[i] = 5;
    }
    r = woff1_huff_build(litlen, lens, 288);
    if (r != WOFF1_INF_OK) {
        return r;
    }
    return woff1_huff_build(dist, dlens, 32);
}

static int woff1_read_dynamic(struct woff1_bits_s *br,
                              struct woff1_huff_s *litlen,
                              struct woff1_huff_s *dist,
                              int *dist_empty)
{
    woff1_u32 v;
    int hlit;
    int hdist;
    int hclen;
    unsigned char cl_lens[WOFF1_MAX_CODELEN];
    unsigned char ll_lens[WOFF1_MAX_LITLEN];
    unsigned char d_lens[WOFF1_MAX_DIST];
    unsigned char all_lens[WOFF1_MAX_LITLEN + WOFF1_MAX_DIST];
    struct woff1_huff_s cl_huff;
    int i;
    int total;
    int sym;
    int repeat;
    int prev;
    int r;
    int nonzero_dist;

    r = woff1_bits_read(br, 5, &v);
    if (r != WOFF1_INF_OK) return r;
    hlit = (int)v + 257;
    r = woff1_bits_read(br, 5, &v);
    if (r != WOFF1_INF_OK) return r;
    hdist = (int)v + 1;
    r = woff1_bits_read(br, 4, &v);
    if (r != WOFF1_INF_OK) return r;
    hclen = (int)v + 4;

    if (hlit > 286 || hdist > 30) {
        return WOFF1_INF_BAD_HUFFMAN;
    }

    for (i = 0; i < WOFF1_MAX_CODELEN; ++i) {
        cl_lens[i] = 0;
    }
    for (i = 0; i < hclen; ++i) {
        r = woff1_bits_read(br, 3, &v);
        if (r != WOFF1_INF_OK) return r;
        cl_lens[woff1_cl_order[i]] = (unsigned char)v;
    }

    r = woff1_huff_build(&cl_huff, cl_lens, WOFF1_MAX_CODELEN);
    if (r != WOFF1_INF_OK) {
        return r;
    }

    total = hlit + hdist;
    for (i = 0; i < total; ++i) {
        all_lens[i] = 0;
    }

    i = 0;
    prev = 0;
    while (i < total) {
        r = woff1_huff_decode(br, &cl_huff, &sym);
        if (r != WOFF1_INF_OK) return r;
        if (sym >= 0 && sym <= 15) {
            all_lens[i] = (unsigned char)sym;
            prev = sym;
            i++;
        } else if (sym == 16) {
            if (i == 0) return WOFF1_INF_BAD_HUFFMAN;
            r = woff1_bits_read(br, 2, &v);
            if (r != WOFF1_INF_OK) return r;
            repeat = (int)v + 3;
            if (i + repeat > total) return WOFF1_INF_BAD_HUFFMAN;
            while (repeat > 0) {
                all_lens[i] = (unsigned char)prev;
                i++;
                repeat--;
            }
        } else if (sym == 17) {
            r = woff1_bits_read(br, 3, &v);
            if (r != WOFF1_INF_OK) return r;
            repeat = (int)v + 3;
            if (i + repeat > total) return WOFF1_INF_BAD_HUFFMAN;
            while (repeat > 0) {
                all_lens[i] = 0;
                i++;
                repeat--;
            }
            prev = 0;
        } else if (sym == 18) {
            r = woff1_bits_read(br, 7, &v);
            if (r != WOFF1_INF_OK) return r;
            repeat = (int)v + 11;
            if (i + repeat > total) return WOFF1_INF_BAD_HUFFMAN;
            while (repeat > 0) {
                all_lens[i] = 0;
                i++;
                repeat--;
            }
            prev = 0;
        } else {
            return WOFF1_INF_BAD_HUFFMAN;
        }
    }

    for (i = 0; i < WOFF1_MAX_LITLEN; ++i) {
        ll_lens[i] = 0;
    }
    for (i = 0; i < WOFF1_MAX_DIST; ++i) {
        d_lens[i] = 0;
    }
    for (i = 0; i < hlit; ++i) {
        ll_lens[i] = all_lens[i];
    }
    for (i = 0; i < hdist; ++i) {
        d_lens[i] = all_lens[hlit + i];
    }

    if (ll_lens[256] == 0) {
        return WOFF1_INF_BAD_HUFFMAN;
    }

    r = woff1_huff_build(litlen, ll_lens, hlit);
    if (r != WOFF1_INF_OK) {
        return r;
    }

    nonzero_dist = 0;
    for (i = 0; i < hdist; ++i) {
        if (d_lens[i] != 0) {
            nonzero_dist = 1;
            break;
        }
    }
    if (!nonzero_dist) {
        *dist_empty = 1;
        dist->num = 0;
        dist->max_bits = 0;
        return WOFF1_INF_OK;
    }
    *dist_empty = 0;
    return woff1_huff_build(dist, d_lens, hdist);
}

static int woff1_inflate_codes(struct woff1_bits_s *br,
                               struct woff1_huff_s *litlen,
                               struct woff1_huff_s *dist,
                               int dist_empty,
                               woff1_u8 *dst,
                               woff1_u32 dst_cap,
                               woff1_u32 *op)
{
    int sym;
    int dsym;
    int r;
    woff1_u32 v;
    woff1_u32 length;
    woff1_u32 distance;
    woff1_u32 i;

    for (;;) {
        r = woff1_huff_decode(br, litlen, &sym);
        if (r != WOFF1_INF_OK) return r;
        if (sym < 256) {
            if (*op >= dst_cap) return WOFF1_INF_OUTPUT_FULL;
            dst[*op] = (woff1_u8)sym;
            *op = *op + 1u;
        } else if (sym == 256) {
            return WOFF1_INF_OK;
        } else if (sym >= 257 && sym <= 285) {
            i = (woff1_u32)(sym - 257);
            length = (woff1_u32)woff1_len_base[i];
            if (woff1_len_extra[i] != 0) {
                r = woff1_bits_read(br, (int)woff1_len_extra[i], &v);
                if (r != WOFF1_INF_OK) return r;
                length += v;
            }
            if (dist_empty || dist->num == 0) {
                return WOFF1_INF_BAD_DISTANCE;
            }
            r = woff1_huff_decode(br, dist, &dsym);
            if (r != WOFF1_INF_OK) return r;
            if (dsym < 0 || dsym >= 30) {
                return WOFF1_INF_BAD_DISTANCE;
            }
            distance = (woff1_u32)woff1_dist_base[dsym];
            if (woff1_dist_extra[dsym] != 0) {
                r = woff1_bits_read(br, (int)woff1_dist_extra[dsym], &v);
                if (r != WOFF1_INF_OK) return r;
                distance += v;
            }
            if (distance == 0 || distance > *op) {
                return WOFF1_INF_BAD_DISTANCE;
            }
            if (length > dst_cap - *op) {
                return WOFF1_INF_OUTPUT_FULL;
            }
            while (length > 0) {
                dst[*op] = dst[*op - distance];
                *op = *op + 1u;
                length--;
            }
        } else {
            return WOFF1_INF_BAD_LENGTH;
        }
    }
}

static int woff1_inflate_deflate(struct woff1_bits_s *br,
                                 woff1_u8 *dst, woff1_u32 dst_cap,
                                 woff1_u32 *dst_len)
{
    int last;
    woff1_u32 bfinal;
    woff1_u32 btype;
    int r;
    woff1_u8 b0;
    woff1_u8 b1;
    woff1_u8 b2;
    woff1_u8 b3;
    woff1_u32 len;
    woff1_u32 nlen;
    woff1_u32 i;
    struct woff1_huff_s litlen;
    struct woff1_huff_s dist;
    int dist_empty;
    woff1_u32 op;

    op = 0;
    last = 0;
    while (!last) {
        r = woff1_bits_read(br, 1, &bfinal);
        if (r != WOFF1_INF_OK) return r;
        r = woff1_bits_read(br, 2, &btype);
        if (r != WOFF1_INF_OK) return r;
        last = (bfinal != 0u);

        if (btype == 0u) {
            woff1_bits_align_byte(br);
            r = woff1_bits_read_byte_aligned(br, &b0);
            if (r != WOFF1_INF_OK) return r;
            r = woff1_bits_read_byte_aligned(br, &b1);
            if (r != WOFF1_INF_OK) return r;
            r = woff1_bits_read_byte_aligned(br, &b2);
            if (r != WOFF1_INF_OK) return r;
            r = woff1_bits_read_byte_aligned(br, &b3);
            if (r != WOFF1_INF_OK) return r;
            len = (woff1_u32)b0 | ((woff1_u32)b1 << 8);
            nlen = (woff1_u32)b2 | ((woff1_u32)b3 << 8);
            if (((len ^ nlen) & 0xffffu) != 0xffffu) {
                return WOFF1_INF_BAD_STORED_BLOCK;
            }
            if (len > dst_cap - op) {
                return WOFF1_INF_OUTPUT_FULL;
            }
            for (i = 0; i < len; ++i) {
                r = woff1_bits_read_byte_aligned(br, &dst[op]);
                if (r != WOFF1_INF_OK) return r;
                op++;
            }
        } else if (btype == 1u) {
            r = woff1_make_fixed(&litlen, &dist);
            if (r != WOFF1_INF_OK) return r;
            r = woff1_inflate_codes(br, &litlen, &dist, 0, dst, dst_cap, &op);
            if (r != WOFF1_INF_OK) return r;
        } else if (btype == 2u) {
            dist_empty = 0;
            r = woff1_read_dynamic(br, &litlen, &dist, &dist_empty);
            if (r != WOFF1_INF_OK) return r;
            r = woff1_inflate_codes(br, &litlen, &dist, dist_empty, dst, dst_cap, &op);
            if (r != WOFF1_INF_OK) return r;
        } else {
            return WOFF1_INF_BAD_BLOCK_TYPE;
        }
    }

    *dst_len = op;
    return WOFF1_INF_OK;
}

woff1_u32 woff1_adler32(const woff1_u8 *buf, woff1_u32 len)
{
    woff1_u32 s1;
    woff1_u32 s2;
    woff1_u32 i;

    s1 = 1u;
    s2 = 0u;
    for (i = 0; i < len; ++i) {
        s1 += (woff1_u32)buf[i];
        if (s1 >= 65521u) s1 -= 65521u;
        s2 += s1;
        s2 %= 65521u;
    }
    return ((s2 & 0xffffu) << 16) | (s1 & 0xffffu);
}

int woff1_inflate_zlib(const woff1_u8 *src, woff1_u32 src_len,
                       woff1_u8 *dst, woff1_u32 dst_cap,
                       woff1_u32 *dst_len, int strict_trailing)
{
    woff1_u8 cmf;
    woff1_u8 flg;
    woff1_u32 header;
    woff1_u32 got_adler;
    woff1_u32 calc_adler;
    struct woff1_bits_s br;
    int r;

    if (dst_len == 0 || src == 0 || dst == 0) {
        return WOFF1_INF_NEED_INPUT;
    }
    *dst_len = 0;
    if (src_len < 6u) {
        return WOFF1_INF_NEED_INPUT;
    }

    cmf = src[0];
    flg = src[1];
    header = ((woff1_u32)cmf << 8) | (woff1_u32)flg;
    if ((header % 31u) != 0u) {
        return WOFF1_INF_BAD_ZLIB_HEADER;
    }
    if ((cmf & 0x0fu) != 8u) {
        return WOFF1_INF_UNSUPPORTED_ZLIB;
    }
    if ((cmf >> 4) > 7u) {
        return WOFF1_INF_UNSUPPORTED_ZLIB;
    }
    if ((flg & 0x20u) != 0u) {
        return WOFF1_INF_UNSUPPORTED_ZLIB;
    }

    woff1_bits_init(&br, src + 2, src_len - 2u);
    r = woff1_inflate_deflate(&br, dst, dst_cap, dst_len);
    if (r != WOFF1_INF_OK) {
        return r;
    }

    woff1_bits_align_byte(&br);
    if (br.size - br.pos < 4u) {
        return WOFF1_INF_NEED_INPUT;
    }
    got_adler = ((woff1_u32)br.src[br.pos] << 24) |
                ((woff1_u32)br.src[br.pos + 1u] << 16) |
                ((woff1_u32)br.src[br.pos + 2u] << 8) |
                ((woff1_u32)br.src[br.pos + 3u]);
    br.pos += 4u;
    calc_adler = woff1_adler32(dst, *dst_len);
    if (got_adler != calc_adler) {
        return WOFF1_INF_BAD_ADLER32;
    }
    if (strict_trailing && br.pos != br.size) {
        return WOFF1_INF_TRAILING_DATA;
    }
    return WOFF1_INF_OK;
}

const char *woff1_inflate_error_string(int code)
{
    switch (code) {
    case WOFF1_INF_OK: return "ok";
    case WOFF1_INF_NEED_INPUT: return "truncated input";
    case WOFF1_INF_OUTPUT_FULL: return "output buffer too small";
    case WOFF1_INF_BAD_ZLIB_HEADER: return "bad zlib header";
    case WOFF1_INF_UNSUPPORTED_ZLIB: return "unsupported zlib feature";
    case WOFF1_INF_BAD_BLOCK_TYPE: return "bad deflate block type";
    case WOFF1_INF_BAD_STORED_BLOCK: return "bad stored block";
    case WOFF1_INF_BAD_HUFFMAN: return "bad huffman stream";
    case WOFF1_INF_BAD_DISTANCE: return "bad back-reference distance";
    case WOFF1_INF_BAD_LENGTH: return "bad length symbol";
    case WOFF1_INF_BAD_ADLER32: return "bad adler32";
    case WOFF1_INF_TRAILING_DATA: return "trailing data after zlib stream";
    default: return "unknown inflate error";
    }
}
