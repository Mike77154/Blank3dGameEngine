#include "bmp_encoder.h"

#define BMP_MATH32_ENABLE_ADD 1
#define BMP_MATH32_ENABLE_MUL 1
#define BMP_MATH32_ENABLE_SCALE_FROM_U8 1
#include "bmp_math32.h"

#include <string.h>

typedef struct bmp_buf_s {
    bmp_u8 *data;
    bmp_u32 size;
    bmp_u32 capacity;
} bmp_buf;

typedef struct bmp_encode_plan_s {
    bmp_u32 header_size;
    bmp_u16 bpp;
    bmp_u32 file_compression;
    bmp_u32 external_mask_bytes;
    bmp_u32 palette_entry_size;
    bmp_u32 palette_entries;
    bmp_u32 palette_bytes;
    bmp_u32 pixel_offset;
    bmp_u32 pixel_data_size;
    bmp_u32 file_size;
    bmp_u32 red_mask;
    bmp_u32 green_mask;
    bmp_u32 blue_mask;
    bmp_u32 alpha_mask;
    bmp_u32 color_space_type;
    bmp_u32 rendering_intent;
    bmp_u32 profile_data_offset;
    bmp_u32 profile_size;
    int is_core;
    int top_down;
} bmp_encode_plan;

static void bmp_write_u16le(bmp_u8 *dst, bmp_u16 v)
{
    dst[0] = (bmp_u8)(v & 0xFFU);
    dst[1] = (bmp_u8)((v >> 8) & 0xFFU);
}

static void bmp_write_u32le(bmp_u8 *dst, bmp_u32 v)
{
    dst[0] = (bmp_u8)(v & 0xFFU);
    dst[1] = (bmp_u8)((v >> 8) & 0xFFU);
    dst[2] = (bmp_u8)((v >> 16) & 0xFFU);
    dst[3] = (bmp_u8)((v >> 24) & 0xFFU);
}

static void bmp_write_s32le(bmp_u8 *dst, bmp_s32 v)
{
    bmp_write_u32le(dst, (bmp_u32)v);
}

static int bmp_buf_reserve(bmp_buf *buf, bmp_u32 extra)
{
    bmp_u32 needed;
    if (!buf) return BMP_ERR_ARGUMENT;
    if (!bmp_u32_add_checked(buf->size, extra, &needed)) return BMP_ERR_OVERFLOW;
    if (needed > buf->capacity) return BMP_ERR_BUFFER_TOO_SMALL;
    return BMP_OK;
}

static int bmp_buf_append_byte(bmp_buf *buf, bmp_u8 v)
{
    int rc = bmp_buf_reserve(buf, 1U);
    if (rc != BMP_OK) return rc;
    buf->data[buf->size++] = v;
    return BMP_OK;
}

static int bmp_buf_append_bytes(bmp_buf *buf, const bmp_u8 *src, bmp_u32 count)
{
    int rc;
    if (count == 0U) return BMP_OK;
    if (!src) return BMP_ERR_ARGUMENT;
    rc = bmp_buf_reserve(buf, count);
    if (rc != BMP_OK) return rc;
    memcpy(buf->data + buf->size, src, count);
    buf->size += count;
    return BMP_OK;
}

static int bmp_calc_mask_shift(bmp_u32 mask, int *shift, int *bits)
{
    bmp_u32 m = mask;
    int s = 0;
    int b = 0;

    if (!shift || !bits) return BMP_ERR_ARGUMENT;
    if (mask == 0U) {
        *shift = 0;
        *bits = 0;
        return BMP_OK;
    }
    while ((m & 1U) == 0U) {
        m >>= 1;
        ++s;
    }
    while ((m & 1U) != 0U) {
        m >>= 1;
        ++b;
    }
    if (m != 0U) return BMP_ERR_MASKS;
    *shift = s;
    *bits = b;
    return BMP_OK;
}

static int bmp_mask_is_contiguous(bmp_u32 mask)
{
    bmp_u32 m = mask;
    if (m == 0U) return 1;
    while ((m & 1U) == 0U) m >>= 1;
    while ((m & 1U) == 1U) m >>= 1;
    return (m == 0U);
}

static int bmp_masks_valid(bmp_u32 r, bmp_u32 g, bmp_u32 b, bmp_u32 a, bmp_u16 bpp)
{
    bmp_u32 limit;
    if (bpp == 32U) limit = 0xFFFFFFFFU;
    else if (bpp < 32U) limit = (1U << bpp) - 1U;
    else return BMP_ERR_MASKS;

    if ((r | g | b) == 0U) return BMP_ERR_MASKS;
    if ((r & ~limit) != 0U || (g & ~limit) != 0U ||
        (b & ~limit) != 0U || (a & ~limit) != 0U) return BMP_ERR_MASKS;
    if ((r & g) != 0U || (r & b) != 0U || (r & a) != 0U ||
        (g & b) != 0U || (g & a) != 0U || (b & a) != 0U) return BMP_ERR_MASKS;
    if (!bmp_mask_is_contiguous(r) || !bmp_mask_is_contiguous(g) ||
        !bmp_mask_is_contiguous(b) || (a != 0U && !bmp_mask_is_contiguous(a))) return BMP_ERR_MASKS;
    return BMP_OK;
}

static bmp_u32 bmp_scale_from_8bit(bmp_u8 value, int bits)
{
    bmp_u32 max_val;
    if (bits <= 0) return 0U;
    if (bits >= 32) max_val = 0xFFFFFFFFU;
    else max_val = (1U << bits) - 1U;
    return bmp_scale_u8_to_u32_exact(value, max_val);
}

static bmp_u32 bmp_pack_channel(bmp_u8 value, bmp_u32 mask)
{
    int shift, bits;
    if (mask == 0U) return 0U;
    if (bmp_calc_mask_shift(mask, &shift, &bits) != BMP_OK) return 0U;
    return (bmp_scale_from_8bit(value, bits) << shift) & mask;
}

static int bmp_validate_input_stride_rgba32(bmp_u32 width, bmp_u32 stride)
{
    bmp_u32 need;
    if (!bmp_u32_mul_checked(width, 4U, &need)) return BMP_ERR_OVERFLOW;
    return (stride >= need) ? BMP_OK : BMP_ERR_ARGUMENT;
}

static int bmp_validate_input_stride_indexed(bmp_u32 width, bmp_u32 stride)
{
    return (stride >= width) ? BMP_OK : BMP_ERR_ARGUMENT;
}

static void bmp_copy_palette_quad(bmp_u8 *dst, const bmp_palette_entry *palette, bmp_u32 entries)
{
    bmp_u32 i;
    for (i = 0; i < entries; ++i) {
        dst[i * 4U + 0U] = palette[i].b;
        dst[i * 4U + 1U] = palette[i].g;
        dst[i * 4U + 2U] = palette[i].r;
        dst[i * 4U + 3U] = 0U;
    }
}

static void bmp_copy_palette_core(bmp_u8 *dst, const bmp_palette_entry *palette, bmp_u32 entries)
{
    bmp_u32 i;
    for (i = 0; i < entries; ++i) {
        dst[i * 3U + 0U] = palette[i].b;
        dst[i * 3U + 1U] = palette[i].g;
        dst[i * 3U + 2U] = palette[i].r;
    }
}

static int bmp_validate_index_range(const bmp_u8 *indices, bmp_u32 width, bmp_u32 height,
                                    bmp_u32 stride, bmp_u32 palette_size)
{
    bmp_u32 y, x;
    if (!indices) return BMP_ERR_ARGUMENT;
    for (y = 0; y < height; ++y) {
        const bmp_u8 *row = indices + y * stride;
        for (x = 0; x < width; ++x) {
            if ((bmp_u32)row[x] >= palette_size) return BMP_ERR_PALETTE;
        }
    }
    return BMP_OK;
}

static int bmp_encode_row_indexed_1bpp(const bmp_u8 *src, bmp_u32 width, bmp_u8 *dst, bmp_u32 dst_size)
{
    bmp_u32 x;
    bmp_u32 bytes = (width + 7U) / 8U;
    if (dst_size < bytes) return BMP_ERR_BUFFER_TOO_SMALL;
    memset(dst, 0, dst_size);
    for (x = 0; x < width; ++x) {
        bmp_u8 idx = (bmp_u8)(src[x] & 1U);
        dst[x >> 3] |= (bmp_u8)(idx << (7U - (x & 7U)));
    }
    return BMP_OK;
}

static int bmp_encode_row_indexed_4bpp(const bmp_u8 *src, bmp_u32 width, bmp_u8 *dst, bmp_u32 dst_size)
{
    bmp_u32 x;
    bmp_u32 bytes = (width + 1U) / 2U;
    if (dst_size < bytes) return BMP_ERR_BUFFER_TOO_SMALL;
    memset(dst, 0, dst_size);
    for (x = 0; x < width; ++x) {
        bmp_u8 idx = (bmp_u8)(src[x] & 0x0FU);
        if ((x & 1U) == 0U) dst[x >> 1] = (bmp_u8)(idx << 4);
        else dst[x >> 1] |= idx;
    }
    return BMP_OK;
}

static int bmp_encode_uncompressed_indexed(const bmp_u8 *indices, bmp_u32 width, bmp_u32 height,
                                           bmp_u32 stride, bmp_u16 bpp, int top_down,
                                           bmp_u8 *pixels, bmp_u32 pixel_capacity,
                                           bmp_u32 *out_size)
{
    bmp_u32 row_stride;
    bmp_u32 y;
    int rc;

    if (!pixels || !out_size) return BMP_ERR_ARGUMENT;
    rc = bmp_calc_image_size(width, height, bpp, out_size);
    if (rc != BMP_OK) return rc;
    if (pixel_capacity < *out_size) return BMP_ERR_BUFFER_TOO_SMALL;
    memset(pixels, 0, *out_size);

    rc = bmp_calc_row_stride(width, bpp, &row_stride);
    if (rc != BMP_OK) return rc;

    for (y = 0; y < height; ++y) {
        bmp_u32 src_y = top_down ? y : (height - 1U - y);
        const bmp_u8 *src_row = indices + src_y * stride;
        bmp_u8 *dst_row = pixels + y * row_stride;
        if (bpp == 1U) rc = bmp_encode_row_indexed_1bpp(src_row, width, dst_row, row_stride);
        else if (bpp == 4U) rc = bmp_encode_row_indexed_4bpp(src_row, width, dst_row, row_stride);
        else {
            if (row_stride < width) return BMP_ERR_BUFFER_TOO_SMALL;
            memcpy(dst_row, src_row, width);
            rc = BMP_OK;
        }
        if (rc != BMP_OK) return rc;
    }

    return BMP_OK;
}

static bmp_u32 bmp_same_run_len(const bmp_u8 *row, bmp_u32 width, bmp_u32 x, bmp_u32 limit)
{
    bmp_u32 n = 1U;
    bmp_u8 v = row[x];
    while (x + n < width && n < limit && row[x + n] == v) ++n;
    return n;
}

static int bmp_encode_rle8_row(const bmp_u8 *row, bmp_u32 width, bmp_buf *buf)
{
    bmp_u32 x = 0;
    int rc;

    while (x < width) {
        bmp_u32 run = bmp_same_run_len(row, width, x, 255U);
        if (run >= 3U) {
            rc = bmp_buf_append_byte(buf, (bmp_u8)run);
            if (rc != BMP_OK) return rc;
            rc = bmp_buf_append_byte(buf, row[x]);
            if (rc != BMP_OK) return rc;
            x += run;
        } else {
            bmp_u32 start = x;
            bmp_u32 literal = 0U;
            while (x < width) {
                run = bmp_same_run_len(row, width, x, 255U);
                if (run >= 3U) break;
                ++x;
                ++literal;
                if (literal == 255U) break;
            }
            if (literal <= 2U) {
                bmp_u32 k;
                for (k = 0; k < literal; ++k) {
                    rc = bmp_buf_append_byte(buf, 1U);
                    if (rc != BMP_OK) return rc;
                    rc = bmp_buf_append_byte(buf, row[start + k]);
                    if (rc != BMP_OK) return rc;
                }
            } else {
                rc = bmp_buf_append_byte(buf, 0U);
                if (rc != BMP_OK) return rc;
                rc = bmp_buf_append_byte(buf, (bmp_u8)literal);
                if (rc != BMP_OK) return rc;
                rc = bmp_buf_append_bytes(buf, row + start, literal);
                if (rc != BMP_OK) return rc;
                if ((literal & 1U) != 0U) {
                    rc = bmp_buf_append_byte(buf, 0U);
                    if (rc != BMP_OK) return rc;
                }
            }
        }
    }

    rc = bmp_buf_append_byte(buf, 0U);
    if (rc != BMP_OK) return rc;
    rc = bmp_buf_append_byte(buf, 0U);
    if (rc != BMP_OK) return rc;
    return BMP_OK;
}

static int bmp_encode_rle8(const bmp_u8 *indices, bmp_u32 width, bmp_u32 height,
                           bmp_u32 stride, bmp_u8 *pixels, bmp_u32 pixel_capacity,
                           bmp_u32 *out_size)
{
    bmp_buf buf;
    bmp_u32 y;
    int rc;

    if (!pixels || !out_size) return BMP_ERR_ARGUMENT;
    buf.data = pixels;
    buf.size = 0U;
    buf.capacity = pixel_capacity;

    for (y = 0; y < height; ++y) {
        bmp_u32 src_y = height - 1U - y;
        rc = bmp_encode_rle8_row(indices + src_y * stride, width, &buf);
        if (rc != BMP_OK) return rc;
    }
    rc = bmp_buf_append_byte(&buf, 0U);
    if (rc != BMP_OK) return rc;
    rc = bmp_buf_append_byte(&buf, 1U);
    if (rc != BMP_OK) return rc;

    *out_size = buf.size;
    return BMP_OK;
}

static int bmp_pack_nibbles(const bmp_u8 *src, bmp_u32 count, bmp_buf *buf)
{
    bmp_u32 i;
    int rc;
    for (i = 0; i < count; i += 2U) {
        bmp_u8 hi = (bmp_u8)(src[i] & 0x0FU);
        bmp_u8 lo = (bmp_u8)(((i + 1U) < count) ? (src[i + 1U] & 0x0FU) : 0U);
        rc = bmp_buf_append_byte(buf, (bmp_u8)((hi << 4) | lo));
        if (rc != BMP_OK) return rc;
    }
    return BMP_OK;
}

static int bmp_encode_rle4_row(const bmp_u8 *row, bmp_u32 width, bmp_buf *buf)
{
    bmp_u32 x = 0;
    int rc;

    while (x < width) {
        bmp_u32 run = bmp_same_run_len(row, width, x, 255U);
        if (run >= 3U) {
            bmp_u8 idx = (bmp_u8)(row[x] & 0x0FU);
            rc = bmp_buf_append_byte(buf, (bmp_u8)run);
            if (rc != BMP_OK) return rc;
            rc = bmp_buf_append_byte(buf, (bmp_u8)((idx << 4) | idx));
            if (rc != BMP_OK) return rc;
            x += run;
        } else {
            bmp_u32 start = x;
            bmp_u32 literal = 0U;
            while (x < width) {
                run = bmp_same_run_len(row, width, x, 255U);
                if (run >= 3U) break;
                ++x;
                ++literal;
                if (literal == 255U) break;
            }
            if (literal <= 2U) {
                bmp_u32 k;
                for (k = 0; k < literal; ++k) {
                    bmp_u8 idx = (bmp_u8)(row[start + k] & 0x0FU);
                    rc = bmp_buf_append_byte(buf, 1U);
                    if (rc != BMP_OK) return rc;
                    rc = bmp_buf_append_byte(buf, (bmp_u8)((idx << 4) | idx));
                    if (rc != BMP_OK) return rc;
                }
            } else {
                bmp_u32 bytes = (literal + 1U) / 2U;
                rc = bmp_buf_append_byte(buf, 0U);
                if (rc != BMP_OK) return rc;
                rc = bmp_buf_append_byte(buf, (bmp_u8)literal);
                if (rc != BMP_OK) return rc;
                rc = bmp_pack_nibbles(row + start, literal, buf);
                if (rc != BMP_OK) return rc;
                if ((bytes & 1U) != 0U) {
                    rc = bmp_buf_append_byte(buf, 0U);
                    if (rc != BMP_OK) return rc;
                }
            }
        }
    }

    rc = bmp_buf_append_byte(buf, 0U);
    if (rc != BMP_OK) return rc;
    rc = bmp_buf_append_byte(buf, 0U);
    if (rc != BMP_OK) return rc;
    return BMP_OK;
}

static int bmp_encode_rle4(const bmp_u8 *indices, bmp_u32 width, bmp_u32 height,
                           bmp_u32 stride, bmp_u8 *pixels, bmp_u32 pixel_capacity,
                           bmp_u32 *out_size)
{
    bmp_buf buf;
    bmp_u32 y;
    int rc;

    if (!pixels || !out_size) return BMP_ERR_ARGUMENT;
    buf.data = pixels;
    buf.size = 0U;
    buf.capacity = pixel_capacity;

    for (y = 0; y < height; ++y) {
        bmp_u32 src_y = height - 1U - y;
        rc = bmp_encode_rle4_row(indices + src_y * stride, width, &buf);
        if (rc != BMP_OK) return rc;
    }
    rc = bmp_buf_append_byte(&buf, 0U);
    if (rc != BMP_OK) return rc;
    rc = bmp_buf_append_byte(&buf, 1U);
    if (rc != BMP_OK) return rc;

    *out_size = buf.size;
    return BMP_OK;
}

static int bmp_encode_uncompressed_rgba32(const bmp_u8 *rgba, bmp_u32 width, bmp_u32 height,
                                          bmp_u32 stride, const bmp_encode_plan *plan,
                                          bmp_u8 *pixels, bmp_u32 pixel_capacity,
                                          bmp_u32 *out_size)
{
    bmp_u32 row_stride;
    bmp_u32 y;
    int rc;

    if (!pixels || !out_size) return BMP_ERR_ARGUMENT;
    rc = bmp_calc_image_size(width, height, plan->bpp, out_size);
    if (rc != BMP_OK) return rc;
    if (pixel_capacity < *out_size) return BMP_ERR_BUFFER_TOO_SMALL;
    memset(pixels, 0, *out_size);

    rc = bmp_calc_row_stride(width, plan->bpp, &row_stride);
    if (rc != BMP_OK) return rc;

    for (y = 0; y < height; ++y) {
        bmp_u32 src_y = plan->top_down ? y : (height - 1U - y);
        const bmp_u8 *src_row = rgba + src_y * stride;
        bmp_u8 *dst_row = pixels + y * row_stride;
        bmp_u32 x;
        switch (plan->bpp) {
        case 16:
            for (x = 0; x < width; ++x) {
                const bmp_u8 *px = src_row + x * 4U;
                bmp_u32 v = bmp_pack_channel(px[0], plan->red_mask) |
                            bmp_pack_channel(px[1], plan->green_mask) |
                            bmp_pack_channel(px[2], plan->blue_mask) |
                            bmp_pack_channel(px[3], plan->alpha_mask);
                dst_row[x * 2U + 0U] = (bmp_u8)(v & 0xFFU);
                dst_row[x * 2U + 1U] = (bmp_u8)((v >> 8) & 0xFFU);
            }
            break;
        case 24:
            for (x = 0; x < width; ++x) {
                const bmp_u8 *px = src_row + x * 4U;
                dst_row[x * 3U + 0U] = px[2];
                dst_row[x * 3U + 1U] = px[1];
                dst_row[x * 3U + 2U] = px[0];
            }
            break;
        case 32:
            for (x = 0; x < width; ++x) {
                const bmp_u8 *px = src_row + x * 4U;
                bmp_u32 v = bmp_pack_channel(px[0], plan->red_mask) |
                            bmp_pack_channel(px[1], plan->green_mask) |
                            bmp_pack_channel(px[2], plan->blue_mask) |
                            bmp_pack_channel(px[3], plan->alpha_mask);
                dst_row[x * 4U + 0U] = (bmp_u8)(v & 0xFFU);
                dst_row[x * 4U + 1U] = (bmp_u8)((v >> 8) & 0xFFU);
                dst_row[x * 4U + 2U] = (bmp_u8)((v >> 16) & 0xFFU);
                dst_row[x * 4U + 3U] = (bmp_u8)((v >> 24) & 0xFFU);
            }
            break;
        default:
            return BMP_ERR_UNSUPPORTED;
        }
    }

    return BMP_OK;
}

static int bmp_plan_rgba32(const bmp_encode_options *opt, bmp_u32 width, bmp_u32 height,
                           bmp_encode_plan *plan)
{
    bmp_u32 fmt;
    bmp_u32 header_size;
    bmp_u32 comp_mode;
    int rc;

    if (!opt || !plan) return BMP_ERR_ARGUMENT;
    memset(plan, 0, sizeof(*plan));

    fmt = opt->format;
    if (fmt == 0U) fmt = BMP_ENC_FMT_BGR24;
    comp_mode = opt->compression;

    switch (fmt) {
    case BMP_ENC_FMT_RGB555:
        plan->bpp = 16U;
        plan->file_compression = BMP_COMP_RGB;
        plan->red_mask   = 0x7C00U;
        plan->green_mask = 0x03E0U;
        plan->blue_mask  = 0x001FU;
        plan->alpha_mask = 0U;
        break;
    case BMP_ENC_FMT_RGB565:
        plan->bpp = 16U;
        plan->file_compression = BMP_COMP_BITFIELDS;
        plan->red_mask   = 0xF800U;
        plan->green_mask = 0x07E0U;
        plan->blue_mask  = 0x001FU;
        plan->alpha_mask = 0U;
        break;
    case BMP_ENC_FMT_BGR24:
        plan->bpp = 24U;
        plan->file_compression = BMP_COMP_RGB;
        break;
    case BMP_ENC_FMT_BGRX32:
        plan->bpp = 32U;
        plan->file_compression = BMP_COMP_RGB;
        plan->red_mask   = 0x00FF0000U;
        plan->green_mask = 0x0000FF00U;
        plan->blue_mask  = 0x000000FFU;
        plan->alpha_mask = 0U;
        break;
    case BMP_ENC_FMT_BGRA32:
        plan->bpp = 32U;
        plan->file_compression = BMP_COMP_BITFIELDS;
        plan->red_mask   = 0x00FF0000U;
        plan->green_mask = 0x0000FF00U;
        plan->blue_mask  = 0x000000FFU;
        plan->alpha_mask = 0xFF000000U;
        break;
    case BMP_ENC_FMT_16_BITFIELDS:
        plan->bpp = 16U;
        plan->file_compression = (opt->alpha_mask != 0U) ? BMP_COMP_ALPHABITFIELDS : BMP_COMP_BITFIELDS;
        plan->red_mask   = opt->red_mask;
        plan->green_mask = opt->green_mask;
        plan->blue_mask  = opt->blue_mask;
        plan->alpha_mask = opt->alpha_mask;
        break;
    case BMP_ENC_FMT_32_BITFIELDS:
        plan->bpp = 32U;
        plan->file_compression = (opt->alpha_mask != 0U) ? BMP_COMP_ALPHABITFIELDS : BMP_COMP_BITFIELDS;
        plan->red_mask   = opt->red_mask;
        plan->green_mask = opt->green_mask;
        plan->blue_mask  = opt->blue_mask;
        plan->alpha_mask = opt->alpha_mask;
        break;
    default:
        return BMP_ERR_UNSUPPORTED;
    }

    if (comp_mode != BMP_ENC_COMP_AUTO) {
        switch (comp_mode) {
        case BMP_ENC_COMP_RGB:
            if (fmt == BMP_ENC_FMT_RGB565 || fmt == BMP_ENC_FMT_16_BITFIELDS || fmt == BMP_ENC_FMT_32_BITFIELDS || fmt == BMP_ENC_FMT_BGRA32)
                return BMP_ERR_UNSUPPORTED;
            plan->file_compression = BMP_COMP_RGB;
            break;
        case BMP_ENC_COMP_BITFIELDS:
            if (!(plan->bpp == 16U || plan->bpp == 32U)) return BMP_ERR_UNSUPPORTED;
            plan->file_compression = BMP_COMP_BITFIELDS;
            break;
        case BMP_ENC_COMP_ALPHABITFIELDS:
            if (!(plan->bpp == 16U || plan->bpp == 32U) || plan->alpha_mask == 0U) return BMP_ERR_UNSUPPORTED;
            plan->file_compression = BMP_COMP_ALPHABITFIELDS;
            break;
        default:
            return BMP_ERR_UNSUPPORTED;
        }
    }

    if ((plan->file_compression == BMP_COMP_BITFIELDS || plan->file_compression == BMP_COMP_ALPHABITFIELDS) &&
        (plan->red_mask | plan->green_mask | plan->blue_mask) == 0U) {
        return BMP_ERR_MASKS;
    }

    if ((plan->red_mask | plan->green_mask | plan->blue_mask) != 0U) {
        rc = bmp_masks_valid(plan->red_mask, plan->green_mask, plan->blue_mask, plan->alpha_mask, plan->bpp);
        if (rc != BMP_OK) return rc;
    }

    header_size = opt->header_size;
    if (header_size == 0U) {
        if (opt->icc_profile_size != 0U || plan->alpha_mask != 0U) header_size = 124U;
        else header_size = 40U;
    }

    switch (header_size) {
    case 12U:
        if (plan->bpp != 24U || plan->file_compression != BMP_COMP_RGB || opt->top_down || opt->icc_profile_size != 0U) {
            return BMP_ERR_UNSUPPORTED;
        }
        if (width > 65535U || height > 65535U) return BMP_ERR_DIMENSIONS;
        plan->is_core = 1;
        break;
    case 40U:
    case 52U:
    case 56U:
    case 108U:
    case 124U:
        break;
    default:
        return BMP_ERR_UNSUPPORTED;
    }

    if (header_size < 52U && plan->file_compression != BMP_COMP_RGB) {
        plan->external_mask_bytes = (plan->alpha_mask != 0U && plan->file_compression == BMP_COMP_ALPHABITFIELDS) ? 16U : 12U;
    }
    if ((header_size == 40U || header_size == 52U) && plan->alpha_mask != 0U) {
        if (header_size == 40U) {
            if (plan->file_compression != BMP_COMP_ALPHABITFIELDS) {
                header_size = 124U;
                plan->external_mask_bytes = 0U;
            }
        } else {
            header_size = 56U;
        }
    }
    if (opt->icc_profile_size != 0U && header_size < 124U) header_size = 124U;
    plan->header_size = header_size;
    plan->top_down = opt->top_down ? 1 : 0;
    if (plan->top_down && plan->is_core) return BMP_ERR_UNSUPPORTED;

    if (plan->file_compression == BMP_COMP_RGB) {
        if (plan->bpp == 16U && fmt != BMP_ENC_FMT_RGB555) {
            return BMP_ERR_UNSUPPORTED;
        }
        if (plan->bpp == 32U && fmt != BMP_ENC_FMT_BGRX32) {
            return BMP_ERR_UNSUPPORTED;
        }
        if (plan->bpp == 24U) {
            plan->red_mask = 0U;
            plan->green_mask = 0U;
            plan->blue_mask = 0U;
            plan->alpha_mask = 0U;
        }
    }

    plan->profile_size = opt->icc_profile_size;
    if (header_size >= 108U) {
        plan->color_space_type = (opt->color_space_type != 0U)
            ? opt->color_space_type
            : ((opt->icc_profile_size != 0U) ? BMP_CSTYPE_PROFILE_EMBEDDED : BMP_CSTYPE_SRGB);
    }
    plan->rendering_intent = opt->rendering_intent;

    return BMP_OK;
}

static int bmp_plan_indexed(const bmp_encode_options *opt, bmp_u32 width, bmp_u32 height,
                            bmp_u32 palette_size, bmp_encode_plan *plan)
{
    bmp_u32 fmt;
    bmp_u32 header_size;
    if (!opt || !plan) return BMP_ERR_ARGUMENT;
    memset(plan, 0, sizeof(*plan));

    fmt = opt->format;
    if (fmt == 0U) fmt = BMP_ENC_FMT_INDEXED8;
    switch (fmt) {
    case BMP_ENC_FMT_INDEXED1:
        plan->bpp = 1U;
        if (palette_size == 0U || palette_size > 2U) return BMP_ERR_PALETTE;
        break;
    case BMP_ENC_FMT_INDEXED4:
        plan->bpp = 4U;
        if (palette_size == 0U || palette_size > 16U) return BMP_ERR_PALETTE;
        break;
    case BMP_ENC_FMT_INDEXED8:
        plan->bpp = 8U;
        if (palette_size == 0U || palette_size > 256U) return BMP_ERR_PALETTE;
        break;
    default:
        return BMP_ERR_UNSUPPORTED;
    }

    switch (opt->compression) {
    case BMP_ENC_COMP_AUTO:
    case BMP_ENC_COMP_RGB:
        plan->file_compression = BMP_COMP_RGB;
        break;
    case BMP_ENC_COMP_RLE4:
        if (plan->bpp != 4U) return BMP_ERR_UNSUPPORTED;
        plan->file_compression = BMP_COMP_RLE4;
        break;
    case BMP_ENC_COMP_RLE8:
        if (plan->bpp != 8U) return BMP_ERR_UNSUPPORTED;
        plan->file_compression = BMP_COMP_RLE8;
        break;
    default:
        return BMP_ERR_UNSUPPORTED;
    }

    header_size = opt->header_size;
    if (header_size == 0U) {
        header_size = (opt->icc_profile_size != 0U) ? 124U : 40U;
    }

    switch (header_size) {
    case 12U:
        if (plan->file_compression != BMP_COMP_RGB || opt->top_down || opt->icc_profile_size != 0U) {
            return BMP_ERR_UNSUPPORTED;
        }
        if (width > 65535U || height > 65535U) return BMP_ERR_DIMENSIONS;
        plan->is_core = 1;
        plan->palette_entries = (bmp_u32)(1U << plan->bpp);
        plan->palette_entry_size = 3U;
        break;
    case 40U:
    case 108U:
    case 124U:
        if ((opt->top_down != 0) && plan->file_compression != BMP_COMP_RGB) return BMP_ERR_UNSUPPORTED;
        plan->palette_entries = palette_size;
        plan->palette_entry_size = 4U;
        break;
    default:
        return BMP_ERR_UNSUPPORTED;
    }

    plan->header_size = header_size;
    plan->top_down = opt->top_down ? 1 : 0;
    plan->profile_size = opt->icc_profile_size;
    if (header_size >= 108U) {
        plan->color_space_type = (opt->color_space_type != 0U)
            ? opt->color_space_type
            : ((opt->icc_profile_size != 0U) ? BMP_CSTYPE_PROFILE_EMBEDDED : BMP_CSTYPE_SRGB);
    }
    plan->rendering_intent = opt->rendering_intent;
    return BMP_OK;
}

static int bmp_finalize_plan(const bmp_encode_options *opt, bmp_encode_plan *plan)
{
    bmp_u32 offset;
    bmp_u32 profile_offset;

    if (!opt || !plan) return BMP_ERR_ARGUMENT;

    if (!bmp_u32_mul_checked(plan->palette_entries, plan->palette_entry_size,
                             &plan->palette_bytes)) {
        return BMP_ERR_OVERFLOW;
    }

    if (!bmp_u32_add_checked(14U, plan->header_size, &offset) ||
        !bmp_u32_add_checked(offset, plan->external_mask_bytes, &offset) ||
        !bmp_u32_add_checked(offset, plan->palette_bytes, &offset)) {
        return BMP_ERR_OVERFLOW;
    }
    plan->pixel_offset = offset;

    if (!bmp_u32_add_checked(offset, plan->pixel_data_size, &plan->file_size) ||
        !bmp_u32_add_checked(plan->file_size, plan->profile_size, &plan->file_size)) {
        return BMP_ERR_OVERFLOW;
    }

    if (plan->header_size >= 124U && plan->profile_size != 0U) {
        if (!bmp_u32_add_checked(plan->pixel_offset, plan->pixel_data_size, &profile_offset) ||
            profile_offset < 14U) {
            return BMP_ERR_OVERFLOW;
        }
        plan->profile_data_offset = profile_offset - 14U;
    } else {
        plan->profile_data_offset = 0U;
    }

    (void)opt;
    return BMP_OK;
}

static void bmp_write_file_header(bmp_u8 *dst, const bmp_encode_plan *plan)
{
    dst[0] = 'B';
    dst[1] = 'M';
    bmp_write_u32le(dst + 2, plan->file_size);
    bmp_write_u16le(dst + 6, 0U);
    bmp_write_u16le(dst + 8, 0U);
    bmp_write_u32le(dst + 10, plan->pixel_offset);
}

static void bmp_write_dib_header(bmp_u8 *dst, bmp_u32 width, bmp_u32 height,
                                 const bmp_encode_options *opt, const bmp_encode_plan *plan)
{
    bmp_s32 signed_height = plan->top_down ? -(bmp_s32)height : (bmp_s32)height;
    memset(dst, 0, plan->header_size);

    if (plan->is_core) {
        bmp_write_u32le(dst + 0, 12U);
        bmp_write_u16le(dst + 4, (bmp_u16)width);
        bmp_write_u16le(dst + 6, (bmp_u16)height);
        bmp_write_u16le(dst + 8, 1U);
        bmp_write_u16le(dst + 10, plan->bpp);
        return;
    }

    bmp_write_u32le(dst + 0, plan->header_size);
    bmp_write_s32le(dst + 4, (bmp_s32)width);
    bmp_write_s32le(dst + 8, signed_height);
    bmp_write_u16le(dst + 12, 1U);
    bmp_write_u16le(dst + 14, plan->bpp);
    bmp_write_u32le(dst + 16, plan->file_compression);
    bmp_write_u32le(dst + 20, plan->pixel_data_size);
    bmp_write_s32le(dst + 24, opt->ppm_x);
    bmp_write_s32le(dst + 28, opt->ppm_y);
    bmp_write_u32le(dst + 32, plan->palette_entries);
    bmp_write_u32le(dst + 36, opt->colors_important);

    if (plan->header_size >= 52U) {
        bmp_write_u32le(dst + 40, plan->red_mask);
        bmp_write_u32le(dst + 44, plan->green_mask);
        bmp_write_u32le(dst + 48, plan->blue_mask);
    }
    if (plan->header_size >= 56U) {
        bmp_write_u32le(dst + 52, plan->alpha_mask);
    }
    if (plan->header_size >= 108U) {
        bmp_write_u32le(dst + 56, plan->color_space_type);
        bmp_write_u32le(dst + 92, 0U);
        bmp_write_u32le(dst + 96, 0U);
        bmp_write_u32le(dst + 100, 0U);
        if (plan->header_size >= 124U) {
            bmp_write_u32le(dst + 108, plan->rendering_intent);
            bmp_write_u32le(dst + 112, plan->profile_data_offset);
            bmp_write_u32le(dst + 116, plan->profile_size);
            bmp_write_u32le(dst + 120, 0U);
        }
    }
}

static void bmp_write_external_masks(bmp_u8 *dst, const bmp_encode_plan *plan)
{
    if (plan->external_mask_bytes >= 12U) {
        bmp_write_u32le(dst + 0, plan->red_mask);
        bmp_write_u32le(dst + 4, plan->green_mask);
        bmp_write_u32le(dst + 8, plan->blue_mask);
    }
    if (plan->external_mask_bytes >= 16U) {
        bmp_write_u32le(dst + 12, plan->alpha_mask);
    }
}

void bmp_encode_options_default(bmp_encode_options *opt)
{
    if (!opt) return;
    memset(opt, 0, sizeof(*opt));
    opt->format = BMP_ENC_FMT_BGR24;
    opt->compression = BMP_ENC_COMP_AUTO;
    opt->ppm_x = 2835;
    opt->ppm_y = 2835;
}

int bmp_encode_rgba32_workspace_size(bmp_u32 width,
                                      bmp_u32 height,
                                      const bmp_encode_options *opt,
                                      bmp_u32 *out_size)
{
    bmp_encode_options local_opt;
    bmp_encode_plan plan;
    int rc;

    if (!out_size || width == 0U || height == 0U) return BMP_ERR_ARGUMENT;
    if (!opt) {
        bmp_encode_options_default(&local_opt);
        opt = &local_opt;
    }
    rc = bmp_plan_rgba32(opt, width, height, &plan);
    if (rc != BMP_OK) return rc;
    return bmp_calc_image_size(width, height, plan.bpp, out_size);
}

int bmp_encode_indexed_workspace_size_bound(bmp_u32 width,
                                             bmp_u32 height,
                                             bmp_u32 palette_size,
                                             const bmp_encode_options *opt,
                                             bmp_u32 *out_size)
{
    bmp_encode_options local_opt;
    bmp_encode_plan plan;
    bmp_u32 per_row;
    bmp_u32 total;
    int rc;

    if (!out_size || width == 0U || height == 0U) return BMP_ERR_ARGUMENT;
    if (!opt) {
        bmp_encode_options_default(&local_opt);
        local_opt.format = BMP_ENC_FMT_INDEXED8;
        opt = &local_opt;
    }
    rc = bmp_plan_indexed(opt, width, height, palette_size, &plan);
    if (rc != BMP_OK) return rc;

    if (plan.file_compression == BMP_COMP_RGB) {
        return bmp_calc_image_size(width, height, plan.bpp, out_size);
    }

    if (!bmp_u32_mul_checked(width, 2U, &per_row) ||
        !bmp_u32_add_checked(per_row, 2U, &per_row) ||
        !bmp_u32_mul_checked(per_row, height, &total) ||
        !bmp_u32_add_checked(total, 2U, &total)) {
        return BMP_ERR_OVERFLOW;
    }
    *out_size = total;
    return BMP_OK;
}

int bmp_encode_rgba32_into(const bmp_u8 *rgba,
                           bmp_u32 width,
                           bmp_u32 height,
                           bmp_u32 stride,
                           const bmp_encode_options *opt,
                           bmp_u8 *out_bmp,
                           bmp_u32 out_capacity,
                           bmp_u32 *out_size,
                           bmp_u8 *workspace,
                           bmp_u32 workspace_size)
{
    bmp_encode_options local_opt;
    bmp_encode_plan plan;
    bmp_u8 *dst;
    int rc;

    if (!rgba || !out_bmp || !out_size || !workspace || width == 0U || height == 0U) {
        return BMP_ERR_ARGUMENT;
    }
    if (opt && opt->icc_profile_size != 0U && !opt->icc_profile_data) return BMP_ERR_ARGUMENT;
    if (!opt) {
        bmp_encode_options_default(&local_opt);
        opt = &local_opt;
    }

    rc = bmp_validate_input_stride_rgba32(width, stride);
    if (rc != BMP_OK) return rc;
    rc = bmp_plan_rgba32(opt, width, height, &plan);
    if (rc != BMP_OK) return rc;

    rc = bmp_encode_uncompressed_rgba32(rgba, width, height, stride, &plan,
                                        workspace, workspace_size,
                                        &plan.pixel_data_size);
    if (rc != BMP_OK) return rc;

    rc = bmp_finalize_plan(opt, &plan);
    if (rc != BMP_OK) return rc;
    *out_size = plan.file_size;
    if (out_capacity < plan.file_size) return BMP_ERR_BUFFER_TOO_SMALL;

    memset(out_bmp, 0, plan.file_size);
    bmp_write_file_header(out_bmp, &plan);
    dst = out_bmp + 14U;
    bmp_write_dib_header(dst, width, height, opt, &plan);
    dst += plan.header_size;

    if (plan.external_mask_bytes != 0U) {
        bmp_write_external_masks(dst, &plan);
    }

    memcpy(out_bmp + plan.pixel_offset, workspace, plan.pixel_data_size);
    if (plan.profile_size != 0U) {
        memcpy(out_bmp + plan.pixel_offset + plan.pixel_data_size,
               opt->icc_profile_data,
               plan.profile_size);
    }
    return BMP_OK;
}

int bmp_encode_indexed_into(const bmp_u8 *indices,
                            bmp_u32 width,
                            bmp_u32 height,
                            bmp_u32 stride,
                            const bmp_palette_entry *palette,
                            bmp_u32 palette_size,
                            const bmp_encode_options *opt,
                            bmp_u8 *out_bmp,
                            bmp_u32 out_capacity,
                            bmp_u32 *out_size,
                            bmp_u8 *workspace,
                            bmp_u32 workspace_size)
{
    bmp_encode_options local_opt;
    bmp_encode_plan plan;
    bmp_u8 *dst;
    int rc;

    if (!indices || !palette || !out_bmp || !out_size || !workspace ||
        width == 0U || height == 0U) return BMP_ERR_ARGUMENT;
    if (opt && opt->icc_profile_size != 0U && !opt->icc_profile_data) return BMP_ERR_ARGUMENT;
    if (!opt) {
        bmp_encode_options_default(&local_opt);
        local_opt.format = BMP_ENC_FMT_INDEXED8;
        opt = &local_opt;
    }

    rc = bmp_validate_input_stride_indexed(width, stride);
    if (rc != BMP_OK) return rc;
    rc = bmp_plan_indexed(opt, width, height, palette_size, &plan);
    if (rc != BMP_OK) return rc;
    rc = bmp_validate_index_range(indices, width, height, stride, palette_size);
    if (rc != BMP_OK) return rc;

    switch (plan.file_compression) {
    case BMP_COMP_RGB:
        rc = bmp_encode_uncompressed_indexed(indices, width, height, stride,
                                             plan.bpp, plan.top_down,
                                             workspace, workspace_size,
                                             &plan.pixel_data_size);
        break;
    case BMP_COMP_RLE4:
        rc = bmp_encode_rle4(indices, width, height, stride,
                             workspace, workspace_size,
                             &plan.pixel_data_size);
        break;
    case BMP_COMP_RLE8:
        rc = bmp_encode_rle8(indices, width, height, stride,
                             workspace, workspace_size,
                             &plan.pixel_data_size);
        break;
    default:
        rc = BMP_ERR_UNSUPPORTED;
        break;
    }
    if (rc != BMP_OK) return rc;

    rc = bmp_finalize_plan(opt, &plan);
    if (rc != BMP_OK) return rc;
    *out_size = plan.file_size;
    if (out_capacity < plan.file_size) return BMP_ERR_BUFFER_TOO_SMALL;

    memset(out_bmp, 0, plan.file_size);
    bmp_write_file_header(out_bmp, &plan);
    dst = out_bmp + 14U;
    bmp_write_dib_header(dst, width, height, opt, &plan);
    dst += plan.header_size;

    if (plan.palette_bytes != 0U) {
        if (plan.is_core) {
            bmp_copy_palette_core(dst, palette, palette_size);
            if (plan.palette_entries > palette_size) {
                memset(dst + palette_size * 3U, 0,
                       (plan.palette_entries - palette_size) * 3U);
            }
        } else {
            bmp_copy_palette_quad(dst, palette, palette_size);
        }
    }

    memcpy(out_bmp + plan.pixel_offset, workspace, plan.pixel_data_size);
    if (plan.profile_size != 0U) {
        memcpy(out_bmp + plan.pixel_offset + plan.pixel_data_size,
               opt->icc_profile_data,
               plan.profile_size);
    }
    return BMP_OK;
}
