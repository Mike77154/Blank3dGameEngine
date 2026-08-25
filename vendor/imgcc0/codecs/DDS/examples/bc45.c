#include <stdio.h>
#include <string.h>

#include "giffany_dds.h"

static void make_height_rgba(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_u32 x, y;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            gdds_u8 v = (gdds_u8)(((x + y) * 255u) / (((w - 1u) + (h - 1u)) ? ((w - 1u) + (h - 1u)) : 1u));
            gdds_u8* p = rgba + (((gdds_size)y * w + x) * 4u);
            p[0] = v;
            p[1] = v;
            p[2] = v;
            p[3] = 255u;
        }
    }
}

static void make_normal_rgba(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_u32 x, y;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            gdds_u8* p = rgba + (((gdds_size)y * w + x) * 4u);
            p[0] = (gdds_u8)((x * 255u) / (w > 1u ? (w - 1u) : 1u));
            p[1] = (gdds_u8)((y * 255u) / (h > 1u ? (h - 1u) : 1u));
            p[2] = 255u;
            p[3] = 255u;
        }
    }
}

int main(void) {
    enum { W = 8, H = 8 };
    gdds_u8 height_rgba[W * H * 4u];
    gdds_u8 normal_rgba[W * H * 4u];
    gdds_encode_options bc4 = gdds_encode_options_default(GDDS_FORMAT_BC4_UNORM);
    gdds_encode_options bc5 = gdds_encode_options_default(GDDS_FORMAT_BC5_UNORM);
    gdds_buffer height_dds = {0};
    gdds_buffer normal_dds = {0};
    gdds_info info = {0};

    make_height_rgba(height_rgba, W, H);
    make_normal_rgba(normal_rgba, W, H);

    if (gdds_encode_memory_rgba8(height_rgba, W, H, &bc4, &height_dds) != GDDS_RESULT_OK) {
        fprintf(stderr, "BC4 encode failed\n");
        return 1;
    }
    if (gdds_encode_memory_rgba8(normal_rgba, W, H, &bc5, &normal_dds) != GDDS_RESULT_OK) {
        fprintf(stderr, "BC5 encode failed\n");
        gdds_buffer_release(&height_dds);
        return 1;
    }
    if (gdds_inspect_memory(normal_dds.data, normal_dds.size, &info) != GDDS_RESULT_OK) {
        fprintf(stderr, "BC5 inspect failed\n");
        gdds_buffer_release(&normal_dds);
        gdds_buffer_release(&height_dds);
        return 1;
    }

    printf("BC4 bytes: %u\n", height_dds.size);
    printf("BC5 bytes: %u\n", normal_dds.size);
    printf("BC5 source_format=%d dx10=%d offset=%u\n",
           (int)info.source_format,
           info.has_dx10_header,
           info.data_offset);

    gdds_buffer_release(&normal_dds);
    gdds_buffer_release(&height_dds);
    return 0;
}
