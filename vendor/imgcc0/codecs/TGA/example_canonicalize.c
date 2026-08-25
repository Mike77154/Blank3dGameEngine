#include "gafnyf_tga.h"

#include <stdio.h>
#include <string.h>

static void write_u16_le(unsigned char *p, unsigned value)
{
    p[0] = (unsigned char)(value & 255u);
    p[1] = (unsigned char)((value >> 8) & 255u);
}

static void write_u32_le(unsigned char *p, unsigned long value)
{
    p[0] = (unsigned char)(value & 255ul);
    p[1] = (unsigned char)((value >> 8) & 255ul);
    p[2] = (unsigned char)((value >> 16) & 255ul);
    p[3] = (unsigned char)((value >> 24) & 255ul);
}

int main(void)
{
    unsigned char pixels[4];
    unsigned char payload[4];
    unsigned char encoded[1024];
    unsigned char canonical[2048];
    size_t encoded_size;
    size_t canonical_size;
    int rc;
    tga_encode_params_ex params;
    tga_developer_field field;
    tga_extension ext;
    tga_footer footer;
    unsigned tag_count;

    pixels[0] = 40u;
    pixels[1] = 80u;
    pixels[2] = 120u;
    pixels[3] = 255u;
    write_u32_le(payload, 0xCAFEBABEul);

    memset(&field, 0, sizeof(field));
    field.tag = TGA_UTIL_TAG_U32;
    field.data = payload;
    field.data_size = 4u;

    memset(&params, 0, sizeof(params));
    params.image.pixels = pixels;
    params.image.width = 1u;
    params.image.height = 1u;
    params.image.stride_bytes = 4u;
    params.image.pixel_format = TGA_PIXFMT_RGBA32;
    params.image.file_pixel_depth = 32u;
    params.image.write_footer = 1;
    params.write_scan_line_table = 1;
    params.developer_fields = &field;
    params.developer_field_count = 1u;

    rc = tga_encode_ex_memory(encoded, sizeof(encoded), &encoded_size, &params);
    if (rc != TGA_OK) {
        fprintf(stderr, "encode_ex failed: %s\n", tga_result_string(rc));
        return 1;
    }

    rc = tga_read_footer_memory(encoded, encoded_size, &footer);
    if (rc != TGA_OK) {
        fprintf(stderr, "footer read failed: %s\n", tga_result_string(rc));
        return 1;
    }

    write_u16_le(encoded + (size_t)footer.extension_offset, 496u);
    rc = tga_canonicalize_memory(canonical, sizeof(canonical), &canonical_size,
                                 encoded, encoded_size, 0);
    if (rc != TGA_OK) {
        fprintf(stderr, "canonicalize failed: %s\n", tga_result_string(rc));
        return 1;
    }

    rc = tga_read_extension_memory(canonical, canonical_size, &ext);
    if (rc != TGA_OK) {
        fprintf(stderr, "read extension failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_read_developer_directory_memory(canonical, canonical_size,
                                             0, 0u, &tag_count);
    if (rc != TGA_OK) {
        fprintf(stderr, "read developer dir failed: %s\n", tga_result_string(rc));
        return 1;
    }

    printf("canonicalized %lu -> %lu bytes, ext_size=%u, dev_tags=%u\n",
           (unsigned long)encoded_size,
           (unsigned long)canonical_size,
           ext.size,
           tag_count);
    return 0;
}
