#include "mfont_internal.h"

#define PCX_HEADER_SIZE 128UL

static mft_u32 pcx_align_even(mft_u32 v)
{
    return (v + 1UL) & ~1UL;
}

static mft_status pcx_prepare_output(mft_image *out,
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

static mft_status pcx_decode_row(mft_u8 *dst,
                                 mft_u32 dst_size,
                                 const mft_u8 *src,
                                 mft_u32 src_size,
                                 mft_u32 *src_pos)
{
    mft_u32 out_pos;
    mft_u32 in_pos;

    out_pos = 0UL;
    in_pos = *src_pos;
    while (out_pos < dst_size) {
        mft_u8 b;
        mft_u32 count;
        mft_u8 value;
        mft_u32 i;

        if (in_pos >= src_size) {
            return MFT_ERR_BOUNDS;
        }
        b = src[in_pos++];
        if ((b & 0xC0U) == 0xC0U) {
            count = (mft_u32)(b & 0x3FU);
            if (count == 0UL) {
                return MFT_ERR_FORMAT;
            }
            if (in_pos >= src_size) {
                return MFT_ERR_BOUNDS;
            }
            value = src[in_pos++];
        } else {
            count = 1UL;
            value = b;
        }
        if (out_pos + count > dst_size) {
            return MFT_ERR_FORMAT;
        }
        for (i = 0UL; i < count; ++i) {
            dst[out_pos++] = value;
        }
    }
    *src_pos = in_pos;
    return MFT_OK;
}

mft_status pcxfnt_decode(mft_image *out,
                         const mft_u8 *pcx,
                         mft_u32 pcx_size)
{
    mft_u32 xmin;
    mft_u32 ymin;
    mft_u32 xmax;
    mft_u32 ymax;
    mft_u32 width;
    mft_u32 height;
    mft_u32 bytes_per_line;
    mft_u32 planes;
    mft_u32 bits_per_plane;
    mft_u32 row_size;
    mft_u32 src_pos;
    mft_status st;
    mft_u32 y;
    mft_u8 rowbuf[MFT_CFG_MAX_SCANLINE];

    if (out == 0 || pcx == 0) {
        return MFT_ERR_ARGS;
    }
    if (pcx_size < PCX_HEADER_SIZE) {
        return MFT_ERR_BOUNDS;
    }
    if (pcx[0] != 0x0AU) {
        return MFT_ERR_SIGNATURE;
    }
    if (pcx[2] != 1U) {
        return MFT_ERR_UNSUPPORTED;
    }

    bits_per_plane = pcx[3];
    xmin = (mft_u32)mft_read_le16(pcx + 4);
    ymin = (mft_u32)mft_read_le16(pcx + 6);
    xmax = (mft_u32)mft_read_le16(pcx + 8);
    ymax = (mft_u32)mft_read_le16(pcx + 10);
    planes = (mft_u32)pcx[65];
    bytes_per_line = (mft_u32)mft_read_le16(pcx + 66);

    if (xmax < xmin || ymax < ymin) {
        return MFT_ERR_FORMAT;
    }
    width = xmax - xmin + 1UL;
    height = ymax - ymin + 1UL;
    if (width == 0UL || height == 0UL) {
        return MFT_ERR_FORMAT;
    }
    /* Some M.U.G.E.N/CharCAD FNT files store PCX rows with an odd
       bytes-per-line value even though the PCX spec recommends even rows.
       Accept it as long as the row is non-zero and large enough for width. */
    if (bytes_per_line == 0UL || bytes_per_line < width) {
        return MFT_ERR_FORMAT;
    }
    row_size = bytes_per_line * planes;
    if (row_size > MFT_CFG_MAX_SCANLINE) {
        return MFT_ERR_CAPACITY;
    }

    if (bits_per_plane == 8U && planes == 1UL) {
        st = pcx_prepare_output(out, width, height, MFT_PIXFMT_INDEX8, 256UL);
    } else if (bits_per_plane == 8U && planes == 3UL) {
        st = pcx_prepare_output(out, width, height, MFT_PIXFMT_RGB24, 0UL);
    } else {
        return MFT_ERR_UNSUPPORTED;
    }
    if (st != MFT_OK) {
        return st;
    }

    src_pos = PCX_HEADER_SIZE;
    for (y = 0UL; y < height; ++y) {
        st = pcx_decode_row(rowbuf, row_size, pcx, pcx_size, &src_pos);
        if (st != MFT_OK) {
            return st;
        }

        if (out->format == MFT_PIXFMT_INDEX8) {
            memcpy(out->pixels + (y * out->stride), rowbuf, (size_t)width);
        } else {
            mft_u8 *dst_row;
            mft_u32 x;
            dst_row = out->pixels + (y * out->stride);
            for (x = 0UL; x < width; ++x) {
                dst_row[(x * 3UL) + 0UL] = rowbuf[x];
                dst_row[(x * 3UL) + 1UL] = rowbuf[bytes_per_line + x];
                dst_row[(x * 3UL) + 2UL] = rowbuf[(bytes_per_line * 2UL) + x];
            }
        }
    }

    if (out->format == MFT_PIXFMT_INDEX8) {
        mft_u32 pal_pos;
        mft_u32 i;
        if (pcx_size < 769UL) {
            return MFT_ERR_BOUNDS;
        }
        pal_pos = pcx_size - 769UL;
        if (pcx[pal_pos] != 0x0CU) {
            return MFT_ERR_UNSUPPORTED;
        }
        pal_pos++;
        for (i = 0UL; i < 256UL; ++i) {
            out->palette[i].r = pcx[pal_pos + (i * 3UL) + 0UL];
            out->palette[i].g = pcx[pal_pos + (i * 3UL) + 1UL];
            out->palette[i].b = pcx[pal_pos + (i * 3UL) + 2UL];
            out->palette[i].a = 255U;
        }
        out->palette_count = 256UL;
    }

    return MFT_OK;
}

static mft_status pcx_emit_byte(mft_u8 *dst,
                                mft_u32 dst_cap,
                                mft_u32 *dst_pos,
                                mft_u8 value)
{
    if (*dst_pos >= dst_cap) {
        return MFT_ERR_CAPACITY;
    }
    dst[*dst_pos] = value;
    (*dst_pos)++;
    return MFT_OK;
}

static mft_status pcx_encode_rle(mft_u8 *dst,
                                 mft_u32 dst_cap,
                                 mft_u32 *dst_pos,
                                 const mft_u8 *src,
                                 mft_u32 src_size)
{
    mft_u32 i;

    i = 0UL;
    while (i < src_size) {
        mft_u8 value;
        mft_u32 run;
        mft_status st;

        value = src[i];
        run = 1UL;
        while ((i + run) < src_size && run < 63UL && src[i + run] == value) {
            ++run;
        }
        if (run > 1UL || (value & 0xC0U) == 0xC0U) {
            st = pcx_emit_byte(dst, dst_cap, dst_pos, (mft_u8)(0xC0U | (mft_u8)run));
            if (st != MFT_OK) {
                return st;
            }
            st = pcx_emit_byte(dst, dst_cap, dst_pos, value);
            if (st != MFT_OK) {
                return st;
            }
        } else {
            st = pcx_emit_byte(dst, dst_cap, dst_pos, value);
            if (st != MFT_OK) {
                return st;
            }
        }
        i += run;
    }
    return MFT_OK;
}

mft_status pcxfnt_encode(mft_u8 *dst,
                         mft_u32 *dst_size,
                         const mft_image *image)
{
    mft_u32 width;
    mft_u32 height;
    mft_u32 bytes_per_line;
    mft_u32 need;
    mft_u32 pos;
    mft_u32 y;
    mft_u8 rowbuf[MFT_CFG_MAX_SCANLINE];
    mft_status st;

    if (dst == 0 || dst_size == 0 || image == 0) {
        return MFT_ERR_ARGS;
    }
    st = mft_image_validate(image);
    if (st != MFT_OK) {
        return st;
    }
    if (image->format != MFT_PIXFMT_INDEX8 && image->format != MFT_PIXFMT_RGB24) {
        return MFT_ERR_UNSUPPORTED;
    }

    width = image->width;
    height = image->height;
    bytes_per_line = pcx_align_even(width);
    if (((image->format == MFT_PIXFMT_INDEX8) && (bytes_per_line > MFT_CFG_MAX_SCANLINE)) ||
        ((image->format == MFT_PIXFMT_RGB24) && ((bytes_per_line * 3UL) > MFT_CFG_MAX_SCANLINE))) {
        return MFT_ERR_CAPACITY;
    }

    need = PCX_HEADER_SIZE + (height * bytes_per_line * ((image->format == MFT_PIXFMT_INDEX8) ? 2UL : 6UL));
    if (image->format == MFT_PIXFMT_INDEX8) {
        need += 769UL;
    }
    if (*dst_size < need) {
        *dst_size = need;
        return MFT_ERR_CAPACITY;
    }

    mft_memzero(dst, PCX_HEADER_SIZE);
    dst[0] = 0x0AU;
    dst[1] = 5U;
    dst[2] = 1U;
    dst[3] = 8U;
    mft_write_le16(dst + 4, 0U);
    mft_write_le16(dst + 6, 0U);
    if (width == 0UL || height == 0UL) {
        return MFT_ERR_FORMAT;
    }
    mft_write_le16(dst + 8, (mft_u16)(width - 1UL));
    mft_write_le16(dst + 10, (mft_u16)(height - 1UL));
    mft_write_le16(dst + 12, (mft_u16)width);
    mft_write_le16(dst + 14, (mft_u16)height);
    dst[64] = 0U;
    dst[65] = (mft_u8)((image->format == MFT_PIXFMT_INDEX8) ? 1U : 3U);
    mft_write_le16(dst + 66, (mft_u16)bytes_per_line);
    mft_write_le16(dst + 68, 1U);

    if (image->format == MFT_PIXFMT_INDEX8) {
        mft_u32 i;
        for (i = 0UL; i < 16UL; ++i) {
            dst[16UL + (i * 3UL) + 0UL] = image->palette[i].r;
            dst[16UL + (i * 3UL) + 1UL] = image->palette[i].g;
            dst[16UL + (i * 3UL) + 2UL] = image->palette[i].b;
        }
    }

    pos = PCX_HEADER_SIZE;
    for (y = 0UL; y < height; ++y) {
        if (image->format == MFT_PIXFMT_INDEX8) {
            memcpy(rowbuf, image->pixels + (y * image->stride), (size_t)width);
            if (bytes_per_line > width) {
                memset(rowbuf + width, 0, (size_t)(bytes_per_line - width));
            }
            st = pcx_encode_rle(dst, *dst_size, &pos, rowbuf, bytes_per_line);
            if (st != MFT_OK) {
                *dst_size = need;
                return st;
            }
        } else {
            const mft_u8 *src_row;
            mft_u32 x;
            src_row = image->pixels + (y * image->stride);
            for (x = 0UL; x < bytes_per_line; ++x) {
                rowbuf[x] = (x < width) ? src_row[(x * 3UL) + 0UL] : 0U;
                rowbuf[bytes_per_line + x] = (x < width) ? src_row[(x * 3UL) + 1UL] : 0U;
                rowbuf[(bytes_per_line * 2UL) + x] = (x < width) ? src_row[(x * 3UL) + 2UL] : 0U;
            }
            st = pcx_encode_rle(dst, *dst_size, &pos, rowbuf, bytes_per_line * 3UL);
            if (st != MFT_OK) {
                *dst_size = need;
                return st;
            }
        }
    }

    if (image->format == MFT_PIXFMT_INDEX8) {
        mft_u32 i;
        dst[pos++] = 0x0CU;
        for (i = 0UL; i < 256UL; ++i) {
            mft_rgba c;
            if (i < image->palette_count) {
                c = image->palette[i];
            } else {
                c.r = 0U;
                c.g = 0U;
                c.b = 0U;
                c.a = 255U;
            }
            dst[pos++] = c.r;
            dst[pos++] = c.g;
            dst[pos++] = c.b;
        }
    }

    *dst_size = pos;
    return MFT_OK;
}
