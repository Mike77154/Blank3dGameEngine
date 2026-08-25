#include "mfont_internal.h"

#define BMP_FILE_HEADER_SIZE 14UL
#define BMP_INFO_HEADER_SIZE 40UL

static mft_u32 bmp_align4(mft_u32 v)
{
    return (v + 3UL) & ~3UL;
}

static mft_s32 bmp_read_le32s(const mft_u8 *p)
{
    mft_u32 v;
    v = mft_read_le32(p);
    if ((v & 0x80000000UL) != 0UL) {
        return (mft_s32)(-((mft_s32)((~v + 1UL) & 0xFFFFFFFFUL)));
    }
    return (mft_s32)v;
}

static mft_status bmp_prepare_output(mft_image *out,
                                     mft_u32 width,
                                     mft_u32 height,
                                     mft_pixel_format format,
                                     mft_u32 palette_count)
{
    mft_u32 req_stride;
    mft_u32 req_cap;

    if (out == 0) {
        return MFT_ERR_ARGS;
    }
    if (format == MFT_PIXFMT_INDEX8) {
        req_stride = width;
    } else if (format == MFT_PIXFMT_RGB24) {
        req_stride = width * 3UL;
    } else if (format == MFT_PIXFMT_RGBA32) {
        req_stride = width * 4UL;
    } else {
        return MFT_ERR_UNSUPPORTED;
    }

    if (out->stride == 0UL) {
        out->stride = req_stride;
    }
    out->width = width;
    out->height = height;
    out->format = format;
    out->palette_count = palette_count;

    if (out->stride < req_stride) {
        return MFT_ERR_CAPACITY;
    }
    req_cap = out->stride * height;
    if (out->pixels == 0 || out->pixels_capacity < req_cap) {
        return MFT_ERR_CAPACITY;
    }
    return MFT_OK;
}

mft_status fntbump_decode(mft_image *out,
                          const mft_u8 *bmp,
                          mft_u32 bmp_size)
{
    mft_u32 off_bits;
    mft_u32 dib_size;
    mft_u32 width;
    mft_u32 height;
    mft_u32 abs_height;
    mft_u32 planes;
    mft_u32 bpp;
    mft_u32 compression;
    int top_down;
    mft_u32 palette_entries;
    mft_u32 palette_start;
    mft_u32 row_size;
    mft_u32 y;
    mft_status st;

    if (out == 0 || bmp == 0) {
        return MFT_ERR_ARGS;
    }
    if (bmp_size < BMP_FILE_HEADER_SIZE + 12UL) {
        return MFT_ERR_BOUNDS;
    }
    if (bmp[0] != 'B' || bmp[1] != 'M') {
        return MFT_ERR_SIGNATURE;
    }

    off_bits = mft_read_le32(bmp + 10);
    dib_size = mft_read_le32(bmp + 14);
    if (dib_size == 12UL) {
        width = (mft_u32)mft_read_le16(bmp + 18);
        height = (mft_u32)mft_read_le16(bmp + 20);
        abs_height = height;
        top_down = 0;
        planes = (mft_u32)mft_read_le16(bmp + 22);
        bpp = (mft_u32)mft_read_le16(bmp + 24);
        compression = 0UL;
        palette_start = 14UL + 12UL;
        palette_entries = (bpp <= 8UL) ? (1UL << bpp) : 0UL;
    } else if (dib_size >= 40UL) {
        mft_s32 sh;
        mft_s32 sw;
        if (bmp_size < 14UL + dib_size) {
            return MFT_ERR_BOUNDS;
        }
        sw = bmp_read_le32s(bmp + 18);
        sh = bmp_read_le32s(bmp + 22);
        if (sw <= 0) {
            return MFT_ERR_FORMAT;
        }
        width = (mft_u32)sw;
        if (sh < 0) {
            top_down = 1;
            abs_height = (mft_u32)(-sh);
        } else {
            top_down = 0;
            abs_height = (mft_u32)sh;
        }
        planes = (mft_u32)mft_read_le16(bmp + 26);
        bpp = (mft_u32)mft_read_le16(bmp + 28);
        compression = mft_read_le32(bmp + 30);
        palette_start = 14UL + dib_size;
        palette_entries = mft_read_le32(bmp + 46);
        if (palette_entries == 0UL && bpp <= 8UL) {
            palette_entries = 1UL << bpp;
        }
    } else {
        return MFT_ERR_UNSUPPORTED;
    }

    if (width == 0UL || abs_height == 0UL) {
        return MFT_ERR_FORMAT;
    }
    if (planes != 1UL) {
        return MFT_ERR_UNSUPPORTED;
    }
    if (compression != 0UL) {
        return MFT_ERR_UNSUPPORTED;
    }

    if (bpp == 8UL) {
        row_size = bmp_align4(width);
        st = bmp_prepare_output(out, width, abs_height, MFT_PIXFMT_INDEX8, palette_entries);
    } else if (bpp == 24UL) {
        row_size = bmp_align4(width * 3UL);
        st = bmp_prepare_output(out, width, abs_height, MFT_PIXFMT_RGB24, 0UL);
    } else if (bpp == 32UL) {
        row_size = width * 4UL;
        st = bmp_prepare_output(out, width, abs_height, MFT_PIXFMT_RGBA32, 0UL);
    } else {
        return MFT_ERR_UNSUPPORTED;
    }
    if (st != MFT_OK) {
        return st;
    }
    if (off_bits > bmp_size || row_size > bmp_size || (row_size * abs_height) > bmp_size ||
        off_bits + (row_size * abs_height) > bmp_size) {
        return MFT_ERR_BOUNDS;
    }

    if (bpp == 8UL) {
        mft_u32 i;
        if (palette_entries == 0UL || palette_entries > 256UL) {
            return MFT_ERR_FORMAT;
        }
        if (dib_size == 12UL) {
            if (palette_start + (palette_entries * 3UL) > bmp_size) {
                return MFT_ERR_BOUNDS;
            }
            for (i = 0UL; i < palette_entries; ++i) {
                out->palette[i].b = bmp[palette_start + (i * 3UL) + 0UL];
                out->palette[i].g = bmp[palette_start + (i * 3UL) + 1UL];
                out->palette[i].r = bmp[palette_start + (i * 3UL) + 2UL];
                out->palette[i].a = 255U;
            }
        } else {
            if (palette_start + (palette_entries * 4UL) > bmp_size) {
                return MFT_ERR_BOUNDS;
            }
            for (i = 0UL; i < palette_entries; ++i) {
                out->palette[i].b = bmp[palette_start + (i * 4UL) + 0UL];
                out->palette[i].g = bmp[palette_start + (i * 4UL) + 1UL];
                out->palette[i].r = bmp[palette_start + (i * 4UL) + 2UL];
                out->palette[i].a = 255U;
            }
        }
    }

    for (y = 0UL; y < abs_height; ++y) {
        mft_u32 src_y;
        const mft_u8 *src_row;
        if (top_down) {
            src_y = y;
        } else {
            src_y = abs_height - 1UL - y;
        }
        src_row = bmp + off_bits + (src_y * row_size);
        if (bpp == 8UL) {
            memcpy(out->pixels + (y * out->stride), src_row, (size_t)width);
        } else if (bpp == 24UL) {
            mft_u8 *dst_row;
            mft_u32 x;
            dst_row = out->pixels + (y * out->stride);
            for (x = 0UL; x < width; ++x) {
                dst_row[(x * 3UL) + 0UL] = src_row[(x * 3UL) + 2UL];
                dst_row[(x * 3UL) + 1UL] = src_row[(x * 3UL) + 1UL];
                dst_row[(x * 3UL) + 2UL] = src_row[(x * 3UL) + 0UL];
            }
        } else {
            mft_u8 *dst_row;
            mft_u32 x;
            dst_row = out->pixels + (y * out->stride);
            for (x = 0UL; x < width; ++x) {
                dst_row[(x * 4UL) + 0UL] = src_row[(x * 4UL) + 2UL];
                dst_row[(x * 4UL) + 1UL] = src_row[(x * 4UL) + 1UL];
                dst_row[(x * 4UL) + 2UL] = src_row[(x * 4UL) + 0UL];
                dst_row[(x * 4UL) + 3UL] = src_row[(x * 4UL) + 3UL];
            }
        }
    }

    return MFT_OK;
}

mft_status fntbump_encode(mft_u8 *dst,
                          mft_u32 *dst_size,
                          const mft_image *image)
{
    mft_u32 width;
    mft_u32 height;
    mft_u32 bpp;
    mft_u32 row_size;
    mft_u32 palette_entries;
    mft_u32 off_bits;
    mft_u32 file_size;
    mft_u32 y;

    if (dst == 0 || dst_size == 0 || image == 0) {
        return MFT_ERR_ARGS;
    }
    if (mft_image_validate(image) != MFT_OK) {
        return MFT_ERR_IMAGE;
    }

    width = image->width;
    height = image->height;
    if (image->format == MFT_PIXFMT_INDEX8) {
        bpp = 8UL;
        row_size = bmp_align4(width);
        palette_entries = 256UL;
    } else if (image->format == MFT_PIXFMT_RGB24) {
        bpp = 24UL;
        row_size = bmp_align4(width * 3UL);
        palette_entries = 0UL;
    } else if (image->format == MFT_PIXFMT_RGBA32) {
        bpp = 32UL;
        row_size = width * 4UL;
        palette_entries = 0UL;
    } else {
        return MFT_ERR_UNSUPPORTED;
    }

    off_bits = BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + (palette_entries * 4UL);
    file_size = off_bits + (row_size * height);
    if (*dst_size < file_size) {
        *dst_size = file_size;
        return MFT_ERR_CAPACITY;
    }

    mft_memzero(dst, file_size);
    dst[0] = 'B';
    dst[1] = 'M';
    mft_write_le32(dst + 2, file_size);
    mft_write_le32(dst + 10, off_bits);
    mft_write_le32(dst + 14, BMP_INFO_HEADER_SIZE);
    mft_write_le32(dst + 18, width);
    mft_write_le32(dst + 22, height);
    mft_write_le16(dst + 26, 1U);
    mft_write_le16(dst + 28, (mft_u16)bpp);
    mft_write_le32(dst + 30, 0UL);
    mft_write_le32(dst + 34, row_size * height);
    mft_write_le32(dst + 38, 2835UL);
    mft_write_le32(dst + 42, 2835UL);
    mft_write_le32(dst + 46, palette_entries);
    mft_write_le32(dst + 50, 0UL);

    if (palette_entries > 0UL) {
        mft_u32 i;
        for (i = 0UL; i < palette_entries; ++i) {
            mft_rgba c;
            if (i < image->palette_count) {
                c = image->palette[i];
            } else {
                c.r = 0U;
                c.g = 0U;
                c.b = 0U;
                c.a = 255U;
            }
            dst[BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + (i * 4UL) + 0UL] = c.b;
            dst[BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + (i * 4UL) + 1UL] = c.g;
            dst[BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + (i * 4UL) + 2UL] = c.r;
            dst[BMP_FILE_HEADER_SIZE + BMP_INFO_HEADER_SIZE + (i * 4UL) + 3UL] = 0U;
        }
    }

    for (y = 0UL; y < height; ++y) {
        mft_u32 src_y;
        mft_u8 *dst_row;
        const mft_u8 *src_row;
        src_y = height - 1UL - y;
        dst_row = dst + off_bits + (y * row_size);
        src_row = image->pixels + (src_y * image->stride);
        if (image->format == MFT_PIXFMT_INDEX8) {
            memcpy(dst_row, src_row, (size_t)width);
            if (row_size > width) {
                memset(dst_row + width, 0, (size_t)(row_size - width));
            }
        } else if (image->format == MFT_PIXFMT_RGB24) {
            mft_u32 x;
            for (x = 0UL; x < width; ++x) {
                dst_row[(x * 3UL) + 0UL] = src_row[(x * 3UL) + 2UL];
                dst_row[(x * 3UL) + 1UL] = src_row[(x * 3UL) + 1UL];
                dst_row[(x * 3UL) + 2UL] = src_row[(x * 3UL) + 0UL];
            }
            if (row_size > (width * 3UL)) {
                memset(dst_row + (width * 3UL), 0, (size_t)(row_size - (width * 3UL)));
            }
        } else {
            mft_u32 x;
            for (x = 0UL; x < width; ++x) {
                dst_row[(x * 4UL) + 0UL] = src_row[(x * 4UL) + 2UL];
                dst_row[(x * 4UL) + 1UL] = src_row[(x * 4UL) + 1UL];
                dst_row[(x * 4UL) + 2UL] = src_row[(x * 4UL) + 0UL];
                dst_row[(x * 4UL) + 3UL] = src_row[(x * 4UL) + 3UL];
            }
        }
    }

    *dst_size = file_size;
    return MFT_OK;
}
