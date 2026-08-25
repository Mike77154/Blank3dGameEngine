#include <stdio.h>
#include <string.h>

#include "giffany_dds.h"
#include "test_support89.h"

static int expect_result(const char* label, gdds_result got, gdds_result expected) {
    if (got != expected) {
        fprintf(stderr, "%s: expected %s, got %s\n",
                label, gdds_result_string(expected), gdds_result_string(got));
        return 0;
    }
    return 1;
}

static void make_signed_field_rgba(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_test_make_signed_xy(rgba, w, h, 4, 5);
}

static int test_bc4_snorm_roundtrip(void) {
    enum { W = 12, H = 12 };
    gdds_u8 rgba[W * H * 4u];
    gdds_encode_options enc;
    gdds_buffer dds;
    gdds_info info;
    gdds_image image;
    enc = gdds_encode_options_default(GDDS_FORMAT_BC4_SNORM);
    memset(&dds, 0, sizeof(dds)); memset(&info, 0, sizeof(info)); memset(&image, 0, sizeof(image));
    make_signed_field_rgba(rgba, W, H);
    if (!gdds_format_is_snorm(GDDS_FORMAT_BC4_SNORM) || gdds_format_is_snorm(GDDS_FORMAT_BC5_UNORM)) return 0;
    if (!expect_result("encode BC4_SNORM", gdds_encode_memory_rgba8(rgba, W, H, &enc, &dds), GDDS_RESULT_OK)) return 0;
    if (!expect_result("inspect BC4_SNORM", gdds_inspect_memory(dds.data, dds.size, &info), GDDS_RESULT_OK)) return 0;
    if (info.source_format != GDDS_FORMAT_BC4_SNORM || !info.has_dx10_header || info.data_offset != 148u) return 0;
    if (!expect_result("decode BC4_SNORM", gdds_decode_memory(dds.data, dds.size, &image), GDDS_RESULT_OK)) return 0;
    if (!gdds_test_channel_mae_le_x2(rgba, image.pixels, W, H, 0u, 0u, 24u)) return 0;
    if (gdds_test_channel_error_sum(image.pixels, image.pixels, W, H, 0u, 1u) != 0u ||
        gdds_test_channel_error_sum(image.pixels, image.pixels, W, H, 0u, 2u) != 0u) return 0;
    if (!gdds_test_channel_constant(image.pixels, W, H, 3u, 255u)) return 0;
    gdds_image_release(&image); gdds_buffer_release(&dds);
    return 1;
}

static int test_bc5_snorm_roundtrip(void) {
    enum { W = 12, H = 12 };
    gdds_u8 rgba[W * H * 4u];
    gdds_encode_options enc;
    gdds_buffer dds;
    gdds_info info;
    gdds_image image;
    enc = gdds_encode_options_default(GDDS_FORMAT_BC5_SNORM);
    memset(&dds, 0, sizeof(dds)); memset(&info, 0, sizeof(info)); memset(&image, 0, sizeof(image));
    make_signed_field_rgba(rgba, W, H);
    if (!expect_result("encode BC5_SNORM", gdds_encode_memory_rgba8(rgba, W, H, &enc, &dds), GDDS_RESULT_OK)) return 0;
    if (!expect_result("inspect BC5_SNORM", gdds_inspect_memory(dds.data, dds.size, &info), GDDS_RESULT_OK)) return 0;
    if (info.source_format != GDDS_FORMAT_BC5_SNORM || !info.has_dx10_header || info.data_offset != 148u) return 0;
    if (!expect_result("decode BC5_SNORM", gdds_decode_memory(dds.data, dds.size, &image), GDDS_RESULT_OK)) return 0;
    if (!gdds_test_channel_mae_le_x2(rgba, image.pixels, W, H, 0u, 0u, 24u) ||
        !gdds_test_channel_mae_le_x2(rgba, image.pixels, W, H, 1u, 1u, 24u)) return 0;
    if (!gdds_test_channel_constant(image.pixels, W, H, 2u, 128u) ||
        !gdds_test_channel_constant(image.pixels, W, H, 3u, 255u)) return 0;
    gdds_image_release(&image); gdds_buffer_release(&dds);
    return 1;
}

int main(void) {
    if (!test_bc4_snorm_roundtrip()) return 1;
    if (!test_bc5_snorm_roundtrip()) return 1;
    puts("test_bc45_snorm: ok");
    return 0;
}
