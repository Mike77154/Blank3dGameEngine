#ifndef GDDS_TEST_SUPPORT89_H
#define GDDS_TEST_SUPPORT89_H

#include "giffany_dds.h"

static gdds_u32 gdds_test_abs_error_sum(const gdds_u8* a,
                                        const gdds_u8* b,
                                        gdds_size n) {
    gdds_size i;
    gdds_u32 sum;
    int d;
    sum = 0u;
    for (i = 0u; i < n; ++i) {
        d = (int)a[i] - (int)b[i];
        if (d < 0) d = -d;
        sum += (gdds_u32)d;
    }
    return sum;
}

static gdds_u32 gdds_test_channel_error_sum(const gdds_u8* a,
                                            const gdds_u8* b,
                                            gdds_u32 w,
                                            gdds_u32 h,
                                            gdds_u32 ca,
                                            gdds_u32 cb) {
    gdds_size i;
    gdds_size count;
    gdds_u32 sum;
    int d;
    count = (gdds_size)(w * h);
    sum = 0u;
    for (i = 0u; i < count; ++i) {
        d = (int)a[i * 4u + ca] - (int)b[i * 4u + cb];
        if (d < 0) d = -d;
        sum += (gdds_u32)d;
    }
    return sum;
}

static int gdds_test_mae_le(const gdds_u8* a,
                            const gdds_u8* b,
                            gdds_size n,
                            gdds_u32 limit) {
    gdds_u32 sum;
    sum = gdds_test_abs_error_sum(a, b, n);
    return sum <= limit * n;
}

static int gdds_test_channel_mae_le_x2(const gdds_u8* a,
                                       const gdds_u8* b,
                                       gdds_u32 w,
                                       gdds_u32 h,
                                       gdds_u32 ca,
                                       gdds_u32 cb,
                                       gdds_u32 limit_x2) {
    gdds_u32 sum;
    gdds_u32 count;
    sum = gdds_test_channel_error_sum(a, b, w, h, ca, cb);
    count = w * h;
    return (sum * 2u) <= (limit_x2 * count);
}

static int gdds_test_channel_constant(const gdds_u8* rgba,
                                      gdds_u32 w,
                                      gdds_u32 h,
                                      gdds_u32 channel,
                                      gdds_u8 value) {
    gdds_size i;
    gdds_size count;
    count = (gdds_size)(w * h);
    for (i = 0u; i < count; ++i) {
        if (rgba[i * 4u + channel] != value) return 0;
    }
    return 1;
}

static gdds_u8 gdds_test_pack_signed_fraction(gdds_s32 numerator,
                                              gdds_s32 denominator) {
    gdds_s32 scaled_num;
    gdds_s32 scaled_den;
    if (denominator <= 0) return 128u;
    if (numerator < -denominator) numerator = -denominator;
    if (numerator > denominator) numerator = denominator;
    scaled_num = (numerator + denominator) * 255 + denominator;
    scaled_den = denominator * 2;
    if (scaled_num <= 0) return 0u;
    if (scaled_num >= scaled_den * 255) return 255u;
    return (gdds_u8)(scaled_num / scaled_den);
}

static void gdds_test_make_signed_xy(gdds_u8* rgba,
                                     gdds_u32 w,
                                     gdds_u32 h,
                                     gdds_s32 range_num,
                                     gdds_s32 range_den) {
    gdds_u32 x;
    gdds_u32 y;
    gdds_s32 nx;
    gdds_s32 ny;
    gdds_s32 dx;
    gdds_s32 dy;
    gdds_u8* p;
    dx = (gdds_s32)(w > 1u ? w - 1u : 1u);
    dy = (gdds_s32)(h > 1u ? h - 1u : 1u);
    for (y = 0u; y < h; ++y) {
        for (x = 0u; x < w; ++x) {
            nx = range_num * ((gdds_s32)(2u * x) - dx);
            ny = range_num * ((gdds_s32)(2u * y) - dy);
            p = rgba + ((gdds_size)(y * w + x) * 4u);
            p[0] = gdds_test_pack_signed_fraction(nx, range_den * dx);
            p[1] = gdds_test_pack_signed_fraction(ny, range_den * dy);
            p[2] = 128u;
            p[3] = 255u;
        }
    }
}

static gdds_u32 gdds_test_isqrt(gdds_u32 value) {
    gdds_u32 bit;
    gdds_u32 root;
    gdds_u32 trial;
    bit = 1u << 30;
    root = 0u;
    while (bit > value) bit >>= 2;
    while (bit != 0u) {
        trial = root + bit;
        if (value >= trial) {
            value -= trial;
            root = (root >> 1) + bit;
        } else {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}

static int gdds_test_normals_unitish(const gdds_u8* rgba,
                                     gdds_u32 w,
                                     gdds_u32 h,
                                     gdds_u32 tolerance_units) {
    gdds_size i;
    gdds_size count;
    gdds_s32 sx;
    gdds_s32 sy;
    gdds_s32 sz;
    gdds_u32 len2;
    gdds_u32 len;
    gdds_u32 diff;
    count = (gdds_size)(w * h);
    for (i = 0u; i < count; ++i) {
        sx = (gdds_s32)rgba[i * 4u + 0u] * 2 - 255;
        sy = (gdds_s32)rgba[i * 4u + 1u] * 2 - 255;
        sz = (gdds_s32)rgba[i * 4u + 2u] * 2 - 255;
        len2 = (gdds_u32)(sx * sx + sy * sy + sz * sz);
        len = gdds_test_isqrt(len2);
        diff = len > 255u ? len - 255u : 255u - len;
        if (diff > tolerance_units) return 0;
    }
    return 1;
}

#endif
