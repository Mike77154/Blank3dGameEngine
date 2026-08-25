#include "inflate_huffman.h"
#include <stdlib.h>
#include <string.h>

static unsigned zragf_reverse_bits(unsigned code, int bits)
{
    unsigned rev = 0u;
    int i;
    for (i = 0; i < bits; ++i) {
        rev = (rev << 1) | (code & 1u);
        code >>= 1u;
    }
    return rev;
}

int zragf_inflate_build_table(const int *lengths,
                              int nlen,
                              int table_bits,
                              zragf_idec_table *out_tab)
{
    int max_bits = 0;
    int count = 0;
    int i;
    int bl_count[32];
    int next_code[32];
    unsigned code;
    zragf_idec_entry *table;

    if (!lengths || !out_tab)
        return 0;
    if (table_bits <= 0 || table_bits > 15)
        return 0;

    memset(out_tab, 0, sizeof(*out_tab));
    memset(bl_count, 0, sizeof(bl_count));
    memset(next_code, 0, sizeof(next_code));

    for (i = 0; i < nlen; ++i) {
        int l = lengths[i];
        if (l > 0) {
            if (l > 15)
                return 0;
            if (l > max_bits)
                max_bits = l;
            count++;
            bl_count[l]++;
        }
    }

    if (count == 0) {
        out_tab->table = NULL;
        out_tab->table_bits = table_bits;
        out_tab->count = 0;
        out_tab->max_bits = 0;
        return 1;
    }

    table = (zragf_idec_entry *)zragf_alloc_default(NULL,
                                                    (zragf_size_t)count,
                                                    sizeof(zragf_idec_entry));
    if (!table)
        return 0;

    {
        int left = 1;
        for (i = 1; i <= 15; ++i) {
            left <<= 1;
            left -= bl_count[i];
            if (left < 0) {
                zragf_free_default(NULL, table);
                return 0;
            }
        }
    }

    code = 0u;
    for (i = 1; i <= 15; ++i) {
        code = (code + (unsigned)bl_count[i - 1]) << 1;
        next_code[i] = (int)code;
    }

    count = 0;
    for (i = 0; i < nlen; ++i) {
        int l = lengths[i];
        if (l > 0) {
            unsigned c = (unsigned)next_code[l]++;
            table[count].code = zragf_reverse_bits(c, l);
            table[count].bits = l;
            table[count].symbol = i;
            count++;
        }
    }

    out_tab->table = table;
    out_tab->table_bits = table_bits;
    out_tab->count = count;
    out_tab->max_bits = max_bits;
    return 1;
}

int zragf_inflate_decode_symbol(zragf_idec_table *tab,
                                zragf_getbit_fn   getbit,
                                void             *ctx,
                                int              *out_sym)
{
    unsigned acc = 0u;
    int bits_read;
    int bit;
    int i;

    if (!tab || !getbit || !out_sym)
        return 0;
    if (!tab->table || tab->count <= 0 || tab->max_bits <= 0)
        return 0;

    for (bits_read = 1; bits_read <= tab->max_bits; ++bits_read) {
        if (!getbit(ctx, &bit))
            return 0;
        acc |= (unsigned)(bit & 1) << (bits_read - 1);

        for (i = 0; i < tab->count; ++i) {
            if (tab->table[i].bits == bits_read && tab->table[i].code == acc) {
                *out_sym = tab->table[i].symbol;
                return 1;
            }
        }
    }

    return 0;
}

int zragf_inflate_init_fixed(zragf_inflate_huff *h)
{
    int i;
    int lengths_ll[ZRAGF_INF_FIXED_LITLEN];
    int lengths_d[ZRAGF_INF_MAX_DIST];

    if (!h)
        return 0;

    for (i = 0; i < ZRAGF_INF_FIXED_LITLEN; ++i)
        lengths_ll[i] = 0;
    for (i = 0; i < ZRAGF_INF_MAX_DIST; ++i)
        lengths_d[i] = 0;

    for (i = 0; i <= 143; ++i)
        lengths_ll[i] = 8;
    for (i = 144; i <= 255; ++i)
        lengths_ll[i] = 9;
    for (i = 256; i <= 279; ++i)
        lengths_ll[i] = 7;
    for (i = 280; i <= 287; ++i)
        lengths_ll[i] = 8;
    for (i = 0; i < 30; ++i)
        lengths_d[i] = 5;

    if (!zragf_inflate_build_table(lengths_ll,
                                   ZRAGF_INF_FIXED_LITLEN,
                                   9,
                                   &h->litlen))
        return 0;

    if (!zragf_inflate_build_table(lengths_d,
                                   ZRAGF_INF_MAX_DIST,
                                   5,
                                   &h->dist)) {
        zragf_free_default(NULL, h->litlen.table);
        h->litlen.table = NULL;
        h->litlen.count = 0;
        h->litlen.max_bits = 0;
        return 0;
    }

    return 1;
}

void zragf_inflate_free(zragf_inflate_huff *h)
{
    if (!h)
        return;

    if (h->litlen.table)
        zragf_free_default(NULL, h->litlen.table);
    if (h->dist.table)
        zragf_free_default(NULL, h->dist.table);

    memset(h, 0, sizeof(*h));
}
