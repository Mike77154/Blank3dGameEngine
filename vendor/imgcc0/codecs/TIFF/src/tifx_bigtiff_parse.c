/* SPDX-License-Identifier: CC0-1.0 */
#include "tifx.h"
#include "tifx_internal.h"

#include <limits.h>
#include <string.h>

#define TIFX_TYPE_SHORT 3U
#define TIFX_TYPE_LONG 4U
#define TIFX_TYPE_RATIONAL 5U
#define TIFX_TYPE_LONG8 16U
#define TIFX_TYPE_IFD8 18U

#define TIFX_TAG_NEW_SUBFILE_TYPE 254U
#define TIFX_TAG_IMAGE_WIDTH 256U
#define TIFX_TAG_IMAGE_LENGTH 257U
#define TIFX_TAG_BITS_PER_SAMPLE 258U
#define TIFX_TAG_COMPRESSION 259U
#define TIFX_TAG_PHOTOMETRIC 262U
#define TIFX_TAG_FILL_ORDER 266U
#define TIFX_TAG_STRIP_OFFSETS 273U
#define TIFX_TAG_ORIENTATION 274U
#define TIFX_TAG_SAMPLES_PER_PIXEL 277U
#define TIFX_TAG_ROWS_PER_STRIP 278U
#define TIFX_TAG_STRIP_BYTE_COUNTS 279U
#define TIFX_TAG_X_RESOLUTION 282U
#define TIFX_TAG_Y_RESOLUTION 283U
#define TIFX_TAG_PLANAR_CONFIGURATION 284U
#define TIFX_TAG_T4_OPTIONS 292U
#define TIFX_TAG_T6_OPTIONS 293U
#define TIFX_TAG_RESOLUTION_UNIT 296U
#define TIFX_TAG_PAGE_NUMBER 297U
#define TIFX_TAG_PREDICTOR 317U
#define TIFX_TAG_TILE_WIDTH 322U
#define TIFX_TAG_TILE_LENGTH 323U
#define TIFX_TAG_TILE_OFFSETS 324U
#define TIFX_TAG_TILE_BYTE_COUNTS 325U
#define TIFX_TAG_COLOR_MAP 320U
#define TIFX_TAG_SUBIFDS 330U
#define TIFX_TAG_EXTRA_SAMPLES 338U
#define TIFX_TAG_SAMPLE_FORMAT 339U

static unsigned short tifx_bt_read_u16(const unsigned char *p, int is_big_endian)
{
    if (is_big_endian) {
        return (unsigned short)(((unsigned short)p[0] << 8) | (unsigned short)p[1]);
    }
    return (unsigned short)(((unsigned short)p[1] << 8) | (unsigned short)p[0]);
}

static unsigned long tifx_bt_read_u32(const unsigned char *p, int is_big_endian)
{
    if (is_big_endian) {
        return ((unsigned long)p[0] << 24) |
               ((unsigned long)p[1] << 16) |
               ((unsigned long)p[2] << 8) |
               (unsigned long)p[3];
    }
    return ((unsigned long)p[3] << 24) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[1] << 8) |
           (unsigned long)p[0];
}

#if ULONG_MAX > 0xFFFFFFFFUL
static unsigned long tifx_bt_read_u64(const unsigned char *p, int is_big_endian)
{
    if (is_big_endian) {
        return ((unsigned long)p[0] << 56) |
               ((unsigned long)p[1] << 48) |
               ((unsigned long)p[2] << 40) |
               ((unsigned long)p[3] << 32) |
               ((unsigned long)p[4] << 24) |
               ((unsigned long)p[5] << 16) |
               ((unsigned long)p[6] << 8) |
               (unsigned long)p[7];
    }
    return ((unsigned long)p[7] << 56) |
           ((unsigned long)p[6] << 48) |
           ((unsigned long)p[5] << 40) |
           ((unsigned long)p[4] << 32) |
           ((unsigned long)p[3] << 24) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[1] << 8) |
           (unsigned long)p[0];
}
#else
static unsigned long tifx_bt_read_u64(const unsigned char *p, int is_big_endian)
{
    (void)p;
    (void)is_big_endian;
    return 0UL;
}
#endif

static int tifx_bt_add_ul(unsigned long a, unsigned long b, unsigned long *out)
{
    if (a > ULONG_MAX - b) {
        return 0;
    }
    *out = a + b;
    return 1;
}

static int tifx_bt_mul_ul(unsigned long a, unsigned long b, unsigned long *out)
{
    if (a != 0UL && b > ULONG_MAX / a) {
        return 0;
    }
    *out = a * b;
    return 1;
}

static unsigned long tifx_bt_ceil_div(unsigned long value, unsigned long divisor)
{
    if (value == 0UL) {
        return 0UL;
    }
    return ((value - 1UL) / divisor) + 1UL;
}

static unsigned long tifx_bt_type_size(unsigned short type)
{
    switch (type) {
        case TIFX_TYPE_SHORT: return 2UL;
        case TIFX_TYPE_LONG: return 4UL;
        case TIFX_TYPE_RATIONAL: return 8UL;
        case TIFX_TYPE_LONG8: return 8UL;
        case TIFX_TYPE_IFD8: return 8UL;
        default: return 0UL;
    }
}

static int tifx_bt_entry_ptr(const unsigned char *file_bytes,
                             unsigned long file_size,
                             int is_big_endian,
                             const unsigned char *entry,
                             unsigned short expected_type,
                             unsigned long expected_count,
                             const unsigned char **out_ptr)
{
    unsigned short type;
    unsigned long count;
    unsigned long item_size;
    unsigned long total_size;
    unsigned long offset;

    type = tifx_bt_read_u16(entry + 2, is_big_endian);
    count = tifx_bt_read_u64(entry + 4, is_big_endian);
    if (type != expected_type || count != expected_count) {
        return TIFX_ERR_BAD_FORMAT;
    }

    item_size = tifx_bt_type_size(type);
    if (item_size == 0UL) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (!tifx_bt_mul_ul(item_size, count, &total_size)) {
        return TIFX_ERR_OVERFLOW;
    }

    if (total_size <= 8UL) {
        *out_ptr = entry + 12;
        return TIFX_OK;
    }

    offset = tifx_bt_read_u64(entry + 12, is_big_endian);
    if (offset > file_size) {
        return TIFX_ERR_TRUNCATED;
    }
    if (total_size > file_size - offset) {
        return TIFX_ERR_TRUNCATED;
    }
    *out_ptr = file_bytes + offset;
    return TIFX_OK;
}

static int tifx_bt_entry_scalar_u32(const unsigned char *file_bytes,
                                    unsigned long file_size,
                                    int is_big_endian,
                                    const unsigned char *entry,
                                    unsigned short type,
                                    unsigned long count,
                                    unsigned long *out_value)
{
    const unsigned char *ptr;
    int rc;

    if (count != 1UL) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (type != TIFX_TYPE_SHORT && type != TIFX_TYPE_LONG &&
        type != TIFX_TYPE_LONG8 && type != TIFX_TYPE_IFD8) {
        return TIFX_ERR_UNSUPPORTED;
    }

    rc = tifx_bt_entry_ptr(file_bytes, file_size, is_big_endian,
                           entry, type, 1UL, &ptr);
    if (rc != TIFX_OK) {
        return rc;
    }

    if (type == TIFX_TYPE_SHORT) {
        *out_value = (unsigned long)tifx_bt_read_u16(ptr, is_big_endian);
    } else if (type == TIFX_TYPE_LONG) {
        *out_value = tifx_bt_read_u32(ptr, is_big_endian);
    } else {
        *out_value = tifx_bt_read_u64(ptr, is_big_endian);
    }
    return TIFX_OK;
}

static int tifx_bt_entry_short_array(const unsigned char *file_bytes,
                                     unsigned long file_size,
                                     int is_big_endian,
                                     const unsigned char *entry,
                                     unsigned long count,
                                     unsigned short *out_values,
                                     unsigned long max_values)
{
    const unsigned char *ptr;
    unsigned long i;
    int rc;

    if (count > max_values) {
        return TIFX_ERR_UNSUPPORTED;
    }

    rc = tifx_bt_entry_ptr(file_bytes, file_size, is_big_endian,
                           entry, TIFX_TYPE_SHORT, count, &ptr);
    if (rc != TIFX_OK) {
        return rc;
    }

    for (i = 0UL; i < count; ++i) {
        out_values[i] = tifx_bt_read_u16(ptr + (i * 2UL), is_big_endian);
    }
    return TIFX_OK;
}

static int tifx_bt_entry_u32_array(const unsigned char *file_bytes,
                                   unsigned long file_size,
                                   int is_big_endian,
                                   const unsigned char *entry,
                                   unsigned short type,
                                   unsigned long count,
                                   unsigned long *out_values,
                                   unsigned long max_values)
{
    const unsigned char *ptr;
    unsigned long item_size;
    unsigned long i;

    if (count > max_values) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (type != TIFX_TYPE_SHORT && type != TIFX_TYPE_LONG &&
        type != TIFX_TYPE_LONG8 && type != TIFX_TYPE_IFD8) {
        return TIFX_ERR_UNSUPPORTED;
    }

    item_size = tifx_bt_type_size(type);
    if (item_size == 0UL) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (!tifx_bt_mul_ul(item_size, count, &item_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (item_size <= 8UL) {
        ptr = entry + 12;
    } else {
        unsigned long offset;
        offset = tifx_bt_read_u64(entry + 12, is_big_endian);
        if (offset > file_size) {
            return TIFX_ERR_TRUNCATED;
        }
        if (item_size > file_size - offset) {
            return TIFX_ERR_TRUNCATED;
        }
        ptr = file_bytes + offset;
    }

    if (type == TIFX_TYPE_SHORT) {
        for (i = 0UL; i < count; ++i) {
            out_values[i] = (unsigned long)tifx_bt_read_u16(ptr + (i * 2UL), is_big_endian);
        }
    } else if (type == TIFX_TYPE_LONG) {
        for (i = 0UL; i < count; ++i) {
            out_values[i] = tifx_bt_read_u32(ptr + (i * 4UL), is_big_endian);
        }
    } else {
        for (i = 0UL; i < count; ++i) {
            out_values[i] = tifx_bt_read_u64(ptr + (i * 8UL), is_big_endian);
        }
    }
    return TIFX_OK;
}

static int tifx_bt_entry_rational_fixed(const unsigned char *file_bytes,
                                        unsigned long file_size,
                                        int is_big_endian,
                                        const unsigned char *entry,
                                        tifx_fixed *out_value)
{
    const unsigned char *ptr;
    unsigned long numerator;
    unsigned long denominator;
    int rc;

    rc = tifx_bt_entry_ptr(file_bytes, file_size, is_big_endian,
                           entry, TIFX_TYPE_RATIONAL, 1UL, &ptr);
    if (rc != TIFX_OK) {
        return rc;
    }

    numerator = tifx_bt_read_u32(ptr, is_big_endian);
    denominator = tifx_bt_read_u32(ptr + 4, is_big_endian);
    *out_value = tifx_fp_from_ratio(numerator, denominator);
    return TIFX_OK;
}

static int tifx_bt_copy_bits_defaults(tifx_image_info *info,
                                      const unsigned short *bits_values,
                                      unsigned short bits_count)
{
    unsigned short i;

    if (bits_count == 0U) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (bits_count > TIFX_MAX_SAMPLES) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (bits_count != info->samples_per_pixel) {
        return TIFX_ERR_BAD_FORMAT;
    }

    info->bits_per_sample_count = bits_count;
    for (i = 0U; i < bits_count; ++i) {
        info->bits_per_sample[i] = bits_values[i];
    }
    return TIFX_OK;
}

static int tifx_bt_validate_layout(tifx_image_info *info,
                                   unsigned short sample_format_values_present,
                                   const unsigned short *sample_format_values,
                                   unsigned short sample_format_count)
{
    unsigned short bits0;
    unsigned long palette_entries;
    unsigned short i;

    if (sample_format_values_present) {
        if (sample_format_count == 0U || sample_format_count > info->samples_per_pixel) {
            return TIFX_ERR_BAD_FORMAT;
        }
        for (i = 0U; i < sample_format_count; ++i) {
            if (sample_format_values[i] != 1U) {
                return TIFX_ERR_UNSUPPORTED;
            }
        }
    }

    if (info->bits_per_sample_count == 0U) {
        return TIFX_ERR_BAD_FORMAT;
    }
    bits0 = info->bits_per_sample[0];

    if (info->extra_samples_count > info->samples_per_pixel) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (info->fill_order != 1U && info->fill_order != 2U) {
        return TIFX_ERR_UNSUPPORTED;
    }

    if (info->compression == 2U || info->compression == 3U || info->compression == 4U) {
        if ((info->photometric != 0U && info->photometric != 1U) ||
            info->samples_per_pixel != 1U ||
            bits0 != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
    }
    if ((info->compression == 5U || info->compression == 8U || info->compression == 32946U) &&
        info->fill_order != 1U) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (info->compression == 3U && (info->t4_options & ~7UL) != 0UL) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (info->compression == 4U && (info->t6_options & ~2UL) != 0UL) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (info->predictor == 0U) {
        info->predictor = TIFX_PREDICTOR_NONE;
    }
    if (info->predictor != TIFX_PREDICTOR_NONE &&
        info->predictor != TIFX_PREDICTOR_HORIZONTAL) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (info->predictor != TIFX_PREDICTOR_NONE &&
        info->compression != 5U && info->compression != 8U && info->compression != 32946U) {
        return TIFX_ERR_UNSUPPORTED;
    }

    if (info->photometric == 0U || info->photometric == 1U) {
        if (info->samples_per_pixel != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (bits0 != 1U && bits0 != 4U && bits0 != 8U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (info->predictor == TIFX_PREDICTOR_HORIZONTAL && bits0 != 8U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        info->pixel_format = TIFX_PIXEL_GRAY8;
        return TIFX_OK;
    }

    if (info->photometric == 3U) {
        if (info->samples_per_pixel != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (bits0 != 4U && bits0 != 8U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (info->predictor != TIFX_PREDICTOR_NONE) {
            return TIFX_ERR_UNSUPPORTED;
        }
        palette_entries = 1UL << bits0;
        if ((unsigned long)info->color_map_count != (palette_entries * 3UL)) {
            return TIFX_ERR_BAD_FORMAT;
        }
        info->pixel_format = TIFX_PIXEL_RGB24;
        return TIFX_OK;
    }

    if (info->photometric == 2U) {
        if (info->samples_per_pixel != 3U && info->samples_per_pixel != 4U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (info->bits_per_sample_count < info->samples_per_pixel) {
            return TIFX_ERR_BAD_FORMAT;
        }
        for (i = 0U; i < info->samples_per_pixel; ++i) {
            if (info->bits_per_sample[i] != 8U) {
                return TIFX_ERR_UNSUPPORTED;
            }
        }
        if (info->predictor == TIFX_PREDICTOR_HORIZONTAL &&
            info->planar_config != 1U && info->planar_config != 2U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (info->samples_per_pixel == 4U &&
            (info->alpha_mode == TIFX_ALPHA_ASSOCIATED ||
             info->alpha_mode == TIFX_ALPHA_UNASSOCIATED)) {
            info->pixel_format = TIFX_PIXEL_RGBA32;
            return TIFX_OK;
        }
        info->pixel_format = TIFX_PIXEL_RGB24;
        return TIFX_OK;
    }

    return TIFX_ERR_UNSUPPORTED;
}

static int tifx_bt_read_header(const unsigned char *file_bytes,
                               unsigned long size,
                               int *out_is_big_endian,
                               unsigned long *out_first_ifd_offset)
{
    int is_big_endian;
    unsigned long first_ifd_offset;

    if (file_bytes == 0 || out_is_big_endian == 0 || out_first_ifd_offset == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (size < 16UL) {
        return TIFX_ERR_TRUNCATED;
    }

    if (file_bytes[0] == 'I' && file_bytes[1] == 'I') {
        is_big_endian = 0;
    } else if (file_bytes[0] == 'M' && file_bytes[1] == 'M') {
        is_big_endian = 1;
    } else {
        return TIFX_ERR_BAD_FORMAT;
    }

    if (tifx_bt_read_u16(file_bytes + 2, is_big_endian) != 43U) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (tifx_bt_read_u16(file_bytes + 4, is_big_endian) != 8U) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (tifx_bt_read_u16(file_bytes + 6, is_big_endian) != 0U) {
        return TIFX_ERR_BAD_FORMAT;
    }

    first_ifd_offset = tifx_bt_read_u64(file_bytes + 8, is_big_endian);
    if (first_ifd_offset == 0UL) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (first_ifd_offset > size || size - first_ifd_offset < 8UL) {
        return TIFX_ERR_TRUNCATED;
    }

    *out_is_big_endian = is_big_endian;
    *out_first_ifd_offset = first_ifd_offset;
    return TIFX_OK;
}

static int tifx_bt_read_ifd_trailer(const unsigned char *file_bytes,
                                    unsigned long size,
                                    int is_big_endian,
                                    unsigned long ifd_offset,
                                    unsigned long *out_tag_count,
                                    unsigned long *out_next_ifd_offset)
{
    unsigned long tag_count;
    unsigned long entry_table_bytes;
    unsigned long next_ifd_offset;

    if (file_bytes == 0 || out_tag_count == 0 || out_next_ifd_offset == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (ifd_offset > size || size - ifd_offset < 8UL) {
        return TIFX_ERR_TRUNCATED;
    }

    tag_count = tifx_bt_read_u64(file_bytes + ifd_offset, is_big_endian);
    if (!tifx_bt_mul_ul(tag_count, 20UL, &entry_table_bytes)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (!tifx_bt_add_ul(entry_table_bytes, 16UL, &entry_table_bytes)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (entry_table_bytes > size - ifd_offset) {
        return TIFX_ERR_TRUNCATED;
    }

    next_ifd_offset = tifx_bt_read_u64(file_bytes + ifd_offset + 8UL + (tag_count * 20UL),
                                       is_big_endian);
    if (next_ifd_offset != 0UL) {
        if (next_ifd_offset > size || size - next_ifd_offset < 8UL) {
            return TIFX_ERR_TRUNCATED;
        }
    }

    *out_tag_count = tag_count;
    *out_next_ifd_offset = next_ifd_offset;
    return TIFX_OK;
}

static int tifx_bt_walk_ifds(const unsigned char *file_bytes,
                             unsigned long size,
                             int is_big_endian,
                             unsigned long first_ifd_offset,
                             unsigned long target_page_index,
                             unsigned long *out_target_ifd_offset,
                             unsigned long *out_page_count)
{
    unsigned long seen_offsets[TIFX_MAX_PAGES];
    unsigned long current_ifd_offset;
    unsigned long next_ifd_offset;
    unsigned long tag_count;
    unsigned long page_count;
    unsigned long i;
    int rc;

    current_ifd_offset = first_ifd_offset;
    page_count = 0UL;
    while (current_ifd_offset != 0UL) {
        if (page_count >= TIFX_MAX_PAGES) {
            return TIFX_ERR_UNSUPPORTED;
        }
        for (i = 0UL; i < page_count; ++i) {
            if (seen_offsets[i] == current_ifd_offset) {
                return TIFX_ERR_BAD_FORMAT;
            }
        }
        seen_offsets[page_count] = current_ifd_offset;
        if (out_target_ifd_offset != 0 && page_count == target_page_index) {
            *out_target_ifd_offset = current_ifd_offset;
        }
        ++page_count;

        rc = tifx_bt_read_ifd_trailer(file_bytes,
                                      size,
                                      is_big_endian,
                                      current_ifd_offset,
                                      &tag_count,
                                      &next_ifd_offset);
        if (rc != TIFX_OK) {
            return rc;
        }
        (void)tag_count;
        current_ifd_offset = next_ifd_offset;
    }

    if (page_count == 0UL) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (out_page_count != 0) {
        *out_page_count = page_count;
    }
    if (out_target_ifd_offset != 0 && target_page_index >= page_count) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    return TIFX_OK;
}

static int tifx_bt_read_subifd_offsets(const unsigned char *file_bytes,
                                      unsigned long size,
                                      int is_big_endian,
                                      unsigned long ifd_offset,
                                      unsigned long *out_offsets,
                                      unsigned long max_offsets,
                                      unsigned long *out_count)
{
    unsigned long tag_count;
    unsigned long next_ifd_offset;
    unsigned long i;
    int rc;

    if (file_bytes == 0 || out_offsets == 0 || out_count == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    *out_count = 0UL;
    rc = tifx_bt_read_ifd_trailer(file_bytes,
                                  size,
                                  is_big_endian,
                                  ifd_offset,
                                  &tag_count,
                                  &next_ifd_offset);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)next_ifd_offset;

    for (i = 0UL; i < tag_count; ++i) {
        const unsigned char *entry;
        unsigned short tag;
        unsigned short type;
        unsigned long count;

        entry = file_bytes + ifd_offset + 8UL + (i * 20UL);
        tag = tifx_bt_read_u16(entry, is_big_endian);
        if (tag != TIFX_TAG_SUBIFDS) {
            continue;
        }

        type = tifx_bt_read_u16(entry + 2, is_big_endian);
        count = tifx_bt_read_u64(entry + 4, is_big_endian);
        if (type != TIFX_TYPE_IFD8 && type != TIFX_TYPE_LONG8 && type != TIFX_TYPE_LONG) {
            return TIFX_ERR_UNSUPPORTED;
        }
        rc = tifx_bt_entry_u32_array(file_bytes,
                                     size,
                                     is_big_endian,
                                     entry,
                                     type,
                                     count,
                                     out_offsets,
                                     max_offsets);
        if (rc != TIFX_OK) {
            return rc;
        }
        *out_count = count;

        if (count == 1UL && max_offsets > 1UL && out_offsets[0] != 0UL) {
            unsigned long chain_count;
            unsigned long current_child_offset;

            chain_count = 1UL;
            current_child_offset = out_offsets[0];
            while (current_child_offset != 0UL) {
                unsigned long child_tag_count;
                unsigned long child_next_ifd_offset;

                rc = tifx_bt_read_ifd_trailer(file_bytes,
                                              size,
                                              is_big_endian,
                                              current_child_offset,
                                              &child_tag_count,
                                              &child_next_ifd_offset);
                if (rc != TIFX_OK) {
                    return rc;
                }
                (void)child_tag_count;
                if (child_next_ifd_offset == 0UL) {
                    break;
                }
                if (child_next_ifd_offset == current_child_offset) {
                    return TIFX_ERR_BAD_FORMAT;
                }
                if (chain_count >= max_offsets) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                out_offsets[chain_count++] = child_next_ifd_offset;
                current_child_offset = child_next_ifd_offset;
            }
            *out_count = chain_count;
        }
        return TIFX_OK;
    }

    return TIFX_OK;
}

static int tifx_bt_walk_subifd_path(const unsigned char *file_bytes,
                                    unsigned long size,
                                    int is_big_endian,
                                    unsigned long first_ifd_offset,
                                    unsigned long page_index,
                                    const unsigned long *subifd_path,
                                    unsigned long subifd_path_length,
                                    unsigned long *out_page_count,
                                    unsigned long *out_parent_ifd_offset,
                                    unsigned long *out_current_ifd_offset,
                                    unsigned long *out_subifd_index)
{
    unsigned long current_ifd_offset;
    unsigned long page_count;
    unsigned long sub_offsets[TIFX_MAX_SUBIFDS];
    unsigned long sub_count;
    unsigned long level;
    int rc;

    if (file_bytes == 0 || out_page_count == 0 || out_parent_ifd_offset == 0 ||
        out_current_ifd_offset == 0 || out_subifd_index == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (subifd_path_length > 0UL && subifd_path == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    rc = tifx_bt_walk_ifds(file_bytes,
                           size,
                           is_big_endian,
                           first_ifd_offset,
                           page_index,
                           &current_ifd_offset,
                           &page_count);
    if (rc != TIFX_OK) {
        return rc;
    }

    *out_page_count = page_count;
    *out_parent_ifd_offset = 0UL;
    *out_subifd_index = 0UL;

    for (level = 0UL; level < subifd_path_length; ++level) {
        unsigned long child_index;

        rc = tifx_bt_read_subifd_offsets(file_bytes,
                                         size,
                                         is_big_endian,
                                         current_ifd_offset,
                                         sub_offsets,
                                         TIFX_MAX_SUBIFDS,
                                         &sub_count);
        if (rc != TIFX_OK) {
            return rc;
        }
        child_index = subifd_path[level];
        if (child_index >= sub_count) {
            return TIFX_ERR_BAD_ARGUMENT;
        }

        *out_parent_ifd_offset = current_ifd_offset;
        current_ifd_offset = sub_offsets[child_index];
        *out_subifd_index = child_index;
    }

    *out_current_ifd_offset = current_ifd_offset;
    return TIFX_OK;
}

static int tifx_bt_parse_ifd(tifx_image_info *info,
                             const void *data,
                             unsigned long size,
                             int is_big_endian,
                             unsigned long first_ifd_offset,
                             unsigned long parent_ifd_offset,
                             unsigned long ifd_offset,
                             unsigned long page_index,
                             unsigned long page_count,
                             unsigned long subifd_depth,
                             unsigned long subifd_index)
{
    const unsigned char *file_bytes;
    unsigned long entry_table_bytes;
    unsigned long i;
    unsigned long tag_count;
    unsigned long offsets_count;
    unsigned long byte_counts_count;
    unsigned long tile_offsets_count;
    unsigned long tile_byte_counts_count;
    unsigned long strip_rows;
    unsigned long base_strips;
    unsigned long expected_strip_count;
    unsigned long next_ifd_offset;
    unsigned short bits_values[TIFX_MAX_SAMPLES];
    unsigned short sample_format_values[TIFX_MAX_SAMPLES];
    unsigned short extra_samples_values[TIFX_MAX_SAMPLES];
    unsigned short bits_count;
    unsigned short sample_format_count;
    unsigned short extra_samples_count;
    unsigned short have_width;
    unsigned short have_height;
    unsigned short have_photometric;
    unsigned short have_offsets;
    unsigned short have_byte_counts;
    unsigned short have_color_map;
    unsigned short have_sample_format;
    unsigned short have_extra_samples;
    unsigned short have_tile_width;
    unsigned short have_tile_length;
    unsigned short have_tile_offsets;
    unsigned short have_tile_byte_counts;
    int rc;

    if (info == 0 || data == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    tifx_image_info_init(info);
    file_bytes = (const unsigned char *)data;
    info->file_size = size;
    info->container_format = TIFX_CONTAINER_BIGTIFF;
    info->is_big_endian = (unsigned short)(is_big_endian != 0);
    info->first_ifd_offset = first_ifd_offset;
    info->current_ifd_offset = ifd_offset;
    info->parent_ifd_offset = parent_ifd_offset;
    info->page_index = page_index;
    info->page_count = page_count;
    info->subifd_depth = subifd_depth;
    info->subifd_index = subifd_index;

    rc = tifx_bt_read_ifd_trailer(file_bytes,
                                  size,
                                  is_big_endian,
                                  ifd_offset,
                                  &tag_count,
                                  &next_ifd_offset);
    if (rc != TIFX_OK) {
        return rc;
    }
    info->next_ifd_offset = next_ifd_offset;

    if (!tifx_bt_mul_ul(tag_count, 20UL, &entry_table_bytes)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (!tifx_bt_add_ul(entry_table_bytes, 16UL, &entry_table_bytes)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (entry_table_bytes > size - ifd_offset) {
        return TIFX_ERR_TRUNCATED;
    }

    bits_count = 0U;
    sample_format_count = 0U;
    extra_samples_count = 0U;
    have_width = 0U;
    have_height = 0U;
    have_photometric = 0U;
    have_offsets = 0U;
    have_byte_counts = 0U;
    have_color_map = 0U;
    have_sample_format = 0U;
    have_extra_samples = 0U;
    have_tile_width = 0U;
    have_tile_length = 0U;
    have_tile_offsets = 0U;
    have_tile_byte_counts = 0U;
    offsets_count = 0UL;
    byte_counts_count = 0UL;
    tile_offsets_count = 0UL;
    tile_byte_counts_count = 0UL;

    for (i = 0UL; i < tag_count; ++i) {
        const unsigned char *entry;
        unsigned short tag;
        unsigned short type;
        unsigned long count;
        unsigned long scalar;

        entry = file_bytes + ifd_offset + 8UL + (i * 20UL);
        tag = tifx_bt_read_u16(entry, is_big_endian);
        type = tifx_bt_read_u16(entry + 2, is_big_endian);
        count = tifx_bt_read_u64(entry + 4, is_big_endian);

        switch (tag) {
            case TIFX_TAG_NEW_SUBFILE_TYPE:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->new_subfile_type = scalar;
                break;

            case TIFX_TAG_IMAGE_WIDTH:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->width = scalar;
                have_width = 1U;
                break;

            case TIFX_TAG_IMAGE_LENGTH:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->height = scalar;
                have_height = 1U;
                break;

            case TIFX_TAG_BITS_PER_SAMPLE:
                if (type != TIFX_TYPE_SHORT) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                rc = tifx_bt_entry_short_array(file_bytes, size, is_big_endian,
                                               entry, count, bits_values,
                                               (unsigned long)TIFX_MAX_SAMPLES);
                if (rc != TIFX_OK) return rc;
                bits_count = (unsigned short)count;
                break;

            case TIFX_TAG_COMPRESSION:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->compression = (unsigned short)scalar;
                break;

            case TIFX_TAG_PHOTOMETRIC:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->photometric = (unsigned short)scalar;
                have_photometric = 1U;
                break;

            case TIFX_TAG_FILL_ORDER:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->fill_order = (unsigned short)scalar;
                break;

            case TIFX_TAG_STRIP_OFFSETS:
                rc = tifx_bt_entry_u32_array(file_bytes, size, is_big_endian,
                                             entry, type, count,
                                             info->strip_offsets, TIFX_MAX_STRIPS);
                if (rc != TIFX_OK) return rc;
                offsets_count = count;
                have_offsets = 1U;
                break;

            case TIFX_TAG_ORIENTATION:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->orientation = (unsigned short)scalar;
                break;

            case TIFX_TAG_SAMPLES_PER_PIXEL:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->samples_per_pixel = (unsigned short)scalar;
                break;

            case TIFX_TAG_ROWS_PER_STRIP:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->rows_per_strip = scalar;
                break;

            case TIFX_TAG_STRIP_BYTE_COUNTS:
                rc = tifx_bt_entry_u32_array(file_bytes, size, is_big_endian,
                                             entry, type, count,
                                             info->strip_byte_counts, TIFX_MAX_STRIPS);
                if (rc != TIFX_OK) return rc;
                byte_counts_count = count;
                have_byte_counts = 1U;
                break;

            case TIFX_TAG_X_RESOLUTION:
                rc = tifx_bt_entry_rational_fixed(file_bytes, size, is_big_endian,
                                                  entry, &info->x_resolution);
                if (rc != TIFX_OK) return rc;
                break;

            case TIFX_TAG_Y_RESOLUTION:
                rc = tifx_bt_entry_rational_fixed(file_bytes, size, is_big_endian,
                                                  entry, &info->y_resolution);
                if (rc != TIFX_OK) return rc;
                break;

            case TIFX_TAG_T4_OPTIONS:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->t4_options = scalar;
                break;

            case TIFX_TAG_T6_OPTIONS:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->t6_options = scalar;
                break;

            case TIFX_TAG_PLANAR_CONFIGURATION:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->planar_config = (unsigned short)scalar;
                break;

            case TIFX_TAG_RESOLUTION_UNIT:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->resolution_unit = (unsigned short)scalar;
                break;

            case TIFX_TAG_PREDICTOR:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                           entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->predictor = (unsigned short)scalar;
                break;

            case TIFX_TAG_PAGE_NUMBER:
                if (type != TIFX_TYPE_SHORT || count != 2UL) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                rc = tifx_bt_entry_short_array(file_bytes, size, is_big_endian,
                                               entry, 2UL, info->page_number, 2UL);
                if (rc != TIFX_OK) return rc;
                break;

            case TIFX_TAG_COLOR_MAP:
                if (type != TIFX_TYPE_SHORT) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                rc = tifx_bt_entry_short_array(file_bytes, size, is_big_endian,
                                               entry, count, info->color_map,
                                               (unsigned long)TIFX_MAX_COLORMAP_SHORTS);
                if (rc != TIFX_OK) return rc;
                info->color_map_count = (unsigned short)count;
                have_color_map = 1U;
                break;

            case TIFX_TAG_TILE_WIDTH:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->tile_width = scalar;
                have_tile_width = 1U;
                break;

            case TIFX_TAG_TILE_LENGTH:
                rc = tifx_bt_entry_scalar_u32(file_bytes, size, is_big_endian,
                                              entry, type, count, &scalar);
                if (rc != TIFX_OK) return rc;
                info->tile_length = scalar;
                have_tile_length = 1U;
                break;

            case TIFX_TAG_TILE_OFFSETS:
                rc = tifx_bt_entry_u32_array(file_bytes, size, is_big_endian,
                                             entry, type, count,
                                             info->tile_offsets, TIFX_MAX_TILES);
                if (rc != TIFX_OK) return rc;
                tile_offsets_count = count;
                have_tile_offsets = 1U;
                break;

            case TIFX_TAG_TILE_BYTE_COUNTS:
                rc = tifx_bt_entry_u32_array(file_bytes, size, is_big_endian,
                                             entry, type, count,
                                             info->tile_byte_counts, TIFX_MAX_TILES);
                if (rc != TIFX_OK) return rc;
                tile_byte_counts_count = count;
                have_tile_byte_counts = 1U;
                break;

            case TIFX_TAG_SUBIFDS:
                rc = tifx_bt_read_subifd_offsets(file_bytes,
                                                 size,
                                                 is_big_endian,
                                                 ifd_offset,
                                                 info->subifd_offsets,
                                                 TIFX_MAX_SUBIFDS,
                                                 &info->subifd_count);
                if (rc != TIFX_OK) return rc;
                break;

            case TIFX_TAG_SAMPLE_FORMAT:
                if (type != TIFX_TYPE_SHORT) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                rc = tifx_bt_entry_short_array(file_bytes, size, is_big_endian,
                                               entry, count, sample_format_values,
                                               (unsigned long)TIFX_MAX_SAMPLES);
                if (rc != TIFX_OK) return rc;
                sample_format_count = (unsigned short)count;
                have_sample_format = 1U;
                break;

            case TIFX_TAG_EXTRA_SAMPLES:
                if (type != TIFX_TYPE_SHORT) {
                    return TIFX_ERR_UNSUPPORTED;
                }
                rc = tifx_bt_entry_short_array(file_bytes, size, is_big_endian,
                                               entry, count, extra_samples_values,
                                               (unsigned long)TIFX_MAX_SAMPLES);
                if (rc != TIFX_OK) return rc;
                extra_samples_count = (unsigned short)count;
                have_extra_samples = 1U;
                break;

            default:
                break;
        }
    }

    if (!have_width || !have_height || !have_photometric) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (info->width == 0UL || info->height == 0UL) {
        return TIFX_ERR_BAD_FORMAT;
    }
    if (info->samples_per_pixel == 0U || info->samples_per_pixel > TIFX_MAX_SAMPLES) {
        return TIFX_ERR_UNSUPPORTED;
    }

    if (bits_count == 0U) {
        if ((info->photometric == 0U || info->photometric == 1U) &&
            info->samples_per_pixel == 1U) {
            bits_values[0] = 1U;
            bits_count = 1U;
        } else {
            return TIFX_ERR_BAD_FORMAT;
        }
    }

    if (bits_count == 1U && info->samples_per_pixel > 1U) {
        unsigned short j;
        for (j = 1U; j < info->samples_per_pixel; ++j) {
            bits_values[j] = bits_values[0];
        }
        bits_count = info->samples_per_pixel;
    }

    rc = tifx_bt_copy_bits_defaults(info, bits_values, bits_count);
    if (rc != TIFX_OK) {
        return rc;
    }

    if (have_extra_samples) {
        unsigned short j;
        info->extra_samples_count = extra_samples_count;
        for (j = 0U; j < extra_samples_count; ++j) {
            info->extra_samples[j] = extra_samples_values[j];
        }
        if (extra_samples_count > 0U &&
            (extra_samples_values[0] == TIFX_ALPHA_ASSOCIATED ||
             extra_samples_values[0] == TIFX_ALPHA_UNASSOCIATED)) {
            info->alpha_mode = extra_samples_values[0];
        }
    }

    if (info->photometric == 3U && !have_color_map) {
        return TIFX_ERR_BAD_FORMAT;
    }

    if (have_offsets || have_byte_counts) {
        if (!have_offsets || !have_byte_counts) {
            return TIFX_ERR_BAD_FORMAT;
        }
        if (have_tile_width || have_tile_length || have_tile_offsets || have_tile_byte_counts) {
            return TIFX_ERR_BAD_FORMAT;
        }
        if (offsets_count == 0UL || offsets_count != byte_counts_count) {
            return TIFX_ERR_BAD_FORMAT;
        }
        if (info->rows_per_strip == 0UL || info->rows_per_strip > info->height) {
            info->rows_per_strip = info->height;
        }
        strip_rows = info->rows_per_strip;
        base_strips = tifx_bt_ceil_div(info->height, strip_rows);
        expected_strip_count = base_strips;
        if (info->planar_config == 2U) {
            if (!tifx_bt_mul_ul(expected_strip_count,
                                (unsigned long)info->samples_per_pixel,
                                &expected_strip_count)) {
                return TIFX_ERR_OVERFLOW;
            }
        }
        if (offsets_count != expected_strip_count) {
            return TIFX_ERR_BAD_FORMAT;
        }
        info->strip_count = offsets_count;
        info->storage_layout = TIFX_LAYOUT_STRIPS;
    } else {
        unsigned long expected_tile_count;
        int tile_rc;

        if (!have_tile_width || !have_tile_length || !have_tile_offsets || !have_tile_byte_counts) {
            return TIFX_ERR_BAD_FORMAT;
        }
        if (info->tile_width == 0UL || info->tile_length == 0UL) {
            return TIFX_ERR_BAD_FORMAT;
        }
        if (tile_offsets_count == 0UL || tile_offsets_count != tile_byte_counts_count) {
            return TIFX_ERR_BAD_FORMAT;
        }
        tile_rc = tifx__compute_tile_count(info->width,
                                           info->height,
                                           info->tile_width,
                                           info->tile_length,
                                           &expected_tile_count);
        if (tile_rc != TIFX_OK) {
            return tile_rc;
        }
        if (info->planar_config == 2U) {
            if (!tifx_bt_mul_ul(expected_tile_count,
                                (unsigned long)info->samples_per_pixel,
                                &expected_tile_count)) {
                return TIFX_ERR_OVERFLOW;
            }
        }
        if (tile_offsets_count != expected_tile_count) {
            return TIFX_ERR_BAD_FORMAT;
        }
        info->tile_count = tile_offsets_count;
        info->storage_layout = TIFX_LAYOUT_TILES;
    }

    rc = tifx_bt_validate_layout(info,
                                 have_sample_format,
                                 sample_format_values,
                                 sample_format_count);
    if (rc != TIFX_OK) {
        return rc;
    }

    return TIFX_OK;
}

int tifx_bigtiff_page_count_memory(const void *data,
                                   unsigned long size,
                                   unsigned long *out_page_count)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)data;
    (void)size;
    if (out_page_count != 0) {
        *out_page_count = 0UL;
    }
    return TIFX_ERR_UNSUPPORTED;
#else
    const unsigned char *file_bytes;
    int is_big_endian;
    unsigned long first_ifd_offset;
    int rc;

    if (data == 0 || out_page_count == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    file_bytes = (const unsigned char *)data;
    rc = tifx_bt_read_header(file_bytes, size, &is_big_endian, &first_ifd_offset);
    if (rc != TIFX_OK) {
        return rc;
    }

    return tifx_bt_walk_ifds(file_bytes,
                             size,
                             is_big_endian,
                             first_ifd_offset,
                             TIFX_MAX_PAGES,
                             0,
                             out_page_count);
#endif
}

int tifx_bigtiff_subifd_count_memory(const void *data,
                                     unsigned long size,
                                     unsigned long page_index,
                                     const unsigned long *subifd_path,
                                     unsigned long subifd_path_length,
                                     unsigned long *out_subifd_count)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)data;
    (void)size;
    (void)page_index;
    (void)subifd_path;
    (void)subifd_path_length;
    if (out_subifd_count != 0) {
        *out_subifd_count = 0UL;
    }
    return TIFX_ERR_UNSUPPORTED;
#else
    const unsigned char *file_bytes;
    int is_big_endian;
    unsigned long first_ifd_offset;
    unsigned long page_count;
    unsigned long parent_ifd_offset;
    unsigned long current_ifd_offset;
    unsigned long subifd_index;
    unsigned long subifd_offsets[TIFX_MAX_SUBIFDS];
    int rc;

    if (data == 0 || out_subifd_count == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (subifd_path_length > TIFX_MAX_SUBIFD_DEPTH) {
        return TIFX_ERR_UNSUPPORTED;
    }

    file_bytes = (const unsigned char *)data;
    rc = tifx_bt_read_header(file_bytes, size, &is_big_endian, &first_ifd_offset);
    if (rc != TIFX_OK) {
        return rc;
    }

    rc = tifx_bt_walk_subifd_path(file_bytes,
                                  size,
                                  is_big_endian,
                                  first_ifd_offset,
                                  page_index,
                                  subifd_path,
                                  subifd_path_length,
                                  &page_count,
                                  &parent_ifd_offset,
                                  &current_ifd_offset,
                                  &subifd_index);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)page_count;
    (void)parent_ifd_offset;
    (void)subifd_index;

    return tifx_bt_read_subifd_offsets(file_bytes,
                                       size,
                                       is_big_endian,
                                       current_ifd_offset,
                                       subifd_offsets,
                                       TIFX_MAX_SUBIFDS,
                                       out_subifd_count);
#endif
}

int tifx_parse_bigtiff_node_memory(tifx_image_info *info,
                                   const void *data,
                                   unsigned long size,
                                   unsigned long page_index,
                                   const unsigned long *subifd_path,
                                   unsigned long subifd_path_length)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)info;
    (void)data;
    (void)size;
    (void)page_index;
    (void)subifd_path;
    (void)subifd_path_length;
    return TIFX_ERR_UNSUPPORTED;
#else
    const unsigned char *file_bytes;
    int is_big_endian;
    unsigned long first_ifd_offset;
    unsigned long page_count;
    unsigned long parent_ifd_offset;
    unsigned long current_ifd_offset;
    unsigned long subifd_index;
    int rc;

    if (info == 0 || data == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (subifd_path_length > TIFX_MAX_SUBIFD_DEPTH) {
        return TIFX_ERR_UNSUPPORTED;
    }

    file_bytes = (const unsigned char *)data;
    rc = tifx_bt_read_header(file_bytes, size, &is_big_endian, &first_ifd_offset);
    if (rc != TIFX_OK) {
        return rc;
    }

    rc = tifx_bt_walk_subifd_path(file_bytes,
                                  size,
                                  is_big_endian,
                                  first_ifd_offset,
                                  page_index,
                                  subifd_path,
                                  subifd_path_length,
                                  &page_count,
                                  &parent_ifd_offset,
                                  &current_ifd_offset,
                                  &subifd_index);
    if (rc != TIFX_OK) {
        return rc;
    }

    return tifx_bt_parse_ifd(info,
                             data,
                             size,
                             is_big_endian,
                             first_ifd_offset,
                             parent_ifd_offset,
                             current_ifd_offset,
                             page_index,
                             page_count,
                             subifd_path_length,
                             subifd_index);
#endif
}

int tifx_parse_bigtiff_page_memory(tifx_image_info *info,
                                   const void *data,
                                   unsigned long size,
                                   unsigned long page_index)
{
    return tifx_parse_bigtiff_node_memory(info, data, size, page_index, 0, 0UL);
}

int tifx_parse_bigtiff_memory(tifx_image_info *info, const void *data, unsigned long size)
{
    return tifx_parse_bigtiff_page_memory(info, data, size, 0UL);
}
