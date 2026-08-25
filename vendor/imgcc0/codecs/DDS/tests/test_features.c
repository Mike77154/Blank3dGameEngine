#include <stdio.h>
#include <string.h>
#include "giffany_dds/gdds.h"

#define W 13u
#define H 11u
static gdds_u8 pixels[W * H * 4u];

static void fill_pixels(void) {
    gdds_u32 x;
    gdds_u32 y;
    for (y = 0u; y < H; ++y) {
        for (x = 0u; x < W; ++x) {
            gdds_size i = (y * W + x) * 4u;
            pixels[i + 0u] = (gdds_u8)((x * 23u + y * 3u + 29u) & 255u);
            pixels[i + 1u] = (gdds_u8)((x * 7u + y * 31u + 61u) & 255u);
            pixels[i + 2u] = (gdds_u8)((x * 11u + y * 19u + 113u) & 255u);
            pixels[i + 3u] = 255u;
        }
    }
}

int main(void) {
    gdds_encode_options eo;
    gdds_mipmap_options mo;
    gdds_normal_map_encode_options no;
    gdds_buffer encoded;
    gdds_info info;
    gdds_mip_info mip;
    gdds_image image;
    gdds_u8 work[W * H * 4u];
    gdds_result rc;
    gdds_u32 level;

    fill_pixels();
    eo = gdds_encode_options_default(GDDS_FORMAT_DXT5);
    mo = gdds_mipmap_options_default();
    mo.mip_count = 4u;
    memset(&encoded, 0, sizeof(encoded));
    rc = gdds_encode_memory_rgba8_auto_mips(pixels, W, H, &eo, &mo, &encoded);
    if (rc != GDDS_RESULT_OK) return 1;
    rc = gdds_inspect_memory(encoded.data, encoded.size, &info);
    if (rc != GDDS_RESULT_OK || info.mip_count != 4u) return 2;
    for (level = 0u; level < 4u; ++level) {
        rc = gdds_get_mip_info(encoded.data, encoded.size, level, &mip);
        if (rc != GDDS_RESULT_OK || mip.data_size == 0u) return 3;
        memset(&image, 0, sizeof(image));
        rc = gdds_decode_mip_memory(encoded.data, encoded.size, level, &image);
        if (rc != GDDS_RESULT_OK) return 4;
        gdds_image_release(&image);
    }
    gdds_buffer_release(&encoded);

    memcpy(work, pixels, sizeof(work));
    if (gdds_normal_map_reconstruct_z_rgba8(work, W, H) != GDDS_RESULT_OK) return 5;
    if (gdds_normal_map_normalize_rgba8(work, W, H) != GDDS_RESULT_OK) return 6;
    if (gdds_normal_map_invert_y_rgba8(work, W, H) != GDDS_RESULT_OK) return 7;

    no = gdds_normal_map_encode_options_default();
    no.input_layout = GDDS_NORMAL_MAP_LAYOUT_XY_RG;
    no.output_format = GDDS_FORMAT_BC5_SNORM;
    mo = gdds_mipmap_options_default();
    mo.mip_count = 4u;
    memset(&encoded, 0, sizeof(encoded));
    rc = gdds_encode_bc5_normal_map_rgba8_auto_mips(pixels, W, H, &no, &mo, &encoded);
    if (rc != GDDS_RESULT_OK) return 8;
    memset(&image, 0, sizeof(image));
    rc = gdds_decode_bc5_normal_map_memory(encoded.data, encoded.size, 0, &image);
    if (rc != GDDS_RESULT_OK || image.width != W || image.height != H) return 9;
    gdds_image_release(&image);
    gdds_buffer_release(&encoded);
    puts("features: ok");
    return 0;
}
