#include "gafnyf_tga.h"

#include <stdio.h>
#include <string.h>

int main(void)
{
    unsigned char pixels[8] = {
        255u,   0u,   0u, 255u,
          0u, 128u, 255u, 128u
    };
    unsigned char out[1024];
    size_t out_size = 0u;
    int rc;
    tga_encode_params_ex params;
    tga_extension ext;
    tga_extension read_back;
    tga_u32 aspect_q16;
    tga_u32 gamma_q16;

    memset(&ext, 0, sizeof(ext));
    strcpy(ext.software_id, "gafnyf_tga v2.9 example");
    ext.attributes_type = TGA_ATTRIBUTES_TYPE_ALPHA;
    (void)tga_extension_set_pixel_aspect_q16(&ext, 0x00018000ul);
    (void)tga_extension_set_gamma_q16(&ext, 0x00010000ul);

    memset(&params, 0, sizeof(params));
    params.image.pixels = pixels;
    params.image.width = 2u;
    params.image.height = 1u;
    params.image.stride_bytes = 8u;
    params.image.pixel_format = TGA_PIXFMT_RGBA32;
    params.image.file_pixel_depth = 32u;
    params.image.write_footer = 1;
    params.extension = &ext;
    params.write_scan_line_table = 1;

    rc = tga_encode_ex_memory(out, sizeof(out), &out_size, &params);
    if (rc != TGA_OK) {
        fprintf(stderr, "encode failed: %s\n", tga_result_string(rc));
        return 1;
    }

    rc = tga_read_extension_memory(out, out_size, &read_back);
    if (rc != TGA_OK) {
        fprintf(stderr, "read extension failed: %s\n", tga_result_string(rc));
        return 1;
    }

    rc = tga_extension_pixel_aspect_q16(&read_back, &aspect_q16);
    if (rc != TGA_OK) {
        fprintf(stderr, "aspect read failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_extension_gamma_q16(&read_back, &gamma_q16);
    if (rc != TGA_OK) {
        fprintf(stderr, "gamma read failed: %s\n", tga_result_string(rc));
        return 1;
    }

    printf("encoded bytes: %lu\n", (unsigned long)out_size);
    printf("attr type: %s\n", tga_attributes_type_string(read_back.attributes_type));
    printf("pixel aspect q16: 0x%08lx\n", (unsigned long)aspect_q16);
    printf("gamma q16: 0x%08lx\n", (unsigned long)gamma_q16);

    rc = tga_patch_extension_attributes_type_memory(out, out_size,
                                                    TGA_ATTRIBUTES_TYPE_PREMULTIPLIED);
    if (rc != TGA_OK) {
        fprintf(stderr, "patch attr failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_read_extension_memory(out, out_size, &read_back);
    if (rc != TGA_OK) {
        fprintf(stderr, "read patched extension failed: %s\n", tga_result_string(rc));
        return 1;
    }
    printf("patched attr type: %s\n",
           tga_attributes_type_string(read_back.attributes_type));

    return 0;
}
