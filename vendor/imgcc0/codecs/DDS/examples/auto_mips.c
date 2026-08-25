#include <stdio.h>
#include <string.h>

#include "giffany_dds.h"

static void make_checker(gdds_u8* rgba, gdds_u32 width, gdds_u32 height) {
    gdds_u32 x, y;
    for (y = 0; y < height; ++y) {
        for (x = 0; x < width; ++x) {
            gdds_u8* p = rgba + (((gdds_size)y * width + x) * 4u);
            int on = (((x >> 3u) ^ (y >> 3u)) & 1u) != 0;
            p[0] = on ? 255u : 20u;
            p[1] = on ? 80u : 220u;
            p[2] = on ? 220u : 40u;
            p[3] = (gdds_u8)((x * 255u) / (width > 1u ? (width - 1u) : 1u));
        }
    }
}

int main(void) {
    enum { WIDTH = 32, HEIGHT = 16 };
    gdds_u8 rgba[WIDTH * HEIGHT * 4u];
    gdds_encode_options enc = gdds_encode_options_default(GDDS_FORMAT_DXT5);
    gdds_mipmap_options mips = gdds_mipmap_options_default();
    gdds_generated_mipchain chain = {0};
    gdds_buffer dds = {0};
    gdds_info info = {0};
    gdds_result rc;

    make_checker(rgba, WIDTH, HEIGHT);

    mips.alpha_filter = GDDS_MIP_ALPHA_FILTER_PREMULTIPLIED;

    rc = gdds_generate_mipchain_rgba8(rgba, WIDTH, HEIGHT, &mips, &chain);
    if (rc != GDDS_RESULT_OK) {
        fprintf(stderr, "generate failed: %s\n", gdds_result_string(rc));
        return 1;
    }

    rc = gdds_encode_memory_rgba8_auto_mips(rgba, WIDTH, HEIGHT, &enc, &mips, &dds);
    if (rc != GDDS_RESULT_OK) {
        fprintf(stderr, "encode failed: %s\n", gdds_result_string(rc));
        gdds_generated_mipchain_release(&chain);
        return 1;
    }

    rc = gdds_inspect_memory(dds.data, dds.size, &info);
    if (rc != GDDS_RESULT_OK) {
        fprintf(stderr, "inspect failed: %s\n", gdds_result_string(rc));
        gdds_buffer_release(&dds);
        gdds_generated_mipchain_release(&chain);
        return 1;
    }

    printf("generated %u mip levels, wrote %u-byte DDS with %u mips\n",
           chain.level_count,
           dds.size,
           info.mip_count);

    gdds_buffer_release(&dds);
    gdds_generated_mipchain_release(&chain);
    return 0;
}
