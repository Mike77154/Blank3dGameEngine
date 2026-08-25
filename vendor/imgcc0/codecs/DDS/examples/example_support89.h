#ifndef GDDS_EXAMPLE_SUPPORT89_H
#define GDDS_EXAMPLE_SUPPORT89_H

#include "giffany_dds.h"

static gdds_u8 gdds_example_pack_signed_fraction(gdds_s32 numerator,
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

static void gdds_example_make_signed_xy(gdds_u8* rgba,
                                        gdds_u32 w,
                                        gdds_u32 h,
                                        gdds_s32 range_num,
                                        gdds_s32 range_den) {
    gdds_u32 x;
    gdds_u32 y;
    gdds_s32 dx;
    gdds_s32 dy;
    gdds_s32 nx;
    gdds_s32 ny;
    gdds_u8* p;
    dx = (gdds_s32)(w > 1u ? w - 1u : 1u);
    dy = (gdds_s32)(h > 1u ? h - 1u : 1u);
    for (y = 0u; y < h; ++y) {
        for (x = 0u; x < w; ++x) {
            nx = range_num * ((gdds_s32)(2u * x) - dx);
            ny = range_num * ((gdds_s32)(2u * y) - dy);
            p = rgba + ((gdds_size)(y * w + x) * 4u);
            p[0] = gdds_example_pack_signed_fraction(nx, range_den * dx);
            p[1] = gdds_example_pack_signed_fraction(ny, range_den * dy);
            p[2] = 128u;
            p[3] = 255u;
        }
    }
}

#endif
