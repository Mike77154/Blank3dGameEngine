/* deflate_fixed.c */

#include "deflate_fixed.h"
#include "deflate_huffman_shared.h"

/* --- helpers de bitwriter LSB-first --- */

static int zragf_bw_put_byte_lsbf(zragf_bw_lsbf *bw, zragf_u8 b)
{
    if (!bw || !bw->out)
        return 0;
    if (bw->out_pos >= bw->out_cap)
        return 0;
    bw->out[bw->out_pos++] = b;
    return 1;
}

/* escribe 'count' bits, LSB-first */
static int zragf_bw_put_bits_lsbf(zragf_bw_lsbf *bw,
                                  unsigned value,
                                  int count)
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
        if (!zragf_bw_put_byte_lsbf(bw, (zragf_u8)bw->bitbuf))
            return 0;
        bw->bitbuf >>= 8u;
        bw->bitcount -= 8;
    }
    return 1;
}

static ZRAGF_MAYBE_UNUSED int zragf_bw_flush_bits_lsbf(zragf_bw_lsbf *bw)
{
    if (bw->bitcount > 0) {
        if (!zragf_bw_put_byte_lsbf(bw, (zragf_u8)bw->bitbuf))
            return 0;
        bw->bitbuf = 0;
        bw->bitcount = 0;
    }
    return 1;
}

/* --- inicialización de tablas FIXED (RFC1951) --- */

void zragf_fixed_init_tables(zragf_fixed_tables *ft)
{
    int i;
    if (!ft)
        return;

    /* Literales/longitudes 0–143: 8 bits, códigos 00110000..10111111 (48..191) */
    for (i = 0; i <= 143; ++i) {
        ft->litlen[i].len  = 8;
        ft->litlen[i].code = zragf_deflate_huff_reverse_bits(0x30u + (unsigned)i, 8);
    }

    /* 144–255: 9 bits, códigos 110010000..111111111 (400..511) */
    for (i = 144; i <= 255; ++i) {
        ft->litlen[i].len  = 9;
        ft->litlen[i].code = zragf_deflate_huff_reverse_bits(0x190u + (unsigned)(i - 144), 9);
    }

    /* 256–279: 7 bits, códigos 0000000..0010111 (0..23) */
    for (i = 256; i <= 279; ++i) {
        ft->litlen[i].len  = 7;
        ft->litlen[i].code = zragf_deflate_huff_reverse_bits((unsigned)(i - 256), 7);
    }

    /* 280–287: 8 bits, códigos 11000000..11000111 (192..199) */
    for (i = 280; i <= 287; ++i) {
        ft->litlen[i].len  = 8;
        ft->litlen[i].code = zragf_deflate_huff_reverse_bits(0xC0u + (unsigned)(i - 280), 8);
    }

    /* distancias 0–29: todos 5 bits, códigos 00000..11101 */
    for (i = 0; i < 30; ++i) {
        ft->dist[i].len  = 5;
        ft->dist[i].code = zragf_deflate_huff_reverse_bits((unsigned)i, 5);
    }
}

const zragf_fixed_tables *zragf_fixed_get_tables(void)
{
    static zragf_fixed_tables ft;
    static int init_done = 0;
    if (!init_done) {
        zragf_fixed_init_tables(&ft);
        init_done = 1;
    }
    return &ft;
}

/* --- tablas de longitud y extra bits para longitudes/distancias --- */

/* tablas clásicas DEFLATE */
typedef struct {
    int base;
    int extra_bits;
} zragf_len_info;

typedef struct {
    int base;
    int extra_bits;
} zragf_dist_info;

/* 29 códigos de longitud (257–285) */
static const zragf_len_info zragf_len_table[29] = {
    {3,0},{4,0},{5,0},{6,0},{7,0},{8,0},{9,0},{10,0},
    {11,1},{13,1},{15,1},{17,1},
    {19,2},{23,2},{27,2},{31,2},
    {35,3},{43,3},{51,3},{59,3},
    {67,4},{83,4},{99,4},{115,4},
    {131,5},{163,5},{195,5},{227,5},
    {258,0}
};

/* 30 códigos de distancia (0–29) */
static const zragf_dist_info zragf_dist_table[30] = {
    {1,0},{2,0},{3,0},{4,0},
    {5,1},{7,1},
    {9,2},{13,2},
    {17,3},{25,3},
    {33,4},{49,4},
    {65,5},{97,5},
    {129,6},{193,6},
    {257,7},{385,7},
    {513,8},{769,8},
    {1025,9},{1537,9},
    {2049,10},{3073,10},
    {4097,11},{6145,11},
    {8193,12},{12289,12},
    {16385,13},{24577,13}
};

static int zragf_len_to_sym(int len, int *out_sym, int *out_extra)
{
    int i;
    if (len < 3) len = 3;
    if (len > 258) len = 258;

    for (i = 0; i < 29; ++i) {
        int base = zragf_len_table[i].base;
        int eb   = zragf_len_table[i].extra_bits;
        int maxv;
        if (i == 27)
            maxv = 257;
        else if (i == 28)
            maxv = 258;
        else
            maxv = base + ((1 << eb) - 1);
        if (len >= base && len <= maxv) {
            *out_sym   = 257 + i;
            *out_extra = len - base;
            return eb;
        }
    }
    /* 258 caso especial */
    *out_sym   = 285;
    *out_extra = 0;
    return 0;
}

static int zragf_dist_to_sym(int dist, int *out_sym, int *out_extra)
{
    int i;
    if (dist < 1) dist = 1;
    if (dist > 32768) dist = 32768;

    for (i = 0; i < 30; ++i) {
        int base = zragf_dist_table[i].base;
        int eb   = zragf_dist_table[i].extra_bits;
        int maxv = base + ((1 << eb) - 1);
        if (dist >= base && dist <= maxv) {
            *out_sym   = i;
            *out_extra = dist - base;
            return eb;
        }
    }
    *out_sym   = 29;
    *out_extra = dist - zragf_dist_table[29].base;
    return zragf_dist_table[29].extra_bits;
}

/* --- emisión de un símbolo Huffman fijo --- */

static int zragf_emit_litlen_fixed(zragf_bw_lsbf *bw,
                                   const zragf_fixed_tables *ft,
                                   int sym)
{
    zragf_hcode_fixed hc;
    if (sym < 0)   sym = 0;
    if (sym > 285) sym = 285;
    hc = ft->litlen[sym];
    return zragf_bw_put_bits_lsbf(bw, hc.code, hc.len);
}

static int zragf_emit_dist_fixed(zragf_bw_lsbf *bw,
                                 const zragf_fixed_tables *ft,
                                 int sym)
{
    zragf_hcode_fixed hc;
    if (sym < 0)  sym = 0;
    if (sym > 29) sym = 29;
    hc = ft->dist[sym];
    return zragf_bw_put_bits_lsbf(bw, hc.code, hc.len);
}

/* --- bloque FIXED completo --- */

int zragf_fixed_emit_block(zragf_bw_lsbf            *bw,
                           const zragf_fixed_tables *ft,
                           const zragf_token        *toks,
                           zragf_size_t              ntoks,
                           int                       final_block)
{
    zragf_size_t i;

    if (!bw || !ft)
        return 0;

    /* BFINAL + BTYPE=01 */
    if (!zragf_bw_put_bits_lsbf(bw, final_block ? 1u : 0u, 1))
        return 0;
    if (!zragf_bw_put_bits_lsbf(bw, 1u, 2))
        return 0;

    /* Recorrer tokens */
    for (i = 0; i < ntoks; ++i) {
        const zragf_token *t = &toks[i];

        if (t->type == ZRAGF_TOK_LITERAL) {
            if (!zragf_emit_litlen_fixed(bw, ft, t->lit))
                return 0;
        } else if (t->type == ZRAGF_TOK_MATCH) {
            int len_sym, len_extra, len_ebits;
            int dist_sym, dist_extra, dist_ebits;

            len_ebits  = zragf_len_to_sym(t->len, &len_sym, &len_extra);
            dist_ebits = zragf_dist_to_sym(t->dist, &dist_sym, &dist_extra);

            /* código de longitud */
            if (!zragf_emit_litlen_fixed(bw, ft, len_sym))
                return 0;
            if (len_ebits > 0 &&
                !zragf_bw_put_bits_lsbf(bw, (unsigned)len_extra, len_ebits))
                return 0;

            /* código de distancia */
            if (!zragf_emit_dist_fixed(bw, ft, dist_sym))
                return 0;
            if (dist_ebits > 0 &&
                !zragf_bw_put_bits_lsbf(bw, (unsigned)dist_extra, dist_ebits))
                return 0;
        }
    }

    /* END-OF-BLOCK (256) */
    if (!zragf_emit_litlen_fixed(bw, ft, 256))
        return 0;

    /* No alineamos aquí: RFC permite que siguiente bloque continúe en bit. 
       Si quieres, puedes llamar a zragf_bw_flush_bits_lsbf al final de todo. */

    return 1;
}
