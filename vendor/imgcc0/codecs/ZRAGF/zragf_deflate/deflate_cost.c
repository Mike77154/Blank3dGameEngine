#include "deflate_cost.h"
#include "deflate_huffman_shared.h"
#include <string.h>

typedef struct {
    zragf_size_t bytes;
    int bitcount;
} zragf_cost_bw;

typedef zragf_deflate_cl_token zragf_cost_cl;

#define ZRAGF_COST_CL_TOKEN_CAP ZRAGF_DEFLATE_CL_TOKEN_CAP

static const int zragf_cost_len_base[29] = {
    3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
    35,43,51,59,67,83,99,115,131,163,195,227,258
};
static const int zragf_cost_len_extra[29] = {
    0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
    3,3,3,3,4,4,4,4,5,5,5,5,0
};
static const int zragf_cost_dist_base[30] = {
    1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
    257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577
};
static const int zragf_cost_dist_extra[30] = {
    0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,
    7,7,8,8,9,9,10,10,11,11,12,12,13,13
};

typedef struct {
    unsigned short sym;
    unsigned short extra;
    unsigned char  ebits;
} zragf_cost_sym_desc;

static int zragf_cost_len_to_sym(int len, int *sym, int *extra);
static int zragf_cost_dist_to_sym(int dist, int *sym, int *extra);

static zragf_cost_sym_desc zragf_cost_len_desc[259];
static zragf_cost_sym_desc zragf_cost_dist_desc[32769];
static int zragf_cost_tables_ready = 0;

static void zragf_cost_init_tables(void)
{
    int len;
    int dist;
    if (zragf_cost_tables_ready)
        return;
    for (len = 0; len <= 258; ++len) {
        int sym = 285;
        int extra = 0;
        int ebits = zragf_cost_len_to_sym(len, &sym, &extra);
        zragf_cost_len_desc[len].sym = (unsigned short)sym;
        zragf_cost_len_desc[len].extra = (unsigned short)extra;
        zragf_cost_len_desc[len].ebits = (unsigned char)ebits;
    }
    for (dist = 0; dist <= 32768; ++dist) {
        int sym = 29;
        int extra = 0;
        int ebits = zragf_cost_dist_to_sym(dist, &sym, &extra);
        zragf_cost_dist_desc[dist].sym = (unsigned short)sym;
        zragf_cost_dist_desc[dist].extra = (unsigned short)extra;
        zragf_cost_dist_desc[dist].ebits = (unsigned char)ebits;
    }
    zragf_cost_tables_ready = 1;
}
static void zragf_cost_put_bits(zragf_cost_bw *bw, int count)
{
    int total;
    if (!bw || count <= 0)
        return;
    total = bw->bitcount + count;
    bw->bytes += (zragf_size_t)(total / 8);
    bw->bitcount = total % 8;
}

static void zragf_cost_align_byte(zragf_cost_bw *bw)
{
    if (!bw)
        return;
    if (bw->bitcount != 0) {
        bw->bytes += 1u;
        bw->bitcount = 0;
    }
}

static void zragf_cost_finish(zragf_cost_bw *bw, int flush_final_bits)
{
    if (!bw)
        return;
    if (flush_final_bits && bw->bitcount != 0) {
        bw->bytes += 1u;
        bw->bitcount = 0;
    }
}

static int zragf_cost_len_to_sym(int len, int *sym, int *extra)
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
            maxv = zragf_cost_len_base[i] + ((1 << zragf_cost_len_extra[i]) - 1);
        if (len >= zragf_cost_len_base[i] && len <= maxv) {
            *sym = 257 + i;
            *extra = len - zragf_cost_len_base[i];
            return zragf_cost_len_extra[i];
        }
    }
    *sym = 285;
    *extra = 0;
    return 0;
}

static int zragf_cost_dist_to_sym(int dist, int *sym, int *extra)
{
    int i;
    if (dist < 1)
        dist = 1;
    if (dist > 32768)
        dist = 32768;
    for (i = 0; i < 30; ++i) {
        int maxv = zragf_cost_dist_base[i] + ((1 << zragf_cost_dist_extra[i]) - 1);
        if (dist >= zragf_cost_dist_base[i] && dist <= maxv) {
            *sym = i;
            *extra = dist - zragf_cost_dist_base[i];
            return zragf_cost_dist_extra[i];
        }
    }
    *sym = 29;
    *extra = dist - zragf_cost_dist_base[29];
    return zragf_cost_dist_extra[29];
}

static int zragf_cost_fixed_ll_len(int sym)
{
    if (sym < 0)
        sym = 0;
    if (sym <= 143)
        return 8;
    if (sym <= 255)
        return 9;
    if (sym <= 279)
        return 7;
    if (sym <= 287)
        return 8;
    return 8;
}

int zragf_deflate_cost_stored(const zragf_u8 *src,
                              zragf_size_t src_size,
                              int final_block,
                              int start_bitcount,
                              int flush_final_bits,
                              zragf_deflate_cost_result *out)
{
    zragf_cost_bw bw;
    zragf_size_t remaining;

    (void)src;
    if (!out)
        return 0;

    bw.bytes = 0u;
    bw.bitcount = (start_bitcount >= 0) ? (start_bitcount & 7) : 0;

    if (src_size == 0u) {
        zragf_cost_put_bits(&bw, 3);
        zragf_cost_align_byte(&bw);
        bw.bytes += 4u;
        zragf_cost_finish(&bw, flush_final_bits);
        out->size_bytes = bw.bytes;
        out->end_bitcount = bw.bitcount;
        return 1;
    }

    remaining = src_size;
    while (remaining > 0u) {
        zragf_size_t chunk = (remaining > 65535u) ? 65535u : remaining;
        int is_last_chunk = (remaining <= 65535u) ? 1 : 0;
        int bfinal = (is_last_chunk && final_block) ? 1 : 0;
        (void)bfinal;
        zragf_cost_put_bits(&bw, 3);
        zragf_cost_align_byte(&bw);
        bw.bytes += 4u + chunk;
        remaining -= chunk;
    }
    zragf_cost_finish(&bw, flush_final_bits);
    out->size_bytes = bw.bytes;
    out->end_bitcount = bw.bitcount;
    return 1;
}

int zragf_deflate_cost_fixed(const zragf_token *toks,
                             zragf_size_t ntoks,
                             int final_block,
                             int start_bitcount,
                             int flush_final_bits,
                             zragf_deflate_cost_result *out)
{
    zragf_cost_bw bw;
    zragf_size_t i;
    (void)final_block;
    if (!out)
        return 0;
    bw.bytes = 0u;
    bw.bitcount = (start_bitcount >= 0) ? (start_bitcount & 7) : 0;

    zragf_cost_put_bits(&bw, 3);
    for (i = 0u; i < ntoks; ++i) {
        if (toks[i].type == ZRAGF_TOK_LITERAL) {
            zragf_cost_put_bits(&bw, zragf_cost_fixed_ll_len(toks[i].lit));
        } else {
            int len_sym;
            int len_extra;
            int len_ebits;
            int dist_sym;
            int dist_extra;
            int dist_ebits;
            len_ebits = zragf_cost_len_to_sym(toks[i].len, &len_sym, &len_extra);
            dist_ebits = zragf_cost_dist_to_sym(toks[i].dist, &dist_sym, &dist_extra);
            (void)dist_sym;
            zragf_cost_put_bits(&bw, zragf_cost_fixed_ll_len(len_sym));
            zragf_cost_put_bits(&bw, len_ebits);
            zragf_cost_put_bits(&bw, 5);
            zragf_cost_put_bits(&bw, dist_ebits);
        }
    }
    zragf_cost_put_bits(&bw, zragf_cost_fixed_ll_len(256));
    zragf_cost_finish(&bw, flush_final_bits);
    out->size_bytes = bw.bytes;
    out->end_bitcount = bw.bitcount;
    return 1;
}

int zragf_deflate_cost_dynamic_prepared(const zragf_token *toks,
                                        zragf_size_t ntoks,
                                        const zragf_deflate_dynamic_prepared *prep,
                                        int final_block,
                                        int start_bitcount,
                                        int flush_final_bits,
                                        zragf_deflate_cost_result *out)
{
    zragf_cost_bw bw;
    int i;

    (void)final_block;
    if (!out || !prep || !prep->valid)
        return 0;

    zragf_cost_init_tables();

    bw.bytes = 0u;
    bw.bitcount = (start_bitcount >= 0) ? (start_bitcount & 7) : 0;

    zragf_cost_put_bits(&bw, 3);
    zragf_cost_put_bits(&bw, (int)prep->header_bits);

    for (i = 0; i < (int)ntoks; ++i) {
        if (toks[i].type == ZRAGF_TOK_LITERAL) {
            int sym = toks[i].lit;
            if (sym < 0)
                sym = 0;
            if (sym > 285)
                sym = 285;
            zragf_cost_put_bits(&bw, prep->ll_len[sym]);
        } else {
            int len = toks[i].len;
            int dist = toks[i].dist;
            const zragf_cost_sym_desc *ldesc;
            const zragf_cost_sym_desc *ddesc;
            if (len < 3) len = 3;
            if (len > 258) len = 258;
            if (dist < 1) dist = 1;
            if (dist > 32768) dist = 32768;
            ldesc = &zragf_cost_len_desc[len];
            ddesc = &zragf_cost_dist_desc[dist];
            if (ldesc->sym > 285 || ddesc->sym > 29)
                return 0;
            zragf_cost_put_bits(&bw, prep->ll_len[ldesc->sym]);
            zragf_cost_put_bits(&bw, ldesc->ebits);
            zragf_cost_put_bits(&bw, prep->d_len[ddesc->sym]);
            zragf_cost_put_bits(&bw, ddesc->ebits);
        }
    }
    zragf_cost_put_bits(&bw, prep->ll_len[256]);
    zragf_cost_finish(&bw, flush_final_bits);
    out->size_bytes = bw.bytes;
    out->end_bitcount = bw.bitcount;
    return 1;
}

int zragf_deflate_cost_dynamic(const zragf_token *toks,
                               zragf_size_t ntoks,
                               const zragf_block_stats *stats,
                               int final_block,
                               int start_bitcount,
                               int flush_final_bits,
                               zragf_deflate_cost_result *out)
{
    zragf_deflate_dynamic_prepared prep;
    if (!zragf_deflate_prepare_dynamic(stats, &prep))
        return 0;
    return zragf_deflate_cost_dynamic_prepared(toks, ntoks, &prep,
                                               final_block, start_bitcount, flush_final_bits, out);
}
