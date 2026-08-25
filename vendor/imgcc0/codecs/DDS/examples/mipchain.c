#include <stdio.h>
#include <string.h>

#include "giffany_dds.h"

#define EX_BASE_W 16u
#define EX_BASE_H 16u
#define EX_MIPS 5u
#define EX_LEVEL_BYTES (EX_BASE_W * EX_BASE_H * 4u)

static gdds_u8 g_level_storage[EX_MIPS][EX_LEVEL_BYTES];

static void fill_checker(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_u32 x;
    gdds_u32 y;
    gdds_u32 on;
    gdds_u8* p;
    for (y = 0u; y < h; ++y) {
        for (x = 0u; x < w; ++x) {
            p = rgba + ((gdds_size)(y * w + x) * 4u);
            on = ((x >> 2u) ^ (y >> 2u)) & 1u;
            p[0] = (gdds_u8)(on ? 255u : 32u);
            p[1] = (gdds_u8)(on ? 64u : 200u);
            p[2] = (gdds_u8)(on ? 32u : 255u);
            p[3] = 255u;
        }
    }
}

static void downsample_box(const gdds_u8* src,
                           gdds_u32 src_w,
                           gdds_u32 src_h,
                           gdds_u8* dst,
                           gdds_u32 dst_w,
                           gdds_u32 dst_h) {
    gdds_u32 x;
    gdds_u32 y;
    gdds_u32 c;
    gdds_u32 sx0;
    gdds_u32 sy0;
    gdds_u32 sx1;
    gdds_u32 sy1;
    gdds_u32 a;
    gdds_u32 b;
    gdds_u32 d;
    gdds_u32 e;
    for (y = 0u; y < dst_h; ++y) {
        for (x = 0u; x < dst_w; ++x) {
            sx0 = x * 2u;
            sy0 = y * 2u;
            sx1 = (sx0 + 1u < src_w) ? sx0 + 1u : src_w - 1u;
            sy1 = (sy0 + 1u < src_h) ? sy0 + 1u : src_h - 1u;
            for (c = 0u; c < 4u; ++c) {
                a = src[((gdds_size)(sy0 * src_w + sx0) * 4u) + c];
                b = src[((gdds_size)(sy0 * src_w + sx1) * 4u) + c];
                d = src[((gdds_size)(sy1 * src_w + sx0) * 4u) + c];
                e = src[((gdds_size)(sy1 * src_w + sx1) * 4u) + c];
                dst[((gdds_size)(y * dst_w + x) * 4u) + c] = (gdds_u8)((a + b + d + e + 2u) / 4u);
            }
        }
    }
}

int main(void) {
    gdds_mip_level_rgba8 levels[EX_MIPS];
    gdds_u32 widths[EX_MIPS];
    gdds_u32 heights[EX_MIPS];
    gdds_encode_options enc;
    gdds_buffer dds;
    gdds_mip_info mip;
    gdds_image decoded;
    gdds_u32 level;

    memset(&dds, 0, sizeof(dds));
    memset(&mip, 0, sizeof(mip));
    memset(&decoded, 0, sizeof(decoded));
    enc = gdds_encode_options_default(GDDS_FORMAT_DXT5);

    widths[0] = EX_BASE_W;
    heights[0] = EX_BASE_H;
    fill_checker(g_level_storage[0], widths[0], heights[0]);
    for (level = 0u; level < EX_MIPS; ++level) {
        if (level > 0u) {
            widths[level] = widths[level - 1u] > 1u ? widths[level - 1u] >> 1u : 1u;
            heights[level] = heights[level - 1u] > 1u ? heights[level - 1u] >> 1u : 1u;
            downsample_box(g_level_storage[level - 1u], widths[level - 1u], heights[level - 1u],
                           g_level_storage[level], widths[level], heights[level]);
        }
        levels[level].pixels = g_level_storage[level];
        levels[level].size = widths[level] * heights[level] * 4u;
    }

    if (gdds_encode_mipchain_memory_rgba8(levels, EX_MIPS, EX_BASE_W, EX_BASE_H, &enc, &dds) != GDDS_RESULT_OK) {
        fprintf(stderr, "encode failed\n");
        return 1;
    }
    if (gdds_get_mip_info(dds.data, dds.size, 2u, &mip) != GDDS_RESULT_OK) {
        fprintf(stderr, "mip info failed\n");
        return 1;
    }
    if (gdds_decode_mip_memory(dds.data, dds.size, 2u, &decoded) != GDDS_RESULT_OK) {
        fprintf(stderr, "decode failed\n");
        return 1;
    }

    printf("encoded %u mip levels\n", EX_MIPS);
    printf("level 2: %ux%u at byte offset %u (%u bytes)\n",
           mip.width, mip.height, mip.data_offset, mip.data_size);
    printf("decoded mip 2 into %u RGBA bytes\n", decoded.size);
    gdds_image_release(&decoded);
    gdds_buffer_release(&dds);
    return 0;
}
