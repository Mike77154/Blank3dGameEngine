#include "zragf_huff.h"

typedef struct zragf_huff_node_s {
    zragf_u32 freq;
    int left;
    int right;
    int sym;
} zragf_huff_node;

static void
zragf_huff_clear_tables(zragf_huff_tables *ht)
{
    zragf_u32 i;
    for (i = 0; i < ZRAGF_HUFF_SYMS; ++i) {
        ht->code[i] = 0;
        ht->bits[i] = 0;
    }
}

static void
zragf_huff_build_lengths_from_tree(zragf_huff_node *nodes,
                                   int root,
                                   zragf_u8 *lens,
                                   zragf_u8 depth,
                                   zragf_u8 *out_max_bits)
{
    if (root < 0)
        return;
    if (nodes[root].left < 0 && nodes[root].right < 0) {
        int s = nodes[root].sym;
        if (s >= 0 && s < (int)ZRAGF_HUFF_SYMS) {
            if (depth == 0)
                depth = 1;
            lens[s] = depth;
            if (depth > *out_max_bits)
                *out_max_bits = depth;
        }
        return;
    }
    zragf_huff_build_lengths_from_tree(nodes, nodes[root].left, lens, (zragf_u8)(depth + 1u), out_max_bits);
    zragf_huff_build_lengths_from_tree(nodes, nodes[root].right, lens, (zragf_u8)(depth + 1u), out_max_bits);
}

static zragf_status
zragf_huff_build_canonical(const zragf_u8 *lens,
                           zragf_u8 max_bits,
                           zragf_huff_tables *ht)
{
    zragf_u32 bl_count[ZRAGF_HUFF_MAX_BITS + 1];
    zragf_u16 next_code[ZRAGF_HUFF_MAX_BITS + 1];
    zragf_u32 i;
    zragf_u16 code = 0;
    if (!ht || !lens)
        return ZRAGF_ERR_NULL_POINTER;
    zragf_huff_clear_tables(ht);
    if (max_bits == 0 || max_bits > ZRAGF_HUFF_MAX_BITS)
        return ZRAGF_ERR_INTERNAL;
    for (i = 0; i <= ZRAGF_HUFF_MAX_BITS; ++i)
        bl_count[i] = 0;
    for (i = 0; i < ZRAGF_HUFF_SYMS; ++i) {
        zragf_u8 l = lens[i];
        if (l > 0) {
            if (l > ZRAGF_HUFF_MAX_BITS)
                return ZRAGF_ERR_INTERNAL;
            bl_count[l]++;
        }
    }
    for (i = 0; i <= ZRAGF_HUFF_MAX_BITS; ++i)
        next_code[i] = 0;
    for (i = 1; i <= max_bits; ++i) {
        code = (zragf_u16)((code + bl_count[i - 1]) << 1);
        next_code[i] = code;
    }
    for (i = 0; i < ZRAGF_HUFF_SYMS; ++i) {
        zragf_u8 l = lens[i];
        if (l != 0) {
            zragf_u16 c = next_code[l];
            ht->code[i] = c;
            ht->bits[i] = l;
            next_code[l] = (zragf_u16)(c + 1);
        }
    }
    return ZRAGF_OK;
}

zragf_status
zragf_huff_build_from_freq(const zragf_u32 *freq,
                           zragf_huff_tables *ht,
                           zragf_u8 *out_max_bits)
{
    zragf_huff_node nodes[ZRAGF_HUFF_SYMS * 2];
    int active[ZRAGF_HUFF_SYMS * 2];
    zragf_u8 lens[ZRAGF_HUFF_SYMS];
    int node_count = 0;
    int active_count = 0;
    int i;
    zragf_u8 max_bits = 0;
    if (!freq || !ht || !out_max_bits)
        return ZRAGF_ERR_NULL_POINTER;
    for (i = 0; i < (int)ZRAGF_HUFF_SYMS; ++i)
        lens[i] = 0;
    for (i = 0; i < (int)ZRAGF_HUFF_SYMS * 2; ++i) {
        nodes[i].freq = 0;
        nodes[i].left = -1;
        nodes[i].right = -1;
        nodes[i].sym = -1;
        active[i] = -1;
    }
    for (i = 0; i < (int)ZRAGF_HUFF_SYMS; ++i) {
        if (freq[i] != 0) {
            nodes[node_count].freq = freq[i];
            nodes[node_count].left = -1;
            nodes[node_count].right = -1;
            nodes[node_count].sym = i;
            active[active_count++] = node_count;
            node_count++;
        }
    }
    if (active_count == 0) {
        *out_max_bits = 1;
        for (i = 0; i < (int)ZRAGF_HUFF_SYMS; ++i) {
            lens[i] = 0;
        }
        lens[0] = 1;
        zragf_huff_clear_tables(ht);
        ht->code[0] = 0;
        ht->bits[0] = 1;
        *out_max_bits = 1;
        return ZRAGF_OK;
    }
    if (active_count == 1) {
        int idx = active[0];
        int s = nodes[idx].sym;
        for (i = 0; i < (int)ZRAGF_HUFF_SYMS; ++i)
            lens[i] = 0;
        lens[s] = 1;
        max_bits = 1;
        *out_max_bits = max_bits;
        return zragf_huff_build_canonical(lens, max_bits, ht);
    }
    while (active_count > 1) {
        int min1 = -1;
        int min2 = -1;
        int j;
        for (j = 0; j < active_count; ++j) {
            int idx = active[j];
            if (min1 < 0 || nodes[idx].freq < nodes[min1].freq)
                min1 = idx;
        }
        for (j = 0; j < active_count; ++j) {
            int idx = active[j];
            if (idx == min1)
                continue;
            if (min2 < 0 || nodes[idx].freq < nodes[min2].freq)
                min2 = idx;
        }
        nodes[node_count].freq = nodes[min1].freq + nodes[min2].freq;
        nodes[node_count].left = min1;
        nodes[node_count].right = min2;
        nodes[node_count].sym = -1;
        {
            int new_active[ZRAGF_HUFF_SYMS * 2];
            int new_count = 0;
            int k;
            for (k = 0; k < active_count; ++k) {
                int idx = active[k];
                if (idx == min1 || idx == min2)
                    continue;
                new_active[new_count++] = idx;
            }
            new_active[new_count++] = node_count;
            for (k = 0; k < new_count; ++k)
                active[k] = new_active[k];
            active_count = new_count;
        }
        node_count++;
        if (node_count >= (int)(ZRAGF_HUFF_SYMS * 2))
            return ZRAGF_ERR_INTERNAL;
    }
    for (i = 0; i < (int)ZRAGF_HUFF_SYMS; ++i)
        lens[i] = 0;
    max_bits = 0;
    zragf_huff_build_lengths_from_tree(nodes, active[0], lens, 0, &max_bits);
    if (max_bits == 0)
        max_bits = 1;
    if (max_bits > ZRAGF_HUFF_MAX_BITS)
        return ZRAGF_ERR_INTERNAL;
    *out_max_bits = max_bits;
    return zragf_huff_build_canonical(lens, max_bits, ht);
}

zragf_status
zragf_huff_build_from_lengths(const zragf_u8 *lens,
                              zragf_u8 max_bits,
                              zragf_huff_tables *ht)
{
    if (!lens || !ht)
        return ZRAGF_ERR_NULL_POINTER;
    if (max_bits == 0 || max_bits > ZRAGF_HUFF_MAX_BITS)
        return ZRAGF_ERR_INTERNAL;
    return zragf_huff_build_canonical(lens, max_bits, ht);
}
