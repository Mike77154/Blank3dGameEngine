/* SPDX-License-Identifier: CC0-1.0 */
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
#define TIFX_TAG_SUBIFDS 330U
#define TIFX_TAG_EXTRA_SAMPLES 338U

static void tifx_bt_write_u16le(unsigned char *p, unsigned short value)
{
    p[0] = (unsigned char)(value & 0xFFU);
    p[1] = (unsigned char)((value >> 8) & 0xFFU);
}

static void tifx_bt_write_u32le(unsigned char *p, unsigned long value)
{
    p[0] = (unsigned char)(value & 0xFFUL);
    p[1] = (unsigned char)((value >> 8) & 0xFFUL);
    p[2] = (unsigned char)((value >> 16) & 0xFFUL);
    p[3] = (unsigned char)((value >> 24) & 0xFFUL);
}

#if ULONG_MAX > 0xFFFFFFFFUL
static void tifx_bt_write_u64le(unsigned char *p, unsigned long value)
{
    p[0] = (unsigned char)(value & 0xFFUL);
    p[1] = (unsigned char)((value >> 8) & 0xFFUL);
    p[2] = (unsigned char)((value >> 16) & 0xFFUL);
    p[3] = (unsigned char)((value >> 24) & 0xFFUL);
    p[4] = (unsigned char)((value >> 32) & 0xFFUL);
    p[5] = (unsigned char)((value >> 40) & 0xFFUL);
    p[6] = (unsigned char)((value >> 48) & 0xFFUL);
    p[7] = (unsigned char)((value >> 56) & 0xFFUL);
}
#else
static void tifx_bt_write_u64le(unsigned char *p, unsigned long value)
{
    (void)p;
    (void)value;
}
#endif

static void tifx_bt_write_u64le_array(unsigned char *dst,
                                      const unsigned long *values,
                                      unsigned long count)
{
    unsigned long i;

    if (dst == 0 || values == 0) {
        return;
    }
    for (i = 0UL; i < count; ++i) {
        tifx_bt_write_u64le(dst + (i * 8UL), values[i]);
    }
}

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

static unsigned long tifx_bt_align8(unsigned long value)
{
    return (value + 7UL) & ~7UL;
}

static unsigned long tifx_bt_ceil_div(unsigned long value, unsigned long divisor)
{
    if (value == 0UL) {
        return 0UL;
    }
    return ((value - 1UL) / divisor) + 1UL;
}

static unsigned long tifx_bt_effective_rows_per_strip(const tifx_write_params *params)
{
    if (params == 0) {
        return 0UL;
    }
    if (params->rows_per_strip == 0UL || params->rows_per_strip > params->height) {
        return params->height;
    }
    return params->rows_per_strip;
}

static unsigned short tifx_bt_effective_planar_config(const tifx_write_params *params)
{
    if (params == 0 || params->planar_config == 0U) {
        return 1U;
    }
    return params->planar_config;
}

static unsigned long tifx_bt_write_sample_count(const tifx_write_params *params)
{
    if (params == 0) {
        return 1UL;
    }
    if (params->pixel_format == TIFX_PIXEL_RGB24) {
        return 3UL;
    }
    if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        return 4UL;
    }
    return 1UL;
}

static unsigned short tifx_bt_effective_fill_order(const tifx_write_params *params)
{
    if (params == 0 || params->fill_order == 0U) {
        return 1U;
    }
    return params->fill_order;
}

static unsigned short tifx_bt_effective_predictor(const tifx_write_params *params)
{
    if (params == 0 || params->predictor == 0U) {
        return TIFX_PREDICTOR_NONE;
    }
    return params->predictor;
}

static int tifx_bt_should_write_fill_order_tag(const tifx_write_params *params,
                                               unsigned short compression,
                                               unsigned short fill_order)
{
    if (params == 0 || params->pixel_format != TIFX_PIXEL_BILEVEL) {
        return 0;
    }
    if (fill_order != 1U) {
        return 1;
    }
    return (compression == 2U || compression == 3U || compression == 4U) ? 1 : 0;
}

static int tifx_bt_compute_strip_count(unsigned long height,
                                       unsigned long rows_per_strip,
                                       unsigned long *out_strip_count)
{
    unsigned long strip_count;

    if (rows_per_strip == 0UL || out_strip_count == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    strip_count = tifx_bt_ceil_div(height, rows_per_strip);
    if (strip_count == 0UL || strip_count > TIFX_MAX_STRIPS) {
        return TIFX_ERR_UNSUPPORTED;
    }
    *out_strip_count = strip_count;
    return TIFX_OK;
}

static int tifx_bt_validate_write_params(const tifx_write_params *params,
                                         unsigned long *out_stride_min,
                                         unsigned long *out_rows_per_strip,
                                         unsigned long *out_strip_count,
                                         unsigned long *out_image_size,
                                         unsigned short *out_tag_count,
                                         unsigned long *out_extra_size)
{
    unsigned long stride_min;
    unsigned long rows_per_strip;
    unsigned long strip_count;
    unsigned long image_size;
    unsigned long bytes_per_pixel;
    unsigned short tag_count;
    unsigned long extra_size;
    unsigned short compression;
    unsigned short fill_order;
    unsigned short planar_config;
    unsigned short predictor;
    unsigned long sample_count;
    int rc;

    if (!tifx_bigtiff_available()) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (params == 0 || params->pixels == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (params->width == 0UL || params->height == 0UL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }
    fill_order = tifx_bt_effective_fill_order(params);
    planar_config = tifx_bt_effective_planar_config(params);
    predictor = tifx_bt_effective_predictor(params);
    sample_count = tifx_bt_write_sample_count(params);
    if (fill_order != 1U && fill_order != 2U) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (predictor != TIFX_PREDICTOR_NONE && predictor != TIFX_PREDICTOR_HORIZONTAL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (params->deflate_mode != TIFX_DEFLATE_AUTO &&
        params->deflate_mode != TIFX_DEFLATE_STORED &&
        params->deflate_mode != TIFX_DEFLATE_FIXED &&
        params->deflate_mode != TIFX_DEFLATE_DYNAMIC) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    rows_per_strip = tifx_bt_effective_rows_per_strip(params);
    if (rows_per_strip > 0xFFFFFFFFUL || params->width > 0xFFFFFFFFUL || params->height > 0xFFFFFFFFUL) {
        return TIFX_ERR_UNSUPPORTED;
    }
    rc = tifx_bt_compute_strip_count(params->height, rows_per_strip, &strip_count);
    if (rc != TIFX_OK) {
        return rc;
    }
    if (planar_config == 2U && sample_count > 1UL) {
        unsigned long total_segments;

        if (!tifx_bt_mul_ul(strip_count, sample_count, &total_segments)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (total_segments == 0UL || total_segments > TIFX_MAX_STRIPS) {
            return TIFX_ERR_UNSUPPORTED;
        }
        strip_count = total_segments;
    }

    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        if (compression != 1U && compression != 5U && compression != 8U && compression != 32946U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (fill_order != 1U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (planar_config != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (predictor == TIFX_PREDICTOR_HORIZONTAL &&
            (compression != 5U && compression != 8U && compression != 32946U)) {
            return TIFX_ERR_UNSUPPORTED;
        }
        bytes_per_pixel = 1UL;
        tag_count = 12U;
        extra_size = 0UL;
        if (!tifx_bt_mul_ul(params->width, bytes_per_pixel, &stride_min)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (planar_config == 2U) {
            stride_min = params->width;
        }
        if (!tifx_bt_mul_ul(stride_min, params->height, &image_size)) {
            return TIFX_ERR_OVERFLOW;
        }
    } else if (params->pixel_format == TIFX_PIXEL_RGB24) {
        if (compression != 1U && compression != 5U && compression != 8U && compression != 32946U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (fill_order != 1U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (planar_config != 1U && planar_config != 2U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (predictor == TIFX_PREDICTOR_HORIZONTAL &&
            (compression != 5U && compression != 8U && compression != 32946U)) {
            return TIFX_ERR_UNSUPPORTED;
        }
        bytes_per_pixel = 3UL;
        tag_count = 14U;
        extra_size = 0UL;
        if (!tifx_bt_mul_ul(params->width, bytes_per_pixel, &stride_min)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (planar_config == 2U) {
            stride_min = params->width;
        }
        if (!tifx_bt_mul_ul(stride_min, params->height, &image_size)) {
            return TIFX_ERR_OVERFLOW;
        }
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        if (compression != 1U && compression != 5U && compression != 8U && compression != 32946U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (fill_order != 1U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (planar_config != 1U && planar_config != 2U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (predictor == TIFX_PREDICTOR_HORIZONTAL &&
            (compression != 5U && compression != 8U && compression != 32946U)) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (params->alpha_mode != TIFX_ALPHA_ASSOCIATED &&
            params->alpha_mode != TIFX_ALPHA_UNASSOCIATED) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        bytes_per_pixel = 4UL;
        tag_count = 15U;
        extra_size = 0UL;
        if (!tifx_bt_mul_ul(params->width, bytes_per_pixel, &stride_min)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (planar_config == 2U) {
            stride_min = params->width;
        }
        if (!tifx_bt_mul_ul(stride_min, params->height, &image_size)) {
            return TIFX_ERR_OVERFLOW;
        }
    } else if (params->pixel_format == TIFX_PIXEL_BILEVEL) {
        unsigned long strip_byte_counts[TIFX_MAX_STRIPS];

        if (planar_config != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        stride_min = tifx_bt_ceil_div(params->width, 8UL);
        extra_size = 0UL;
        if (params->photometric != 0U && params->photometric != 1U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (predictor != TIFX_PREDICTOR_NONE) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (compression == 1U || compression == 2U || compression == 5U || compression == 8U || compression == 32946U) {
            if ((compression == 5U || compression == 8U || compression == 32946U) && fill_order != 1U) {
                return TIFX_ERR_UNSUPPORTED;
            }
            tag_count = 12U;
        } else if (compression == 3U) {
            if ((params->t4_options & ~5UL) != 0UL) {
                return TIFX_ERR_UNSUPPORTED;
            }
            tag_count = 13U;
        } else if (compression == 4U) {
            if (params->t6_options != 0UL) {
                return TIFX_ERR_UNSUPPORTED;
            }
            tag_count = 13U;
        } else {
            return TIFX_ERR_UNSUPPORTED;
        }

        rc = tifx__build_strip_byte_counts(params,
                                           stride_min,
                                           rows_per_strip,
                                           strip_count,
                                           strip_byte_counts,
                                           &image_size);
        if (rc != TIFX_OK) {
            return rc;
        }
    } else {
        return TIFX_ERR_UNSUPPORTED;
    }

    if (tifx_bt_should_write_fill_order_tag(params, compression, fill_order)) {
        ++tag_count;
    }
    if (predictor == TIFX_PREDICTOR_HORIZONTAL) {
        ++tag_count;
    }
    if (params->stride < stride_min) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (strip_count > 1UL) {
        unsigned long arrays_size;
        if (!tifx_bt_mul_ul(strip_count, 16UL, &arrays_size)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (!tifx_bt_add_ul(extra_size, arrays_size, &extra_size)) {
            return TIFX_ERR_OVERFLOW;
        }
    }

    *out_stride_min = stride_min;
    *out_rows_per_strip = rows_per_strip;
    *out_strip_count = strip_count;
    *out_image_size = image_size;
    *out_tag_count = tag_count;
    *out_extra_size = extra_size;
    return TIFX_OK;
}

static void tifx_bt_write_tag_scalar(unsigned char *entry,
                                     unsigned short tag,
                                     unsigned short type,
                                     unsigned long value)
{
    memset(entry, 0, 20U);
    tifx_bt_write_u16le(entry + 0, tag);
    tifx_bt_write_u16le(entry + 2, type);
    tifx_bt_write_u64le(entry + 4, 1UL);
    if (type == TIFX_TYPE_SHORT) {
        tifx_bt_write_u16le(entry + 12, (unsigned short)value);
    } else if (type == TIFX_TYPE_LONG) {
        tifx_bt_write_u32le(entry + 12, value);
    } else {
        tifx_bt_write_u64le(entry + 12, value);
    }
}

static void tifx_bt_write_tag_offset(unsigned char *entry,
                                     unsigned short tag,
                                     unsigned short type,
                                     unsigned long count,
                                     unsigned long offset)
{
    memset(entry, 0, 20U);
    tifx_bt_write_u16le(entry + 0, tag);
    tifx_bt_write_u16le(entry + 2, type);
    tifx_bt_write_u64le(entry + 4, count);
    tifx_bt_write_u64le(entry + 12, offset);
}

static void tifx_bt_write_tag_inline_shorts(unsigned char *entry,
                                            unsigned short tag,
                                            const unsigned short *values,
                                            unsigned long count)
{
    unsigned long i;

    memset(entry, 0, 20U);
    tifx_bt_write_u16le(entry + 0, tag);
    tifx_bt_write_u16le(entry + 2, TIFX_TYPE_SHORT);
    tifx_bt_write_u64le(entry + 4, count);
    for (i = 0UL; i < count && i < 4UL; ++i) {
        tifx_bt_write_u16le(entry + 12 + (i * 2UL), values[i]);
    }
}

static void tifx_bt_write_tag_inline_rational(unsigned char *entry,
                                              unsigned short tag,
                                              unsigned long numerator,
                                              unsigned long denominator)
{
    memset(entry, 0, 20U);
    tifx_bt_write_u16le(entry + 0, tag);
    tifx_bt_write_u16le(entry + 2, TIFX_TYPE_RATIONAL);
    tifx_bt_write_u64le(entry + 4, 1UL);
    tifx_bt_write_u32le(entry + 12, numerator);
    tifx_bt_write_u32le(entry + 16, denominator);
}

unsigned long tifx_write_bigtiff_buffer_size(const tifx_write_params *params)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)params;
    return 0UL;
#else
    unsigned long stride_min;
    unsigned long rows_per_strip;
    unsigned long strip_count;
    unsigned long image_size;
    unsigned short tag_count;
    unsigned long extra_size;
    unsigned long cursor;
    unsigned long ifd_size;
    int rc;
    unsigned long strip_byte_counts[TIFX_MAX_STRIPS];
    unsigned short compression;
    unsigned long strip_index;

    rc = tifx_bt_validate_write_params(params,
                                       &stride_min,
                                       &rows_per_strip,
                                       &strip_count,
                                       &image_size,
                                       &tag_count,
                                       &extra_size);
    if (rc != TIFX_OK) {
        return 0UL;
    }
    (void)stride_min;
    (void)image_size;
    (void)extra_size;

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }

    rc = tifx__build_strip_byte_counts(params,
                                       stride_min,
                                       rows_per_strip,
                                       strip_count,
                                       strip_byte_counts,
                                       &image_size);
    if (rc != TIFX_OK) {
        return 0UL;
    }
    (void)compression;

    cursor = 16UL;
    for (strip_index = 0UL; strip_index < strip_count; ++strip_index) {
        cursor = tifx_bt_align8(cursor);
        if (!tifx_bt_add_ul(cursor, strip_byte_counts[strip_index], &cursor)) {
            return 0UL;
        }
    }

    cursor = tifx_bt_align8(cursor);
    ifd_size = 8UL + ((unsigned long)tag_count * 20UL) + 8UL;
    if (!tifx_bt_add_ul(cursor, ifd_size, &cursor)) {
        return 0UL;
    }
    cursor = tifx_bt_align8(cursor);
    if (strip_count > 1UL) {
        unsigned long arrays_size;
        if (!tifx_bt_mul_ul(strip_count, 16UL, &arrays_size)) {
            return 0UL;
        }
        if (!tifx_bt_add_ul(cursor, arrays_size, &cursor)) {
            return 0UL;
        }
    }
    return cursor;
#endif
}

int tifx_write_bigtiff_memory(void *dst,
                              unsigned long dst_size,
                              const tifx_write_params *params,
                              unsigned long *written_size)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)dst;
    (void)dst_size;
    (void)params;
    if (written_size != 0) {
        *written_size = 0UL;
    }
    return TIFX_ERR_UNSUPPORTED;
#else
    unsigned long stride_min;
    unsigned long rows_per_strip;
    unsigned long strip_count;
    unsigned long image_size;
    unsigned short tag_count;
    unsigned long extra_size;
    unsigned long strip_offsets[TIFX_MAX_STRIPS];
    unsigned long strip_byte_counts[TIFX_MAX_STRIPS];
    unsigned short compression;
    unsigned short fill_order;
    unsigned short planar_config;
    unsigned short predictor;
    unsigned short needs_fill_order_tag;
    unsigned short photometric;
    unsigned short resolution_unit;
    unsigned long total_size;
    unsigned long ifd_offset;
    unsigned long ifd_size;
    unsigned long extra_cursor;
    unsigned long strip_offsets_array_offset;
    unsigned long strip_byte_counts_array_offset;
    unsigned long strip_index;
    unsigned short entry_index;
    unsigned char *out;
    tifx_fixed xres;
    tifx_fixed yres;
    int rc;

    if (dst == 0 || params == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    rc = tifx_bt_validate_write_params(params,
                                       &stride_min,
                                       &rows_per_strip,
                                       &strip_count,
                                       &image_size,
                                       &tag_count,
                                       &extra_size);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)image_size;
    (void)extra_size;

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }
    fill_order = tifx_bt_effective_fill_order(params);
    planar_config = tifx_bt_effective_planar_config(params);
    predictor = tifx_bt_effective_predictor(params);
    needs_fill_order_tag = (unsigned short)tifx_bt_should_write_fill_order_tag(params, compression, fill_order);
    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        photometric = 1U;
    } else if (params->pixel_format == TIFX_PIXEL_RGB24 ||
               params->pixel_format == TIFX_PIXEL_RGBA32) {
        photometric = 2U;
    } else {
        photometric = params->photometric;
    }

    rc = tifx__build_strip_byte_counts(params,
                                       stride_min,
                                       rows_per_strip,
                                       strip_count,
                                       strip_byte_counts,
                                       &image_size);
    if (rc != TIFX_OK) {
        return rc;
    }

    total_size = tifx_write_bigtiff_buffer_size(params);
    if (total_size == 0UL) {
        return TIFX_ERR_OVERFLOW;
    }
    if (dst_size < total_size) {
        return TIFX_ERR_NO_SPACE;
    }

    out = (unsigned char *)dst;
    memset(out, 0, total_size);

    out[0] = 'I';
    out[1] = 'I';
    tifx_bt_write_u16le(out + 2, 43U);
    tifx_bt_write_u16le(out + 4, 8U);
    tifx_bt_write_u16le(out + 6, 0U);

    extra_cursor = 16UL;
    for (strip_index = 0UL; strip_index < strip_count; ++strip_index) {
        unsigned long start_row;
        unsigned long rows_this_strip;

        extra_cursor = tifx_bt_align8(extra_cursor);
        strip_offsets[strip_index] = extra_cursor;
        start_row = strip_index * rows_per_strip;
        rows_this_strip = rows_per_strip;
        if (rows_this_strip > params->height - start_row) {
            rows_this_strip = params->height - start_row;
        }

        rc = tifx__write_segment_payload(out + extra_cursor,
                                         strip_byte_counts[strip_index],
                                         params,
                                         stride_min,
                                         strip_index,
                                         rows_per_strip);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (!tifx_bt_add_ul(extra_cursor, strip_byte_counts[strip_index], &extra_cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
    }

    ifd_offset = tifx_bt_align8(extra_cursor);
    tifx_bt_write_u64le(out + 8, ifd_offset);
    ifd_size = 8UL + ((unsigned long)tag_count * 20UL) + 8UL;
    extra_cursor = tifx_bt_align8(ifd_offset + ifd_size);

    strip_offsets_array_offset = 0UL;
    strip_byte_counts_array_offset = 0UL;
    if (strip_count > 1UL) {
        strip_offsets_array_offset = extra_cursor;
        if (!tifx_bt_add_ul(extra_cursor, strip_count * 8UL, &extra_cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        extra_cursor = tifx_bt_align8(extra_cursor);
        strip_byte_counts_array_offset = extra_cursor;
        if (!tifx_bt_add_ul(extra_cursor, strip_count * 8UL, &extra_cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        extra_cursor = tifx_bt_align8(extra_cursor);
    }

    if (extra_cursor != total_size) {
        return TIFX_ERR_BAD_FORMAT;
    }

    tifx_bt_write_u64le(out + ifd_offset, (unsigned long)tag_count);
    entry_index = 0U;

    tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_IMAGE_WIDTH, TIFX_TYPE_LONG, params->width);
    tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_IMAGE_LENGTH, TIFX_TYPE_LONG, params->height);

    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 8UL);
    } else if (params->pixel_format == TIFX_PIXEL_RGB24) {
        static const unsigned short rgb_bits[3] = { 8U, 8U, 8U };
        tifx_bt_write_tag_inline_shorts(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                        TIFX_TAG_BITS_PER_SAMPLE,
                                        rgb_bits,
                                        3UL);
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        static const unsigned short rgba_bits[4] = { 8U, 8U, 8U, 8U };
        tifx_bt_write_tag_inline_shorts(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                        TIFX_TAG_BITS_PER_SAMPLE,
                                        rgba_bits,
                                        4UL);
    } else {
        tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 1UL);
    }

    tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_COMPRESSION, TIFX_TYPE_SHORT, (unsigned long)compression);
    tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_PHOTOMETRIC, TIFX_TYPE_SHORT, (unsigned long)photometric);

    if (needs_fill_order_tag) {
        tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_FILL_ORDER, TIFX_TYPE_SHORT, (unsigned long)fill_order);
    }

    if (strip_count == 1UL) {
        tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_STRIP_OFFSETS, TIFX_TYPE_LONG8, strip_offsets[0]);
    } else {
        tifx_bt_write_tag_offset(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_STRIP_OFFSETS, TIFX_TYPE_LONG8, strip_count,
                                 strip_offsets_array_offset);
    }

    tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_ORIENTATION, TIFX_TYPE_SHORT, 1UL);

    if (params->pixel_format == TIFX_PIXEL_RGB24 || params->pixel_format == TIFX_PIXEL_RGBA32) {
        tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_SAMPLES_PER_PIXEL,
                                 TIFX_TYPE_SHORT,
                                 (params->pixel_format == TIFX_PIXEL_RGBA32) ? 4UL : 3UL);
    }

    tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_ROWS_PER_STRIP, TIFX_TYPE_LONG, rows_per_strip);

    if (strip_count == 1UL) {
        tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_STRIP_BYTE_COUNTS, TIFX_TYPE_LONG8, strip_byte_counts[0]);
    } else {
        tifx_bt_write_tag_offset(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_STRIP_BYTE_COUNTS, TIFX_TYPE_LONG8, strip_count,
                                 strip_byte_counts_array_offset);
    }

    xres = params->x_resolution;
    yres = params->y_resolution;
    if (xres <= 0) xres = TIFX_FP_FROM_INT(72L);
    if (yres <= 0) yres = TIFX_FP_FROM_INT(72L);

    if (predictor == TIFX_PREDICTOR_HORIZONTAL) {
        tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_PREDICTOR, TIFX_TYPE_SHORT, (unsigned long)predictor);
    }
    tifx_bt_write_tag_inline_rational(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                      TIFX_TAG_X_RESOLUTION,
                                      tifx_fp_to_rational_numerator(xres),
                                      tifx_fp_to_rational_denominator());
    tifx_bt_write_tag_inline_rational(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                      TIFX_TAG_Y_RESOLUTION,
                                      tifx_fp_to_rational_numerator(yres),
                                      tifx_fp_to_rational_denominator());

    if (params->pixel_format == TIFX_PIXEL_RGB24 || params->pixel_format == TIFX_PIXEL_RGBA32) {
        tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_PLANAR_CONFIGURATION, TIFX_TYPE_SHORT, (unsigned long)planar_config);
    }
    if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_EXTRA_SAMPLES, TIFX_TYPE_SHORT, params->alpha_mode);
    }
    if (params->pixel_format == TIFX_PIXEL_BILEVEL && compression == 3U) {
        tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_T4_OPTIONS, TIFX_TYPE_LONG, params->t4_options);
    }
    if (params->pixel_format == TIFX_PIXEL_BILEVEL && compression == 4U) {
        tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_T6_OPTIONS, TIFX_TYPE_LONG, params->t6_options);
    }

    resolution_unit = params->resolution_unit;
    if (resolution_unit == 0U) {
        resolution_unit = 2U;
    }
    tifx_bt_write_tag_scalar(out + ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_RESOLUTION_UNIT, TIFX_TYPE_SHORT,
                             (unsigned long)resolution_unit);

    if (entry_index != tag_count) {
        return TIFX_ERR_BAD_FORMAT;
    }

    tifx_bt_write_u64le(out + ifd_offset + 8UL + ((unsigned long)tag_count * 20UL), 0UL);

    if (strip_count > 1UL) {
        tifx_bt_write_u64le_array(out + strip_offsets_array_offset, strip_offsets, strip_count);
        tifx_bt_write_u64le_array(out + strip_byte_counts_array_offset, strip_byte_counts, strip_count);
    }

    if (written_size != 0) {
        *written_size = total_size;
    }
    return TIFX_OK;
#endif
}

static int tifx_bt_measure_multipage_page_layout(const tifx_write_params *params,
                                                 unsigned short extra_tag_count,
                                                 unsigned long page_base_offset,
                                                 unsigned long *out_ifd_offset,
                                                 unsigned long *out_page_end)
{
    unsigned long stride_min;
    unsigned long rows_per_strip;
    unsigned long strip_count;
    unsigned long image_size;
    unsigned short tag_count;
    unsigned long extra_size;
    unsigned long strip_byte_counts[TIFX_MAX_STRIPS];
    unsigned long cursor;
    unsigned long ifd_offset;
    unsigned long ifd_size;
    unsigned long arrays_size;
    int rc;

    if (params == 0 || out_ifd_offset == 0 || out_page_end == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    rc = tifx_bt_validate_write_params(params,
                                       &stride_min,
                                       &rows_per_strip,
                                       &strip_count,
                                       &image_size,
                                       &tag_count,
                                       &extra_size);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)image_size;
    (void)extra_size;

    if ((unsigned long)tag_count + (unsigned long)extra_tag_count > 65535UL) {
        return TIFX_ERR_OVERFLOW;
    }
    tag_count = (unsigned short)(tag_count + extra_tag_count);

    rc = tifx__build_strip_byte_counts(params,
                                       stride_min,
                                       rows_per_strip,
                                       strip_count,
                                       strip_byte_counts,
                                       &image_size);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)image_size;

    cursor = page_base_offset;
    {
        unsigned long strip_index;
        for (strip_index = 0UL; strip_index < strip_count; ++strip_index) {
            cursor = tifx_bt_align8(cursor);
            if (!tifx_bt_add_ul(cursor, strip_byte_counts[strip_index], &cursor)) {
                return TIFX_ERR_OVERFLOW;
            }
        }
    }

    ifd_offset = tifx_bt_align8(cursor);
    if (!tifx_bt_mul_ul((unsigned long)tag_count, 20UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (!tifx_bt_add_ul(ifd_size, 16UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    cursor = tifx_bt_align8(ifd_offset + ifd_size);

    if (strip_count > 1UL) {
        if (!tifx_bt_mul_ul(strip_count, 8UL, &arrays_size)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (!tifx_bt_add_ul(cursor, arrays_size, &cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        cursor = tifx_bt_align8(cursor);
        if (!tifx_bt_add_ul(cursor, arrays_size, &cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        cursor = tifx_bt_align8(cursor);
    }

    *out_ifd_offset = ifd_offset;
    *out_page_end = cursor;
    return TIFX_OK;
}

static int tifx_bt_write_multipage_page(unsigned char *out,
                                        const tifx_write_params *params,
                                        unsigned long page_base_offset,
                                        unsigned long current_ifd_offset,
                                        unsigned long next_ifd_offset,
                                        unsigned long page_index,
                                        unsigned long page_count,
                                        unsigned long *out_page_end)
{
    unsigned long stride_min;
    unsigned long rows_per_strip;
    unsigned long strip_count;
    unsigned long image_size;
    unsigned short base_tag_count;
    unsigned long extra_size;
    unsigned long strip_offsets[TIFX_MAX_STRIPS];
    unsigned long strip_byte_counts[TIFX_MAX_STRIPS];
    unsigned short compression;
    unsigned short fill_order;
    unsigned short planar_config;
    unsigned short predictor;
    unsigned short needs_fill_order_tag;
    unsigned short photometric;
    unsigned short resolution_unit;
    unsigned short tag_count;
    unsigned short entry_index;
    unsigned long cursor;
    unsigned long ifd_size;
    unsigned long extra_cursor;
    unsigned long strip_offsets_array_offset;
    unsigned long strip_byte_counts_array_offset;
    unsigned long strip_index;
    unsigned short page_number_values[2];
    tifx_fixed xres;
    tifx_fixed yres;
    int rc;

    if (out == 0 || params == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    rc = tifx_bt_validate_write_params(params,
                                       &stride_min,
                                       &rows_per_strip,
                                       &strip_count,
                                       &image_size,
                                       &base_tag_count,
                                       &extra_size);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)extra_size;

    tag_count = base_tag_count;
    if (page_count > 1UL) {
        if ((unsigned long)tag_count + 2UL > 65535UL) {
            return TIFX_ERR_OVERFLOW;
        }
        tag_count = (unsigned short)(tag_count + 2U);
    }

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }
    fill_order = tifx_bt_effective_fill_order(params);
    planar_config = tifx_bt_effective_planar_config(params);
    predictor = tifx_bt_effective_predictor(params);
    needs_fill_order_tag = (unsigned short)tifx_bt_should_write_fill_order_tag(params,
                                                                               compression,
                                                                               fill_order);
    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        photometric = 1U;
    } else if (params->pixel_format == TIFX_PIXEL_RGB24 ||
               params->pixel_format == TIFX_PIXEL_RGBA32) {
        photometric = 2U;
    } else {
        photometric = params->photometric;
    }

    rc = tifx__build_strip_byte_counts(params,
                                       stride_min,
                                       rows_per_strip,
                                       strip_count,
                                       strip_byte_counts,
                                       &image_size);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)image_size;

    cursor = page_base_offset;
    for (strip_index = 0UL; strip_index < strip_count; ++strip_index) {
        unsigned long start_row;
        unsigned long rows_this_strip;

        cursor = tifx_bt_align8(cursor);
        strip_offsets[strip_index] = cursor;

        start_row = strip_index * rows_per_strip;
        rows_this_strip = rows_per_strip;
        if (rows_this_strip > params->height - start_row) {
            rows_this_strip = params->height - start_row;
        }

        rc = tifx__write_segment_payload(out + cursor,
                                         strip_byte_counts[strip_index],
                                         params,
                                         stride_min,
                                         strip_index,
                                         rows_per_strip);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (!tifx_bt_add_ul(cursor, strip_byte_counts[strip_index], &cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
    }

    cursor = tifx_bt_align8(cursor);
    if (cursor != current_ifd_offset) {
        return TIFX_ERR_BAD_FORMAT;
    }

    if (!tifx_bt_mul_ul((unsigned long)tag_count, 20UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (!tifx_bt_add_ul(ifd_size, 16UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    extra_cursor = tifx_bt_align8(current_ifd_offset + ifd_size);

    strip_offsets_array_offset = 0UL;
    strip_byte_counts_array_offset = 0UL;
    if (strip_count > 1UL) {
        strip_offsets_array_offset = extra_cursor;
        if (!tifx_bt_add_ul(extra_cursor, strip_count * 8UL, &extra_cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        extra_cursor = tifx_bt_align8(extra_cursor);
        strip_byte_counts_array_offset = extra_cursor;
        if (!tifx_bt_add_ul(extra_cursor, strip_count * 8UL, &extra_cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        extra_cursor = tifx_bt_align8(extra_cursor);
    }

    tifx_bt_write_u64le(out + current_ifd_offset, (unsigned long)tag_count);
    entry_index = 0U;

    if (page_count > 1UL) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_NEW_SUBFILE_TYPE,
                                 TIFX_TYPE_LONG,
                                 2UL);
    }

    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_IMAGE_WIDTH, TIFX_TYPE_LONG, params->width);
    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_IMAGE_LENGTH, TIFX_TYPE_LONG, params->height);

    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 8UL);
    } else if (params->pixel_format == TIFX_PIXEL_RGB24) {
        static const unsigned short rgb_bits[3] = { 8U, 8U, 8U };
        tifx_bt_write_tag_inline_shorts(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                        TIFX_TAG_BITS_PER_SAMPLE,
                                        rgb_bits,
                                        3UL);
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        static const unsigned short rgba_bits[4] = { 8U, 8U, 8U, 8U };
        tifx_bt_write_tag_inline_shorts(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                        TIFX_TAG_BITS_PER_SAMPLE,
                                        rgba_bits,
                                        4UL);
    } else {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 1UL);
    }

    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_COMPRESSION, TIFX_TYPE_SHORT, (unsigned long)compression);
    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_PHOTOMETRIC, TIFX_TYPE_SHORT, (unsigned long)photometric);

    if (needs_fill_order_tag) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_FILL_ORDER, TIFX_TYPE_SHORT, (unsigned long)fill_order);
    }

    if (strip_count == 1UL) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_STRIP_OFFSETS, TIFX_TYPE_LONG8, strip_offsets[0]);
    } else {
        tifx_bt_write_tag_offset(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_STRIP_OFFSETS, TIFX_TYPE_LONG8, strip_count,
                                 strip_offsets_array_offset);
    }

    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_ORIENTATION, TIFX_TYPE_SHORT, 1UL);

    if (params->pixel_format == TIFX_PIXEL_RGB24 || params->pixel_format == TIFX_PIXEL_RGBA32) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_SAMPLES_PER_PIXEL,
                                 TIFX_TYPE_SHORT,
                                 (params->pixel_format == TIFX_PIXEL_RGBA32) ? 4UL : 3UL);
    }

    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_ROWS_PER_STRIP, TIFX_TYPE_LONG, rows_per_strip);

    if (strip_count == 1UL) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_STRIP_BYTE_COUNTS, TIFX_TYPE_LONG8, strip_byte_counts[0]);
    } else {
        tifx_bt_write_tag_offset(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_STRIP_BYTE_COUNTS, TIFX_TYPE_LONG8, strip_count,
                                 strip_byte_counts_array_offset);
    }

    xres = params->x_resolution;
    yres = params->y_resolution;
    if (xres <= 0) {
        xres = TIFX_FP_FROM_INT(72L);
    }
    if (yres <= 0) {
        yres = TIFX_FP_FROM_INT(72L);
    }

    if (predictor == TIFX_PREDICTOR_HORIZONTAL) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_PREDICTOR, TIFX_TYPE_SHORT, (unsigned long)predictor);
    }
    tifx_bt_write_tag_inline_rational(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                      TIFX_TAG_X_RESOLUTION,
                                      tifx_fp_to_rational_numerator(xres),
                                      tifx_fp_to_rational_denominator());
    tifx_bt_write_tag_inline_rational(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                      TIFX_TAG_Y_RESOLUTION,
                                      tifx_fp_to_rational_numerator(yres),
                                      tifx_fp_to_rational_denominator());

    if (params->pixel_format == TIFX_PIXEL_RGB24 || params->pixel_format == TIFX_PIXEL_RGBA32) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_PLANAR_CONFIGURATION, TIFX_TYPE_SHORT, (unsigned long)planar_config);
    }
    if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_EXTRA_SAMPLES, TIFX_TYPE_SHORT, params->alpha_mode);
    }
    if (params->pixel_format == TIFX_PIXEL_BILEVEL && compression == 3U) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_T4_OPTIONS, TIFX_TYPE_LONG, params->t4_options);
    }
    if (params->pixel_format == TIFX_PIXEL_BILEVEL && compression == 4U) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_T6_OPTIONS, TIFX_TYPE_LONG, params->t6_options);
    }

    resolution_unit = params->resolution_unit;
    if (resolution_unit == 0U) {
        resolution_unit = 2U;
    }
    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_RESOLUTION_UNIT, TIFX_TYPE_SHORT,
                             (unsigned long)resolution_unit);

    if (page_count > 1UL) {
        page_number_values[0] = (unsigned short)page_index;
        page_number_values[1] = (unsigned short)page_count;
        tifx_bt_write_tag_inline_shorts(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                        TIFX_TAG_PAGE_NUMBER,
                                        page_number_values,
                                        2UL);
    }

    if (entry_index != tag_count) {
        return TIFX_ERR_BAD_FORMAT;
    }

    tifx_bt_write_u64le(out + current_ifd_offset + 8UL + ((unsigned long)tag_count * 20UL),
                        next_ifd_offset);

    if (strip_count > 1UL) {
        tifx_bt_write_u64le_array(out + strip_offsets_array_offset, strip_offsets, strip_count);
        tifx_bt_write_u64le_array(out + strip_byte_counts_array_offset, strip_byte_counts, strip_count);
    }

    if (out_page_end != 0) {
        *out_page_end = extra_cursor;
    }
    return TIFX_OK;
}

unsigned long tifx_write_bigtiff_pages_buffer_size(const tifx_write_params *pages,
                                                   unsigned long page_count)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)pages;
    (void)page_count;
    return 0UL;
#else
    unsigned long cursor;
    unsigned long page_base_offset;
    unsigned long page_ifd_offset;
    unsigned long page_end;
    unsigned long page_index;
    int rc;

    if (!tifx_bigtiff_available()) {
        return 0UL;
    }
    if (pages == 0 || page_count == 0UL) {
        return 0UL;
    }
    if (page_count > TIFX_MAX_PAGES || page_count > 65535UL) {
        return 0UL;
    }

    cursor = 16UL;
    for (page_index = 0UL; page_index < page_count; ++page_index) {
        page_base_offset = tifx_bt_align8(cursor);
        rc = tifx_bt_measure_multipage_page_layout(&pages[page_index],
                                                   (unsigned short)(page_count > 1UL ? 2U : 0U),
                                                   page_base_offset,
                                                   &page_ifd_offset,
                                                   &page_end);
        if (rc != TIFX_OK) {
            return 0UL;
        }
        (void)page_ifd_offset;
        cursor = page_end;
    }
    return cursor;
#endif
}

int tifx_write_bigtiff_pages_memory(void *dst,
                                    unsigned long dst_size,
                                    const tifx_write_params *pages,
                                    unsigned long page_count,
                                    unsigned long *written_size)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)dst;
    (void)dst_size;
    (void)pages;
    (void)page_count;
    if (written_size != 0) {
        *written_size = 0UL;
    }
    return TIFX_ERR_UNSUPPORTED;
#else
    unsigned char *out;
    unsigned long total_size;
    unsigned long cursor;
    unsigned long page_base_offset;
    unsigned long page_ifd_offsets[TIFX_MAX_PAGES];
    unsigned long page_end;
    unsigned long page_index;
    int rc;

    if (dst == 0 || pages == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (!tifx_bigtiff_available()) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (page_count == 0UL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (page_count > TIFX_MAX_PAGES || page_count > 65535UL) {
        return TIFX_ERR_UNSUPPORTED;
    }

    total_size = tifx_write_bigtiff_pages_buffer_size(pages, page_count);
    if (total_size == 0UL) {
        return TIFX_ERR_OVERFLOW;
    }
    if (dst_size < total_size) {
        return TIFX_ERR_NO_SPACE;
    }

    cursor = 16UL;
    for (page_index = 0UL; page_index < page_count; ++page_index) {
        page_base_offset = tifx_bt_align8(cursor);
        rc = tifx_bt_measure_multipage_page_layout(&pages[page_index],
                                                   (unsigned short)(page_count > 1UL ? 2U : 0U),
                                                   page_base_offset,
                                                   &page_ifd_offsets[page_index],
                                                   &page_end);
        if (rc != TIFX_OK) {
            return rc;
        }
        cursor = page_end;
    }
    if (cursor != total_size) {
        return TIFX_ERR_BAD_FORMAT;
    }

    out = (unsigned char *)dst;
    memset(out, 0, total_size);
    out[0] = 'I';
    out[1] = 'I';
    tifx_bt_write_u16le(out + 2, 43U);
    tifx_bt_write_u16le(out + 4, 8U);
    tifx_bt_write_u16le(out + 6, 0U);
    tifx_bt_write_u64le(out + 8, page_ifd_offsets[0]);

    cursor = 16UL;
    for (page_index = 0UL; page_index < page_count; ++page_index) {
        unsigned long next_ifd_offset;

        page_base_offset = tifx_bt_align8(cursor);
        next_ifd_offset = 0UL;
        if (page_index + 1UL < page_count) {
            next_ifd_offset = page_ifd_offsets[page_index + 1UL];
        }

        rc = tifx_bt_write_multipage_page(out,
                                          &pages[page_index],
                                          page_base_offset,
                                          page_ifd_offsets[page_index],
                                          next_ifd_offset,
                                          page_index,
                                          page_count,
                                          &page_end);
        if (rc != TIFX_OK) {
            return rc;
        }
        cursor = page_end;
    }

    if (cursor != total_size) {
        return TIFX_ERR_BAD_FORMAT;
    }

    if (written_size != 0) {
        *written_size = total_size;
    }
    return TIFX_OK;
#endif
}


static unsigned long tifx_bt_effective_new_subfile_type(const tifx_write_params *params,
                                                        unsigned short auto_page_bit)
{
    unsigned long value;

    value = 0UL;
    if (params != 0) {
        value = params->new_subfile_type;
    }
    if (auto_page_bit != 0U) {
        value |= (unsigned long)auto_page_bit;
    }
    return value;
}

static unsigned short tifx_bt_effective_subifd_style(const tifx_write_params *params)
{
    if (params == 0) {
        return TIFX_SUBIFD_STYLE_TREE;
    }
    if (params->subifd_style == TIFX_SUBIFD_STYLE_ADOBE_CHAIN) {
        return TIFX_SUBIFD_STYLE_ADOBE_CHAIN;
    }
    return TIFX_SUBIFD_STYLE_TREE;
}

static int tifx_bt_tree_validate_write_params(const tifx_write_params *params,
                                              unsigned long *out_stride_min,
                                              unsigned long *out_segment_span,
                                              unsigned long *out_segment_count,
                                              unsigned long *out_image_size,
                                              unsigned short *out_tag_count,
                                              unsigned long *out_extra_size,
                                              int *out_is_tiled)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)params;
    (void)out_stride_min;
    (void)out_segment_span;
    (void)out_segment_count;
    (void)out_image_size;
    (void)out_tag_count;
    (void)out_extra_size;
    (void)out_is_tiled;
    return TIFX_ERR_UNSUPPORTED;
#else
    unsigned long stride_min;
    unsigned long input_stride_min;
    unsigned long segment_span;
    unsigned long segment_count;
    unsigned long image_size;
    unsigned short tag_count;
    unsigned long extra_size;
    unsigned short compression;
    unsigned short fill_order;
    unsigned short planar_config;
    unsigned short predictor;
    unsigned long sample_count;
    int rc;
    int is_tiled;

    if (!tifx_bigtiff_available()) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (params == 0 || params->pixels == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (params->width == 0UL || params->height == 0UL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }
    fill_order = tifx_bt_effective_fill_order(params);
    planar_config = tifx_bt_effective_planar_config(params);
    predictor = tifx_bt_effective_predictor(params);
    sample_count = tifx_bt_write_sample_count(params);
    if (fill_order != 1U && fill_order != 2U) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (predictor != TIFX_PREDICTOR_NONE && predictor != TIFX_PREDICTOR_HORIZONTAL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (params->deflate_mode != TIFX_DEFLATE_AUTO &&
        params->deflate_mode != TIFX_DEFLATE_STORED &&
        params->deflate_mode != TIFX_DEFLATE_FIXED &&
        params->deflate_mode != TIFX_DEFLATE_DYNAMIC) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    is_tiled = tifx__params_is_tiled(params);
    if (is_tiled < 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    if (is_tiled) {
        segment_span = params->tile_length;
        if ((params->tile_width & 15UL) != 0UL || (params->tile_length & 15UL) != 0UL) {
            return TIFX_ERR_UNSUPPORTED;
        }
        rc = tifx__compute_tile_count(params->width,
                                      params->height,
                                      params->tile_width,
                                      params->tile_length,
                                      &segment_count);
        if (rc != TIFX_OK) {
            return rc;
        }
    } else {
        segment_span = tifx_bt_effective_rows_per_strip(params);
        if (segment_span > 0xFFFFFFFFUL || params->width > 0xFFFFFFFFUL || params->height > 0xFFFFFFFFUL) {
            return TIFX_ERR_UNSUPPORTED;
        }
        rc = tifx_bt_compute_strip_count(params->height, segment_span, &segment_count);
        if (rc != TIFX_OK) {
            return rc;
        }
    }
    if (planar_config == 2U && sample_count > 1UL) {
        unsigned long total_segments;
        unsigned long limit;

        limit = (unsigned long)(is_tiled ? TIFX_MAX_TILES : TIFX_MAX_STRIPS);
        if (!tifx_bt_mul_ul(segment_count, sample_count, &total_segments)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (total_segments == 0UL || total_segments > limit) {
            return TIFX_ERR_UNSUPPORTED;
        }
        segment_count = total_segments;
    }

    extra_size = 0UL;
    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        if ((compression != 1U && compression != 32773U && compression != 5U &&
             compression != 8U && compression != 32946U) || fill_order != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (planar_config != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (predictor == TIFX_PREDICTOR_HORIZONTAL &&
            (compression != 5U && compression != 8U && compression != 32946U)) {
            return TIFX_ERR_UNSUPPORTED;
        }
        input_stride_min = params->width;
        stride_min = is_tiled ? params->tile_width : input_stride_min;
        tag_count = (unsigned short)(is_tiled ? 13U : 12U);
    } else if (params->pixel_format == TIFX_PIXEL_RGB24) {
        if ((compression != 1U && compression != 32773U && compression != 5U &&
             compression != 8U && compression != 32946U) || fill_order != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (planar_config != 1U && planar_config != 2U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (predictor == TIFX_PREDICTOR_HORIZONTAL &&
            (compression != 5U && compression != 8U && compression != 32946U)) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (!tifx_bt_mul_ul(params->width, 3UL, &input_stride_min)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (planar_config == 2U) {
            stride_min = is_tiled ? params->tile_width : params->width;
        } else if (is_tiled) {
            if (!tifx_bt_mul_ul(params->tile_width, 3UL, &stride_min)) {
                return TIFX_ERR_OVERFLOW;
            }
        } else {
            stride_min = input_stride_min;
        }
        tag_count = (unsigned short)(is_tiled ? 15U : 14U);
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        if ((compression != 1U && compression != 32773U && compression != 5U &&
             compression != 8U && compression != 32946U) || fill_order != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (planar_config != 1U && planar_config != 2U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (predictor == TIFX_PREDICTOR_HORIZONTAL &&
            (compression != 5U && compression != 8U && compression != 32946U)) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (params->alpha_mode != TIFX_ALPHA_ASSOCIATED &&
            params->alpha_mode != TIFX_ALPHA_UNASSOCIATED) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (!tifx_bt_mul_ul(params->width, 4UL, &input_stride_min)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (planar_config == 2U) {
            stride_min = is_tiled ? params->tile_width : params->width;
        } else if (is_tiled) {
            if (!tifx_bt_mul_ul(params->tile_width, 4UL, &stride_min)) {
                return TIFX_ERR_OVERFLOW;
            }
        } else {
            stride_min = input_stride_min;
        }
        tag_count = (unsigned short)(is_tiled ? 16U : 15U);
    } else if (params->pixel_format == TIFX_PIXEL_BILEVEL) {
        unsigned long segment_byte_counts[TIFX_MAX_TILES > TIFX_MAX_STRIPS ? TIFX_MAX_TILES : TIFX_MAX_STRIPS];

        if (planar_config != 1U) {
            return TIFX_ERR_UNSUPPORTED;
        }
        if (params->photometric != 0U && params->photometric != 1U) {
            return TIFX_ERR_BAD_ARGUMENT;
        }
        if (predictor != TIFX_PREDICTOR_NONE) {
            return TIFX_ERR_UNSUPPORTED;
        }
        input_stride_min = tifx_bt_ceil_div(params->width, 8UL);
        stride_min = is_tiled ? tifx_bt_ceil_div(params->tile_width, 8UL) : input_stride_min;
        if (is_tiled) {
            if (compression != 1U && compression != 32773U) {
                return TIFX_ERR_UNSUPPORTED;
            }
            tag_count = 13U;
        } else if (compression == 1U || compression == 2U || compression == 32773U ||
                   compression == 5U || compression == 8U || compression == 32946U) {
            if ((compression == 5U || compression == 8U || compression == 32946U) && fill_order != 1U) {
                return TIFX_ERR_UNSUPPORTED;
            }
            tag_count = 12U;
        } else if (compression == 3U) {
            if ((params->t4_options & ~5UL) != 0UL) {
                return TIFX_ERR_UNSUPPORTED;
            }
            tag_count = 13U;
        } else if (compression == 4U) {
            if (params->t6_options != 0UL) {
                return TIFX_ERR_UNSUPPORTED;
            }
            tag_count = 13U;
        } else {
            return TIFX_ERR_UNSUPPORTED;
        }

        rc = tifx__build_strip_byte_counts(params,
                                           stride_min,
                                           segment_span,
                                           segment_count,
                                           segment_byte_counts,
                                           &image_size);
        if (rc != TIFX_OK) {
            return rc;
        }
    } else {
        return TIFX_ERR_UNSUPPORTED;
    }

    if (params->pixel_format != TIFX_PIXEL_BILEVEL) {
        unsigned long segment_byte_counts[TIFX_MAX_TILES > TIFX_MAX_STRIPS ? TIFX_MAX_TILES : TIFX_MAX_STRIPS];
        rc = tifx__build_strip_byte_counts(params,
                                           stride_min,
                                           segment_span,
                                           segment_count,
                                           segment_byte_counts,
                                           &image_size);
        if (rc != TIFX_OK) {
            return rc;
        }
    }

    if (tifx_bt_should_write_fill_order_tag(params, compression, fill_order)) {
        ++tag_count;
    }
    if (predictor == TIFX_PREDICTOR_HORIZONTAL) {
        ++tag_count;
    }
    if (params->stride < input_stride_min) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (segment_count > 1UL) {
        unsigned long arrays_size;
        if (!tifx_bt_mul_ul(segment_count, 16UL, &arrays_size)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (!tifx_bt_add_ul(extra_size, arrays_size, &extra_size)) {
            return TIFX_ERR_OVERFLOW;
        }
    }

    *out_stride_min = stride_min;
    *out_segment_span = segment_span;
    *out_segment_count = segment_count;
    *out_image_size = image_size;
    *out_tag_count = tag_count;
    *out_extra_size = extra_size;
    if (out_is_tiled != 0) {
        *out_is_tiled = is_tiled;
    }
    return TIFX_OK;
#endif
}

static int tifx_bt_measure_tree_node_layout(const tifx_bigtiff_node *node,
                                            unsigned long page_base_offset,
                                            unsigned short auto_page_bit,
                                            unsigned long *out_ifd_offset,
                                            unsigned long *out_node_end,
                                            unsigned long *out_child_ifd_offsets,
                                            unsigned long depth)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)node;
    (void)page_base_offset;
    (void)auto_page_bit;
    (void)out_ifd_offset;
    (void)out_node_end;
    (void)out_child_ifd_offsets;
    (void)depth;
    return TIFX_ERR_UNSUPPORTED;
#else
    const tifx_write_params *params;
    unsigned long stride_min;
    unsigned long segment_span;
    unsigned long segment_count;
    unsigned long image_size;
    unsigned short base_tag_count;
    unsigned long extra_size;
    unsigned short tag_count;
    unsigned long segment_byte_counts[TIFX_MAX_TILES > TIFX_MAX_STRIPS ? TIFX_MAX_TILES : TIFX_MAX_STRIPS];
    unsigned long cursor;
    unsigned long ifd_offset;
    unsigned long ifd_size;
    unsigned long child_index;
    unsigned long child_ifd_offset;
    unsigned long child_end;
    unsigned long effective_new_subfile_type;
    unsigned short subifd_style;
    int rc;
    int is_tiled;

    if (node == 0 || out_ifd_offset == 0 || out_node_end == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (depth > TIFX_MAX_SUBIFD_DEPTH) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (node->child_count > TIFX_MAX_SUBIFDS) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (node->child_count > 0UL && node->children == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    params = &node->image;
    rc = tifx_bt_tree_validate_write_params(params,
                                            &stride_min,
                                            &segment_span,
                                            &segment_count,
                                            &image_size,
                                            &base_tag_count,
                                            &extra_size,
                                            &is_tiled);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)extra_size;
    (void)is_tiled;

    subifd_style = tifx_bt_effective_subifd_style(params);
    effective_new_subfile_type = tifx_bt_effective_new_subfile_type(params, auto_page_bit);
    tag_count = base_tag_count;
    if (effective_new_subfile_type != 0UL) {
        if ((unsigned long)tag_count == 65535UL) {
            return TIFX_ERR_OVERFLOW;
        }
        ++tag_count;
    }
    if (auto_page_bit != 0U) {
        if ((unsigned long)tag_count == 65535UL) {
            return TIFX_ERR_OVERFLOW;
        }
        ++tag_count;
    }
    if (node->child_count > 0UL) {
        if ((unsigned long)tag_count == 65535UL) {
            return TIFX_ERR_OVERFLOW;
        }
        ++tag_count;
    }

    rc = tifx__build_strip_byte_counts(params,
                                       stride_min,
                                       segment_span,
                                       segment_count,
                                       segment_byte_counts,
                                       &image_size);
    if (rc != TIFX_OK) {
        return rc;
    }

    cursor = tifx_bt_align8(page_base_offset);
    for (child_index = 0UL; child_index < segment_count; ++child_index) {
        cursor = tifx_bt_align8(cursor);
        if (!tifx_bt_add_ul(cursor, segment_byte_counts[child_index], &cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
    }

    for (child_index = 0UL; child_index < node->child_count; ++child_index) {
        cursor = tifx_bt_align8(cursor);
        rc = tifx_bt_measure_tree_node_layout(&node->children[child_index],
                                              cursor,
                                              0U,
                                              &child_ifd_offset,
                                              &child_end,
                                              0,
                                              depth + 1UL);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (out_child_ifd_offsets != 0) {
            out_child_ifd_offsets[child_index] = child_ifd_offset;
        }
        cursor = child_end;
    }

    ifd_offset = tifx_bt_align8(cursor);
    if (!tifx_bt_mul_ul((unsigned long)tag_count, 20UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (!tifx_bt_add_ul(ifd_size, 16UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    cursor = tifx_bt_align8(ifd_offset + ifd_size);

    if (segment_count > 1UL) {
        unsigned long arrays_size;
        if (!tifx_bt_mul_ul(segment_count, 8UL, &arrays_size)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (!tifx_bt_add_ul(cursor, arrays_size, &cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        cursor = tifx_bt_align8(cursor);
        if (!tifx_bt_add_ul(cursor, arrays_size, &cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        cursor = tifx_bt_align8(cursor);
    }

    if (node->child_count > 1UL && subifd_style == TIFX_SUBIFD_STYLE_TREE) {
        unsigned long arrays_size;
        if (!tifx_bt_mul_ul(node->child_count, 8UL, &arrays_size)) {
            return TIFX_ERR_OVERFLOW;
        }
        if (!tifx_bt_add_ul(cursor, arrays_size, &cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        cursor = tifx_bt_align8(cursor);
    }

    *out_ifd_offset = ifd_offset;
    *out_node_end = cursor;
    return TIFX_OK;
#endif
}

static int tifx_bt_write_tree_node(unsigned char *out,
                                   const tifx_bigtiff_node *node,
                                   unsigned long page_base_offset,
                                   unsigned long current_ifd_offset,
                                   unsigned long next_ifd_offset,
                                   unsigned short auto_page_bit,
                                   unsigned long page_index,
                                   unsigned long page_count,
                                   unsigned long *out_node_end,
                                   unsigned long depth)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)out;
    (void)node;
    (void)page_base_offset;
    (void)current_ifd_offset;
    (void)next_ifd_offset;
    (void)auto_page_bit;
    (void)page_index;
    (void)page_count;
    (void)out_node_end;
    (void)depth;
    return TIFX_ERR_UNSUPPORTED;
#else
    const tifx_write_params *params;
    unsigned long stride_min;
    unsigned long segment_span;
    unsigned long segment_count;
    unsigned long image_size;
    unsigned short base_tag_count;
    unsigned long extra_size;
    unsigned short tag_count;
    unsigned long segment_offsets[TIFX_MAX_TILES > TIFX_MAX_STRIPS ? TIFX_MAX_TILES : TIFX_MAX_STRIPS];
    unsigned long segment_byte_counts[TIFX_MAX_TILES > TIFX_MAX_STRIPS ? TIFX_MAX_TILES : TIFX_MAX_STRIPS];
    unsigned long child_ifd_offsets[TIFX_MAX_SUBIFDS];
    unsigned long child_base_offsets[TIFX_MAX_SUBIFDS];
    unsigned long child_end_offsets[TIFX_MAX_SUBIFDS];
    unsigned short compression;
    unsigned short fill_order;
    unsigned short planar_config;
    unsigned short predictor;
    unsigned short needs_fill_order_tag;
    unsigned short photometric;
    unsigned short resolution_unit;
    unsigned short entry_index;
    unsigned long cursor;
    unsigned long ifd_size;
    unsigned long extra_cursor;
    unsigned long segment_offsets_array_offset;
    unsigned long segment_byte_counts_array_offset;
    unsigned long subifd_offsets_array_offset;
    unsigned long segment_index;
    unsigned long child_index;
    unsigned long child_end;
    unsigned long effective_new_subfile_type;
    unsigned short subifd_style;
    unsigned short page_number_values[2];
    tifx_fixed xres;
    tifx_fixed yres;
    int rc;
    int is_tiled;

    if (out == 0 || node == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (depth > TIFX_MAX_SUBIFD_DEPTH) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (node->child_count > TIFX_MAX_SUBIFDS) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (node->child_count > 0UL && node->children == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }

    params = &node->image;
    rc = tifx_bt_tree_validate_write_params(params,
                                            &stride_min,
                                            &segment_span,
                                            &segment_count,
                                            &image_size,
                                            &base_tag_count,
                                            &extra_size,
                                            &is_tiled);
    if (rc != TIFX_OK) {
        return rc;
    }
    (void)extra_size;

    compression = params->compression;
    if (compression == 0U) {
        compression = 1U;
    }
    fill_order = tifx_bt_effective_fill_order(params);
    planar_config = tifx_bt_effective_planar_config(params);
    predictor = tifx_bt_effective_predictor(params);
    needs_fill_order_tag = (unsigned short)tifx_bt_should_write_fill_order_tag(params,
                                                                               compression,
                                                                               fill_order);
    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        photometric = 1U;
    } else if (params->pixel_format == TIFX_PIXEL_RGB24 ||
               params->pixel_format == TIFX_PIXEL_RGBA32) {
        photometric = 2U;
    } else {
        photometric = params->photometric;
    }

    subifd_style = tifx_bt_effective_subifd_style(params);
    effective_new_subfile_type = tifx_bt_effective_new_subfile_type(params, auto_page_bit);
    tag_count = base_tag_count;
    if (effective_new_subfile_type != 0UL) {
        ++tag_count;
    }
    if (auto_page_bit != 0U) {
        ++tag_count;
    }
    if (node->child_count > 0UL) {
        ++tag_count;
    }

    rc = tifx__build_strip_byte_counts(params,
                                       stride_min,
                                       segment_span,
                                       segment_count,
                                       segment_byte_counts,
                                       &image_size);
    if (rc != TIFX_OK) {
        return rc;
    }

    cursor = tifx_bt_align8(page_base_offset);
    for (segment_index = 0UL; segment_index < segment_count; ++segment_index) {
        cursor = tifx_bt_align8(cursor);
        segment_offsets[segment_index] = cursor;
        rc = tifx__write_segment_payload(out + cursor,
                                         segment_byte_counts[segment_index],
                                         params,
                                         stride_min,
                                         segment_index,
                                         segment_span);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (!tifx_bt_add_ul(cursor, segment_byte_counts[segment_index], &cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
    }

    for (child_index = 0UL; child_index < node->child_count; ++child_index) {
        unsigned long child_ifd_offset;

        cursor = tifx_bt_align8(cursor);
        child_base_offsets[child_index] = cursor;
        rc = tifx_bt_measure_tree_node_layout(&node->children[child_index],
                                              cursor,
                                              0U,
                                              &child_ifd_offset,
                                              &child_end,
                                              0,
                                              depth + 1UL);
        if (rc != TIFX_OK) {
            return rc;
        }
        child_ifd_offsets[child_index] = child_ifd_offset;
        child_end_offsets[child_index] = child_end;
        cursor = child_end;
    }

    for (child_index = 0UL; child_index < node->child_count; ++child_index) {
        unsigned long child_next_ifd_offset;

        child_next_ifd_offset = 0UL;
        if (subifd_style == TIFX_SUBIFD_STYLE_ADOBE_CHAIN && child_index + 1UL < node->child_count) {
            child_next_ifd_offset = child_ifd_offsets[child_index + 1UL];
        }
        child_end = child_end_offsets[child_index];
        rc = tifx_bt_write_tree_node(out,
                                     &node->children[child_index],
                                     child_base_offsets[child_index],
                                     child_ifd_offsets[child_index],
                                     child_next_ifd_offset,
                                     0U,
                                     0UL,
                                     0UL,
                                     &child_end,
                                     depth + 1UL);
        if (rc != TIFX_OK) {
            return rc;
        }
        if (child_end != child_end_offsets[child_index]) {
            return TIFX_ERR_BAD_FORMAT;
        }
    }

    cursor = tifx_bt_align8(cursor);
    if (cursor != current_ifd_offset) {
        return TIFX_ERR_BAD_FORMAT;
    }

    if (!tifx_bt_mul_ul((unsigned long)tag_count, 20UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    if (!tifx_bt_add_ul(ifd_size, 16UL, &ifd_size)) {
        return TIFX_ERR_OVERFLOW;
    }
    extra_cursor = tifx_bt_align8(current_ifd_offset + ifd_size);

    segment_offsets_array_offset = 0UL;
    segment_byte_counts_array_offset = 0UL;
    subifd_offsets_array_offset = 0UL;
    if (segment_count > 1UL) {
        segment_offsets_array_offset = extra_cursor;
        if (!tifx_bt_add_ul(extra_cursor, segment_count * 8UL, &extra_cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        extra_cursor = tifx_bt_align8(extra_cursor);
        segment_byte_counts_array_offset = extra_cursor;
        if (!tifx_bt_add_ul(extra_cursor, segment_count * 8UL, &extra_cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        extra_cursor = tifx_bt_align8(extra_cursor);
    }
    if (node->child_count > 1UL && subifd_style == TIFX_SUBIFD_STYLE_TREE) {
        subifd_offsets_array_offset = extra_cursor;
        if (!tifx_bt_add_ul(extra_cursor, node->child_count * 8UL, &extra_cursor)) {
            return TIFX_ERR_OVERFLOW;
        }
        extra_cursor = tifx_bt_align8(extra_cursor);
    }

    tifx_bt_write_u64le(out + current_ifd_offset, (unsigned long)tag_count);
    entry_index = 0U;

    if (effective_new_subfile_type != 0UL) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_NEW_SUBFILE_TYPE,
                                 TIFX_TYPE_LONG,
                                 effective_new_subfile_type);
    }

    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_IMAGE_WIDTH, TIFX_TYPE_LONG, params->width);
    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_IMAGE_LENGTH, TIFX_TYPE_LONG, params->height);

    if (params->pixel_format == TIFX_PIXEL_GRAY8) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 8UL);
    } else if (params->pixel_format == TIFX_PIXEL_RGB24) {
        static const unsigned short rgb_bits[3] = { 8U, 8U, 8U };
        tifx_bt_write_tag_inline_shorts(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                        TIFX_TAG_BITS_PER_SAMPLE,
                                        rgb_bits,
                                        3UL);
    } else if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        static const unsigned short rgba_bits[4] = { 8U, 8U, 8U, 8U };
        tifx_bt_write_tag_inline_shorts(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                        TIFX_TAG_BITS_PER_SAMPLE,
                                        rgba_bits,
                                        4UL);
    } else {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_BITS_PER_SAMPLE, TIFX_TYPE_SHORT, 1UL);
    }

    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_COMPRESSION, TIFX_TYPE_SHORT, (unsigned long)compression);
    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_PHOTOMETRIC, TIFX_TYPE_SHORT, (unsigned long)photometric);

    if (needs_fill_order_tag) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_FILL_ORDER, TIFX_TYPE_SHORT, (unsigned long)fill_order);
    }

    if (is_tiled) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_TILE_WIDTH, TIFX_TYPE_LONG, params->tile_width);
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_TILE_LENGTH, TIFX_TYPE_LONG, params->tile_length);
        if (segment_count == 1UL) {
            tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                     TIFX_TAG_TILE_OFFSETS, TIFX_TYPE_LONG8, segment_offsets[0]);
        } else {
            tifx_bt_write_tag_offset(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                     TIFX_TAG_TILE_OFFSETS, TIFX_TYPE_LONG8, segment_count,
                                     segment_offsets_array_offset);
        }
    } else {
        if (segment_count == 1UL) {
            tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                     TIFX_TAG_STRIP_OFFSETS, TIFX_TYPE_LONG8, segment_offsets[0]);
        } else {
            tifx_bt_write_tag_offset(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                     TIFX_TAG_STRIP_OFFSETS, TIFX_TYPE_LONG8, segment_count,
                                     segment_offsets_array_offset);
        }
    }

    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_ORIENTATION, TIFX_TYPE_SHORT, 1UL);

    if (params->pixel_format == TIFX_PIXEL_RGB24 || params->pixel_format == TIFX_PIXEL_RGBA32) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_SAMPLES_PER_PIXEL,
                                 TIFX_TYPE_SHORT,
                                 (params->pixel_format == TIFX_PIXEL_RGBA32) ? 4UL : 3UL);
    }

    if (is_tiled) {
        if (segment_count == 1UL) {
            tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                     TIFX_TAG_TILE_BYTE_COUNTS, TIFX_TYPE_LONG8, segment_byte_counts[0]);
        } else {
            tifx_bt_write_tag_offset(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                     TIFX_TAG_TILE_BYTE_COUNTS, TIFX_TYPE_LONG8, segment_count,
                                     segment_byte_counts_array_offset);
        }
    } else {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_ROWS_PER_STRIP, TIFX_TYPE_LONG, segment_span);
        if (segment_count == 1UL) {
            tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                     TIFX_TAG_STRIP_BYTE_COUNTS, TIFX_TYPE_LONG8, segment_byte_counts[0]);
        } else {
            tifx_bt_write_tag_offset(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                     TIFX_TAG_STRIP_BYTE_COUNTS, TIFX_TYPE_LONG8, segment_count,
                                     segment_byte_counts_array_offset);
        }
    }

    xres = params->x_resolution;
    yres = params->y_resolution;
    if (xres <= 0) {
        xres = TIFX_FP_FROM_INT(72L);
    }
    if (yres <= 0) {
        yres = TIFX_FP_FROM_INT(72L);
    }

    if (predictor == TIFX_PREDICTOR_HORIZONTAL) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_PREDICTOR, TIFX_TYPE_SHORT, (unsigned long)predictor);
    }
    tifx_bt_write_tag_inline_rational(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                      TIFX_TAG_X_RESOLUTION,
                                      tifx_fp_to_rational_numerator(xres),
                                      tifx_fp_to_rational_denominator());
    tifx_bt_write_tag_inline_rational(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                      TIFX_TAG_Y_RESOLUTION,
                                      tifx_fp_to_rational_numerator(yres),
                                      tifx_fp_to_rational_denominator());

    if (params->pixel_format == TIFX_PIXEL_RGB24 || params->pixel_format == TIFX_PIXEL_RGBA32) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_PLANAR_CONFIGURATION, TIFX_TYPE_SHORT, (unsigned long)planar_config);
    }
    if (params->pixel_format == TIFX_PIXEL_RGBA32) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_EXTRA_SAMPLES, TIFX_TYPE_SHORT, params->alpha_mode);
    }
    if (!is_tiled && params->pixel_format == TIFX_PIXEL_BILEVEL && compression == 3U) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_T4_OPTIONS, TIFX_TYPE_LONG, params->t4_options);
    }
    if (!is_tiled && params->pixel_format == TIFX_PIXEL_BILEVEL && compression == 4U) {
        tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_T6_OPTIONS, TIFX_TYPE_LONG, params->t6_options);
    }

    resolution_unit = params->resolution_unit;
    if (resolution_unit == 0U) {
        resolution_unit = 2U;
    }
    tifx_bt_write_tag_scalar(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                             TIFX_TAG_RESOLUTION_UNIT, TIFX_TYPE_SHORT,
                             (unsigned long)resolution_unit);

    if (auto_page_bit != 0U) {
        page_number_values[0] = (unsigned short)page_index;
        page_number_values[1] = (unsigned short)page_count;
        tifx_bt_write_tag_inline_shorts(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                        TIFX_TAG_PAGE_NUMBER,
                                        page_number_values,
                                        2UL);
    }

    if (node->child_count > 0UL) {
        unsigned long subifd_count;
        unsigned long subifd_value;

        subifd_count = (subifd_style == TIFX_SUBIFD_STYLE_ADOBE_CHAIN) ? 1UL : node->child_count;
        if (subifd_count == 1UL) {
            subifd_value = child_ifd_offsets[0];
        } else {
            subifd_value = subifd_offsets_array_offset;
        }
        tifx_bt_write_tag_offset(out + current_ifd_offset + 8UL + ((unsigned long)entry_index++ * 20UL),
                                 TIFX_TAG_SUBIFDS,
                                 TIFX_TYPE_IFD8,
                                 subifd_count,
                                 subifd_value);
    }

    if (entry_index != tag_count) {
        return TIFX_ERR_BAD_FORMAT;
    }

    tifx_bt_write_u64le(out + current_ifd_offset + 8UL + ((unsigned long)tag_count * 20UL),
                        next_ifd_offset);

    if (segment_count > 1UL) {
        tifx_bt_write_u64le_array(out + segment_offsets_array_offset, segment_offsets, segment_count);
        tifx_bt_write_u64le_array(out + segment_byte_counts_array_offset, segment_byte_counts, segment_count);
    }
    if (node->child_count > 1UL && subifd_style == TIFX_SUBIFD_STYLE_TREE) {
        tifx_bt_write_u64le_array(out + subifd_offsets_array_offset, child_ifd_offsets, node->child_count);
    }

    if (out_node_end != 0) {
        *out_node_end = extra_cursor;
    }
    return TIFX_OK;
#endif
}

unsigned long tifx_write_bigtiff_tree_buffer_size(const tifx_bigtiff_node *pages,
                                                  unsigned long page_count)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)pages;
    (void)page_count;
    return 0UL;
#else
    unsigned long cursor;
    unsigned long page_base_offset;
    unsigned long page_ifd_offset;
    unsigned long page_end;
    unsigned long page_index;
    int rc;

    if (!tifx_bigtiff_available()) {
        return 0UL;
    }
    if (pages == 0 || page_count == 0UL) {
        return 0UL;
    }
    if (page_count > TIFX_MAX_PAGES || page_count > 65535UL) {
        return 0UL;
    }

    cursor = 16UL;
    for (page_index = 0UL; page_index < page_count; ++page_index) {
        page_base_offset = tifx_bt_align8(cursor);
        rc = tifx_bt_measure_tree_node_layout(&pages[page_index],
                                              page_base_offset,
                                              (unsigned short)(page_count > 1UL ? 2U : 0U),
                                              &page_ifd_offset,
                                              &page_end,
                                              0,
                                              0UL);
        if (rc != TIFX_OK) {
            return 0UL;
        }
        (void)page_ifd_offset;
        cursor = page_end;
    }
    return cursor;
#endif
}

int tifx_write_bigtiff_tree_memory(void *dst,
                                   unsigned long dst_size,
                                   const tifx_bigtiff_node *pages,
                                   unsigned long page_count,
                                   unsigned long *written_size)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    (void)dst;
    (void)dst_size;
    (void)pages;
    (void)page_count;
    if (written_size != 0) {
        *written_size = 0UL;
    }
    return TIFX_ERR_UNSUPPORTED;
#else
    unsigned char *out;
    unsigned long total_size;
    unsigned long cursor;
    unsigned long page_base_offset;
    unsigned long page_ifd_offsets[TIFX_MAX_PAGES];
    unsigned long page_end;
    unsigned long page_index;
    int rc;

    if (dst == 0 || pages == 0) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (!tifx_bigtiff_available()) {
        return TIFX_ERR_UNSUPPORTED;
    }
    if (page_count == 0UL) {
        return TIFX_ERR_BAD_ARGUMENT;
    }
    if (page_count > TIFX_MAX_PAGES || page_count > 65535UL) {
        return TIFX_ERR_UNSUPPORTED;
    }

    total_size = tifx_write_bigtiff_tree_buffer_size(pages, page_count);
    if (total_size == 0UL) {
        return TIFX_ERR_OVERFLOW;
    }
    if (dst_size < total_size) {
        return TIFX_ERR_NO_SPACE;
    }

    cursor = 16UL;
    for (page_index = 0UL; page_index < page_count; ++page_index) {
        page_base_offset = tifx_bt_align8(cursor);
        rc = tifx_bt_measure_tree_node_layout(&pages[page_index],
                                              page_base_offset,
                                              (unsigned short)(page_count > 1UL ? 2U : 0U),
                                              &page_ifd_offsets[page_index],
                                              &page_end,
                                              0,
                                              0UL);
        if (rc != TIFX_OK) {
            return rc;
        }
        cursor = page_end;
    }
    if (cursor != total_size) {
        return TIFX_ERR_BAD_FORMAT;
    }

    out = (unsigned char *)dst;
    memset(out, 0, total_size);
    out[0] = 'I';
    out[1] = 'I';
    tifx_bt_write_u16le(out + 2, 43U);
    tifx_bt_write_u16le(out + 4, 8U);
    tifx_bt_write_u16le(out + 6, 0U);
    tifx_bt_write_u64le(out + 8, page_ifd_offsets[0]);

    cursor = 16UL;
    for (page_index = 0UL; page_index < page_count; ++page_index) {
        unsigned long next_ifd_offset;

        page_base_offset = tifx_bt_align8(cursor);
        next_ifd_offset = 0UL;
        if (page_index + 1UL < page_count) {
            next_ifd_offset = page_ifd_offsets[page_index + 1UL];
        }

        rc = tifx_bt_write_tree_node(out,
                                     &pages[page_index],
                                     page_base_offset,
                                     page_ifd_offsets[page_index],
                                     next_ifd_offset,
                                     (unsigned short)(page_count > 1UL ? 2U : 0U),
                                     page_index,
                                     page_count,
                                     &page_end,
                                     0UL);
        if (rc != TIFX_OK) {
            return rc;
        }
        cursor = page_end;
    }

    if (cursor != total_size) {
        return TIFX_ERR_BAD_FORMAT;
    }

    if (written_size != 0) {
        *written_size = total_size;
    }
    return TIFX_OK;
#endif
}
