#include <stdio.h>
#include <string.h>

#include "giffany_dds.h"

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
    gdds_u8* p;
    gdds_u32 denom;
    for (y = 0u; y < h; ++y) {
        for (x = 0u; x < w; ++x) {
            p = rgba + ((gdds_size)(y * w + x) * 4u);
            p[0] = (gdds_u8)((x * 255u) / (w > 1u ? w - 1u : 1u));
            p[1] = (gdds_u8)(255u - ((y * 255u) / (h > 1u ? h - 1u : 1u)));
            p[2] = (gdds_u8)(((x * 17u) ^ (y * 29u)) & 255u);
            denom = ((w - 1u) + (h - 1u));
            if (denom == 0u) denom = 1u;
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

static int test_generate_and_encode_exact(void) {
    gdds_u8 rgba[8u * 4u * 4u];
    gdds_u8 expected[8u * 4u * 4u];
    gdds_generated_mipchain chain;
    gdds_encode_options enc;
    gdds_buffer dds;
    gdds_info info;
    gdds_u32 level;
    gdds_u32 expected_w;
    gdds_u32 expected_h;
    gdds_size bytes;
    gdds_image img;

    memset(&chain, 0, sizeof(chain));
    memset(&dds, 0, sizeof(dds));
    memset(&info, 0, sizeof(info));
    enc = gdds_encode_options_default(GDDS_FORMAT_RGBA8);
    make_alpha_pattern(rgba, 8u, 4u);

    if (!expect_result("generate default mipchain",
                       gdds_generate_mipchain_rgba8(rgba, 8u, 4u, NULL, &chain),
                       GDDS_RESULT_OK)) return 0;
    if (chain.level_count != gdds_calc_full_mip_count_2d(8u, 4u)) return 0;
    if (memcmp(chain.levels[0].pixels, rgba, sizeof(rgba)) != 0) return 0;

    expected_w = 8u;
    expected_h = 4u;
    for (level = 1u; level < chain.level_count; ++level) {
        expected_w = expected_w > 1u ? expected_w >> 1u : 1u;
        expected_h = expected_h > 1u ? expected_h >> 1u : 1u;
        bytes = expected_w * expected_h * 4u;
        downsample_box_rgba8(chain.levels[level - 1u].pixels,
                             chain.levels[level - 1u].width,
                             chain.levels[level - 1u].height,
                             expected, expected_w, expected_h);
        if (chain.levels[level].width != expected_w ||
            chain.levels[level].height != expected_h ||
            memcmp(chain.levels[level].pixels, expected, bytes) != 0) {
            fprintf(stderr, "generated level mismatch at %u\n", level);
            return 0;
        }
    }

    if (!expect_result("encode auto mips rgba8",
                       gdds_encode_memory_rgba8_auto_mips(rgba, 8u, 4u, &enc, NULL, &dds),
                       GDDS_RESULT_OK)) return 0;
    if (!expect_result("inspect auto-mipped rgba8",
                       gdds_inspect_memory(dds.data, dds.size, &info), GDDS_RESULT_OK)) return 0;
    if (info.mip_count != chain.level_count) return 0;

    for (level = 0u; level < chain.level_count; ++level) {
        memset(&img, 0, sizeof(img));
        if (!expect_result("decode generated mip",
                           gdds_decode_mip_memory(dds.data, dds.size, level, &img),
                           GDDS_RESULT_OK)) return 0;
        if (img.size != chain.levels[level].size ||
            memcmp(img.pixels, chain.levels[level].pixels, img.size) != 0) return 0;
        gdds_image_release(&img);
    }
    gdds_buffer_release(&dds);
    gdds_generated_mipchain_release(&chain);
    return 1;
}

static int test_partial_chain(void) {
    gdds_u8 rgba[8u * 8u * 4u];
    gdds_generated_mipchain chain;
    gdds_mipmap_options opt;
    gdds_encode_options enc;
    gdds_buffer dds;
    gdds_info info;

    memset(&chain, 0, sizeof(chain));
    memset(&dds, 0, sizeof(dds));
    memset(&info, 0, sizeof(info));
    opt = gdds_mipmap_options_default();
    enc = gdds_encode_options_default(GDDS_FORMAT_RGBA8);
    make_alpha_pattern(rgba, 8u, 8u);
    opt.mip_count = 2u;

    if (!expect_result("generate partial mipchain",
                       gdds_generate_mipchain_rgba8(rgba, 8u, 8u, &opt, &chain),
                       GDDS_RESULT_OK)) return 0;
    if (chain.level_count != 2u || chain.levels[1].width != 4u || chain.levels[1].height != 4u) return 0;
    if (!expect_result("encode partial auto mips",
                       gdds_encode_memory_rgba8_auto_mips(rgba, 8u, 8u, &enc, &opt, &dds),
                       GDDS_RESULT_OK)) return 0;
    if (!expect_result("inspect partial auto mips",
                       gdds_inspect_memory(dds.data, dds.size, &info), GDDS_RESULT_OK)) return 0;
    if (info.mip_count != 2u) return 0;
    gdds_buffer_release(&dds);
    gdds_generated_mipchain_release(&chain);
    return 1;
}

static int test_premultiplied_alpha_filter(void) {
    gdds_u8 rgba[16] = {
        255u, 0u, 0u, 255u,
        0u, 255u, 0u, 0u,
        0u, 0u, 255u, 0u,
        255u, 255u, 255u, 0u
    };
    gdds_u8 straight_pixel[4];
    gdds_generated_mipchain chain;
    gdds_mipmap_options opt;
    const gdds_u8* p;

    memset(&chain, 0, sizeof(chain));
    opt = gdds_mipmap_options_default();
    opt.mip_count = 2u;
    if (!expect_result("generate straight alpha mips",
                       gdds_generate_mipchain_rgba8(rgba, 2u, 2u, &opt, &chain), GDDS_RESULT_OK)) return 0;
    memcpy(straight_pixel, chain.levels[1].pixels, 4u);
    gdds_generated_mipchain_release(&chain);

    opt.alpha_filter = GDDS_MIP_ALPHA_FILTER_PREMULTIPLIED;
    if (!expect_result("generate premult alpha mips",
                       gdds_generate_mipchain_rgba8(rgba, 2u, 2u, &opt, &chain), GDDS_RESULT_OK)) return 0;
    p = chain.levels[1].pixels;
    if (!(straight_pixel[0] == 128u && straight_pixel[1] == 128u && straight_pixel[2] == 128u && straight_pixel[3] == 64u)) return 0;
    if (!(p[0] == 255u && p[1] == 0u && p[2] == 0u && p[3] == 64u)) return 0;
    gdds_generated_mipchain_release(&chain);
    return 1;
}

int main(void) {
    if (!test_generate_and_encode_exact()) return 1;
    if (!test_partial_chain()) return 1;
    if (!test_premultiplied_alpha_filter()) return 1;
    puts("ok");
    return 0;
}
