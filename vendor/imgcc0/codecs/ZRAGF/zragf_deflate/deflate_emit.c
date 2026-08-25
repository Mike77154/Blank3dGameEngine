#include "deflate_emit.h"
#include "deflate_fixed.h"
#include "deflate_huffman_shared.h"
#include <string.h>

typedef struct
{
    zragf_u8    *out;
    zragf_size_t pos;
    zragf_size_t cap;
    unsigned     bitbuf;
    int          bitcount;
} zragf_emit_bw;

typedef zragf_deflate_huff_code zragf_emit_code;
typedef zragf_deflate_cl_token zragf_emit_cl;

#define ZRAGF_EMIT_CL_TOKEN_CAP ZRAGF_DEFLATE_CL_TOKEN_CAP

static int zragf_emit_len_to_sym(int len, int *sym, int *extra);
static int zragf_emit_dist_to_sym(int dist, int *sym, int *extra);

typedef struct {
    unsigned short sym;
    unsigned short extra;
    unsigned char  ebits;
} zragf_emit_sym_desc;

static zragf_emit_sym_desc zragf_emit_len_desc[259];
static zragf_emit_sym_desc zragf_emit_dist_desc[32769];
static int zragf_emit_tables_ready = 0;

static void zragf_emit_init_tables(void)
{
    int len;
    int dist;
    if (zragf_emit_tables_ready)
        return;
    for (len = 0; len <= 258; ++len) {
        int sym = 285;
        int extra = 0;
        int ebits = zragf_emit_len_to_sym(len, &sym, &extra);
        zragf_emit_len_desc[len].sym = (unsigned short)sym;
        zragf_emit_len_desc[len].extra = (unsigned short)extra;
        zragf_emit_len_desc[len].ebits = (unsigned char)ebits;
    }
    for (dist = 0; dist <= 32768; ++dist) {
        int sym = 29;
        int extra = 0;
        int ebits = zragf_emit_dist_to_sym(dist, &sym, &extra);
        zragf_emit_dist_desc[dist].sym = (unsigned short)sym;
        zragf_emit_dist_desc[dist].extra = (unsigned short)extra;
        zragf_emit_dist_desc[dist].ebits = (unsigned char)ebits;
    }
    zragf_emit_tables_ready = 1;
}

static const int zragf_len_base[29] = {
    3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
    35,43,51,59,67,83,99,115,131,163,195,227,258
};
static const int zragf_len_extra[29] = {
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
    3,3,3,3,4,4,4,4,5,5,5,5,0
};
static const int zragf_dist_base[30] = {
    1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
    257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577
};
static const int zragf_dist_extra[30] = {
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,
    7,7,8,8,9,9,10,10,11,11,12,12,13,13
};
static int zragf_emit_bw_reserve(zragf_emit_bw *bw, zragf_size_t extra)
{
    return (bw && bw->out && bw->pos + extra <= bw->cap) ? 1 : 0;
}

static int zragf_emit_bw_put_byte(zragf_emit_bw *bw, zragf_u8 b)
{
    if (!zragf_emit_bw_reserve(bw, 1u))
        return 0;
    bw->out[bw->pos++] = b;
    return 1;
}

static int zragf_emit_bw_put_bits(zragf_emit_bw *bw, unsigned value, int count)
{
    unsigned mask;
    if (!bw)
        return 0;
    if (count < 0 || count > 24)
        return 0;
    if (count == 0)
        return 1;

    mask = (1u << count) - 1u;
    bw->bitbuf |= (value & mask) << bw->bitcount;
    bw->bitcount += count;

    while (bw->bitcount >= 8) {
        if (!zragf_emit_bw_put_byte(bw, (zragf_u8)bw->bitbuf))
            return 0;
        bw->bitbuf >>= 8u;
        bw->bitcount -= 8;
    }
    return 1;
}

static int zragf_emit_bw_flush_bits(zragf_emit_bw *bw)
{
    if (bw->bitcount > 0) {
        if (!zragf_emit_bw_put_byte(bw, (zragf_u8)bw->bitbuf))
            return 0;
        bw->bitbuf = 0u;
        bw->bitcount = 0;
    }
    return 1;
}

static int zragf_emit_bw_align_byte(zragf_emit_bw *bw)
{
    if (!bw)
        return 0;
    if (bw->bitcount == 0)
        return 1;
    return zragf_emit_bw_put_bits(bw, 0u, 8 - bw->bitcount);
}

static int zragf_emit_bw_finish(zragf_emit_bw *bw,
                                unsigned *bitbuf_io,
                                int *bitcount_io,
                                int flush_bits)
{
    if (!bw)
        return 0;
    if (flush_bits) {
        if (!zragf_emit_bw_flush_bits(bw))
            return 0;
        if (bitbuf_io)
            *bitbuf_io = 0u;
        if (bitcount_io)
            *bitcount_io = 0;
    } else {
        if (bitbuf_io)
            *bitbuf_io = bw->bitbuf;
        if (bitcount_io)
            *bitcount_io = bw->bitcount;
    }
    return 1;
}

static int zragf_emit_len_to_sym(int len, int *sym, int *extra)
{
    int i;
    if (len < 3)
        len = 3;
    if (len > 258)
        len = 258;
    for (i = 0; i < 29; ++i) {
        int maxv;
        if (i == 27)
            maxv = 257;
        else if (i == 28)
            maxv = 258;
        else
            maxv = zragf_len_base[i] + ((1 << zragf_len_extra[i]) - 1);
        if (len >= zragf_len_base[i] && len <= maxv) {
            *sym = 257 + i;
            *extra = len - zragf_len_base[i];
            return zragf_len_extra[i];
        }
    }
    *sym = 285;
    *extra = 0;
    return 0;
}

static int zragf_emit_dist_to_sym(int dist, int *sym, int *extra)
{
    int i;
    if (dist < 1)
        dist = 1;
    if (dist > 32768)
        dist = 32768;
    for (i = 0; i < 30; ++i) {
        int maxv = zragf_dist_base[i] + ((1 << zragf_dist_extra[i]) - 1);
        if (dist >= zragf_dist_base[i] && dist <= maxv) {
            *sym = i;
            *extra = dist - zragf_dist_base[i];
            return zragf_dist_extra[i];
        }
    }
    *sym = 29;
    *extra = dist - zragf_dist_base[29];
    return zragf_dist_extra[29];
}

static int zragf_emit_symbol_lsb(zragf_emit_bw *bw,
                                 const zragf_emit_code *codes,
                                 int sym)
{
    if (!bw || !codes || sym < 0)
        return 0;
    if (codes[sym].len <= 0)
        return 0;
    return zragf_emit_bw_put_bits(bw, codes[sym].code, codes[sym].len);
}

static int zragf_emit_tokens_with_codes_lsb(zragf_emit_bw *bw,
                                            const zragf_token *toks,
                                            zragf_size_t ntoks,
                                            const zragf_emit_code *ll_codes,
                                            const zragf_emit_code *d_codes)
{
    zragf_size_t i;

    zragf_emit_init_tables();

    for (i = 0; i < ntoks; ++i) {
        const zragf_token *t = &toks[i];
        if (t->type == ZRAGF_TOK_LITERAL) {
            if (!zragf_emit_symbol_lsb(bw, ll_codes, t->lit))
                return 0;
        } else {
            int len = t->len;
            int dist = t->dist;
            const zragf_emit_sym_desc *ldesc;
            const zragf_emit_sym_desc *ddesc;

            if (len < 3) len = 3;
            if (len > 258) len = 258;
            if (dist < 1) dist = 1;
            if (dist > 32768) dist = 32768;
            ldesc = &zragf_emit_len_desc[len];
            ddesc = &zragf_emit_dist_desc[dist];

            if (!zragf_emit_symbol_lsb(bw, ll_codes, ldesc->sym))
                return 0;
            if (ldesc->ebits > 0 && !zragf_emit_bw_put_bits(bw, (unsigned)ldesc->extra, ldesc->ebits))
                return 0;
            if (!zragf_emit_symbol_lsb(bw, d_codes, ddesc->sym))
                return 0;
            if (ddesc->ebits > 0 && !zragf_emit_bw_put_bits(bw, (unsigned)ddesc->extra, ddesc->ebits))
                return 0;
        }
    }

    return zragf_emit_symbol_lsb(bw, ll_codes, 256);
}

int zragf_deflate_emit_dynamic_block_prepared(const zragf_token *toks,
                                              zragf_size_t ntoks,
                                              const zragf_deflate_dynamic_prepared *prep,
                                              zragf_u8 *dst,
                                              zragf_size_t dst_cap,
                                              zragf_size_t *dst_size,
                                              int final_block,
                                              unsigned *bitbuf_io,
                                              int *bitcount_io,
                                              int flush_final_bits)
{
    zragf_emit_bw bw;
    int i;

    if (!dst || !dst_size || !prep || !prep->valid)
        return 0;

    bw.out = dst;
    bw.pos = 0u;
    bw.cap = dst_cap;
    bw.bitbuf = bitbuf_io ? *bitbuf_io : 0u;
    bw.bitcount = bitcount_io ? *bitcount_io : 0;

    if (!zragf_emit_bw_put_bits(&bw, final_block ? 1u : 0u, 1))
        return 0;
    if (!zragf_emit_bw_put_bits(&bw, 2u, 2))
        return 0;
    if (!zragf_emit_bw_put_bits(&bw, (unsigned)prep->hlit, 5))
        return 0;
    if (!zragf_emit_bw_put_bits(&bw, (unsigned)prep->hdist, 5))
        return 0;
    if (!zragf_emit_bw_put_bits(&bw, (unsigned)prep->hclen, 4))
        return 0;

    for (i = 0; i < 4 + prep->hclen; ++i) {
        if (!zragf_emit_bw_put_bits(&bw, (unsigned)prep->cl_len[zragf_deflate_cl_order[i]], 3))
            return 0;
    }

    for (i = 0; i < (int)prep->rle_count; ++i) {
        if (!zragf_emit_symbol_lsb(&bw, prep->cl_codes, prep->rle[i].sym))
            return 0;
        if (prep->rle[i].extra_bits > 0 &&
            !zragf_emit_bw_put_bits(&bw, prep->rle[i].extra_val, prep->rle[i].extra_bits))
            return 0;
    }

    if (!zragf_emit_tokens_with_codes_lsb(&bw, toks, ntoks, prep->ll_codes, prep->d_codes))
        return 0;
    if (!zragf_emit_bw_finish(&bw, bitbuf_io, bitcount_io, flush_final_bits))
        return 0;

    *dst_size = bw.pos;
    return 1;
}

int zragf_deflate_emit_dynamic_block(const zragf_token *toks,
                                     zragf_size_t ntoks,
                                     const zragf_block_stats *stats,
                                     zragf_u8 *dst,
                                     zragf_size_t dst_cap,
                                     zragf_size_t *dst_size,
                                     int final_block,
                                     unsigned *bitbuf_io,
                                     int *bitcount_io,
                                     int flush_final_bits)
{
    zragf_deflate_dynamic_prepared prep;
    if (!zragf_deflate_prepare_dynamic(stats, &prep))
        return 0;
    return zragf_deflate_emit_dynamic_block_prepared(toks, ntoks, &prep,
                                                     dst, dst_cap, dst_size,
                                                     final_block, bitbuf_io, bitcount_io,
                                                     flush_final_bits);
}

int zragf_deflate_emit_fixed_block(const zragf_token *toks,
                                   zragf_size_t ntoks,
                                   zragf_u8 *dst,
                                   zragf_size_t dst_cap,
                                   zragf_size_t *dst_size,
                                   int final_block,
                                   unsigned *bitbuf_io,
                                   int *bitcount_io,
                                   int flush_final_bits)
{
    const zragf_fixed_tables *ft;
    zragf_bw_lsbf bw;

    if (!dst || !dst_size)
        return 0;

    ft = zragf_fixed_get_tables();
    if (!ft)
        return 0;
    bw.out = dst;
    bw.out_pos = 0u;
    bw.out_cap = dst_cap;
    bw.bitbuf = bitbuf_io ? *bitbuf_io : 0u;
    bw.bitcount = bitcount_io ? *bitcount_io : 0;

    if (!zragf_fixed_emit_block(&bw, ft, toks, ntoks, final_block))
        return 0;
    if (flush_final_bits) {
        if (bw.bitcount > 0) {
            if (bw.out_pos >= bw.out_cap)
                return 0;
            bw.out[bw.out_pos++] = (zragf_u8)bw.bitbuf;
            bw.bitbuf = 0u;
            bw.bitcount = 0;
        }
        if (bitbuf_io)
            *bitbuf_io = 0u;
        if (bitcount_io)
            *bitcount_io = 0;
    } else {
        if (bitbuf_io)
            *bitbuf_io = bw.bitbuf;
        if (bitcount_io)
            *bitcount_io = bw.bitcount;
    }
    *dst_size = bw.out_pos;
    return 1;
}

int zragf_deflate_emit_stored_chunk(const zragf_u8 *src,
                                    zragf_size_t src_size,
                                    zragf_u8 *dst,
                                    zragf_size_t dst_cap,
                                    zragf_size_t *dst_size,
                                    int final_block,
                                    unsigned *bitbuf_io,
                                    int *bitcount_io)
{
    zragf_emit_bw bw;
    zragf_size_t remaining;
    zragf_size_t offset;

    if (!dst || !dst_size)
        return 0;
    if (src_size > 0u && !src)
        return 0;

    bw.out = dst;
    bw.pos = 0u;
    bw.cap = dst_cap;
    bw.bitbuf = bitbuf_io ? *bitbuf_io : 0u;
    bw.bitcount = bitcount_io ? *bitcount_io : 0;

    remaining = src_size;
    offset = 0u;

    if (src_size == 0u) {
        if (!zragf_emit_bw_put_bits(&bw, final_block ? 1u : 0u, 1))
            return 0;
        if (!zragf_emit_bw_put_bits(&bw, 0u, 2))
            return 0;
        if (!zragf_emit_bw_align_byte(&bw))
            return 0;
        if (!zragf_emit_bw_reserve(&bw, 4u))
            return 0;
        bw.out[bw.pos++] = 0u;
        bw.out[bw.pos++] = 0u;
        bw.out[bw.pos++] = 0xFFu;
        bw.out[bw.pos++] = 0xFFu;
        if (!zragf_emit_bw_finish(&bw, bitbuf_io, bitcount_io, 1))
            return 0;
        *dst_size = bw.pos;
        return 1;
    }

    while (remaining > 0u) {
        zragf_size_t chunk = (remaining > 65535u) ? 65535u : remaining;
        int is_last_chunk = (remaining <= 65535u) ? 1 : 0;
        int bfinal = (is_last_chunk && final_block) ? 1 : 0;
        zragf_u16 len = (zragf_u16)chunk;
        zragf_u16 nlen = (zragf_u16)~len;
        zragf_size_t i;

        if (!zragf_emit_bw_put_bits(&bw, (unsigned)bfinal, 1))
            return 0;
        if (!zragf_emit_bw_put_bits(&bw, 0u, 2))
            return 0;
        if (!zragf_emit_bw_align_byte(&bw))
            return 0;
        if (!zragf_emit_bw_reserve(&bw, chunk + 4u))
            return 0;

        bw.out[bw.pos++] = (zragf_u8)(len & 0xFFu);
        bw.out[bw.pos++] = (zragf_u8)((len >> 8) & 0xFFu);
        bw.out[bw.pos++] = (zragf_u8)(nlen & 0xFFu);
        bw.out[bw.pos++] = (zragf_u8)((nlen >> 8) & 0xFFu);
        for (i = 0u; i < chunk; ++i)
            bw.out[bw.pos++] = src[offset + i];

        remaining -= chunk;
        offset += chunk;
    }

    if (!zragf_emit_bw_finish(&bw, bitbuf_io, bitcount_io, 1))
        return 0;
    *dst_size = bw.pos;
    return 1;
}
