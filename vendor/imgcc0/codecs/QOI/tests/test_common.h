#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#include <stddef.h>

#include "qoi89.h"

struct vector_case {
    const char *name;
    qoi89_desc desc;
    const unsigned char *pixels;
    size_t pixels_len;
    const unsigned char *qoi;
    size_t qoi_len;
};

extern const struct vector_case test_vectors[];
extern const size_t test_vectors_count;

void test_fail(const char *where, const char *msg);
void test_expect_status(const char *where, qoi89_status got, qoi89_status expected);
void test_expect_mem(const char *where, const void *a, const void *b, size_t n);

void test_fill_pattern(
    unsigned char *pixels,
    size_t pixels_len,
    unsigned int channels,
    unsigned long case_id
);

#endif /* TEST_COMMON_H */
