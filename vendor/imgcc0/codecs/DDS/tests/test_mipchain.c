#include <stdio.h>
#include <string.h>

#include "giffany_dds.h"
#include "test_support89.h"

#define TEST_MAX_MIPS 8u
#define TEST_LEVEL_CAP (8u * 8u * 4u)

static gdds_u8 g_pixels[TEST_MAX_MIPS][TEST_LEVEL_CAP];
static gdds_mip_level_rgba8 g_levels[TEST_MAX_MIPS];
static gdds_u32 g_widths[TEST_MAX_MIPS];
static gdds_u32 g_heights[TEST_MAX_MIPS];

static int expect_result(const char* label, gdds_result got, gdds_result expected) {
    if (got != expected) {
        fprintf(stderr, "%s: expected %s, got %s\n",
                label, gdds_result_string(expected), gdds_result_string(got));
        return 0;
    }
    return 1;
}

static void make_alpha_pattern(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_u32 x;
    gdds_u32 y;
    gdds_u32 denom;
    gdds_u8* p;
    denom = (w - 1u) + (h - 1u);
    if (denom == 0u) denom = 1u;
    for (y = 0u; y < h; ++y) {
        for (x = 0u; x < w; ++x) {
            p = rgba + ((gdds_size)(y * w + x) * 4u);
            p[0] = (gdds_u8)((x * 255u) / (w > 1u ? w - 1u : 1u));
            p[1] = (gdds_u8)(255u - ((y * 255u) / (h > 1u ? h - 1u : 1u)));
            p[2] = (gdds_u8)(((x * 17u) ^ (y * 29u)) & 255u);
            p[3] = (gdds_u8)(((x + y) * 255u) / denom);
        }
    }
}

static void downsample_box_rgba8(const gdds_u8* src,
                                 gdds_u32 src_w,
                                 gdds_u32 src_h,
                                 gdds_u8* dst,
                                 gdds_u32 dst_w,
                                 gdds_u32 dst_h) {
    gdds_u32 y;
    gdds_u32 x;
    gdds_u32 c;
    gdds_u32 x0;
    gdds_u32 y0;
    gdds_u32 x1;
    gdds_u32 y1;
    gdds_u32 a;
    gdds_u32 b;
    gdds_u32 d;
    gdds_u32 e;
    for (y = 0u; y < dst_h; ++y) {
        for (x = 0u; x < dst_w; ++x) {
            x0 = x * 2u;
            y0 = y * 2u;
            x1 = x0 + 1u < src_w ? x0 + 1u : src_w - 1u;
            y1 = y0 + 1u < src_h ? y0 + 1u : src_h - 1u;
            for (c = 0u; c < 4u; ++c) {
                a = src[((gdds_size)(y0 * src_w + x0) * 4u) + c];
                b = src[((gdds_size)(y0 * src_w + x1) * 4u) + c];
                d = src[((gdds_size)(y1 * src_w + x0) * 4u) + c];
                e = src[((gdds_size)(y1 * src_w + x1) * 4u) + c];
                dst[((gdds_size)(y * dst_w + x) * 4u) + c] = (gdds_u8)((a + b + d + e + 2u) / 4u);
            }
        }
    }
}

static int build_mipchain(gdds_u32 base_w, gdds_u32 base_h, gdds_u32* out_count) {
    gdds_u32 count;
    gdds_u32 level;
    count = gdds_calc_full_mip_count_2d(base_w, base_h);
    if (count > TEST_MAX_MIPS || base_w * base_h * 4u > TEST_LEVEL_CAP) return 0;
    g_widths[0] = base_w;
    g_heights[0] = base_h;
    make_alpha_pattern(g_pixels[0], base_w, base_h);
    for (level = 0u; level < count; ++level) {
        if (level > 0u) {
            g_widths[level] = g_widths[level - 1u] > 1u ? g_widths[level - 1u] >> 1u : 1u;
            g_heights[level] = g_heights[level - 1u] > 1u ? g_heights[level - 1u] >> 1u : 1u;
            downsample_box_rgba8(g_pixels[level - 1u], g_widths[level - 1u], g_heights[level - 1u],
                                 g_pixels[level], g_widths[level], g_heights[level]);
        }
        g_levels[level].pixels = g_pixels[level];
        g_levels[level].size = g_widths[level] * g_heights[level] * 4u;
    }
    *out_count = count;
    return 1;
}

static int test_rgba8_mipchain_exact(void) {
    gdds_u32 count;
    gdds_u32 level;
    gdds_size expected_chain;
    gdds_size expected_size;
    gdds_encode_options options;
    gdds_buffer dds;
    gdds_info info;
    gdds_mip_info mip;
    gdds_image image;
    gdds_mip_info scratch;

    count = 0u;
    expected_chain = 0u;
    options = gdds_encode_options_default(GDDS_FORMAT_RGBA8);
    memset(&dds, 0, sizeof(dds)); memset(&info, 0, sizeof(info));
    if (!build_mipchain(8u, 4u, &count)) return 0;
    if (!expect_result("encode rgba8 mipchain",
                       gdds_encode_mipchain_memory_rgba8(g_levels, count, 8u, 4u, &options, &dds), GDDS_RESULT_OK)) return 0;
    if (!expect_result("inspect rgba8 mipchain", gdds_inspect_memory(dds.data, dds.size, &info), GDDS_RESULT_OK)) return 0;
    if (info.mip_count != count) return 0;
    for (level = 0u; level < count; ++level) {
        memset(&mip, 0, sizeof(mip)); memset(&image, 0, sizeof(image));
        expected_size = g_widths[level] * g_heights[level] * 4u;
        expected_chain += expected_size;
        if (!expect_result("get mip info", gdds_get_mip_info(dds.data, dds.size, level, &mip), GDDS_RESULT_OK)) return 0;
        if (mip.width != g_widths[level] || mip.height != g_heights[level] || mip.data_size != expected_size) return 0;
        if (level == 0u && mip.data_offset != info.data_offset) return 0;
        if (!expect_result("decode mip rgba8", gdds_decode_mip_memory(dds.data, dds.size, level, &image), GDDS_RESULT_OK)) return 0;
        if (memcmp(image.pixels, g_pixels[level], expected_size) != 0) return 0;
        gdds_image_release(&image);
    }
    if (info.full_chain_size != expected_chain) return 0;
    memset(&scratch, 0, sizeof(scratch));
    if (!expect_result("out-of-range mip info", gdds_get_mip_info(dds.data, dds.size, count, &scratch), GDDS_RESULT_INVALID_ARGUMENT)) return 0;
    gdds_buffer_release(&dds);
    return 1;
}

static int test_dxt5_mipchain_decode(void) {
    gdds_u32 count;
    gdds_encode_options options;
    gdds_buffer dds;
    gdds_image image;
    gdds_size n;
    count = 0u;
    options = gdds_encode_options_default(GDDS_FORMAT_DXT5);
    memset(&dds, 0, sizeof(dds)); memset(&image, 0, sizeof(image));
    if (!build_mipchain(8u, 8u, &count)) return 0;
    if (!expect_result("encode dxt5 mipchain",
                       gdds_encode_mipchain_memory_rgba8(g_levels, count, 8u, 8u, &options, &dds), GDDS_RESULT_OK)) return 0;
    if (!expect_result("decode dxt5 mip 2", gdds_decode_mip_memory(dds.data, dds.size, 2u, &image), GDDS_RESULT_OK)) return 0;
    n = g_widths[2] * g_heights[2] * 4u;
    if (!gdds_test_mae_le(image.pixels, g_pixels[2], n, 50u)) return 0;
    gdds_image_release(&image); gdds_buffer_release(&dds);
    return 1;
}

static int test_invalid_mipchain_rejected(void) {
    gdds_u8 base[4u * 4u * 4u];
    gdds_u8 bad[4u * 4u * 4u];
    gdds_mip_level_rgba8 levels[2];
    gdds_encode_options options;
    gdds_buffer output;
    memset(base, 0, sizeof(base)); memset(bad, 0, sizeof(bad)); memset(&output, 0, sizeof(output));
    options = gdds_encode_options_default(GDDS_FORMAT_RGBA8);
    levels[0].pixels = base; levels[0].size = sizeof(base);
    levels[1].pixels = bad; levels[1].size = 15u;
    return expect_result("invalid mipchain rejected",
                         gdds_encode_mipchain_memory_rgba8(levels, 2u, 4u, 4u, &options, &output),
                         GDDS_RESULT_INVALID_ARGUMENT);
}

int main(void) {
    if (!test_rgba8_mipchain_exact()) return 1;
    if (!test_dxt5_mipchain_decode()) return 1;
    if (!test_invalid_mipchain_rejected()) return 1;
    puts("ok");
    return 0;
}
