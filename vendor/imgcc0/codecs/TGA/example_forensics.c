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
    unsigned char pixel[4] = { 80u, 40u, 20u, 255u };
    unsigned char encoded[2048];
    unsigned char text_payload[3] = { 'b', 'a', 'd' };
    unsigned char u32_payload[4] = { 1u, 0u, 0u, 0u };
    size_t encoded_size = 0u;
    int rc;
    tga_extension ext;
    tga_developer_field fields[2];
    tga_encode_params_ex params;
    tga_footer footer;
    tga_forensic_report report;

    memset(&ext, 0, sizeof(ext));
    strcpy(ext.software_id, "example-forensics");

    memset(fields, 0, sizeof(fields));
    fields[0].tag = TGA_UTIL_TAG_TEXT_UTF8;
    fields[0].data = text_payload;
    fields[0].data_size = 3u;
    fields[1].tag = TGA_UTIL_TAG_U32;
    fields[1].data = u32_payload;
    fields[1].data_size = 4u;

    memset(&params, 0, sizeof(params));
    params.image.pixels = pixel;
    params.image.width = 1u;
    params.image.height = 1u;
    params.image.stride_bytes = 4u;
    params.image.pixel_format = TGA_PIXFMT_RGBA32;
    params.image.file_pixel_depth = 32u;
    params.image.write_footer = 1;
    params.extension = &ext;
    params.developer_fields = fields;
    params.developer_field_count = 2u;

    rc = tga_encode_ex_memory(encoded, sizeof(encoded), &encoded_size, &params);
    if (rc != TGA_OK) {
        fprintf(stderr, "encode_ex failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_read_footer_memory(encoded, encoded_size, &footer);
    if (rc != TGA_OK) {
        fprintf(stderr, "footer failed: %s\n", tga_result_string(rc));
        return 1;
    }

    write_u16_le(encoded + (size_t)footer.extension_offset, 494u);
    write_u32_le(encoded + (size_t)footer.extension_offset + 482u,
                 (unsigned long)encoded_size - 1ul);

    memset(&report, 0, sizeof(report));
    rc = tga_forensics_memory(encoded, encoded_size, &report);
    if (rc != TGA_OK) {
        fprintf(stderr, "forensics failed: %s\n", tga_result_string(rc));
        return 1;
    }

    printf("flags=0x%08x ext_mismatch=%u bad_color_ref=%u text_not_terminated=%u\n",
           report.flags,
           report.extension_size_mismatch_count,
           report.bad_color_correction_ref_count,
           report.utility_text_not_terminated_count);
    return 0;
}
