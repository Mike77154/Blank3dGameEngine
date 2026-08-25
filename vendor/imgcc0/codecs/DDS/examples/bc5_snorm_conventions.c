#include <stdio.h>
#include <string.h>

#include "giffany_dds.h"
#include "example_support89.h"

static void make_opengl_xy_normal_map(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_example_make_signed_xy(rgba, w, h, 2, 5);
}

int main(void) {
    enum { W = 16, H = 16 };
    gdds_u8 rgba[W * H * 4u];
    gdds_normal_map_encode_options enc;
    gdds_normal_map_decode_options dec;
    gdds_buffer dds;
    gdds_image decoded;
    gdds_info info;

    memset(&dds, 0, sizeof(dds));
    memset(&decoded, 0, sizeof(decoded));
    memset(&info, 0, sizeof(info));
    enc = gdds_normal_map_encode_options_default();
    dec = gdds_normal_map_decode_options_default();
    make_opengl_xy_normal_map(rgba, W, H);

    enc.input_layout = GDDS_NORMAL_MAP_LAYOUT_XY_RG;
    enc.input_convention = GDDS_NORMAL_MAP_CONVENTION_OPENGL;
    enc.output_convention = GDDS_NORMAL_MAP_CONVENTION_DIRECTX;
    enc.output_format = GDDS_FORMAT_BC5_SNORM;

    if (gdds_encode_bc5_normal_map_rgba8(rgba, W, H, &enc, &dds) != GDDS_RESULT_OK) {
        fprintf(stderr, "encode failed\n");
        return 1;
    }
    if (gdds_inspect_memory(dds.data, dds.size, &info) != GDDS_RESULT_OK) {
        fprintf(stderr, "inspect failed\n");
        gdds_buffer_release(&dds);
        return 1;
    }

    dec.stored_convention = GDDS_NORMAL_MAP_CONVENTION_DIRECTX;
    dec.output_convention = GDDS_NORMAL_MAP_CONVENTION_GLTF;
    if (gdds_decode_bc5_normal_map_memory(dds.data, dds.size, &dec, &decoded) != GDDS_RESULT_OK) {
        fprintf(stderr, "decode failed\n");
        gdds_buffer_release(&dds);
        return 1;
    }

    printf("format: %d\n", (int)info.source_format);
    printf("convention export: %s -> %s\n",
           gdds_normal_map_convention_string(enc.input_convention),
           gdds_normal_map_convention_string(enc.output_convention));
    printf("decoded pixel[0] = (%u, %u, %u, %u)\n",
           decoded.pixels[0], decoded.pixels[1], decoded.pixels[2], decoded.pixels[3]);

    gdds_image_release(&decoded);
    gdds_buffer_release(&dds);
    return 0;
}
