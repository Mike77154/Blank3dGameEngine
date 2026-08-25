#include "gafnyf_tga.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    unsigned char pixels[4] = { 0u, 1u, 1u, 0u };
    unsigned char palette[8] = {
        255u,   0u,   0u, 255u,
          0u, 128u, 255u, 255u
    };
    unsigned char file_data[256];
    size_t file_size = 0u;
    int rc;
    tga_encode_params params;

    memset(&params, 0, sizeof(params));
    params.pixels = pixels;
    params.width = 2u;
    params.height = 2u;
    params.stride_bytes = 2u;
    params.pixel_format = TGA_PIXFMT_INDEX8;
    params.palette = palette;
    params.palette_count = 2u;
    params.palette_format = TGA_PIXFMT_RGBA32;
    params.palette_file_entry_bits = 16u;
    params.file_pixel_depth = 8u;
    params.write_footer = 1;

    rc = tga_encode_memory(file_data, sizeof(file_data), &file_size, &params);
    if (rc != TGA_OK) {
        fprintf(stderr, "encode failed: %s\n", tga_result_string(rc));
        return 1;
    }

    printf("encoded %lu bytes with %u-bit palette entries\n",
           (unsigned long)file_size, params.palette_file_entry_bits);
    return 0;
}
