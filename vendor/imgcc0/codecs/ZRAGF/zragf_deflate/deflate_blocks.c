#include "deflate_blocks.h"
#include <string.h>

static unsigned short zragf_len_sym_table_store[259];
static unsigned short zragf_dist_sym_table_store[32769];
static int zragf_blocks_tables_ready = 0;

static int zragf_len_to_sym_exact_slow(int length)
{
    static const int base[29] = {
        3,4,5,6,7,8,9,10,11,13,15,17,19,23,27,31,
        35,43,51,59,67,83,99,115,131,163,195,227,258
    };
    static const int extra[29] = {
        0,0,0,0,0,0,0,0,1,1,1,1,2,2,2,2,
        3,3,3,3,4,4,4,4,5,5,5,5,0
    };
    int i;

    if (length < 3)
        length = 3;
    if (length > 258)
        length = 258;

    for (i = 0; i < 29; ++i) {
        int maxv;
        if (i == 27)
            maxv = 257;
        else if (i == 28)
            maxv = 258;
        else
            maxv = base[i] + ((1 << extra[i]) - 1);
        if (length >= base[i] && length <= maxv)
            return 257 + i;
    }
    return 285;
}

static int zragf_dist_to_sym_exact_slow(int dist)
{
    static const int base[30] = {
        1,2,3,4,5,7,9,13,17,25,33,49,65,97,129,193,
        257,385,513,769,1025,1537,2049,3073,4097,6145,8193,12289,16385,24577
    };
    static const int extra[30] = {
        0,0,0,0,1,1,2,2,3,3,4,4,5,5,6,6,
        7,7,8,8,9,9,10,10,11,11,12,12,13,13
    };
    int i;

    if (dist < 1)
        dist = 1;
    if (dist > 32768)
        dist = 32768;

    for (i = 0; i < 30; ++i) {
        int maxv = base[i] + ((1 << extra[i]) - 1);
        if (dist >= base[i] && dist <= maxv)
            return i;
    }
    return 29;
}

static void zragf_blocks_init_tables(void)
{
    int i;
    if (zragf_blocks_tables_ready)
        return;
    zragf_len_sym_table_store[0] = 257u;
    zragf_len_sym_table_store[1] = 257u;
    zragf_len_sym_table_store[2] = 257u;
    for (i = 3; i <= 258; ++i)
        zragf_len_sym_table_store[i] = (unsigned short)zragf_len_to_sym_exact_slow(i);
    zragf_dist_sym_table_store[0] = 0u;
    for (i = 1; i <= 32768; ++i)
        zragf_dist_sym_table_store[i] = (unsigned short)zragf_dist_to_sym_exact_slow(i);
    zragf_blocks_tables_ready = 1;
}

const unsigned short *zragf_blocks_len_sym_table(void)
{
    zragf_blocks_init_tables();
    return zragf_len_sym_table_store;
}

const unsigned short *zragf_blocks_dist_sym_table(void)
{
    zragf_blocks_init_tables();
    return zragf_dist_sym_table_store;
}

void zragf_blocks_reset_stats(zragf_block_stats *st)
{
    if (!st)
        return;
    memset(st->litlen_freq, 0, sizeof(st->litlen_freq));
    memset(st->dist_freq,   0, sizeof(st->dist_freq));
    st->raw_size = 0;
}

void zragf_blocks_add_literal(zragf_block_stats *st, int lit)
{
    if (!st)
        return;

    if (lit < 0)
        return;
    if (lit > 255)
        lit = 255;

    st->litlen_freq[lit]++;
    st->raw_size++;
}

void zragf_blocks_add_match(zragf_block_stats *st,
                            int length,
                            int dist)
{
    const unsigned short *len_tab;
    const unsigned short *dist_tab;

    if (!st)
        return;

    zragf_blocks_init_tables();
    len_tab = zragf_len_sym_table_store;
    dist_tab = zragf_dist_sym_table_store;
    if (length < 0)
        length = 0;
    if (length > 258)
        length = 258;
    if (dist < 0)
        dist = 0;
    if (dist > 32768)
        dist = 32768;

    st->litlen_freq[len_tab[length]]++;
    st->dist_freq[dist_tab[dist]]++;
    st->raw_size += length;
}

static int zragf_blocks_is_biased(const zragf_block_stats *st)
{
    int i;
    unsigned total = 0u;
    unsigned top = 0u;

    for (i = 0; i < 286; ++i)
        total += st->litlen_freq[i];

    if (total == 0u)
        return 0;

    for (i = 0; i < 286; ++i) {
        if (st->litlen_freq[i] > top)
            top = st->litlen_freq[i];
    }

    return (top * 100u / total > 40u) ? 1 : 0;
}

static int zragf_blocks_is_uniform(const zragf_block_stats *st)
{
    int i;
    int nonzero = 0;
    unsigned minv = 0u;
    unsigned maxv = 0u;

    for (i = 0; i < 286; ++i) {
        unsigned v = st->litlen_freq[i];
        if (v == 0u)
            continue;
        if (nonzero == 0) {
            minv = maxv = v;
        } else {
            if (v < minv)
                minv = v;
            if (v > maxv)
                maxv = v;
        }
        nonzero++;
    }

    if (nonzero < 4)
        return 0;

    return (maxv <= minv * 3u) ? 1 : 0;
}

zragf_block_type zragf_blocks_choose_type(const zragf_block_stats *st,
                                          const zragf_deflate_state *core)
{
    (void)core;

    if (st->raw_size < 32)
        return ZRAGF_BLOCK_STORED;
    if (zragf_blocks_is_uniform(st))
        return ZRAGF_BLOCK_FIXED;
    if (zragf_blocks_is_biased(st))
        return ZRAGF_BLOCK_DYNAMIC;
    if (st->raw_size < 1024)
        return ZRAGF_BLOCK_FIXED;
    return ZRAGF_BLOCK_DYNAMIC;
}
