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

static void make_opengl_xy_normal_map(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_test_make_signed_xy(rgba, w, h, 3, 5);
}

static int test_convention_flip_helper(void) {
    enum { W = 8, H = 8 };
    gdds_u8 rgba[W * H * 4u];
    gdds_u8 original[W * H * 4u];
    gdds_size i;
    make_opengl_xy_normal_map(rgba, W, H);
    memcpy(original, rgba, sizeof(rgba));

    if (strcmp(gdds_normal_map_convention_string(GDDS_NORMAL_MAP_CONVENTION_OPENGL), "opengl") != 0) return 0;
    if (!gdds_normal_map_convention_needs_y_flip(GDDS_NORMAL_MAP_CONVENTION_OPENGL,
                                                  GDDS_NORMAL_MAP_CONVENTION_DIRECTX)) return 0;
    if (gdds_normal_map_convention_needs_y_flip(GDDS_NORMAL_MAP_CONVENTION_OPENGL,
                                                 GDDS_NORMAL_MAP_CONVENTION_GLTF)) return 0;
    if (!expect_result("OpenGL to DirectX",
                       gdds_normal_map_convert_convention_rgba8(rgba, W, H,
                                                               GDDS_NORMAL_MAP_LAYOUT_XY_RG,
                                                               GDDS_NORMAL_MAP_CONVENTION_OPENGL,
                                                               GDDS_NORMAL_MAP_CONVENTION_DIRECTX),
                       GDDS_RESULT_OK)) return 0;
    for (i = 0u; i < (gdds_size)(W * H); ++i) {
        if (rgba[i * 4u + 1u] != (gdds_u8)(255u - original[i * 4u + 1u])) return 0;
    }
    if (!expect_result("DirectX to OpenGL",
                       gdds_normal_map_convert_convention_rgba8(rgba, W, H,
                                                               GDDS_NORMAL_MAP_LAYOUT_XY_RG,
                                                               GDDS_NORMAL_MAP_CONVENTION_DIRECTX,
                                                               GDDS_NORMAL_MAP_CONVENTION_OPENGL),
                       GDDS_RESULT_OK)) return 0;
    if (memcmp(rgba, original, sizeof(rgba)) != 0) return 0;
    return 1;
}

static int test_bc5_snorm_opengl_to_gltf_pipeline(void) {
    enum { W = 16, H = 16 };
    gdds_u8 rgba_xy[W * H * 4u];
    gdds_u8 rgba_xyz[W * H * 4u];
    gdds_normal_map_encode_options enc;
    gdds_normal_map_decode_options dec;
    gdds_buffer dds;
    gdds_info info;
    gdds_image image;

    enc = gdds_normal_map_encode_options_default();
    dec = gdds_normal_map_decode_options_default();
    memset(&dds, 0, sizeof(dds)); memset(&info, 0, sizeof(info)); memset(&image, 0, sizeof(image));
    make_opengl_xy_normal_map(rgba_xy, W, H);
    memcpy(rgba_xyz, rgba_xy, sizeof(rgba_xy));
    if (!expect_result("reconstruct original z", gdds_normal_map_reconstruct_z_rgba8(rgba_xyz, W, H), GDDS_RESULT_OK)) return 0;
    if (!expect_result("normalize original xyz", gdds_normal_map_normalize_rgba8(rgba_xyz, W, H), GDDS_RESULT_OK)) return 0;

    enc.input_layout = GDDS_NORMAL_MAP_LAYOUT_XY_RG;
    enc.input_convention = GDDS_NORMAL_MAP_CONVENTION_OPENGL;
    enc.output_convention = GDDS_NORMAL_MAP_CONVENTION_DIRECTX;
    enc.output_format = GDDS_FORMAT_BC5_SNORM;
    enc.normalize = 1;
    if (!expect_result("encode BC5_SNORM normal",
                       gdds_encode_bc5_normal_map_rgba8(rgba_xy, W, H, &enc, &dds), GDDS_RESULT_OK)) return 0;
    if (!expect_result("inspect BC5_SNORM normal", gdds_inspect_memory(dds.data, dds.size, &info), GDDS_RESULT_OK)) return 0;
    if (info.source_format != GDDS_FORMAT_BC5_SNORM || !info.has_dx10_header || info.mip_count != 1u) return 0;

    dec.stored_convention = GDDS_NORMAL_MAP_CONVENTION_DIRECTX;
    dec.output_convention = GDDS_NORMAL_MAP_CONVENTION_GLTF;
    dec.reconstruct_z = 1;
    dec.normalize = 1;
    if (!expect_result("decode BC5_SNORM normal",
                       gdds_decode_bc5_normal_map_memory(dds.data, dds.size, &dec, &image), GDDS_RESULT_OK)) return 0;
    if (!gdds_test_channel_mae_le_x2(rgba_xyz, image.pixels, W, H, 0u, 0u, 23u) ||
        !gdds_test_channel_mae_le_x2(rgba_xyz, image.pixels, W, H, 1u, 1u, 23u) ||
        !gdds_test_channel_mae_le_x2(rgba_xyz, image.pixels, W, H, 2u, 2u, 37u)) return 0;
    if (!gdds_test_normals_unitish(image.pixels, W, H, 16u)) return 0;
    gdds_image_release(&image); gdds_buffer_release(&dds);
    return 1;
}

int main(void) {
    if (!test_convention_flip_helper()) return 1;
    if (!test_bc5_snorm_opengl_to_gltf_pipeline()) return 1;
    puts("test_normal_conventions: ok");
    return 0;
}
