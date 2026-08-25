#include <stdio.h>
#include <string.h>

#include "giffany_dds.h"

static int expect_result(const char* label, gdds_result got, gdds_result expected) {
    if (got != expected) {
        fprintf(stderr, "%s: expected %s, got %s\n",
                label,
                gdds_result_string(expected),
                gdds_result_string(got));
        return 0;
    }
    return 1;
}

static void make_bw_checker_2x2(gdds_u8* rgba) {
    static const gdds_u8 pixels[16] = {
          0u,   0u,   0u, 255u,
        255u, 255u, 255u, 255u,
          0u,   0u,   0u, 255u,
        255u, 255u, 255u, 255u
    };
    memcpy(rgba, pixels, sizeof(pixels));
}

static void make_gradient_4x4(gdds_u8* rgba) {
    gdds_u32 x, y;
    for (y = 0u; y < 4u; ++y) {
        for (x = 0u; x < 4u; ++x) {
            gdds_u8* p = rgba + (((gdds_size)y * 4u + x) * 4u);
            p[0] = (gdds_u8)(x * 64u);
            p[1] = (gdds_u8)(y * 64u);
            p[2] = (gdds_u8)((x + y) * 32u);
            p[3] = (gdds_u8)(255u - (x * 16u + y * 16u));
        }
    }
}

static int test_public_srgb_helper(void) {
    if (!gdds_format_is_srgb(GDDS_FORMAT_RGBA8_SRGB) ||
        !gdds_format_is_srgb(GDDS_FORMAT_DXT5_SRGB) ||
        gdds_format_is_srgb(GDDS_FORMAT_RGBA8) ||
        gdds_format_is_srgb(GDDS_FORMAT_DXT1)) {
        fprintf(stderr, "gdds_format_is_srgb returned unexpected values\n");
        return 0;
    }
    return 1;
}

static int test_standalone_auto_defaults_to_linear(void) {
    gdds_u8 rgba[2u * 2u * 4u];
    gdds_generated_mipchain chain = {0};
    gdds_mipmap_options opt = gdds_mipmap_options_default();
    const gdds_u8* mip;

    make_bw_checker_2x2(rgba);
    opt.mip_count = 2u;

    if (!expect_result("generate auto-color-space mipchain",
                       gdds_generate_mipchain_rgba8(rgba, 2u, 2u, &opt, &chain),
                       GDDS_RESULT_OK)) {
        return 0;
    }

    mip = chain.levels[1].pixels;
    if (!(mip[0] == 128u && mip[1] == 128u && mip[2] == 128u && mip[3] == 255u)) {
        fprintf(stderr, "AUTO standalone mip generation should resolve to linear averaging, got %u %u %u %u\n",
                mip[0], mip[1], mip[2], mip[3]);
        gdds_generated_mipchain_release(&chain);
        return 0;
    }

    gdds_generated_mipchain_release(&chain);
    return 1;
}

static int test_explicit_srgb_downsample(void) {
    gdds_u8 rgba[2u * 2u * 4u];
    gdds_generated_mipchain chain = {0};
    gdds_mipmap_options opt = gdds_mipmap_options_default();
    const gdds_u8* mip;

    make_bw_checker_2x2(rgba);
    opt.mip_count = 2u;
    opt.color_space = GDDS_MIP_COLOR_SPACE_SRGB;

    if (!expect_result("generate sRGB-aware mipchain",
                       gdds_generate_mipchain_rgba8(rgba, 2u, 2u, &opt, &chain),
                       GDDS_RESULT_OK)) {
        return 0;
    }

    mip = chain.levels[1].pixels;
    if (!(mip[0] == 188u && mip[1] == 188u && mip[2] == 188u && mip[3] == 255u)) {
        fprintf(stderr, "unexpected sRGB-aware 50%% gray result: %u %u %u %u\n",
                mip[0], mip[1], mip[2], mip[3]);
        gdds_generated_mipchain_release(&chain);
        return 0;
    }

    gdds_generated_mipchain_release(&chain);
    return 1;
}

static int test_auto_mip_encode_infers_srgb_from_output(void) {
    gdds_u8 rgba[2u * 2u * 4u];
    gdds_encode_options enc = gdds_encode_options_default(GDDS_FORMAT_RGBA8_SRGB);
    gdds_buffer dds = {0};
    gdds_info info = {0};
    gdds_image mip = {0};

    make_bw_checker_2x2(rgba);

    if (!expect_result("encode auto mips as RGBA8_SRGB",
                       gdds_encode_memory_rgba8_auto_mips(rgba, 2u, 2u, &enc, NULL, &dds),
                       GDDS_RESULT_OK)) {
        return 0;
    }

    if (!expect_result("inspect RGBA8_SRGB DDS",
                       gdds_inspect_memory(dds.data, dds.size, &info),
                       GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }

    if (info.source_format != GDDS_FORMAT_RGBA8_SRGB ||
        !info.is_srgb ||
        !info.has_dx10_header ||
        info.mip_count != 2u ||
        info.data_offset != 148u) {
        fprintf(stderr, "unexpected RGBA8_SRGB inspect data\n");
        gdds_buffer_release(&dds);
        return 0;
    }

    if (!expect_result("decode inferred sRGB mip 1",
                       gdds_decode_mip_memory(dds.data, dds.size, 1u, &mip),
                       GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }

    if (!(mip.width == 1u && mip.height == 1u &&
          mip.pixels[0] == 188u && mip.pixels[1] == 188u && mip.pixels[2] == 188u && mip.pixels[3] == 255u)) {
        fprintf(stderr, "unexpected decoded inferred-sRGB mip result: %u %u %u %u\n",
                mip.pixels[0], mip.pixels[1], mip.pixels[2], mip.pixels[3]);
        gdds_image_release(&mip);
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_image_release(&mip);
    gdds_buffer_release(&dds);
    return 1;
}

static int test_srgb_dxt5_writer_uses_dx10(void) {
    gdds_u8 rgba[4u * 4u * 4u];
    gdds_encode_options enc = gdds_encode_options_default(GDDS_FORMAT_DXT5_SRGB);
    gdds_buffer dds = {0};
    gdds_info info = {0};

    make_gradient_4x4(rgba);

    if (!expect_result("encode DXT5_SRGB",
                       gdds_encode_memory_rgba8(rgba, 4u, 4u, &enc, &dds),
                       GDDS_RESULT_OK)) {
        return 0;
    }

    if (!expect_result("inspect DXT5_SRGB",
                       gdds_inspect_memory(dds.data, dds.size, &info),
                       GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }

    if (info.source_format != GDDS_FORMAT_DXT5_SRGB ||
        !info.is_srgb ||
        !info.has_dx10_header ||
        info.data_offset != 148u ||
        info.mip_count != 1u) {
        fprintf(stderr, "unexpected DXT5_SRGB inspect data\n");
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_buffer_release(&dds);
    return 1;
}

int main(void) {
    if (!test_public_srgb_helper()) return 1;
    if (!test_standalone_auto_defaults_to_linear()) return 1;
    if (!test_explicit_srgb_downsample()) return 1;
    if (!test_auto_mip_encode_infers_srgb_from_output()) return 1;
    if (!test_srgb_dxt5_writer_uses_dx10()) return 1;
    printf("ok\n");
    return 0;
}
