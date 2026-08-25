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

static unsigned read_u16_le(const unsigned char *p)
{
    return (unsigned)p[0] | ((unsigned)p[1] << 8);
}

static unsigned long read_u32_le(const unsigned char *p)
{
    return (unsigned long)p[0] |
           ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}

static int test_indexed_palette16(void)
{
    unsigned char pixels[2];
    unsigned char palette_rgba[8];
    unsigned char file_data[256];
    unsigned char decoded_palette[8];
    size_t file_size;
    int rc;
    tga_encode_params params;
    tga_info info;
    tga_palette_info palette_info;

    pixels[0] = 0u;
    pixels[1] = 1u;

    palette_rgba[0] = 255u;
    palette_rgba[1] = 0u;
    palette_rgba[2] = 0u;
    palette_rgba[3] = 255u;
    palette_rgba[4] = 0u;
    palette_rgba[5] = 255u;
    palette_rgba[6] = 0u;
    palette_rgba[7] = 255u;

    memset(&params, 0, sizeof(params));
    params.pixels = pixels;
    params.width = 2u;
    params.height = 1u;
    params.stride_bytes = 2u;
    params.pixel_format = TGA_PIXFMT_INDEX8;
    params.palette = palette_rgba;
    params.palette_count = 2u;
    params.palette_first_index = 0u;
    params.palette_format = TGA_PIXFMT_RGBA32;
    params.palette_file_entry_bits = 16u;
    params.file_pixel_depth = 8u;
    params.write_footer = 1;

    rc = tga_encode_memory(file_data, sizeof(file_data), &file_size, &params);
    if (rc != TGA_OK) {
        fprintf(stderr, "encode indexed 16-bit palette failed: %s\n", tga_result_string(rc));
        return 1;
    }

    rc = tga_probe_memory(file_data, file_size, &info);
    if (rc != TGA_OK) {
        fprintf(stderr, "probe failed: %s\n", tga_result_string(rc));
        return 1;
    }
    if (info.color_map_entry_bits != 16u) {
        fprintf(stderr, "expected 16-bit palette entries, got %u\n", info.color_map_entry_bits);
        return 1;
    }

    memset(decoded_palette, 0, sizeof(decoded_palette));
    memset(&palette_info, 0, sizeof(palette_info));
    rc = tga_decode_color_map_rgba_memory(file_data, file_size,
                                          decoded_palette, 2u,
                                          &palette_info, &info);
    if (rc != TGA_OK) {
        fprintf(stderr, "decode palette failed: %s\n", tga_result_string(rc));
        return 1;
    }

    if (palette_info.entry_bits != 16u || palette_info.entry_count != 2u) {
        fprintf(stderr, "unexpected palette info: bits=%u count=%u\n",
                palette_info.entry_bits, palette_info.entry_count);
        return 1;
    }

    if (decoded_palette[0] != 255u || decoded_palette[1] != 0u ||
        decoded_palette[2] != 0u || decoded_palette[3] != 255u) {
        fprintf(stderr, "decoded red palette entry mismatch\n");
        return 1;
    }
    if (decoded_palette[4] != 0u || decoded_palette[5] != 255u ||
        decoded_palette[6] != 0u || decoded_palette[7] != 255u) {
        fprintf(stderr, "decoded green palette entry mismatch\n");
        return 1;
    }

    return 0;
}

static int test_forensics_v28(void)
{
    unsigned char pixels[4];
    unsigned char encoded[4096];
    unsigned char payload_text[2];
    unsigned char payload_u32[4];
    unsigned char payload_rect[16];
    size_t encoded_size;
    int rc;
    tga_encode_params_ex params;
    tga_extension ext;
    tga_developer_field fields[3];
    tga_footer footer;
    tga_forensic_report report;
    unsigned long tag0_off;
    unsigned dir_count;
    unsigned char *dir;

    pixels[0] = 10u;
    pixels[1] = 20u;
    pixels[2] = 30u;
    pixels[3] = 255u;

    payload_text[0] = 'o';
    payload_text[1] = 'k';
    write_u32_le(payload_u32, 1234ul);
    memset(payload_rect, 0, sizeof(payload_rect));
    write_u32_le(payload_rect + 0, 1ul);
    write_u32_le(payload_rect + 4, 2ul);
    write_u32_le(payload_rect + 8, 3ul);
    write_u32_le(payload_rect + 12, 4ul);

    memset(&ext, 0, sizeof(ext));
    strcpy(ext.software_id, "v28-selftest");

    memset(fields, 0, sizeof(fields));
    fields[0].tag = TGA_UTIL_TAG_TEXT_UTF8;
    fields[0].data = payload_text;
    fields[0].data_size = 2u;
    fields[1].tag = TGA_UTIL_TAG_U32;
    fields[1].data = payload_u32;
    fields[1].data_size = 4u;
    fields[2].tag = TGA_UTIL_TAG_RECT_U32;
    fields[2].data = payload_rect;
    fields[2].data_size = 16u;

    memset(&params, 0, sizeof(params));
    params.image.pixels = pixels;
    params.image.width = 1u;
    params.image.height = 1u;
    params.image.stride_bytes = 4u;
    params.image.pixel_format = TGA_PIXFMT_RGBA32;
    params.image.file_pixel_depth = 32u;
    params.image.write_footer = 1;
    params.extension = &ext;
    params.developer_fields = fields;
    params.developer_field_count = 3u;

    rc = tga_encode_ex_memory(encoded, sizeof(encoded), &encoded_size, &params);
    if (rc != TGA_OK) {
        fprintf(stderr, "encode_ex failed: %s\n", tga_result_string(rc));
        return 1;
    }

    rc = tga_read_footer_memory(encoded, encoded_size, &footer);
    if (rc != TGA_OK || !footer.present) {
        fprintf(stderr, "footer read failed: %s\n", tga_result_string(rc));
        return 1;
    }

    if (footer.extension_offset == 0ul || footer.developer_offset == 0ul) {
        fprintf(stderr, "expected extension+developer offsets\n");
        return 1;
    }

    write_u16_le(encoded + (size_t)footer.extension_offset, 494u);
    write_u32_le(encoded + (size_t)footer.extension_offset + 482u,
                 (unsigned long)encoded_size - 1ul);

    dir = encoded + (size_t)footer.developer_offset;
    dir_count = read_u16_le(dir);
    if (dir_count != 3u) {
        fprintf(stderr, "expected 3 developer tags, got %u\n", dir_count);
        return 1;
    }

    tag0_off = read_u32_le(dir + 2u + 0u * 10u + 2u);
    write_u32_le(dir + 2u + 1u * 10u + 2u, tag0_off);
    write_u32_le(dir + 2u + 1u * 10u + 6u, 8ul);
    write_u32_le(dir + 2u + 2u * 10u + 2u, (unsigned long)encoded_size + 32ul);

    memset(&report, 0, sizeof(report));
    rc = tga_forensics_memory(encoded, encoded_size, &report);
    if (rc != TGA_OK) {
        fprintf(stderr, "forensics failed: %s\n", tga_result_string(rc));
        return 1;
    }

    if (report.extension_size_mismatch_count == 0u ||
        report.bad_color_correction_ref_count == 0u ||
        report.developer_field_overlap_count == 0u ||
        report.developer_field_out_of_file_count == 0u ||
        report.utility_payload_mismatch_count == 0u ||
        report.utility_text_not_terminated_count == 0u) {
        fprintf(stderr, "forensics counters missing: ext=%u color=%u overlap=%u oob=%u util=%u text=%u\n",
                report.extension_size_mismatch_count,
                report.bad_color_correction_ref_count,
                report.developer_field_overlap_count,
                report.developer_field_out_of_file_count,
                report.utility_payload_mismatch_count,
                report.utility_text_not_terminated_count);
        return 1;
    }

    if ((report.flags & TGA_FORENSIC_EXTENSION_SIZE_MISMATCH) == 0u ||
        (report.flags & TGA_FORENSIC_BAD_COLOR_CORRECTION_REF) == 0u ||
        (report.flags & TGA_FORENSIC_DEVELOPER_FIELD_OVERLAP) == 0u ||
        (report.flags & TGA_FORENSIC_DEVELOPER_FIELD_OUT_OF_FILE) == 0u ||
        (report.flags & TGA_FORENSIC_UTILITY_PAYLOAD_MISMATCH) == 0u ||
        (report.flags & TGA_FORENSIC_UTILITY_TEXT_NOT_TERMINATED) == 0u) {
        fprintf(stderr, "forensics flags missing: 0x%08x\n", report.flags);
        return 1;
    }

    return 0;
}

static int test_extension_helpers_v29(void)
{
    unsigned char pixels[4];
    unsigned char encoded[1024];
    size_t encoded_size;
    int rc;
    tga_encode_params_ex params;
    tga_extension ext;
    tga_u32 q16;

    pixels[0] = 1u;
    pixels[1] = 2u;
    pixels[2] = 3u;
    pixels[3] = 4u;

    memset(&params, 0, sizeof(params));
    params.image.pixels = pixels;
    params.image.width = 1u;
    params.image.height = 1u;
    params.image.stride_bytes = 4u;
    params.image.pixel_format = TGA_PIXFMT_RGBA32;
    params.image.file_pixel_depth = 32u;
    params.image.write_footer = 1;
    params.write_scan_line_table = 1;

    rc = tga_encode_ex_memory(encoded, sizeof(encoded), &encoded_size, &params);
    if (rc != TGA_OK) {
        fprintf(stderr, "encode_ex v29 failed: %s\n", tga_result_string(rc));
        return 1;
    }

    rc = tga_read_extension_memory(encoded, encoded_size, &ext);
    if (rc != TGA_OK) {
        fprintf(stderr, "read extension v29 failed: %s\n", tga_result_string(rc));
        return 1;
    }
    if (ext.attributes_type != TGA_ATTRIBUTES_TYPE_ALPHA) {
        fprintf(stderr, "expected auto alpha attributes type, got %u\n",
                ext.attributes_type);
        return 1;
    }
    if (tga_default_attributes_type_from_encode_params(&params.image) !=
        TGA_ATTRIBUTES_TYPE_ALPHA) {
        fprintf(stderr, "default attr type from encode params mismatch\n");
        return 1;
    }

    memset(&ext, 0, sizeof(ext));
    rc = tga_extension_set_pixel_aspect_q16(&ext, 0x00018000ul);
    if (rc != TGA_OK || ext.pixel_numerator != 3u || ext.pixel_denominator != 2u) {
        fprintf(stderr, "set pixel aspect q16 failed\n");
        return 1;
    }
    rc = tga_extension_pixel_aspect_q16(&ext, &q16);
    if (rc != TGA_OK || q16 != 0x00018000ul) {
        fprintf(stderr, "read pixel aspect q16 failed: %s %lu\n",
                tga_result_string(rc), (unsigned long)q16);
        return 1;
    }

    rc = tga_extension_set_gamma_q16(&ext, 0x00008000ul);
    if (rc != TGA_OK || ext.gamma_numerator != 1u || ext.gamma_denominator != 2u) {
        fprintf(stderr, "set gamma q16 failed\n");
        return 1;
    }
    rc = tga_extension_gamma_q16(&ext, &q16);
    if (rc != TGA_OK || q16 != 0x00008000ul) {
        fprintf(stderr, "read gamma q16 failed: %s %lu\n",
                tga_result_string(rc), (unsigned long)q16);
        return 1;
    }

    rc = tga_patch_extension_attributes_type_memory(encoded, encoded_size,
                                                    TGA_ATTRIBUTES_TYPE_PREMULTIPLIED);
    if (rc != TGA_OK) {
        fprintf(stderr, "patch attr memory failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_patch_extension_pixel_aspect_q16_memory(encoded, encoded_size,
                                                     0x00008000ul);
    if (rc != TGA_OK) {
        fprintf(stderr, "patch aspect memory failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_patch_extension_gamma_q16_memory(encoded, encoded_size,
                                              0x00020000ul);
    if (rc != TGA_OK) {
        fprintf(stderr, "patch gamma memory failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_read_extension_memory(encoded, encoded_size, &ext);
    if (rc != TGA_OK) {
        fprintf(stderr, "read patched extension failed: %s\n", tga_result_string(rc));
        return 1;
    }
    if (ext.attributes_type != TGA_ATTRIBUTES_TYPE_PREMULTIPLIED ||
        ext.pixel_numerator != 1u || ext.pixel_denominator != 2u ||
        ext.gamma_numerator != 2u || ext.gamma_denominator != 1u) {
        fprintf(stderr, "patched extension mismatch\n");
        return 1;
    }

    if (strcmp(tga_attributes_type_string(TGA_ATTRIBUTES_TYPE_PREMULTIPLIED),
               "TGA_ATTRIBUTES_TYPE_PREMULTIPLIED") != 0) {
        fprintf(stderr, "attribute type string mismatch\n");
        return 1;
    }

#ifndef GAFNYF_TGA_NO_STDIO
    {
        FILE *fp;
        tga_extension file_ext;

        fp = tmpfile();
        if (fp == 0) {
            fprintf(stderr, "tmpfile failed\n");
            return 1;
        }
        if (fwrite(encoded, 1u, encoded_size, fp) != encoded_size) {
            fprintf(stderr, "tmpfile write failed\n");
            fclose(fp);
            return 1;
        }
        rewind(fp);
        rc = tga_patch_extension_attributes_type_file(fp,
                                                      TGA_ATTRIBUTES_TYPE_ALPHA);
        if (rc != TGA_OK) {
            fprintf(stderr, "file attr patch failed: %s\n", tga_result_string(rc));
            fclose(fp);
            return 1;
        }
        rc = tga_patch_extension_pixel_aspect_q16_file(fp, 0x00010000ul);
        if (rc != TGA_OK) {
            fprintf(stderr, "file aspect patch failed: %s\n", tga_result_string(rc));
            fclose(fp);
            return 1;
        }
        rc = tga_patch_extension_gamma_q16_file(fp, 0x00008000ul);
        if (rc != TGA_OK) {
            fprintf(stderr, "file gamma patch failed: %s\n", tga_result_string(rc));
            fclose(fp);
            return 1;
        }
        rewind(fp);
        rc = tga_read_extension_file(fp, &file_ext);
        fclose(fp);
        if (rc != TGA_OK) {
            fprintf(stderr, "read patched file extension failed: %s\n", tga_result_string(rc));
            return 1;
        }
        if (file_ext.attributes_type != TGA_ATTRIBUTES_TYPE_ALPHA ||
            file_ext.pixel_numerator != 1u || file_ext.pixel_denominator != 1u ||
            file_ext.gamma_numerator != 1u || file_ext.gamma_denominator != 2u) {
            fprintf(stderr, "patched file extension mismatch\n");
            return 1;
        }
    }
#endif

    return 0;
}

static int test_forensics_v29_semantics(void)
{
    unsigned char pixels[4];
    unsigned char encoded[1024];
    size_t encoded_size;
    int rc;
    tga_encode_params_ex params;
    tga_footer footer;
    tga_forensic_report report;
    size_t ext_off;

    pixels[0] = 5u;
    pixels[1] = 6u;
    pixels[2] = 7u;
    pixels[3] = 8u;

    memset(&params, 0, sizeof(params));
    params.image.pixels = pixels;
    params.image.width = 1u;
    params.image.height = 1u;
    params.image.stride_bytes = 4u;
    params.image.pixel_format = TGA_PIXFMT_RGBA32;
    params.image.file_pixel_depth = 32u;
    params.image.write_footer = 1;
    params.write_scan_line_table = 1;

    rc = tga_encode_ex_memory(encoded, sizeof(encoded), &encoded_size, &params);
    if (rc != TGA_OK) {
        fprintf(stderr, "encode_ex v29 forensics failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_read_footer_memory(encoded, encoded_size, &footer);
    if (rc != TGA_OK || !footer.present || footer.extension_offset == 0ul) {
        fprintf(stderr, "footer read v29 failed: %s\n", tga_result_string(rc));
        return 1;
    }

    ext_off = (size_t)footer.extension_offset;
    encoded[ext_off + 494u] = 0u;
    write_u16_le(encoded + ext_off + 367u, 13u);
    write_u16_le(encoded + ext_off + 420u, 0u);
    write_u16_le(encoded + ext_off + 422u, 61u);
    write_u16_le(encoded + ext_off + 424u, 0u);
    write_u16_le(encoded + ext_off + 474u, 0u);
    write_u16_le(encoded + ext_off + 476u, 1u);
    write_u16_le(encoded + ext_off + 478u, 1u);
    write_u16_le(encoded + ext_off + 480u, 0u);

    memset(&report, 0, sizeof(report));
    rc = tga_forensics_memory(encoded, encoded_size, &report);
    if (rc != TGA_OK) {
        fprintf(stderr, "forensics v29 failed: %s\n", tga_result_string(rc));
        return 1;
    }

    if (report.attribute_type_mismatch_count == 0u ||
        report.bad_timestamp_count == 0u ||
        report.bad_job_time_count == 0u ||
        report.bad_pixel_aspect_count == 0u ||
        report.bad_gamma_count == 0u) {
        fprintf(stderr, "v29 counters missing: mismatch=%u ts=%u job=%u aspect=%u gamma=%u\n",
                report.attribute_type_mismatch_count,
                report.bad_timestamp_count,
                report.bad_job_time_count,
                report.bad_pixel_aspect_count,
                report.bad_gamma_count);
        return 1;
    }

    if ((report.flags & TGA_FORENSIC_ATTRIBUTE_TYPE_MISMATCH) == 0u ||
        (report.flags & TGA_FORENSIC_BAD_TIMESTAMP) == 0u ||
        (report.flags & TGA_FORENSIC_BAD_JOB_TIME) == 0u ||
        (report.flags & TGA_FORENSIC_BAD_PIXEL_ASPECT) == 0u ||
        (report.flags & TGA_FORENSIC_BAD_GAMMA) == 0u) {
        fprintf(stderr, "v29 flags missing: 0x%08x\n", report.flags);
        return 1;
    }

    encoded[ext_off + 494u] = 9u;
    memset(&report, 0, sizeof(report));
    rc = tga_forensics_memory(encoded, encoded_size, &report);
    if (rc != TGA_OK) {
        fprintf(stderr, "forensics v29 bad attr failed: %s\n", tga_result_string(rc));
        return 1;
    }
    if (report.bad_attributes_type_count == 0u ||
        (report.flags & TGA_FORENSIC_BAD_ATTRIBUTES_TYPE) == 0u) {
        fprintf(stderr, "bad attr type not detected\n");
        return 1;
    }

    return 0;
}

static int test_truecolor16_helpers_v30(void)
{
    unsigned char rgb565_pixels[4];
    unsigned char argb1555_pixels[4];
    unsigned char encoded[1024];
    unsigned char decoded[4];
    size_t encoded_size;
    int rc;
    tga_encode_params params;
    tga_decode_params dec;
    tga_info info;
    tga_u16 pixel16;
    tga_u8 r;
    tga_u8 g;
    tga_u8 b;
    tga_u8 a;

    pixel16 = tga_pack_rgb565(255u, 0u, 0u);
    if (pixel16 != 0xF800u) {
        fprintf(stderr, "rgb565 pack mismatch: 0x%04x\n", (unsigned)pixel16);
        return 1;
    }
    tga_unpack_rgb565((tga_u16)0x07E0u, &r, &g, &b);
    if (r != 0u || g != 255u || b != 0u) {
        fprintf(stderr, "rgb565 unpack mismatch: %u %u %u\n",
                (unsigned)r, (unsigned)g, (unsigned)b);
        return 1;
    }
    pixel16 = tga_pack_argb1555(255u, 255u, 0u, 0u);
    if (pixel16 != 0xFC00u) {
        fprintf(stderr, "argb1555 pack mismatch: 0x%04x\n", (unsigned)pixel16);
        return 1;
    }
    tga_unpack_argb1555((tga_u16)0xFC00u, &a, &r, &g, &b);
    if (a != 255u || r != 255u || g != 0u || b != 0u) {
        fprintf(stderr, "argb1555 unpack mismatch: %u %u %u %u\n",
                (unsigned)a, (unsigned)r, (unsigned)g, (unsigned)b);
        return 1;
    }

    write_u16_le(rgb565_pixels + 0u, 0xF800u);
    write_u16_le(rgb565_pixels + 2u, 0x07E0u);
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
        fprintf(stderr, "rgb565 encode failed: %s\n", tga_result_string(rc));
        return 1;
    }

    memset(decoded, 0, sizeof(decoded));
    memset(&dec, 0, sizeof(dec));
    dec.pixels = decoded;
    dec.stride_bytes = 4u;
    dec.pixel_format = TGA_PIXFMT_RGB565;
    rc = tga_decode_memory(encoded, encoded_size, &dec, &info);
    if (rc != TGA_OK) {
        fprintf(stderr, "rgb565 decode failed: %s\n", tga_result_string(rc));
        return 1;
    }
    if (info.pixel_depth != 16u || memcmp(decoded, rgb565_pixels, sizeof(decoded)) != 0) {
        fprintf(stderr, "rgb565 roundtrip mismatch\n");
        return 1;
    }

    write_u16_le(argb1555_pixels + 0u, 0xFC00u);
    write_u16_le(argb1555_pixels + 2u, 0x03E0u);
    memset(&params, 0, sizeof(params));
    params.pixels = argb1555_pixels;
    params.width = 2u;
    params.height = 1u;
    params.stride_bytes = 4u;
    params.pixel_format = TGA_PIXFMT_ARGB1555;
    params.file_pixel_depth = 16u;
    params.write_footer = 1;
    rc = tga_encode_memory(encoded, sizeof(encoded), &encoded_size, &params);
    if (rc != TGA_OK) {
        fprintf(stderr, "argb1555 encode failed: %s\n", tga_result_string(rc));
        return 1;
    }

    memset(decoded, 0, sizeof(decoded));
    memset(&dec, 0, sizeof(dec));
    dec.pixels = decoded;
    dec.stride_bytes = 4u;
    dec.pixel_format = TGA_PIXFMT_ARGB1555;
    rc = tga_decode_memory(encoded, encoded_size, &dec, &info);
    if (rc != TGA_OK) {
        fprintf(stderr, "argb1555 decode failed: %s\n", tga_result_string(rc));
        return 1;
    }
    if (info.pixel_depth != 16u || memcmp(decoded, argb1555_pixels, sizeof(decoded)) != 0) {
        fprintf(stderr, "argb1555 roundtrip mismatch\n");
        return 1;
    }

    return 0;
}

static int test_canonicalize_v30(void)
{
    unsigned char pixels[8];
    unsigned char raw_file[1024];
    unsigned char repaired[2048];
    unsigned char canonical[4096];
    unsigned char decoded_raw[8];
    unsigned char decoded_repaired[8];
    unsigned char util_u32[4];
    unsigned char dev_value[4];
    size_t raw_size;
    size_t repaired_size;
    size_t canonical_size;
    size_t size_query;
    size_t field_size;
    int rc;
    tga_encode_params params;
    tga_encode_params_ex ex_params;
    tga_decode_params dec;
    tga_extension ext;
    tga_footer footer;
    tga_info info;
    tga_developer_field field;
    tga_developer_tag tags[1];
    unsigned tag_count;
    unsigned scan_count;
#ifndef GAFNYF_TGA_NO_STDIO
    FILE *src_fp;
    FILE *dst_fp;
#endif

    pixels[0] = 1u;
    pixels[1] = 2u;
    pixels[2] = 3u;
    pixels[3] = 4u;
    pixels[4] = 5u;
    pixels[5] = 6u;
    pixels[6] = 7u;
    pixels[7] = 255u;

    memset(&params, 0, sizeof(params));
    params.pixels = pixels;
    params.width = 2u;
    params.height = 1u;
    params.stride_bytes = 8u;
    params.pixel_format = TGA_PIXFMT_RGBA32;
    params.file_pixel_depth = 32u;
    params.write_footer = 0;
    rc = tga_encode_memory(raw_file, sizeof(raw_file), &raw_size, &params);
    if (rc != TGA_OK) {
        fprintf(stderr, "raw encode v30 failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_read_footer_memory(raw_file, raw_size, &footer);
    if (rc != TGA_ERR_NO_FOOTER) {
        fprintf(stderr, "expected no footer before repair, got %s\n", tga_result_string(rc));
        return 1;
    }

    rc = tga_repair_tga2_size_memory(raw_file, raw_size, &size_query);
    if (rc != TGA_OK) {
        fprintf(stderr, "repair size failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_repair_tga2_memory(repaired, sizeof(repaired), &repaired_size,
                                raw_file, raw_size);
    if (rc != TGA_OK) {
        fprintf(stderr, "repair memory failed: %s\n", tga_result_string(rc));
        return 1;
    }
    if (repaired_size != size_query) {
        fprintf(stderr, "repair size mismatch: %lu %lu\n",
                (unsigned long)repaired_size,
                (unsigned long)size_query);
        return 1;
    }
    rc = tga_read_footer_memory(repaired, repaired_size, &footer);
    if (rc != TGA_OK || !footer.present || footer.extension_offset == 0ul) {
        fprintf(stderr, "repaired footer missing: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_read_extension_memory(repaired, repaired_size, &ext);
    if (rc != TGA_OK) {
        fprintf(stderr, "repaired extension missing: %s\n", tga_result_string(rc));
        return 1;
    }
    if (ext.size != 495u || ext.attributes_type != TGA_ATTRIBUTES_TYPE_ALPHA) {
        fprintf(stderr, "repaired extension mismatch: size=%u attr=%u\n",
                ext.size, ext.attributes_type);
        return 1;
    }

    memset(&dec, 0, sizeof(dec));
    dec.pixels = decoded_raw;
    dec.stride_bytes = 8u;
    dec.pixel_format = TGA_PIXFMT_RGBA32;
    rc = tga_decode_memory(raw_file, raw_size, &dec, &info);
    if (rc != TGA_OK) {
        fprintf(stderr, "decode raw after repair failed: %s\n", tga_result_string(rc));
        return 1;
    }
    dec.pixels = decoded_repaired;
    rc = tga_decode_memory(repaired, repaired_size, &dec, &info);
    if (rc != TGA_OK) {
        fprintf(stderr, "decode repaired failed: %s\n", tga_result_string(rc));
        return 1;
    }
    if (memcmp(decoded_raw, decoded_repaired, sizeof(decoded_raw)) != 0) {
        fprintf(stderr, "repair changed pixel payload\n");
        return 1;
    }

    write_u32_le(util_u32, 0x12345678ul);
    memset(&field, 0, sizeof(field));
    field.tag = TGA_UTIL_TAG_U32;
    field.data = util_u32;
    field.data_size = 4u;
    memset(&ex_params, 0, sizeof(ex_params));
    ex_params.image.pixels = pixels;
    ex_params.image.width = 1u;
    ex_params.image.height = 1u;
    ex_params.image.stride_bytes = 4u;
    ex_params.image.pixel_format = TGA_PIXFMT_RGBA32;
    ex_params.image.file_pixel_depth = 32u;
    ex_params.image.write_footer = 1;
    ex_params.write_scan_line_table = 1;
    ex_params.developer_fields = &field;
    ex_params.developer_field_count = 1u;
    rc = tga_encode_ex_memory(raw_file, sizeof(raw_file), &raw_size, &ex_params);
    if (rc != TGA_OK) {
        fprintf(stderr, "encode_ex for canonicalize failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_read_footer_memory(raw_file, raw_size, &footer);
    if (rc != TGA_OK || !footer.present || footer.extension_offset == 0ul) {
        fprintf(stderr, "canonicalize footer read failed: %s\n", tga_result_string(rc));
        return 1;
    }
    write_u16_le(raw_file + (size_t)footer.extension_offset, 496u);

    rc = tga_canonicalize_memory(canonical, sizeof(canonical), &canonical_size,
                                 raw_file, raw_size, 0);
    if (rc != TGA_OK) {
        fprintf(stderr, "canonicalize memory failed: %s\n", tga_result_string(rc));
        return 1;
    }
    rc = tga_read_extension_memory(canonical, canonical_size, &ext);
    if (rc != TGA_OK || ext.size != 495u) {
        fprintf(stderr, "canonicalized extension size mismatch: %s size=%u\n",
                tga_result_string(rc), ext.size);
        return 1;
    }
    rc = tga_read_scan_line_table_memory(canonical, canonical_size, 0, 0u, &scan_count);
    if (rc != TGA_OK || scan_count != 1u) {
        fprintf(stderr, "canonicalized scanline table missing: %s count=%u\n",
                tga_result_string(rc), scan_count);
        return 1;
    }
    rc = tga_read_developer_directory_memory(canonical, canonical_size,
                                             tags, 1u, &tag_count);
    if (rc != TGA_OK || tag_count != 1u || tags[0].tag != TGA_UTIL_TAG_U32) {
        fprintf(stderr, "canonicalized developer dir mismatch: %s count=%u tag=%u\n",
                tga_result_string(rc), tag_count, tag_count != 0u ? tags[0].tag : 0u);
        return 1;
    }
    rc = tga_read_developer_tag_memory(canonical, canonical_size,
                                       TGA_UTIL_TAG_U32,
                                       dev_value,
                                       sizeof(dev_value),
                                       &field_size);
    if (rc != TGA_OK || field_size != 4u || memcmp(dev_value, util_u32, 4u) != 0) {
        fprintf(stderr, "canonicalized developer payload mismatch: %s\n",
                tga_result_string(rc));
        return 1;
    }

#ifndef GAFNYF_TGA_NO_STDIO
    src_fp = tmpfile();
    dst_fp = tmpfile();
    if (src_fp == 0 || dst_fp == 0) {
        fprintf(stderr, "tmpfile failed for canonicalize_file\n");
        if (src_fp != 0) fclose(src_fp);
        if (dst_fp != 0) fclose(dst_fp);
        return 1;
    }
    if (fwrite(raw_file, 1u, raw_size, src_fp) != raw_size) {
        fprintf(stderr, "tmpfile src write failed\n");
        fclose(src_fp);
        fclose(dst_fp);
        return 1;
    }
    rewind(src_fp);
    rc = tga_canonicalize_file(dst_fp, src_fp, 0);
    if (rc != TGA_OK) {
        fprintf(stderr, "canonicalize_file failed: %s\n", tga_result_string(rc));
        fclose(src_fp);
        fclose(dst_fp);
        return 1;
    }
    rewind(dst_fp);
    rc = tga_read_extension_file(dst_fp, &ext);
    if (rc != TGA_OK || ext.size != 495u) {
        fprintf(stderr, "canonicalize_file extension mismatch: %s size=%u\n",
                tga_result_string(rc), ext.size);
        fclose(src_fp);
        fclose(dst_fp);
        return 1;
    }
    fclose(src_fp);
    fclose(dst_fp);
#endif

    return 0;
}

int main(void)
{
    if (test_indexed_palette16() != 0) {
        return 1;
    }
    if (test_forensics_v28() != 0) {
        return 1;
    }
    if (test_extension_helpers_v29() != 0) {
        return 1;
    }
    if (test_forensics_v29_semantics() != 0) {
        return 1;
    }
    if (test_truecolor16_helpers_v30() != 0) {
        return 1;
    }
    if (test_canonicalize_v30() != 0) {
        return 1;
    }

    puts("OK");
    return 0;
}
