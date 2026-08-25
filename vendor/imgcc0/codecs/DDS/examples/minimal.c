#include <stdio.h>
#include <string.h>

#include "giffany_dds.h"

static void fill_demo_image(gdds_u8* rgba, gdds_u32 width, gdds_u32 height) {
    gdds_u32 x, y;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            gdds_u8* p = rgba + (((gdds_size)y * width + x) * 4u);
            p[0] = (gdds_u8)((x * 255u) / (width > 1u ? width - 1u : 1u));
            p[1] = (gdds_u8)((y * 255u) / (height > 1u ? height - 1u : 1u));
            p[2] = (gdds_u8)(((x ^ y) * 255u) / (((width > height ? width : height) > 1u) ? ((width > height ? width : height) - 1u) : 1u));
            p[3] = 255u;
        }
    }
}

int main(void) {
    const gdds_u32 width = 8u;
    const gdds_u32 height = 8u;
    gdds_u8 rgba[8u * 8u * 4u];
    gdds_encode_options enc = gdds_encode_options_default(GDDS_FORMAT_DXT5);
    gdds_buffer dds = {0};
    gdds_info info = {0};
    gdds_image decoded = {0};
    gdds_result rc;

    fill_demo_image(rgba, width, height);

    rc = gdds_encode_memory_rgba8(rgba, width, height, &enc, &dds);
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

    rc = gdds_decode_memory(dds.data, dds.size, &decoded);
    if (rc != GDDS_RESULT_OK) {
        fprintf(stderr, "decode failed: %s\n", gdds_result_string(rc));
        gdds_buffer_release(&dds);
        return 1;
    }

    printf("encoded %u bytes, decoded %ux%u, source format=%d, alpha mode=%d, warnings=0x%X\n",
           dds.size,
           decoded.width,
           decoded.height,
           (int)decoded.source_format,
           (int)decoded.alpha_mode,
           info.warning_flags);

    gdds_image_release(&decoded);
    gdds_buffer_release(&dds);
    return 0;
}
