#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test_common.h"

static const unsigned char vec_pixels_rgb_force[] = {
    0xc8u, 0x0au, 0x96u
};
static const unsigned char vec_qoi_rgb_force[] = {
    0x71u,0x6fu,0x69u,0x66u,0x00u,0x00u,0x00u,0x01u,0x00u,0x00u,0x00u,0x01u,0x03u,0x00u,
    0xfeu,0xc8u,0x0au,0x96u,
    0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x01u
};

static const unsigned char vec_pixels_rgba1[] = {
    0x01u, 0x02u, 0x03u, 0x04u
};
static const unsigned char vec_qoi_rgba1[] = {
    0x71u,0x6fu,0x69u,0x66u,0x00u,0x00u,0x00u,0x01u,0x00u,0x00u,0x00u,0x01u,0x04u,0x00u,
    0xffu,0x01u,0x02u,0x03u,0x04u,
    0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x01u
};

static const unsigned char vec_pixels_diff2[] = {
    0x0au,0x14u,0x1eu,0xffu,
    0x0bu,0x13u,0x1fu,0xffu
};
static const unsigned char vec_qoi_diff2[] = {
    0x71u,0x6fu,0x69u,0x66u,0x00u,0x00u,0x00u,0x02u,0x00u,0x00u,0x00u,0x01u,0x04u,0x00u,
    0xfeu,0x0au,0x14u,0x1eu,0x77u,
    0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x01u
};

static const unsigned char vec_pixels_luma2[] = {
    0x32u,0x32u,0x32u,0xffu,
    0x38u,0x3cu,0x39u,0xffu
};
static const unsigned char vec_qoi_luma2[] = {
    0x71u,0x6fu,0x69u,0x66u,0x00u,0x00u,0x00u,0x02u,0x00u,0x00u,0x00u,0x01u,0x04u,0x00u,
    0xfeu,0x32u,0x32u,0x32u,0xaau,0x45u,
    0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x01u
};

static const unsigned char vec_pixels_index3[] = {
    0x01u,0x02u,0x03u,0xffu,
    0x04u,0x05u,0x06u,0xffu,
    0x01u,0x02u,0x03u,0xffu
};
static const unsigned char vec_qoi_index3[] = {
    0x71u,0x6fu,0x69u,0x66u,0x00u,0x00u,0x00u,0x03u,0x00u,0x00u,0x00u,0x01u,0x04u,0x00u,
    0xa2u,0x79u,0xa3u,0x88u,0x17u,
    0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x01u
};

static const unsigned char vec_pixels_run3[] = {
    0x09u,0x08u,0x07u,0xffu,
    0x09u,0x08u,0x07u,0xffu,
    0x09u,0x08u,0x07u,0xffu
};
static const unsigned char vec_qoi_run3[] = {
    0x71u,0x6fu,0x69u,0x66u,0x00u,0x00u,0x00u,0x03u,0x00u,0x00u,0x00u,0x01u,0x04u,0x00u,
    0xa8u,0x97u,0xc1u,
    0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x00u,0x01u
};

const struct vector_case test_vectors[] = {
    { "rgb_force", { 1ul, 1ul, 3u, 0u }, vec_pixels_rgb_force, sizeof(vec_pixels_rgb_force), vec_qoi_rgb_force, sizeof(vec_qoi_rgb_force) },
    { "rgba1",     { 1ul, 1ul, 4u, 0u }, vec_pixels_rgba1,     sizeof(vec_pixels_rgba1),     vec_qoi_rgba1,     sizeof(vec_qoi_rgba1) },
    { "diff2",     { 2ul, 1ul, 4u, 0u }, vec_pixels_diff2,     sizeof(vec_pixels_diff2),     vec_qoi_diff2,     sizeof(vec_qoi_diff2) },
    { "luma2",     { 2ul, 1ul, 4u, 0u }, vec_pixels_luma2,     sizeof(vec_pixels_luma2),     vec_qoi_luma2,     sizeof(vec_qoi_luma2) },
    { "index3",    { 3ul, 1ul, 4u, 0u }, vec_pixels_index3,    sizeof(vec_pixels_index3),    vec_qoi_index3,    sizeof(vec_qoi_index3) },
    { "run3",      { 3ul, 1ul, 4u, 0u }, vec_pixels_run3,      sizeof(vec_pixels_run3),      vec_qoi_run3,      sizeof(vec_qoi_run3) }
};

const size_t test_vectors_count = sizeof(test_vectors) / sizeof(test_vectors[0]);

static unsigned long test_seed = 0x12345678ul;

void test_fail(const char *where, const char *msg) {
    fprintf(stderr, "[FAIL] %s: %s\n", where, msg);
    exit(1);
}

void test_expect_status(const char *where, qoi89_status got, qoi89_status expected) {
    if (got != expected) {
        fprintf(stderr, "[FAIL] %s: expected %s, got %s\n",
            where, qoi89_status_string(expected), qoi89_status_string(got));
        exit(1);
    }
}

void test_expect_mem(const char *where, const void *a, const void *b, size_t n) {
    if (memcmp(a, b, n) != 0) {
        fprintf(stderr, "[FAIL] %s: memory mismatch (%lu bytes)\n", where, (unsigned long)n);
        exit(1);
    }
}

static unsigned int test_next_rand8(void) {
    test_seed = test_seed * 1664525ul + 1013904223ul;
    return (unsigned int)((test_seed >> 16) & 0xfful);
}

void test_fill_pattern(
    unsigned char *pixels,
    size_t pixels_len,
    unsigned int channels,
    unsigned long case_id
) {
    size_t pos;
    unsigned char last_r;
    unsigned char last_g;
    unsigned char last_b;
    unsigned char last_a;

    last_r = 0u;
    last_g = 0u;
    last_b = 0u;
    last_a = 255u;

    for (pos = 0u; pos < pixels_len; pos += (size_t)channels) {
        unsigned int mode;
        unsigned char r;
        unsigned char g;
        unsigned char b;
        unsigned char a;

        mode = (unsigned int)((case_id + (unsigned long)(pos / (size_t)channels)) % 7ul);
        if (mode == 0u && pos != 0u) {
            r = last_r;
            g = last_g;
            b = last_b;
            a = last_a;
        }
        else if (mode == 1u) {
            r = (unsigned char)((unsigned int)last_r + 1u);
            g = (unsigned char)((unsigned int)last_g + 255u);
            b = (unsigned char)((unsigned int)last_b + 1u);
            a = last_a;
        }
        else if (mode == 2u) {
            unsigned int vg;
            vg = test_next_rand8() % 31u;
            r = (unsigned char)((unsigned int)last_r + vg + 2u);
            g = (unsigned char)((unsigned int)last_g + vg);
            b = (unsigned char)((unsigned int)last_b + vg + 1u);
            a = last_a;
        }
        else {
            r = (unsigned char)test_next_rand8();
            g = (unsigned char)test_next_rand8();
            b = (unsigned char)test_next_rand8();
            a = (channels == 4u) ? (unsigned char)test_next_rand8() : 255u;
        }

        pixels[pos + 0u] = r;
        pixels[pos + 1u] = g;
        pixels[pos + 2u] = b;
        if (channels == 4u) {
            pixels[pos + 3u] = a;
        }

        last_r = r;
        last_g = g;
        last_b = b;
        last_a = a;
    }
}
