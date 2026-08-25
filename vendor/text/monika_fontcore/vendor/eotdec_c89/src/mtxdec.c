#include "mtxdec.h"
#include <string.h>

#define MTX_MAX_2BYTE_DIST 512UL
#define MTX_LEN_WIDTH 3
#define MTX_DIST_WIDTH 3
#define MTX_BIT_RANGE (MTX_LEN_WIDTH - 1)

struct mtx_bitio {
    const unsigned char *mem;
    unsigned long size;
    unsigned long index;
    unsigned int input_bit_count;
    unsigned int input_bit_buffer;
    unsigned long bytes_in;
    int err;
};

struct mtx_node {
    short up;
    short left;
    short right;
    short code;
    long weight;
};

struct mtx_ahuff {
    struct mtx_node tree[EOTDEC_MTX_MAX_AHUFF_RANGE * 2];
    short symbol_index[EOTDEC_MTX_MAX_AHUFF_RANGE];
    long range;
    long bit_count;
    long bit_count2;
    int max_symbol;
    long count_a;
    long count_b;
    long sym_count;
    struct mtx_bitio *bio;
    int err;
};

struct mtx_rle {
    unsigned char escape;
    unsigned char count;
    unsigned char state;
};

struct mtx_state {
    struct mtx_bitio bitin;
    struct mtx_ahuff dist;
    struct mtx_ahuff len;
    struct mtx_ahuff sym;
    struct mtx_rle rle;
    unsigned long out_len;
    unsigned long num_dist_ranges;
    unsigned long dist_max;
    unsigned long dup2;
    unsigned long dup4;
    unsigned long dup6;
    unsigned long num_syms;
    unsigned long max_copy_distance;
    int using_rle;
    int err;
};

static unsigned char g_mtx_block[EOTDEC_MTX_MAX_COMPRESSED_BLOCK];
static unsigned char g_mtx_hist[EOTDEC_MTX_MAX_COPY_DISTANCE];

static int mtx_seek_abs(FILE *fp, unsigned long off) {
    if (off > 0x7fffffffUL) return EOTDEC_ERR_RANGE;
    if (fseek(fp, (long)off, SEEK_SET) != 0) return EOTDEC_ERR_IO;
    return EOTDEC_OK;
}

static int mtx_rd_byte_eot(FILE *fp, unsigned long flags, unsigned int *v) {
    int c;
    c = fgetc(fp);
    if (c == EOF) return EOTDEC_ERR_SHORT;
    *v = (unsigned int)(c & 255);
    if ((flags & EOTDEC_FLAG_XORENCRYPTDATA) != 0UL) {
        *v = (unsigned int)((*v ^ EOTDEC_XORKEY) & 255U);
    }
    return EOTDEC_OK;
}

static unsigned long mtx_read24(const unsigned char *p) {
    return ((unsigned long)p[0] << 16) | ((unsigned long)p[1] << 8) | (unsigned long)p[2];
}

static void mtx_set_dist_range(struct mtx_state *s, unsigned long length) {
    s->num_dist_ranges = 1UL;
    s->dist_max = 1UL << (MTX_DIST_WIDTH * s->num_dist_ranges);
    while (s->dist_max < length && s->num_dist_ranges < 32UL) {
        s->num_dist_ranges++;
        if ((MTX_DIST_WIDTH * s->num_dist_ranges) >= 31UL) {
            s->dist_max = 0x7fffffffUL;
            break;
        }
        s->dist_max = 1UL << (MTX_DIST_WIDTH * s->num_dist_ranges);
    }
    s->dup2 = 256UL + (1UL << MTX_LEN_WIDTH) * s->num_dist_ranges;
    s->dup4 = s->dup2 + 1UL;
    s->dup6 = s->dup4 + 1UL;
    s->num_syms = s->dup6 + 1UL;
}

static long mtx_bits_used(unsigned long x) {
    long n;
    n = 0;
    do {
        n++;
        x >>= 1;
    } while (x != 0UL);
    return n;
}

static void mtx_bitio_init(struct mtx_bitio *b, const unsigned char *mem, unsigned long size) {
    b->mem = mem;
    b->size = size;
    b->index = 0UL;
    b->input_bit_count = 0U;
    b->input_bit_buffer = 0U;
    b->bytes_in = 0UL;
    b->err = EOTDEC_OK;
}

static int mtx_input_bit(struct mtx_bitio *b) {
    int out;
    if (b->err != EOTDEC_OK) return 0;
    if (b->input_bit_count == 0U) {
        if (b->index >= b->size) {
            b->err = EOTDEC_ERR_SHORT;
            return 0;
        }
        b->input_bit_buffer = (unsigned int)b->mem[b->index++];
        b->input_bit_count = 8U;
        b->bytes_in++;
    }
    b->input_bit_buffer <<= 1;
    b->input_bit_count--;
    out = (b->input_bit_buffer & 0x100U) ? 1 : 0;
    return out;
}

static unsigned long mtx_read_value(struct mtx_bitio *b, long nbits) {
    unsigned long value;
    long i;
    value = 0UL;
    for (i = nbits; i > 0; i--) {
        value <<= 1;
        if (mtx_input_bit(b)) value |= 1UL;
    }
    return value;
}

static long mtx_ahuff_init_weight(struct mtx_ahuff *a, int idx) {
    if (a->tree[idx].code < 0) {
        a->tree[idx].weight = mtx_ahuff_init_weight(a, a->tree[idx].left) +
                              mtx_ahuff_init_weight(a, a->tree[idx].right);
    }
    return a->tree[idx].weight;
}

static void mtx_ahuff_swap_nodes(struct mtx_ahuff *a, short x, short y) {
    short code;
    short upx;
    short upy;
    struct mtx_node tmp;

    if (x <= 1 || y <= 1 || x == y) {
        a->err = EOTDEC_ERR_MTX_CORRUPT;
        return;
    }
    upx = a->tree[x].up;
    upy = a->tree[y].up;

    tmp = a->tree[x];
    a->tree[x] = a->tree[y];
    a->tree[y] = tmp;
    a->tree[x].up = upx;
    a->tree[y].up = upy;

    code = a->tree[x].code;
    if (code < 0) {
        a->tree[a->tree[x].left].up = x;
        a->tree[a->tree[x].right].up = x;
    } else if ((unsigned int)code < (unsigned int)a->range) {
        a->symbol_index[code] = x;
    } else {
        a->err = EOTDEC_ERR_MTX_CORRUPT;
        return;
    }

    code = a->tree[y].code;
    if (code < 0) {
        a->tree[a->tree[y].left].up = y;
        a->tree[a->tree[y].right].up = y;
    } else if ((unsigned int)code < (unsigned int)a->range) {
        a->symbol_index[code] = y;
    } else {
        a->err = EOTDEC_ERR_MTX_CORRUPT;
        return;
    }
}

static void mtx_ahuff_update_weight(struct mtx_ahuff *a, short idx) {
    short b;
    long weight;
    if (a->err != EOTDEC_OK) return;
    while (idx != 1) {
        weight = a->tree[idx].weight;
        b = (short)(idx - 1);
        if (b > 0 && a->tree[b].weight == weight) {
            do {
                b--;
            } while (b > 0 && a->tree[b].weight == weight);
            b++;
            if (b > 1) {
                mtx_ahuff_swap_nodes(a, idx, b);
                idx = b;
                if (a->err != EOTDEC_OK) return;
            }
        }
        a->tree[idx].weight = weight + 1L;
        idx = a->tree[idx].up;
    }
    a->tree[1].weight++;
}

static int mtx_ahuff_init(struct mtx_ahuff *a, struct mtx_bitio *bio, short range_in) {
    short i;
    short limit;
    short range;
    long j;

    if (range_in <= 0 || range_in > EOTDEC_MTX_MAX_AHUFF_RANGE) return EOTDEC_ERR_MTX_LIMIT;
    memset(a, 0, sizeof(*a));
    a->bio = bio;
    range = range_in;
    a->range = (long)range_in;
    a->bit_count = mtx_bits_used((unsigned long)(range_in - 1));
    a->bit_count2 = 0L;
    if (range_in > 256 && range_in < 512) {
        short tmp;
        tmp = (short)(range_in - 256);
        a->bit_count2 = mtx_bits_used((unsigned long)(tmp - 1)) + 1L;
    }
    a->max_symbol = range - 1;
    a->sym_count = 0L;
    a->count_a = 100L;
    a->count_b = 100L;
    a->err = EOTDEC_OK;

    a->tree[0].weight = -1L;
    limit = (short)(2 * range);
    for (i = 2; i < limit; i++) {
        a->tree[i].up = (short)(i / 2);
        a->tree[i].weight = 1L;
    }
    for (i = 1; i < range; i++) {
        a->tree[i].left = (short)(2 * i);
        a->tree[i].right = (short)(2 * i + 1);
        a->tree[i].code = -1;
    }
    for (i = 0; i < range; i++) {
        a->tree[range + i].code = i;
        a->tree[range + i].left = -1;
        a->tree[range + i].right = -1;
        a->symbol_index[i] = (short)(range + i);
    }
    mtx_ahuff_init_weight(a, 1);

    if (a->bit_count2 != 0L) {
        if (range <= 258) return EOTDEC_ERR_MTX_LIMIT;
        mtx_ahuff_update_weight(a, a->symbol_index[256]);
        mtx_ahuff_update_weight(a, a->symbol_index[257]);
        for (i = 0; i < 12; i++) mtx_ahuff_update_weight(a, a->symbol_index[range - 3]);
        for (i = 0; i < 6; i++) mtx_ahuff_update_weight(a, a->symbol_index[range - 2]);
    } else {
        for (j = 0; j < 2L; j++) {
            for (i = 0; i < range; i++) mtx_ahuff_update_weight(a, a->symbol_index[i]);
        }
    }
    a->count_a = 0L;
    a->count_b = 0L;
    return a->err;
}

static short mtx_ahuff_read_symbol(struct mtx_ahuff *a) {
    short idx;
    short symbol;
    if (a->err != EOTDEC_OK || a->bio->err != EOTDEC_OK) return 0;
    idx = 1;
    do {
        idx = (short)(mtx_input_bit(a->bio) ? a->tree[idx].right : a->tree[idx].left);
        if (idx <= 0 || idx >= (short)(2 * a->range)) {
            a->err = EOTDEC_ERR_MTX_CORRUPT;
            return 0;
        }
        symbol = a->tree[idx].code;
    } while (symbol < 0);
    mtx_ahuff_update_weight(a, idx);
    return symbol;
}

static long mtx_decode_length(struct mtx_state *s, int first_symbol, unsigned long *num_dist_ranges) {
    unsigned long mask;
    unsigned long value;
    long bits;
    int first_time;
    int done;

    mask = 1UL << MTX_BIT_RANGE;
    value = 0UL;
    first_time = 1;
    done = 0;
    do {
        if (first_time) {
            bits = (long)first_symbol - 256L;
            first_time = 0;
            if (bits < 0) {
                s->err = EOTDEC_ERR_MTX_CORRUPT;
                return 0L;
            }
            *num_dist_ranges = (unsigned long)(bits / (1L << MTX_LEN_WIDTH)) + 1UL;
            if (*num_dist_ranges < 1UL || *num_dist_ranges > s->num_dist_ranges) {
                s->err = EOTDEC_ERR_MTX_CORRUPT;
                return 0L;
            }
            bits = bits % (1L << MTX_LEN_WIDTH);
        } else {
            bits = (long)mtx_ahuff_read_symbol(&s->len);
        }
        done = ((unsigned long)bits & mask) == 0UL;
        bits &= (long)(~mask);
        value <<= MTX_BIT_RANGE;
        value |= (unsigned long)bits;
    } while (!done && s->err == EOTDEC_OK && s->len.err == EOTDEC_OK && s->bitin.err == EOTDEC_OK);
    return (long)(value + 2UL);
}

static unsigned long mtx_decode_distance(struct mtx_state *s, unsigned long dist_ranges) {
    unsigned long i;
    unsigned long bits;
    unsigned long value;
    value = 0UL;
    for (i = dist_ranges; i > 0UL; i--) {
        bits = (unsigned long)mtx_ahuff_read_symbol(&s->dist);
        value <<= MTX_DIST_WIDTH;
        value |= bits;
    }
    value += 1UL;
    return value;
}

static void mtx_init_history(unsigned long copy_distance) {
    unsigned long i;
    unsigned long j;
    unsigned long k;
    i = 0UL;
    for (k = 0UL; k < 32UL; k++) {
        for (j = 0UL; j < 96UL; j++) {
            if (i < copy_distance) g_mtx_hist[i] = (unsigned char)k;
            i++;
            if (i < copy_distance) g_mtx_hist[i] = (unsigned char)j;
            i++;
        }
    }
    j = 0UL;
    while (i < EOTDEC_MTX_PRELOAD_SIZE && j < 256UL) {
        if (i < copy_distance) g_mtx_hist[i] = (unsigned char)j;
        i++;
        if (i < copy_distance) g_mtx_hist[i] = (unsigned char)j;
        i++;
        if (i < copy_distance) g_mtx_hist[i] = (unsigned char)j;
        i++;
        if (i < copy_distance) g_mtx_hist[i] = (unsigned char)j;
        i++;
        j++;
    }
}

static unsigned long mtx_mod_index(long idx, unsigned long modulo) {
    while (idx < 0L) idx += (long)modulo;
    while ((unsigned long)idx >= modulo) idx -= (long)modulo;
    return (unsigned long)idx;
}

static int mtx_direct_put(FILE *out, unsigned char value, unsigned long *count) {
    if (fputc((int)value, out) == EOF) return EOTDEC_ERR_IO;
    (*count)++;
    return EOTDEC_OK;
}

static int mtx_rle_put(struct mtx_rle *r, FILE *out, unsigned char value, unsigned long *count) {
    int ret;
    if (r->state == 100U) {
        r->escape = value;
        r->state = 0U;
        return EOTDEC_OK;
    }
    if (r->state == 0U) {
        if (value == r->escape) {
            r->state = 1U;
        } else {
            ret = mtx_direct_put(out, value, count);
            if (ret != EOTDEC_OK) return ret;
        }
    } else if (r->state == 1U) {
        r->count = value;
        if (r->count == 0U) {
            ret = mtx_direct_put(out, r->escape, count);
            if (ret != EOTDEC_OK) return ret;
            r->state = 0U;
        } else {
            r->state = 2U;
        }
    } else if (r->state == 2U) {
        unsigned int i;
        for (i = 0U; i < (unsigned int)r->count; i++) {
            ret = mtx_direct_put(out, value, count);
            if (ret != EOTDEC_OK) return ret;
        }
        r->state = 0U;
    } else {
        return EOTDEC_ERR_MTX_CORRUPT;
    }
    return EOTDEC_OK;
}

static int mtx_emit_decoded(struct mtx_state *s, FILE *out, unsigned char value, unsigned long *raw_count) {
    if (s->using_rle) {
        return mtx_rle_put(&s->rle, out, value, raw_count);
    }
    return mtx_direct_put(out, value, raw_count);
}

static int mtx_lzcomp_unpack_to_file(const unsigned char *src, unsigned long src_size,
                                     unsigned int version, unsigned long copy_limit,
                                     FILE *out, unsigned long *raw_size) {
    struct mtx_state s;
    unsigned long pos;
    unsigned long dst;
    unsigned long num_dist_ranges;
    unsigned long distance;
    unsigned long length;
    unsigned long j;
    unsigned long src_idx;
    long start;
    short symbol;
    unsigned char value;
    int r;

    memset(&s, 0, sizeof(s));
    *raw_size = 0UL;

    s.max_copy_distance = copy_limit;
    if (s.max_copy_distance < (EOTDEC_MTX_PRELOAD_SIZE + 64UL)) s.max_copy_distance = EOTDEC_MTX_PRELOAD_SIZE + 64UL;
    if (s.max_copy_distance > EOTDEC_MTX_MAX_COPY_DISTANCE) return EOTDEC_ERR_MTX_LIMIT;

    mtx_bitio_init(&s.bitin, src, src_size);
    if (version == 1U) {
        s.using_rle = 0;
    } else {
        s.using_rle = mtx_input_bit(&s.bitin) ? 1 : 0;
    }
    r = mtx_ahuff_init(&s.dist, &s.bitin, (short)(1 << MTX_DIST_WIDTH));
    if (r != EOTDEC_OK) return r;
    r = mtx_ahuff_init(&s.len, &s.bitin, (short)(1 << MTX_LEN_WIDTH));
    if (r != EOTDEC_OK) return r;
    s.out_len = mtx_read_value(&s.bitin, 24L);
    if (s.bitin.err != EOTDEC_OK) return s.bitin.err;
    mtx_set_dist_range(&s, s.out_len);
    if (s.num_syms > EOTDEC_MTX_MAX_AHUFF_RANGE) return EOTDEC_ERR_MTX_LIMIT;
    r = mtx_ahuff_init(&s.sym, &s.bitin, (short)s.num_syms);
    if (r != EOTDEC_OK) return r;

    s.rle.state = 100U;
    mtx_init_history(s.max_copy_distance);
    dst = EOTDEC_MTX_PRELOAD_SIZE;
    if (dst >= s.max_copy_distance) dst %= s.max_copy_distance;

    for (pos = 0UL; pos < s.out_len && s.err == EOTDEC_OK; ) {
        symbol = mtx_ahuff_read_symbol(&s.sym);
        if (s.sym.err != EOTDEC_OK) return s.sym.err;
        if (s.bitin.err != EOTDEC_OK) return s.bitin.err;
        if (symbol < 256) {
            value = (unsigned char)symbol;
            g_mtx_hist[dst] = value;
            dst++;
            if (dst >= s.max_copy_distance) dst = 0UL;
            pos++;
            r = mtx_emit_decoded(&s, out, value, raw_size);
            if (r != EOTDEC_OK) return r;
        } else if ((unsigned long)symbol == s.dup2 || (unsigned long)symbol == s.dup4 || (unsigned long)symbol == s.dup6) {
            unsigned long back;
            if ((unsigned long)symbol == s.dup2) back = 2UL;
            else if ((unsigned long)symbol == s.dup4) back = 4UL;
            else back = 6UL;
            src_idx = (dst >= back) ? (dst - back) : (s.max_copy_distance + dst - back);
            value = g_mtx_hist[src_idx];
            g_mtx_hist[dst] = value;
            dst++;
            if (dst >= s.max_copy_distance) dst = 0UL;
            pos++;
            r = mtx_emit_decoded(&s, out, value, raw_size);
            if (r != EOTDEC_OK) return r;
        } else {
            length = (unsigned long)mtx_decode_length(&s, symbol, &num_dist_ranges);
            if (s.err != EOTDEC_OK) return s.err;
            if (s.len.err != EOTDEC_OK) return s.len.err;
            if (s.bitin.err != EOTDEC_OK) return s.bitin.err;
            distance = mtx_decode_distance(&s, num_dist_ranges);
            if (s.dist.err != EOTDEC_OK) return s.dist.err;
            if (s.bitin.err != EOTDEC_OK) return s.bitin.err;
            if (distance >= MTX_MAX_2BYTE_DIST) length++;
            if (length == 0UL || pos + length > s.out_len) return EOTDEC_ERR_MTX_CORRUPT;
            if (distance + length - 1UL > s.max_copy_distance) return EOTDEC_ERR_MTX_CORRUPT;
            start = (long)dst - (long)distance - (long)length + 1L;
            for (j = 0UL; j < length; j++) {
                src_idx = mtx_mod_index(start + (long)j, s.max_copy_distance);
                value = g_mtx_hist[src_idx];
                g_mtx_hist[dst] = value;
                dst++;
                if (dst >= s.max_copy_distance) dst = 0UL;
                pos++;
                r = mtx_emit_decoded(&s, out, value, raw_size);
                if (r != EOTDEC_OK) return r;
            }
        }
    }

    if (s.bitin.err != EOTDEC_OK) return s.bitin.err;
    if (s.using_rle && s.rle.state != 0U) return EOTDEC_ERR_MTX_CORRUPT;
    return EOTDEC_OK;
}

int eotdec_mtx_probe_file(FILE *fp, const struct eotdec_info *eot, struct eotdec_mtx_info *mtx) {
    unsigned int v;
    unsigned long i;
    int r;
    unsigned char h[10];

    if (fp == 0 || eot == 0 || mtx == 0) return EOTDEC_ERR_IO;
    memset(mtx, 0, sizeof(*mtx));
    if ((eot->flags & EOTDEC_FLAG_TTCOMPRESSED) == 0UL) return EOTDEC_ERR_UNSUPPORTED;
    if (eot->font_data_size < 10UL) return EOTDEC_ERR_MTX_CORRUPT;
    r = mtx_seek_abs(fp, eot->font_offset);
    if (r != EOTDEC_OK) return r;
    for (i = 0UL; i < 10UL; i++) {
        r = mtx_rd_byte_eot(fp, eot->flags, &v);
        if (r != EOTDEC_OK) return r;
        h[i] = (unsigned char)v;
    }
    mtx->version = (unsigned int)h[0];
    mtx->copy_limit = mtx_read24(h + 1);
    mtx->offset_data2 = mtx_read24(h + 4);
    mtx->offset_data3 = mtx_read24(h + 7);
    mtx->mtx_size = eot->font_data_size;

    if (mtx->version == 0U) return EOTDEC_ERR_MTX_CORRUPT;
    if (mtx->offset_data2 < 10UL || mtx->offset_data3 < mtx->offset_data2 || mtx->offset_data3 > mtx->mtx_size) return EOTDEC_ERR_MTX_CORRUPT;
    if (mtx->copy_limit == 0UL) return EOTDEC_ERR_MTX_CORRUPT;
    mtx->block_comp_size[0] = mtx->offset_data2 - 10UL;
    mtx->block_comp_size[1] = mtx->offset_data3 - mtx->offset_data2;
    mtx->block_comp_size[2] = mtx->mtx_size - mtx->offset_data3;
    return EOTDEC_OK;
}

static int mtx_read_block(FILE *fp, const struct eotdec_info *eot, unsigned long mtx_rel_off, unsigned long len) {
    unsigned long i;
    unsigned int v;
    int r;
    if (len > EOTDEC_MTX_MAX_COMPRESSED_BLOCK) return EOTDEC_ERR_MTX_LIMIT;
    r = mtx_seek_abs(fp, eot->font_offset + mtx_rel_off);
    if (r != EOTDEC_OK) return r;
    for (i = 0UL; i < len; i++) {
        r = mtx_rd_byte_eot(fp, eot->flags, &v);
        if (r != EOTDEC_OK) return r;
        g_mtx_block[i] = (unsigned char)v;
    }
    return EOTDEC_OK;
}

static int mtx_make_name(char *out, unsigned long out_sz, const char *prefix, int block_no) {
    const char *suffix;
    unsigned long a;
    unsigned long b;
    unsigned long i;
    if (block_no == 1) suffix = ".block1_font_tables.ctf";
    else if (block_no == 2) suffix = ".block2_push_data.ctf";
    else suffix = ".block3_glyph_insns.ctf";
    a = (unsigned long)strlen(prefix);
    b = (unsigned long)strlen(suffix);
    if (a + b + 1UL > out_sz) return EOTDEC_ERR_RANGE;
    for (i = 0UL; i < a; i++) out[i] = prefix[i];
    for (i = 0UL; i < b; i++) out[a + i] = suffix[i];
    out[a + b] = '\0';
    return EOTDEC_OK;
}

int eotdec_mtx_unpack_blocks_file(FILE *fp, const struct eotdec_info *eot, const char *prefix, struct eotdec_mtx_info *mtx_out) {
    struct eotdec_mtx_info mtx;
    char name[512];
    FILE *out;
    int r;
    int b;
    unsigned long rel_off[3];

    if (fp == 0 || eot == 0 || prefix == 0) return EOTDEC_ERR_IO;
    r = eotdec_mtx_probe_file(fp, eot, &mtx);
    if (r != EOTDEC_OK) return r;
    if (mtx.copy_limit > EOTDEC_MTX_MAX_COPY_DISTANCE) return EOTDEC_ERR_MTX_LIMIT;

    rel_off[0] = 10UL;
    rel_off[1] = mtx.offset_data2;
    rel_off[2] = mtx.offset_data3;

    for (b = 0; b < 3; b++) {
        r = mtx_read_block(fp, eot, rel_off[b], mtx.block_comp_size[b]);
        if (r != EOTDEC_OK) return r;
        r = mtx_make_name(name, sizeof(name), prefix, b + 1);
        if (r != EOTDEC_OK) return r;
        out = fopen(name, "wb");
        if (!out) return EOTDEC_ERR_IO;
        r = mtx_lzcomp_unpack_to_file(g_mtx_block, mtx.block_comp_size[b], mtx.version, mtx.copy_limit, out, &mtx.block_raw_size[b]);
        if (fclose(out) != 0 && r == EOTDEC_OK) r = EOTDEC_ERR_IO;
        if (r != EOTDEC_OK) return r;
    }

    if (mtx_out != 0) *mtx_out = mtx;
    return EOTDEC_OK;
}

void eotdec_mtx_print_info(FILE *out, const struct eotdec_mtx_info *m) {
    if (out == 0 || m == 0) return;
    fprintf(out, "MTX / MicroType Express\n");
    fprintf(out, "  version:         %u\n", m->version);
    fprintf(out, "  copy_limit:      %lu\n", m->copy_limit);
    fprintf(out, "  mtx_size:        %lu\n", m->mtx_size);
    fprintf(out, "  offset_data2:    %lu\n", m->offset_data2);
    fprintf(out, "  offset_data3:    %lu\n", m->offset_data3);
    fprintf(out, "  block1_comp:     %lu\n", m->block_comp_size[0]);
    fprintf(out, "  block2_comp:     %lu\n", m->block_comp_size[1]);
    fprintf(out, "  block3_comp:     %lu\n", m->block_comp_size[2]);
    if (m->block_raw_size[0] || m->block_raw_size[1] || m->block_raw_size[2]) {
        fprintf(out, "  block1_raw:      %lu\n", m->block_raw_size[0]);
        fprintf(out, "  block2_raw:      %lu\n", m->block_raw_size[1]);
        fprintf(out, "  block3_raw:      %lu\n", m->block_raw_size[2]);
    }
}
