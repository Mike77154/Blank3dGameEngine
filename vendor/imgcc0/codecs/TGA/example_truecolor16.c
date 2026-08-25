#include "gafnyf_tga.h"

#include <stdio.h>
#include <string.h>

static void write_u16_le(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)(value & 255u);
    p[1] = (unsigned char)((value >> 8) & 255u);
}

int main(void)
{
    unsigned char rgb565_pixels[4];
    unsigned char encoded[256];
    unsigned char decoded[4];
    size_t encoded_size;
    int rc;
    tga_encode_params params;
    tga_decode_params dec;
    tga_info info;
    tga_u8 r;
    tga_u8 g;
    tga_u8 b;

    write_u16_le(rgb565_pixels + 0u, (unsigned)tga_pack_rgb565(255u, 0u, 0u));
    write_u16_le(rgb565_pixels + 2u, (unsigned)tga_pack_rgb565(0u, 255u, 0u));

    memset(&params, 0, sizeof(params));
    params.pixels = rgb565_pixels;
    params.width = 2u;
    params.height = 1u;
    params.stride_bytes = 4u;
    params.pixel_format = TGA_PIXFMT_RGB565;
    params.file_pixel_depth = 16u;
    params.write_footer = 1;

    rc = tga_encode_memory(encoded, sizeof(encoded), &encoded_size, &params);
    if (rc != TGA_OK) {
        fprintf(stderr, "encode failed: %s\n", tga_result_string(rc));
        return 1;
    }

    memset(decoded, 0, sizeof(decoded));
    memset(&dec, 0, sizeof(dec));
    dec.pixels = decoded;
    dec.stride_bytes = 4u;
    dec.pixel_format = TGA_PIXFMT_RGB565;
    rc = tga_decode_memory(encoded, encoded_size, &dec, &info);
    if (rc != TGA_OK) {
        fprintf(stderr, "decode failed: %s\n", tga_result_string(rc));
        return 1;
    }

    tga_unpack_rgb565((tga_u16)((unsigned)decoded[0] | ((unsigned)decoded[1] << 8)),
                      &r, &g, &b);
    printf("encoded %lu bytes, pixel0 RGB=(%u,%u,%u), file_depth=%u\n",
           (unsigned long)encoded_size,
           (unsigned)r, (unsigned)g, (unsigned)b,
           info.pixel_depth);

    return 0;
}
