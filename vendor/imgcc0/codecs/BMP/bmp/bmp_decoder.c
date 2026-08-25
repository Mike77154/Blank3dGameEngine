#include "bmp_decoder.h"

#define BMP_MATH32_ENABLE_MUL 1
#define BMP_MATH32_ENABLE_SCALE_TO_U8 1
#include "bmp_math32.h"

#include <string.h>

static int bmp_check_decode_limits(const bmp_image *img, const bmp_limits *limits);
static int bmp_min_rgba_stride(const bmp_image *img, bmp_u32 *out_stride);

static void bmp_set_px(bmp_u8 *row, bmp_u32 x,
                       bmp_u8 r, bmp_u8 g, bmp_u8 b, bmp_u8 a)
{
    bmp_u8 *p = row + x * 4U;
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = a;
}

static void bmp_fill_rgba32(bmp_u8 *out, bmp_u32 width, bmp_u32 height, bmp_u32 stride,
                            bmp_u8 r, bmp_u8 g, bmp_u8 b, bmp_u8 a)
{
    bmp_u32 y, x;
    for (y = 0; y < height; ++y) {
        bmp_u8 *row = out + y * stride;
        for (x = 0; x < width; ++x) {
            bmp_set_px(row, x, r, g, b, a);
        }
    }
}

static bmp_u8 *bmp_out_row(bmp_u8 *out, bmp_u32 stride, bmp_u32 height,
                           bmp_u32 stored_y, int is_top_down)
{
    if (is_top_down) {
        return out + stored_y * stride;
    }
    return out + (height - 1U - stored_y) * stride;
}

static void bmp_get_palette_color(const bmp_image *img, bmp_u32 index,
                                  bmp_u8 *r, bmp_u8 *g, bmp_u8 *b, bmp_u8 *a)
{
    const bmp_metadata *m = &img->meta;
    const bmp_u8 *p;

    *r = *g = *b = 0;
    *a = 255;

    if (!img->palette || index >= m->palette_entries || img->palette_entry_size == 0U) {
        return;
    }

    p = img->palette + index * img->palette_entry_size;
    *b = p[0];
    *g = p[1];
    *r = p[2];
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
    if (m != 0U) {
        return BMP_ERR_MASKS;
    }

    *shift = s;
    *bits  = b;
    return BMP_OK;
}

static bmp_u8 bmp_scale_to_8bit(bmp_u32 value, int bits)
{
    bmp_u32 max_val;
    if (bits <= 0) return 0U;
    if (bits >= 32) max_val = 0xFFFFFFFFU;
    else max_val = (1U << bits) - 1U;
    return bmp_scale_u32_to_u8_exact(value, max_val);
}

static int bmp_mask_is_contiguous(bmp_u32 mask)
{
    bmp_u32 m = mask;
    if (m == 0U) return 1;
    while ((m & 1U) == 0U) m >>= 1;
    while ((m & 1U) == 1U) m >>= 1;
    return (m == 0U);
}

static int bmp_masks_valid(const bmp_metadata *m, bmp_u16 bpp)
{
    bmp_u32 limit;
    bmp_u32 rgb_or;

    if (bpp == 32U) limit = 0xFFFFFFFFU;
    else if (bpp < 32U) limit = (1U << bpp) - 1U;
    else return BMP_ERR_MASKS;

    rgb_or = m->red_mask | m->green_mask | m->blue_mask;
    if (rgb_or == 0U) return BMP_ERR_MASKS;

    if ((m->red_mask & ~limit) != 0U ||
        (m->green_mask & ~limit) != 0U ||
        (m->blue_mask & ~limit) != 0U ||
        (m->alpha_mask & ~limit) != 0U) {
        return BMP_ERR_MASKS;
    }

    if ((m->red_mask & m->green_mask) != 0U ||
        (m->red_mask & m->blue_mask)  != 0U ||
        (m->red_mask & m->alpha_mask) != 0U ||
        (m->green_mask & m->blue_mask)  != 0U ||
        (m->green_mask & m->alpha_mask) != 0U ||
        (m->blue_mask & m->alpha_mask)  != 0U) {
        return BMP_ERR_MASKS;
    }

    if (!bmp_mask_is_contiguous(m->red_mask) ||
        !bmp_mask_is_contiguous(m->green_mask) ||
        !bmp_mask_is_contiguous(m->blue_mask) ||
        (m->alpha_mask != 0U && !bmp_mask_is_contiguous(m->alpha_mask))) {
        return BMP_ERR_MASKS;
    }

    return BMP_OK;
}


int bmp_calc_rgba32_buffer_size(const bmp_image *img,
                                bmp_u32 *out_size)
{
    bmp_u32 stride;
    int rc;

    if (!img || !out_size) return BMP_ERR_ARGUMENT;
    rc = bmp_min_rgba_stride(img, &stride);
    if (rc != BMP_OK) return rc;
    if (!bmp_u32_mul_checked(stride, (bmp_u32)img->meta.height, out_size)) {
        return BMP_ERR_OVERFLOW;
    }
    return BMP_OK;
}

static int bmp_check_decode_limits(const bmp_image *img, const bmp_limits *limits)
{
    bmp_limits resolved;
    bmp_u32 total;
    bmp_u32 stride;
    int rc;

    if (!img) return BMP_ERR_ARGUMENT;

    if (limits) resolved = *limits;
    else bmp_get_global_limits(&resolved);

    rc = bmp_min_rgba_stride(img, &stride);
    if (rc != BMP_OK) return rc;
    if (!bmp_u32_mul_checked(stride, (bmp_u32)img->meta.height, &total)) {
        return BMP_ERR_OVERFLOW;
    }
    if (resolved.max_decoded_bytes != 0U && total > resolved.max_decoded_bytes) {
        return BMP_ERR_LIMITS;
    }
    return BMP_OK;
}

static int bmp_min_rgba_stride(const bmp_image *img, bmp_u32 *out_stride)
{
    if (!img || !out_stride) return BMP_ERR_ARGUMENT;
    if (!bmp_u32_mul_checked((bmp_u32)img->meta.width, 4U, out_stride)) {
        return BMP_ERR_OVERFLOW;
    }
    return BMP_OK;
}

static int bmp_decode_indexed_1bpp(const bmp_image *img, bmp_u8 *out, bmp_u32 stride)
{
    const bmp_metadata *m = &img->meta;
    const bmp_u8 *src = img->pixel_data;
    bmp_u32 width = (bmp_u32)m->width;
    bmp_u32 height = (bmp_u32)m->height;
    bmp_u32 row_bytes;
    bmp_u32 y;
    int rc = bmp_calc_row_stride(width, 1U, &row_bytes);
    if (rc != BMP_OK) return rc;
    if (img->pixel_data_size < row_bytes * height) return BMP_ERR_STREAM;

    for (y = 0; y < height; ++y) {
        bmp_u32 src_y = m->is_top_down ? y : (height - 1U - y);
        const bmp_u8 *src_row = src + src_y * row_bytes;
        bmp_u8 *dst_row = out + y * stride;
        bmp_u32 x;
        for (x = 0; x < width; ++x) {
            bmp_u8 idx = (bmp_u8)((src_row[x >> 3] >> (7U - (x & 7U))) & 1U);
            bmp_u8 r, g, b, a;
            bmp_get_palette_color(img, idx, &r, &g, &b, &a);
            bmp_set_px(dst_row, x, r, g, b, 255);
        }
    }
    return BMP_OK;
}

static int bmp_decode_indexed_2bpp(const bmp_image *img, bmp_u8 *out, bmp_u32 stride)
{
    const bmp_metadata *m = &img->meta;
    const bmp_u8 *src = img->pixel_data;
    bmp_u32 width = (bmp_u32)m->width;
    bmp_u32 height = (bmp_u32)m->height;
    bmp_u32 row_bytes;
    bmp_u32 y;
    int rc = bmp_calc_row_stride(width, 2U, &row_bytes);
    if (rc != BMP_OK) return rc;
    if (img->pixel_data_size < row_bytes * height) return BMP_ERR_STREAM;

    for (y = 0; y < height; ++y) {
        bmp_u32 src_y = m->is_top_down ? y : (height - 1U - y);
        const bmp_u8 *src_row = src + src_y * row_bytes;
        bmp_u8 *dst_row = out + y * stride;
        bmp_u32 x;
        for (x = 0; x < width; ++x) {
            bmp_u8 byte = src_row[x >> 2];
            bmp_u8 shift = (bmp_u8)(6U - ((x & 3U) * 2U));
            bmp_u8 idx = (bmp_u8)((byte >> shift) & 0x03U);
            bmp_u8 r, g, b, a;
            bmp_get_palette_color(img, idx, &r, &g, &b, &a);
            bmp_set_px(dst_row, x, r, g, b, 255);
        }
    }
    return BMP_OK;
}

static int bmp_decode_indexed_4bpp(const bmp_image *img, bmp_u8 *out, bmp_u32 stride)
{
    const bmp_metadata *m = &img->meta;
    const bmp_u8 *src = img->pixel_data;
    bmp_u32 width = (bmp_u32)m->width;
    bmp_u32 height = (bmp_u32)m->height;
    bmp_u32 row_bytes;
    bmp_u32 y;
    int rc = bmp_calc_row_stride(width, 4U, &row_bytes);
    if (rc != BMP_OK) return rc;
    if (img->pixel_data_size < row_bytes * height) return BMP_ERR_STREAM;

    for (y = 0; y < height; ++y) {
        bmp_u32 src_y = m->is_top_down ? y : (height - 1U - y);
        const bmp_u8 *src_row = src + src_y * row_bytes;
        bmp_u8 *dst_row = out + y * stride;
        bmp_u32 x;
        for (x = 0; x < width; ++x) {
            bmp_u8 byte = src_row[x >> 1];
            bmp_u8 idx = (bmp_u8)(((x & 1U) == 0U) ? (byte >> 4) : (byte & 0x0FU));
            bmp_u8 r, g, b, a;
            bmp_get_palette_color(img, idx, &r, &g, &b, &a);
            bmp_set_px(dst_row, x, r, g, b, 255);
        }
    }
    return BMP_OK;
}

static int bmp_decode_indexed_8bpp(const bmp_image *img, bmp_u8 *out, bmp_u32 stride)
{
    const bmp_metadata *m = &img->meta;
    const bmp_u8 *src = img->pixel_data;
    bmp_u32 width = (bmp_u32)m->width;
    bmp_u32 height = (bmp_u32)m->height;
    bmp_u32 row_bytes;
    bmp_u32 y;
    int rc = bmp_calc_row_stride(width, 8U, &row_bytes);
    if (rc != BMP_OK) return rc;
    if (img->pixel_data_size < row_bytes * height) return BMP_ERR_STREAM;

    for (y = 0; y < height; ++y) {
        bmp_u32 src_y = m->is_top_down ? y : (height - 1U - y);
        const bmp_u8 *src_row = src + src_y * row_bytes;
        bmp_u8 *dst_row = out + y * stride;
        bmp_u32 x;
        for (x = 0; x < width; ++x) {
            bmp_u8 r, g, b, a;
            bmp_get_palette_color(img, src_row[x], &r, &g, &b, &a);
            bmp_set_px(dst_row, x, r, g, b, 255);
        }
    }
    return BMP_OK;
}

static int bmp_decode_24bpp(const bmp_image *img, bmp_u8 *out, bmp_u32 stride)
{
    const bmp_metadata *m = &img->meta;
    const bmp_u8 *src = img->pixel_data;
    bmp_u32 width = (bmp_u32)m->width;
    bmp_u32 height = (bmp_u32)m->height;
    bmp_u32 row_bytes;
    bmp_u32 y;
    int rc = bmp_calc_row_stride(width, 24U, &row_bytes);
    if (rc != BMP_OK) return rc;
    if (img->pixel_data_size < row_bytes * height) return BMP_ERR_STREAM;

    for (y = 0; y < height; ++y) {
        bmp_u32 src_y = m->is_top_down ? y : (height - 1U - y);
        const bmp_u8 *src_row = src + src_y * row_bytes;
        bmp_u8 *dst_row = out + y * stride;
        bmp_u32 x;
        for (x = 0; x < width; ++x) {
            const bmp_u8 *p = src_row + x * 3U;
            bmp_set_px(dst_row, x, p[2], p[1], p[0], 255);
        }
    }
    return BMP_OK;
}

static int bmp_decode_bitfields(const bmp_image *img, bmp_u8 *out, bmp_u32 stride, bmp_u16 bpp)
{
    const bmp_metadata *m = &img->meta;
    bmp_u32 width = (bmp_u32)m->width;
    bmp_u32 height = (bmp_u32)m->height;
    bmp_u32 row_bytes;
    bmp_u32 y;
    int rs, gs, bs, as, rb, gb, bb, ab;
    int rc;

    rc = bmp_masks_valid(m, bpp);
    if (rc != BMP_OK) return rc;

    rc = bmp_calc_mask_shift(m->red_mask,   &rs, &rb);
    if (rc != BMP_OK) return rc;
    rc = bmp_calc_mask_shift(m->green_mask, &gs, &gb);
    if (rc != BMP_OK) return rc;
    rc = bmp_calc_mask_shift(m->blue_mask,  &bs, &bb);
    if (rc != BMP_OK) return rc;
    rc = bmp_calc_mask_shift(m->alpha_mask, &as, &ab);
    if (rc != BMP_OK) return rc;

    rc = bmp_calc_row_stride(width, bpp, &row_bytes);
    if (rc != BMP_OK) return rc;
    if (img->pixel_data_size < row_bytes * height) return BMP_ERR_STREAM;

    for (y = 0; y < height; ++y) {
        bmp_u32 src_y = m->is_top_down ? y : (height - 1U - y);
        const bmp_u8 *src_row = img->pixel_data + src_y * row_bytes;
        bmp_u8 *dst_row = out + y * stride;
        bmp_u32 x;
        for (x = 0; x < width; ++x) {
            bmp_u32 val;
            bmp_u32 rv, gv, bv, av;
            bmp_u8 r, g, b, a;
            if (bpp == 16U) {
                const bmp_u8 *p = src_row + x * 2U;
                val = (bmp_u32)p[0] | ((bmp_u32)p[1] << 8);
            } else {
                const bmp_u8 *p = src_row + x * 4U;
                val = (bmp_u32)p[0] |
                      ((bmp_u32)p[1] << 8) |
                      ((bmp_u32)p[2] << 16) |
                      ((bmp_u32)p[3] << 24);
            }

            rv = (val & m->red_mask)   >> rs;
            gv = (val & m->green_mask) >> gs;
            bv = (val & m->blue_mask)  >> bs;
            av = (m->alpha_mask != 0U) ? ((val & m->alpha_mask) >> as) : 0U;

            r = bmp_scale_to_8bit(rv, rb);
            g = bmp_scale_to_8bit(gv, gb);
            b = bmp_scale_to_8bit(bv, bb);
            a = (m->alpha_mask != 0U) ? bmp_scale_to_8bit(av, ab) : 255;
            bmp_set_px(dst_row, x, r, g, b, a);
        }
    }

    return BMP_OK;
}

static int bmp_decode_16bpp(const bmp_image *img, bmp_u8 *out, bmp_u32 stride)
{
    bmp_image temp = *img;

    if ((img->meta.red_mask | img->meta.green_mask | img->meta.blue_mask) != 0U) {
        return bmp_decode_bitfields(img, out, stride, 16U);
    }

    temp.meta.red_mask   = 0x7C00U;
    temp.meta.green_mask = 0x03E0U;
    temp.meta.blue_mask  = 0x001FU;
    temp.meta.alpha_mask = 0U;
    return bmp_decode_bitfields(&temp, out, stride, 16U);
}

static int bmp_decode_32bpp(const bmp_image *img, bmp_u8 *out, bmp_u32 stride)
{
    bmp_image temp = *img;

    if ((img->meta.red_mask | img->meta.green_mask | img->meta.blue_mask) != 0U) {
        return bmp_decode_bitfields(img, out, stride, 32U);
    }

    temp.meta.red_mask   = 0x00FF0000U;
    temp.meta.green_mask = 0x0000FF00U;
    temp.meta.blue_mask  = 0x000000FFU;
    temp.meta.alpha_mask = 0U;
    return bmp_decode_bitfields(&temp, out, stride, 32U);
}

static int bmp_decode_rle8(const bmp_image *img, bmp_u8 *out, bmp_u32 stride)
{
    const bmp_metadata *m = &img->meta;
    bmp_u32 width = (bmp_u32)m->width;
    bmp_u32 height = (bmp_u32)m->height;
    bmp_u32 x = 0;
    bmp_u32 y = 0;
    bmp_u32 i = 0;

    bmp_fill_rgba32(out, width, height, stride, 0, 0, 0, 255);

    while (i < img->pixel_data_size) {
        bmp_u8 count = img->pixel_data[i++];
        if (count != 0U) {
            bmp_u8 value;
            bmp_u8 r, g, b, a;
            bmp_u32 k;
            if (i >= img->pixel_data_size) return BMP_ERR_STREAM;
            if (y >= height || x > width) return BMP_ERR_FORMAT;
            value = img->pixel_data[i++];
            bmp_get_palette_color(img, value, &r, &g, &b, &a);
            if (count > width - x) return BMP_ERR_FORMAT;
            for (k = 0; k < count; ++k) {
                bmp_u8 *row = bmp_out_row(out, stride, height, y, m->is_top_down);
                bmp_set_px(row, x + k, r, g, b, 255);
            }
            x += count;
        } else {
            bmp_u8 cmd;
            if (i >= img->pixel_data_size) return BMP_ERR_STREAM;
            cmd = img->pixel_data[i++];
            if (cmd == 0U) {
                x = 0;
                ++y;
                if (y > height) return BMP_ERR_FORMAT;
            } else if (cmd == 1U) {
                return BMP_OK;
            } else if (cmd == 2U) {
                bmp_u8 dx, dy;
                if (i + 1U >= img->pixel_data_size) return BMP_ERR_STREAM;
                dx = img->pixel_data[i++];
                dy = img->pixel_data[i++];
                if (dx > width - x) return BMP_ERR_FORMAT;
                if (dy > height - y) return BMP_ERR_FORMAT;
                x += dx;
                y += dy;
                if (x > width || y > height) return BMP_ERR_FORMAT;
            } else {
                bmp_u32 num = cmd;
                bmp_u32 k;
                if (i + num > img->pixel_data_size) return BMP_ERR_STREAM;
                if (y >= height || num > width - x) return BMP_ERR_FORMAT;
                for (k = 0; k < num; ++k) {
                    bmp_u8 r, g, b, a;
                    bmp_u8 *row = bmp_out_row(out, stride, height, y, m->is_top_down);
                    bmp_get_palette_color(img, img->pixel_data[i + k], &r, &g, &b, &a);
                    bmp_set_px(row, x + k, r, g, b, 255);
                }
                i += num;
                if ((num & 1U) != 0U) {
                    if (i >= img->pixel_data_size) return BMP_ERR_STREAM;
                    ++i;
                }
                x += num;
            }
        }
    }

    return BMP_ERR_STREAM;
}

static int bmp_decode_rle4(const bmp_image *img, bmp_u8 *out, bmp_u32 stride)
{
    const bmp_metadata *m = &img->meta;
    bmp_u32 width = (bmp_u32)m->width;
    bmp_u32 height = (bmp_u32)m->height;
    bmp_u32 x = 0;
    bmp_u32 y = 0;
    bmp_u32 i = 0;

    bmp_fill_rgba32(out, width, height, stride, 0, 0, 0, 255);

    while (i < img->pixel_data_size) {
        bmp_u8 count = img->pixel_data[i++];
        if (count != 0U) {
            bmp_u8 packed;
            bmp_u8 idx0, idx1;
            bmp_u32 k;
            if (i >= img->pixel_data_size) return BMP_ERR_STREAM;
            if (y >= height || x > width) return BMP_ERR_FORMAT;
            packed = img->pixel_data[i++];
            idx0 = (bmp_u8)(packed >> 4);
            idx1 = (bmp_u8)(packed & 0x0FU);
            if (count > width - x) return BMP_ERR_FORMAT;
            for (k = 0; k < count; ++k) {
                bmp_u8 idx = (bmp_u8)(((k & 1U) == 0U) ? idx0 : idx1);
                bmp_u8 r, g, b, a;
                bmp_u8 *row = bmp_out_row(out, stride, height, y, m->is_top_down);
                bmp_get_palette_color(img, idx, &r, &g, &b, &a);
                bmp_set_px(row, x + k, r, g, b, 255);
            }
            x += count;
        } else {
            bmp_u8 cmd;
            if (i >= img->pixel_data_size) return BMP_ERR_STREAM;
            cmd = img->pixel_data[i++];
            if (cmd == 0U) {
                x = 0;
                ++y;
                if (y > height) return BMP_ERR_FORMAT;
            } else if (cmd == 1U) {
                return BMP_OK;
            } else if (cmd == 2U) {
                bmp_u8 dx, dy;
                if (i + 1U >= img->pixel_data_size) return BMP_ERR_STREAM;
                dx = img->pixel_data[i++];
                dy = img->pixel_data[i++];
                if (dx > width - x) return BMP_ERR_FORMAT;
                if (dy > height - y) return BMP_ERR_FORMAT;
                x += dx;
                y += dy;
                if (x > width || y > height) return BMP_ERR_FORMAT;
            } else {
                bmp_u32 num_pixels = cmd;
                bmp_u32 byte_count = (num_pixels + 1U) / 2U;
                bmp_u32 k;
                if (i + byte_count > img->pixel_data_size) return BMP_ERR_STREAM;
                if (y >= height || num_pixels > width - x) return BMP_ERR_FORMAT;
                for (k = 0; k < num_pixels; ++k) {
                    bmp_u8 byte = img->pixel_data[i + (k >> 1)];
                    bmp_u8 idx = (bmp_u8)(((k & 1U) == 0U) ? (byte >> 4) : (byte & 0x0FU));
                    bmp_u8 r, g, b, a;
                    bmp_u8 *row = bmp_out_row(out, stride, height, y, m->is_top_down);
                    bmp_get_palette_color(img, idx, &r, &g, &b, &a);
                    bmp_set_px(row, x + k, r, g, b, 255);
                }
                i += byte_count;
                if ((byte_count & 1U) != 0U) {
                    if (i >= img->pixel_data_size) return BMP_ERR_STREAM;
                    ++i;
                }
                x += num_pixels;
            }
        }
    }

    return BMP_ERR_STREAM;
}

int bmp_decode_to_rgba32_with_limits(const bmp_image *img,
                                     const bmp_limits *limits,
                                     bmp_u8 *out_rgba,
                                     bmp_u32 out_stride)
{
    bmp_u32 min_stride;
    int rc;

    if (!img || !out_rgba) return BMP_ERR_ARGUMENT;
    rc = bmp_check_decode_limits(img, limits);
    if (rc != BMP_OK) return rc;
    rc = bmp_min_rgba_stride(img, &min_stride);
    if (rc != BMP_OK) return rc;
    if (out_stride < min_stride) return BMP_ERR_BUFFER_TOO_SMALL;

    switch (img->meta.compression) {
    case BMP_COMP_RGB:
        switch (img->meta.bpp) {
        case 1:  return bmp_decode_indexed_1bpp(img, out_rgba, out_stride);
        case 2:  return bmp_decode_indexed_2bpp(img, out_rgba, out_stride);
        case 4:  return bmp_decode_indexed_4bpp(img, out_rgba, out_stride);
        case 8:  return bmp_decode_indexed_8bpp(img, out_rgba, out_stride);
        case 16: return bmp_decode_16bpp(img, out_rgba, out_stride);
        case 24: return bmp_decode_24bpp(img, out_rgba, out_stride);
        case 32: return bmp_decode_32bpp(img, out_rgba, out_stride);
        default: return BMP_ERR_UNSUPPORTED;
        }
    case BMP_COMP_BITFIELDS:
    case BMP_COMP_ALPHABITFIELDS:
        switch (img->meta.bpp) {
        case 16: return bmp_decode_16bpp(img, out_rgba, out_stride);
        case 32: return bmp_decode_32bpp(img, out_rgba, out_stride);
        default: return BMP_ERR_UNSUPPORTED;
        }
    case BMP_COMP_RLE8:
        return bmp_decode_rle8(img, out_rgba, out_stride);
    case BMP_COMP_RLE4:
        return bmp_decode_rle4(img, out_rgba, out_stride);
    default:
        return BMP_ERR_UNSUPPORTED;
    }
}

int bmp_decode_to_rgba32(const bmp_image *img,
                         bmp_u8 *out_rgba,
                         bmp_u32 out_stride)
{
    return bmp_decode_to_rgba32_with_limits(img, 0, out_rgba, out_stride);
}
