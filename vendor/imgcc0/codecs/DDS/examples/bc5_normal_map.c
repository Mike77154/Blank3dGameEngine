#include <stdio.h>

#include "giffany_dds.h"
#include "example_support89.h"

static void make_normal_rgba(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_example_make_signed_xy(rgba, w, h, 2, 5);
    (void)gdds_normal_map_reconstruct_z_rgba8(rgba, w, h);
    (void)gdds_normal_map_normalize_rgba8(rgba, w, h);
}

int main(void) {
    enum { W = 16, H = 16 };
    gdds_u8 rgba[W * H * 4u];
    gdds_normal_map_encode_options enc;
    gdds_normal_map_decode_options dec;
    gdds_buffer dds;
    gdds_image decoded;

    memset(&dds, 0, sizeof(dds));
    memset(&decoded, 0, sizeof(decoded));
    enc = gdds_normal_map_encode_options_default();
    dec = gdds_normal_map_decode_options_default();
    make_normal_rgba(rgba, W, H);

    enc.input_layout = GDDS_NORMAL_MAP_LAYOUT_XYZ_RGB;
    enc.output_format = GDDS_FORMAT_BC5_UNORM;
    enc.normalize = 1;
    if (gdds_encode_bc5_normal_map_rgba8(rgba, W, H, &enc, &dds) != GDDS_RESULT_OK) {
        fprintf(stderr, "encode failed\n");
        return 1;
    }

    dec.reconstruct_z = 1;
    dec.normalize = 1;
    if (gdds_decode_bc5_normal_map_memory(dds.data, dds.size, &dec, &decoded) != GDDS_RESULT_OK) {
        fprintf(stderr, "decode failed\n");
        gdds_buffer_release(&dds);
        return 1;
    }

    printf("BC5 normal-map DDS bytes: %u\n", dds.size);
    printf("decoded %ux%u, pixel[0]=(%u,%u,%u,%u)\n",
           decoded.width,
           decoded.height,
           decoded.pixels[0], decoded.pixels[1], decoded.pixels[2], decoded.pixels[3]);

    gdds_image_release(&decoded);
    gdds_buffer_release(&dds);
    return 0;
}
