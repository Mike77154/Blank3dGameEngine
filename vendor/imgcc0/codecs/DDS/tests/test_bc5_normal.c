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

static void make_normal_map(gdds_u8* rgba, gdds_u32 w, gdds_u32 h, int xy_only) {
    gdds_test_make_signed_xy(rgba, w, h, 2, 5);
    if (!xy_only) {
        (void)gdds_normal_map_reconstruct_z_rgba8(rgba, w, h);
        (void)gdds_normal_map_normalize_rgba8(rgba, w, h);
    }
}

static int test_inplace_normal_utilities(void) {
    enum { W = 8, H = 8 };
    gdds_u8 rgba[W * H * 4u];
    gdds_u8 before_g[W * H];
    gdds_size i;
    make_normal_map(rgba, W, H, 1);
    if (!expect_result("reconstruct z", gdds_normal_map_reconstruct_z_rgba8(rgba, W, H), GDDS_RESULT_OK)) return 0;
    if (!gdds_test_normals_unitish(rgba, W, H, 10u)) return 0;
    for (i = 0u; i < (gdds_size)(W * H); ++i) rgba[i * 4u + 2u] = 153u;
    if (!expect_result("normalize", gdds_normal_map_normalize_rgba8(rgba, W, H), GDDS_RESULT_OK)) return 0;
    if (!gdds_test_normals_unitish(rgba, W, H, 10u)) return 0;
    for (i = 0u; i < (gdds_size)(W * H); ++i) before_g[i] = rgba[i * 4u + 1u];
    if (!expect_result("invert y", gdds_normal_map_invert_y_rgba8(rgba, W, H), GDDS_RESULT_OK)) return 0;
    for (i = 0u; i < (gdds_size)(W * H); ++i) {
        if (rgba[i * 4u + 1u] != (gdds_u8)(255u - before_g[i])) return 0;
    }
    return 1;
}

static int test_bc5_normal_encode_decode_xyz(void) {
    enum { W = 16, H = 16 };
    gdds_u8 rgba[W * H * 4u];
    gdds_normal_map_encode_options enc;
    gdds_buffer dds;
    gdds_info info;
    gdds_image image;
    enc = gdds_normal_map_encode_options_default();
    memset(&dds, 0, sizeof(dds)); memset(&info, 0, sizeof(info)); memset(&image, 0, sizeof(image));
    make_normal_map(rgba, W, H, 0);
    if (!expect_result("encode bc5 normal xyz", gdds_encode_bc5_normal_map_rgba8(rgba, W, H, &enc, &dds), GDDS_RESULT_OK)) return 0;
    if (!expect_result("inspect bc5 normal xyz", gdds_inspect_memory(dds.data, dds.size, &info), GDDS_RESULT_OK)) return 0;
    if (info.source_format != GDDS_FORMAT_BC5_UNORM || !info.has_dx10_header || info.mip_count != 1u) return 0;
    if (!expect_result("decode bc5 normal xyz", gdds_decode_bc5_normal_map_memory(dds.data, dds.size, NULL, &image), GDDS_RESULT_OK)) return 0;
    if (!gdds_test_channel_mae_le_x2(rgba, image.pixels, W, H, 0u, 0u, 21u) ||
        !gdds_test_channel_mae_le_x2(rgba, image.pixels, W, H, 1u, 1u, 21u) ||
        !gdds_test_channel_mae_le_x2(rgba, image.pixels, W, H, 2u, 2u, 34u)) return 0;
    if (!gdds_test_normals_unitish(image.pixels, W, H, 14u)) return 0;
    gdds_image_release(&image); gdds_buffer_release(&dds);
    return 1;
}

static int test_bc5_normal_auto_mips_xy(void) {
    enum { W = 16, H = 8 };
    gdds_u8 rgba[W * H * 4u];
    gdds_normal_map_encode_options enc;
    gdds_mipmap_options mip;
    gdds_buffer dds;
    gdds_info info;
    gdds_image image;
    gdds_u32 expected_mips;
    enc = gdds_normal_map_encode_options_default();
    mip = gdds_mipmap_options_default();
    memset(&dds, 0, sizeof(dds)); memset(&info, 0, sizeof(info)); memset(&image, 0, sizeof(image));
    expected_mips = gdds_calc_full_mip_count_2d(W, H);
    make_normal_map(rgba, W, H, 1);
    enc.input_layout = GDDS_NORMAL_MAP_LAYOUT_XY_RG;
    enc.normalize = 1;
    enc.invert_y = 1;
    mip.mip_count = 0u;
    mip.color_space = GDDS_MIP_COLOR_SPACE_LINEAR;
    if (!expect_result("encode bc5 normal auto mips",
                       gdds_encode_bc5_normal_map_rgba8_auto_mips(rgba, W, H, &enc, &mip, &dds), GDDS_RESULT_OK)) return 0;
    if (!expect_result("inspect bc5 normal auto mips", gdds_inspect_memory(dds.data, dds.size, &info), GDDS_RESULT_OK)) return 0;
    if (info.source_format != GDDS_FORMAT_BC5_UNORM || info.mip_count != expected_mips) return 0;
    if (!expect_result("decode bc5 normal mip 2",
                       gdds_decode_bc5_normal_map_mip_memory(dds.data, dds.size, 2u, NULL, &image), GDDS_RESULT_OK)) return 0;
    if (image.width != 4u || image.height != 2u) return 0;
    if (!gdds_test_normals_unitish(image.pixels, image.width, image.height, 22u)) return 0;
    gdds_image_release(&image); gdds_buffer_release(&dds);
    return 1;
}

static int test_bc5_normal_decode_rejects_non_bc5(void) {
    enum { W = 8, H = 8 };
    gdds_u8 rgba[W * H * 4u];
    gdds_encode_options enc;
    gdds_buffer dds;
    gdds_image image;
    enc = gdds_encode_options_default(GDDS_FORMAT_BC4_UNORM);
    memset(&dds, 0, sizeof(dds)); memset(&image, 0, sizeof(image)); memset(rgba, 0, sizeof(rgba));
    if (!expect_result("encode bc4", gdds_encode_memory_rgba8(rgba, W, H, &enc, &dds), GDDS_RESULT_OK)) return 0;
    if (!expect_result("decode bc5 normal reject",
                       gdds_decode_bc5_normal_map_memory(dds.data, dds.size, NULL, &image), GDDS_RESULT_UNSUPPORTED)) return 0;
    gdds_buffer_release(&dds);
    return 1;
}

int main(void) {
    if (!test_inplace_normal_utilities()) return 1;
    if (!test_bc5_normal_encode_decode_xyz()) return 1;
    if (!test_bc5_normal_auto_mips_xy()) return 1;
    if (!test_bc5_normal_decode_rejects_non_bc5()) return 1;
    puts("ok");
    return 0;
}
