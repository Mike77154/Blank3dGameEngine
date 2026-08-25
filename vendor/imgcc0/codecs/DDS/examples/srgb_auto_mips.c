#include <stdio.h>

#include "giffany_dds.h"

static void make_bw_checker(gdds_u8* rgba) {
    static const gdds_u8 pixels[2u * 2u * 4u] = {
          0u,   0u,   0u, 255u,
        255u, 255u, 255u, 255u,
          0u,   0u,   0u, 255u,
        255u, 255u, 255u, 255u
    };
    gdds_u32 i;
    for (i = 0u; i < sizeof(pixels); ++i) {
        rgba[i] = pixels[i];
    }
}

int main(void) {
    gdds_u8 rgba[2u * 2u * 4u];
    gdds_encode_options enc = gdds_encode_options_default(GDDS_FORMAT_RGBA8_SRGB);
    gdds_buffer dds = {0};
    gdds_image mip1 = {0};
    gdds_info info = {0};
    gdds_result rc;

    make_bw_checker(rgba);

    /* With mip options omitted, AUTO color space is inferred from the output format. */
    rc = gdds_encode_memory_rgba8_auto_mips(rgba, 2u, 2u, &enc, NULL, &dds);
    if (rc != GDDS_RESULT_OK) {
        fprintf(stderr, "encode failed: %s\n", gdds_result_string(rc));
        return 1;
    }

    rc = gdds_inspect_memory(dds.data, dds.size, &info);
    if (rc != GDDS_RESULT_OK) {
        fprintf(stderr, "inspect failed: %s\n", gdds_result_string(rc));
        gdds_buffer_release(&dds);
        return 1;
    }

    rc = gdds_decode_mip_memory(dds.data, dds.size, 1u, &mip1);
    if (rc != GDDS_RESULT_OK) {
        fprintf(stderr, "decode failed: %s\n", gdds_result_string(rc));
        gdds_buffer_release(&dds);
        return 1;
    }

    printf("source format: %d, dx10: %d, sRGB: %d, mip1 gray: %u\n",
           (int)info.source_format,
           info.has_dx10_header,
           info.is_srgb,
           mip1.pixels[0]);

    gdds_image_release(&mip1);
    gdds_buffer_release(&dds);
    return 0;
}
