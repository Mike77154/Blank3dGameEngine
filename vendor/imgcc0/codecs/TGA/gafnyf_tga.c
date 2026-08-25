#include "gafnyf_tga.h"

#include <limits.h>
#include <string.h>

#if CHAR_BIT != 8
#error "gafnyf_tga requires 8-bit bytes"

int tga_inspect_file(FILE *fp, tga_inspect *out_inspect)
{
    tga_info info;
    tga_footer footer;
    tga_u8 buf[10];
    size_t color_map_size;
    size_t image_data_offset;
    size_t image_data_size;
    size_t ext_size;
    size_t dir_size;
    size_t tag_bytes;
    size_t min_payload;
    size_t max_payload;
    size_t field_offset;
    size_t field_size;
    long saved_pos;
    long end_pos;
    unsigned count;
    unsigned i;
    int rc;

    if (fp == 0 || out_inspect == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_inspect, 0, sizeof(*out_inspect));

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    out_inspect->file_size = (size_t)end_pos;
    (void)fseek(fp, saved_pos, SEEK_SET);

    rc = tga_probe_file(fp, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    out_inspect->info = info;

    out_inspect->image_id_offset = 18u;
    out_inspect->image_id_size = (size_t)info.id_length;
    if (!tga__region_fits(out_inspect->image_id_offset,
                          out_inspect->image_id_size,
                          out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__color_map_size_from_info(&info, &color_map_size)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->color_map_offset = out_inspect->image_id_offset + out_inspect->image_id_size;
    out_inspect->color_map_size = color_map_size;
    if (!tga__region_fits(out_inspect->color_map_offset,
                          out_inspect->color_map_size,
                          out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__image_data_offset_from_info(&info, &image_data_offset)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->image_data_offset = image_data_offset;
    if (!tga__region_fits(image_data_offset, 0u, out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__validate_decode_info(&info);
    if (rc == TGA_OK) {
        rc = tga__measure_image_data_size_file(fp, &info,
                                               image_data_offset,
                                               &image_data_size);
        if (rc != TGA_OK) {
            return rc;
        }
        out_inspect->image_data_size = image_data_size;
    }

    rc = tga_read_footer_file(fp, &footer);
    if (rc == TGA_OK) {
        out_inspect->footer = footer;
        out_inspect->is_tga2 = 1;
        out_inspect->footer_offset = out_inspect->file_size - 26u;
        out_inspect->footer_size = 26u;
    } else if (rc != TGA_ERR_NO_FOOTER) {
        return rc;
    }

    if (out_inspect->is_tga2 && footer.extension_offset != 0ul) {
        rc = tga__read_section_file_impl(fp,
                                         (size_t)footer.extension_offset,
                                         2u,
                                         buf,
                                         2u,
                                         0);
        if (rc != TGA_OK) {
            return rc;
        }
        ext_size = (size_t)tga__read_u16_le(buf);
        if (ext_size < 495u) {
            return TGA_ERR_BAD_FORMAT;
        }
        if (!tga__region_fits((size_t)footer.extension_offset,
                              ext_size,
                              out_inspect->file_size)) {
            return TGA_ERR_TRUNCATED;
        }

        out_inspect->has_extension = 1;
        out_inspect->extension_area_offset = (size_t)footer.extension_offset;
        out_inspect->extension_area_size = ext_size;

        rc = tga_read_extension_file(fp, &out_inspect->extension);
        if (rc != TGA_OK) {
            return rc;
        }

        if (out_inspect->extension.scan_line_offset != 0ul) {
            if (!tga__checked_mul_size((size_t)info.height, 4u,
                                       &out_inspect->scan_line_table_size)) {
                return TGA_ERR_OVERFLOW;
            }
            out_inspect->scan_line_table_offset =
                (size_t)out_inspect->extension.scan_line_offset;
            if (!tga__region_fits(out_inspect->scan_line_table_offset,
                                  out_inspect->scan_line_table_size,
                                  out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            out_inspect->has_scan_line_table = 1;
        }

        if (out_inspect->extension.postage_stamp_offset != 0ul) {
            rc = tga_read_postage_stamp_info_file(fp,
                                                  &out_inspect->postage_stamp_info);
            if (rc != TGA_OK) {
                return rc;
            }
            out_inspect->postage_stamp_offset =
                (size_t)out_inspect->extension.postage_stamp_offset;
            out_inspect->postage_stamp_size =
                out_inspect->postage_stamp_info.block_size;
            out_inspect->has_postage_stamp = 1;
        }

        if (out_inspect->extension.color_correction_offset != 0ul) {
            out_inspect->color_correction_offset =
                (size_t)out_inspect->extension.color_correction_offset;
            out_inspect->color_correction_size =
                (size_t)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8u;
            if (!tga__region_fits(out_inspect->color_correction_offset,
                                  out_inspect->color_correction_size,
                                  out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            out_inspect->has_color_correction_table = 1;
        }
    }

    if (out_inspect->is_tga2 && footer.developer_offset != 0ul) {
        rc = tga__read_section_file_impl(fp,
                                         (size_t)footer.developer_offset,
                                         2u,
                                         buf,
                                         2u,
                                         0);
        if (rc != TGA_OK) {
            return rc;
        }
        count = (unsigned)tga__read_u16_le(buf);
        if (!tga__checked_mul_size((size_t)count, 10u, &tag_bytes)) {
            return TGA_ERR_OVERFLOW;
        }
        dir_size = 2u + tag_bytes;
        if (!tga__region_fits((size_t)footer.developer_offset,
                              dir_size,
                              out_inspect->file_size)) {
            return TGA_ERR_TRUNCATED;
        }

        out_inspect->has_developer_directory = 1;
        out_inspect->developer_directory_offset = (size_t)footer.developer_offset;
        out_inspect->developer_directory_size = dir_size;
        out_inspect->developer_tag_count = count;

        min_payload = (size_t)-1;
        max_payload = 0u;
        for (i = 0u; i < count; ++i) {
            rc = tga__read_section_file_impl(fp,
                                             (size_t)footer.developer_offset + 2u + (size_t)i * 10u,
                                             10u,
                                             buf,
                                             10u,
                                             0);
            if (rc != TGA_OK) {
                return rc;
            }
            field_offset = (size_t)tga__read_u32_le(buf + 2u);
            field_size = (size_t)tga__read_u32_le(buf + 6u);
            if (!tga__region_fits(field_offset, field_size, out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            if (field_size == 0u) {
                continue;
            }
            if (min_payload == (size_t)-1 || field_offset < min_payload) {
                min_payload = field_offset;
            }
            if (field_offset + field_size > max_payload) {
                max_payload = field_offset + field_size;
            }
        }

        if (min_payload != (size_t)-1) {
            out_inspect->has_developer_payload = 1;
            out_inspect->developer_payload_offset = min_payload;
            out_inspect->developer_payload_size = max_payload - min_payload;
        }
    }

    return TGA_OK;
}

int tga_read_image_id_file(FILE *fp,
                           void *dst,
                           size_t dst_capacity,
                           size_t *out_id_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.image_id_offset,
                                       inspect.image_id_size,
                                       dst,
                                       dst_capacity,
                                       out_id_size);
}

int tga_read_color_map_file(FILE *fp,
                            void *dst,
                            size_t dst_capacity,
                            size_t *out_color_map_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.color_map_offset,
                                       inspect.color_map_size,
                                       dst,
                                       dst_capacity,
                                       out_color_map_size);
}

int tga_read_image_data_file(FILE *fp,
                             void *dst,
                             size_t dst_capacity,
                             size_t *out_image_data_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.image_data_offset,
                                       inspect.image_data_size,
                                       dst,
                                       dst_capacity,
                                       out_image_data_size);
}
#endif

#if USHRT_MAX < 65535u
#error "gafnyf_tga requires unsigned short with at least 16 bits"

int tga_inspect_file(FILE *fp, tga_inspect *out_inspect)
{
    tga_info info;
    tga_footer footer;
    tga_u8 buf[10];
    size_t color_map_size;
    size_t image_data_offset;
    size_t image_data_size;
    size_t ext_size;
    size_t dir_size;
    size_t tag_bytes;
    size_t min_payload;
    size_t max_payload;
    size_t field_offset;
    size_t field_size;
    long saved_pos;
    long end_pos;
    unsigned count;
    unsigned i;
    int rc;

    if (fp == 0 || out_inspect == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_inspect, 0, sizeof(*out_inspect));

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    out_inspect->file_size = (size_t)end_pos;
    (void)fseek(fp, saved_pos, SEEK_SET);

    rc = tga_probe_file(fp, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    out_inspect->info = info;

    out_inspect->image_id_offset = 18u;
    out_inspect->image_id_size = (size_t)info.id_length;
    if (!tga__region_fits(out_inspect->image_id_offset,
                          out_inspect->image_id_size,
                          out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__color_map_size_from_info(&info, &color_map_size)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->color_map_offset = out_inspect->image_id_offset + out_inspect->image_id_size;
    out_inspect->color_map_size = color_map_size;
    if (!tga__region_fits(out_inspect->color_map_offset,
                          out_inspect->color_map_size,
                          out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__image_data_offset_from_info(&info, &image_data_offset)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->image_data_offset = image_data_offset;
    if (!tga__region_fits(image_data_offset, 0u, out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__validate_decode_info(&info);
    if (rc == TGA_OK) {
        rc = tga__measure_image_data_size_file(fp, &info,
                                               image_data_offset,
                                               &image_data_size);
        if (rc != TGA_OK) {
            return rc;
        }
        out_inspect->image_data_size = image_data_size;
    }

    rc = tga_read_footer_file(fp, &footer);
    if (rc == TGA_OK) {
        out_inspect->footer = footer;
        out_inspect->is_tga2 = 1;
        out_inspect->footer_offset = out_inspect->file_size - 26u;
        out_inspect->footer_size = 26u;
    } else if (rc != TGA_ERR_NO_FOOTER) {
        return rc;
    }

    if (out_inspect->is_tga2 && footer.extension_offset != 0ul) {
        rc = tga__read_section_file_impl(fp,
                                         (size_t)footer.extension_offset,
                                         2u,
                                         buf,
                                         2u,
                                         0);
        if (rc != TGA_OK) {
            return rc;
        }
        ext_size = (size_t)tga__read_u16_le(buf);
        if (ext_size < 495u) {
            return TGA_ERR_BAD_FORMAT;
        }
        if (!tga__region_fits((size_t)footer.extension_offset,
                              ext_size,
                              out_inspect->file_size)) {
            return TGA_ERR_TRUNCATED;
        }

        out_inspect->has_extension = 1;
        out_inspect->extension_area_offset = (size_t)footer.extension_offset;
        out_inspect->extension_area_size = ext_size;

        rc = tga_read_extension_file(fp, &out_inspect->extension);
        if (rc != TGA_OK) {
            return rc;
        }

        if (out_inspect->extension.scan_line_offset != 0ul) {
            if (!tga__checked_mul_size((size_t)info.height, 4u,
                                       &out_inspect->scan_line_table_size)) {
                return TGA_ERR_OVERFLOW;
            }
            out_inspect->scan_line_table_offset =
                (size_t)out_inspect->extension.scan_line_offset;
            if (!tga__region_fits(out_inspect->scan_line_table_offset,
                                  out_inspect->scan_line_table_size,
                                  out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            out_inspect->has_scan_line_table = 1;
        }

        if (out_inspect->extension.postage_stamp_offset != 0ul) {
            rc = tga_read_postage_stamp_info_file(fp,
                                                  &out_inspect->postage_stamp_info);
            if (rc != TGA_OK) {
                return rc;
            }
            out_inspect->postage_stamp_offset =
                (size_t)out_inspect->extension.postage_stamp_offset;
            out_inspect->postage_stamp_size =
                out_inspect->postage_stamp_info.block_size;
            out_inspect->has_postage_stamp = 1;
        }

        if (out_inspect->extension.color_correction_offset != 0ul) {
            out_inspect->color_correction_offset =
                (size_t)out_inspect->extension.color_correction_offset;
            out_inspect->color_correction_size =
                (size_t)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8u;
            if (!tga__region_fits(out_inspect->color_correction_offset,
                                  out_inspect->color_correction_size,
                                  out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            out_inspect->has_color_correction_table = 1;
        }
    }

    if (out_inspect->is_tga2 && footer.developer_offset != 0ul) {
        rc = tga__read_section_file_impl(fp,
                                         (size_t)footer.developer_offset,
                                         2u,
                                         buf,
                                         2u,
                                         0);
        if (rc != TGA_OK) {
            return rc;
        }
        count = (unsigned)tga__read_u16_le(buf);
        if (!tga__checked_mul_size((size_t)count, 10u, &tag_bytes)) {
            return TGA_ERR_OVERFLOW;
        }
        dir_size = 2u + tag_bytes;
        if (!tga__region_fits((size_t)footer.developer_offset,
                              dir_size,
                              out_inspect->file_size)) {
            return TGA_ERR_TRUNCATED;
        }

        out_inspect->has_developer_directory = 1;
        out_inspect->developer_directory_offset = (size_t)footer.developer_offset;
        out_inspect->developer_directory_size = dir_size;
        out_inspect->developer_tag_count = count;

        min_payload = (size_t)-1;
        max_payload = 0u;
        for (i = 0u; i < count; ++i) {
            rc = tga__read_section_file_impl(fp,
                                             (size_t)footer.developer_offset + 2u + (size_t)i * 10u,
                                             10u,
                                             buf,
                                             10u,
                                             0);
            if (rc != TGA_OK) {
                return rc;
            }
            field_offset = (size_t)tga__read_u32_le(buf + 2u);
            field_size = (size_t)tga__read_u32_le(buf + 6u);
            if (!tga__region_fits(field_offset, field_size, out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            if (field_size == 0u) {
                continue;
            }
            if (min_payload == (size_t)-1 || field_offset < min_payload) {
                min_payload = field_offset;
            }
            if (field_offset + field_size > max_payload) {
                max_payload = field_offset + field_size;
            }
        }

        if (min_payload != (size_t)-1) {
            out_inspect->has_developer_payload = 1;
            out_inspect->developer_payload_offset = min_payload;
            out_inspect->developer_payload_size = max_payload - min_payload;
        }
    }

    return TGA_OK;
}

int tga_read_image_id_file(FILE *fp,
                           void *dst,
                           size_t dst_capacity,
                           size_t *out_id_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.image_id_offset,
                                       inspect.image_id_size,
                                       dst,
                                       dst_capacity,
                                       out_id_size);
}

int tga_read_color_map_file(FILE *fp,
                            void *dst,
                            size_t dst_capacity,
                            size_t *out_color_map_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.color_map_offset,
                                       inspect.color_map_size,
                                       dst,
                                       dst_capacity,
                                       out_color_map_size);
}

int tga_read_image_data_file(FILE *fp,
                             void *dst,
                             size_t dst_capacity,
                             size_t *out_image_data_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.image_data_offset,
                                       inspect.image_data_size,
                                       dst,
                                       dst_capacity,
                                       out_image_data_size);
}
#endif

#if ULONG_MAX < 4294967295ul
#error "gafnyf_tga requires unsigned long with at least 32 bits"

int tga_inspect_file(FILE *fp, tga_inspect *out_inspect)
{
    tga_info info;
    tga_footer footer;
    tga_u8 buf[10];
    size_t color_map_size;
    size_t image_data_offset;
    size_t image_data_size;
    size_t ext_size;
    size_t dir_size;
    size_t tag_bytes;
    size_t min_payload;
    size_t max_payload;
    size_t field_offset;
    size_t field_size;
    long saved_pos;
    long end_pos;
    unsigned count;
    unsigned i;
    int rc;

    if (fp == 0 || out_inspect == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_inspect, 0, sizeof(*out_inspect));

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    out_inspect->file_size = (size_t)end_pos;
    (void)fseek(fp, saved_pos, SEEK_SET);

    rc = tga_probe_file(fp, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    out_inspect->info = info;

    out_inspect->image_id_offset = 18u;
    out_inspect->image_id_size = (size_t)info.id_length;
    if (!tga__region_fits(out_inspect->image_id_offset,
                          out_inspect->image_id_size,
                          out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__color_map_size_from_info(&info, &color_map_size)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->color_map_offset = out_inspect->image_id_offset + out_inspect->image_id_size;
    out_inspect->color_map_size = color_map_size;
    if (!tga__region_fits(out_inspect->color_map_offset,
                          out_inspect->color_map_size,
                          out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__image_data_offset_from_info(&info, &image_data_offset)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->image_data_offset = image_data_offset;
    if (!tga__region_fits(image_data_offset, 0u, out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__validate_decode_info(&info);
    if (rc == TGA_OK) {
        rc = tga__measure_image_data_size_file(fp, &info,
                                               image_data_offset,
                                               &image_data_size);
        if (rc != TGA_OK) {
            return rc;
        }
        out_inspect->image_data_size = image_data_size;
    }

    rc = tga_read_footer_file(fp, &footer);
    if (rc == TGA_OK) {
        out_inspect->footer = footer;
        out_inspect->is_tga2 = 1;
        out_inspect->footer_offset = out_inspect->file_size - 26u;
        out_inspect->footer_size = 26u;
    } else if (rc != TGA_ERR_NO_FOOTER) {
        return rc;
    }

    if (out_inspect->is_tga2 && footer.extension_offset != 0ul) {
        rc = tga__read_section_file_impl(fp,
                                         (size_t)footer.extension_offset,
                                         2u,
                                         buf,
                                         2u,
                                         0);
        if (rc != TGA_OK) {
            return rc;
        }
        ext_size = (size_t)tga__read_u16_le(buf);
        if (ext_size < 495u) {
            return TGA_ERR_BAD_FORMAT;
        }
        if (!tga__region_fits((size_t)footer.extension_offset,
                              ext_size,
                              out_inspect->file_size)) {
            return TGA_ERR_TRUNCATED;
        }

        out_inspect->has_extension = 1;
        out_inspect->extension_area_offset = (size_t)footer.extension_offset;
        out_inspect->extension_area_size = ext_size;

        rc = tga_read_extension_file(fp, &out_inspect->extension);
        if (rc != TGA_OK) {
            return rc;
        }

        if (out_inspect->extension.scan_line_offset != 0ul) {
            if (!tga__checked_mul_size((size_t)info.height, 4u,
                                       &out_inspect->scan_line_table_size)) {
                return TGA_ERR_OVERFLOW;
            }
            out_inspect->scan_line_table_offset =
                (size_t)out_inspect->extension.scan_line_offset;
            if (!tga__region_fits(out_inspect->scan_line_table_offset,
                                  out_inspect->scan_line_table_size,
                                  out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            out_inspect->has_scan_line_table = 1;
        }

        if (out_inspect->extension.postage_stamp_offset != 0ul) {
            rc = tga_read_postage_stamp_info_file(fp,
                                                  &out_inspect->postage_stamp_info);
            if (rc != TGA_OK) {
                return rc;
            }
            out_inspect->postage_stamp_offset =
                (size_t)out_inspect->extension.postage_stamp_offset;
            out_inspect->postage_stamp_size =
                out_inspect->postage_stamp_info.block_size;
            out_inspect->has_postage_stamp = 1;
        }

        if (out_inspect->extension.color_correction_offset != 0ul) {
            out_inspect->color_correction_offset =
                (size_t)out_inspect->extension.color_correction_offset;
            out_inspect->color_correction_size =
                (size_t)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8u;
            if (!tga__region_fits(out_inspect->color_correction_offset,
                                  out_inspect->color_correction_size,
                                  out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            out_inspect->has_color_correction_table = 1;
        }
    }

    if (out_inspect->is_tga2 && footer.developer_offset != 0ul) {
        rc = tga__read_section_file_impl(fp,
                                         (size_t)footer.developer_offset,
                                         2u,
                                         buf,
                                         2u,
                                         0);
        if (rc != TGA_OK) {
            return rc;
        }
        count = (unsigned)tga__read_u16_le(buf);
        if (!tga__checked_mul_size((size_t)count, 10u, &tag_bytes)) {
            return TGA_ERR_OVERFLOW;
        }
        dir_size = 2u + tag_bytes;
        if (!tga__region_fits((size_t)footer.developer_offset,
                              dir_size,
                              out_inspect->file_size)) {
            return TGA_ERR_TRUNCATED;
        }

        out_inspect->has_developer_directory = 1;
        out_inspect->developer_directory_offset = (size_t)footer.developer_offset;
        out_inspect->developer_directory_size = dir_size;
        out_inspect->developer_tag_count = count;

        min_payload = (size_t)-1;
        max_payload = 0u;
        for (i = 0u; i < count; ++i) {
            rc = tga__read_section_file_impl(fp,
                                             (size_t)footer.developer_offset + 2u + (size_t)i * 10u,
                                             10u,
                                             buf,
                                             10u,
                                             0);
            if (rc != TGA_OK) {
                return rc;
            }
            field_offset = (size_t)tga__read_u32_le(buf + 2u);
            field_size = (size_t)tga__read_u32_le(buf + 6u);
            if (!tga__region_fits(field_offset, field_size, out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            if (field_size == 0u) {
                continue;
            }
            if (min_payload == (size_t)-1 || field_offset < min_payload) {
                min_payload = field_offset;
            }
            if (field_offset + field_size > max_payload) {
                max_payload = field_offset + field_size;
            }
        }

        if (min_payload != (size_t)-1) {
            out_inspect->has_developer_payload = 1;
            out_inspect->developer_payload_offset = min_payload;
            out_inspect->developer_payload_size = max_payload - min_payload;
        }
    }

    return TGA_OK;
}

int tga_read_image_id_file(FILE *fp,
                           void *dst,
                           size_t dst_capacity,
                           size_t *out_id_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.image_id_offset,
                                       inspect.image_id_size,
                                       dst,
                                       dst_capacity,
                                       out_id_size);
}

int tga_read_color_map_file(FILE *fp,
                            void *dst,
                            size_t dst_capacity,
                            size_t *out_color_map_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.color_map_offset,
                                       inspect.color_map_size,
                                       dst,
                                       dst_capacity,
                                       out_color_map_size);
}

int tga_read_image_data_file(FILE *fp,
                             void *dst,
                             size_t dst_capacity,
                             size_t *out_image_data_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.image_data_offset,
                                       inspect.image_data_size,
                                       dst,
                                       dst_capacity,
                                       out_image_data_size);
}
#endif

typedef struct tga__rgba {
    tga_u8 r;
    tga_u8 g;
    tga_u8 b;
    tga_u8 a;
} tga__rgba;

static const tga_u8 tga__footer_sig[18] = {
    'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0'
};

static tga_u16 tga__read_u16_le(const tga_u8 *src);
static tga_u32 tga__read_u32_le(const tga_u8 *src);
static void tga__write_u16_le(tga_u8 *dst, unsigned value);
static void tga__write_u32_le(tga_u8 *dst, tga_u32 value);

static int tga__is_color_mapped_type(unsigned image_type)
{
    return image_type == 1u || image_type == 9u;
}

static int tga__is_truecolor_type(unsigned image_type)
{
    return image_type == 2u || image_type == 10u;
}

static int tga__is_gray_type(unsigned image_type)
{
    return image_type == 3u || image_type == 11u;
}

static int tga__is_rle_type(unsigned image_type)
{
    return image_type == 9u || image_type == 10u || image_type == 11u;
}

static unsigned tga__raw_image_type(unsigned image_type)
{
    if (image_type == 9u) {
        return 1u;
    }
    if (image_type == 10u) {
        return 2u;
    }
    if (image_type == 11u) {
        return 3u;
    }
    return image_type;
}

static int tga__parse_header(const tga_u8 hdr[18], tga_info *out_info)
{
    tga_info info;

    if (out_info == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    info.id_length = (unsigned)hdr[0];
    info.color_map_type = (unsigned)hdr[1];
    info.image_type = (unsigned)hdr[2];
    info.color_map_origin = (unsigned)hdr[3] | ((unsigned)hdr[4] << 8);
    info.color_map_length = (unsigned)hdr[5] | ((unsigned)hdr[6] << 8);
    info.color_map_entry_bits = (unsigned)hdr[7];
    info.x_origin = (unsigned)hdr[8] | ((unsigned)hdr[9] << 8);
    info.y_origin = (unsigned)hdr[10] | ((unsigned)hdr[11] << 8);
    info.width = (unsigned)hdr[12] | ((unsigned)hdr[13] << 8);
    info.height = (unsigned)hdr[14] | ((unsigned)hdr[15] << 8);
    info.pixel_depth = (unsigned)hdr[16];
    info.descriptor = (unsigned)hdr[17];
    info.is_rle = tga__is_rle_type(info.image_type);
    info.is_color_mapped = tga__is_color_mapped_type(info.image_type);
    info.is_grayscale = tga__is_gray_type(info.image_type);
    info.file_top_origin = (info.descriptor & 0x20u) != 0u;
    info.file_right_to_left = (info.descriptor & 0x10u) != 0u;
    info.has_footer = 0;

    *out_info = info;
    return TGA_OK;
}

static unsigned tga__file_sample_bytes(unsigned bits)
{
    if (bits <= 8u) {
        return 1u;
    }
    if (bits <= 16u) {
        return 2u;
    }
    if (bits <= 24u) {
        return 3u;
    }
    return 4u;
}

static int tga__read_exact(tga_reader *reader, void *dst, size_t bytes)
{
    if (bytes == 0u) {
        return 1;
    }
    if (reader == 0 || reader->read == 0) {
        return 0;
    }
    return reader->read(reader->user, dst, bytes) == bytes;
}

static int tga__write_exact(tga_writer *writer, const void *src, size_t bytes)
{
    if (bytes == 0u) {
        return 1;
    }
    if (writer == 0 || writer->write == 0) {
        return 0;
    }
    return writer->write(writer->user, src, bytes) == bytes;
}

static int tga__skip(tga_reader *reader, size_t bytes)
{
    tga_u8 tmp[64];
    size_t chunk;

    while (bytes > 0u) {
        chunk = bytes;
        if (chunk > sizeof(tmp)) {
            chunk = sizeof(tmp);
        }
        if (!tga__read_exact(reader, tmp, chunk)) {
            return 0;
        }
        bytes -= chunk;
    }
    return 1;
}

static tga_u8 tga__scale_5_to_8(unsigned v)
{
    return (tga_u8)((v * 255u + 15u) / 31u);
}

static tga_u8 tga__scale_6_to_8(unsigned v)
{
    return (tga_u8)((v * 255u + 31u) / 63u);
}

static unsigned tga__scale_8_to_5(unsigned v)
{
    return (unsigned)((v * 31u + 127u) / 255u);
}

static unsigned tga__scale_8_to_6(unsigned v)
{
    return (unsigned)((v * 63u + 127u) / 255u);
}

tga_u16 tga_pack_rgb565(tga_u8 r, tga_u8 g, tga_u8 b)
{
    unsigned r5;
    unsigned g6;
    unsigned b5;

    r5 = tga__scale_8_to_5((unsigned)r);
    g6 = tga__scale_8_to_6((unsigned)g);
    b5 = tga__scale_8_to_5((unsigned)b);
    return (tga_u16)((r5 << 11) | (g6 << 5) | b5);
}

tga_u16 tga_pack_argb1555(tga_u8 a, tga_u8 r, tga_u8 g, tga_u8 b)
{
    unsigned a1;
    unsigned r5;
    unsigned g5;
    unsigned b5;

    a1 = a >= 128u ? 1u : 0u;
    r5 = tga__scale_8_to_5((unsigned)r);
    g5 = tga__scale_8_to_5((unsigned)g);
    b5 = tga__scale_8_to_5((unsigned)b);
    return (tga_u16)((a1 << 15) | (r5 << 10) | (g5 << 5) | b5);
}

void tga_unpack_rgb565(tga_u16 pixel,
                       tga_u8 *out_r,
                       tga_u8 *out_g,
                       tga_u8 *out_b)
{
    if (out_r != 0) {
        *out_r = tga__scale_5_to_8(((unsigned)pixel >> 11) & 31u);
    }
    if (out_g != 0) {
        *out_g = tga__scale_6_to_8(((unsigned)pixel >> 5) & 63u);
    }
    if (out_b != 0) {
        *out_b = tga__scale_5_to_8((unsigned)pixel & 31u);
    }
}

void tga_unpack_argb1555(tga_u16 pixel,
                         tga_u8 *out_a,
                         tga_u8 *out_r,
                         tga_u8 *out_g,
                         tga_u8 *out_b)
{
    if (out_a != 0) {
        *out_a = (pixel & 0x8000u) != 0u ? 255u : 0u;
    }
    if (out_r != 0) {
        *out_r = tga__scale_5_to_8(((unsigned)pixel >> 10) & 31u);
    }
    if (out_g != 0) {
        *out_g = tga__scale_5_to_8(((unsigned)pixel >> 5) & 31u);
    }
    if (out_b != 0) {
        *out_b = tga__scale_5_to_8((unsigned)pixel & 31u);
    }
}

static tga_u8 tga__rgb_to_gray(tga_u8 r, tga_u8 g, tga_u8 b)
{
    return (tga_u8)(((unsigned)r * 77u + (unsigned)g * 150u + (unsigned)b * 29u + 128u) >> 8);
}

static int tga__memory_format_has_alpha(tga_pixel_format fmt)
{
    return fmt == TGA_PIXFMT_RGBA32 ||
           fmt == TGA_PIXFMT_BGRA32 ||
           fmt == TGA_PIXFMT_ARGB1555;
}

static int tga__pixel_format_valid(tga_pixel_format fmt)
{
    return fmt == TGA_PIXFMT_GRAY8 ||
           fmt == TGA_PIXFMT_RGB24 ||
           fmt == TGA_PIXFMT_BGR24 ||
           fmt == TGA_PIXFMT_RGBA32 ||
           fmt == TGA_PIXFMT_BGRA32 ||
           fmt == TGA_PIXFMT_INDEX8 ||
           fmt == TGA_PIXFMT_RGB565 ||
           fmt == TGA_PIXFMT_ARGB1555;
}

unsigned tga_bytes_per_pixel(tga_pixel_format fmt)
{
    switch (fmt) {
        case TGA_PIXFMT_GRAY8: return 1u;
        case TGA_PIXFMT_RGB24: return 3u;
        case TGA_PIXFMT_BGR24: return 3u;
        case TGA_PIXFMT_RGBA32: return 4u;
        case TGA_PIXFMT_BGRA32: return 4u;
        case TGA_PIXFMT_INDEX8: return 1u;
        case TGA_PIXFMT_RGB565: return 2u;
        case TGA_PIXFMT_ARGB1555: return 2u;
        default: return 0u;
    }
}

static void tga__decode_color_sample(const tga_u8 *src, unsigned bits,
                                     unsigned attr_bits, int grayscale,
                                     tga__rgba *out_rgba)
{
    unsigned w;
    tga__rgba rgba;

    if (grayscale) {
        if (bits == 16u) {
            rgba.r = src[0];
            rgba.g = src[0];
            rgba.b = src[0];
            rgba.a = src[1];
        } else {
            rgba.r = src[0];
            rgba.g = src[0];
            rgba.b = src[0];
            rgba.a = 255u;
        }
        *out_rgba = rgba;
        return;
    }

    if (bits == 15u || bits == 16u) {
        w = (unsigned)src[0] | ((unsigned)src[1] << 8);
        rgba.b = tga__scale_5_to_8((w >> 0) & 31u);
        rgba.g = tga__scale_5_to_8((w >> 5) & 31u);
        rgba.r = tga__scale_5_to_8((w >> 10) & 31u);
        if (bits == 16u && attr_bits != 0u) {
            rgba.a = (w & 0x8000u) != 0u ? 255u : 0u;
        } else {
            rgba.a = 255u;
        }
        *out_rgba = rgba;
        return;
    }

    if (bits == 24u) {
        rgba.b = src[0];
        rgba.g = src[1];
        rgba.r = src[2];
        rgba.a = 255u;
        *out_rgba = rgba;
        return;
    }

    rgba.b = src[0];
    rgba.g = src[1];
    rgba.r = src[2];
    rgba.a = src[3];
    *out_rgba = rgba;
}

static void tga__store_output_pixel(tga_u8 *dst_row, unsigned x,
                                    tga_pixel_format fmt,
                                    const tga__rgba *rgba)
{
    tga_u8 *dst;
    tga_u16 pixel16;

    switch (fmt) {
        case TGA_PIXFMT_GRAY8:
            dst_row[x] = tga__rgb_to_gray(rgba->r, rgba->g, rgba->b);
            return;

        case TGA_PIXFMT_RGB24:
            dst = dst_row + ((size_t)x * 3u);
            dst[0] = rgba->r;
            dst[1] = rgba->g;
            dst[2] = rgba->b;
            return;

        case TGA_PIXFMT_BGR24:
            dst = dst_row + ((size_t)x * 3u);
            dst[0] = rgba->b;
            dst[1] = rgba->g;
            dst[2] = rgba->r;
            return;

        case TGA_PIXFMT_RGBA32:
            dst = dst_row + ((size_t)x * 4u);
            dst[0] = rgba->r;
            dst[1] = rgba->g;
            dst[2] = rgba->b;
            dst[3] = rgba->a;
            return;

        case TGA_PIXFMT_BGRA32:
            dst = dst_row + ((size_t)x * 4u);
            dst[0] = rgba->b;
            dst[1] = rgba->g;
            dst[2] = rgba->r;
            dst[3] = rgba->a;
            return;

        case TGA_PIXFMT_RGB565:
            dst = dst_row + ((size_t)x * 2u);
            pixel16 = tga_pack_rgb565(rgba->r, rgba->g, rgba->b);
            dst[0] = (tga_u8)(pixel16 & 255u);
            dst[1] = (tga_u8)((pixel16 >> 8) & 255u);
            return;

        case TGA_PIXFMT_ARGB1555:
            dst = dst_row + ((size_t)x * 2u);
            pixel16 = tga_pack_argb1555(rgba->a, rgba->r, rgba->g, rgba->b);
            dst[0] = (tga_u8)(pixel16 & 255u);
            dst[1] = (tga_u8)((pixel16 >> 8) & 255u);
            return;

        default:
            return;
    }
}

static void tga__load_memory_pixel_rgba(const tga_u8 *row, unsigned x,
                                        tga_pixel_format fmt,
                                        tga__rgba *out_rgba)
{
    const tga_u8 *src;
    tga__rgba rgba;
    tga_u16 pixel16;

    switch (fmt) {
        case TGA_PIXFMT_GRAY8:
            rgba.r = row[x];
            rgba.g = row[x];
            rgba.b = row[x];
            rgba.a = 255u;
            *out_rgba = rgba;
            return;

        case TGA_PIXFMT_RGB24:
            src = row + ((size_t)x * 3u);
            rgba.r = src[0];
            rgba.g = src[1];
            rgba.b = src[2];
            rgba.a = 255u;
            *out_rgba = rgba;
            return;

        case TGA_PIXFMT_BGR24:
            src = row + ((size_t)x * 3u);
            rgba.b = src[0];
            rgba.g = src[1];
            rgba.r = src[2];
            rgba.a = 255u;
            *out_rgba = rgba;
            return;

        case TGA_PIXFMT_RGBA32:
            src = row + ((size_t)x * 4u);
            rgba.r = src[0];
            rgba.g = src[1];
            rgba.b = src[2];
            rgba.a = src[3];
            *out_rgba = rgba;
            return;

        case TGA_PIXFMT_BGRA32:
            src = row + ((size_t)x * 4u);
            rgba.b = src[0];
            rgba.g = src[1];
            rgba.r = src[2];
            rgba.a = src[3];
            *out_rgba = rgba;
            return;

        case TGA_PIXFMT_RGB565:
            src = row + ((size_t)x * 2u);
            pixel16 = (tga_u16)tga__read_u16_le(src);
            tga_unpack_rgb565(pixel16, &rgba.r, &rgba.g, &rgba.b);
            rgba.a = 255u;
            *out_rgba = rgba;
            return;

        case TGA_PIXFMT_ARGB1555:
            src = row + ((size_t)x * 2u);
            pixel16 = (tga_u16)tga__read_u16_le(src);
            tga_unpack_argb1555(pixel16, &rgba.a, &rgba.r, &rgba.g, &rgba.b);
            *out_rgba = rgba;
            return;

        default:
            rgba.r = 0u;
            rgba.g = 0u;
            rgba.b = 0u;
            rgba.a = 255u;
            *out_rgba = rgba;
            return;
    }
}

static void tga__encode_direct_sample(tga_u8 *dst, unsigned file_bits,
                                      const tga__rgba *rgba,
                                      int source_has_alpha)
{
    unsigned r5;
    unsigned g5;
    unsigned b5;
    unsigned a1;
    unsigned w;

    if (file_bits == 16u) {
        r5 = tga__scale_8_to_5((unsigned)rgba->r);
        g5 = tga__scale_8_to_5((unsigned)rgba->g);
        b5 = tga__scale_8_to_5((unsigned)rgba->b);
        a1 = source_has_alpha && rgba->a >= 128u ? 1u : 0u;
        w = (a1 << 15) | (r5 << 10) | (g5 << 5) | b5;
        dst[0] = (tga_u8)(w & 255u);
        dst[1] = (tga_u8)((w >> 8) & 255u);
        return;
    }

    if (file_bits == 24u) {
        dst[0] = rgba->b;
        dst[1] = rgba->g;
        dst[2] = rgba->r;
        return;
    }

    dst[0] = rgba->b;
    dst[1] = rgba->g;
    dst[2] = rgba->r;
    dst[3] = source_has_alpha ? rgba->a : 255u;
}

static int tga__palette_format_bits(tga_pixel_format fmt)
{
    switch (fmt) {
        case TGA_PIXFMT_RGB565:
        case TGA_PIXFMT_ARGB1555:
            return 16;
        case TGA_PIXFMT_RGB24:
        case TGA_PIXFMT_BGR24:
            return 24;
        case TGA_PIXFMT_RGBA32:
        case TGA_PIXFMT_BGRA32:
            return 32;
        default:
            return 0;
    }
}

static unsigned tga__encoded_palette_bits(const tga_encode_params *params)
{
    unsigned bits;

    if (params->pixel_format != TGA_PIXFMT_INDEX8) {
        return 0u;
    }

    bits = params->palette_file_entry_bits;
    if (bits == 0u) {
        bits = (unsigned)tga__palette_format_bits(params->palette_format);
    }
    return bits;
}

static void tga__encode_palette_sample(tga_u8 *dst, unsigned file_bits,
                                       tga_pixel_format fmt,
                                       const tga_u8 *src)
{
    tga__rgba rgba;
    unsigned r5;
    unsigned g5;
    unsigned b5;
    unsigned a1;
    unsigned w;

    tga__load_memory_pixel_rgba(src, 0u, fmt, &rgba);

    if (file_bits == 15u || file_bits == 16u) {
        r5 = tga__scale_8_to_5((unsigned)rgba.r);
        g5 = tga__scale_8_to_5((unsigned)rgba.g);
        b5 = tga__scale_8_to_5((unsigned)rgba.b);
        if (file_bits == 16u) {
            a1 = tga__memory_format_has_alpha(fmt)
                ? (rgba.a >= 128u ? 1u : 0u)
                : 1u;
        } else {
            a1 = 0u;
        }
        w = (a1 << 15) | (r5 << 10) | (g5 << 5) | b5;
        dst[0] = (tga_u8)(w & 255u);
        dst[1] = (tga_u8)((w >> 8) & 255u);
        return;
    }

    if (file_bits == 24u) {
        dst[0] = rgba.b;
        dst[1] = rgba.g;
        dst[2] = rgba.r;
        return;
    }

    dst[0] = rgba.b;
    dst[1] = rgba.g;
    dst[2] = rgba.r;
    dst[3] = rgba.a;
}

int tga_probe_memory(const void *src, size_t src_size, tga_info *out_info)
{
    int rc;
    const tga_u8 *bytes;

    if (src == 0 || out_info == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (src_size < 18u) {
        return TGA_ERR_TRUNCATED;
    }

    bytes = (const tga_u8 *)src;
    rc = tga__parse_header(bytes, out_info);
    if (rc != TGA_OK) {
        return rc;
    }

    if (src_size >= 26u) {
        if (memcmp(bytes + src_size - 18u, tga__footer_sig, 18u) == 0) {
            out_info->has_footer = 1;
        }
    }

    return TGA_OK;
}

static int tga__checked_mul_size(size_t a, size_t b, size_t *out)
{
    if (a != 0u && b > ((size_t)-1) / a) {
        return 0;
    }
    *out = a * b;
    return 1;
}

static int tga__checked_add_size(size_t a, size_t b, size_t *out)
{
    if (a > (size_t)-1 - b) {
        return 0;
    }
    *out = a + b;
    return 1;
}

static int tga__region_fits(size_t offset, size_t size, size_t total)
{
    if (offset > total) {
        return 0;
    }
    if (size > total - offset) {
        return 0;
    }
    return 1;
}

static int tga__color_map_size_from_info(const tga_info *info, size_t *out_size)
{
    size_t entry_bytes;

    entry_bytes = (size_t)tga__file_sample_bytes(info->color_map_entry_bits);
    return tga__checked_mul_size((size_t)info->color_map_length, entry_bytes, out_size);
}

static int tga__image_data_offset_from_info(const tga_info *info, size_t *out_offset)
{
    size_t color_map_size;
    size_t base;

    if (!tga__color_map_size_from_info(info, &color_map_size)) {
        return 0;
    }
    base = 18u + (size_t)info->id_length;
    if (base > ((size_t)-1) - color_map_size) {
        return 0;
    }
    *out_offset = base + color_map_size;
    return 1;
}

static int tga__measure_image_data_size_memory(const tga_u8 *bytes,
                                               size_t src_size,
                                               const tga_info *info,
                                               size_t image_data_offset,
                                               size_t *out_size)
{
    size_t sample_bytes;
    size_t total_pixels;
    size_t total_size;
    size_t pos;
    unsigned packet_header;
    size_t count;
    size_t need;
    size_t produced;

    sample_bytes = (size_t)tga__file_sample_bytes(info->pixel_depth);
    if (!tga__checked_mul_size((size_t)info->width, (size_t)info->height, &total_pixels)) {
        return TGA_ERR_OVERFLOW;
    }

    if (!info->is_rle) {
        if (!tga__checked_mul_size(total_pixels, sample_bytes, &total_size)) {
            return TGA_ERR_OVERFLOW;
        }
        if (!tga__region_fits(image_data_offset, total_size, src_size)) {
            return TGA_ERR_TRUNCATED;
        }
        *out_size = total_size;
        return TGA_OK;
    }

    pos = image_data_offset;
    produced = 0u;
    while (produced < total_pixels) {
        if (!tga__region_fits(pos, 1u, src_size)) {
            return TGA_ERR_TRUNCATED;
        }
        packet_header = (unsigned)bytes[pos];
        ++pos;
        count = (size_t)(packet_header & 127u) + 1u;
        if (count > total_pixels - produced) {
            return TGA_ERR_BAD_FORMAT;
        }
        if ((packet_header & 128u) != 0u) {
            need = sample_bytes;
        } else {
            if (!tga__checked_mul_size(count, sample_bytes, &need)) {
                return TGA_ERR_OVERFLOW;
            }
        }
        if (!tga__region_fits(pos, need, src_size)) {
            return TGA_ERR_TRUNCATED;
        }
        pos += need;
        produced += count;
    }

    *out_size = pos - image_data_offset;
    return TGA_OK;
}

static int tga__read_section_memory(const void *src,
                                    size_t src_size,
                                    size_t offset,
                                    size_t size,
                                    void *dst,
                                    size_t dst_capacity,
                                    size_t *out_size)
{
    const tga_u8 *bytes;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_size != 0) {
        *out_size = size;
    }
    if (!tga__region_fits(offset, size, src_size)) {
        return TGA_ERR_TRUNCATED;
    }
    if (dst == 0 || dst_capacity == 0u) {
        return TGA_OK;
    }
    if (dst_capacity < size) {
        return TGA_ERR_CAPACITY;
    }
    bytes = (const tga_u8 *)src;
    if (size != 0u) {
        memcpy(dst, bytes + offset, size);
    }
    return TGA_OK;
}

static int tga__validate_decode_info(const tga_info *info)
{
    if (info->width == 0u || info->height == 0u) {
        return TGA_ERR_BAD_FORMAT;
    }

    if ((info->descriptor & 0xC0u) != 0u) {
        return TGA_ERR_UNSUPPORTED;
    }

    if (info->color_map_type != 0u && info->color_map_type != 1u) {
        return TGA_ERR_BAD_FORMAT;
    }

    if (!info->is_color_mapped &&
        !tga__is_truecolor_type(info->image_type) &&
        !info->is_grayscale) {
        return TGA_ERR_UNSUPPORTED;
    }

    if (info->is_color_mapped) {
        if (info->color_map_type != 1u || info->color_map_length == 0u) {
            return TGA_ERR_BAD_FORMAT;
        }
        if (info->pixel_depth != 8u &&
            info->pixel_depth != 15u &&
            info->pixel_depth != 16u) {
            return TGA_ERR_UNSUPPORTED;
        }
        if (info->color_map_entry_bits != 15u &&
            info->color_map_entry_bits != 16u &&
            info->color_map_entry_bits != 24u &&
            info->color_map_entry_bits != 32u) {
            return TGA_ERR_UNSUPPORTED;
        }
        return TGA_OK;
    }

    if (tga__is_truecolor_type(info->image_type)) {
        if (info->pixel_depth != 15u &&
            info->pixel_depth != 16u &&
            info->pixel_depth != 24u &&
            info->pixel_depth != 32u) {
            return TGA_ERR_UNSUPPORTED;
        }
        return TGA_OK;
    }

    if (info->is_grayscale) {
        if (info->pixel_depth != 8u && info->pixel_depth != 16u) {
            return TGA_ERR_UNSUPPORTED;
        }
        return TGA_OK;
    }

    return TGA_ERR_UNSUPPORTED;
}

static int tga__load_palette(tga_reader *reader, const tga_info *info,
                             tga_u8 *palette_rgba, unsigned palette_capacity)
{
    unsigned entry_bytes;
    unsigned i;
    tga_u8 sample[4];
    tga__rgba rgba;

    if (info->color_map_length == 0u) {
        return TGA_OK;
    }

    entry_bytes = tga__file_sample_bytes(info->color_map_entry_bits);

    if (!info->is_color_mapped) {
        return tga__skip(reader, (size_t)entry_bytes * (size_t)info->color_map_length)
            ? TGA_OK : TGA_ERR_TRUNCATED;
    }

    if (palette_rgba == 0 || palette_capacity < info->color_map_length) {
        return TGA_ERR_NEED_PALETTE_BUFFER;
    }

    for (i = 0u; i < info->color_map_length; ++i) {
        if (!tga__read_exact(reader, sample, entry_bytes)) {
            return TGA_ERR_TRUNCATED;
        }
        tga__decode_color_sample(sample, info->color_map_entry_bits,
                                 info->color_map_entry_bits == 16u ? 1u : 0u,
                                 0, &rgba);
        palette_rgba[(size_t)i * 4u + 0u] = rgba.r;
        palette_rgba[(size_t)i * 4u + 1u] = rgba.g;
        palette_rgba[(size_t)i * 4u + 2u] = rgba.b;
        palette_rgba[(size_t)i * 4u + 3u] = rgba.a;
    }

    return TGA_OK;
}

static int tga__decode_index_sample(const tga_info *info, const tga_u8 *sample,
                                    const tga_u8 *palette_rgba,
                                    tga__rgba *out_rgba)
{
    unsigned idx;
    size_t off;

    if (info->pixel_depth == 8u) {
        idx = (unsigned)sample[0];
    } else {
        idx = (unsigned)sample[0] | ((unsigned)sample[1] << 8);
        if (info->pixel_depth == 15u) {
            idx &= 0x7FFFu;
        }
    }

    if (idx < info->color_map_origin) {
        return TGA_ERR_PALETTE_RANGE;
    }
    idx -= info->color_map_origin;
    if (idx >= info->color_map_length) {
        return TGA_ERR_PALETTE_RANGE;
    }

    off = (size_t)idx * 4u;
    out_rgba->r = palette_rgba[off + 0u];
    out_rgba->g = palette_rgba[off + 1u];
    out_rgba->b = palette_rgba[off + 2u];
    out_rgba->a = palette_rgba[off + 3u];
    return TGA_OK;
}

static void tga__parse_color_correction_table(const tga_u8 *src,
                                              tga_color_correction_entry *table)
{
    unsigned i;

    for (i = 0u; i < TGA_COLOR_CORRECTION_ENTRY_COUNT; ++i) {
        table[i].alpha = tga__read_u16_le(src + (size_t)i * 8u + 0u);
        table[i].red = tga__read_u16_le(src + (size_t)i * 8u + 2u);
        table[i].green = tga__read_u16_le(src + (size_t)i * 8u + 4u);
        table[i].blue = tga__read_u16_le(src + (size_t)i * 8u + 6u);
    }
}

static void tga__fill_postage_stamp_info(const tga_info *info,
                                         unsigned width,
                                         unsigned height,
                                         tga_postage_stamp_info *out_info)
{
    unsigned sample_bytes;

    if (out_info == 0) {
        return;
    }

    memset(out_info, 0, sizeof(*out_info));
    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    out_info->width = width;
    out_info->height = height;
    out_info->pixel_depth = info->pixel_depth;
    out_info->sample_bytes = sample_bytes;
    out_info->data_size = (size_t)width * (size_t)height * (size_t)sample_bytes;
    out_info->block_size = 2u + out_info->data_size;
}

static int tga__decode_uncompressed_pixels_from_reader(tga_reader *reader,
                                                       const tga_info *info,
                                                       unsigned width,
                                                       unsigned height,
                                                       const tga_decode_params *params)
{
    unsigned sample_bytes;
    unsigned long total_pixels;
    unsigned long produced;
    unsigned long file_row;
    unsigned long file_col;
    unsigned logical_y;
    unsigned logical_x;
    unsigned dst_y;
    tga_u8 *dst_row;
    tga_u8 sample[4];
    tga__rgba rgba;
    int rc;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    total_pixels = (unsigned long)width * (unsigned long)height;
    produced = 0u;

    while (produced < total_pixels) {
        if (!tga__read_exact(reader, sample, sample_bytes)) {
            return TGA_ERR_TRUNCATED;
        }

        file_row = produced / (unsigned long)width;
        file_col = produced % (unsigned long)width;
        logical_y = info->file_top_origin
            ? (unsigned)file_row
            : (unsigned)(height - 1u - (unsigned)file_row);
        logical_x = info->file_right_to_left
            ? (unsigned)(width - 1u - (unsigned)file_col)
            : (unsigned)file_col;

        if (params->output_top_origin) {
            dst_y = logical_y;
        } else {
            dst_y = height - 1u - logical_y;
        }

        dst_row = (tga_u8 *)params->pixels + (size_t)dst_y * (size_t)params->stride_bytes;

        if (info->is_color_mapped) {
            rc = tga__decode_index_sample(info, sample,
                                          params->palette_rgba,
                                          &rgba);
            if (rc != TGA_OK) {
                return rc;
            }
        } else {
            tga__decode_color_sample(sample, info->pixel_depth,
                                     info->descriptor & 0x0Fu,
                                     info->is_grayscale, &rgba);
        }

        tga__store_output_pixel(dst_row, logical_x, params->pixel_format, &rgba);
        ++produced;
    }

    return TGA_OK;
}

static int tga__decode_stream_pixels(tga_reader *reader, const tga_info *info,
                                     const tga_decode_params *params)
{
    unsigned sample_bytes;
    unsigned out_bpp;
    unsigned long total_pixels;
    unsigned long produced;
    unsigned long i;
    unsigned long file_row;
    unsigned long file_col;
    unsigned logical_y;
    unsigned logical_x;
    unsigned dst_y;
    tga_u8 *dst_row;
    tga_u8 sample[4];
    tga__rgba rgba;
    int rc;
    unsigned count;
    unsigned packet_header;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    out_bpp = tga_bytes_per_pixel(params->pixel_format);
    total_pixels = (unsigned long)info->width * (unsigned long)info->height;
    produced = 0u;

    while (produced < total_pixels) {
        if (info->is_rle) {
            if (!tga__read_exact(reader, sample, 1u)) {
                return TGA_ERR_TRUNCATED;
            }
            packet_header = (unsigned)sample[0];
            count = (packet_header & 0x7Fu) + 1u;

            if (count > total_pixels - produced) {
                return TGA_ERR_BAD_FORMAT;
            }

            if ((packet_header & 0x80u) != 0u) {
                if (!tga__read_exact(reader, sample, sample_bytes)) {
                    return TGA_ERR_TRUNCATED;
                }
                for (i = 0u; i < (unsigned long)count; ++i) {
                    file_row = produced / (unsigned long)info->width;
                    file_col = produced % (unsigned long)info->width;
                    logical_y = info->file_top_origin
                        ? (unsigned)file_row
                        : (unsigned)(info->height - 1u - (unsigned)file_row);
                    logical_x = info->file_right_to_left
                        ? (unsigned)(info->width - 1u - (unsigned)file_col)
                        : (unsigned)file_col;

                    if (params->output_top_origin) {
                        dst_y = logical_y;
                    } else {
                        dst_y = info->height - 1u - logical_y;
                    }

                    dst_row = (tga_u8 *)params->pixels + (size_t)dst_y * (size_t)params->stride_bytes;

                    if (info->is_color_mapped) {
                        rc = tga__decode_index_sample(info, sample,
                                                      params->palette_rgba,
                                                      &rgba);
                        if (rc != TGA_OK) {
                            return rc;
                        }
                    } else {
                        tga__decode_color_sample(sample, info->pixel_depth,
                                                 info->descriptor & 0x0Fu,
                                                 info->is_grayscale, &rgba);
                    }

                    tga__store_output_pixel(dst_row, logical_x, params->pixel_format, &rgba);
                    ++produced;
                }
            } else {
                for (i = 0u; i < (unsigned long)count; ++i) {
                    if (!tga__read_exact(reader, sample, sample_bytes)) {
                        return TGA_ERR_TRUNCATED;
                    }

                    file_row = produced / (unsigned long)info->width;
                    file_col = produced % (unsigned long)info->width;
                    logical_y = info->file_top_origin
                        ? (unsigned)file_row
                        : (unsigned)(info->height - 1u - (unsigned)file_row);
                    logical_x = info->file_right_to_left
                        ? (unsigned)(info->width - 1u - (unsigned)file_col)
                        : (unsigned)file_col;

                    if (params->output_top_origin) {
                        dst_y = logical_y;
                    } else {
                        dst_y = info->height - 1u - logical_y;
                    }

                    dst_row = (tga_u8 *)params->pixels + (size_t)dst_y * (size_t)params->stride_bytes;

                    if (info->is_color_mapped) {
                        rc = tga__decode_index_sample(info, sample,
                                                      params->palette_rgba,
                                                      &rgba);
                        if (rc != TGA_OK) {
                            return rc;
                        }
                    } else {
                        tga__decode_color_sample(sample, info->pixel_depth,
                                                 info->descriptor & 0x0Fu,
                                                 info->is_grayscale, &rgba);
                    }

                    tga__store_output_pixel(dst_row, logical_x, params->pixel_format, &rgba);
                    ++produced;
                }
            }
        } else {
            if (!tga__read_exact(reader, sample, sample_bytes)) {
                return TGA_ERR_TRUNCATED;
            }

            file_row = produced / (unsigned long)info->width;
            file_col = produced % (unsigned long)info->width;
            logical_y = info->file_top_origin
                ? (unsigned)file_row
                : (unsigned)(info->height - 1u - (unsigned)file_row);
            logical_x = info->file_right_to_left
                ? (unsigned)(info->width - 1u - (unsigned)file_col)
                : (unsigned)file_col;

            if (params->output_top_origin) {
                dst_y = logical_y;
            } else {
                dst_y = info->height - 1u - logical_y;
            }

            dst_row = (tga_u8 *)params->pixels + (size_t)dst_y * (size_t)params->stride_bytes;

            if (info->is_color_mapped) {
                rc = tga__decode_index_sample(info, sample,
                                              params->palette_rgba,
                                              &rgba);
                if (rc != TGA_OK) {
                    return rc;
                }
            } else {
                tga__decode_color_sample(sample, info->pixel_depth,
                                         info->descriptor & 0x0Fu,
                                         info->is_grayscale, &rgba);
            }

            tga__store_output_pixel(dst_row, logical_x, params->pixel_format, &rgba);
            ++produced;
        }
    }

    (void)out_bpp;
    return TGA_OK;
}

int tga_decode(tga_reader *reader, const tga_decode_params *params, tga_info *out_info)
{
    tga_u8 hdr[18];
    tga_info info;
    unsigned out_bpp;
    unsigned long min_stride;
    int rc;

    if (reader == 0 || params == 0 || params->pixels == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__pixel_format_valid(params->pixel_format) ||
        params->pixel_format == TGA_PIXFMT_INDEX8) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    if (!tga__read_exact(reader, hdr, sizeof(hdr))) {
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__parse_header(hdr, &info);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__validate_decode_info(&info);
    if (rc != TGA_OK) {
        return rc;
    }

    out_bpp = tga_bytes_per_pixel(params->pixel_format);
    min_stride = (unsigned long)info.width * (unsigned long)out_bpp;
    if (params->stride_bytes < min_stride) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    if (!tga__skip(reader, (size_t)info.id_length)) {
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__load_palette(reader, &info, params->palette_rgba, params->palette_capacity);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__decode_stream_pixels(reader, &info, params);
    if (rc != TGA_OK) {
        return rc;
    }

    if (out_info != 0) {
        *out_info = info;
    }
    return TGA_OK;
}

int tga_decode_memory(const void *src, size_t src_size,
                      const tga_decode_params *params, tga_info *out_info)
{
    tga_mem_reader mem;
    tga_reader reader;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    mem.data = (const tga_u8 *)src;
    mem.size = src_size;
    mem.pos = 0u;

    reader.read = tga_mem_read;
    reader.user = &mem;
    return tga_decode(&reader, params, out_info);
}

static int tga__validate_index_pixels(const tga_encode_params *params)
{
    unsigned y;
    unsigned x;
    unsigned src_y;
    const tga_u8 *row;
    unsigned file_bits;
    unsigned value;

    if (params->palette_count == 0u || params->palette_count > 256u) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    file_bits = params->file_pixel_depth;
    if (file_bits == 0u) {
        file_bits = 8u;
    }

    for (y = 0u; y < params->height; ++y) {
        if (params->input_top_origin) {
            src_y = y;
        } else {
            src_y = params->height - 1u - y;
        }

        row = (const tga_u8 *)params->pixels + (size_t)src_y * (size_t)params->stride_bytes;
        for (x = 0u; x < params->width; ++x) {
            if ((unsigned)row[x] >= params->palette_count) {
                return TGA_ERR_PALETTE_RANGE;
            }
            value = params->palette_first_index + (unsigned)row[x];
            if (file_bits == 8u && value > 255u) {
                return TGA_ERR_INDEX_RANGE;
            }
        }
    }

    return TGA_OK;
}

static int tga__validate_encode_params(const tga_encode_params *params)
{
    unsigned src_bpp;
    unsigned long min_stride;
    unsigned palette_bits;

    if (params == 0 || params->pixels == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (params->width == 0u || params->height == 0u) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__pixel_format_valid(params->pixel_format)) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (params->width > 65535u || params->height > 65535u) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (params->image_id_length > 255u) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (params->image_id_length != 0u && params->image_id == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    src_bpp = tga_bytes_per_pixel(params->pixel_format);
    min_stride = (unsigned long)params->width * (unsigned long)src_bpp;
    if (params->stride_bytes < min_stride) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    if (params->pixel_format == TGA_PIXFMT_INDEX8) {
        if (params->palette == 0) {
            return TGA_ERR_BAD_ARGUMENT;
        }
        palette_bits = tga__encoded_palette_bits(params);
        if ((unsigned)tga__palette_format_bits(params->palette_format) == 0u) {
            return TGA_ERR_BAD_ARGUMENT;
        }
        if (palette_bits != 16u && palette_bits != 24u && palette_bits != 32u) {
            return TGA_ERR_BAD_ARGUMENT;
        }
        if (params->file_pixel_depth != 0u &&
            params->file_pixel_depth != 8u &&
            params->file_pixel_depth != 16u) {
            return TGA_ERR_BAD_ARGUMENT;
        }
        if (params->palette_first_index + params->palette_count > 65535u) {
            return TGA_ERR_BAD_ARGUMENT;
        }
        return tga__validate_index_pixels(params);
    }

    if (params->pixel_format == TGA_PIXFMT_GRAY8) {
        return TGA_OK;
    }

    if (params->file_pixel_depth != 16u &&
        params->file_pixel_depth != 24u &&
        params->file_pixel_depth != 32u) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    return TGA_OK;
}

static unsigned tga__encoded_file_bits(const tga_encode_params *params)
{
    if (params->pixel_format == TGA_PIXFMT_GRAY8) {
        return 8u;
    }
    if (params->pixel_format == TGA_PIXFMT_INDEX8) {
        return params->file_pixel_depth != 0u ? params->file_pixel_depth : 8u;
    }
    return params->file_pixel_depth;
}

static unsigned tga__image_type_for_encode(const tga_encode_params *params)
{
    if (params->pixel_format == TGA_PIXFMT_GRAY8) {
        return params->rle ? 11u : 3u;
    }
    if (params->pixel_format == TGA_PIXFMT_INDEX8) {
        return params->rle ? 9u : 1u;
    }
    return params->rle ? 10u : 2u;
}

static unsigned tga__descriptor_for_encode(const tga_encode_params *params)
{
    unsigned desc;
    unsigned bits;

    desc = 0u;
    bits = tga__encoded_file_bits(params);

    if (params->write_top_origin) {
        desc |= 0x20u;
    }
    if (params->write_right_to_left) {
        desc |= 0x10u;
    }

    if (params->pixel_format == TGA_PIXFMT_INDEX8) {
        return desc;
    }
    if (params->pixel_format == TGA_PIXFMT_GRAY8) {
        return desc;
    }

    if (bits == 16u) {
        if (tga__memory_format_has_alpha(params->pixel_format)) {
            desc |= 1u;
        }
    } else if (bits == 32u) {
        desc |= 8u;
    }

    return desc;
}

unsigned tga_default_attributes_type_from_info(const tga_info *info)
{
    if (info == 0) {
        return TGA_ATTRIBUTES_TYPE_NONE;
    }
    return (info->descriptor & 0x0Fu) != 0u ? TGA_ATTRIBUTES_TYPE_ALPHA
                                             : TGA_ATTRIBUTES_TYPE_NONE;
}

unsigned tga_default_attributes_type_from_encode_params(const tga_encode_params *params)
{
    if (params == 0) {
        return TGA_ATTRIBUTES_TYPE_NONE;
    }
    return (tga__descriptor_for_encode(params) & 0x0Fu) != 0u ?
           TGA_ATTRIBUTES_TYPE_ALPHA :
           TGA_ATTRIBUTES_TYPE_NONE;
}

static unsigned tga__palette_bytes(const tga_encode_params *params)
{
    unsigned bits;

    if (params->pixel_format != TGA_PIXFMT_INDEX8) {
        return 0u;
    }
    bits = tga__encoded_palette_bits(params);
    return tga__file_sample_bytes(bits);
}

static const tga_u8 *tga__palette_entry_ptr(const tga_encode_params *params, unsigned index)
{
    unsigned bpp;

    bpp = tga_bytes_per_pixel(params->palette_format);
    return (const tga_u8 *)params->palette + (size_t)index * (size_t)bpp;
}

static const tga_u8 *tga__source_row_ptr(const tga_encode_params *params, unsigned logical_y)
{
    unsigned src_y;

    if (params->input_top_origin) {
        src_y = logical_y;
    } else {
        src_y = params->height - 1u - logical_y;
    }
    return (const tga_u8 *)params->pixels + (size_t)src_y * (size_t)params->stride_bytes;
}

static void tga__encode_image_sample_at(const tga_encode_params *params,
                                        unsigned logical_y, unsigned logical_x,
                                        tga_u8 *out_sample)
{
    const tga_u8 *row;
    const tga_u8 *src;
    tga__rgba rgba;
    unsigned file_bits;

    row = tga__source_row_ptr(params, logical_y);

    if (params->pixel_format == TGA_PIXFMT_INDEX8) {
        unsigned value;
        unsigned file_bits_local;

        value = params->palette_first_index + (unsigned)row[logical_x];
        file_bits_local = tga__encoded_file_bits(params);
        out_sample[0] = (tga_u8)(value & 255u);
        if (file_bits_local > 8u) {
            out_sample[1] = (tga_u8)((value >> 8) & 255u);
        }
        return;
    }

    if (params->pixel_format == TGA_PIXFMT_GRAY8) {
        out_sample[0] = row[logical_x];
        return;
    }

    file_bits = params->file_pixel_depth;
    tga__load_memory_pixel_rgba(row, logical_x, params->pixel_format, &rgba);
    tga__encode_direct_sample(out_sample, file_bits, &rgba,
                              tga__memory_format_has_alpha(params->pixel_format));
    (void)src;
}

static int tga__samples_equal(const tga_u8 *a, const tga_u8 *b, unsigned bytes)
{
    unsigned i;

    for (i = 0u; i < bytes; ++i) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

static unsigned tga__file_row_to_logical_y(const tga_encode_params *params,
                                           unsigned file_row)
{
    if (params->write_top_origin) {
        return file_row;
    }
    return params->height - 1u - file_row;
}

static unsigned tga__file_col_to_logical_x(const tga_encode_params *params,
                                           unsigned file_col)
{
    if (params->write_right_to_left) {
        return params->width - 1u - file_col;
    }
    return file_col;
}

static unsigned long tga__encoded_row_size_exact(const tga_encode_params *params,
                                                 unsigned file_row)
{
    unsigned file_bytes;
    unsigned x;
    unsigned logical_y;
    unsigned logical_x;
    unsigned run;
    unsigned raw;
    unsigned long total;
    tga_u8 a[4];
    tga_u8 b[4];

    file_bytes = tga__file_sample_bytes(tga__encoded_file_bits(params));

    if (!params->rle) {
        return (unsigned long)params->width * (unsigned long)file_bytes;
    }

    total = 0ul;
    logical_y = tga__file_row_to_logical_y(params, file_row);
    x = 0u;

    while (x < params->width) {
        logical_x = tga__file_col_to_logical_x(params, x);
        tga__encode_image_sample_at(params, logical_y, logical_x, a);

        run = 1u;
        while (x + run < params->width && run < 128u) {
            logical_x = tga__file_col_to_logical_x(params, x + run);
            tga__encode_image_sample_at(params, logical_y, logical_x, b);
            if (!tga__samples_equal(a, b, file_bytes)) {
                break;
            }
            ++run;
        }

        if (run >= 2u) {
            total += 1ul + (unsigned long)file_bytes;
            x += run;
            continue;
        }

        raw = 1u;
        while (x + raw < params->width && raw < 128u) {
            logical_x = tga__file_col_to_logical_x(params, x + raw);
            tga__encode_image_sample_at(params, logical_y, logical_x, a);

            run = 1u;
            while (x + raw + run < params->width && run < 128u) {
                logical_x = tga__file_col_to_logical_x(params, x + raw + run);
                tga__encode_image_sample_at(params, logical_y, logical_x, b);
                if (!tga__samples_equal(a, b, file_bytes)) {
                    break;
                }
                ++run;
            }

            if (run >= 2u) {
                break;
            }

            ++raw;
        }

        total += 1ul + (unsigned long)raw * (unsigned long)file_bytes;
        x += raw;
    }

    return total;
}

static unsigned long tga__image_data_offset(const tga_encode_params *params)
{
    unsigned long total;

    total = 18ul + (unsigned long)params->image_id_length;
    total += (unsigned long)params->palette_count *
             (unsigned long)tga__palette_bytes(params);
    return total;
}

static int tga__postage_stamp_has_custom_source(const tga_encode_params_ex *params)
{
    return params->postage_stamp_source != 0 &&
           params->postage_stamp_source->pixels != 0;
}

static int tga__validate_postage_stamp_source(const tga_encode_params_ex *params)
{
    const tga_postage_stamp_source *src;
    unsigned src_bpp;
    unsigned long min_stride;

    src = params->postage_stamp_source;
    if (src == 0 || src->pixels == 0) {
        return TGA_OK;
    }

    if (src->width == 0u || src->height == 0u ||
        src->width > 255u || src->height > 255u) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__pixel_format_valid(src->pixel_format)) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    if (params->image.pixel_format == TGA_PIXFMT_INDEX8) {
        if (src->pixel_format != TGA_PIXFMT_INDEX8) {
            return TGA_ERR_BAD_ARGUMENT;
        }
    } else if (src->pixel_format == TGA_PIXFMT_INDEX8) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    src_bpp = tga_bytes_per_pixel(src->pixel_format);
    min_stride = (unsigned long)src->width * (unsigned long)src_bpp;
    if (src->stride_bytes < min_stride) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    return TGA_OK;
}

static int tga__validate_postage_stamp_request(const tga_encode_params_ex *params)
{
    int rc;

    if (!params->write_postage_stamp) {
        return TGA_OK;
    }

    rc = tga__validate_postage_stamp_source(params);
    if (rc != TGA_OK) {
        return rc;
    }

    if (!tga__postage_stamp_has_custom_source(params) &&
        (params->postage_stamp_max_width > 255u ||
         params->postage_stamp_max_height > 255u)) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    return TGA_OK;
}

static void tga__resolve_postage_stamp_limits(const tga_encode_params_ex *params,
                                              unsigned *out_max_width,
                                              unsigned *out_max_height)
{
    unsigned max_width;
    unsigned max_height;

    max_width = params->postage_stamp_max_width;
    max_height = params->postage_stamp_max_height;
    if (max_width == 0u) {
        max_width = 64u;
    }
    if (max_height == 0u) {
        max_height = 64u;
    }

    *out_max_width = max_width;
    *out_max_height = max_height;
}

static void tga__choose_postage_stamp_size(const tga_encode_params_ex *params,
                                           unsigned *out_width,
                                           unsigned *out_height)
{
    const tga_postage_stamp_source *src;
    unsigned max_width;
    unsigned max_height;
    unsigned src_width;
    unsigned src_height;
    unsigned width;
    unsigned height;

    src = params->postage_stamp_source;
    if (src != 0 && src->pixels != 0) {
        *out_width = src->width;
        *out_height = src->height;
        return;
    }

    tga__resolve_postage_stamp_limits(params, &max_width, &max_height);

    src_width = params->image.width;
    src_height = params->image.height;

    if (src_width <= max_width && src_height <= max_height) {
        *out_width = src_width;
        *out_height = src_height;
        return;
    }

    if ((unsigned long)src_width * (unsigned long)max_height >=
        (unsigned long)src_height * (unsigned long)max_width) {
        width = max_width;
        height = (unsigned)(((unsigned long)src_height * (unsigned long)max_width +
                             (unsigned long)src_width / 2ul) /
                            (unsigned long)src_width);
        if (height == 0u) {
            height = 1u;
        }
    } else {
        height = max_height;
        width = (unsigned)(((unsigned long)src_width * (unsigned long)max_height +
                            (unsigned long)src_height / 2ul) /
                           (unsigned long)src_height);
        if (width == 0u) {
            width = 1u;
        }
    }

    *out_width = width;
    *out_height = height;
}

static int tga__compute_postage_stamp_block_size(const tga_encode_params_ex *params,
                                                 unsigned *out_width,
                                                 unsigned *out_height,
                                                 unsigned long *out_size)
{
    unsigned width;
    unsigned height;
    unsigned sample_bytes;
    unsigned long size;

    if (!params->write_postage_stamp) {
        if (out_width != 0) {
            *out_width = 0u;
        }
        if (out_height != 0) {
            *out_height = 0u;
        }
        *out_size = 0ul;
        return TGA_OK;
    }

    tga__choose_postage_stamp_size(params, &width, &height);
    sample_bytes = tga__file_sample_bytes(tga__encoded_file_bits(&params->image));
    size = 2ul + (unsigned long)width * (unsigned long)height * (unsigned long)sample_bytes;

    if (out_width != 0) {
        *out_width = width;
    }
    if (out_height != 0) {
        *out_height = height;
    }
    *out_size = size;
    return TGA_OK;
}

static const tga_u8 *tga__postage_source_row_ptr(const tga_postage_stamp_source *src,
                                                 unsigned logical_y)
{
    unsigned src_y;

    if (src->input_top_origin) {
        src_y = logical_y;
    } else {
        src_y = src->height - 1u - logical_y;
    }
    return (const tga_u8 *)src->pixels + (size_t)src_y * (size_t)src->stride_bytes;
}

static void tga__encode_postage_custom_sample(const tga_encode_params *image,
                                              const tga_postage_stamp_source *src,
                                              unsigned logical_y,
                                              unsigned logical_x,
                                              tga_u8 *out_sample)
{
    const tga_u8 *row;
    tga__rgba rgba;
    unsigned file_bits;
    tga_u8 gray;

    row = tga__postage_source_row_ptr(src, logical_y);

    if (image->pixel_format == TGA_PIXFMT_INDEX8) {
        unsigned value;
        unsigned file_bits_local;

        value = image->palette_first_index + (unsigned)row[logical_x];
        file_bits_local = tga__encoded_file_bits(image);
        out_sample[0] = (tga_u8)(value & 255u);
        if (file_bits_local > 8u) {
            out_sample[1] = (tga_u8)((value >> 8) & 255u);
        }
        return;
    }

    if (image->pixel_format == TGA_PIXFMT_GRAY8) {
        if (src->pixel_format == TGA_PIXFMT_GRAY8) {
            out_sample[0] = row[logical_x];
            return;
        }
        tga__load_memory_pixel_rgba(row, logical_x, src->pixel_format, &rgba);
        gray = tga__rgb_to_gray(rgba.r, rgba.g, rgba.b);
        out_sample[0] = gray;
        return;
    }

    if (src->pixel_format == TGA_PIXFMT_GRAY8) {
        gray = row[logical_x];
        rgba.r = gray;
        rgba.g = gray;
        rgba.b = gray;
        rgba.a = 255u;
    } else {
        tga__load_memory_pixel_rgba(row, logical_x, src->pixel_format, &rgba);
    }

    file_bits = image->file_pixel_depth;
    tga__encode_direct_sample(out_sample, file_bits, &rgba,
                              tga__memory_format_has_alpha(src->pixel_format));
}

static int tga__validate_developer_fields(const tga_developer_field *fields,
                                          unsigned count)
{
    unsigned i;

    if (count == 0u) {
        return TGA_OK;
    }
    if (fields == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (count > 65535u) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    for (i = 0u; i < count; ++i) {
        if (fields[i].tag > 65535u) {
            return TGA_ERR_BAD_ARGUMENT;
        }
        if (fields[i].data_size != 0ul && fields[i].data == 0) {
            return TGA_ERR_BAD_ARGUMENT;
        }
    }

    return TGA_OK;
}

static int tga__compute_developer_payload_size(const tga_developer_field *fields,
                                               unsigned count,
                                               unsigned long *out_total)
{
    unsigned i;
    unsigned long total;

    total = 0ul;
    for (i = 0u; i < count; ++i) {
        if ((unsigned long)fields[i].data_size > 4294967295ul - total) {
            return TGA_ERR_OVERFLOW;
        }
        total += (unsigned long)fields[i].data_size;
    }

    *out_total = total;
    return TGA_OK;
}

static unsigned long tga__developer_directory_size(unsigned count)
{
    return 2ul + (unsigned long)count * 10ul;
}

static int tga__write_developer_area(tga_writer *writer,
                                     const tga_developer_field *fields,
                                     unsigned count,
                                     tga_u32 developer_data_offset)
{
    unsigned i;
    unsigned long current_offset;
    tga_u8 buf2[2];
    tga_u8 entry[10];

    if (count == 0u) {
        return TGA_OK;
    }

    for (i = 0u; i < count; ++i) {
        if (fields[i].data_size != 0ul) {
            if (!tga__write_exact(writer, fields[i].data, (size_t)fields[i].data_size)) {
                return TGA_ERR_IO;
            }
        }
    }

    tga__write_u16_le(buf2, count);
    if (!tga__write_exact(writer, buf2, sizeof(buf2))) {
        return TGA_ERR_IO;
    }

    current_offset = (unsigned long)developer_data_offset;
    for (i = 0u; i < count; ++i) {
        tga__write_u16_le(entry + 0u, fields[i].tag);
        tga__write_u32_le(entry + 2u, (tga_u32)current_offset);
        tga__write_u32_le(entry + 6u, fields[i].data_size);
        if (!tga__write_exact(writer, entry, sizeof(entry))) {
            return TGA_ERR_IO;
        }
        current_offset += (unsigned long)fields[i].data_size;
    }

    return TGA_OK;
}

static int tga__write_scan_line_table(tga_writer *writer,
                                      const tga_encode_params *params,
                                      tga_u32 image_data_offset)
{
    unsigned file_row;
    unsigned long current_offset;
    unsigned long row_size;
    tga_u8 entry[4];

    current_offset = (unsigned long)image_data_offset;
    for (file_row = 0u; file_row < params->height; ++file_row) {
        tga__write_u32_le(entry, (tga_u32)current_offset);
        if (!tga__write_exact(writer, entry, sizeof(entry))) {
            return TGA_ERR_IO;
        }
        row_size = tga__encoded_row_size_exact(params, file_row);
        current_offset += row_size;
    }

    return TGA_OK;
}

static int tga__write_color_correction_table(tga_writer *writer,
                                              const tga_color_correction_entry *table)
{
    unsigned i;
    tga_u8 entry[8];

    if (table == 0) {
        return TGA_OK;
    }

    for (i = 0u; i < TGA_COLOR_CORRECTION_ENTRY_COUNT; ++i) {
        tga__write_u16_le(entry + 0u, table[i].alpha);
        tga__write_u16_le(entry + 2u, table[i].red);
        tga__write_u16_le(entry + 4u, table[i].green);
        tga__write_u16_le(entry + 6u, table[i].blue);
        if (!tga__write_exact(writer, entry, sizeof(entry))) {
            return TGA_ERR_IO;
        }
    }

    return TGA_OK;
}

static int tga__write_postage_stamp(tga_writer *writer,
                                    const tga_encode_params_ex *params,
                                    unsigned stamp_width,
                                    unsigned stamp_height)
{
    const tga_postage_stamp_source *custom;
    unsigned file_bytes;
    unsigned file_row;
    unsigned file_col;
    unsigned logical_stamp_y;
    unsigned logical_stamp_x;
    unsigned src_y;
    unsigned src_x;
    unsigned long numer;
    unsigned long denom;
    tga_u8 dims[2];
    tga_u8 sample[4];

    custom = params->postage_stamp_source;

    dims[0] = (tga_u8)stamp_width;
    dims[1] = (tga_u8)stamp_height;
    if (!tga__write_exact(writer, dims, sizeof(dims))) {
        return TGA_ERR_IO;
    }

    file_bytes = tga__file_sample_bytes(tga__encoded_file_bits(&params->image));
    denom = (unsigned long)stamp_width * 2ul;

    for (file_row = 0u; file_row < stamp_height; ++file_row) {
        logical_stamp_y = params->image.write_top_origin
            ? file_row
            : (stamp_height - 1u - file_row);

        if (custom == 0 || custom->pixels == 0) {
            numer = ((unsigned long)logical_stamp_y * 2ul + 1ul) * (unsigned long)params->image.height;
            src_y = (unsigned)(numer / ((unsigned long)stamp_height * 2ul));
            if (src_y >= params->image.height) {
                src_y = params->image.height - 1u;
            }
        } else {
            src_y = logical_stamp_y;
        }

        for (file_col = 0u; file_col < stamp_width; ++file_col) {
            logical_stamp_x = params->image.write_right_to_left
                ? (stamp_width - 1u - file_col)
                : file_col;

            if (custom == 0 || custom->pixels == 0) {
                numer = ((unsigned long)logical_stamp_x * 2ul + 1ul) * (unsigned long)params->image.width;
                src_x = (unsigned)(numer / denom);
                if (src_x >= params->image.width) {
                    src_x = params->image.width - 1u;
                }
                tga__encode_image_sample_at(&params->image, src_y, src_x, sample);
            } else {
                src_x = logical_stamp_x;
                tga__encode_postage_custom_sample(&params->image, custom,
                                                  src_y, src_x, sample);
            }

            if (!tga__write_exact(writer, sample, file_bytes)) {
                return TGA_ERR_IO;
            }
        }
    }

    return TGA_OK;
}

static int tga__write_header(tga_writer *writer, const tga_encode_params *params)
{
    tga_u8 hdr[18];
    unsigned image_type;
    unsigned desc;
    unsigned bits;
    unsigned palette_bits;
    unsigned first_index;
    unsigned length;

    image_type = tga__image_type_for_encode(params);
    desc = tga__descriptor_for_encode(params);
    bits = tga__encoded_file_bits(params);
    palette_bits = tga__encoded_palette_bits(params);
    first_index = params->pixel_format == TGA_PIXFMT_INDEX8 ? params->palette_first_index : 0u;
    length = params->pixel_format == TGA_PIXFMT_INDEX8 ? params->palette_count : 0u;

    memset(hdr, 0, sizeof(hdr));
    hdr[0] = (tga_u8)params->image_id_length;
    hdr[1] = (tga_u8)(params->pixel_format == TGA_PIXFMT_INDEX8 ? 1u : 0u);
    hdr[2] = (tga_u8)image_type;
    hdr[3] = (tga_u8)(first_index & 255u);
    hdr[4] = (tga_u8)((first_index >> 8) & 255u);
    hdr[5] = (tga_u8)(length & 255u);
    hdr[6] = (tga_u8)((length >> 8) & 255u);
    hdr[7] = (tga_u8)palette_bits;
    hdr[8] = 0u;
    hdr[9] = 0u;
    hdr[10] = 0u;
    hdr[11] = 0u;
    hdr[12] = (tga_u8)(params->width & 255u);
    hdr[13] = (tga_u8)((params->width >> 8) & 255u);
    hdr[14] = (tga_u8)(params->height & 255u);
    hdr[15] = (tga_u8)((params->height >> 8) & 255u);
    hdr[16] = (tga_u8)bits;
    hdr[17] = (tga_u8)desc;

    if (!tga__write_exact(writer, hdr, sizeof(hdr))) {
        return TGA_ERR_IO;
    }
    if (params->image_id_length != 0u) {
        if (!tga__write_exact(writer, params->image_id, params->image_id_length)) {
            return TGA_ERR_IO;
        }
    }

    return TGA_OK;
}

static int tga__write_palette(tga_writer *writer, const tga_encode_params *params)
{
    unsigned i;
    unsigned bytes;
    unsigned file_bits;
    tga_u8 sample[4];
    const tga_u8 *src;

    if (params->pixel_format != TGA_PIXFMT_INDEX8) {
        return TGA_OK;
    }

    file_bits = tga__encoded_palette_bits(params);
    bytes = tga__palette_bytes(params);
    for (i = 0u; i < params->palette_count; ++i) {
        src = tga__palette_entry_ptr(params, i);
        tga__encode_palette_sample(sample, file_bits, params->palette_format, src);
        if (!tga__write_exact(writer, sample, bytes)) {
            return TGA_ERR_IO;
        }
    }

    return TGA_OK;
}

static int tga__write_raw_pixels(tga_writer *writer, const tga_encode_params *params)
{
    unsigned file_bytes;
    unsigned file_row;
    unsigned file_col;
    unsigned logical_y;
    unsigned logical_x;
    tga_u8 sample[4];

    file_bytes = tga__file_sample_bytes(tga__encoded_file_bits(params));

    for (file_row = 0u; file_row < params->height; ++file_row) {
        logical_y = tga__file_row_to_logical_y(params, file_row);

        for (file_col = 0u; file_col < params->width; ++file_col) {
            logical_x = tga__file_col_to_logical_x(params, file_col);

            tga__encode_image_sample_at(params, logical_y, logical_x, sample);
            if (!tga__write_exact(writer, sample, file_bytes)) {
                return TGA_ERR_IO;
            }
        }
    }

    return TGA_OK;
}

static int tga__write_rle_pixels(tga_writer *writer, const tga_encode_params *params)
{
    unsigned file_bytes;
    unsigned file_row;
    unsigned x;
    unsigned logical_y;
    unsigned logical_x;
    unsigned run;
    unsigned raw;
    tga_u8 a[4];
    tga_u8 b[4];
    tga_u8 hdr;

    file_bytes = tga__file_sample_bytes(tga__encoded_file_bits(params));

    for (file_row = 0u; file_row < params->height; ++file_row) {
        logical_y = tga__file_row_to_logical_y(params, file_row);

        x = 0u;
        while (x < params->width) {
            logical_x = tga__file_col_to_logical_x(params, x);
            tga__encode_image_sample_at(params, logical_y, logical_x, a);

            run = 1u;
            while (x + run < params->width && run < 128u) {
                logical_x = tga__file_col_to_logical_x(params, x + run);
                tga__encode_image_sample_at(params, logical_y, logical_x, b);
                if (!tga__samples_equal(a, b, file_bytes)) {
                    break;
                }
                ++run;
            }

            if (run >= 2u) {
                hdr = (tga_u8)(0x80u | (run - 1u));
                if (!tga__write_exact(writer, &hdr, 1u)) {
                    return TGA_ERR_IO;
                }
                if (!tga__write_exact(writer, a, file_bytes)) {
                    return TGA_ERR_IO;
                }
                x += run;
                continue;
            }

            raw = 1u;
            while (x + raw < params->width && raw < 128u) {
                logical_x = tga__file_col_to_logical_x(params, x + raw);
                tga__encode_image_sample_at(params, logical_y, logical_x, a);

                run = 1u;
                while (x + raw + run < params->width && run < 128u) {
                    logical_x = tga__file_col_to_logical_x(params, x + raw + run);
                    tga__encode_image_sample_at(params, logical_y, logical_x, b);
                    if (!tga__samples_equal(a, b, file_bytes)) {
                        break;
                    }
                    ++run;
                }

                if (run >= 2u) {
                    break;
                }

                ++raw;
            }

            hdr = (tga_u8)(raw - 1u);
            if (!tga__write_exact(writer, &hdr, 1u)) {
                return TGA_ERR_IO;
            }

            while (raw > 0u) {
                logical_x = tga__file_col_to_logical_x(params, x);
                tga__encode_image_sample_at(params, logical_y, logical_x, a);
                if (!tga__write_exact(writer, a, file_bytes)) {
                    return TGA_ERR_IO;
                }
                ++x;
                --raw;
            }
        }
    }

    return TGA_OK;
}

static int tga__write_footer(tga_writer *writer)
{
    tga_u8 footer[26];

    memset(footer, 0, sizeof(footer));
    memcpy(footer + 8u, tga__footer_sig, 18u);

    if (!tga__write_exact(writer, footer, sizeof(footer))) {
        return TGA_ERR_IO;
    }
    return TGA_OK;
}

unsigned long tga_encode_max_size(const tga_encode_params *params)
{
    unsigned bits;
    unsigned bytes;
    unsigned palette_bytes;
    unsigned long total;
    unsigned long row_chunk;
    unsigned long image_bytes;
    int rc;

    if (params == 0) {
        return 0ul;
    }
    rc = tga__validate_encode_params(params);
    if (rc != TGA_OK) {
        return 0ul;
    }

    bits = tga__encoded_file_bits(params);
    bytes = tga__file_sample_bytes(bits);
    palette_bytes = tga__palette_bytes(params);

    total = 18ul + (unsigned long)params->image_id_length;
    total += (unsigned long)params->palette_count * (unsigned long)palette_bytes;

    if (params->rle) {
        row_chunk = (unsigned long)params->width * (unsigned long)bytes;
        row_chunk += ((unsigned long)params->width + 127ul) / 128ul;
        image_bytes = row_chunk * (unsigned long)params->height;
    } else {
        image_bytes = (unsigned long)params->width * (unsigned long)params->height * (unsigned long)bytes;
    }

    total += image_bytes;
    if (params->write_footer) {
        total += 26ul;
    }

    return total;
}

int tga_encode(tga_writer *writer, const tga_encode_params *params)
{
    int rc;

    rc = tga__validate_encode_params(params);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__write_header(writer, params);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__write_palette(writer, params);
    if (rc != TGA_OK) {
        return rc;
    }

    if (params->rle) {
        rc = tga__write_rle_pixels(writer, params);
    } else {
        rc = tga__write_raw_pixels(writer, params);
    }
    if (rc != TGA_OK) {
        return rc;
    }

    if (params->write_footer) {
        rc = tga__write_footer(writer);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    return TGA_OK;
}

int tga_encode_memory(void *dst, size_t dst_capacity, size_t *dst_size,
                      const tga_encode_params *params)
{
    tga_mem_writer mem;
    tga_writer writer;
    int rc;

    if (dst == 0 || dst_size == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    mem.data = (tga_u8 *)dst;
    mem.capacity = dst_capacity;
    mem.pos = 0u;
    mem.overflowed = 0;

    writer.write = tga_mem_write;
    writer.user = &mem;

    rc = tga_encode(&writer, params);
    *dst_size = mem.pos;

    if (rc == TGA_ERR_IO && mem.overflowed) {
        return TGA_ERR_CAPACITY;
    }
    return rc;
}

typedef struct tga__count_writer {
    size_t pos;
    int overflowed;
} tga__count_writer;

static size_t tga__count_write(void *user, const void *src, size_t bytes)
{
    tga__count_writer *cw;

    (void)src;

    if (user == 0) {
        return 0u;
    }

    cw = (tga__count_writer *)user;
    if (cw->pos > (size_t)-1 - bytes) {
        cw->overflowed = 1;
        return 0u;
    }

    cw->pos += bytes;
    return bytes;
}

int tga_encode_size(const tga_encode_params *params, size_t *out_size)
{
    tga__count_writer cw;
    tga_writer writer;
    int rc;

    if (params == 0 || out_size == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    cw.pos = 0u;
    cw.overflowed = 0;
    writer.write = tga__count_write;
    writer.user = &cw;

    rc = tga_encode(&writer, params);
    if (rc == TGA_ERR_IO && cw.overflowed) {
        return TGA_ERR_OVERFLOW;
    }
    if (rc != TGA_OK) {
        return rc;
    }

    *out_size = cw.pos;
    return TGA_OK;
}

size_t tga_mem_read(void *user, void *dst, size_t bytes)
{
    tga_mem_reader *mem;
    size_t remain;

    if (user == 0) {
        return 0u;
    }

    mem = (tga_mem_reader *)user;
    if (mem->pos > mem->size) {
        return 0u;
    }

    remain = mem->size - mem->pos;
    if (bytes > remain) {
        bytes = remain;
    }
    if (bytes != 0u) {
        memcpy(dst, mem->data + mem->pos, bytes);
        mem->pos += bytes;
    }
    return bytes;
}

size_t tga_mem_write(void *user, const void *src, size_t bytes)
{
    tga_mem_writer *mem;

    if (user == 0) {
        return 0u;
    }

    mem = (tga_mem_writer *)user;
    if (mem->pos > mem->capacity) {
        mem->overflowed = 1;
        return 0u;
    }
    if (bytes > mem->capacity - mem->pos) {
        mem->overflowed = 1;
        return 0u;
    }

    if (bytes != 0u) {
        memcpy(mem->data + mem->pos, src, bytes);
        mem->pos += bytes;
    }
    return bytes;
}



const char *tga_result_string(int rc)
{
    switch (rc) {
        case TGA_OK: return "TGA_OK";
        case TGA_ERR_BAD_ARGUMENT: return "TGA_ERR_BAD_ARGUMENT";
        case TGA_ERR_IO: return "TGA_ERR_IO";
        case TGA_ERR_BAD_MAGIC: return "TGA_ERR_BAD_MAGIC";
        case TGA_ERR_UNSUPPORTED: return "TGA_ERR_UNSUPPORTED";
        case TGA_ERR_TRUNCATED: return "TGA_ERR_TRUNCATED";
        case TGA_ERR_BAD_FORMAT: return "TGA_ERR_BAD_FORMAT";
        case TGA_ERR_OVERFLOW: return "TGA_ERR_OVERFLOW";
        case TGA_ERR_NEED_PALETTE_BUFFER: return "TGA_ERR_NEED_PALETTE_BUFFER";
        case TGA_ERR_PALETTE_RANGE: return "TGA_ERR_PALETTE_RANGE";
        case TGA_ERR_CAPACITY: return "TGA_ERR_CAPACITY";
        case TGA_ERR_NO_FOOTER: return "TGA_ERR_NO_FOOTER";
        case TGA_ERR_NO_EXTENSION: return "TGA_ERR_NO_EXTENSION";
        case TGA_ERR_NO_DEVELOPER_AREA: return "TGA_ERR_NO_DEVELOPER_AREA";
        case TGA_ERR_TOO_MANY_TAGS: return "TGA_ERR_TOO_MANY_TAGS";
        case TGA_ERR_NOT_FOUND: return "TGA_ERR_NOT_FOUND";
        case TGA_ERR_NO_SCAN_LINE_TABLE: return "TGA_ERR_NO_SCAN_LINE_TABLE";
        case TGA_ERR_NO_POSTAGE_STAMP: return "TGA_ERR_NO_POSTAGE_STAMP";
        case TGA_ERR_NO_COLOR_CORRECTION_TABLE: return "TGA_ERR_NO_COLOR_CORRECTION_TABLE";
        case TGA_ERR_INDEX_RANGE: return "TGA_ERR_INDEX_RANGE";
        default: return "TGA_ERR_UNKNOWN";
    }
}

const char *tga_section_kind_string(unsigned kind)
{
    switch (kind) {
        case TGA_SECTION_HEADER: return "TGA_SECTION_HEADER";
        case TGA_SECTION_IMAGE_ID: return "TGA_SECTION_IMAGE_ID";
        case TGA_SECTION_COLOR_MAP: return "TGA_SECTION_COLOR_MAP";
        case TGA_SECTION_IMAGE_DATA: return "TGA_SECTION_IMAGE_DATA";
        case TGA_SECTION_EXTENSION_AREA: return "TGA_SECTION_EXTENSION_AREA";
        case TGA_SECTION_COLOR_CORRECTION_TABLE: return "TGA_SECTION_COLOR_CORRECTION_TABLE";
        case TGA_SECTION_POSTAGE_STAMP: return "TGA_SECTION_POSTAGE_STAMP";
        case TGA_SECTION_SCAN_LINE_TABLE: return "TGA_SECTION_SCAN_LINE_TABLE";
        case TGA_SECTION_DEVELOPER_DIRECTORY: return "TGA_SECTION_DEVELOPER_DIRECTORY";
        case TGA_SECTION_DEVELOPER_FIELD: return "TGA_SECTION_DEVELOPER_FIELD";
        case TGA_SECTION_FOOTER: return "TGA_SECTION_FOOTER";
        default: return "TGA_SECTION_UNKNOWN";
    }
}

const char *tga_attributes_type_string(unsigned attributes_type)
{
    switch (attributes_type) {
        case TGA_ATTRIBUTES_TYPE_NONE:
            return "TGA_ATTRIBUTES_TYPE_NONE";
        case TGA_ATTRIBUTES_TYPE_UNDEFINED_IGNORE:
            return "TGA_ATTRIBUTES_TYPE_UNDEFINED_IGNORE";
        case TGA_ATTRIBUTES_TYPE_UNDEFINED_RETAIN:
            return "TGA_ATTRIBUTES_TYPE_UNDEFINED_RETAIN";
        case TGA_ATTRIBUTES_TYPE_ALPHA:
            return "TGA_ATTRIBUTES_TYPE_ALPHA";
        case TGA_ATTRIBUTES_TYPE_PREMULTIPLIED:
            return "TGA_ATTRIBUTES_TYPE_PREMULTIPLIED";
        default:
            return "TGA_ATTRIBUTES_TYPE_UNKNOWN";
    }
}

static tga_u16 tga__read_u16_le(const tga_u8 *src)
{
    return (tga_u16)((unsigned)src[0] | ((unsigned)src[1] << 8));
}

static tga_u32 tga__read_u32_le(const tga_u8 *src)
{
    return (tga_u32)((tga_u32)src[0] |
                     ((tga_u32)src[1] << 8) |
                     ((tga_u32)src[2] << 16) |
                     ((tga_u32)src[3] << 24));
}

static void tga__write_u16_le(tga_u8 *dst, unsigned value)
{
    dst[0] = (tga_u8)(value & 255u);
    dst[1] = (tga_u8)((value >> 8) & 255u);
}

static void tga__write_u32_le(tga_u8 *dst, tga_u32 value)
{
    dst[0] = (tga_u8)(value & 255ul);
    dst[1] = (tga_u8)((value >> 8) & 255ul);
    dst[2] = (tga_u8)((value >> 16) & 255ul);
    dst[3] = (tga_u8)((value >> 24) & 255ul);
}

static tga_u32 tga__gcd_u32(tga_u32 a, tga_u32 b)
{
    while (b != 0ul) {
        tga_u32 t = a % b;
        a = b;
        b = t;
    }
    return a;
}

static int tga__ratio_to_q16(unsigned numerator,
                             unsigned denominator,
                             tga_u32 *out_q16)
{
    tga_u32 scaled;

    if (out_q16 == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (numerator == 0u && denominator == 0u) {
        *out_q16 = 0ul;
        return TGA_OK;
    }
    if (numerator == 0u || denominator == 0u) {
        return TGA_ERR_BAD_FORMAT;
    }

    scaled = (((tga_u32)numerator << 16) + ((tga_u32)denominator / 2ul)) /
             (tga_u32)denominator;
    *out_q16 = scaled;
    return TGA_OK;
}

static void tga__q16_to_ratio(tga_u32 q16,
                              unsigned *out_numerator,
                              unsigned *out_denominator)
{
    tga_u32 num;
    tga_u32 den;
    tga_u32 g;

    if (out_numerator == 0 || out_denominator == 0) {
        return;
    }
    if (q16 == 0ul) {
        *out_numerator = 0u;
        *out_denominator = 0u;
        return;
    }

    num = q16;
    den = 65536ul;
    g = tga__gcd_u32(num, den);
    if (g != 0ul) {
        num /= g;
        den /= g;
    }

    while (num > 65535ul || den > 65535ul) {
        num = (num + 1ul) >> 1;
        den = (den + 1ul) >> 1;
    }

    g = tga__gcd_u32(num, den);
    if (g != 0ul) {
        num /= g;
        den /= g;
    }

    if (num == 0ul) {
        num = 1ul;
    }
    if (den == 0ul) {
        den = 1ul;
    }

    *out_numerator = (unsigned)num;
    *out_denominator = (unsigned)den;
}

static int tga__extension_patch_memory(void *src,
                                       size_t src_size,
                                       size_t field_offset,
                                       const void *patch,
                                       size_t patch_size)
{
    tga_u8 *bytes;
    tga_footer footer;
    size_t abs_off;
    int rc;

    if (src == 0 || patch == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga_read_footer_memory(src, src_size, &footer);
    if (rc != TGA_OK) {
        return rc;
    }
    if (!footer.present || footer.extension_offset == 0ul) {
        return TGA_ERR_NO_EXTENSION;
    }

    abs_off = (size_t)footer.extension_offset + field_offset;
    if (!tga__region_fits(abs_off, patch_size, src_size)) {
        return TGA_ERR_TRUNCATED;
    }

    bytes = (tga_u8 *)src;
    memcpy(bytes + abs_off, patch, patch_size);
    return TGA_OK;
}

#ifndef GAFNYF_TGA_NO_STDIO
static int tga__extension_patch_file(FILE *fp,
                                     size_t field_offset,
                                     const void *patch,
                                     size_t patch_size)
{
    tga_footer footer;
    long saved_pos;
    size_t abs_off;
    int rc;

    if (fp == 0 || patch == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    rc = tga_read_footer_file(fp, &footer);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }
    if (!footer.present || footer.extension_offset == 0ul) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_NO_EXTENSION;
    }

    abs_off = (size_t)footer.extension_offset + field_offset;
    if (fseek(fp, (long)abs_off, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if (fwrite(patch, 1u, patch_size, fp) != patch_size) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if (fflush(fp) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if (fseek(fp, saved_pos, SEEK_SET) != 0) {
        return TGA_ERR_IO;
    }
    return TGA_OK;
}
#endif

static int tga__parse_footer_bytes(const tga_u8 footer[26], tga_footer *out_footer)
{
    if (memcmp(footer + 8u, tga__footer_sig, 18u) != 0) {
        if (out_footer != 0) {
            memset(out_footer, 0, sizeof(*out_footer));
        }
        return TGA_ERR_NO_FOOTER;
    }

    if (out_footer != 0) {
        out_footer->present = 1;
        out_footer->extension_offset = tga__read_u32_le(footer + 0u);
        out_footer->developer_offset = tga__read_u32_le(footer + 4u);
    }
    return TGA_OK;
}

static void tga__read_fixed_string(const tga_u8 *src, unsigned field_len,
                                   char *dst, unsigned dst_cap)
{
    unsigned i;

    if (dst == 0 || dst_cap == 0u) {
        return;
    }

    i = 0u;
    while (i + 1u < dst_cap && i < field_len && src[i] != 0u) {
        dst[i] = (char)src[i];
        ++i;
    }
    dst[i] = '\0';
}

static void tga__write_fixed_string(tga_u8 *dst, unsigned field_len, const char *src)
{
    unsigned i;

    for (i = 0u; i < field_len; ++i) {
        dst[i] = (tga_u8)' ';
    }

    if (field_len == 0u) {
        return;
    }

    if (src == 0) {
        dst[0] = 0u;
        return;
    }

    i = 0u;
    while (i + 1u < field_len && src[i] != '\0') {
        dst[i] = (tga_u8)src[i];
        ++i;
    }
    dst[i] = 0u;
}

static void tga__parse_extension_bytes(const tga_u8 *ext, tga_extension *out_extension)
{
    memset(out_extension, 0, sizeof(*out_extension));

    out_extension->size = (unsigned)tga__read_u16_le(ext + 0u);
    tga__read_fixed_string(ext + 2u, 41u,
                           out_extension->author_name,
                           (unsigned)sizeof(out_extension->author_name));
    tga__read_fixed_string(ext + 43u, 324u,
                           out_extension->author_comment,
                           (unsigned)sizeof(out_extension->author_comment));

    out_extension->stamp_month = (unsigned)tga__read_u16_le(ext + 367u);
    out_extension->stamp_day = (unsigned)tga__read_u16_le(ext + 369u);
    out_extension->stamp_year = (unsigned)tga__read_u16_le(ext + 371u);
    out_extension->stamp_hour = (unsigned)tga__read_u16_le(ext + 373u);
    out_extension->stamp_minute = (unsigned)tga__read_u16_le(ext + 375u);
    out_extension->stamp_second = (unsigned)tga__read_u16_le(ext + 377u);

    tga__read_fixed_string(ext + 379u, 41u,
                           out_extension->job_name,
                           (unsigned)sizeof(out_extension->job_name));
    out_extension->job_hour = (unsigned)tga__read_u16_le(ext + 420u);
    out_extension->job_minute = (unsigned)tga__read_u16_le(ext + 422u);
    out_extension->job_second = (unsigned)tga__read_u16_le(ext + 424u);

    tga__read_fixed_string(ext + 426u, 41u,
                           out_extension->software_id,
                           (unsigned)sizeof(out_extension->software_id));
    out_extension->software_version_number = (unsigned)tga__read_u16_le(ext + 467u);
    out_extension->software_version_letter = (unsigned)ext[469u];
    out_extension->key_color = tga__read_u32_le(ext + 470u);
    out_extension->pixel_numerator = (unsigned)tga__read_u16_le(ext + 474u);
    out_extension->pixel_denominator = (unsigned)tga__read_u16_le(ext + 476u);
    out_extension->gamma_numerator = (unsigned)tga__read_u16_le(ext + 478u);
    out_extension->gamma_denominator = (unsigned)tga__read_u16_le(ext + 480u);
    out_extension->color_correction_offset = tga__read_u32_le(ext + 482u);
    out_extension->postage_stamp_offset = tga__read_u32_le(ext + 486u);
    out_extension->scan_line_offset = tga__read_u32_le(ext + 490u);
    out_extension->attributes_type = (unsigned)ext[494u];
}

static void tga__build_extension_bytes(tga_u8 out_ext[495], const tga_extension *extension)
{
    memset(out_ext, 0, 495u);
    tga__write_u16_le(out_ext + 0u, 495u);

    if (extension == 0) {
        return;
    }

    tga__write_fixed_string(out_ext + 2u, 41u, extension->author_name);
    tga__write_fixed_string(out_ext + 43u, 324u, extension->author_comment);

    tga__write_u16_le(out_ext + 367u, extension->stamp_month);
    tga__write_u16_le(out_ext + 369u, extension->stamp_day);
    tga__write_u16_le(out_ext + 371u, extension->stamp_year);
    tga__write_u16_le(out_ext + 373u, extension->stamp_hour);
    tga__write_u16_le(out_ext + 375u, extension->stamp_minute);
    tga__write_u16_le(out_ext + 377u, extension->stamp_second);

    tga__write_fixed_string(out_ext + 379u, 41u, extension->job_name);
    tga__write_u16_le(out_ext + 420u, extension->job_hour);
    tga__write_u16_le(out_ext + 422u, extension->job_minute);
    tga__write_u16_le(out_ext + 424u, extension->job_second);

    tga__write_fixed_string(out_ext + 426u, 41u, extension->software_id);
    tga__write_u16_le(out_ext + 467u, extension->software_version_number);
    out_ext[469u] = (tga_u8)extension->software_version_letter;
    tga__write_u32_le(out_ext + 470u, extension->key_color);
    tga__write_u16_le(out_ext + 474u, extension->pixel_numerator);
    tga__write_u16_le(out_ext + 476u, extension->pixel_denominator);
    tga__write_u16_le(out_ext + 478u, extension->gamma_numerator);
    tga__write_u16_le(out_ext + 480u, extension->gamma_denominator);
    tga__write_u32_le(out_ext + 482u, extension->color_correction_offset);
    tga__write_u32_le(out_ext + 486u, extension->postage_stamp_offset);
    tga__write_u32_le(out_ext + 490u, extension->scan_line_offset);
    out_ext[494u] = (tga_u8)extension->attributes_type;
}

static void tga__build_extension_bytes_ex(tga_u8 out_ext[495],
                                          const tga_extension *extension,
                                          tga_u32 color_correction_offset,
                                          tga_u32 postage_stamp_offset,
                                          tga_u32 scan_line_offset)
{
    tga__build_extension_bytes(out_ext, extension);
    tga__write_u32_le(out_ext + 482u, color_correction_offset);
    tga__write_u32_le(out_ext + 486u, postage_stamp_offset);
    tga__write_u32_le(out_ext + 490u, scan_line_offset);
}

int tga_read_footer_memory(const void *src, size_t src_size, tga_footer *out_footer)
{
    const tga_u8 *bytes;

    if (src == 0 || out_footer == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_footer, 0, sizeof(*out_footer));

    if (src_size < 26u) {
        return TGA_ERR_NO_FOOTER;
    }

    bytes = (const tga_u8 *)src;
    return tga__parse_footer_bytes(bytes + src_size - 26u, out_footer);
}

int tga_read_extension_memory(const void *src, size_t src_size, tga_extension *out_extension)
{
    const tga_u8 *bytes;
    const tga_u8 *ext;
    tga_footer footer;
    int rc;
    unsigned size;

    if (src == 0 || out_extension == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga_read_footer_memory(src, src_size, &footer);
    if (rc != TGA_OK) {
        return rc;
    }

    if (footer.extension_offset == 0ul) {
        return TGA_ERR_NO_EXTENSION;
    }

    bytes = (const tga_u8 *)src;
    if ((size_t)footer.extension_offset + 2u > src_size) {
        return TGA_ERR_TRUNCATED;
    }

    ext = bytes + (size_t)footer.extension_offset;
    size = (unsigned)tga__read_u16_le(ext + 0u);
    if (size < 495u) {
        return TGA_ERR_BAD_FORMAT;
    }
    if ((size_t)footer.extension_offset + (size_t)size > src_size) {
        return TGA_ERR_TRUNCATED;
    }

    tga__parse_extension_bytes(ext, out_extension);
    return TGA_OK;
}

int tga_extension_pixel_aspect_q16(const tga_extension *extension, tga_u32 *out_q16)
{
    if (extension == 0 || out_q16 == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    return tga__ratio_to_q16(extension->pixel_numerator,
                             extension->pixel_denominator,
                             out_q16);
}

int tga_extension_gamma_q16(const tga_extension *extension, tga_u32 *out_q16)
{
    if (extension == 0 || out_q16 == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    return tga__ratio_to_q16(extension->gamma_numerator,
                             extension->gamma_denominator,
                             out_q16);
}

int tga_extension_set_pixel_aspect_q16(tga_extension *extension, tga_u32 q16)
{
    unsigned num;
    unsigned den;

    if (extension == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    tga__q16_to_ratio(q16, &num, &den);
    extension->pixel_numerator = num;
    extension->pixel_denominator = den;
    return TGA_OK;
}

int tga_extension_set_gamma_q16(tga_extension *extension, tga_u32 q16)
{
    unsigned num;
    unsigned den;

    if (extension == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    tga__q16_to_ratio(q16, &num, &den);
    extension->gamma_numerator = num;
    extension->gamma_denominator = den;
    return TGA_OK;
}

int tga_patch_extension_attributes_type_memory(void *src,
                                               size_t src_size,
                                               unsigned attributes_type)
{
    tga_u8 value;

    if (attributes_type > 255u) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    value = (tga_u8)attributes_type;
    return tga__extension_patch_memory(src, src_size, 494u, &value, 1u);
}

int tga_patch_extension_pixel_aspect_q16_memory(void *src,
                                                 size_t src_size,
                                                 tga_u32 q16)
{
    tga_u8 patch[4];
    unsigned num;
    unsigned den;

    tga__q16_to_ratio(q16, &num, &den);
    tga__write_u16_le(patch + 0u, num);
    tga__write_u16_le(patch + 2u, den);
    return tga__extension_patch_memory(src, src_size, 474u, patch, sizeof(patch));
}

int tga_patch_extension_gamma_q16_memory(void *src,
                                         size_t src_size,
                                         tga_u32 q16)
{
    tga_u8 patch[4];
    unsigned num;
    unsigned den;

    tga__q16_to_ratio(q16, &num, &den);
    tga__write_u16_le(patch + 0u, num);
    tga__write_u16_le(patch + 2u, den);
    return tga__extension_patch_memory(src, src_size, 478u, patch, sizeof(patch));
}

int tga_read_developer_directory_memory(const void *src, size_t src_size,
                                        tga_developer_tag *tags,
                                        unsigned tag_capacity,
                                        unsigned *out_tag_count)
{
    const tga_u8 *bytes;
    const tga_u8 *dir;
    tga_footer footer;
    int rc;
    unsigned count;
    unsigned i;
    size_t need;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    if (out_tag_count != 0) {
        *out_tag_count = 0u;
    }

    rc = tga_read_footer_memory(src, src_size, &footer);
    if (rc != TGA_OK) {
        return rc;
    }

    if (footer.developer_offset == 0ul) {
        return TGA_ERR_NO_DEVELOPER_AREA;
    }

    bytes = (const tga_u8 *)src;
    if ((size_t)footer.developer_offset + 2u > src_size) {
        return TGA_ERR_TRUNCATED;
    }

    dir = bytes + (size_t)footer.developer_offset;
    count = (unsigned)tga__read_u16_le(dir + 0u);
    need = (size_t)footer.developer_offset + 2u + (size_t)count * 10u;
    if (need > src_size) {
        return TGA_ERR_TRUNCATED;
    }

    if (out_tag_count != 0) {
        *out_tag_count = count;
    }

    if (tags == 0 || tag_capacity == 0u) {
        return TGA_OK;
    }

    if (tag_capacity < count) {
        return TGA_ERR_TOO_MANY_TAGS;
    }

    for (i = 0u; i < count; ++i) {
        const tga_u8 *entry = dir + 2u + (size_t)i * 10u;
        tags[i].tag = (unsigned)tga__read_u16_le(entry + 0u);
        tags[i].data_offset = tga__read_u32_le(entry + 2u);
        tags[i].data_size = tga__read_u32_le(entry + 6u);
    }

    return TGA_OK;
}

int tga_read_scan_line_table_memory(const void *src, size_t src_size,
                                    tga_u32 *offsets,
                                    unsigned offset_capacity,
                                    unsigned *out_offset_count)
{
    const tga_u8 *bytes;
    tga_extension ext;
    tga_info info;
    int rc;
    size_t need;
    unsigned i;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_offset_count != 0) {
        *out_offset_count = 0u;
    }

    rc = tga_read_extension_memory(src, src_size, &ext);
    if (rc != TGA_OK) {
        return rc;
    }
    if (ext.scan_line_offset == 0ul) {
        return TGA_ERR_NO_SCAN_LINE_TABLE;
    }

    rc = tga_probe_memory(src, src_size, &info);
    if (rc != TGA_OK) {
        return rc;
    }

    need = (size_t)ext.scan_line_offset + (size_t)info.height * 4u;
    if (need > src_size) {
        return TGA_ERR_TRUNCATED;
    }

    if (out_offset_count != 0) {
        *out_offset_count = info.height;
    }

    if (offsets == 0 || offset_capacity == 0u) {
        return TGA_OK;
    }
    if (offset_capacity < info.height) {
        return TGA_ERR_CAPACITY;
    }

    bytes = (const tga_u8 *)src + (size_t)ext.scan_line_offset;
    for (i = 0u; i < info.height; ++i) {
        offsets[i] = tga__read_u32_le(bytes + (size_t)i * 4u);
    }

    return TGA_OK;
}

int tga_read_color_correction_table_memory(const void *src, size_t src_size,
                                           tga_color_correction_entry *table,
                                           unsigned table_capacity)
{
    const tga_u8 *bytes;
    tga_extension ext;
    size_t need;
    int rc;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga_read_extension_memory(src, src_size, &ext);
    if (rc != TGA_OK) {
        return rc;
    }
    if (ext.color_correction_offset == 0ul) {
        return TGA_ERR_NO_COLOR_CORRECTION_TABLE;
    }

    need = (size_t)ext.color_correction_offset +
           (size_t)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8u;
    if (need > src_size) {
        return TGA_ERR_TRUNCATED;
    }

    if (table == 0 || table_capacity == 0u) {
        return TGA_OK;
    }
    if (table_capacity < TGA_COLOR_CORRECTION_ENTRY_COUNT) {
        return TGA_ERR_CAPACITY;
    }

    bytes = (const tga_u8 *)src + (size_t)ext.color_correction_offset;
    tga__parse_color_correction_table(bytes, table);
    return TGA_OK;
}

int tga_read_postage_stamp_info_memory(const void *src, size_t src_size,
                                       tga_postage_stamp_info *out_info)
{
    const tga_u8 *bytes;
    const tga_u8 *stamp;
    tga_extension ext;
    tga_info info;
    unsigned width;
    unsigned height;
    unsigned sample_bytes;
    size_t data_size;
    size_t need;
    int rc;

    if (src == 0 || out_info == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga_probe_memory(src, src_size, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__validate_decode_info(&info);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga_read_extension_memory(src, src_size, &ext);
    if (rc != TGA_OK) {
        return rc;
    }
    if (ext.postage_stamp_offset == 0ul) {
        return TGA_ERR_NO_POSTAGE_STAMP;
    }

    if ((size_t)ext.postage_stamp_offset + 2u > src_size) {
        return TGA_ERR_TRUNCATED;
    }

    bytes = (const tga_u8 *)src;
    stamp = bytes + (size_t)ext.postage_stamp_offset;
    width = (unsigned)stamp[0];
    height = (unsigned)stamp[1];
    if (width == 0u || height == 0u) {
        return TGA_ERR_BAD_FORMAT;
    }

    sample_bytes = tga__file_sample_bytes(info.pixel_depth);
    data_size = (size_t)width * (size_t)height * (size_t)sample_bytes;
    need = (size_t)ext.postage_stamp_offset + 2u;
    if (data_size > src_size - need) {
        return TGA_ERR_TRUNCATED;
    }

    tga__fill_postage_stamp_info(&info, width, height, out_info);
    return TGA_OK;
}

int tga_read_postage_stamp_memory(const void *src, size_t src_size,
                                  void *dst,
                                  size_t dst_capacity,
                                  size_t *out_stamp_size,
                                  tga_postage_stamp_info *out_info)
{
    const tga_u8 *bytes;
    tga_extension ext;
    tga_postage_stamp_info info;
    int rc;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_stamp_size != 0) {
        *out_stamp_size = 0u;
    }

    rc = tga_read_postage_stamp_info_memory(src, src_size, &info);
    if (rc != TGA_OK) {
        return rc;
    }

    if (out_stamp_size != 0) {
        *out_stamp_size = info.data_size;
    }
    if (out_info != 0) {
        *out_info = info;
    }

    if (dst == 0 || dst_capacity == 0u) {
        return TGA_OK;
    }
    if (dst_capacity < info.data_size) {
        return TGA_ERR_CAPACITY;
    }

    rc = tga_read_extension_memory(src, src_size, &ext);
    if (rc != TGA_OK) {
        return rc;
    }

    bytes = (const tga_u8 *)src + (size_t)ext.postage_stamp_offset + 2u;
    if (info.data_size != 0u) {
        memcpy(dst, bytes, info.data_size);
    }
    return TGA_OK;
}

int tga_decode_postage_stamp_memory(const void *src, size_t src_size,
                                    const tga_decode_params *params,
                                    tga_info *out_info)
{
    const tga_u8 *bytes;
    tga_info info;
    tga_info stamp_info;
    tga_extension ext;
    tga_postage_stamp_info stamp;
    tga_mem_reader palette_mem;
    tga_mem_reader stamp_mem;
    tga_reader reader;
    int rc;
    unsigned out_bpp;
    unsigned long min_stride;

    if (src == 0 || params == 0 || params->pixels == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__pixel_format_valid(params->pixel_format) ||
        params->pixel_format == TGA_PIXFMT_INDEX8) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    bytes = (const tga_u8 *)src;
    rc = tga_probe_memory(src, src_size, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__validate_decode_info(&info);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga_read_postage_stamp_info_memory(src, src_size, &stamp);
    if (rc != TGA_OK) {
        return rc;
    }

    out_bpp = tga_bytes_per_pixel(params->pixel_format);
    min_stride = (unsigned long)stamp.width * (unsigned long)out_bpp;
    if (params->stride_bytes < min_stride) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    palette_mem.data = bytes + 18u;
    palette_mem.size = src_size - 18u;
    palette_mem.pos = 0u;
    reader.read = tga_mem_read;
    reader.user = &palette_mem;

    if (!tga__skip(&reader, (size_t)info.id_length)) {
        return TGA_ERR_TRUNCATED;
    }
    rc = tga__load_palette(&reader, &info,
                           params->palette_rgba,
                           params->palette_capacity);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga_read_extension_memory(src, src_size, &ext);
    if (rc != TGA_OK) {
        return rc;
    }

    stamp_mem.data = bytes + (size_t)ext.postage_stamp_offset + 2u;
    stamp_mem.size = stamp.data_size;
    stamp_mem.pos = 0u;
    reader.user = &stamp_mem;

    rc = tga__decode_uncompressed_pixels_from_reader(&reader, &info,
                                                     stamp.width, stamp.height,
                                                     params);
    if (rc != TGA_OK) {
        return rc;
    }

    if (out_info != 0) {
        stamp_info = info;
        stamp_info.width = stamp.width;
        stamp_info.height = stamp.height;
        stamp_info.image_type = tga__raw_image_type(info.image_type);
        stamp_info.is_rle = 0;
        *out_info = stamp_info;
    }

    return TGA_OK;
}

int tga_find_developer_tag_memory(const void *src, size_t src_size,
                                  unsigned tag,
                                  tga_developer_tag *out_tag)
{
    const tga_u8 *bytes;
    const tga_u8 *dir;
    tga_footer footer;
    size_t need;
    unsigned count;
    unsigned i;
    int rc;

    if (src == 0 || out_tag == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_tag, 0, sizeof(*out_tag));

    rc = tga_read_footer_memory(src, src_size, &footer);
    if (rc != TGA_OK) {
        return rc;
    }
    if (footer.developer_offset == 0ul) {
        return TGA_ERR_NO_DEVELOPER_AREA;
    }

    bytes = (const tga_u8 *)src;
    if ((size_t)footer.developer_offset + 2u > src_size) {
        return TGA_ERR_TRUNCATED;
    }

    dir = bytes + (size_t)footer.developer_offset;
    count = (unsigned)tga__read_u16_le(dir + 0u);
    need = (size_t)footer.developer_offset + 2u + (size_t)count * 10u;
    if (need > src_size) {
        return TGA_ERR_TRUNCATED;
    }

    for (i = 0u; i < count; ++i) {
        const tga_u8 *entry;
        tga_u32 data_offset;
        tga_u32 data_size;
        size_t end;

        entry = dir + 2u + (size_t)i * 10u;
        if ((unsigned)tga__read_u16_le(entry + 0u) != tag) {
            continue;
        }

        data_offset = tga__read_u32_le(entry + 2u);
        data_size = tga__read_u32_le(entry + 6u);
        end = (size_t)data_offset + (size_t)data_size;
        if (end < (size_t)data_offset || end > src_size) {
            return TGA_ERR_TRUNCATED;
        }

        out_tag->tag = tag;
        out_tag->data_offset = data_offset;
        out_tag->data_size = data_size;
        return TGA_OK;
    }

    return TGA_ERR_NOT_FOUND;
}

int tga_read_developer_field_memory(const void *src, size_t src_size,
                                    const tga_developer_tag *tag,
                                    void *dst,
                                    size_t dst_capacity,
                                    size_t *out_field_size)
{
    const tga_u8 *bytes;
    size_t start;
    size_t size;
    size_t end;

    if (src == 0 || tag == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    start = (size_t)tag->data_offset;
    size = (size_t)tag->data_size;
    end = start + size;
    if (end < start || end > src_size) {
        return TGA_ERR_TRUNCATED;
    }

    if (out_field_size != 0) {
        *out_field_size = size;
    }

    if (dst == 0 || dst_capacity == 0u) {
        return size == 0u ? TGA_OK : (dst == 0 && dst_capacity == 0u ? TGA_OK : TGA_ERR_CAPACITY);
    }
    if (dst_capacity < size) {
        return TGA_ERR_CAPACITY;
    }

    bytes = (const tga_u8 *)src;
    if (size != 0u) {
        memcpy(dst, bytes + start, size);
    }
    return TGA_OK;
}

int tga_read_developer_tag_memory(const void *src, size_t src_size,
                                  unsigned tag,
                                  void *dst,
                                  size_t dst_capacity,
                                  size_t *out_field_size)
{
    tga_developer_tag info;
    int rc;

    rc = tga_find_developer_tag_memory(src, src_size, tag, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga_read_developer_field_memory(src, src_size, &info,
                                           dst, dst_capacity, out_field_size);
}

int tga_read_image_id_memory(const void *src, size_t src_size,
                             void *dst,
                             size_t dst_capacity,
                             size_t *out_id_size)
{
    tga_info info;
    int rc;

    rc = tga_probe_memory(src, src_size, &info);
    if (rc != TGA_OK) {
        return rc;
    }

    return tga__read_section_memory(src, src_size,
                                    18u,
                                    (size_t)info.id_length,
                                    dst,
                                    dst_capacity,
                                    out_id_size);
}

int tga_read_color_map_memory(const void *src, size_t src_size,
                              void *dst,
                              size_t dst_capacity,
                              size_t *out_color_map_size)
{
    tga_info info;
    size_t color_map_size;
    size_t offset;
    int rc;

    rc = tga_probe_memory(src, src_size, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    if (!tga__color_map_size_from_info(&info, &color_map_size)) {
        return TGA_ERR_OVERFLOW;
    }
    offset = 18u + (size_t)info.id_length;
    return tga__read_section_memory(src, src_size,
                                    offset,
                                    color_map_size,
                                    dst,
                                    dst_capacity,
                                    out_color_map_size);
}

int tga_read_image_data_memory(const void *src, size_t src_size,
                               void *dst,
                               size_t dst_capacity,
                               size_t *out_image_data_size)
{
    const tga_u8 *bytes;
    tga_info info;
    size_t offset;
    size_t size;
    int rc;

    rc = tga_probe_memory(src, src_size, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__validate_decode_info(&info);
    if (rc != TGA_OK) {
        return rc;
    }
    if (!tga__image_data_offset_from_info(&info, &offset)) {
        return TGA_ERR_OVERFLOW;
    }

    bytes = (const tga_u8 *)src;
    rc = tga__measure_image_data_size_memory(bytes, src_size, &info, offset, &size);
    if (rc != TGA_OK) {
        return rc;
    }

    return tga__read_section_memory(src, src_size,
                                    offset,
                                    size,
                                    dst,
                                    dst_capacity,
                                    out_image_data_size);
}

int tga_inspect_memory(const void *src, size_t src_size, tga_inspect *out_inspect)
{
    const tga_u8 *bytes;
    const tga_u8 *dir;
    tga_info info;
    tga_footer footer;
    size_t color_map_size;
    size_t image_data_offset;
    size_t image_data_size;
    size_t ext_size;
    size_t dir_size;
    size_t tag_bytes;
    size_t min_payload;
    size_t max_payload;
    size_t field_offset;
    size_t field_size;
    unsigned count;
    unsigned i;
    int rc;

    if (src == 0 || out_inspect == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_inspect, 0, sizeof(*out_inspect));
    out_inspect->file_size = src_size;
    bytes = (const tga_u8 *)src;

    rc = tga_probe_memory(src, src_size, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    out_inspect->info = info;

    out_inspect->image_id_offset = 18u;
    out_inspect->image_id_size = (size_t)info.id_length;
    if (!tga__region_fits(out_inspect->image_id_offset,
                          out_inspect->image_id_size,
                          src_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__color_map_size_from_info(&info, &color_map_size)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->color_map_offset = out_inspect->image_id_offset + out_inspect->image_id_size;
    out_inspect->color_map_size = color_map_size;
    if (!tga__region_fits(out_inspect->color_map_offset,
                          out_inspect->color_map_size,
                          src_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__image_data_offset_from_info(&info, &image_data_offset)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->image_data_offset = image_data_offset;
    if (!tga__region_fits(image_data_offset, 0u, src_size)) {
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__validate_decode_info(&info);
    if (rc == TGA_OK) {
        rc = tga__measure_image_data_size_memory(bytes, src_size, &info,
                                                 image_data_offset,
                                                 &image_data_size);
        if (rc != TGA_OK) {
            return rc;
        }
        out_inspect->image_data_size = image_data_size;
    }

    rc = tga_read_footer_memory(src, src_size, &footer);
    if (rc == TGA_OK) {
        out_inspect->footer = footer;
        out_inspect->is_tga2 = 1;
        out_inspect->footer_offset = src_size - 26u;
        out_inspect->footer_size = 26u;
    } else if (rc != TGA_ERR_NO_FOOTER) {
        return rc;
    }

    if (out_inspect->is_tga2 && footer.extension_offset != 0ul) {
        if (!tga__region_fits((size_t)footer.extension_offset, 2u, src_size)) {
            return TGA_ERR_TRUNCATED;
        }
        ext_size = (size_t)tga__read_u16_le(bytes + (size_t)footer.extension_offset);
        if (ext_size < 495u) {
            return TGA_ERR_BAD_FORMAT;
        }
        if (!tga__region_fits((size_t)footer.extension_offset, ext_size, src_size)) {
            return TGA_ERR_TRUNCATED;
        }

        out_inspect->has_extension = 1;
        out_inspect->extension_area_offset = (size_t)footer.extension_offset;
        out_inspect->extension_area_size = ext_size;
        tga__parse_extension_bytes(bytes + (size_t)footer.extension_offset,
                                   &out_inspect->extension);

        if (out_inspect->extension.scan_line_offset != 0ul) {
            if (!tga__checked_mul_size((size_t)info.height, 4u, &out_inspect->scan_line_table_size)) {
                return TGA_ERR_OVERFLOW;
            }
            out_inspect->scan_line_table_offset = (size_t)out_inspect->extension.scan_line_offset;
            if (!tga__region_fits(out_inspect->scan_line_table_offset,
                                  out_inspect->scan_line_table_size,
                                  src_size)) {
                return TGA_ERR_TRUNCATED;
            }
            out_inspect->has_scan_line_table = 1;
        }

        if (out_inspect->extension.postage_stamp_offset != 0ul) {
            rc = tga_read_postage_stamp_info_memory(src, src_size,
                                                    &out_inspect->postage_stamp_info);
            if (rc != TGA_OK) {
                return rc;
            }
            out_inspect->postage_stamp_offset =
                (size_t)out_inspect->extension.postage_stamp_offset;
            out_inspect->postage_stamp_size = out_inspect->postage_stamp_info.block_size;
            out_inspect->has_postage_stamp = 1;
        }

        if (out_inspect->extension.color_correction_offset != 0ul) {
            out_inspect->color_correction_offset =
                (size_t)out_inspect->extension.color_correction_offset;
            out_inspect->color_correction_size =
                (size_t)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8u;
            if (!tga__region_fits(out_inspect->color_correction_offset,
                                  out_inspect->color_correction_size,
                                  src_size)) {
                return TGA_ERR_TRUNCATED;
            }
            out_inspect->has_color_correction_table = 1;
        }
    }

    if (out_inspect->is_tga2 && footer.developer_offset != 0ul) {
        if (!tga__region_fits((size_t)footer.developer_offset, 2u, src_size)) {
            return TGA_ERR_TRUNCATED;
        }
        dir = bytes + (size_t)footer.developer_offset;
        count = (unsigned)tga__read_u16_le(dir + 0u);
        if (!tga__checked_mul_size((size_t)count, 10u, &tag_bytes)) {
            return TGA_ERR_OVERFLOW;
        }
        dir_size = 2u + tag_bytes;
        if (!tga__region_fits((size_t)footer.developer_offset, dir_size, src_size)) {
            return TGA_ERR_TRUNCATED;
        }

        out_inspect->has_developer_directory = 1;
        out_inspect->developer_directory_offset = (size_t)footer.developer_offset;
        out_inspect->developer_directory_size = dir_size;
        out_inspect->developer_tag_count = count;

        min_payload = (size_t)-1;
        max_payload = 0u;
        for (i = 0u; i < count; ++i) {
            const tga_u8 *entry;

            entry = dir + 2u + (size_t)i * 10u;
            field_offset = (size_t)tga__read_u32_le(entry + 2u);
            field_size = (size_t)tga__read_u32_le(entry + 6u);
            if (!tga__region_fits(field_offset, field_size, src_size)) {
                return TGA_ERR_TRUNCATED;
            }
            if (field_size == 0u) {
                continue;
            }
            if (min_payload == (size_t)-1 || field_offset < min_payload) {
                min_payload = field_offset;
            }
            if (field_offset + field_size > max_payload) {
                max_payload = field_offset + field_size;
            }
        }

        if (min_payload != (size_t)-1) {
            out_inspect->has_developer_payload = 1;
            out_inspect->developer_payload_offset = min_payload;
            out_inspect->developer_payload_size = max_payload - min_payload;
        }
    }

    return TGA_OK;
}

static unsigned long tga__encoded_image_size_exact(const tga_encode_params *params)
{
    unsigned file_bytes;
    unsigned file_row;
    unsigned x;
    unsigned logical_y;
    unsigned logical_x;
    unsigned run;
    unsigned raw;
    unsigned long total;
    tga_u8 a[4];
    tga_u8 b[4];

    file_bytes = tga__file_sample_bytes(tga__encoded_file_bits(params));

    if (!params->rle) {
        return (unsigned long)params->width *
               (unsigned long)params->height *
               (unsigned long)file_bytes;
    }

    total = 0ul;

    for (file_row = 0u; file_row < params->height; ++file_row) {
        logical_y = tga__file_row_to_logical_y(params, file_row);

        x = 0u;
        while (x < params->width) {
            logical_x = tga__file_col_to_logical_x(params, x);
            tga__encode_image_sample_at(params, logical_y, logical_x, a);

            run = 1u;
            while (x + run < params->width && run < 128u) {
                logical_x = params->write_right_to_left
                    ? (params->width - 1u - (x + run))
                    : (x + run);
                tga__encode_image_sample_at(params, logical_y, logical_x, b);
                if (!tga__samples_equal(a, b, file_bytes)) {
                    break;
                }
                ++run;
            }

            if (run >= 2u) {
                total += 1ul + (unsigned long)file_bytes;
                x += run;
                continue;
            }

            raw = 1u;
            while (x + raw < params->width && raw < 128u) {
                logical_x = params->write_right_to_left
                    ? (params->width - 1u - (x + raw))
                    : (x + raw);
                tga__encode_image_sample_at(params, logical_y, logical_x, a);

                run = 1u;
                while (x + raw + run < params->width && run < 128u) {
                    logical_x = params->write_right_to_left
                        ? (params->width - 1u - (x + raw + run))
                        : (x + raw + run);
                    tga__encode_image_sample_at(params, logical_y, logical_x, b);
                    if (!tga__samples_equal(a, b, file_bytes)) {
                        break;
                    }
                    ++run;
                }

                if (run >= 2u) {
                    break;
                }

                ++raw;
            }

            total += 1ul + (unsigned long)raw * (unsigned long)file_bytes;
            x += raw;
        }
    }

    return total;
}

static int tga__write_footer_ex(tga_writer *writer,
                                tga_u32 extension_offset,
                                tga_u32 developer_offset)
{
    tga_u8 footer[26];

    memset(footer, 0, sizeof(footer));
    tga__write_u32_le(footer + 0u, extension_offset);
    tga__write_u32_le(footer + 4u, developer_offset);
    memcpy(footer + 8u, tga__footer_sig, 18u);

    if (!tga__write_exact(writer, footer, sizeof(footer))) {
        return TGA_ERR_IO;
    }
    return TGA_OK;
}

unsigned long tga_encode_ex_max_size(const tga_encode_params_ex *params)
{
    unsigned long total;
    unsigned long add;
    unsigned long stamp_size;
    int rc;
    int has_extension;
    int has_developer;
    int want_footer;

    if (params == 0) {
        return 0ul;
    }

    rc = tga__validate_postage_stamp_request(params);
    if (rc != TGA_OK) {
        return 0ul;
    }

    rc = tga__validate_developer_fields(params->developer_fields,
                                        params->developer_field_count);
    if (rc != TGA_OK) {
        return 0ul;
    }

    total = tga_encode_max_size(&params->image);
    if (total == 0ul) {
        return 0ul;
    }

    has_extension = params->extension != 0 ||
                    params->write_scan_line_table ||
                    params->write_postage_stamp ||
                    params->color_correction_table != 0;
    has_developer = params->developer_field_count != 0u;
    want_footer = params->image.write_footer || has_extension || has_developer;

    if (has_developer) {
        rc = tga__compute_developer_payload_size(params->developer_fields,
                                                 params->developer_field_count,
                                                 &add);
        if (rc != TGA_OK) {
            return 0ul;
        }
        if (add > ULONG_MAX - total) {
            return 0ul;
        }
        total += add;

        add = tga__developer_directory_size(params->developer_field_count);
        if (add > ULONG_MAX - total) {
            return 0ul;
        }
        total += add;
    }

    if (has_extension) {
        if (495ul > ULONG_MAX - total) {
            return 0ul;
        }
        total += 495ul;
    }

    if (params->write_scan_line_table) {
        add = (unsigned long)params->image.height * 4ul;
        if (add > ULONG_MAX - total) {
            return 0ul;
        }
        total += add;
    }

    if (params->write_postage_stamp) {
        rc = tga__compute_postage_stamp_block_size(params, 0, 0, &stamp_size);
        if (rc != TGA_OK) {
            return 0ul;
        }
        if (stamp_size > ULONG_MAX - total) {
            return 0ul;
        }
        total += stamp_size;
    }

    if (params->color_correction_table != 0) {
        add = (unsigned long)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8ul;
        if (add > ULONG_MAX - total) {
            return 0ul;
        }
        total += add;
    }

    if (want_footer && !params->image.write_footer) {
        if (26ul > ULONG_MAX - total) {
            return 0ul;
        }
        total += 26ul;
    }

    return total;
}

int tga_encode_ex(tga_writer *writer, const tga_encode_params_ex *params)
{
    int rc;
    int has_extension;
    int has_developer;
    int want_footer;
    unsigned long image_bytes;
    unsigned long pos;
    unsigned long developer_payload_size;
    unsigned long developer_directory_size;
    unsigned long scan_line_table_size;
    unsigned long postage_stamp_size;
    tga_u32 image_data_offset;
    tga_u32 developer_offset;
    tga_u32 extension_offset;
    tga_u32 scan_line_offset;
    tga_u32 developer_data_offset;
    tga_u32 postage_stamp_offset;
    tga_u32 color_correction_offset;
    unsigned postage_stamp_width;
    unsigned postage_stamp_height;
    tga_u8 ext[495];

    if (writer == 0 || params == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    has_extension = params->extension != 0 ||
                    params->write_scan_line_table ||
                    params->write_postage_stamp ||
                    params->color_correction_table != 0;
    has_developer = params->developer_field_count != 0u;
    if (!has_extension && !has_developer) {
        return tga_encode(writer, &params->image);
    }

    rc = tga__validate_encode_params(&params->image);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__validate_postage_stamp_request(params);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__validate_developer_fields(params->developer_fields,
                                        params->developer_field_count);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__compute_developer_payload_size(params->developer_fields,
                                             params->developer_field_count,
                                             &developer_payload_size);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__compute_postage_stamp_block_size(params,
                                               &postage_stamp_width,
                                               &postage_stamp_height,
                                               &postage_stamp_size);
    if (rc != TGA_OK) {
        return rc;
    }

    image_bytes = tga__encoded_image_size_exact(&params->image);
    image_data_offset = (tga_u32)tga__image_data_offset(&params->image);

    pos = (unsigned long)image_data_offset + image_bytes;
    if (pos > 4294967295ul) {
        return TGA_ERR_OVERFLOW;
    }

    developer_offset = 0ul;
    developer_data_offset = 0ul;
    if (has_developer) {
        developer_data_offset = (tga_u32)pos;
        if (developer_payload_size > 4294967295ul - pos) {
            return TGA_ERR_OVERFLOW;
        }
        pos += developer_payload_size;
        developer_offset = (tga_u32)pos;
        developer_directory_size = tga__developer_directory_size(params->developer_field_count);
        if (developer_directory_size > 4294967295ul - pos) {
            return TGA_ERR_OVERFLOW;
        }
        pos += developer_directory_size;
    }

    extension_offset = 0ul;
    if (has_extension) {
        extension_offset = (tga_u32)pos;
        if (495ul > 4294967295ul - pos) {
            return TGA_ERR_OVERFLOW;
        }
        pos += 495ul;
    }

    scan_line_offset = 0ul;
    if (params->write_scan_line_table) {
        scan_line_offset = (tga_u32)pos;
        scan_line_table_size = (unsigned long)params->image.height * 4ul;
        if (scan_line_table_size > 4294967295ul - pos) {
            return TGA_ERR_OVERFLOW;
        }
        pos += scan_line_table_size;
    }

    postage_stamp_offset = 0ul;
    if (params->write_postage_stamp) {
        postage_stamp_offset = (tga_u32)pos;
        if (postage_stamp_size > 4294967295ul - pos) {
            return TGA_ERR_OVERFLOW;
        }
        pos += postage_stamp_size;
    }

    color_correction_offset = 0ul;
    if (params->color_correction_table != 0) {
        color_correction_offset = (tga_u32)pos;
        if ((unsigned long)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8ul > 4294967295ul - pos) {
            return TGA_ERR_OVERFLOW;
        }
        pos += (unsigned long)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8ul;
    }

    want_footer = params->image.write_footer || has_extension || has_developer;
    if (want_footer && 26ul > 4294967295ul - pos) {
        return TGA_ERR_OVERFLOW;
    }

    rc = tga__write_header(writer, &params->image);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__write_palette(writer, &params->image);
    if (rc != TGA_OK) {
        return rc;
    }

    if (params->image.rle) {
        rc = tga__write_rle_pixels(writer, &params->image);
    } else {
        rc = tga__write_raw_pixels(writer, &params->image);
    }
    if (rc != TGA_OK) {
        return rc;
    }

    if (has_developer) {
        rc = tga__write_developer_area(writer,
                                       params->developer_fields,
                                       params->developer_field_count,
                                       developer_data_offset);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    if (has_extension) {
        tga_extension ext_copy;

        memset(&ext_copy, 0, sizeof(ext_copy));
        if (params->extension != 0) {
            ext_copy = *params->extension;
        }
        if (ext_copy.attributes_type == 0u) {
            ext_copy.attributes_type =
                tga_default_attributes_type_from_encode_params(&params->image);
        }

        tga__build_extension_bytes_ex(ext, &ext_copy,
                                      color_correction_offset,
                                      postage_stamp_offset,
                                      scan_line_offset);
        if (!tga__write_exact(writer, ext, sizeof(ext))) {
            return TGA_ERR_IO;
        }
    }

    if (params->write_scan_line_table) {
        rc = tga__write_scan_line_table(writer, &params->image, image_data_offset);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    if (params->write_postage_stamp) {
        rc = tga__write_postage_stamp(writer, params,
                                      postage_stamp_width,
                                      postage_stamp_height);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    if (params->color_correction_table != 0) {
        rc = tga__write_color_correction_table(writer,
                                               params->color_correction_table);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    if (want_footer) {
        rc = tga__write_footer_ex(writer, extension_offset, developer_offset);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    return TGA_OK;
}

int tga_encode_ex_memory(void *dst, size_t dst_capacity, size_t *dst_size,
                         const tga_encode_params_ex *params)
{
    tga_mem_writer mem;
    tga_writer writer;
    int rc;

    if (dst == 0 || dst_size == 0 || params == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    mem.data = (tga_u8 *)dst;
    mem.capacity = dst_capacity;
    mem.pos = 0u;
    mem.overflowed = 0;

    writer.write = tga_mem_write;
    writer.user = &mem;

    rc = tga_encode_ex(&writer, params);
    *dst_size = mem.pos;

    if (rc == TGA_ERR_IO && mem.overflowed) {
        return TGA_ERR_CAPACITY;
    }
    return rc;
}

int tga_encode_ex_size(const tga_encode_params_ex *params, size_t *out_size)
{
    tga__count_writer cw;
    tga_writer writer;
    int rc;

    if (params == 0 || out_size == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    cw.pos = 0u;
    cw.overflowed = 0;
    writer.write = tga__count_write;
    writer.user = &cw;

    rc = tga_encode_ex(&writer, params);
    if (rc == TGA_ERR_IO && cw.overflowed) {
        return TGA_ERR_OVERFLOW;
    }
    if (rc != TGA_OK) {
        return rc;
    }

    *out_size = cw.pos;
    return TGA_OK;
}


typedef struct tga__canonical_plan {
    tga_inspect inspect;
    tga_canonicalize_options options;
    tga_extension extension;
    size_t base_size;
    size_t total_size;
    size_t developer_payload_size;
    size_t developer_directory_size;
    size_t extension_size;
    size_t scan_line_size;
    size_t postage_stamp_size;
    size_t color_correction_size;
    size_t footer_size;
    size_t developer_payload_offset;
    size_t developer_directory_offset;
    size_t extension_offset;
    size_t scan_line_offset;
    size_t postage_stamp_offset;
    size_t color_correction_offset;
    unsigned developer_field_count;
    int keep_developer_area;
    int keep_scan_line_table;
    int keep_postage_stamp;
    int keep_color_correction;
    int have_extension;
    int want_footer;
} tga__canonical_plan;

static void tga__normalize_canonicalize_options(const tga_canonicalize_options *options,
                                                tga_canonicalize_options *out)
{
    if (out == 0) {
        return;
    }
    if (options != 0) {
        *out = *options;
    } else {
        out->flags = TGA_CANONICALIZE_DEFAULT;
        out->extension_override = 0;
    }
}

static int tga__count_kept_developer_fields_memory(const tga_u8 *bytes,
                                                   size_t src_size,
                                                   const tga_inspect *inspect,
                                                   unsigned *out_count,
                                                   size_t *out_payload_size)
{
    const tga_u8 *dir;
    const tga_u8 *entry;
    unsigned count;
    unsigned kept;
    unsigned i;
    size_t payload_size;
    size_t field_offset;
    size_t field_size;

    if (out_count != 0) {
        *out_count = 0u;
    }
    if (out_payload_size != 0) {
        *out_payload_size = 0u;
    }
    if (inspect == 0 || !inspect->has_developer_directory) {
        return TGA_OK;
    }

    dir = bytes + inspect->developer_directory_offset;
    count = (unsigned)tga__read_u16_le(dir + 0u);
    kept = 0u;
    payload_size = 0u;
    for (i = 0u; i < count; ++i) {
        entry = dir + 2u + (size_t)i * 10u;
        field_offset = (size_t)tga__read_u32_le(entry + 2u);
        field_size = (size_t)tga__read_u32_le(entry + 6u);
        if (!tga__region_fits(field_offset, field_size, src_size)) {
            continue;
        }
        if (!tga__checked_add_size(payload_size, field_size, &payload_size)) {
            return TGA_ERR_OVERFLOW;
        }
        ++kept;
    }

    if (out_count != 0) {
        *out_count = kept;
    }
    if (out_payload_size != 0) {
        *out_payload_size = payload_size;
    }
    return TGA_OK;
}

static int tga__build_canonical_plan(const tga_inspect *inspect,
                                     const tga_canonicalize_options *options,
                                     unsigned kept_developer_count,
                                     size_t kept_developer_payload_size,
                                     tga__canonical_plan *out_plan)
{
    tga_canonicalize_options opts;
    size_t pos;
    size_t add;
    unsigned long pos32;

    if (inspect == 0 || out_plan == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_plan, 0, sizeof(*out_plan));
    out_plan->inspect = *inspect;
    tga__normalize_canonicalize_options(options, &opts);
    out_plan->options = opts;

    if (!tga__checked_add_size(inspect->image_data_offset,
                               inspect->image_data_size,
                               &out_plan->base_size)) {
        return TGA_ERR_OVERFLOW;
    }

    out_plan->keep_developer_area =
        ((opts.flags & TGA_CANONICALIZE_KEEP_DEVELOPER_AREA) != 0u) &&
        kept_developer_count != 0u;
    out_plan->keep_scan_line_table =
        ((opts.flags & TGA_CANONICALIZE_KEEP_SCAN_LINE_TABLE) != 0u) &&
        inspect->has_scan_line_table;
    out_plan->keep_postage_stamp =
        ((opts.flags & TGA_CANONICALIZE_KEEP_POSTAGE_STAMP) != 0u) &&
        inspect->has_postage_stamp;
    out_plan->keep_color_correction =
        ((opts.flags & TGA_CANONICALIZE_KEEP_COLOR_CORRECTION) != 0u) &&
        inspect->has_color_correction_table;

    if (out_plan->keep_developer_area) {
        out_plan->developer_field_count = kept_developer_count;
        out_plan->developer_payload_size = kept_developer_payload_size;
        if (!tga__checked_mul_size((size_t)kept_developer_count, 10u, &add) ||
            !tga__checked_add_size(2u, add, &out_plan->developer_directory_size)) {
            return TGA_ERR_OVERFLOW;
        }
    }

    if (opts.extension_override != 0) {
        out_plan->extension = *opts.extension_override;
        out_plan->have_extension = 1;
    } else if ((opts.flags & TGA_CANONICALIZE_KEEP_EXTENSION) != 0u &&
               inspect->has_extension) {
        out_plan->extension = inspect->extension;
        out_plan->have_extension = 1;
    }

    if (!out_plan->have_extension &&
        (((opts.flags & TGA_CANONICALIZE_SYNTHESIZE_EXTENSION) != 0u) ||
         out_plan->keep_scan_line_table ||
         out_plan->keep_postage_stamp ||
         out_plan->keep_color_correction)) {
        memset(&out_plan->extension, 0, sizeof(out_plan->extension));
        out_plan->have_extension = 1;
    }

    if (out_plan->have_extension) {
        out_plan->extension.size = 495u;
        out_plan->extension.color_correction_offset = 0ul;
        out_plan->extension.postage_stamp_offset = 0ul;
        out_plan->extension.scan_line_offset = 0ul;
        if ((opts.flags & TGA_CANONICALIZE_NORMALIZE_EXTENSION) != 0u) {
            if (out_plan->extension.attributes_type == 0u ||
                out_plan->extension.attributes_type > TGA_ATTRIBUTES_TYPE_PREMULTIPLIED) {
                out_plan->extension.attributes_type =
                    tga_default_attributes_type_from_info(&inspect->info);
            }
        }
        out_plan->extension_size = 495u;
    }

    out_plan->scan_line_size = out_plan->keep_scan_line_table
        ? inspect->scan_line_table_size : 0u;
    out_plan->postage_stamp_size = out_plan->keep_postage_stamp
        ? inspect->postage_stamp_size : 0u;
    out_plan->color_correction_size = out_plan->keep_color_correction
        ? inspect->color_correction_size : 0u;
    out_plan->want_footer =
        ((opts.flags & TGA_CANONICALIZE_FORCE_FOOTER) != 0u) ||
        out_plan->keep_developer_area ||
        out_plan->have_extension ||
        inspect->footer.present;
    out_plan->footer_size = out_plan->want_footer ? 26u : 0u;

    pos = out_plan->base_size;
    if ((unsigned long)pos > 4294967295ul) {
        return TGA_ERR_OVERFLOW;
    }

    if (out_plan->keep_developer_area) {
        out_plan->developer_payload_offset = pos;
        if (!tga__checked_add_size(pos, out_plan->developer_payload_size, &pos)) {
            return TGA_ERR_OVERFLOW;
        }
        out_plan->developer_directory_offset = pos;
        if (!tga__checked_add_size(pos, out_plan->developer_directory_size, &pos)) {
            return TGA_ERR_OVERFLOW;
        }
    }

    if (out_plan->have_extension) {
        out_plan->extension_offset = pos;
        if (!tga__checked_add_size(pos, out_plan->extension_size, &pos)) {
            return TGA_ERR_OVERFLOW;
        }
    }

    if (out_plan->keep_scan_line_table) {
        out_plan->scan_line_offset = pos;
        if (!tga__checked_add_size(pos, out_plan->scan_line_size, &pos)) {
            return TGA_ERR_OVERFLOW;
        }
    }

    if (out_plan->keep_postage_stamp) {
        out_plan->postage_stamp_offset = pos;
        if (!tga__checked_add_size(pos, out_plan->postage_stamp_size, &pos)) {
            return TGA_ERR_OVERFLOW;
        }
    }

    if (out_plan->keep_color_correction) {
        out_plan->color_correction_offset = pos;
        if (!tga__checked_add_size(pos, out_plan->color_correction_size, &pos)) {
            return TGA_ERR_OVERFLOW;
        }
    }

    if (out_plan->want_footer) {
        if (!tga__checked_add_size(pos, out_plan->footer_size, &pos)) {
            return TGA_ERR_OVERFLOW;
        }
    }

    pos32 = (unsigned long)pos;
    if (pos32 > 4294967295ul) {
        return TGA_ERR_OVERFLOW;
    }
    out_plan->total_size = pos;

    if (out_plan->have_extension) {
        if (out_plan->keep_color_correction) {
            out_plan->extension.color_correction_offset =
                (tga_u32)out_plan->color_correction_offset;
        }
        if (out_plan->keep_postage_stamp) {
            out_plan->extension.postage_stamp_offset =
                (tga_u32)out_plan->postage_stamp_offset;
        }
        if (out_plan->keep_scan_line_table) {
            out_plan->extension.scan_line_offset =
                (tga_u32)out_plan->scan_line_offset;
        }
    }

    return TGA_OK;
}

static int tga__write_developer_area_from_source_memory(tga_writer *writer,
                                                        const tga_u8 *bytes,
                                                        size_t src_size,
                                                        const tga_inspect *inspect,
                                                        tga_u32 developer_data_offset)
{
    const tga_u8 *dir;
    const tga_u8 *entry;
    tga_u8 count_buf[2];
    tga_u8 out_entry[10];
    unsigned count;
    unsigned kept;
    unsigned i;
    unsigned long current_offset;
    size_t field_offset;
    size_t field_size;

    if (inspect == 0 || !inspect->has_developer_directory) {
        return TGA_OK;
    }

    dir = bytes + inspect->developer_directory_offset;
    count = (unsigned)tga__read_u16_le(dir + 0u);
    kept = 0u;
    for (i = 0u; i < count; ++i) {
        entry = dir + 2u + (size_t)i * 10u;
        field_offset = (size_t)tga__read_u32_le(entry + 2u);
        field_size = (size_t)tga__read_u32_le(entry + 6u);
        if (!tga__region_fits(field_offset, field_size, src_size)) {
            continue;
        }
        if (field_size != 0u) {
            if (!tga__write_exact(writer, bytes + field_offset, field_size)) {
                return TGA_ERR_IO;
            }
        }
        ++kept;
    }

    tga__write_u16_le(count_buf, kept);
    if (!tga__write_exact(writer, count_buf, sizeof(count_buf))) {
        return TGA_ERR_IO;
    }

    current_offset = (unsigned long)developer_data_offset;
    for (i = 0u; i < count; ++i) {
        entry = dir + 2u + (size_t)i * 10u;
        field_offset = (size_t)tga__read_u32_le(entry + 2u);
        field_size = (size_t)tga__read_u32_le(entry + 6u);
        if (!tga__region_fits(field_offset, field_size, src_size)) {
            continue;
        }
        tga__write_u16_le(out_entry + 0u, (unsigned)tga__read_u16_le(entry + 0u));
        tga__write_u32_le(out_entry + 2u, (tga_u32)current_offset);
        tga__write_u32_le(out_entry + 6u, (tga_u32)field_size);
        if (!tga__write_exact(writer, out_entry, sizeof(out_entry))) {
            return TGA_ERR_IO;
        }
        current_offset += (unsigned long)field_size;
    }

    return TGA_OK;
}

int tga_canonicalize_size_memory(const void *src,
                                 size_t src_size,
                                 const tga_canonicalize_options *options,
                                 size_t *out_size)
{
    const tga_u8 *bytes;
    tga_inspect inspect;
    tga__canonical_plan plan;
    unsigned kept_developer_count;
    size_t kept_developer_payload_size;
    int rc;

    if (src == 0 || out_size == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga_inspect_memory(src, src_size, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }

    bytes = (const tga_u8 *)src;
    rc = tga__count_kept_developer_fields_memory(bytes,
                                                 src_size,
                                                 &inspect,
                                                 &kept_developer_count,
                                                 &kept_developer_payload_size);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__build_canonical_plan(&inspect,
                                   options,
                                   kept_developer_count,
                                   kept_developer_payload_size,
                                   &plan);
    if (rc != TGA_OK) {
        return rc;
    }

    *out_size = plan.total_size;
    return TGA_OK;
}

int tga_canonicalize_memory(void *dst,
                            size_t dst_capacity,
                            size_t *dst_size,
                            const void *src,
                            size_t src_size,
                            const tga_canonicalize_options *options)
{
    const tga_u8 *bytes;
    tga_inspect inspect;
    tga__canonical_plan plan;
    tga_mem_writer mem;
    tga_writer writer;
    tga_u8 ext[495];
    unsigned kept_developer_count;
    size_t kept_developer_payload_size;
    int rc;

    if (dst == 0 || src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (dst_size != 0) {
        *dst_size = 0u;
    }

    rc = tga_inspect_memory(src, src_size, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }

    bytes = (const tga_u8 *)src;
    rc = tga__count_kept_developer_fields_memory(bytes,
                                                 src_size,
                                                 &inspect,
                                                 &kept_developer_count,
                                                 &kept_developer_payload_size);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__build_canonical_plan(&inspect,
                                   options,
                                   kept_developer_count,
                                   kept_developer_payload_size,
                                   &plan);
    if (rc != TGA_OK) {
        return rc;
    }

    if (dst_size != 0) {
        *dst_size = plan.total_size;
    }
    if (dst_capacity < plan.total_size) {
        return TGA_ERR_CAPACITY;
    }

    mem.data = (tga_u8 *)dst;
    mem.capacity = dst_capacity;
    mem.pos = 0u;
    mem.overflowed = 0;
    writer.write = tga_mem_write;
    writer.user = &mem;

    if (!tga__write_exact(&writer, bytes, plan.base_size)) {
        return TGA_ERR_IO;
    }

    if (plan.keep_developer_area) {
        rc = tga__write_developer_area_from_source_memory(&writer,
                                                          bytes,
                                                          src_size,
                                                          &inspect,
                                                          (tga_u32)plan.developer_payload_offset);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    if (plan.have_extension) {
        tga__build_extension_bytes_ex(ext,
                                      &plan.extension,
                                      plan.keep_color_correction ?
                                          (tga_u32)plan.color_correction_offset : 0ul,
                                      plan.keep_postage_stamp ?
                                          (tga_u32)plan.postage_stamp_offset : 0ul,
                                      plan.keep_scan_line_table ?
                                          (tga_u32)plan.scan_line_offset : 0ul);
        if (!tga__write_exact(&writer, ext, sizeof(ext))) {
            return TGA_ERR_IO;
        }
    }

    if (plan.keep_scan_line_table) {
        if (!tga__write_exact(&writer,
                              bytes + inspect.scan_line_table_offset,
                              plan.scan_line_size)) {
            return TGA_ERR_IO;
        }
    }
    if (plan.keep_postage_stamp) {
        if (!tga__write_exact(&writer,
                              bytes + inspect.postage_stamp_offset,
                              plan.postage_stamp_size)) {
            return TGA_ERR_IO;
        }
    }
    if (plan.keep_color_correction) {
        if (!tga__write_exact(&writer,
                              bytes + inspect.color_correction_offset,
                              plan.color_correction_size)) {
            return TGA_ERR_IO;
        }
    }

    if (plan.want_footer) {
        rc = tga__write_footer_ex(&writer,
                                  plan.have_extension ?
                                      (tga_u32)plan.extension_offset : 0ul,
                                  plan.keep_developer_area ?
                                      (tga_u32)plan.developer_directory_offset : 0ul);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    if (dst_size != 0) {
        *dst_size = mem.pos;
    }
    if (mem.overflowed) {
        return TGA_ERR_CAPACITY;
    }
    return TGA_OK;
}

int tga_repair_tga2_size_memory(const void *src, size_t src_size, size_t *out_size)
{
    return tga_canonicalize_size_memory(src, src_size, 0, out_size);
}

int tga_repair_tga2_memory(void *dst,
                           size_t dst_capacity,
                           size_t *dst_size,
                           const void *src,
                           size_t src_size)
{
    return tga_canonicalize_memory(dst, dst_capacity, dst_size,
                                   src, src_size, 0);
}

#ifndef GAFNYF_TGA_NO_STDIO

size_t tga_file_read(void *user, void *dst, size_t bytes)
{
    tga_file_reader *fr;

    if (user == 0) {
        return 0u;
    }

    fr = (tga_file_reader *)user;
    if (fr->fp == 0) {
        return 0u;
    }

    return fread(dst, 1u, bytes, fr->fp);
}

size_t tga_file_write(void *user, const void *src, size_t bytes)
{
    tga_file_writer *fw;

    if (user == 0) {
        return 0u;
    }

    fw = (tga_file_writer *)user;
    if (fw->fp == 0) {
        return 0u;
    }

    return fwrite(src, 1u, bytes, fw->fp);
}

static int tga__read_section_file_impl(FILE *fp,
                                       size_t offset,
                                       size_t size,
                                       void *dst,
                                       size_t dst_capacity,
                                       size_t *out_size)
{
    long saved_pos;
    long end_pos;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_size != 0) {
        *out_size = size;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if ((unsigned long)offset > (unsigned long)end_pos ||
        (unsigned long)size > (unsigned long)end_pos - (unsigned long)offset) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    if (dst == 0 || dst_capacity == 0u) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_OK;
    }
    if (dst_capacity < size) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_CAPACITY;
    }

    if (fseek(fp, (long)offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if (size != 0u && fread(dst, 1u, size, fp) != size) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    return TGA_OK;
}

static int tga__copy_file_region(FILE *fp,
                                 size_t offset,
                                 size_t size,
                                 tga_writer *writer)
{
    long saved_pos;
    long end_pos;
    tga_u8 buf[512];
    size_t chunk;
    int rc;

    if (fp == 0 || writer == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if ((unsigned long)offset > (unsigned long)end_pos ||
        (unsigned long)size > (unsigned long)end_pos - (unsigned long)offset) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }
    if (fseek(fp, (long)offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    while (size != 0u) {
        chunk = size;
        if (chunk > sizeof(buf)) {
            chunk = sizeof(buf);
        }
        if (fread(buf, 1u, chunk, fp) != chunk) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_IO;
        }
        if (!tga__write_exact(writer, buf, chunk)) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_IO;
        }
        size -= chunk;
    }

    rc = fseek(fp, saved_pos, SEEK_SET) == 0 ? TGA_OK : TGA_ERR_IO;
    return rc;
}

static int tga__count_kept_developer_fields_file(FILE *fp,
                                                 const tga_inspect *inspect,
                                                 unsigned *out_count,
                                                 size_t *out_payload_size)
{
    tga_u8 entry[10];
    unsigned count;
    unsigned kept;
    unsigned i;
    size_t payload_size;
    size_t field_offset;
    size_t field_size;
    int rc;

    if (out_count != 0) {
        *out_count = 0u;
    }
    if (out_payload_size != 0) {
        *out_payload_size = 0u;
    }
    if (fp == 0 || inspect == 0 || !inspect->has_developer_directory) {
        return TGA_OK;
    }

    count = inspect->developer_tag_count;
    kept = 0u;
    payload_size = 0u;
    for (i = 0u; i < count; ++i) {
        rc = tga__read_section_file_impl(fp,
                                         inspect->developer_directory_offset + 2u + (size_t)i * 10u,
                                         10u,
                                         entry,
                                         sizeof(entry),
                                         0);
        if (rc != TGA_OK) {
            return rc;
        }
        field_offset = (size_t)tga__read_u32_le(entry + 2u);
        field_size = (size_t)tga__read_u32_le(entry + 6u);
        if (!tga__region_fits(field_offset, field_size, inspect->file_size)) {
            continue;
        }
        if (!tga__checked_add_size(payload_size, field_size, &payload_size)) {
            return TGA_ERR_OVERFLOW;
        }
        ++kept;
    }

    if (out_count != 0) {
        *out_count = kept;
    }
    if (out_payload_size != 0) {
        *out_payload_size = payload_size;
    }
    return TGA_OK;
}

static int tga__write_developer_area_from_source_file(tga_writer *writer,
                                                      FILE *fp,
                                                      const tga_inspect *inspect,
                                                      tga_u32 developer_data_offset)
{
    tga_u8 entry[10];
    tga_u8 count_buf[2];
    tga_u8 out_entry[10];
    unsigned count;
    unsigned kept;
    unsigned i;
    unsigned long current_offset;
    size_t field_offset;
    size_t field_size;
    int rc;

    if (writer == 0 || fp == 0 || inspect == 0 || !inspect->has_developer_directory) {
        return TGA_OK;
    }

    count = inspect->developer_tag_count;
    kept = 0u;
    for (i = 0u; i < count; ++i) {
        rc = tga__read_section_file_impl(fp,
                                         inspect->developer_directory_offset + 2u + (size_t)i * 10u,
                                         10u,
                                         entry,
                                         sizeof(entry),
                                         0);
        if (rc != TGA_OK) {
            return rc;
        }
        field_offset = (size_t)tga__read_u32_le(entry + 2u);
        field_size = (size_t)tga__read_u32_le(entry + 6u);
        if (!tga__region_fits(field_offset, field_size, inspect->file_size)) {
            continue;
        }
        rc = tga__copy_file_region(fp, field_offset, field_size, writer);
        if (rc != TGA_OK) {
            return rc;
        }
        ++kept;
    }

    tga__write_u16_le(count_buf, kept);
    if (!tga__write_exact(writer, count_buf, sizeof(count_buf))) {
        return TGA_ERR_IO;
    }

    current_offset = (unsigned long)developer_data_offset;
    for (i = 0u; i < count; ++i) {
        rc = tga__read_section_file_impl(fp,
                                         inspect->developer_directory_offset + 2u + (size_t)i * 10u,
                                         10u,
                                         entry,
                                         sizeof(entry),
                                         0);
        if (rc != TGA_OK) {
            return rc;
        }
        field_offset = (size_t)tga__read_u32_le(entry + 2u);
        field_size = (size_t)tga__read_u32_le(entry + 6u);
        if (!tga__region_fits(field_offset, field_size, inspect->file_size)) {
            continue;
        }
        tga__write_u16_le(out_entry + 0u, (unsigned)tga__read_u16_le(entry + 0u));
        tga__write_u32_le(out_entry + 2u, (tga_u32)current_offset);
        tga__write_u32_le(out_entry + 6u, (tga_u32)field_size);
        if (!tga__write_exact(writer, out_entry, sizeof(out_entry))) {
            return TGA_ERR_IO;
        }
        current_offset += (unsigned long)field_size;
    }

    return TGA_OK;
}

int tga_canonicalize_file(FILE *dst,
                          FILE *src,
                          const tga_canonicalize_options *options)
{
    tga_inspect inspect;
    tga__canonical_plan plan;
    tga_file_writer fw;
    tga_writer writer;
    tga_u8 ext[495];
    unsigned kept_developer_count;
    size_t kept_developer_payload_size;
    int rc;

    if (dst == 0 || src == 0 || dst == src) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga_inspect_file(src, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__count_kept_developer_fields_file(src,
                                               &inspect,
                                               &kept_developer_count,
                                               &kept_developer_payload_size);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__build_canonical_plan(&inspect,
                                   options,
                                   kept_developer_count,
                                   kept_developer_payload_size,
                                   &plan);
    if (rc != TGA_OK) {
        return rc;
    }

    fw.fp = dst;
    writer.write = tga_file_write;
    writer.user = &fw;

    rc = tga__copy_file_region(src, 0u, plan.base_size, &writer);
    if (rc != TGA_OK) {
        return rc;
    }
    if (plan.keep_developer_area) {
        rc = tga__write_developer_area_from_source_file(&writer,
                                                        src,
                                                        &inspect,
                                                        (tga_u32)plan.developer_payload_offset);
        if (rc != TGA_OK) {
            return rc;
        }
    }
    if (plan.have_extension) {
        tga__build_extension_bytes_ex(ext,
                                      &plan.extension,
                                      plan.keep_color_correction ?
                                          (tga_u32)plan.color_correction_offset : 0ul,
                                      plan.keep_postage_stamp ?
                                          (tga_u32)plan.postage_stamp_offset : 0ul,
                                      plan.keep_scan_line_table ?
                                          (tga_u32)plan.scan_line_offset : 0ul);
        if (!tga__write_exact(&writer, ext, sizeof(ext))) {
            return TGA_ERR_IO;
        }
    }
    if (plan.keep_scan_line_table) {
        rc = tga__copy_file_region(src,
                                   inspect.scan_line_table_offset,
                                   plan.scan_line_size,
                                   &writer);
        if (rc != TGA_OK) {
            return rc;
        }
    }
    if (plan.keep_postage_stamp) {
        rc = tga__copy_file_region(src,
                                   inspect.postage_stamp_offset,
                                   plan.postage_stamp_size,
                                   &writer);
        if (rc != TGA_OK) {
            return rc;
        }
    }
    if (plan.keep_color_correction) {
        rc = tga__copy_file_region(src,
                                   inspect.color_correction_offset,
                                   plan.color_correction_size,
                                   &writer);
        if (rc != TGA_OK) {
            return rc;
        }
    }
    if (plan.want_footer) {
        rc = tga__write_footer_ex(&writer,
                                  plan.have_extension ?
                                      (tga_u32)plan.extension_offset : 0ul,
                                  plan.keep_developer_area ?
                                      (tga_u32)plan.developer_directory_offset : 0ul);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    return TGA_OK;
}

int tga_repair_tga2_file(FILE *dst, FILE *src)
{
    return tga_canonicalize_file(dst, src, 0);
}

static int tga__measure_image_data_size_file(FILE *fp,
                                             const tga_info *info,
                                             size_t image_data_offset,
                                             size_t *out_size)
{
    long saved_pos;
    long end_pos;
    size_t sample_bytes;
    size_t total_pixels;
    size_t total_size;
    size_t pos;
    size_t produced;
    int c;
    unsigned packet_header;
    size_t count;
    size_t need;

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    sample_bytes = (size_t)tga__file_sample_bytes(info->pixel_depth);
    if (!tga__checked_mul_size((size_t)info->width, (size_t)info->height, &total_pixels)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_OVERFLOW;
    }

    if (!info->is_rle) {
        if (!tga__checked_mul_size(total_pixels, sample_bytes, &total_size)) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_OVERFLOW;
        }
        if ((unsigned long)image_data_offset > (unsigned long)end_pos ||
            (unsigned long)total_size > (unsigned long)end_pos - (unsigned long)image_data_offset) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_TRUNCATED;
        }
        *out_size = total_size;
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_OK;
    }

    if (fseek(fp, (long)image_data_offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    pos = image_data_offset;
    produced = 0u;
    while (produced < total_pixels) {
        if ((unsigned long)pos >= (unsigned long)end_pos) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_TRUNCATED;
        }
        c = fgetc(fp);
        if (c == EOF) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_IO;
        }
        packet_header = (unsigned)(c & 255);
        ++pos;
        count = (size_t)(packet_header & 127u) + 1u;
        if (count > total_pixels - produced) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_BAD_FORMAT;
        }
        if ((packet_header & 128u) != 0u) {
            need = sample_bytes;
        } else {
            if (!tga__checked_mul_size(count, sample_bytes, &need)) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_OVERFLOW;
            }
        }
        if ((unsigned long)need > (unsigned long)end_pos - (unsigned long)pos) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_TRUNCATED;
        }
        if (fseek(fp, (long)need, SEEK_CUR) != 0) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_IO;
        }
        pos += need;
        produced += count;
    }

    *out_size = pos - image_data_offset;
    (void)fseek(fp, saved_pos, SEEK_SET);
    return TGA_OK;
}

int tga_decode_file(FILE *fp, const tga_decode_params *params, tga_info *out_info)
{
    tga_file_reader fr;
    tga_reader reader;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    fr.fp = fp;
    reader.read = tga_file_read;
    reader.user = &fr;
    return tga_decode(&reader, params, out_info);
}

int tga_encode_file(FILE *fp, const tga_encode_params *params)
{
    tga_file_writer fw;
    tga_writer writer;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    fw.fp = fp;
    writer.write = tga_file_write;
    writer.user = &fw;
    return tga_encode(&writer, params);
}

int tga_encode_ex_file(FILE *fp, const tga_encode_params_ex *params)
{
    tga_file_writer fw;
    tga_writer writer;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    fw.fp = fp;
    writer.write = tga_file_write;
    writer.user = &fw;
    return tga_encode_ex(&writer, params);
}

int tga_read_footer_file(FILE *fp, tga_footer *out_footer)
{
    long saved_pos;
    long end_pos;
    tga_u8 footer[26];
    int rc;

    if (fp == 0 || out_footer == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_footer, 0, sizeof(*out_footer));

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if (end_pos < 26L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_NO_FOOTER;
    }

    if (fseek(fp, end_pos - 26L, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if (fread(footer, 1u, sizeof(footer), fp) != sizeof(footer)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    (void)fseek(fp, saved_pos, SEEK_SET);

    rc = tga__parse_footer_bytes(footer, out_footer);
    return rc;
}

int tga_read_extension_file(FILE *fp, tga_extension *out_extension)
{
    long saved_pos;
    long end_pos;
    tga_footer footer;
    tga_u8 ext[495];
    tga_u8 size_buf[2];
    unsigned size;
    int rc;

    if (fp == 0 || out_extension == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga_read_footer_file(fp, &footer);
    if (rc != TGA_OK) {
        return rc;
    }

    if (footer.extension_offset == 0ul) {
        return TGA_ERR_NO_EXTENSION;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if ((long)footer.extension_offset + 2L > end_pos) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    if (fseek(fp, (long)footer.extension_offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if (fread(size_buf, 1u, 2u, fp) != 2u) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    size = (unsigned)tga__read_u16_le(size_buf);
    if (size < 495u) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_BAD_FORMAT;
    }
    if ((long)footer.extension_offset + (long)size > end_pos) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    if (fseek(fp, (long)footer.extension_offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if (fread(ext, 1u, sizeof(ext), fp) != sizeof(ext)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    (void)fseek(fp, saved_pos, SEEK_SET);

    tga__parse_extension_bytes(ext, out_extension);
    return TGA_OK;
}

int tga_patch_extension_attributes_type_file(FILE *fp, unsigned attributes_type)
{
    tga_u8 value;

    if (attributes_type > 255u) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    value = (tga_u8)attributes_type;
    return tga__extension_patch_file(fp, 494u, &value, 1u);
}

int tga_patch_extension_pixel_aspect_q16_file(FILE *fp, tga_u32 q16)
{
    tga_u8 patch[4];
    unsigned num;
    unsigned den;

    tga__q16_to_ratio(q16, &num, &den);
    tga__write_u16_le(patch + 0u, num);
    tga__write_u16_le(patch + 2u, den);
    return tga__extension_patch_file(fp, 474u, patch, sizeof(patch));
}

int tga_patch_extension_gamma_q16_file(FILE *fp, tga_u32 q16)
{
    tga_u8 patch[4];
    unsigned num;
    unsigned den;

    tga__q16_to_ratio(q16, &num, &den);
    tga__write_u16_le(patch + 0u, num);
    tga__write_u16_le(patch + 2u, den);
    return tga__extension_patch_file(fp, 478u, patch, sizeof(patch));
}

int tga_read_scan_line_table_file(FILE *fp, tga_u32 *offsets,
                                  unsigned offset_capacity,
                                  unsigned *out_offset_count)
{
    long saved_pos;
    long end_pos;
    tga_extension ext;
    tga_info info;
    unsigned i;
    tga_u8 entry[4];
    int rc;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_offset_count != 0) {
        *out_offset_count = 0u;
    }

    rc = tga_read_extension_file(fp, &ext);
    if (rc != TGA_OK) {
        return rc;
    }
    if (ext.scan_line_offset == 0ul) {
        return TGA_ERR_NO_SCAN_LINE_TABLE;
    }

    rc = tga_probe_file(fp, &info);
    if (rc != TGA_OK) {
        return rc;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if ((unsigned long)ext.scan_line_offset + (unsigned long)info.height * 4ul <
        (unsigned long)ext.scan_line_offset ||
        (unsigned long)end_pos < (unsigned long)ext.scan_line_offset +
                                 (unsigned long)info.height * 4ul) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    if (out_offset_count != 0) {
        *out_offset_count = info.height;
    }

    if (offsets == 0 || offset_capacity == 0u) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_OK;
    }
    if (offset_capacity < info.height) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_CAPACITY;
    }

    if (fseek(fp, (long)ext.scan_line_offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    for (i = 0u; i < info.height; ++i) {
        if (fread(entry, 1u, sizeof(entry), fp) != sizeof(entry)) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_IO;
        }
        offsets[i] = tga__read_u32_le(entry);
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    return TGA_OK;
}

int tga_read_color_correction_table_file(FILE *fp,
                                         tga_color_correction_entry *table,
                                         unsigned table_capacity)
{
    long saved_pos;
    long end_pos;
    tga_extension ext;
    tga_u8 raw[TGA_COLOR_CORRECTION_ENTRY_COUNT * 8u];
    int rc;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga_read_extension_file(fp, &ext);
    if (rc != TGA_OK) {
        return rc;
    }
    if (ext.color_correction_offset == 0ul) {
        return TGA_ERR_NO_COLOR_CORRECTION_TABLE;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if ((unsigned long)ext.color_correction_offset +
        (unsigned long)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8ul >
        (unsigned long)end_pos) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    if (table == 0 || table_capacity == 0u) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_OK;
    }
    if (table_capacity < TGA_COLOR_CORRECTION_ENTRY_COUNT) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_CAPACITY;
    }

    if (fseek(fp, (long)ext.color_correction_offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if (fread(raw, 1u, sizeof(raw), fp) != sizeof(raw)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    tga__parse_color_correction_table(raw, table);
    return TGA_OK;
}

int tga_read_postage_stamp_info_file(FILE *fp,
                                     tga_postage_stamp_info *out_info)
{
    long saved_pos;
    long end_pos;
    tga_extension ext;
    tga_info info;
    tga_u8 dims[2];
    unsigned width;
    unsigned height;
    unsigned sample_bytes;
    unsigned long data_size;
    int rc;

    if (fp == 0 || out_info == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga_probe_file(fp, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__validate_decode_info(&info);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga_read_extension_file(fp, &ext);
    if (rc != TGA_OK) {
        return rc;
    }
    if (ext.postage_stamp_offset == 0ul) {
        return TGA_ERR_NO_POSTAGE_STAMP;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if ((long)ext.postage_stamp_offset + 2L > end_pos) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    if (fseek(fp, (long)ext.postage_stamp_offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if (fread(dims, 1u, sizeof(dims), fp) != sizeof(dims)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    width = (unsigned)dims[0];
    height = (unsigned)dims[1];
    if (width == 0u || height == 0u) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_BAD_FORMAT;
    }

    sample_bytes = tga__file_sample_bytes(info.pixel_depth);
    data_size = (unsigned long)width * (unsigned long)height * (unsigned long)sample_bytes;
    if ((unsigned long)ext.postage_stamp_offset + 2ul + data_size <
        (unsigned long)ext.postage_stamp_offset + 2ul ||
        (unsigned long)ext.postage_stamp_offset + 2ul + data_size >
        (unsigned long)end_pos) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    tga__fill_postage_stamp_info(&info, width, height, out_info);
    return TGA_OK;
}

int tga_read_postage_stamp_file(FILE *fp,
                                void *dst,
                                size_t dst_capacity,
                                size_t *out_stamp_size,
                                tga_postage_stamp_info *out_info)
{
    long saved_pos;
    tga_extension ext;
    tga_postage_stamp_info info;
    int rc;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_stamp_size != 0) {
        *out_stamp_size = 0u;
    }

    rc = tga_read_postage_stamp_info_file(fp, &info);
    if (rc != TGA_OK) {
        return rc;
    }

    if (out_stamp_size != 0) {
        *out_stamp_size = info.data_size;
    }
    if (out_info != 0) {
        *out_info = info;
    }

    if (dst == 0 || dst_capacity == 0u) {
        return TGA_OK;
    }
    if (dst_capacity < info.data_size) {
        return TGA_ERR_CAPACITY;
    }

    rc = tga_read_extension_file(fp, &ext);
    if (rc != TGA_OK) {
        return rc;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, (long)ext.postage_stamp_offset + 2L, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if (info.data_size != 0u &&
        fread(dst, 1u, info.data_size, fp) != info.data_size) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    return TGA_OK;
}

int tga_decode_postage_stamp_file(FILE *fp,
                                  const tga_decode_params *params,
                                  tga_info *out_info)
{
    long saved_pos;
    tga_file_reader fr;
    tga_reader reader;
    tga_info info;
    tga_info stamp_info;
    tga_extension ext;
    tga_postage_stamp_info stamp;
    tga_u8 hdr[18];
    int rc;
    unsigned out_bpp;
    unsigned long min_stride;

    if (fp == 0 || params == 0 || params->pixels == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__pixel_format_valid(params->pixel_format) ||
        params->pixel_format == TGA_PIXFMT_INDEX8) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    rc = tga_probe_file(fp, &info);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }
    rc = tga__validate_decode_info(&info);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }

    rc = tga_read_postage_stamp_info_file(fp, &stamp);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }

    out_bpp = tga_bytes_per_pixel(params->pixel_format);
    min_stride = (unsigned long)stamp.width * (unsigned long)out_bpp;
    if (params->stride_bytes < min_stride) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_BAD_ARGUMENT;
    }

    if (fseek(fp, 0L, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if (fread(hdr, 1u, sizeof(hdr), fp) != sizeof(hdr)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    fr.fp = fp;
    reader.read = tga_file_read;
    reader.user = &fr;

    if (!tga__skip(&reader, (size_t)info.id_length)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }
    rc = tga__load_palette(&reader, &info,
                           params->palette_rgba,
                           params->palette_capacity);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }

    rc = tga_read_extension_file(fp, &ext);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }

    if (fseek(fp, (long)ext.postage_stamp_offset + 2L, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    rc = tga__decode_uncompressed_pixels_from_reader(&reader, &info,
                                                     stamp.width, stamp.height,
                                                     params);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }

    if (out_info != 0) {
        stamp_info = info;
        stamp_info.width = stamp.width;
        stamp_info.height = stamp.height;
        stamp_info.image_type = tga__raw_image_type(info.image_type);
        stamp_info.is_rle = 0;
        *out_info = stamp_info;
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    return TGA_OK;
}

int tga_read_developer_directory_file(FILE *fp,
                                      tga_developer_tag *tags,
                                      unsigned tag_capacity,
                                      unsigned *out_tag_count)
{
    long saved_pos;
    long end_pos;
    tga_footer footer;
    tga_u8 buf2[2];
    tga_u8 entry[10];
    unsigned count;
    unsigned i;
    int rc;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_tag_count != 0) {
        *out_tag_count = 0u;
    }

    rc = tga_read_footer_file(fp, &footer);
    if (rc != TGA_OK) {
        return rc;
    }
    if (footer.developer_offset == 0ul) {
        return TGA_ERR_NO_DEVELOPER_AREA;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if ((long)footer.developer_offset + 2L > end_pos) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    if (fseek(fp, (long)footer.developer_offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if (fread(buf2, 1u, sizeof(buf2), fp) != sizeof(buf2)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    count = (unsigned)tga__read_u16_le(buf2);

    if ((unsigned long)footer.developer_offset + 2ul + (unsigned long)count * 10ul <
        (unsigned long)footer.developer_offset ||
        (unsigned long)end_pos < (unsigned long)footer.developer_offset +
                                 2ul + (unsigned long)count * 10ul) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    if (out_tag_count != 0) {
        *out_tag_count = count;
    }

    if (tags == 0 || tag_capacity == 0u) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_OK;
    }
    if (tag_capacity < count) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TOO_MANY_TAGS;
    }

    for (i = 0u; i < count; ++i) {
        if (fread(entry, 1u, sizeof(entry), fp) != sizeof(entry)) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_IO;
        }
        tags[i].tag = (unsigned)tga__read_u16_le(entry + 0u);
        tags[i].data_offset = tga__read_u32_le(entry + 2u);
        tags[i].data_size = tga__read_u32_le(entry + 6u);
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    return TGA_OK;
}

int tga_find_developer_tag_file(FILE *fp, unsigned tag, tga_developer_tag *out_tag)
{
    long saved_pos;
    long end_pos;
    tga_footer footer;
    tga_u8 buf2[2];
    tga_u8 entry[10];
    unsigned count;
    unsigned i;
    int rc;

    if (fp == 0 || out_tag == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_tag, 0, sizeof(*out_tag));

    rc = tga_read_footer_file(fp, &footer);
    if (rc != TGA_OK) {
        return rc;
    }
    if (footer.developer_offset == 0ul) {
        return TGA_ERR_NO_DEVELOPER_AREA;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if ((long)footer.developer_offset + 2L > end_pos) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    if (fseek(fp, (long)footer.developer_offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if (fread(buf2, 1u, sizeof(buf2), fp) != sizeof(buf2)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    count = (unsigned)tga__read_u16_le(buf2);

    if ((unsigned long)footer.developer_offset + 2ul + (unsigned long)count * 10ul <
        (unsigned long)footer.developer_offset ||
        (unsigned long)end_pos < (unsigned long)footer.developer_offset +
                                 2ul + (unsigned long)count * 10ul) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    for (i = 0u; i < count; ++i) {
        unsigned long data_end;

        if (fread(entry, 1u, sizeof(entry), fp) != sizeof(entry)) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_IO;
        }
        if ((unsigned)tga__read_u16_le(entry + 0u) != tag) {
            continue;
        }

        out_tag->tag = tag;
        out_tag->data_offset = tga__read_u32_le(entry + 2u);
        out_tag->data_size = tga__read_u32_le(entry + 6u);
        data_end = (unsigned long)out_tag->data_offset +
                   (unsigned long)out_tag->data_size;
        if (data_end < (unsigned long)out_tag->data_offset ||
            data_end > (unsigned long)end_pos) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            memset(out_tag, 0, sizeof(*out_tag));
            return TGA_ERR_TRUNCATED;
        }

        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_OK;
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    return TGA_ERR_NOT_FOUND;
}

int tga_read_developer_field_file(FILE *fp,
                                  const tga_developer_tag *tag,
                                  void *dst,
                                  size_t dst_capacity,
                                  size_t *out_field_size)
{
    long saved_pos;
    long end_pos;

    if (fp == 0 || tag == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    if (out_field_size != 0) {
        *out_field_size = (size_t)tag->data_size;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if ((unsigned long)tag->data_offset + (unsigned long)tag->data_size <
        (unsigned long)tag->data_offset ||
        (unsigned long)tag->data_offset + (unsigned long)tag->data_size >
        (unsigned long)end_pos) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    if (dst == 0 || dst_capacity == 0u) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return tag->data_size == 0ul ? TGA_OK : (dst == 0 && dst_capacity == 0u ? TGA_OK : TGA_ERR_CAPACITY);
    }
    if (dst_capacity < (size_t)tag->data_size) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_CAPACITY;
    }

    if (fseek(fp, (long)tag->data_offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    if (tag->data_size != 0ul &&
        fread(dst, 1u, (size_t)tag->data_size, fp) != (size_t)tag->data_size) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    return TGA_OK;
}

int tga_read_developer_tag_file(FILE *fp, unsigned tag,
                                void *dst,
                                size_t dst_capacity,
                                size_t *out_field_size)
{
    tga_developer_tag info;
    int rc;

    rc = tga_find_developer_tag_file(fp, tag, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga_read_developer_field_file(fp, &info,
                                         dst, dst_capacity, out_field_size);
}

int tga_probe_file(FILE *fp, tga_info *out_info)
{
    long saved_pos;
    long end_pos;
    tga_u8 hdr[18];
    tga_u8 footer[26];
    int rc;

    if (fp == 0 || out_info == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    if (fseek(fp, 0L, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    if (fread(hdr, 1u, sizeof(hdr), fp) != sizeof(hdr)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__parse_header(hdr, out_info);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    end_pos = ftell(fp);
    if (end_pos >= 26L) {
        if (fseek(fp, end_pos - 26L, SEEK_SET) == 0) {
            if (fread(footer, 1u, sizeof(footer), fp) == sizeof(footer)) {
                if (memcmp(footer + 8u, tga__footer_sig, 18u) == 0) {
                    out_info->has_footer = 1;
                }
            }
        }
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    return TGA_OK;
}


int tga_inspect_file(FILE *fp, tga_inspect *out_inspect)
{
    tga_info info;
    tga_footer footer;
    tga_u8 buf[10];
    size_t color_map_size;
    size_t image_data_offset;
    size_t image_data_size;
    size_t ext_size;
    size_t dir_size;
    size_t tag_bytes;
    size_t min_payload;
    size_t max_payload;
    size_t field_offset;
    size_t field_size;
    long saved_pos;
    long end_pos;
    unsigned count;
    unsigned i;
    int rc;

    if (fp == 0 || out_inspect == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_inspect, 0, sizeof(*out_inspect));

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    if (end_pos < 0L) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    out_inspect->file_size = (size_t)end_pos;
    (void)fseek(fp, saved_pos, SEEK_SET);

    rc = tga_probe_file(fp, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    out_inspect->info = info;

    out_inspect->image_id_offset = 18u;
    out_inspect->image_id_size = (size_t)info.id_length;
    if (!tga__region_fits(out_inspect->image_id_offset,
                          out_inspect->image_id_size,
                          out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__color_map_size_from_info(&info, &color_map_size)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->color_map_offset = out_inspect->image_id_offset + out_inspect->image_id_size;
    out_inspect->color_map_size = color_map_size;
    if (!tga__region_fits(out_inspect->color_map_offset,
                          out_inspect->color_map_size,
                          out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__image_data_offset_from_info(&info, &image_data_offset)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->image_data_offset = image_data_offset;
    if (!tga__region_fits(image_data_offset, 0u, out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__validate_decode_info(&info);
    if (rc == TGA_OK) {
        rc = tga__measure_image_data_size_file(fp, &info,
                                               image_data_offset,
                                               &image_data_size);
        if (rc != TGA_OK) {
            return rc;
        }
        out_inspect->image_data_size = image_data_size;
    }

    rc = tga_read_footer_file(fp, &footer);
    if (rc == TGA_OK) {
        out_inspect->footer = footer;
        out_inspect->is_tga2 = 1;
        out_inspect->footer_offset = out_inspect->file_size - 26u;
        out_inspect->footer_size = 26u;
    } else if (rc != TGA_ERR_NO_FOOTER) {
        return rc;
    }

    if (out_inspect->is_tga2 && footer.extension_offset != 0ul) {
        rc = tga__read_section_file_impl(fp,
                                         (size_t)footer.extension_offset,
                                         2u,
                                         buf,
                                         2u,
                                         0);
        if (rc != TGA_OK) {
            return rc;
        }
        ext_size = (size_t)tga__read_u16_le(buf);
        if (ext_size < 495u) {
            return TGA_ERR_BAD_FORMAT;
        }
        if (!tga__region_fits((size_t)footer.extension_offset,
                              ext_size,
                              out_inspect->file_size)) {
            return TGA_ERR_TRUNCATED;
        }

        out_inspect->has_extension = 1;
        out_inspect->extension_area_offset = (size_t)footer.extension_offset;
        out_inspect->extension_area_size = ext_size;

        rc = tga_read_extension_file(fp, &out_inspect->extension);
        if (rc != TGA_OK) {
            return rc;
        }

        if (out_inspect->extension.scan_line_offset != 0ul) {
            if (!tga__checked_mul_size((size_t)info.height, 4u,
                                       &out_inspect->scan_line_table_size)) {
                return TGA_ERR_OVERFLOW;
            }
            out_inspect->scan_line_table_offset =
                (size_t)out_inspect->extension.scan_line_offset;
            if (!tga__region_fits(out_inspect->scan_line_table_offset,
                                  out_inspect->scan_line_table_size,
                                  out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            out_inspect->has_scan_line_table = 1;
        }

        if (out_inspect->extension.postage_stamp_offset != 0ul) {
            rc = tga_read_postage_stamp_info_file(fp,
                                                  &out_inspect->postage_stamp_info);
            if (rc != TGA_OK) {
                return rc;
            }
            out_inspect->postage_stamp_offset =
                (size_t)out_inspect->extension.postage_stamp_offset;
            out_inspect->postage_stamp_size =
                out_inspect->postage_stamp_info.block_size;
            out_inspect->has_postage_stamp = 1;
        }

        if (out_inspect->extension.color_correction_offset != 0ul) {
            out_inspect->color_correction_offset =
                (size_t)out_inspect->extension.color_correction_offset;
            out_inspect->color_correction_size =
                (size_t)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8u;
            if (!tga__region_fits(out_inspect->color_correction_offset,
                                  out_inspect->color_correction_size,
                                  out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            out_inspect->has_color_correction_table = 1;
        }
    }

    if (out_inspect->is_tga2 && footer.developer_offset != 0ul) {
        rc = tga__read_section_file_impl(fp,
                                         (size_t)footer.developer_offset,
                                         2u,
                                         buf,
                                         2u,
                                         0);
        if (rc != TGA_OK) {
            return rc;
        }
        count = (unsigned)tga__read_u16_le(buf);
        if (!tga__checked_mul_size((size_t)count, 10u, &tag_bytes)) {
            return TGA_ERR_OVERFLOW;
        }
        dir_size = 2u + tag_bytes;
        if (!tga__region_fits((size_t)footer.developer_offset,
                              dir_size,
                              out_inspect->file_size)) {
            return TGA_ERR_TRUNCATED;
        }

        out_inspect->has_developer_directory = 1;
        out_inspect->developer_directory_offset = (size_t)footer.developer_offset;
        out_inspect->developer_directory_size = dir_size;
        out_inspect->developer_tag_count = count;

        min_payload = (size_t)-1;
        max_payload = 0u;
        for (i = 0u; i < count; ++i) {
            rc = tga__read_section_file_impl(fp,
                                             (size_t)footer.developer_offset + 2u + (size_t)i * 10u,
                                             10u,
                                             buf,
                                             10u,
                                             0);
            if (rc != TGA_OK) {
                return rc;
            }
            field_offset = (size_t)tga__read_u32_le(buf + 2u);
            field_size = (size_t)tga__read_u32_le(buf + 6u);
            if (!tga__region_fits(field_offset, field_size, out_inspect->file_size)) {
                return TGA_ERR_TRUNCATED;
            }
            if (field_size == 0u) {
                continue;
            }
            if (min_payload == (size_t)-1 || field_offset < min_payload) {
                min_payload = field_offset;
            }
            if (field_offset + field_size > max_payload) {
                max_payload = field_offset + field_size;
            }
        }

        if (min_payload != (size_t)-1) {
            out_inspect->has_developer_payload = 1;
            out_inspect->developer_payload_offset = min_payload;
            out_inspect->developer_payload_size = max_payload - min_payload;
        }
    }

    return TGA_OK;
}

int tga_read_image_id_file(FILE *fp,
                           void *dst,
                           size_t dst_capacity,
                           size_t *out_id_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.image_id_offset,
                                       inspect.image_id_size,
                                       dst,
                                       dst_capacity,
                                       out_id_size);
}

int tga_read_color_map_file(FILE *fp,
                            void *dst,
                            size_t dst_capacity,
                            size_t *out_color_map_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.color_map_offset,
                                       inspect.color_map_size,
                                       dst,
                                       dst_capacity,
                                       out_color_map_size);
}

int tga_read_image_data_file(FILE *fp,
                             void *dst,
                             size_t dst_capacity,
                             size_t *out_image_data_size)
{
    tga_inspect inspect;
    int rc;

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga__read_section_file_impl(fp,
                                       inspect.image_data_offset,
                                       inspect.image_data_size,
                                       dst,
                                       dst_capacity,
                                       out_image_data_size);
}
#endif

static tga_u8 *tga__rows_output_row_ptr(const tga_decode_rows_params *params,
                                        unsigned logical_row)
{
    unsigned rel_row;
    unsigned dst_row;

    rel_row = logical_row - params->first_row;
    if (params->output_top_origin) {
        dst_row = rel_row;
    } else {
        dst_row = params->row_count - 1u - rel_row;
    }

    return (tga_u8 *)params->pixels + (size_t)dst_row * (size_t)params->stride_bytes;
}

static int tga__decode_sample_to_rows_output(const tga_info *info,
                                             const tga_decode_rows_params *params,
                                             tga_u8 *dst_row,
                                             unsigned logical_x,
                                             const tga_u8 *sample)
{
    tga__rgba rgba;
    int rc;

    if (info->is_color_mapped) {
        rc = tga__decode_index_sample(info, sample,
                                      params->palette_rgba,
                                      &rgba);
        if (rc != TGA_OK) {
            return rc;
        }
    } else {
        tga__decode_color_sample(sample, info->pixel_depth,
                                 info->descriptor & 0x0Fu,
                                 info->is_grayscale,
                                 &rgba);
    }

    tga__store_output_pixel(dst_row, logical_x, params->pixel_format, &rgba);
    return TGA_OK;
}

static int tga__decode_saved_row_raw_memory(const tga_u8 *src_row,
                                            const tga_info *info,
                                            const tga_decode_rows_params *params,
                                            unsigned logical_row)
{
    unsigned sample_bytes;
    unsigned saved_col;
    unsigned logical_x;
    tga_u8 *dst_row;
    int rc;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    dst_row = tga__rows_output_row_ptr(params, logical_row);

    for (saved_col = 0u; saved_col < info->width; ++saved_col) {
        if (info->file_right_to_left) {
            logical_x = info->width - 1u - saved_col;
        } else {
            logical_x = saved_col;
        }

        rc = tga__decode_sample_to_rows_output(info, params,
                                               dst_row,
                                               logical_x,
                                               src_row + (size_t)saved_col * (size_t)sample_bytes);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    return TGA_OK;
}

static int tga__decode_saved_row_rle_memory(const tga_u8 *bytes,
                                            size_t src_size,
                                            size_t row_offset,
                                            size_t row_limit,
                                            const tga_info *info,
                                            const tga_decode_rows_params *params,
                                            unsigned logical_row)
{
    size_t pos;
    unsigned sample_bytes;
    unsigned produced;
    unsigned packet_header;
    unsigned count;
    unsigned i;
    unsigned saved_col;
    unsigned logical_x;
    size_t need;
    tga_u8 *dst_row;
    int rc;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    dst_row = tga__rows_output_row_ptr(params, logical_row);
    pos = row_offset;
    produced = 0u;

    while (produced < info->width) {
        if (!tga__region_fits(pos, 1u, src_size) || pos >= row_limit) {
            return TGA_ERR_TRUNCATED;
        }

        packet_header = (unsigned)bytes[pos];
        ++pos;
        count = (packet_header & 127u) + 1u;
        if (count > info->width - produced) {
            return TGA_ERR_BAD_FORMAT;
        }

        if ((packet_header & 128u) != 0u) {
            need = (size_t)sample_bytes;
            if (!tga__region_fits(pos, need, src_size) || pos + need > row_limit) {
                return TGA_ERR_TRUNCATED;
            }

            for (i = 0u; i < count; ++i) {
                saved_col = produced + i;
                if (info->file_right_to_left) {
                    logical_x = info->width - 1u - saved_col;
                } else {
                    logical_x = saved_col;
                }

                rc = tga__decode_sample_to_rows_output(info, params,
                                                       dst_row,
                                                       logical_x,
                                                       bytes + pos);
                if (rc != TGA_OK) {
                    return rc;
                }
            }

            pos += need;
        } else {
            if (!tga__checked_mul_size((size_t)count,
                                       (size_t)sample_bytes,
                                       &need)) {
                return TGA_ERR_OVERFLOW;
            }
            if (!tga__region_fits(pos, need, src_size) || pos + need > row_limit) {
                return TGA_ERR_TRUNCATED;
            }

            for (i = 0u; i < count; ++i) {
                saved_col = produced + i;
                if (info->file_right_to_left) {
                    logical_x = info->width - 1u - saved_col;
                } else {
                    logical_x = saved_col;
                }

                rc = tga__decode_sample_to_rows_output(info, params,
                                                       dst_row,
                                                       logical_x,
                                                       bytes + pos + (size_t)i * (size_t)sample_bytes);
                if (rc != TGA_OK) {
                    return rc;
                }
            }

            pos += need;
        }

        produced += count;
    }

    return TGA_OK;
}

int tga_read_rle_packet_table_memory(const void *src, size_t src_size,
                                     tga_packet_info *packets,
                                     unsigned packet_capacity,
                                     unsigned *out_packet_count)
{
    const tga_u8 *bytes;
    tga_inspect inspect;
    const tga_info *info;
    size_t sample_bytes;
    size_t total_pixels;
    size_t pos;
    size_t need;
    size_t first_pixel;
    size_t last_pixel;
    unsigned packet_index;
    unsigned packet_header;
    unsigned count;
    int rc;
    int overflowed_capacity;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_packet_count != 0) {
        *out_packet_count = 0u;
    }

    rc = tga_inspect_memory(src, src_size, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }

    info = &inspect.info;
    if (!info->is_rle) {
        return TGA_ERR_UNSUPPORTED;
    }

    bytes = (const tga_u8 *)src;
    sample_bytes = (size_t)tga__file_sample_bytes(info->pixel_depth);
    if (!tga__checked_mul_size((size_t)info->width,
                               (size_t)info->height,
                               &total_pixels)) {
        return TGA_ERR_OVERFLOW;
    }

    pos = inspect.image_data_offset;
    first_pixel = 0u;
    packet_index = 0u;
    overflowed_capacity = 0;

    while (first_pixel < total_pixels) {
        if (!tga__region_fits(pos, 1u, src_size)) {
            return TGA_ERR_TRUNCATED;
        }

        packet_header = (unsigned)bytes[pos];
        ++pos;
        count = (packet_header & 127u) + 1u;
        if ((size_t)count > total_pixels - first_pixel) {
            return TGA_ERR_BAD_FORMAT;
        }

        if ((packet_header & 128u) != 0u) {
            need = sample_bytes;
        } else {
            if (!tga__checked_mul_size((size_t)count, sample_bytes, &need)) {
                return TGA_ERR_OVERFLOW;
            }
        }
        if (!tga__region_fits(pos, need, src_size)) {
            return TGA_ERR_TRUNCATED;
        }

        last_pixel = first_pixel + (size_t)count - 1u;

        if (packets != 0 && packet_capacity != 0u) {
            if (packet_index < packet_capacity) {
                packets[packet_index].header_offset = pos - 1u;
                packets[packet_index].payload_offset = pos;
                packets[packet_index].payload_size = need;
                packets[packet_index].packet_size = 1u + need;
                packets[packet_index].packet_index = packet_index;
                packets[packet_index].pixel_count = count;
                packets[packet_index].first_pixel_index = (unsigned)first_pixel;
                packets[packet_index].first_saved_row = (unsigned)(first_pixel / (size_t)info->width);
                packets[packet_index].first_saved_col = (unsigned)(first_pixel % (size_t)info->width);
                packets[packet_index].last_saved_row = (unsigned)(last_pixel / (size_t)info->width);
                packets[packet_index].last_saved_col = (unsigned)(last_pixel % (size_t)info->width);
                packets[packet_index].is_rle = (packet_header & 128u) != 0u;
                packets[packet_index].crosses_scanline =
                    packets[packet_index].first_saved_row != packets[packet_index].last_saved_row;
            } else {
                overflowed_capacity = 1;
            }
        }

        pos += need;
        first_pixel += (size_t)count;
        ++packet_index;
    }

    if (out_packet_count != 0) {
        *out_packet_count = packet_index;
    }
    if (overflowed_capacity) {
        return TGA_ERR_CAPACITY;
    }
    return TGA_OK;
}

int tga_decode_rows_memory(const void *src, size_t src_size,
                           const tga_decode_rows_params *params,
                           tga_info *out_info)
{
    const tga_u8 *bytes;
    tga_inspect inspect;
    tga_mem_reader palette_mem;
    tga_reader reader;
    tga_info info;
    size_t out_bpp;
    size_t min_stride;
    size_t sample_bytes;
    size_t row_bytes;
    size_t image_end;
    size_t row_offset;
    size_t next_row_offset;
    size_t rel_saved_row_offset;
    unsigned logical_row;
    unsigned saved_row;
    int rc;

    if (src == 0 || params == 0 || params->pixels == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__pixel_format_valid(params->pixel_format) ||
        params->pixel_format == TGA_PIXFMT_INDEX8) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (params->row_count == 0u) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga_inspect_memory(src, src_size, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }

    info = inspect.info;
    if (params->first_row >= info.height ||
        params->row_count > info.height - params->first_row) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    out_bpp = (size_t)tga_bytes_per_pixel(params->pixel_format);
    if (!tga__checked_mul_size((size_t)info.width, out_bpp, &min_stride)) {
        return TGA_ERR_OVERFLOW;
    }
    if ((size_t)params->stride_bytes < min_stride) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    bytes = (const tga_u8 *)src;
    palette_mem.data = bytes + 18u;
    palette_mem.size = src_size - 18u;
    palette_mem.pos = 0u;
    reader.read = tga_mem_read;
    reader.user = &palette_mem;

    if (!tga__skip(&reader, (size_t)info.id_length)) {
        return TGA_ERR_TRUNCATED;
    }
    rc = tga__load_palette(&reader, &info,
                           params->palette_rgba,
                           params->palette_capacity);
    if (rc != TGA_OK) {
        return rc;
    }

    sample_bytes = (size_t)tga__file_sample_bytes(info.pixel_depth);
    if (!tga__checked_mul_size((size_t)info.width, sample_bytes, &row_bytes)) {
        return TGA_ERR_OVERFLOW;
    }
    image_end = inspect.image_data_offset + inspect.image_data_size;

    for (logical_row = params->first_row;
         logical_row < params->first_row + params->row_count;
         ++logical_row) {
        if (info.file_top_origin) {
            saved_row = logical_row;
        } else {
            saved_row = info.height - 1u - logical_row;
        }

        if (!info.is_rle) {
            if (!tga__checked_mul_size((size_t)saved_row,
                                       row_bytes,
                                       &rel_saved_row_offset)) {
                return TGA_ERR_OVERFLOW;
            }
            if (inspect.image_data_offset > ((size_t)-1) - rel_saved_row_offset) {
                return TGA_ERR_OVERFLOW;
            }
            row_offset = inspect.image_data_offset + rel_saved_row_offset;
            if (!tga__region_fits(row_offset, row_bytes, src_size)) {
                return TGA_ERR_TRUNCATED;
            }

            rc = tga__decode_saved_row_raw_memory(bytes + row_offset,
                                                  &info,
                                                  params,
                                                  logical_row);
            if (rc != TGA_OK) {
                return rc;
            }
        } else {
            if (!inspect.has_scan_line_table) {
                return TGA_ERR_NO_SCAN_LINE_TABLE;
            }
            row_offset = (size_t)tga__read_u32_le(bytes + inspect.scan_line_table_offset + (size_t)saved_row * 4u);
            if (saved_row + 1u < info.height) {
                next_row_offset = (size_t)tga__read_u32_le(bytes + inspect.scan_line_table_offset + (size_t)(saved_row + 1u) * 4u);
            } else {
                next_row_offset = image_end;
            }
            if (row_offset < inspect.image_data_offset ||
                row_offset > image_end ||
                next_row_offset < row_offset ||
                next_row_offset > image_end) {
                return TGA_ERR_BAD_FORMAT;
            }

            rc = tga__decode_saved_row_rle_memory(bytes, src_size,
                                                  row_offset,
                                                  next_row_offset,
                                                  &info,
                                                  params,
                                                  logical_row);
            if (rc != TGA_OK) {
                return rc;
            }
        }
    }

    if (out_info != 0) {
        *out_info = info;
    }
    return TGA_OK;
}

static tga_u8 *tga__rect_output_row_ptr(const tga_decode_rect_params *params,
                                        unsigned logical_row)
{
    unsigned rel_row;
    unsigned dst_row;

    rel_row = logical_row - params->first_row;
    if (params->output_top_origin) {
        dst_row = rel_row;
    } else {
        dst_row = params->row_count - 1u - rel_row;
    }

    return (tga_u8 *)params->pixels + (size_t)dst_row * (size_t)params->stride_bytes;
}

static int tga__decode_sample_to_rect_output(const tga_info *info,
                                             const tga_decode_rect_params *params,
                                             tga_u8 *dst_row,
                                             unsigned logical_x,
                                             const tga_u8 *sample)
{
    tga__rgba rgba;
    unsigned rel_x;
    int rc;

    if (logical_x < params->first_col ||
        logical_x >= params->first_col + params->col_count) {
        return TGA_OK;
    }

    if (info->is_color_mapped) {
        rc = tga__decode_index_sample(info, sample,
                                      params->palette_rgba,
                                      &rgba);
        if (rc != TGA_OK) {
            return rc;
        }
    } else {
        tga__decode_color_sample(sample, info->pixel_depth,
                                 info->descriptor & 0x0Fu,
                                 info->is_grayscale,
                                 &rgba);
    }

    rel_x = logical_x - params->first_col;
    tga__store_output_pixel(dst_row, rel_x, params->pixel_format, &rgba);
    return TGA_OK;
}

static int tga__decode_saved_row_raw_rect_memory(const tga_u8 *src_row,
                                                 const tga_info *info,
                                                 const tga_decode_rect_params *params,
                                                 unsigned logical_row)
{
    unsigned sample_bytes;
    unsigned saved_col;
    unsigned logical_x;
    tga_u8 *dst_row;
    int rc;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    dst_row = tga__rect_output_row_ptr(params, logical_row);

    for (saved_col = 0u; saved_col < info->width; ++saved_col) {
        if (info->file_right_to_left) {
            logical_x = info->width - 1u - saved_col;
        } else {
            logical_x = saved_col;
        }

        rc = tga__decode_sample_to_rect_output(info, params,
                                               dst_row,
                                               logical_x,
                                               src_row + (size_t)saved_col * (size_t)sample_bytes);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    return TGA_OK;
}

static int tga__decode_saved_row_rle_rect_memory(const tga_u8 *bytes,
                                                 size_t src_size,
                                                 size_t row_offset,
                                                 size_t row_limit,
                                                 const tga_info *info,
                                                 const tga_decode_rect_params *params,
                                                 unsigned logical_row)
{
    size_t pos;
    unsigned sample_bytes;
    unsigned produced;
    unsigned packet_header;
    unsigned count;
    unsigned i;
    unsigned saved_col;
    unsigned logical_x;
    size_t need;
    tga_u8 *dst_row;
    int rc;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    dst_row = tga__rect_output_row_ptr(params, logical_row);
    pos = row_offset;
    produced = 0u;

    while (produced < info->width) {
        if (!tga__region_fits(pos, 1u, src_size) || pos >= row_limit) {
            return TGA_ERR_TRUNCATED;
        }

        packet_header = (unsigned)bytes[pos];
        ++pos;
        count = (packet_header & 127u) + 1u;
        if (count > info->width - produced) {
            return TGA_ERR_BAD_FORMAT;
        }

        if ((packet_header & 128u) != 0u) {
            need = (size_t)sample_bytes;
            if (!tga__region_fits(pos, need, src_size) || pos + need > row_limit) {
                return TGA_ERR_TRUNCATED;
            }

            for (i = 0u; i < count; ++i) {
                saved_col = produced + i;
                if (info->file_right_to_left) {
                    logical_x = info->width - 1u - saved_col;
                } else {
                    logical_x = saved_col;
                }

                rc = tga__decode_sample_to_rect_output(info, params,
                                                       dst_row,
                                                       logical_x,
                                                       bytes + pos);
                if (rc != TGA_OK) {
                    return rc;
                }
            }

            pos += need;
        } else {
            if (!tga__checked_mul_size((size_t)count,
                                       (size_t)sample_bytes,
                                       &need)) {
                return TGA_ERR_OVERFLOW;
            }
            if (!tga__region_fits(pos, need, src_size) || pos + need > row_limit) {
                return TGA_ERR_TRUNCATED;
            }

            for (i = 0u; i < count; ++i) {
                saved_col = produced + i;
                if (info->file_right_to_left) {
                    logical_x = info->width - 1u - saved_col;
                } else {
                    logical_x = saved_col;
                }

                rc = tga__decode_sample_to_rect_output(info, params,
                                                       dst_row,
                                                       logical_x,
                                                       bytes + pos + (size_t)i * (size_t)sample_bytes);
                if (rc != TGA_OK) {
                    return rc;
                }
            }

            pos += need;
        }

        produced += count;
    }

    return TGA_OK;
}

int tga_decode_rect_memory(const void *src, size_t src_size,
                           const tga_decode_rect_params *params,
                           tga_info *out_info)
{
    const tga_u8 *bytes;
    tga_inspect inspect;
    tga_mem_reader palette_mem;
    tga_reader reader;
    tga_info info;
    size_t out_bpp;
    size_t min_stride;
    size_t sample_bytes;
    size_t row_bytes;
    size_t image_end;
    size_t row_offset;
    size_t next_row_offset;
    size_t rel_saved_row_offset;
    unsigned logical_row;
    unsigned saved_row;
    int rc;

    if (src == 0 || params == 0 || params->pixels == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__pixel_format_valid(params->pixel_format) ||
        params->pixel_format == TGA_PIXFMT_INDEX8) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (params->row_count == 0u || params->col_count == 0u) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga_inspect_memory(src, src_size, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }

    info = inspect.info;
    if (params->first_row >= info.height ||
        params->row_count > info.height - params->first_row ||
        params->first_col >= info.width ||
        params->col_count > info.width - params->first_col) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    out_bpp = (size_t)tga_bytes_per_pixel(params->pixel_format);
    if (!tga__checked_mul_size((size_t)params->col_count, out_bpp, &min_stride)) {
        return TGA_ERR_OVERFLOW;
    }
    if ((size_t)params->stride_bytes < min_stride) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    bytes = (const tga_u8 *)src;
    palette_mem.data = bytes + 18u;
    palette_mem.size = src_size - 18u;
    palette_mem.pos = 0u;
    reader.read = tga_mem_read;
    reader.user = &palette_mem;

    if (!tga__skip(&reader, (size_t)info.id_length)) {
        return TGA_ERR_TRUNCATED;
    }
    rc = tga__load_palette(&reader, &info,
                           params->palette_rgba,
                           params->palette_capacity);
    if (rc != TGA_OK) {
        return rc;
    }

    sample_bytes = (size_t)tga__file_sample_bytes(info.pixel_depth);
    if (!tga__checked_mul_size((size_t)info.width, sample_bytes, &row_bytes)) {
        return TGA_ERR_OVERFLOW;
    }
    image_end = inspect.image_data_offset + inspect.image_data_size;

    for (logical_row = params->first_row;
         logical_row < params->first_row + params->row_count;
         ++logical_row) {
        if (info.file_top_origin) {
            saved_row = logical_row;
        } else {
            saved_row = info.height - 1u - logical_row;
        }

        if (!info.is_rle) {
            if (!tga__checked_mul_size((size_t)saved_row,
                                       row_bytes,
                                       &rel_saved_row_offset)) {
                return TGA_ERR_OVERFLOW;
            }
            if (inspect.image_data_offset > ((size_t)-1) - rel_saved_row_offset) {
                return TGA_ERR_OVERFLOW;
            }
            row_offset = inspect.image_data_offset + rel_saved_row_offset;
            if (!tga__region_fits(row_offset, row_bytes, src_size)) {
                return TGA_ERR_TRUNCATED;
            }

            rc = tga__decode_saved_row_raw_rect_memory(bytes + row_offset,
                                                       &info,
                                                       params,
                                                       logical_row);
            if (rc != TGA_OK) {
                return rc;
            }
        } else {
            if (!inspect.has_scan_line_table) {
                return TGA_ERR_NO_SCAN_LINE_TABLE;
            }
            row_offset = (size_t)tga__read_u32_le(bytes + inspect.scan_line_table_offset + (size_t)saved_row * 4u);
            if (saved_row + 1u < info.height) {
                next_row_offset = (size_t)tga__read_u32_le(bytes + inspect.scan_line_table_offset + (size_t)(saved_row + 1u) * 4u);
            } else {
                next_row_offset = image_end;
            }
            if (row_offset < inspect.image_data_offset ||
                row_offset > image_end ||
                next_row_offset < row_offset ||
                next_row_offset > image_end) {
                return TGA_ERR_BAD_FORMAT;
            }

            rc = tga__decode_saved_row_rle_rect_memory(bytes, src_size,
                                                       row_offset,
                                                       next_row_offset,
                                                       &info,
                                                       params,
                                                       logical_row);
            if (rc != TGA_OK) {
                return rc;
            }
        }
    }

    if (out_info != 0) {
        *out_info = info;
    }
    return TGA_OK;
}

#ifndef GAFNYF_TGA_NO_STDIO
static int tga__decode_saved_row_raw_file(FILE *fp,
                                          const tga_info *info,
                                          const tga_decode_rows_params *params,
                                          unsigned logical_row)
{
    unsigned sample_bytes;
    unsigned saved_col;
    unsigned logical_x;
    tga_u8 *dst_row;
    tga_u8 sample[4];
    int rc;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    dst_row = tga__rows_output_row_ptr(params, logical_row);

    for (saved_col = 0u; saved_col < info->width; ++saved_col) {
        if (fread(sample, 1u, sample_bytes, fp) != sample_bytes) {
            return TGA_ERR_IO;
        }

        if (info->file_right_to_left) {
            logical_x = info->width - 1u - saved_col;
        } else {
            logical_x = saved_col;
        }

        rc = tga__decode_sample_to_rows_output(info, params,
                                               dst_row,
                                               logical_x,
                                               sample);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    return TGA_OK;
}

static int tga__decode_saved_row_rle_file(FILE *fp,
                                          const tga_info *info,
                                          const tga_decode_rows_params *params,
                                          unsigned logical_row)
{
    unsigned sample_bytes;
    unsigned produced;
    unsigned packet_header;
    unsigned count;
    unsigned i;
    unsigned saved_col;
    unsigned logical_x;
    tga_u8 *dst_row;
    tga_u8 sample[4];
    int rc;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    dst_row = tga__rows_output_row_ptr(params, logical_row);
    produced = 0u;

    while (produced < info->width) {
        if (fread(sample, 1u, 1u, fp) != 1u) {
            return TGA_ERR_IO;
        }
        packet_header = (unsigned)sample[0];
        count = (packet_header & 127u) + 1u;
        if (count > info->width - produced) {
            return TGA_ERR_BAD_FORMAT;
        }

        if ((packet_header & 128u) != 0u) {
            if (fread(sample, 1u, sample_bytes, fp) != sample_bytes) {
                return TGA_ERR_IO;
            }
            for (i = 0u; i < count; ++i) {
                saved_col = produced + i;
                if (info->file_right_to_left) {
                    logical_x = info->width - 1u - saved_col;
                } else {
                    logical_x = saved_col;
                }

                rc = tga__decode_sample_to_rows_output(info, params,
                                                       dst_row,
                                                       logical_x,
                                                       sample);
                if (rc != TGA_OK) {
                    return rc;
                }
            }
        } else {
            for (i = 0u; i < count; ++i) {
                if (fread(sample, 1u, sample_bytes, fp) != sample_bytes) {
                    return TGA_ERR_IO;
                }
                saved_col = produced + i;
                if (info->file_right_to_left) {
                    logical_x = info->width - 1u - saved_col;
                } else {
                    logical_x = saved_col;
                }

                rc = tga__decode_sample_to_rows_output(info, params,
                                                       dst_row,
                                                       logical_x,
                                                       sample);
                if (rc != TGA_OK) {
                    return rc;
                }
            }
        }

        produced += count;
    }

    return TGA_OK;
}

int tga_read_rle_packet_table_file(FILE *fp,
                                   tga_packet_info *packets,
                                   unsigned packet_capacity,
                                   unsigned *out_packet_count)
{
    tga_inspect inspect;
    const tga_info *info;
    size_t sample_bytes;
    size_t total_pixels;
    size_t first_pixel;
    size_t need;
    size_t last_pixel;
    unsigned packet_index;
    unsigned packet_header;
    unsigned count;
    long saved_pos;
    long header_pos;
    long payload_pos;
    tga_u8 header_byte;
    int rc;
    int overflowed_capacity;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_packet_count != 0) {
        *out_packet_count = 0u;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }

    info = &inspect.info;
    if (!info->is_rle) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_UNSUPPORTED;
    }

    sample_bytes = (size_t)tga__file_sample_bytes(info->pixel_depth);
    if (!tga__checked_mul_size((size_t)info->width,
                               (size_t)info->height,
                               &total_pixels)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_OVERFLOW;
    }

    if (fseek(fp, (long)inspect.image_data_offset, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }

    first_pixel = 0u;
    packet_index = 0u;
    overflowed_capacity = 0;

    while (first_pixel < total_pixels) {
        header_pos = ftell(fp);
        if (header_pos < 0L) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_IO;
        }
        if (fread(&header_byte, 1u, 1u, fp) != 1u) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_IO;
        }
        payload_pos = ftell(fp);
        if (payload_pos < 0L) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_IO;
        }

        packet_header = (unsigned)header_byte;
        count = (packet_header & 127u) + 1u;
        if ((size_t)count > total_pixels - first_pixel) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_BAD_FORMAT;
        }

        if ((packet_header & 128u) != 0u) {
            need = sample_bytes;
        } else {
            if (!tga__checked_mul_size((size_t)count, sample_bytes, &need)) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_OVERFLOW;
            }
        }

        if (need != 0u && fseek(fp, (long)need, SEEK_CUR) != 0) {
            (void)fseek(fp, saved_pos, SEEK_SET);
            return TGA_ERR_IO;
        }

        last_pixel = first_pixel + (size_t)count - 1u;

        if (packets != 0 && packet_capacity != 0u) {
            if (packet_index < packet_capacity) {
                packets[packet_index].header_offset = (size_t)header_pos;
                packets[packet_index].payload_offset = (size_t)payload_pos;
                packets[packet_index].payload_size = need;
                packets[packet_index].packet_size = 1u + need;
                packets[packet_index].packet_index = packet_index;
                packets[packet_index].pixel_count = count;
                packets[packet_index].first_pixel_index = (unsigned)first_pixel;
                packets[packet_index].first_saved_row = (unsigned)(first_pixel / (size_t)info->width);
                packets[packet_index].first_saved_col = (unsigned)(first_pixel % (size_t)info->width);
                packets[packet_index].last_saved_row = (unsigned)(last_pixel / (size_t)info->width);
                packets[packet_index].last_saved_col = (unsigned)(last_pixel % (size_t)info->width);
                packets[packet_index].is_rle = (packet_header & 128u) != 0u;
                packets[packet_index].crosses_scanline =
                    packets[packet_index].first_saved_row != packets[packet_index].last_saved_row;
            } else {
                overflowed_capacity = 1;
            }
        }

        first_pixel += (size_t)count;
        ++packet_index;
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    if (out_packet_count != 0) {
        *out_packet_count = packet_index;
    }
    if (overflowed_capacity) {
        return TGA_ERR_CAPACITY;
    }
    return TGA_OK;
}

int tga_decode_rows_file(FILE *fp,
                         const tga_decode_rows_params *params,
                         tga_info *out_info)
{
    tga_inspect inspect;
    tga_info info;
    tga_file_reader fr;
    tga_reader reader;
    size_t out_bpp;
    size_t min_stride;
    size_t sample_bytes;
    size_t row_bytes;
    size_t image_end;
    size_t row_offset;
    size_t next_row_offset;
    size_t rel_saved_row_offset;
    long saved_pos;
    unsigned logical_row;
    unsigned saved_row;
    int rc;

    if (fp == 0 || params == 0 || params->pixels == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__pixel_format_valid(params->pixel_format) ||
        params->pixel_format == TGA_PIXFMT_INDEX8) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (params->row_count == 0u) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }

    info = inspect.info;
    if (params->first_row >= info.height ||
        params->row_count > info.height - params->first_row) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_BAD_ARGUMENT;
    }

    out_bpp = (size_t)tga_bytes_per_pixel(params->pixel_format);
    if (!tga__checked_mul_size((size_t)info.width, out_bpp, &min_stride)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_OVERFLOW;
    }
    if ((size_t)params->stride_bytes < min_stride) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_BAD_ARGUMENT;
    }

    if (fseek(fp, 18L, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    fr.fp = fp;
    reader.read = tga_file_read;
    reader.user = &fr;

    if (!tga__skip(&reader, (size_t)info.id_length)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }
    rc = tga__load_palette(&reader, &info,
                           params->palette_rgba,
                           params->palette_capacity);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }

    sample_bytes = (size_t)tga__file_sample_bytes(info.pixel_depth);
    if (!tga__checked_mul_size((size_t)info.width, sample_bytes, &row_bytes)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_OVERFLOW;
    }
    image_end = inspect.image_data_offset + inspect.image_data_size;

    for (logical_row = params->first_row;
         logical_row < params->first_row + params->row_count;
         ++logical_row) {
        if (info.file_top_origin) {
            saved_row = logical_row;
        } else {
            saved_row = info.height - 1u - logical_row;
        }

        if (!info.is_rle) {
            if (!tga__checked_mul_size((size_t)saved_row,
                                       row_bytes,
                                       &rel_saved_row_offset)) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_OVERFLOW;
            }
            if (inspect.image_data_offset > ((size_t)-1) - rel_saved_row_offset) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_OVERFLOW;
            }
            row_offset = inspect.image_data_offset + rel_saved_row_offset;

            if (fseek(fp, (long)row_offset, SEEK_SET) != 0) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_IO;
            }
            rc = tga__decode_saved_row_raw_file(fp, &info, params, logical_row);
            if (rc != TGA_OK) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return rc;
            }
        } else {
            if (!inspect.has_scan_line_table) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_NO_SCAN_LINE_TABLE;
            }
            if (fseek(fp, (long)inspect.scan_line_table_offset + (long)((size_t)saved_row * 4u), SEEK_SET) != 0) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_IO;
            }
            {
                tga_u8 buf[4];
                if (fread(buf, 1u, 4u, fp) != 4u) {
                    (void)fseek(fp, saved_pos, SEEK_SET);
                    return TGA_ERR_IO;
                }
                row_offset = (size_t)tga__read_u32_le(buf);
                if (saved_row + 1u < info.height) {
                    if (fread(buf, 1u, 4u, fp) != 4u) {
                        (void)fseek(fp, saved_pos, SEEK_SET);
                        return TGA_ERR_IO;
                    }
                    next_row_offset = (size_t)tga__read_u32_le(buf);
                } else {
                    next_row_offset = image_end;
                }
            }

            if (row_offset < inspect.image_data_offset ||
                row_offset > image_end ||
                next_row_offset < row_offset ||
                next_row_offset > image_end) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_BAD_FORMAT;
            }

            if (fseek(fp, (long)row_offset, SEEK_SET) != 0) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_IO;
            }
            rc = tga__decode_saved_row_rle_file(fp, &info, params, logical_row);
            if (rc != TGA_OK) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return rc;
            }
        }
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    if (out_info != 0) {
        *out_info = info;
    }
    return TGA_OK;
}

static int tga__decode_saved_row_raw_rect_file(FILE *fp,
                                               const tga_info *info,
                                               const tga_decode_rect_params *params,
                                               unsigned logical_row)
{
    unsigned sample_bytes;
    unsigned saved_col;
    unsigned logical_x;
    tga_u8 *dst_row;
    tga_u8 sample[4];
    int rc;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    dst_row = tga__rect_output_row_ptr(params, logical_row);

    for (saved_col = 0u; saved_col < info->width; ++saved_col) {
        if (fread(sample, 1u, sample_bytes, fp) != sample_bytes) {
            return TGA_ERR_IO;
        }

        if (info->file_right_to_left) {
            logical_x = info->width - 1u - saved_col;
        } else {
            logical_x = saved_col;
        }

        rc = tga__decode_sample_to_rect_output(info, params,
                                               dst_row,
                                               logical_x,
                                               sample);
        if (rc != TGA_OK) {
            return rc;
        }
    }

    return TGA_OK;
}

static int tga__decode_saved_row_rle_rect_file(FILE *fp,
                                               const tga_info *info,
                                               const tga_decode_rect_params *params,
                                               unsigned logical_row)
{
    unsigned sample_bytes;
    unsigned produced;
    unsigned packet_header;
    unsigned count;
    unsigned i;
    unsigned saved_col;
    unsigned logical_x;
    tga_u8 *dst_row;
    tga_u8 sample[4];
    int rc;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    dst_row = tga__rect_output_row_ptr(params, logical_row);
    produced = 0u;

    while (produced < info->width) {
        if (fread(sample, 1u, 1u, fp) != 1u) {
            return TGA_ERR_IO;
        }
        packet_header = (unsigned)sample[0];
        count = (packet_header & 127u) + 1u;
        if (count > info->width - produced) {
            return TGA_ERR_BAD_FORMAT;
        }

        if ((packet_header & 128u) != 0u) {
            if (fread(sample, 1u, sample_bytes, fp) != sample_bytes) {
                return TGA_ERR_IO;
            }
            for (i = 0u; i < count; ++i) {
                saved_col = produced + i;
                if (info->file_right_to_left) {
                    logical_x = info->width - 1u - saved_col;
                } else {
                    logical_x = saved_col;
                }

                rc = tga__decode_sample_to_rect_output(info, params,
                                                       dst_row,
                                                       logical_x,
                                                       sample);
                if (rc != TGA_OK) {
                    return rc;
                }
            }
        } else {
            for (i = 0u; i < count; ++i) {
                if (fread(sample, 1u, sample_bytes, fp) != sample_bytes) {
                    return TGA_ERR_IO;
                }
                saved_col = produced + i;
                if (info->file_right_to_left) {
                    logical_x = info->width - 1u - saved_col;
                } else {
                    logical_x = saved_col;
                }

                rc = tga__decode_sample_to_rect_output(info, params,
                                                       dst_row,
                                                       logical_x,
                                                       sample);
                if (rc != TGA_OK) {
                    return rc;
                }
            }
        }

        produced += count;
    }

    return TGA_OK;
}

int tga_decode_rect_file(FILE *fp,
                         const tga_decode_rect_params *params,
                         tga_info *out_info)
{
    tga_inspect inspect;
    tga_info info;
    tga_file_reader fr;
    tga_reader reader;
    size_t out_bpp;
    size_t min_stride;
    size_t sample_bytes;
    size_t row_bytes;
    size_t image_end;
    size_t row_offset;
    size_t next_row_offset;
    size_t rel_saved_row_offset;
    long saved_pos;
    unsigned logical_row;
    unsigned saved_row;
    int rc;

    if (fp == 0 || params == 0 || params->pixels == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__pixel_format_valid(params->pixel_format) ||
        params->pixel_format == TGA_PIXFMT_INDEX8) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (params->row_count == 0u || params->col_count == 0u) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }

    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }

    info = inspect.info;
    if (params->first_row >= info.height ||
        params->row_count > info.height - params->first_row ||
        params->first_col >= info.width ||
        params->col_count > info.width - params->first_col) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_BAD_ARGUMENT;
    }

    out_bpp = (size_t)tga_bytes_per_pixel(params->pixel_format);
    if (!tga__checked_mul_size((size_t)params->col_count, out_bpp, &min_stride)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_OVERFLOW;
    }
    if ((size_t)params->stride_bytes < min_stride) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_BAD_ARGUMENT;
    }

    if (fseek(fp, 18L, SEEK_SET) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    fr.fp = fp;
    reader.read = tga_file_read;
    reader.user = &fr;

    if (!tga__skip(&reader, (size_t)info.id_length)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_TRUNCATED;
    }
    rc = tga__load_palette(&reader, &info,
                           params->palette_rgba,
                           params->palette_capacity);
    if (rc != TGA_OK) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return rc;
    }

    sample_bytes = (size_t)tga__file_sample_bytes(info.pixel_depth);
    if (!tga__checked_mul_size((size_t)info.width, sample_bytes, &row_bytes)) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_OVERFLOW;
    }
    image_end = inspect.image_data_offset + inspect.image_data_size;

    for (logical_row = params->first_row;
         logical_row < params->first_row + params->row_count;
         ++logical_row) {
        if (info.file_top_origin) {
            saved_row = logical_row;
        } else {
            saved_row = info.height - 1u - logical_row;
        }

        if (!info.is_rle) {
            if (!tga__checked_mul_size((size_t)saved_row,
                                       row_bytes,
                                       &rel_saved_row_offset)) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_OVERFLOW;
            }
            if (inspect.image_data_offset > ((size_t)-1) - rel_saved_row_offset) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_OVERFLOW;
            }
            row_offset = inspect.image_data_offset + rel_saved_row_offset;

            if (fseek(fp, (long)row_offset, SEEK_SET) != 0) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_IO;
            }
            rc = tga__decode_saved_row_raw_rect_file(fp, &info, params, logical_row);
            if (rc != TGA_OK) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return rc;
            }
        } else {
            if (!inspect.has_scan_line_table) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_NO_SCAN_LINE_TABLE;
            }
            if (fseek(fp, (long)inspect.scan_line_table_offset + (long)((size_t)saved_row * 4u), SEEK_SET) != 0) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_IO;
            }
            {
                tga_u8 buf[4];
                if (fread(buf, 1u, 4u, fp) != 4u) {
                    (void)fseek(fp, saved_pos, SEEK_SET);
                    return TGA_ERR_IO;
                }
                row_offset = (size_t)tga__read_u32_le(buf);
                if (saved_row + 1u < info.height) {
                    if (fread(buf, 1u, 4u, fp) != 4u) {
                        (void)fseek(fp, saved_pos, SEEK_SET);
                        return TGA_ERR_IO;
                    }
                    next_row_offset = (size_t)tga__read_u32_le(buf);
                } else {
                    next_row_offset = image_end;
                }
            }

            if (row_offset < inspect.image_data_offset ||
                row_offset > image_end ||
                next_row_offset < row_offset ||
                next_row_offset > image_end) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_BAD_FORMAT;
            }

            if (fseek(fp, (long)row_offset, SEEK_SET) != 0) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return TGA_ERR_IO;
            }
            rc = tga__decode_saved_row_rle_rect_file(fp, &info, params, logical_row);
            if (rc != TGA_OK) {
                (void)fseek(fp, saved_pos, SEEK_SET);
                return rc;
            }
        }
    }

    (void)fseek(fp, saved_pos, SEEK_SET);
    if (out_info != 0) {
        *out_info = info;
    }
    return TGA_OK;
}
#endif


static int tga__swizzle_source_valid(unsigned source)
{
    return source <= (unsigned)TGA_SWIZZLE_LUMA;
}

static int tga__validate_swizzle(const tga_swizzle *swizzle)
{
    unsigned i;

    if (swizzle == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (swizzle->output_components == 0u || swizzle->output_components > 4u) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    for (i = 0u; i < swizzle->output_components; ++i) {
        if (!tga__swizzle_source_valid(swizzle->source[i])) {
            return TGA_ERR_BAD_ARGUMENT;
        }
    }
    return TGA_OK;
}

unsigned tga_channel_count_from_mask(unsigned mask)
{
    unsigned count;

    if ((mask & ~(TGA_CHANNEL_RED | TGA_CHANNEL_GREEN |
                  TGA_CHANNEL_BLUE | TGA_CHANNEL_ALPHA)) != 0u) {
        return 0u;
    }

    count = 0u;
    if ((mask & TGA_CHANNEL_RED) != 0u) {
        ++count;
    }
    if ((mask & TGA_CHANNEL_GREEN) != 0u) {
        ++count;
    }
    if ((mask & TGA_CHANNEL_BLUE) != 0u) {
        ++count;
    }
    if ((mask & TGA_CHANNEL_ALPHA) != 0u) {
        ++count;
    }
    return count;
}

int tga_swizzle_from_channel_mask(unsigned mask, tga_swizzle *out_swizzle)
{
    unsigned n;

    if (out_swizzle == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    n = tga_channel_count_from_mask(mask);
    if (n == 0u) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_swizzle, 0, sizeof(*out_swizzle));
    if ((mask & TGA_CHANNEL_RED) != 0u) {
        out_swizzle->source[out_swizzle->output_components++] = (unsigned)TGA_SWIZZLE_RED;
    }
    if ((mask & TGA_CHANNEL_GREEN) != 0u) {
        out_swizzle->source[out_swizzle->output_components++] = (unsigned)TGA_SWIZZLE_GREEN;
    }
    if ((mask & TGA_CHANNEL_BLUE) != 0u) {
        out_swizzle->source[out_swizzle->output_components++] = (unsigned)TGA_SWIZZLE_BLUE;
    }
    if ((mask & TGA_CHANNEL_ALPHA) != 0u) {
        out_swizzle->source[out_swizzle->output_components++] = (unsigned)TGA_SWIZZLE_ALPHA;
    }
    return TGA_OK;
}

static tga_u8 tga__swizzle_component(const tga__rgba *rgba, unsigned source)
{
    switch (source) {
        case TGA_SWIZZLE_ZERO:
            return 0u;
        case TGA_SWIZZLE_ONE:
            return 255u;
        case TGA_SWIZZLE_RED:
            return rgba->r;
        case TGA_SWIZZLE_GREEN:
            return rgba->g;
        case TGA_SWIZZLE_BLUE:
            return rgba->b;
        case TGA_SWIZZLE_ALPHA:
            return rgba->a;
        case TGA_SWIZZLE_LUMA:
            return tga__rgb_to_gray(rgba->r, rgba->g, rgba->b);
        default:
            return 0u;
    }
}

static void tga__store_swizzled_pixel(tga_u8 *dst_row, unsigned x,
                                      const tga__rgba *rgba,
                                      const tga_swizzle *swizzle)
{
    tga_u8 *dst;
    unsigned i;

    dst = dst_row + (size_t)x * (size_t)swizzle->output_components;
    for (i = 0u; i < swizzle->output_components; ++i) {
        dst[i] = tga__swizzle_component(rgba, swizzle->source[i]);
    }
}

int tga_swizzle_copy(const tga_swizzle_copy_params *params)
{
    unsigned src_bpp;
    size_t min_src_stride;
    size_t min_dst_stride;
    unsigned y;
    unsigned src_y;
    unsigned dst_y;
    const tga_u8 *src_row;
    tga_u8 *dst_row;
    unsigned x;
    tga__rgba rgba;
    int rc;

    if (params == 0 || params->src_pixels == 0 || params->dst_pixels == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (params->width == 0u || params->height == 0u) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__pixel_format_valid(params->src_pixel_format) ||
        params->src_pixel_format == TGA_PIXFMT_INDEX8) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga__validate_swizzle(&params->swizzle);
    if (rc != TGA_OK) {
        return rc;
    }

    src_bpp = tga_bytes_per_pixel(params->src_pixel_format);
    if (!tga__checked_mul_size((size_t)params->width,
                               (size_t)src_bpp,
                               &min_src_stride)) {
        return TGA_ERR_OVERFLOW;
    }
    if (!tga__checked_mul_size((size_t)params->width,
                               (size_t)params->swizzle.output_components,
                               &min_dst_stride)) {
        return TGA_ERR_OVERFLOW;
    }
    if ((size_t)params->src_stride_bytes < min_src_stride ||
        (size_t)params->dst_stride_bytes < min_dst_stride) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    for (y = 0u; y < params->height; ++y) {
        src_y = params->src_top_origin ? y : (params->height - 1u - y);
        dst_y = params->dst_top_origin ? y : (params->height - 1u - y);
        src_row = (const tga_u8 *)params->src_pixels +
                  (size_t)src_y * (size_t)params->src_stride_bytes;
        dst_row = (tga_u8 *)params->dst_pixels +
                  (size_t)dst_y * (size_t)params->dst_stride_bytes;

        for (x = 0u; x < params->width; ++x) {
            tga__load_memory_pixel_rgba(src_row, x, params->src_pixel_format, &rgba);
            tga__store_swizzled_pixel(dst_row, x, &rgba, &params->swizzle);
        }
    }

    return TGA_OK;
}

static int tga__decode_stream_swizzled(tga_reader *reader,
                                       const tga_info *info,
                                       void *pixels,
                                       unsigned stride_bytes,
                                       int output_top_origin,
                                       const tga_u8 *palette_rgba,
                                       const tga_swizzle *swizzle)
{
    unsigned sample_bytes;
    unsigned long total_pixels;
    unsigned long produced;
    unsigned long i;
    unsigned long file_row;
    unsigned long file_col;
    unsigned logical_y;
    unsigned logical_x;
    unsigned dst_y;
    tga_u8 *dst_row;
    tga_u8 sample[4];
    tga__rgba rgba;
    int rc;
    unsigned count;
    unsigned packet_header;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    total_pixels = (unsigned long)info->width * (unsigned long)info->height;
    produced = 0u;

    while (produced < total_pixels) {
        if (info->is_rle) {
            if (!tga__read_exact(reader, sample, 1u)) {
                return TGA_ERR_TRUNCATED;
            }
            packet_header = (unsigned)sample[0];
            count = (packet_header & 0x7Fu) + 1u;

            if (count > total_pixels - produced) {
                return TGA_ERR_BAD_FORMAT;
            }

            if ((packet_header & 0x80u) != 0u) {
                if (!tga__read_exact(reader, sample, sample_bytes)) {
                    return TGA_ERR_TRUNCATED;
                }
                for (i = 0u; i < (unsigned long)count; ++i) {
                    file_row = produced / (unsigned long)info->width;
                    file_col = produced % (unsigned long)info->width;
                    logical_y = info->file_top_origin
                        ? (unsigned)file_row
                        : (unsigned)(info->height - 1u - (unsigned)file_row);
                    logical_x = info->file_right_to_left
                        ? (unsigned)(info->width - 1u - (unsigned)file_col)
                        : (unsigned)file_col;

                    if (output_top_origin) {
                        dst_y = logical_y;
                    } else {
                        dst_y = info->height - 1u - logical_y;
                    }

                    dst_row = (tga_u8 *)pixels + (size_t)dst_y * (size_t)stride_bytes;

                    if (info->is_color_mapped) {
                        rc = tga__decode_index_sample(info, sample,
                                                      palette_rgba,
                                                      &rgba);
                        if (rc != TGA_OK) {
                            return rc;
                        }
                    } else {
                        tga__decode_color_sample(sample, info->pixel_depth,
                                                 info->descriptor & 0x0Fu,
                                                 info->is_grayscale, &rgba);
                    }

                    tga__store_swizzled_pixel(dst_row, logical_x, &rgba, swizzle);
                    ++produced;
                }
            } else {
                for (i = 0u; i < (unsigned long)count; ++i) {
                    if (!tga__read_exact(reader, sample, sample_bytes)) {
                        return TGA_ERR_TRUNCATED;
                    }

                    file_row = produced / (unsigned long)info->width;
                    file_col = produced % (unsigned long)info->width;
                    logical_y = info->file_top_origin
                        ? (unsigned)file_row
                        : (unsigned)(info->height - 1u - (unsigned)file_row);
                    logical_x = info->file_right_to_left
                        ? (unsigned)(info->width - 1u - (unsigned)file_col)
                        : (unsigned)file_col;

                    if (output_top_origin) {
                        dst_y = logical_y;
                    } else {
                        dst_y = info->height - 1u - logical_y;
                    }

                    dst_row = (tga_u8 *)pixels + (size_t)dst_y * (size_t)stride_bytes;

                    if (info->is_color_mapped) {
                        rc = tga__decode_index_sample(info, sample,
                                                      palette_rgba,
                                                      &rgba);
                        if (rc != TGA_OK) {
                            return rc;
                        }
                    } else {
                        tga__decode_color_sample(sample, info->pixel_depth,
                                                 info->descriptor & 0x0Fu,
                                                 info->is_grayscale, &rgba);
                    }

                    tga__store_swizzled_pixel(dst_row, logical_x, &rgba, swizzle);
                    ++produced;
                }
            }
        } else {
            if (!tga__read_exact(reader, sample, sample_bytes)) {
                return TGA_ERR_TRUNCATED;
            }

            file_row = produced / (unsigned long)info->width;
            file_col = produced % (unsigned long)info->width;
            logical_y = info->file_top_origin
                ? (unsigned)file_row
                : (unsigned)(info->height - 1u - (unsigned)file_row);
            logical_x = info->file_right_to_left
                ? (unsigned)(info->width - 1u - (unsigned)file_col)
                : (unsigned)file_col;

            if (output_top_origin) {
                dst_y = logical_y;
            } else {
                dst_y = info->height - 1u - logical_y;
            }

            dst_row = (tga_u8 *)pixels + (size_t)dst_y * (size_t)stride_bytes;

            if (info->is_color_mapped) {
                rc = tga__decode_index_sample(info, sample,
                                              palette_rgba,
                                              &rgba);
                if (rc != TGA_OK) {
                    return rc;
                }
            } else {
                tga__decode_color_sample(sample, info->pixel_depth,
                                         info->descriptor & 0x0Fu,
                                         info->is_grayscale, &rgba);
            }

            tga__store_swizzled_pixel(dst_row, logical_x, &rgba, swizzle);
            ++produced;
        }
    }

    return TGA_OK;
}

int tga_decode_swizzle(tga_reader *reader,
                       const tga_decode_swizzle_params *params,
                       tga_info *out_info)
{
    tga_u8 hdr[18];
    tga_info info;
    size_t min_stride;
    int rc;

    if (reader == 0 || params == 0 || params->pixels == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    rc = tga__validate_swizzle(&params->swizzle);
    if (rc != TGA_OK) {
        return rc;
    }

    if (!tga__read_exact(reader, hdr, sizeof(hdr))) {
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__parse_header(hdr, &info);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__validate_decode_info(&info);
    if (rc != TGA_OK) {
        return rc;
    }

    if (!tga__checked_mul_size((size_t)info.width,
                               (size_t)params->swizzle.output_components,
                               &min_stride)) {
        return TGA_ERR_OVERFLOW;
    }
    if ((size_t)params->stride_bytes < min_stride) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    if (!tga__skip(reader, (size_t)info.id_length)) {
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__load_palette(reader, &info,
                           params->palette_rgba,
                           params->palette_capacity);
    if (rc != TGA_OK) {
        return rc;
    }

    rc = tga__decode_stream_swizzled(reader, &info,
                                     params->pixels,
                                     params->stride_bytes,
                                     params->output_top_origin,
                                     params->palette_rgba,
                                     &params->swizzle);
    if (rc != TGA_OK) {
        return rc;
    }

    if (out_info != 0) {
        *out_info = info;
    }
    return TGA_OK;
}

int tga_decode_swizzle_memory(const void *src, size_t src_size,
                              const tga_decode_swizzle_params *params,
                              tga_info *out_info)
{
    tga_mem_reader mem;
    tga_reader reader;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    mem.data = (const tga_u8 *)src;
    mem.size = src_size;
    mem.pos = 0u;

    reader.read = tga_mem_read;
    reader.user = &mem;
    return tga_decode_swizzle(&reader, params, out_info);
}

int tga_decode_channels(tga_reader *reader,
                        const tga_decode_channels_params *params,
                        tga_info *out_info)
{
    tga_decode_swizzle_params swz;
    int rc;

    if (params == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(&swz, 0, sizeof(swz));
    swz.pixels = params->pixels;
    swz.stride_bytes = params->stride_bytes;
    swz.output_top_origin = params->output_top_origin;
    swz.palette_rgba = params->palette_rgba;
    swz.palette_capacity = params->palette_capacity;
    rc = tga_swizzle_from_channel_mask(params->channel_mask, &swz.swizzle);
    if (rc != TGA_OK) {
        return rc;
    }
    return tga_decode_swizzle(reader, &swz, out_info);
}

int tga_decode_channels_memory(const void *src, size_t src_size,
                               const tga_decode_channels_params *params,
                               tga_info *out_info)
{
    tga_mem_reader mem;
    tga_reader reader;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    mem.data = (const tga_u8 *)src;
    mem.size = src_size;
    mem.pos = 0u;

    reader.read = tga_mem_read;
    reader.user = &mem;
    return tga_decode_channels(&reader, params, out_info);
}

size_t tga_utility_text_size(const char *text)
{
    size_t n;

    if (text == 0) {
        return 0u;
    }

    n = 0u;
    while (text[n] != '\0') {
        ++n;
    }
    if (n == (size_t)-1) {
        return 0u;
    }
    return n + 1u;
}

static int tga__fill_developer_field(tga_developer_field *out_field,
                                     unsigned tag,
                                     void *payload,
                                     size_t payload_size)
{
    if (out_field == 0 || payload == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (tag > 65535u) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (payload_size > 0xFFFFFFFFul) {
        return TGA_ERR_OVERFLOW;
    }
    out_field->tag = tag;
    out_field->data = payload;
    out_field->data_size = (tga_u32)payload_size;
    return TGA_OK;
}

int tga_make_utility_text_field(tga_developer_field *out_field,
                                unsigned tag,
                                const char *text,
                                void *payload,
                                size_t payload_capacity,
                                size_t *out_payload_size)
{
    size_t need;
    int rc;

    if (text == 0 || payload == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    need = tga_utility_text_size(text);
    if (need == 0u) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_payload_size != 0) {
        *out_payload_size = need;
    }
    if (payload_capacity < need) {
        return TGA_ERR_CAPACITY;
    }
    memcpy(payload, text, need);
    rc = tga__fill_developer_field(out_field, tag, payload, need);
    if (rc != TGA_OK) {
        return rc;
    }
    return TGA_OK;
}

int tga_make_utility_u32_field(tga_developer_field *out_field,
                               unsigned tag,
                               tga_u32 value,
                               void *payload,
                               size_t payload_capacity)
{
    tga_u8 *dst;

    if (payload == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (payload_capacity < 4u) {
        return TGA_ERR_CAPACITY;
    }
    dst = (tga_u8 *)payload;
    tga__write_u32_le(dst, value);
    return tga__fill_developer_field(out_field, tag, payload, 4u);
}

int tga_make_utility_u32_pair_field(tga_developer_field *out_field,
                                    unsigned tag,
                                    const tga_utility_u32_pair *value,
                                    void *payload,
                                    size_t payload_capacity)
{
    tga_u8 *dst;

    if (value == 0 || payload == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (payload_capacity < 8u) {
        return TGA_ERR_CAPACITY;
    }
    dst = (tga_u8 *)payload;
    tga__write_u32_le(dst + 0u, value->first);
    tga__write_u32_le(dst + 4u, value->second);
    return tga__fill_developer_field(out_field, tag, payload, 8u);
}

int tga_make_utility_rect_field(tga_developer_field *out_field,
                                unsigned tag,
                                const tga_utility_rect *value,
                                void *payload,
                                size_t payload_capacity)
{
    tga_u8 *dst;

    if (value == 0 || payload == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (payload_capacity < 16u) {
        return TGA_ERR_CAPACITY;
    }
    dst = (tga_u8 *)payload;
    tga__write_u32_le(dst + 0u, value->x);
    tga__write_u32_le(dst + 4u, value->y);
    tga__write_u32_le(dst + 8u, value->width);
    tga__write_u32_le(dst + 12u, value->height);
    return tga__fill_developer_field(out_field, tag, payload, 16u);
}

int tga_make_utility_flags_field(tga_developer_field *out_field,
                                 unsigned tag,
                                 tga_u32 value,
                                 void *payload,
                                 size_t payload_capacity)
{
    return tga_make_utility_u32_field(out_field, tag, value,
                                      payload, payload_capacity);
}

int tga_make_utility_fourcc_field(tga_developer_field *out_field,
                                  unsigned tag,
                                  const char fourcc[4],
                                  void *payload,
                                  size_t payload_capacity)
{
    if (fourcc == 0 || payload == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (payload_capacity < 4u) {
        return TGA_ERR_CAPACITY;
    }
    memcpy(payload, fourcc, 4u);
    return tga__fill_developer_field(out_field, tag, payload, 4u);
}

int tga_parse_utility_text(const void *payload, size_t payload_size,
                           char *dst,
                           size_t dst_capacity,
                           size_t *out_text_size)
{
    if (payload == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_text_size != 0) {
        *out_text_size = payload_size;
    }
    if (dst == 0) {
        return TGA_OK;
    }
    if (dst_capacity < payload_size) {
        return TGA_ERR_CAPACITY;
    }
    if (payload_size != 0u) {
        memcpy(dst, payload, payload_size);
    }
    return TGA_OK;
}

int tga_parse_utility_u32(const void *payload, size_t payload_size,
                          tga_u32 *out_value)
{
    if (payload == 0 || out_value == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (payload_size != 4u) {
        return TGA_ERR_BAD_FORMAT;
    }
    *out_value = tga__read_u32_le((const tga_u8 *)payload);
    return TGA_OK;
}

int tga_parse_utility_u32_pair(const void *payload, size_t payload_size,
                               tga_utility_u32_pair *out_value)
{
    const tga_u8 *src;

    if (payload == 0 || out_value == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (payload_size != 8u) {
        return TGA_ERR_BAD_FORMAT;
    }
    src = (const tga_u8 *)payload;
    out_value->first = tga__read_u32_le(src + 0u);
    out_value->second = tga__read_u32_le(src + 4u);
    return TGA_OK;
}

int tga_parse_utility_rect(const void *payload, size_t payload_size,
                           tga_utility_rect *out_value)
{
    const tga_u8 *src;

    if (payload == 0 || out_value == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (payload_size != 16u) {
        return TGA_ERR_BAD_FORMAT;
    }
    src = (const tga_u8 *)payload;
    out_value->x = tga__read_u32_le(src + 0u);
    out_value->y = tga__read_u32_le(src + 4u);
    out_value->width = tga__read_u32_le(src + 8u);
    out_value->height = tga__read_u32_le(src + 12u);
    return TGA_OK;
}

int tga_parse_utility_fourcc(const void *payload, size_t payload_size,
                             char out_fourcc[4])
{
    if (payload == 0 || out_fourcc == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (payload_size != 4u) {
        return TGA_ERR_BAD_FORMAT;
    }
    memcpy(out_fourcc, payload, 4u);
    return TGA_OK;
}


static unsigned tga__decode_index_value(const tga_info *info, const tga_u8 *sample)
{
    unsigned idx;

    if (info->pixel_depth == 8u) {
        return (unsigned)sample[0];
    }

    idx = (unsigned)sample[0] | ((unsigned)sample[1] << 8);
    if (info->pixel_depth == 15u) {
        idx &= 0x7FFFu;
    }
    return idx;
}

static int tga__load_palette_optional(tga_reader *reader, const tga_info *info,
                                      tga_u8 *palette_rgba,
                                      unsigned palette_capacity)
{
    unsigned entry_bytes;

    if (info->color_map_length == 0u) {
        return TGA_OK;
    }

    entry_bytes = tga__file_sample_bytes(info->color_map_entry_bits);
    if (!info->is_color_mapped) {
        return tga__skip(reader, (size_t)entry_bytes * (size_t)info->color_map_length)
            ? TGA_OK : TGA_ERR_TRUNCATED;
    }

    if (palette_rgba == 0 || palette_capacity == 0u) {
        return tga__skip(reader, (size_t)entry_bytes * (size_t)info->color_map_length)
            ? TGA_OK : TGA_ERR_TRUNCATED;
    }

    return tga__load_palette(reader, info, palette_rgba, palette_capacity);
}

static unsigned tga__index_output_bytes(tga_index_format fmt)
{
    if (fmt == TGA_INDEX_U16) {
        return 2u;
    }
    return 1u;
}

static int tga__store_index_value(tga_u8 *dst_row, unsigned x,
                                  tga_index_format fmt,
                                  unsigned value)
{
    tga_u8 *dst;

    if (fmt == TGA_INDEX_U16) {
        dst = dst_row + (size_t)x * 2u;
        dst[0] = (tga_u8)(value & 255u);
        dst[1] = (tga_u8)((value >> 8) & 255u);
        return TGA_OK;
    }

    if (value > 255u) {
        return TGA_ERR_INDEX_RANGE;
    }
    dst_row[x] = (tga_u8)value;
    return TGA_OK;
}

static void tga__fill_palette_info(const tga_info *info,
                                   tga_palette_info *out_palette_info)
{
    if (out_palette_info == 0) {
        return;
    }
    out_palette_info->first_index = info->color_map_origin;
    out_palette_info->entry_count = info->color_map_length;
    out_palette_info->entry_bits = info->color_map_entry_bits;
}

int tga_decode_color_map_rgba(tga_reader *reader,
                              tga_u8 *palette_rgba,
                              unsigned palette_capacity,
                              tga_palette_info *out_palette_info,
                              tga_info *out_info)
{
    tga_u8 hdr[18];
    tga_info info;
    int rc;

    if (reader == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__read_exact(reader, hdr, sizeof(hdr))) {
        return TGA_ERR_TRUNCATED;
    }
    rc = tga__parse_header(hdr, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__validate_decode_info(&info);
    if (rc != TGA_OK) {
        return rc;
    }
    if (!info.is_color_mapped) {
        return TGA_ERR_UNSUPPORTED;
    }
    if (!tga__skip(reader, (size_t)info.id_length)) {
        return TGA_ERR_TRUNCATED;
    }
    rc = tga__load_palette_optional(reader, &info, palette_rgba, palette_capacity);
    if (rc != TGA_OK) {
        return rc;
    }
    tga__fill_palette_info(&info, out_palette_info);
    if (out_info != 0) {
        *out_info = info;
    }
    return TGA_OK;
}

int tga_decode_color_map_rgba_memory(const void *src, size_t src_size,
                                     tga_u8 *palette_rgba,
                                     unsigned palette_capacity,
                                     tga_palette_info *out_palette_info,
                                     tga_info *out_info)
{
    tga_mem_reader mem;
    tga_reader reader;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    mem.data = (const tga_u8 *)src;
    mem.size = src_size;
    mem.pos = 0u;
    reader.read = tga_mem_read;
    reader.user = &mem;
    return tga_decode_color_map_rgba(&reader, palette_rgba, palette_capacity,
                                     out_palette_info, out_info);
}

static int tga__decode_stream_indices(tga_reader *reader,
                                      const tga_info *info,
                                      void *indices,
                                      unsigned stride_bytes,
                                      int output_top_origin,
                                      int normalize_indices,
                                      tga_index_format index_format)
{
    unsigned sample_bytes;
    unsigned long total_pixels;
    unsigned long produced;
    unsigned long i;
    unsigned long file_row;
    unsigned long file_col;
    unsigned logical_y;
    unsigned logical_x;
    unsigned dst_y;
    unsigned value;
    tga_u8 *dst_row;
    tga_u8 sample[4];
    int rc;
    unsigned count;
    unsigned packet_header;

    sample_bytes = tga__file_sample_bytes(info->pixel_depth);
    total_pixels = (unsigned long)info->width * (unsigned long)info->height;
    produced = 0u;

    while (produced < total_pixels) {
        if (info->is_rle) {
            if (!tga__read_exact(reader, sample, 1u)) {
                return TGA_ERR_TRUNCATED;
            }
            packet_header = (unsigned)sample[0];
            count = (packet_header & 0x7Fu) + 1u;
            if (count > total_pixels - produced) {
                return TGA_ERR_BAD_FORMAT;
            }
            if ((packet_header & 0x80u) != 0u) {
                if (!tga__read_exact(reader, sample, sample_bytes)) {
                    return TGA_ERR_TRUNCATED;
                }
                value = tga__decode_index_value(info, sample);
                if (normalize_indices) {
                    if (value < info->color_map_origin) {
                        return TGA_ERR_PALETTE_RANGE;
                    }
                    value -= info->color_map_origin;
                    if (value >= info->color_map_length) {
                        return TGA_ERR_PALETTE_RANGE;
                    }
                }
                for (i = 0u; i < (unsigned long)count; ++i) {
                    file_row = produced / (unsigned long)info->width;
                    file_col = produced % (unsigned long)info->width;
                    logical_y = info->file_top_origin
                        ? (unsigned)file_row
                        : (unsigned)(info->height - 1u - (unsigned)file_row);
                    logical_x = info->file_right_to_left
                        ? (unsigned)(info->width - 1u - (unsigned)file_col)
                        : (unsigned)file_col;
                    if (output_top_origin) {
                        dst_y = logical_y;
                    } else {
                        dst_y = info->height - 1u - logical_y;
                    }
                    dst_row = (tga_u8 *)indices + (size_t)dst_y * (size_t)stride_bytes;
                    rc = tga__store_index_value(dst_row, logical_x, index_format, value);
                    if (rc != TGA_OK) {
                        return rc;
                    }
                    ++produced;
                }
            } else {
                for (i = 0u; i < (unsigned long)count; ++i) {
                    if (!tga__read_exact(reader, sample, sample_bytes)) {
                        return TGA_ERR_TRUNCATED;
                    }
                    value = tga__decode_index_value(info, sample);
                    if (normalize_indices) {
                        if (value < info->color_map_origin) {
                            return TGA_ERR_PALETTE_RANGE;
                        }
                        value -= info->color_map_origin;
                        if (value >= info->color_map_length) {
                            return TGA_ERR_PALETTE_RANGE;
                        }
                    }
                    file_row = produced / (unsigned long)info->width;
                    file_col = produced % (unsigned long)info->width;
                    logical_y = info->file_top_origin
                        ? (unsigned)file_row
                        : (unsigned)(info->height - 1u - (unsigned)file_row);
                    logical_x = info->file_right_to_left
                        ? (unsigned)(info->width - 1u - (unsigned)file_col)
                        : (unsigned)file_col;
                    if (output_top_origin) {
                        dst_y = logical_y;
                    } else {
                        dst_y = info->height - 1u - logical_y;
                    }
                    dst_row = (tga_u8 *)indices + (size_t)dst_y * (size_t)stride_bytes;
                    rc = tga__store_index_value(dst_row, logical_x, index_format, value);
                    if (rc != TGA_OK) {
                        return rc;
                    }
                    ++produced;
                }
            }
        } else {
            if (!tga__read_exact(reader, sample, sample_bytes)) {
                return TGA_ERR_TRUNCATED;
            }
            value = tga__decode_index_value(info, sample);
            if (normalize_indices) {
                if (value < info->color_map_origin) {
                    return TGA_ERR_PALETTE_RANGE;
                }
                value -= info->color_map_origin;
                if (value >= info->color_map_length) {
                    return TGA_ERR_PALETTE_RANGE;
                }
            }
            file_row = produced / (unsigned long)info->width;
            file_col = produced % (unsigned long)info->width;
            logical_y = info->file_top_origin
                ? (unsigned)file_row
                : (unsigned)(info->height - 1u - (unsigned)file_row);
            logical_x = info->file_right_to_left
                ? (unsigned)(info->width - 1u - (unsigned)file_col)
                : (unsigned)file_col;
            if (output_top_origin) {
                dst_y = logical_y;
            } else {
                dst_y = info->height - 1u - logical_y;
            }
            dst_row = (tga_u8 *)indices + (size_t)dst_y * (size_t)stride_bytes;
            rc = tga__store_index_value(dst_row, logical_x, index_format, value);
            if (rc != TGA_OK) {
                return rc;
            }
            ++produced;
        }
    }

    return TGA_OK;
}

int tga_decode_indexed(tga_reader *reader,
                       const tga_decode_indexed_params *params,
                       tga_palette_info *out_palette_info,
                       tga_info *out_info)
{
    tga_u8 hdr[18];
    tga_info info;
    size_t min_stride;
    int rc;

    if (reader == 0 || params == 0 || params->indices == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (params->index_format != TGA_INDEX_U8 &&
        params->index_format != TGA_INDEX_U16) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__read_exact(reader, hdr, sizeof(hdr))) {
        return TGA_ERR_TRUNCATED;
    }
    rc = tga__parse_header(hdr, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__validate_decode_info(&info);
    if (rc != TGA_OK) {
        return rc;
    }
    if (!info.is_color_mapped) {
        return TGA_ERR_UNSUPPORTED;
    }
    if (!tga__checked_mul_size((size_t)info.width,
                               (size_t)tga__index_output_bytes(params->index_format),
                               &min_stride)) {
        return TGA_ERR_OVERFLOW;
    }
    if ((size_t)params->stride_bytes < min_stride) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (!tga__skip(reader, (size_t)info.id_length)) {
        return TGA_ERR_TRUNCATED;
    }
    rc = tga__load_palette_optional(reader, &info,
                                    params->palette_rgba,
                                    params->palette_capacity);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__decode_stream_indices(reader, &info,
                                    params->indices,
                                    params->stride_bytes,
                                    params->output_top_origin,
                                    params->normalize_indices,
                                    params->index_format);
    if (rc != TGA_OK) {
        return rc;
    }
    tga__fill_palette_info(&info, out_palette_info);
    if (out_info != 0) {
        *out_info = info;
    }
    return TGA_OK;
}

int tga_decode_indexed_memory(const void *src, size_t src_size,
                              const tga_decode_indexed_params *params,
                              tga_palette_info *out_palette_info,
                              tga_info *out_info)
{
    tga_mem_reader mem;
    tga_reader reader;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    mem.data = (const tga_u8 *)src;
    mem.size = src_size;
    mem.pos = 0u;
    reader.read = tga_mem_read;
    reader.user = &mem;
    return tga_decode_indexed(&reader, params, out_palette_info, out_info);
}

static int tga__append_section(tga_section *sections,
                               unsigned section_capacity,
                               unsigned *count,
                               unsigned kind,
                               unsigned tag,
                               size_t offset,
                               size_t size)
{
    if (sections != 0 && *count < section_capacity) {
        sections[*count].kind = kind;
        sections[*count].tag = tag;
        sections[*count].offset = offset;
        sections[*count].size = size;
    }
    ++(*count);
    return TGA_OK;
}

static void tga__sort_sections(tga_section *sections, unsigned count)
{
    unsigned i;
    unsigned j;
    tga_section key;

    for (i = 1u; i < count; ++i) {
        key = sections[i];
        j = i;
        while (j > 0u) {
            if (sections[j - 1u].offset < key.offset) {
                break;
            }
            if (sections[j - 1u].offset == key.offset &&
                sections[j - 1u].kind <= key.kind) {
                break;
            }
            sections[j] = sections[j - 1u];
            --j;
        }
        sections[j] = key;
    }
}

int tga_list_sections_memory(const void *src, size_t src_size,
                             tga_section *sections,
                             unsigned section_capacity,
                             unsigned *out_section_count)
{
    tga_inspect inspect;
    const tga_u8 *bytes;
    unsigned need;
    unsigned count;
    unsigned i;
    int rc;
    size_t dir_off;
    size_t field_off;
    size_t field_size;

    if (src == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_section_count != 0) {
        *out_section_count = 0u;
    }
    rc = tga_inspect_memory(src, src_size, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }

    need = 2u;
    if (inspect.image_id_size != 0u) ++need;
    if (inspect.color_map_size != 0u) ++need;
    if (inspect.image_data_size != 0u) ++need;
    if (inspect.has_developer_directory) need += 1u + inspect.developer_tag_count;
    if (inspect.has_extension) ++need;
    if (inspect.has_color_correction_table) ++need;
    if (inspect.has_postage_stamp) ++need;
    if (inspect.has_scan_line_table) ++need;
    if (inspect.footer_size != 0u) ++need;

    if (out_section_count != 0) {
        *out_section_count = need;
    }
    if (sections == 0 || section_capacity == 0u) {
        return TGA_OK;
    }
    if (section_capacity < need) {
        return TGA_ERR_CAPACITY;
    }

    count = 0u;
    tga__append_section(sections, section_capacity, &count,
                        TGA_SECTION_HEADER, 0u, 0u, 18u);
    if (inspect.image_id_size != 0u) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_IMAGE_ID, 0u,
                            inspect.image_id_offset,
                            inspect.image_id_size);
    }
    if (inspect.color_map_size != 0u) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_COLOR_MAP, 0u,
                            inspect.color_map_offset,
                            inspect.color_map_size);
    }
    if (inspect.image_data_size != 0u) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_IMAGE_DATA, 0u,
                            inspect.image_data_offset,
                            inspect.image_data_size);
    }
    if (inspect.has_developer_directory) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_DEVELOPER_DIRECTORY, 0u,
                            inspect.developer_directory_offset,
                            inspect.developer_directory_size);
        bytes = (const tga_u8 *)src;
        dir_off = inspect.developer_directory_offset + 2u;
        for (i = 0u; i < inspect.developer_tag_count; ++i) {
            field_off = (size_t)tga__read_u32_le(bytes + dir_off + (size_t)i * 10u + 2u);
            field_size = (size_t)tga__read_u32_le(bytes + dir_off + (size_t)i * 10u + 6u);
            tga__append_section(sections, section_capacity, &count,
                                TGA_SECTION_DEVELOPER_FIELD,
                                (unsigned)tga__read_u16_le(bytes + dir_off + (size_t)i * 10u),
                                field_off,
                                field_size);
        }
    }
    if (inspect.has_extension) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_EXTENSION_AREA, 0u,
                            inspect.extension_area_offset,
                            inspect.extension_area_size);
    }
    if (inspect.has_color_correction_table) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_COLOR_CORRECTION_TABLE, 0u,
                            inspect.color_correction_offset,
                            inspect.color_correction_size);
    }
    if (inspect.has_postage_stamp) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_POSTAGE_STAMP, 0u,
                            inspect.postage_stamp_offset,
                            inspect.postage_stamp_size);
    }
    if (inspect.has_scan_line_table) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_SCAN_LINE_TABLE, 0u,
                            inspect.scan_line_table_offset,
                            inspect.scan_line_table_size);
    }
    if (inspect.footer_size != 0u) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_FOOTER, 0u,
                            inspect.footer_offset,
                            inspect.footer_size);
    }

    tga__sort_sections(sections, count);
    if (out_section_count != 0) {
        *out_section_count = count;
    }
    return TGA_OK;
}

static int tga__collect_high_level_sections(const tga_inspect *inspect,
                                            tga_section *sections,
                                            unsigned *out_count)
{
    unsigned count;

    count = 0u;
    tga__append_section(sections, 16u, &count,
                        TGA_SECTION_HEADER, 0u, 0u, 18u);
    if (inspect->image_id_size != 0u) {
        tga__append_section(sections, 16u, &count,
                            TGA_SECTION_IMAGE_ID, 0u,
                            inspect->image_id_offset,
                            inspect->image_id_size);
    }
    if (inspect->color_map_size != 0u) {
        tga__append_section(sections, 16u, &count,
                            TGA_SECTION_COLOR_MAP, 0u,
                            inspect->color_map_offset,
                            inspect->color_map_size);
    }
    if (inspect->image_data_size != 0u) {
        tga__append_section(sections, 16u, &count,
                            TGA_SECTION_IMAGE_DATA, 0u,
                            inspect->image_data_offset,
                            inspect->image_data_size);
    }
    if (inspect->has_developer_directory) {
        tga__append_section(sections, 16u, &count,
                            TGA_SECTION_DEVELOPER_DIRECTORY, 0u,
                            inspect->developer_directory_offset,
                            inspect->developer_directory_size);
    }
    if (inspect->has_extension) {
        tga__append_section(sections, 16u, &count,
                            TGA_SECTION_EXTENSION_AREA, 0u,
                            inspect->extension_area_offset,
                            inspect->extension_area_size);
    }
    if (inspect->has_color_correction_table) {
        tga__append_section(sections, 16u, &count,
                            TGA_SECTION_COLOR_CORRECTION_TABLE, 0u,
                            inspect->color_correction_offset,
                            inspect->color_correction_size);
    }
    if (inspect->has_postage_stamp) {
        tga__append_section(sections, 16u, &count,
                            TGA_SECTION_POSTAGE_STAMP, 0u,
                            inspect->postage_stamp_offset,
                            inspect->postage_stamp_size);
    }
    if (inspect->has_scan_line_table) {
        tga__append_section(sections, 16u, &count,
                            TGA_SECTION_SCAN_LINE_TABLE, 0u,
                            inspect->scan_line_table_offset,
                            inspect->scan_line_table_size);
    }
    if (inspect->footer_size != 0u) {
        tga__append_section(sections, 16u, &count,
                            TGA_SECTION_FOOTER, 0u,
                            inspect->footer_offset,
                            inspect->footer_size);
    }
    tga__sort_sections(sections, count);
    *out_count = count;
    return TGA_OK;
}

static int tga__inspect_forensics_memory(const void *src,
                                        size_t src_size,
                                        tga_inspect *out_inspect)
{
    const tga_u8 *bytes;
    tga_info info;
    tga_footer footer;
    size_t color_map_size;
    size_t image_data_offset;
    size_t image_data_size;
    size_t ext_size;
    size_t dir_size;
    size_t tag_bytes;
    size_t min_payload;
    size_t max_payload;
    size_t field_offset;
    size_t field_size;
    unsigned count;
    unsigned i;
    int rc;
    const tga_u8 *dir;

    if (src == 0 || out_inspect == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_inspect, 0, sizeof(*out_inspect));
    bytes = (const tga_u8 *)src;
    out_inspect->file_size = src_size;

    rc = tga_probe_memory(src, src_size, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    out_inspect->info = info;
    out_inspect->image_id_offset = 18u;
    out_inspect->image_id_size = (size_t)info.id_length;
    if (!tga__region_fits(out_inspect->image_id_offset,
                          out_inspect->image_id_size,
                          src_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__color_map_size_from_info(&info, &color_map_size)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->color_map_offset = out_inspect->image_id_offset + out_inspect->image_id_size;
    out_inspect->color_map_size = color_map_size;
    if (!tga__region_fits(out_inspect->color_map_offset,
                          out_inspect->color_map_size,
                          src_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__image_data_offset_from_info(&info, &image_data_offset)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->image_data_offset = image_data_offset;
    if (!tga__region_fits(image_data_offset, 0u, src_size)) {
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__validate_decode_info(&info);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__measure_image_data_size_memory(bytes, src_size, &info,
                                             image_data_offset,
                                             &image_data_size);
    if (rc != TGA_OK) {
        return rc;
    }
    out_inspect->image_data_size = image_data_size;

    rc = tga_read_footer_memory(src, src_size, &footer);
    if (rc == TGA_OK) {
        out_inspect->footer = footer;
        out_inspect->is_tga2 = 1;
        out_inspect->footer_offset = src_size - 26u;
        out_inspect->footer_size = 26u;
    } else if (rc != TGA_ERR_NO_FOOTER) {
        return rc;
    }

    if (out_inspect->is_tga2 && footer.extension_offset != 0ul &&
        tga__region_fits((size_t)footer.extension_offset, 2u, src_size)) {
        ext_size = (size_t)tga__read_u16_le(bytes + (size_t)footer.extension_offset);
        if (ext_size >= 2u &&
            tga__region_fits((size_t)footer.extension_offset, ext_size, src_size)) {
            out_inspect->has_extension = 1;
            out_inspect->extension_area_offset = (size_t)footer.extension_offset;
            out_inspect->extension_area_size = ext_size;
            if (ext_size >= 495u) {
                tga__parse_extension_bytes(bytes + (size_t)footer.extension_offset,
                                           &out_inspect->extension);
                if (out_inspect->extension.scan_line_offset != 0ul &&
                    tga__checked_mul_size((size_t)info.height, 4u,
                                          &out_inspect->scan_line_table_size) &&
                    tga__region_fits((size_t)out_inspect->extension.scan_line_offset,
                                     out_inspect->scan_line_table_size,
                                     src_size)) {
                    out_inspect->scan_line_table_offset =
                        (size_t)out_inspect->extension.scan_line_offset;
                    out_inspect->has_scan_line_table = 1;
                }
                if (out_inspect->extension.postage_stamp_offset != 0ul &&
                    tga_read_postage_stamp_info_memory(src, src_size,
                                                      &out_inspect->postage_stamp_info) == TGA_OK) {
                    out_inspect->postage_stamp_offset =
                        (size_t)out_inspect->extension.postage_stamp_offset;
                    out_inspect->postage_stamp_size =
                        out_inspect->postage_stamp_info.block_size;
                    out_inspect->has_postage_stamp = 1;
                }
                if (out_inspect->extension.color_correction_offset != 0ul) {
                    out_inspect->color_correction_offset =
                        (size_t)out_inspect->extension.color_correction_offset;
                    out_inspect->color_correction_size =
                        (size_t)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8u;
                    if (tga__region_fits(out_inspect->color_correction_offset,
                                         out_inspect->color_correction_size,
                                         src_size)) {
                        out_inspect->has_color_correction_table = 1;
                    }
                }
            }
        }
    }

    if (out_inspect->is_tga2 && footer.developer_offset != 0ul &&
        tga__region_fits((size_t)footer.developer_offset, 2u, src_size)) {
        dir = bytes + (size_t)footer.developer_offset;
        count = (unsigned)tga__read_u16_le(dir + 0u);
        if (!tga__checked_mul_size((size_t)count, 10u, &tag_bytes)) {
            return TGA_ERR_OVERFLOW;
        }
        dir_size = 2u + tag_bytes;
        if (tga__region_fits((size_t)footer.developer_offset, dir_size, src_size)) {
            out_inspect->has_developer_directory = 1;
            out_inspect->developer_directory_offset = (size_t)footer.developer_offset;
            out_inspect->developer_directory_size = dir_size;
            out_inspect->developer_tag_count = count;
            min_payload = (size_t)-1;
            max_payload = 0u;
            for (i = 0u; i < count; ++i) {
                field_offset = (size_t)tga__read_u32_le(dir + 2u + (size_t)i * 10u + 2u);
                field_size = (size_t)tga__read_u32_le(dir + 2u + (size_t)i * 10u + 6u);
                if (tga__region_fits(field_offset, field_size, src_size)) {
                    if (field_offset < min_payload) {
                        min_payload = field_offset;
                    }
                    if (max_payload < field_offset + field_size) {
                        max_payload = field_offset + field_size;
                    }
                }
            }
            if (min_payload != (size_t)-1 && max_payload > min_payload) {
                out_inspect->has_developer_payload = 1;
                out_inspect->developer_payload_offset = min_payload;
                out_inspect->developer_payload_size = max_payload - min_payload;
            }
        }
    }

    return TGA_OK;
}

#ifndef GAFNYF_TGA_NO_STDIO
static int tga__inspect_forensics_file(FILE *fp, tga_inspect *out_inspect)
{
    tga_info info;
    tga_footer footer;
    tga_u8 buf[10];
    size_t color_map_size;
    size_t image_data_offset;
    size_t image_data_size;
    size_t ext_size;
    size_t dir_size;
    size_t tag_bytes;
    size_t min_payload;
    size_t max_payload;
    size_t field_offset;
    size_t field_size;
    long saved_pos;
    long end_pos;
    unsigned count;
    unsigned i;
    int rc;

    if (fp == 0 || out_inspect == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    memset(out_inspect, 0, sizeof(*out_inspect));

    saved_pos = ftell(fp);
    if (saved_pos < 0L) {
        return TGA_ERR_IO;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        (void)fseek(fp, saved_pos, SEEK_SET);
        return TGA_ERR_IO;
    }
    end_pos = ftell(fp);
    (void)fseek(fp, saved_pos, SEEK_SET);
    if (end_pos < 0L) {
        return TGA_ERR_IO;
    }
    out_inspect->file_size = (size_t)end_pos;

    rc = tga_probe_file(fp, &info);
    if (rc != TGA_OK) {
        return rc;
    }
    out_inspect->info = info;
    out_inspect->image_id_offset = 18u;
    out_inspect->image_id_size = (size_t)info.id_length;
    if (!tga__region_fits(out_inspect->image_id_offset,
                          out_inspect->image_id_size,
                          out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__color_map_size_from_info(&info, &color_map_size)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->color_map_offset = out_inspect->image_id_offset + out_inspect->image_id_size;
    out_inspect->color_map_size = color_map_size;
    if (!tga__region_fits(out_inspect->color_map_offset,
                          out_inspect->color_map_size,
                          out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    if (!tga__image_data_offset_from_info(&info, &image_data_offset)) {
        return TGA_ERR_OVERFLOW;
    }
    out_inspect->image_data_offset = image_data_offset;
    if (!tga__region_fits(image_data_offset, 0u, out_inspect->file_size)) {
        return TGA_ERR_TRUNCATED;
    }

    rc = tga__validate_decode_info(&info);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__measure_image_data_size_file(fp, &info, image_data_offset,
                                           &image_data_size);
    if (rc != TGA_OK) {
        return rc;
    }
    out_inspect->image_data_size = image_data_size;

    rc = tga_read_footer_file(fp, &footer);
    if (rc == TGA_OK) {
        out_inspect->footer = footer;
        out_inspect->is_tga2 = 1;
        out_inspect->footer_offset = out_inspect->file_size - 26u;
        out_inspect->footer_size = 26u;
    } else if (rc != TGA_ERR_NO_FOOTER) {
        return rc;
    }

    if (out_inspect->is_tga2 && footer.extension_offset != 0ul) {
        rc = tga__read_section_file_impl(fp,
                                         (size_t)footer.extension_offset,
                                         2u,
                                         buf,
                                         2u,
                                         0);
        if (rc == TGA_OK) {
            ext_size = (size_t)tga__read_u16_le(buf);
            if (ext_size >= 2u &&
                tga__region_fits((size_t)footer.extension_offset,
                                 ext_size,
                                 out_inspect->file_size)) {
                out_inspect->has_extension = 1;
                out_inspect->extension_area_offset = (size_t)footer.extension_offset;
                out_inspect->extension_area_size = ext_size;
                if (ext_size >= 495u) {
                    tga_u8 ext_buf[495];
                    rc = tga__read_section_file_impl(fp,
                                                     (size_t)footer.extension_offset,
                                                     495u,
                                                     ext_buf,
                                                     sizeof(ext_buf),
                                                     0);
                    if (rc == TGA_OK) {
                        tga__parse_extension_bytes(ext_buf, &out_inspect->extension);
                        if (out_inspect->extension.scan_line_offset != 0ul &&
                            tga__checked_mul_size((size_t)info.height, 4u,
                                                  &out_inspect->scan_line_table_size) &&
                            tga__region_fits((size_t)out_inspect->extension.scan_line_offset,
                                             out_inspect->scan_line_table_size,
                                             out_inspect->file_size)) {
                            out_inspect->scan_line_table_offset =
                                (size_t)out_inspect->extension.scan_line_offset;
                            out_inspect->has_scan_line_table = 1;
                        }
                        if (out_inspect->extension.postage_stamp_offset != 0ul &&
                            tga_read_postage_stamp_info_file(fp,
                                                             &out_inspect->postage_stamp_info) == TGA_OK) {
                            out_inspect->postage_stamp_offset =
                                (size_t)out_inspect->extension.postage_stamp_offset;
                            out_inspect->postage_stamp_size =
                                out_inspect->postage_stamp_info.block_size;
                            out_inspect->has_postage_stamp = 1;
                        }
                        if (out_inspect->extension.color_correction_offset != 0ul) {
                            out_inspect->color_correction_offset =
                                (size_t)out_inspect->extension.color_correction_offset;
                            out_inspect->color_correction_size =
                                (size_t)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8u;
                            if (tga__region_fits(out_inspect->color_correction_offset,
                                                 out_inspect->color_correction_size,
                                                 out_inspect->file_size)) {
                                out_inspect->has_color_correction_table = 1;
                            }
                        }
                    }
                }
            }
        }
    }

    if (out_inspect->is_tga2 && footer.developer_offset != 0ul) {
        rc = tga__read_section_file_impl(fp,
                                         (size_t)footer.developer_offset,
                                         2u,
                                         buf,
                                         2u,
                                         0);
        if (rc == TGA_OK) {
            count = (unsigned)tga__read_u16_le(buf + 0u);
            if (!tga__checked_mul_size((size_t)count, 10u, &tag_bytes)) {
                return TGA_ERR_OVERFLOW;
            }
            dir_size = 2u + tag_bytes;
            if (tga__region_fits((size_t)footer.developer_offset,
                                 dir_size,
                                 out_inspect->file_size)) {
                out_inspect->has_developer_directory = 1;
                out_inspect->developer_directory_offset = (size_t)footer.developer_offset;
                out_inspect->developer_directory_size = dir_size;
                out_inspect->developer_tag_count = count;
                min_payload = (size_t)-1;
                max_payload = 0u;
                for (i = 0u; i < count; ++i) {
                    rc = tga__read_section_file_impl(fp,
                                                     (size_t)footer.developer_offset + 2u + (size_t)i * 10u,
                                                     10u,
                                                     buf,
                                                     sizeof(buf),
                                                     0);
                    if (rc != TGA_OK) {
                        return rc;
                    }
                    field_offset = (size_t)tga__read_u32_le(buf + 2u);
                    field_size = (size_t)tga__read_u32_le(buf + 6u);
                    if (tga__region_fits(field_offset, field_size, out_inspect->file_size)) {
                        if (field_offset < min_payload) {
                            min_payload = field_offset;
                        }
                        if (max_payload < field_offset + field_size) {
                            max_payload = field_offset + field_size;
                        }
                    }
                }
                if (min_payload != (size_t)-1 && max_payload > min_payload) {
                    out_inspect->has_developer_payload = 1;
                    out_inspect->developer_payload_offset = min_payload;
                    out_inspect->developer_payload_size = max_payload - min_payload;
                }
            }
        }
    }

    return TGA_OK;
}
#endif

static void tga__analyze_section_layout(const tga_inspect *inspect,
                                        tga_section *sections,
                                        unsigned section_count,
                                        tga_forensic_report *out_report)
{
    unsigned i;
    size_t cur_end;
    size_t next_off;
    size_t gap;

    out_report->section_count = section_count;
    for (i = 0u; i + 1u < section_count; ++i) {
        cur_end = sections[i].offset + sections[i].size;
        next_off = sections[i + 1u].offset;
        if (next_off < cur_end) {
            ++out_report->overlap_count;
            out_report->flags |= TGA_FORENSIC_BLOCK_OVERLAP;
        } else if (next_off > cur_end) {
            ++out_report->gap_count;
            gap = next_off - cur_end;
            out_report->gap_bytes += gap;
            out_report->flags |= TGA_FORENSIC_BLOCK_GAP;
        }
    }
    if (section_count != 0u) {
        cur_end = sections[section_count - 1u].offset + sections[section_count - 1u].size;
        if (inspect->file_size > cur_end) {
            out_report->trailing_bytes = inspect->file_size - cur_end;
            out_report->flags |= TGA_FORENSIC_TRAILING_BYTES;
        }
    }
}

static int tga__ranges_overlap(size_t off_a, size_t size_a,
                                 size_t off_b, size_t size_b)
{
    size_t end_a;
    size_t end_b;

    if (size_a == 0u || size_b == 0u) {
        return 0;
    }
    end_a = off_a + size_a;
    end_b = off_b + size_b;
    if (end_a < off_a) {
        end_a = (size_t)-1;
    }
    if (end_b < off_b) {
        end_b = (size_t)-1;
    }
    return off_a < end_b && off_b < end_a;
}

static int tga__known_utility_payload(unsigned tag,
                                      size_t *out_size,
                                      int *out_needs_nul)
{
    if (out_size != 0) {
        *out_size = 0u;
    }
    if (out_needs_nul != 0) {
        *out_needs_nul = 0;
    }

    switch (tag) {
        case TGA_UTIL_TAG_TEXT_UTF8:
            if (out_needs_nul != 0) {
                *out_needs_nul = 1;
            }
            return 1;

        case TGA_UTIL_TAG_U32:
        case TGA_UTIL_TAG_FLAGS_U32:
        case TGA_UTIL_TAG_FOURCC:
            if (out_size != 0) {
                *out_size = 4u;
            }
            return 1;

        case TGA_UTIL_TAG_U32_PAIR:
            if (out_size != 0) {
                *out_size = 8u;
            }
            return 1;

        case TGA_UTIL_TAG_RECT_U32:
            if (out_size != 0) {
                *out_size = 16u;
            }
            return 1;

        default:
            return 0;
    }
}

static void tga__flag_extension_size_mismatch(tga_forensic_report *out_report)
{
    ++out_report->extension_size_mismatch_count;
    out_report->flags |= TGA_FORENSIC_EXTENSION_SIZE_MISMATCH;
}

static int tga__field_overlaps_high_level(const tga_inspect *inspect,
                                          size_t offset,
                                          size_t size)
{
    if (tga__ranges_overlap(offset, size, 0u, 18u)) {
        return 1;
    }
    if (inspect->image_id_size != 0u &&
        tga__ranges_overlap(offset, size,
                            inspect->image_id_offset,
                            inspect->image_id_size)) {
        return 1;
    }
    if (inspect->color_map_size != 0u &&
        tga__ranges_overlap(offset, size,
                            inspect->color_map_offset,
                            inspect->color_map_size)) {
        return 1;
    }
    if (inspect->image_data_size != 0u &&
        tga__ranges_overlap(offset, size,
                            inspect->image_data_offset,
                            inspect->image_data_size)) {
        return 1;
    }
    if (inspect->has_extension &&
        tga__ranges_overlap(offset, size,
                            inspect->extension_area_offset,
                            inspect->extension_area_size)) {
        return 1;
    }
    if (inspect->has_color_correction_table &&
        tga__ranges_overlap(offset, size,
                            inspect->color_correction_offset,
                            inspect->color_correction_size)) {
        return 1;
    }
    if (inspect->has_postage_stamp &&
        tga__ranges_overlap(offset, size,
                            inspect->postage_stamp_offset,
                            inspect->postage_stamp_size)) {
        return 1;
    }
    if (inspect->has_scan_line_table &&
        tga__ranges_overlap(offset, size,
                            inspect->scan_line_table_offset,
                            inspect->scan_line_table_size)) {
        return 1;
    }
    if (inspect->has_developer_directory &&
        tga__ranges_overlap(offset, size,
                            inspect->developer_directory_offset,
                            inspect->developer_directory_size)) {
        return 1;
    }
    if (inspect->footer_size != 0u &&
        tga__ranges_overlap(offset, size,
                            inspect->footer_offset,
                            inspect->footer_size)) {
        return 1;
    }
    return 0;
}

static int tga__valid_timestamp_fields(const tga_extension *ext)
{
    if (ext->stamp_month == 0u && ext->stamp_day == 0u &&
        ext->stamp_year == 0u && ext->stamp_hour == 0u &&
        ext->stamp_minute == 0u && ext->stamp_second == 0u) {
        return 1;
    }
    if (ext->stamp_month < 1u || ext->stamp_month > 12u) {
        return 0;
    }
    if (ext->stamp_day < 1u || ext->stamp_day > 31u) {
        return 0;
    }
    if (ext->stamp_year == 0u) {
        return 0;
    }
    if (ext->stamp_hour > 23u || ext->stamp_minute > 59u || ext->stamp_second > 59u) {
        return 0;
    }
    return 1;
}

static int tga__valid_job_time_fields(const tga_extension *ext)
{
    if (ext->job_hour == 0u && ext->job_minute == 0u && ext->job_second == 0u) {
        return 1;
    }
    if (ext->job_minute > 59u || ext->job_second > 59u) {
        return 0;
    }
    return 1;
}

static int tga__valid_ratio_fields(unsigned numerator, unsigned denominator)
{
    if (numerator == 0u && denominator == 0u) {
        return 1;
    }
    if (numerator == 0u || denominator == 0u) {
        return 0;
    }
    return 1;
}

static void tga__analyze_extension_semantics(const tga_info *info,
                                             const tga_extension *ext,
                                             tga_forensic_report *out_report)
{
    unsigned attr_bits;

    if (ext->attributes_type > TGA_ATTRIBUTES_TYPE_PREMULTIPLIED) {
        ++out_report->bad_attributes_type_count;
        out_report->flags |= TGA_FORENSIC_BAD_ATTRIBUTES_TYPE;
    }

    attr_bits = info->descriptor & 0x0Fu;
    if (ext->attributes_type <= TGA_ATTRIBUTES_TYPE_PREMULTIPLIED) {
        if ((attr_bits == 0u && ext->attributes_type != TGA_ATTRIBUTES_TYPE_NONE) ||
            (attr_bits != 0u && ext->attributes_type == TGA_ATTRIBUTES_TYPE_NONE)) {
            ++out_report->attribute_type_mismatch_count;
            out_report->flags |= TGA_FORENSIC_ATTRIBUTE_TYPE_MISMATCH;
        }
    }

    if (!tga__valid_timestamp_fields(ext)) {
        ++out_report->bad_timestamp_count;
        out_report->flags |= TGA_FORENSIC_BAD_TIMESTAMP;
    }
    if (!tga__valid_job_time_fields(ext)) {
        ++out_report->bad_job_time_count;
        out_report->flags |= TGA_FORENSIC_BAD_JOB_TIME;
    }
    if (!tga__valid_ratio_fields(ext->pixel_numerator, ext->pixel_denominator)) {
        ++out_report->bad_pixel_aspect_count;
        out_report->flags |= TGA_FORENSIC_BAD_PIXEL_ASPECT;
    }
    if (!tga__valid_ratio_fields(ext->gamma_numerator, ext->gamma_denominator)) {
        ++out_report->bad_gamma_count;
        out_report->flags |= TGA_FORENSIC_BAD_GAMMA;
    }
}

static void tga__analyze_extension_payloads_memory(const tga_u8 *bytes,
                                                   size_t src_size,
                                                   const tga_inspect *inspect,
                                                   tga_forensic_report *out_report)
{
    tga_footer footer;
    tga_extension ext;
    size_t need;
    size_t data_size;
    unsigned sample_bytes;
    size_t stamp_off;
    unsigned stamp_w;
    unsigned stamp_h;

    if (tga_read_footer_memory(bytes, src_size, &footer) != TGA_OK) {
        return;
    }
    if (!footer.present || footer.extension_offset == 0ul) {
        return;
    }
    if (!tga__region_fits((size_t)footer.extension_offset, 495u, src_size)) {
        tga__flag_extension_size_mismatch(out_report);
        return;
    }

    tga__parse_extension_bytes(bytes + (size_t)footer.extension_offset, &ext);
    if (ext.size != 495u) {
        tga__flag_extension_size_mismatch(out_report);
    }
    tga__analyze_extension_semantics(&inspect->info, &ext, out_report);

    if (ext.color_correction_offset != 0ul) {
        need = (size_t)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8u;
        if (!tga__region_fits((size_t)ext.color_correction_offset, need, src_size)) {
            ++out_report->bad_color_correction_ref_count;
            out_report->flags |= TGA_FORENSIC_BAD_COLOR_CORRECTION_REF;
        }
    }

    if (ext.scan_line_offset != 0ul) {
        if (!tga__checked_mul_size((size_t)inspect->info.height, 4u, &need) ||
            !tga__region_fits((size_t)ext.scan_line_offset, need, src_size)) {
            ++out_report->bad_scan_line_ref_count;
            out_report->flags |= TGA_FORENSIC_BAD_SCAN_LINE_REF;
        }
    }

    if (ext.postage_stamp_offset != 0ul) {
        stamp_off = (size_t)ext.postage_stamp_offset;
        if (!tga__region_fits(stamp_off, 2u, src_size)) {
            ++out_report->bad_postage_stamp_ref_count;
            out_report->flags |= TGA_FORENSIC_BAD_POSTAGE_STAMP_REF;
            return;
        }
        stamp_w = (unsigned)bytes[stamp_off + 0u];
        stamp_h = (unsigned)bytes[stamp_off + 1u];
        sample_bytes = tga__file_sample_bytes(inspect->info.pixel_depth);
        if (stamp_w == 0u || stamp_h == 0u ||
            !tga__checked_mul_size((size_t)stamp_w, (size_t)stamp_h, &data_size) ||
            !tga__checked_mul_size(data_size, (size_t)sample_bytes, &data_size) ||
            2u > (size_t)-1 - data_size ||
            !tga__region_fits(stamp_off, 2u + data_size, src_size)) {
            ++out_report->bad_postage_stamp_ref_count;
            out_report->flags |= TGA_FORENSIC_BAD_POSTAGE_STAMP_REF;
        }
    }
}

static void tga__analyze_developer_tags_memory(const tga_u8 *bytes,
                                               const tga_inspect *inspect,
                                               tga_forensic_report *out_report)
{
    size_t dir_off;
    unsigned i;
    unsigned j;
    unsigned tag_i;
    unsigned tag_j;
    size_t off_i;
    size_t off_j;
    size_t size_i;
    size_t size_j;
    size_t expect_size;
    int needs_nul;
    int fits_i;
    int fits_j;

    if (!inspect->has_developer_directory) {
        return;
    }

    dir_off = inspect->developer_directory_offset + 2u;
    for (i = 0u; i < inspect->developer_tag_count; ++i) {
        tag_i = (unsigned)tga__read_u16_le(bytes + dir_off + (size_t)i * 10u + 0u);
        off_i = (size_t)tga__read_u32_le(bytes + dir_off + (size_t)i * 10u + 2u);
        size_i = (size_t)tga__read_u32_le(bytes + dir_off + (size_t)i * 10u + 6u);
        if (size_i == 0u) {
            ++out_report->zero_size_developer_tag_count;
            out_report->flags |= TGA_FORENSIC_ZERO_SIZE_TAG;
        }
        fits_i = tga__region_fits(off_i, size_i, inspect->file_size);
        if (!fits_i) {
            ++out_report->developer_field_out_of_file_count;
            out_report->flags |= TGA_FORENSIC_DEVELOPER_FIELD_OUT_OF_FILE;
        } else if (tga__field_overlaps_high_level(inspect, off_i, size_i)) {
            ++out_report->developer_field_overlap_count;
            out_report->flags |= TGA_FORENSIC_DEVELOPER_FIELD_OVERLAP;
        }

        if (tga__known_utility_payload(tag_i, &expect_size, &needs_nul)) {
            if (needs_nul) {
                if (!fits_i || size_i == 0u || bytes[off_i + size_i - 1u] != 0u) {
                    ++out_report->utility_text_not_terminated_count;
                    out_report->flags |= TGA_FORENSIC_UTILITY_TEXT_NOT_TERMINATED;
                }
            } else if (size_i != expect_size) {
                ++out_report->utility_payload_mismatch_count;
                out_report->flags |= TGA_FORENSIC_UTILITY_PAYLOAD_MISMATCH;
            }
        }

        for (j = i + 1u; j < inspect->developer_tag_count; ++j) {
            tag_j = (unsigned)tga__read_u16_le(bytes + dir_off + (size_t)j * 10u + 0u);
            off_j = (size_t)tga__read_u32_le(bytes + dir_off + (size_t)j * 10u + 2u);
            size_j = (size_t)tga__read_u32_le(bytes + dir_off + (size_t)j * 10u + 6u);
            if (tag_i == tag_j) {
                ++out_report->duplicate_developer_tag_count;
                out_report->flags |= TGA_FORENSIC_DUPLICATE_TAG;
            }
            fits_j = tga__region_fits(off_j, size_j, inspect->file_size);
            if (fits_i && fits_j && tga__ranges_overlap(off_i, size_i, off_j, size_j)) {
                ++out_report->developer_field_overlap_count;
                out_report->flags |= TGA_FORENSIC_DEVELOPER_FIELD_OVERLAP;
            }
        }
    }
}

static void tga__analyze_scanlines_memory(const tga_u8 *bytes,
                                          const tga_inspect *inspect,
                                          tga_forensic_report *out_report)
{
    size_t table_off;
    size_t image_begin;
    size_t image_end;
    size_t prev;
    size_t cur;
    unsigned i;

    if (!inspect->has_scan_line_table) {
        return;
    }

    table_off = inspect->scan_line_table_offset;
    image_begin = inspect->image_data_offset;
    image_end = inspect->image_data_offset + inspect->image_data_size;
    prev = 0u;
    for (i = 0u; i < inspect->info.height; ++i) {
        cur = (size_t)tga__read_u32_le(bytes + table_off + (size_t)i * 4u);
        if (i != 0u && cur < prev) {
            ++out_report->backward_scan_line_count;
            out_report->flags |= TGA_FORENSIC_SCANLINE_BACKWARD;
        }
        if (cur < image_begin || cur >= image_end) {
            ++out_report->out_of_image_scan_line_count;
            out_report->flags |= TGA_FORENSIC_SCANLINE_OUT_OF_IMAGE;
        }
        prev = cur;
    }
}

static int tga__analyze_rle_packets_memory(const tga_u8 *bytes,
                                           size_t src_size,
                                           const tga_inspect *inspect,
                                           tga_forensic_report *out_report)
{
    const tga_info *info;
    size_t sample_bytes;
    size_t total_pixels;
    size_t pos;
    size_t first_pixel;
    size_t need;
    unsigned packet_header;
    unsigned count;
    size_t last_pixel;

    info = &inspect->info;
    if (!info->is_rle) {
        return TGA_OK;
    }
    sample_bytes = (size_t)tga__file_sample_bytes(info->pixel_depth);
    if (!tga__checked_mul_size((size_t)info->width, (size_t)info->height, &total_pixels)) {
        return TGA_ERR_OVERFLOW;
    }
    pos = inspect->image_data_offset;
    first_pixel = 0u;
    while (first_pixel < total_pixels) {
        if (!tga__region_fits(pos, 1u, src_size)) {
            return TGA_ERR_TRUNCATED;
        }
        packet_header = (unsigned)bytes[pos++];
        count = (packet_header & 127u) + 1u;
        if ((size_t)count > total_pixels - first_pixel) {
            return TGA_ERR_BAD_FORMAT;
        }
        if ((packet_header & 128u) != 0u) {
            need = sample_bytes;
        } else {
            if (!tga__checked_mul_size((size_t)count, sample_bytes, &need)) {
                return TGA_ERR_OVERFLOW;
            }
        }
        if (!tga__region_fits(pos, need, src_size)) {
            return TGA_ERR_TRUNCATED;
        }
        last_pixel = first_pixel + (size_t)count - 1u;
        ++out_report->rle_packet_count;
        if (first_pixel / (size_t)info->width != last_pixel / (size_t)info->width) {
            ++out_report->rle_cross_scanline_count;
            out_report->flags |= TGA_FORENSIC_RLE_CROSSES_SCANLINE;
        }
        pos += need;
        first_pixel += (size_t)count;
    }
    return TGA_OK;
}

int tga_forensics_memory(const void *src, size_t src_size,
                         tga_forensic_report *out_report)
{
    tga_section sections[16];
    unsigned count;
    const tga_u8 *bytes;
    int rc;

    if (src == 0 || out_report == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    memset(out_report, 0, sizeof(*out_report));
    rc = tga__inspect_forensics_memory(src, src_size, &out_report->inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__collect_high_level_sections(&out_report->inspect, sections, &count);
    if (rc != TGA_OK) {
        return rc;
    }
    tga__analyze_section_layout(&out_report->inspect, sections, count, out_report);
    bytes = (const tga_u8 *)src;
    tga__analyze_extension_payloads_memory(bytes, src_size,
                                           &out_report->inspect,
                                           out_report);
    tga__analyze_developer_tags_memory(bytes, &out_report->inspect, out_report);
    tga__analyze_scanlines_memory(bytes, &out_report->inspect, out_report);
    return tga__analyze_rle_packets_memory(bytes, src_size, &out_report->inspect, out_report);
}

#ifndef GAFNYF_TGA_NO_STDIO
int tga_decode_color_map_rgba_file(FILE *fp,
                                   tga_u8 *palette_rgba,
                                   unsigned palette_capacity,
                                   tga_palette_info *out_palette_info,
                                   tga_info *out_info)
{
    tga_file_reader fr;
    tga_reader reader;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    fr.fp = fp;
    reader.read = tga_file_read;
    reader.user = &fr;
    return tga_decode_color_map_rgba(&reader, palette_rgba, palette_capacity,
                                     out_palette_info, out_info);
}

int tga_decode_indexed_file(FILE *fp,
                            const tga_decode_indexed_params *params,
                            tga_palette_info *out_palette_info,
                            tga_info *out_info)
{
    tga_file_reader fr;
    tga_reader reader;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    fr.fp = fp;
    reader.read = tga_file_read;
    reader.user = &fr;
    return tga_decode_indexed(&reader, params, out_palette_info, out_info);
}

int tga_list_sections_file(FILE *fp,
                           tga_section *sections,
                           unsigned section_capacity,
                           unsigned *out_section_count)
{
    tga_inspect inspect;
    unsigned need;
    unsigned count;
    unsigned i;
    int rc;
    tga_u8 tagbuf[10];
    size_t field_off;
    size_t field_size;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    if (out_section_count != 0) {
        *out_section_count = 0u;
    }
    rc = tga_inspect_file(fp, &inspect);
    if (rc != TGA_OK) {
        return rc;
    }

    need = 2u;
    if (inspect.image_id_size != 0u) ++need;
    if (inspect.color_map_size != 0u) ++need;
    if (inspect.image_data_size != 0u) ++need;
    if (inspect.has_developer_directory) need += 1u + inspect.developer_tag_count;
    if (inspect.has_extension) ++need;
    if (inspect.has_color_correction_table) ++need;
    if (inspect.has_postage_stamp) ++need;
    if (inspect.has_scan_line_table) ++need;
    if (inspect.footer_size != 0u) ++need;

    if (out_section_count != 0) {
        *out_section_count = need;
    }
    if (sections == 0 || section_capacity == 0u) {
        return TGA_OK;
    }
    if (section_capacity < need) {
        return TGA_ERR_CAPACITY;
    }

    count = 0u;
    tga__append_section(sections, section_capacity, &count,
                        TGA_SECTION_HEADER, 0u, 0u, 18u);
    if (inspect.image_id_size != 0u) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_IMAGE_ID, 0u,
                            inspect.image_id_offset,
                            inspect.image_id_size);
    }
    if (inspect.color_map_size != 0u) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_COLOR_MAP, 0u,
                            inspect.color_map_offset,
                            inspect.color_map_size);
    }
    if (inspect.image_data_size != 0u) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_IMAGE_DATA, 0u,
                            inspect.image_data_offset,
                            inspect.image_data_size);
    }
    if (inspect.has_developer_directory) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_DEVELOPER_DIRECTORY, 0u,
                            inspect.developer_directory_offset,
                            inspect.developer_directory_size);
        for (i = 0u; i < inspect.developer_tag_count; ++i) {
            rc = tga__read_section_file_impl(fp,
                                             inspect.developer_directory_offset + 2u + (size_t)i * 10u,
                                             10u,
                                             tagbuf,
                                             sizeof(tagbuf),
                                             0);
            if (rc != TGA_OK) {
                return rc;
            }
            field_off = (size_t)tga__read_u32_le(tagbuf + 2u);
            field_size = (size_t)tga__read_u32_le(tagbuf + 6u);
            tga__append_section(sections, section_capacity, &count,
                                TGA_SECTION_DEVELOPER_FIELD,
                                (unsigned)tga__read_u16_le(tagbuf + 0u),
                                field_off,
                                field_size);
        }
    }
    if (inspect.has_extension) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_EXTENSION_AREA, 0u,
                            inspect.extension_area_offset,
                            inspect.extension_area_size);
    }
    if (inspect.has_color_correction_table) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_COLOR_CORRECTION_TABLE, 0u,
                            inspect.color_correction_offset,
                            inspect.color_correction_size);
    }
    if (inspect.has_postage_stamp) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_POSTAGE_STAMP, 0u,
                            inspect.postage_stamp_offset,
                            inspect.postage_stamp_size);
    }
    if (inspect.has_scan_line_table) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_SCAN_LINE_TABLE, 0u,
                            inspect.scan_line_table_offset,
                            inspect.scan_line_table_size);
    }
    if (inspect.footer_size != 0u) {
        tga__append_section(sections, section_capacity, &count,
                            TGA_SECTION_FOOTER, 0u,
                            inspect.footer_offset,
                            inspect.footer_size);
    }

    tga__sort_sections(sections, count);
    if (out_section_count != 0) {
        *out_section_count = count;
    }
    return TGA_OK;
}

static void tga__analyze_extension_payloads_file(FILE *fp,
                                                   const tga_inspect *inspect,
                                                   tga_forensic_report *out_report)
{
    tga_footer footer;
    tga_extension ext;
    tga_u8 ext_buf[495];
    tga_u8 stamp_buf[2];
    size_t need;
    size_t data_size;
    size_t stamp_off;
    unsigned stamp_w;
    unsigned stamp_h;
    unsigned sample_bytes;
    int rc;

    rc = tga_read_footer_file(fp, &footer);
    if (rc != TGA_OK || !footer.present || footer.extension_offset == 0ul) {
        return;
    }
    if (!tga__region_fits((size_t)footer.extension_offset, 495u, inspect->file_size)) {
        tga__flag_extension_size_mismatch(out_report);
        return;
    }
    rc = tga__read_section_file_impl(fp,
                                     (size_t)footer.extension_offset,
                                     495u,
                                     ext_buf,
                                     sizeof(ext_buf),
                                     0);
    if (rc != TGA_OK) {
        tga__flag_extension_size_mismatch(out_report);
        return;
    }

    tga__parse_extension_bytes(ext_buf, &ext);
    if (ext.size != 495u) {
        tga__flag_extension_size_mismatch(out_report);
    }
    tga__analyze_extension_semantics(&inspect->info, &ext, out_report);

    if (ext.color_correction_offset != 0ul) {
        need = (size_t)TGA_COLOR_CORRECTION_ENTRY_COUNT * 8u;
        if (!tga__region_fits((size_t)ext.color_correction_offset, need,
                              inspect->file_size)) {
            ++out_report->bad_color_correction_ref_count;
            out_report->flags |= TGA_FORENSIC_BAD_COLOR_CORRECTION_REF;
        }
    }

    if (ext.scan_line_offset != 0ul) {
        if (!tga__checked_mul_size((size_t)inspect->info.height, 4u, &need) ||
            !tga__region_fits((size_t)ext.scan_line_offset, need,
                              inspect->file_size)) {
            ++out_report->bad_scan_line_ref_count;
            out_report->flags |= TGA_FORENSIC_BAD_SCAN_LINE_REF;
        }
    }

    if (ext.postage_stamp_offset != 0ul) {
        stamp_off = (size_t)ext.postage_stamp_offset;
        rc = tga__read_section_file_impl(fp,
                                         stamp_off,
                                         2u,
                                         stamp_buf,
                                         sizeof(stamp_buf),
                                         0);
        if (rc != TGA_OK) {
            ++out_report->bad_postage_stamp_ref_count;
            out_report->flags |= TGA_FORENSIC_BAD_POSTAGE_STAMP_REF;
            return;
        }
        stamp_w = (unsigned)stamp_buf[0];
        stamp_h = (unsigned)stamp_buf[1];
        sample_bytes = tga__file_sample_bytes(inspect->info.pixel_depth);
        if (stamp_w == 0u || stamp_h == 0u ||
            !tga__checked_mul_size((size_t)stamp_w, (size_t)stamp_h, &data_size) ||
            !tga__checked_mul_size(data_size, (size_t)sample_bytes, &data_size) ||
            2u > (size_t)-1 - data_size ||
            !tga__region_fits(stamp_off, 2u + data_size, inspect->file_size)) {
            ++out_report->bad_postage_stamp_ref_count;
            out_report->flags |= TGA_FORENSIC_BAD_POSTAGE_STAMP_REF;
        }
    }
}

static void tga__analyze_developer_tags_file(FILE *fp,
                                             const tga_inspect *inspect,
                                             tga_forensic_report *out_report)
{
    unsigned i;
    unsigned j;
    tga_u8 tag_i[10];
    tga_u8 tag_j[10];
    tga_u8 last_byte[1];
    int rc;
    unsigned id_i;
    unsigned id_j;
    size_t off_i;
    size_t off_j;
    size_t size_i;
    size_t size_j;
    size_t expect_size;
    int needs_nul;
    int fits_i;
    int fits_j;

    if (!inspect->has_developer_directory) {
        return;
    }
    for (i = 0u; i < inspect->developer_tag_count; ++i) {
        rc = tga__read_section_file_impl(fp,
                                         inspect->developer_directory_offset + 2u + (size_t)i * 10u,
                                         10u,
                                         tag_i,
                                         sizeof(tag_i),
                                         0);
        if (rc != TGA_OK) {
            return;
        }
        id_i = (unsigned)tga__read_u16_le(tag_i + 0u);
        off_i = (size_t)tga__read_u32_le(tag_i + 2u);
        size_i = (size_t)tga__read_u32_le(tag_i + 6u);
        if (size_i == 0ul) {
            ++out_report->zero_size_developer_tag_count;
            out_report->flags |= TGA_FORENSIC_ZERO_SIZE_TAG;
        }
        fits_i = tga__region_fits(off_i, size_i, inspect->file_size);
        if (!fits_i) {
            ++out_report->developer_field_out_of_file_count;
            out_report->flags |= TGA_FORENSIC_DEVELOPER_FIELD_OUT_OF_FILE;
        } else if (tga__field_overlaps_high_level(inspect, off_i, size_i)) {
            ++out_report->developer_field_overlap_count;
            out_report->flags |= TGA_FORENSIC_DEVELOPER_FIELD_OVERLAP;
        }

        if (tga__known_utility_payload(id_i, &expect_size, &needs_nul)) {
            if (needs_nul) {
                if (!fits_i || size_i == 0u) {
                    ++out_report->utility_text_not_terminated_count;
                    out_report->flags |= TGA_FORENSIC_UTILITY_TEXT_NOT_TERMINATED;
                } else {
                    rc = tga__read_section_file_impl(fp,
                                                     off_i + size_i - 1u,
                                                     1u,
                                                     last_byte,
                                                     sizeof(last_byte),
                                                     0);
                    if (rc != TGA_OK || last_byte[0] != 0u) {
                        ++out_report->utility_text_not_terminated_count;
                        out_report->flags |= TGA_FORENSIC_UTILITY_TEXT_NOT_TERMINATED;
                    }
                }
            } else if (size_i != expect_size) {
                ++out_report->utility_payload_mismatch_count;
                out_report->flags |= TGA_FORENSIC_UTILITY_PAYLOAD_MISMATCH;
            }
        }

        for (j = i + 1u; j < inspect->developer_tag_count; ++j) {
            rc = tga__read_section_file_impl(fp,
                                             inspect->developer_directory_offset + 2u + (size_t)j * 10u,
                                             10u,
                                             tag_j,
                                             sizeof(tag_j),
                                             0);
            if (rc != TGA_OK) {
                return;
            }
            id_j = (unsigned)tga__read_u16_le(tag_j + 0u);
            off_j = (size_t)tga__read_u32_le(tag_j + 2u);
            size_j = (size_t)tga__read_u32_le(tag_j + 6u);
            if (id_i == id_j) {
                ++out_report->duplicate_developer_tag_count;
                out_report->flags |= TGA_FORENSIC_DUPLICATE_TAG;
            }
            fits_j = tga__region_fits(off_j, size_j, inspect->file_size);
            if (fits_i && fits_j && tga__ranges_overlap(off_i, size_i, off_j, size_j)) {
                ++out_report->developer_field_overlap_count;
                out_report->flags |= TGA_FORENSIC_DEVELOPER_FIELD_OVERLAP;
            }
        }
    }
}

static void tga__analyze_scanlines_file(FILE *fp,
                                        const tga_inspect *inspect,
                                        tga_forensic_report *out_report)
{
    unsigned i;
    tga_u8 buf[4];
    size_t prev;
    size_t cur;
    size_t image_begin;
    size_t image_end;
    int rc;

    if (!inspect->has_scan_line_table) {
        return;
    }
    image_begin = inspect->image_data_offset;
    image_end = inspect->image_data_offset + inspect->image_data_size;
    prev = 0u;
    for (i = 0u; i < inspect->info.height; ++i) {
        rc = tga__read_section_file_impl(fp,
                                         inspect->scan_line_table_offset + (size_t)i * 4u,
                                         4u,
                                         buf,
                                         sizeof(buf),
                                         0);
        if (rc != TGA_OK) {
            return;
        }
        cur = (size_t)tga__read_u32_le(buf);
        if (i != 0u && cur < prev) {
            ++out_report->backward_scan_line_count;
            out_report->flags |= TGA_FORENSIC_SCANLINE_BACKWARD;
        }
        if (cur < image_begin || cur >= image_end) {
            ++out_report->out_of_image_scan_line_count;
            out_report->flags |= TGA_FORENSIC_SCANLINE_OUT_OF_IMAGE;
        }
        prev = cur;
    }
}

static int tga__analyze_rle_packets_file(FILE *fp,
                                         const tga_inspect *inspect,
                                         tga_forensic_report *out_report)
{
    const tga_info *info;
    size_t sample_bytes;
    size_t total_pixels;
    size_t pos;
    size_t first_pixel;
    size_t need;
    size_t last_pixel;
    tga_u8 hdr[1];
    unsigned packet_header;
    unsigned count;
    int rc;

    info = &inspect->info;
    if (!info->is_rle) {
        return TGA_OK;
    }
    sample_bytes = (size_t)tga__file_sample_bytes(info->pixel_depth);
    if (!tga__checked_mul_size((size_t)info->width, (size_t)info->height, &total_pixels)) {
        return TGA_ERR_OVERFLOW;
    }
    pos = inspect->image_data_offset;
    first_pixel = 0u;
    while (first_pixel < total_pixels) {
        rc = tga__read_section_file_impl(fp, pos, 1u, hdr, sizeof(hdr), 0);
        if (rc != TGA_OK) {
            return rc;
        }
        packet_header = (unsigned)hdr[0];
        ++pos;
        count = (packet_header & 127u) + 1u;
        if ((size_t)count > total_pixels - first_pixel) {
            return TGA_ERR_BAD_FORMAT;
        }
        if ((packet_header & 128u) != 0u) {
            need = sample_bytes;
        } else {
            if (!tga__checked_mul_size((size_t)count, sample_bytes, &need)) {
                return TGA_ERR_OVERFLOW;
            }
        }
        last_pixel = first_pixel + (size_t)count - 1u;
        ++out_report->rle_packet_count;
        if (first_pixel / (size_t)info->width != last_pixel / (size_t)info->width) {
            ++out_report->rle_cross_scanline_count;
            out_report->flags |= TGA_FORENSIC_RLE_CROSSES_SCANLINE;
        }
        pos += need;
        first_pixel += (size_t)count;
    }
    return TGA_OK;
}

int tga_forensics_file(FILE *fp, tga_forensic_report *out_report)
{
    tga_section sections[16];
    unsigned count;
    int rc;

    if (fp == 0 || out_report == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }
    memset(out_report, 0, sizeof(*out_report));
    rc = tga__inspect_forensics_file(fp, &out_report->inspect);
    if (rc != TGA_OK) {
        return rc;
    }
    rc = tga__collect_high_level_sections(&out_report->inspect, sections, &count);
    if (rc != TGA_OK) {
        return rc;
    }
    tga__analyze_section_layout(&out_report->inspect, sections, count, out_report);
    tga__analyze_extension_payloads_file(fp, &out_report->inspect, out_report);
    tga__analyze_developer_tags_file(fp, &out_report->inspect, out_report);
    tga__analyze_scanlines_file(fp, &out_report->inspect, out_report);
    return tga__analyze_rle_packets_file(fp, &out_report->inspect, out_report);
}

#endif

#ifndef GAFNYF_TGA_NO_STDIO
int tga_decode_swizzle_file(FILE *fp,
                            const tga_decode_swizzle_params *params,
                            tga_info *out_info)
{
    tga_file_reader fr;
    tga_reader reader;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    fr.fp = fp;
    reader.read = tga_file_read;
    reader.user = &fr;
    return tga_decode_swizzle(&reader, params, out_info);
}

int tga_decode_channels_file(FILE *fp,
                             const tga_decode_channels_params *params,
                             tga_info *out_info)
{
    tga_file_reader fr;
    tga_reader reader;

    if (fp == 0) {
        return TGA_ERR_BAD_ARGUMENT;
    }

    fr.fp = fp;
    reader.read = tga_file_read;
    reader.user = &fr;
    return tga_decode_channels(&reader, params, out_info);
}
#endif
