#ifndef BMP_MATH32_H
#define BMP_MATH32_H

#include "bmp_types.h"

#ifdef BMP_MATH32_ENABLE_ADD
static int bmp_u32_add_checked(bmp_u32 a, bmp_u32 b, bmp_u32 *out)
{
    if (!out) return 0;
    if (a > 0xFFFFFFFFU - b) return 0;
    *out = a + b;
    return 1;
}
#endif

#ifdef BMP_MATH32_ENABLE_MUL
static int bmp_u32_mul_checked(bmp_u32 a, bmp_u32 b, bmp_u32 *out)
{
    if (!out) return 0;
    if (a != 0U && b > 0xFFFFFFFFU / a) return 0;
    *out = a * b;
    return 1;
}
#endif

#ifdef BMP_MATH32_ENABLE_SCALE_TO_U8
typedef struct bmp_word_pair_s {
    bmp_u32 hi;
    bmp_u32 lo;
} bmp_word_pair;

static bmp_word_pair bmp_mul_u32_u8_pair(bmp_u32 a, bmp_u32 b)
{
    bmp_u32 a0;
    bmp_u32 a1;
    bmp_u32 p0;
    bmp_u32 p1;
    bmp_u32 shifted;
    bmp_u32 lo;
    bmp_u32 carry;
    bmp_word_pair r;

    a0 = a & 0xFFFFU;
    a1 = a >> 16;
    p0 = a0 * b;
    p1 = a1 * b;
    shifted = p1 << 16;
    lo = p0 + shifted;
    carry = (lo < p0) ? 1U : 0U;
    r.lo = lo;
    r.hi = (p1 >> 16) + carry;
    return r;
}

static bmp_word_pair bmp_pair_add_u32(bmp_word_pair a, bmp_u32 b)
{
    bmp_u32 old;
    old = a.lo;
    a.lo += b;
    if (a.lo < old) ++a.hi;
    return a;
}

static int bmp_pair_cmp(bmp_word_pair a, bmp_word_pair b)
{
    if (a.hi < b.hi) return -1;
    if (a.hi > b.hi) return 1;
    if (a.lo < b.lo) return -1;
    if (a.lo > b.lo) return 1;
    return 0;
}

static bmp_u8 bmp_scale_u32_to_u8_exact(bmp_u32 value, bmp_u32 max_value)
{
    bmp_u32 lo;
    bmp_u32 hi;
    bmp_u32 mid;
    bmp_word_pair threshold;
    bmp_word_pair product;

    if (max_value == 0U) return 0U;
    if (value >= max_value) return 255U;

    threshold = bmp_mul_u32_u8_pair(value, 255U);
    threshold = bmp_pair_add_u32(threshold, max_value / 2U);

    lo = 0U;
    hi = 255U;
    while (lo < hi) {
        mid = (lo + hi + 1U) >> 1;
        product = bmp_mul_u32_u8_pair(max_value, mid);
        if (bmp_pair_cmp(product, threshold) <= 0) lo = mid;
        else hi = mid - 1U;
    }
    return (bmp_u8)lo;
}
#endif

#ifdef BMP_MATH32_ENABLE_SCALE_FROM_U8
static bmp_u32 bmp_scale_u8_to_u32_exact(bmp_u8 value, bmp_u32 max_value)
{
    bmp_u32 q;
    bmp_u32 r;
    bmp_u32 base;
    bmp_u32 tail;

    if (value == 0U || max_value == 0U) return 0U;
    if (value == 255U) return max_value;
    q = max_value / 255U;
    r = max_value % 255U;
    base = (bmp_u32)value * q;
    tail = (((bmp_u32)value * r) + 127U) / 255U;
    return base + tail;
}
#endif

#endif
