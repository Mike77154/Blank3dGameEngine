#include "qoi89_internal.h"

const unsigned char qoi89_padding[QOI89_PADDING_SIZE] = {
    0u, 0u, 0u, 0u, 0u, 0u, 0u, 1u
};

unsigned int qoi89_hash_rgba(const qoi89_rgba *px) {
    unsigned int hash;

    hash = (unsigned int)px->r * 3u;
    hash += (unsigned int)px->g * 5u;
    hash += (unsigned int)px->b * 7u;
    hash += (unsigned int)px->a * 11u;
    return hash & 63u;
}

int qoi89_rgba_equal(const qoi89_rgba *a, const qoi89_rgba *b) {
    return a->r == b->r && a->g == b->g && a->b == b->b && a->a == b->a;
}

int qoi89_wrap_diff_u8(unsigned int cur, unsigned int prev) {
    unsigned int diff;

    diff = (cur - prev) & 0xffu;
    if (diff >= 128u) {
        return (int)diff - 256;
    }
    return (int)diff;
}

void qoi89_zero_index(qoi89_rgba *index_table) {
    unsigned int i;

    for (i = 0u; i < QOI89_INDEX_SIZE; ++i) {
        index_table[i].r = 0u;
        index_table[i].g = 0u;
        index_table[i].b = 0u;
        index_table[i].a = 0u;
    }
}

void qoi89_rgba_from_bytes(qoi89_rgba *px, const unsigned char *src, unsigned int channels) {
    px->r = src[0];
    px->g = src[1];
    px->b = src[2];
    if (channels == 4u) {
        px->a = src[3];
    }
    else {
        px->a = 255u;
    }
}

void qoi89_rgba_to_bytes(const qoi89_rgba *px, unsigned char *dst, unsigned int channels) {
    dst[0] = px->r;
    dst[1] = px->g;
    dst[2] = px->b;
    if (channels == 4u) {
        dst[3] = px->a;
    }
}

void qoi89_write_u32be(unsigned char *dst, size_t *pos, unsigned long value) {
    dst[(*pos)++] = (unsigned char)((value >> 24) & 0xfful);
    dst[(*pos)++] = (unsigned char)((value >> 16) & 0xfful);
    dst[(*pos)++] = (unsigned char)((value >> 8) & 0xfful);
    dst[(*pos)++] = (unsigned char)(value & 0xfful);
}

unsigned long qoi89_read_u32be(const unsigned char *src, size_t *pos) {
    unsigned long a;
    unsigned long b;
    unsigned long c;
    unsigned long d;

    a = (unsigned long)src[(*pos)++];
    b = (unsigned long)src[(*pos)++];
    c = (unsigned long)src[(*pos)++];
    d = (unsigned long)src[(*pos)++];

    return (a << 24) | (b << 16) | (c << 8) | d;
}

int qoi89_valid_channels(unsigned int channels) {
    return channels == 3u || channels == 4u;
}

int qoi89_mul_size_t(size_t a, size_t b, size_t *out) {
    size_t max_value;

    if (out == NULL) {
        return 1;
    }

    max_value = (size_t)-1;
    if (a != 0u && b > max_value / a) {
        return 1;
    }

    *out = a * b;
    return 0;
}

int qoi89_add_size_t(size_t a, size_t b, size_t *out) {
    size_t max_value;

    if (out == NULL) {
        return 1;
    }

    max_value = (size_t)-1;
    if (b > max_value - a) {
        return 1;
    }

    *out = a + b;
    return 0;
}

int qoi89_need_space(size_t pos, size_t cap, size_t need) {
    return pos > cap || need > cap - pos;
}
