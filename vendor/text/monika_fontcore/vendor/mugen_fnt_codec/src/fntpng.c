#include "mfont_internal.h"

static const mft_u8 png_sig[8] = { 137U, 80U, 78U, 71U, 13U, 10U, 26U, 10U };

static mft_u32 png_crc_table[256];
static int png_crc_ready = 0;

static void png_crc_init(void)
{
    mft_u32 i;
    mft_u32 j;
    mft_u32 c;
    if (png_crc_ready) {
        return;
    }
    for (i = 0UL; i < 256UL; ++i) {
        c = i;
        for (j = 0UL; j < 8UL; ++j) {
            if ((c & 1UL) != 0UL) {
                c = 0xEDB88320UL ^ (c >> 1);
            } else {
                c >>= 1;
            }
        }
        png_crc_table[i] = c;
    }
    png_crc_ready = 1;
}

static mft_u32 png_crc_start(void)
{
    png_crc_init();
    return 0xFFFFFFFFUL;
}

static mft_u32 png_crc_update(mft_u32 crc, const mft_u8 *data, mft_u32 size)
{
    mft_u32 i;
    for (i = 0UL; i < size; ++i) {
        crc = png_crc_table[(crc ^ data[i]) & 0xFFUL] ^ (crc >> 8);
    }
    return crc;
}

static mft_u32 png_crc_finish(mft_u32 crc)
{
    return crc ^ 0xFFFFFFFFUL;
}

static mft_u32 png_chunk_crc(const char *type, const mft_u8 *data, mft_u32 size)
{
    mft_u32 crc;
    crc = png_crc_start();
    crc = png_crc_update(crc, (const mft_u8 *)type, 4UL);
    if (size > 0UL && data != 0) {
        crc = png_crc_update(crc, data, size);
    }
    return png_crc_finish(crc);
}

static int png_chunk_is_critical(const mft_u8 *type)
{
    return ((type[0] & 0x20U) == 0U) ? 1 : 0;
}

static int png_is_type(const mft_u8 *type, const char *text)
{
    return (type[0] == (mft_u8)text[0] &&
            type[1] == (mft_u8)text[1] &&
            type[2] == (mft_u8)text[2] &&
            type[3] == (mft_u8)text[3]) ? 1 : 0;
}

static mft_u32 png_samples_per_pixel(mft_u32 color_type)
{
    switch (color_type) {
        case 0UL: return 1UL;
        case 2UL: return 3UL;
        case 3UL: return 1UL;
        case 4UL: return 2UL;
        case 6UL: return 4UL;
        default: return 0UL;
    }
}

static int png_is_supported_combo(mft_u32 color_type, mft_u32 bit_depth)
{
    switch (color_type) {
        case 0UL:
            return (bit_depth == 1UL || bit_depth == 2UL || bit_depth == 4UL || bit_depth == 8UL) ? 1 : 0;
        case 2UL:
            return (bit_depth == 8UL) ? 1 : 0;
        case 3UL:
            return (bit_depth == 1UL || bit_depth == 2UL || bit_depth == 4UL || bit_depth == 8UL) ? 1 : 0;
        case 4UL:
            return (bit_depth == 8UL) ? 1 : 0;
        case 6UL:
            return (bit_depth == 8UL) ? 1 : 0;
        default:
            return 0;
    }
}

static mft_u32 png_packed_row_size(mft_u32 width, mft_u32 bit_depth, mft_u32 color_type)
{
    mft_u32 spp;
    mft_u32 bits;
    spp = png_samples_per_pixel(color_type);
    bits = width * spp * bit_depth;
    return (bits + 7UL) / 8UL;
}

static mft_u32 png_filter_bpp(mft_u32 bit_depth, mft_u32 color_type)
{
    mft_u32 spp;
    mft_u32 bits;
    spp = png_samples_per_pixel(color_type);
    bits = spp * bit_depth;
    return (bits + 7UL) / 8UL;
}

static mft_u8 png_paeth(mft_u8 a, mft_u8 b, mft_u8 c)
{
    mft_s32 p;
    mft_s32 pa;
    mft_s32 pb;
    mft_s32 pc;
    p = (mft_s32)a + (mft_s32)b - (mft_s32)c;
    pa = p - (mft_s32)a;
    if (pa < 0) {
        pa = -pa;
    }
    pb = p - (mft_s32)b;
    if (pb < 0) {
        pb = -pb;
    }
    pc = p - (mft_s32)c;
    if (pc < 0) {
        pc = -pc;
    }
    if (pa <= pb && pa <= pc) {
        return a;
    }
    if (pb <= pc) {
        return b;
    }
    return c;
}

static mft_status png_unfilter_row(mft_u8 *dst,
                                   const mft_u8 *src,
                                   const mft_u8 *prev,
                                   mft_u32 row_size,
                                   mft_u32 bpp,
                                   mft_u8 filter)
{
    mft_u32 i;
    switch (filter) {
        case 0U:
            memcpy(dst, src, (size_t)row_size);
            return MFT_OK;
        case 1U:
            for (i = 0UL; i < row_size; ++i) {
                mft_u8 left;
                left = (i >= bpp) ? dst[i - bpp] : 0U;
                dst[i] = (mft_u8)(src[i] + left);
            }
            return MFT_OK;
        case 2U:
            for (i = 0UL; i < row_size; ++i) {
                mft_u8 up;
                up = (prev != 0) ? prev[i] : 0U;
                dst[i] = (mft_u8)(src[i] + up);
            }
            return MFT_OK;
        case 3U:
            for (i = 0UL; i < row_size; ++i) {
                mft_u8 left;
                mft_u8 up;
                left = (i >= bpp) ? dst[i - bpp] : 0U;
                up = (prev != 0) ? prev[i] : 0U;
                dst[i] = (mft_u8)(src[i] + (mft_u8)(((mft_u32)left + (mft_u32)up) / 2UL));
            }
            return MFT_OK;
        case 4U:
            for (i = 0UL; i < row_size; ++i) {
                mft_u8 left;
                mft_u8 up;
                mft_u8 up_left;
                left = (i >= bpp) ? dst[i - bpp] : 0U;
                up = (prev != 0) ? prev[i] : 0U;
                up_left = (prev != 0 && i >= bpp) ? prev[i - bpp] : 0U;
                dst[i] = (mft_u8)(src[i] + png_paeth(left, up, up_left));
            }
            return MFT_OK;
        default:
            return MFT_ERR_UNSUPPORTED;
    }
}

static mft_status png_prepare_output(mft_image *out,
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

static void png_build_gray_palette(mft_rgba *palette,
                                   mft_u32 count,
                                   int has_trns,
                                   mft_u32 trns_gray)
{
    mft_u32 i;
    for (i = 0UL; i < count; ++i) {
        mft_u32 value;
        if (count > 1UL) {
            value = (i * 255UL) / (count - 1UL);
        } else {
            value = 0UL;
        }
        palette[i].r = (mft_u8)value;
        palette[i].g = (mft_u8)value;
        palette[i].b = (mft_u8)value;
        palette[i].a = (has_trns && i == trns_gray) ? 0U : 255U;
    }
}

static void png_unpack_bits(mft_u8 *dst,
                            const mft_u8 *src,
                            mft_u32 width,
                            mft_u32 bit_depth)
{
    mft_u32 x;
    mft_u32 mask;
    mask = (1UL << bit_depth) - 1UL;
    for (x = 0UL; x < width; ++x) {
        mft_u32 bit_pos;
        mft_u32 byte_index;
        mft_u32 shift;
        bit_pos = x * bit_depth;
        byte_index = bit_pos >> 3;
        shift = 8UL - bit_depth - (bit_pos & 7UL);
        dst[x] = (mft_u8)((src[byte_index] >> shift) & mask);
    }
}

static mft_status png_write_chunk(mft_u8 *dst,
                                  mft_u32 dst_cap,
                                  mft_u32 *dst_pos,
                                  const char *type,
                                  const mft_u8 *data,
                                  mft_u32 size)
{
    mft_u32 crc;
    if (*dst_pos + 12UL + size > dst_cap) {
        return MFT_ERR_CAPACITY;
    }
    mft_write_be32(dst + *dst_pos, size);
    *dst_pos += 4UL;
    dst[*dst_pos + 0UL] = (mft_u8)type[0];
    dst[*dst_pos + 1UL] = (mft_u8)type[1];
    dst[*dst_pos + 2UL] = (mft_u8)type[2];
    dst[*dst_pos + 3UL] = (mft_u8)type[3];
    *dst_pos += 4UL;
    if (size > 0UL) {
        memcpy(dst + *dst_pos, data, (size_t)size);
        *dst_pos += size;
    }
    crc = png_chunk_crc(type, data, size);
    mft_write_be32(dst + *dst_pos, crc);
    *dst_pos += 4UL;
    return MFT_OK;
}

mft_status fntpng_decode(mft_image *out,
                         fntpng_workspace *ws,
                         const mft_u8 *png,
                         mft_u32 png_size)
{
    mft_u32 pos;
    int seen_ihdr;
    int seen_idat;
    int seen_iend;
    mft_u32 width;
    mft_u32 height;
    mft_u32 bit_depth;
    mft_u32 color_type;
    mft_u32 compression;
    mft_u32 filter_method;
    mft_u32 interlace;
    mft_rgba palette[256];
    mft_u32 palette_count;
    mft_u8 trns_bytes[256];
    mft_u32 trns_count;
    int has_trns_gray;
    int has_trns_rgb;
    mft_u32 trns_gray;
    mft_u8 trns_r;
    mft_u8 trns_g;
    mft_u8 trns_b;
    mft_u32 idat_size;
    mft_u32 row_size;
    mft_u32 filtered_size;
    mft_u32 bpp;
    mft_u32 y;
    mft_u32 inflate_size;
    mft_pixel_format out_format;
    mft_u32 out_palette_count;
    mft_status st;

    if (out == 0 || ws == 0 || png == 0) {
        return MFT_ERR_ARGS;
    }
    if (png_size < 8UL || memcmp(png, png_sig, 8U) != 0) {
        return MFT_ERR_SIGNATURE;
    }

    pos = 8UL;
    seen_ihdr = 0;
    seen_idat = 0;
    seen_iend = 0;
    width = 0UL;
    height = 0UL;
    bit_depth = 0UL;
    color_type = 0UL;
    compression = 0UL;
    filter_method = 0UL;
    interlace = 0UL;
    palette_count = 0UL;
    trns_count = 0UL;
    has_trns_gray = 0;
    has_trns_rgb = 0;
    trns_gray = 0UL;
    trns_r = 0U;
    trns_g = 0U;
    trns_b = 0U;
    idat_size = 0UL;
    mft_memzero(palette, (mft_u32)sizeof(palette));
    mft_memzero(trns_bytes, (mft_u32)sizeof(trns_bytes));

    while (pos + 12UL <= png_size) {
        mft_u32 chunk_len;
        const mft_u8 *type;
        const mft_u8 *data;
        mft_u32 chunk_crc;
        mft_u32 calc_crc;

        chunk_len = mft_read_be32(png + pos);
        pos += 4UL;
        type = png + pos;
        pos += 4UL;
        if (chunk_len > png_size || pos + chunk_len + 4UL > png_size) {
            return MFT_ERR_BOUNDS;
        }
        data = png + pos;
        chunk_crc = mft_read_be32(data + chunk_len);
        calc_crc = png_crc_start();
        calc_crc = png_crc_update(calc_crc, type, 4UL);
        calc_crc = png_crc_update(calc_crc, data, chunk_len);
        calc_crc = png_crc_finish(calc_crc);
        if (chunk_crc != calc_crc) {
            return MFT_ERR_CHECKSUM;
        }

        if (png_is_type(type, "IHDR")) {
            if (seen_ihdr || chunk_len != 13UL) {
                return MFT_ERR_FORMAT;
            }
            width = mft_read_be32(data + 0);
            height = mft_read_be32(data + 4);
            bit_depth = (mft_u32)data[8];
            color_type = (mft_u32)data[9];
            compression = (mft_u32)data[10];
            filter_method = (mft_u32)data[11];
            interlace = (mft_u32)data[12];
            if (width == 0UL || height == 0UL) {
                return MFT_ERR_FORMAT;
            }
            if (!png_is_supported_combo(color_type, bit_depth)) {
                return MFT_ERR_UNSUPPORTED;
            }
            if (compression != 0UL || filter_method != 0UL) {
                return MFT_ERR_UNSUPPORTED;
            }
            if (interlace != 0UL) {
                return MFT_ERR_UNSUPPORTED;
            }
            seen_ihdr = 1;
        } else if (png_is_type(type, "PLTE")) {
            mft_u32 i;
            if (!seen_ihdr || (chunk_len % 3UL) != 0UL) {
                return MFT_ERR_FORMAT;
            }
            palette_count = chunk_len / 3UL;
            if (palette_count == 0UL || palette_count > 256UL) {
                return MFT_ERR_FORMAT;
            }
            for (i = 0UL; i < palette_count; ++i) {
                palette[i].r = data[(i * 3UL) + 0UL];
                palette[i].g = data[(i * 3UL) + 1UL];
                palette[i].b = data[(i * 3UL) + 2UL];
                palette[i].a = (i < trns_count) ? trns_bytes[i] : 255U;
            }
        } else if (png_is_type(type, "tRNS")) {
            if (!seen_ihdr) {
                return MFT_ERR_FORMAT;
            }
            if (color_type == 3UL) {
                if (chunk_len > 256UL) {
                    return MFT_ERR_FORMAT;
                }
                memcpy(trns_bytes, data, (size_t)chunk_len);
                trns_count = chunk_len;
                if (palette_count > 0UL) {
                    mft_u32 i;
                    for (i = 0UL; i < palette_count; ++i) {
                        palette[i].a = (i < trns_count) ? trns_bytes[i] : 255U;
                    }
                }
            } else if (color_type == 0UL) {
                if (chunk_len != 2UL) {
                    return MFT_ERR_FORMAT;
                }
                has_trns_gray = 1;
                trns_gray = (mft_u32)mft_read_be16(data);
            } else if (color_type == 2UL) {
                if (chunk_len != 6UL) {
                    return MFT_ERR_FORMAT;
                }
                if (data[0] != 0U || data[2] != 0U || data[4] != 0U) {
                    return MFT_ERR_UNSUPPORTED;
                }
                has_trns_rgb = 1;
                trns_r = data[1];
                trns_g = data[3];
                trns_b = data[5];
            } else {
                return MFT_ERR_UNSUPPORTED;
            }
        } else if (png_is_type(type, "IDAT")) {
            if (!seen_ihdr) {
                return MFT_ERR_FORMAT;
            }
            if (idat_size + chunk_len > (mft_u32)sizeof(ws->idat)) {
                return MFT_ERR_CAPACITY;
            }
            memcpy(ws->idat + idat_size, data, (size_t)chunk_len);
            idat_size += chunk_len;
            seen_idat = 1;
        } else if (png_is_type(type, "IEND")) {
            if (chunk_len != 0UL) {
                return MFT_ERR_FORMAT;
            }
            seen_iend = 1;
            pos += chunk_len + 4UL;
            break;
        } else if (png_chunk_is_critical(type)) {
            return MFT_ERR_UNSUPPORTED;
        }

        pos += chunk_len + 4UL;
    }

    if (!seen_ihdr || !seen_idat || !seen_iend) {
        return MFT_ERR_FORMAT;
    }

    row_size = png_packed_row_size(width, bit_depth, color_type);
    bpp = png_filter_bpp(bit_depth, color_type);
    filtered_size = height * (row_size + 1UL);
    if (row_size > (mft_u32)sizeof(ws->row_cur) || filtered_size > (mft_u32)sizeof(ws->filtered)) {
        return MFT_ERR_CAPACITY;
    }

    if (color_type == 3UL) {
        if (palette_count == 0UL) {
            return MFT_ERR_FORMAT;
        }
        out_format = MFT_PIXFMT_INDEX8;
        out_palette_count = palette_count;
    } else if (color_type == 0UL) {
        out_format = MFT_PIXFMT_INDEX8;
        out_palette_count = 1UL << bit_depth;
        png_build_gray_palette(palette, out_palette_count, has_trns_gray, trns_gray);
    } else if (color_type == 2UL) {
        if (has_trns_rgb) {
            out_format = MFT_PIXFMT_RGBA32;
        } else {
            out_format = MFT_PIXFMT_RGB24;
        }
        out_palette_count = 0UL;
    } else if (color_type == 4UL || color_type == 6UL) {
        out_format = MFT_PIXFMT_RGBA32;
        out_palette_count = 0UL;
    } else {
        return MFT_ERR_UNSUPPORTED;
    }

    st = png_prepare_output(out, width, height, out_format, out_palette_count);
    if (st != MFT_OK) {
        return st;
    }
    if (out_format == MFT_PIXFMT_INDEX8) {
        memcpy(out->palette, palette, (size_t)(out_palette_count * sizeof(mft_rgba)));
        out->palette_count = out_palette_count;
    }

    inflate_size = filtered_size;
    st = zlf_inflate_zlib(ws->filtered, &inflate_size, ws->idat, idat_size, 1);
    if (st != MFT_OK) {
        return st;
    }
    if (inflate_size != filtered_size) {
        return MFT_ERR_FORMAT;
    }

    mft_memzero(ws->row_prev, row_size);
    for (y = 0UL; y < height; ++y) {
        const mft_u8 *src_row;
        mft_u8 filter;
        filter = ws->filtered[y * (row_size + 1UL)];
        src_row = ws->filtered + (y * (row_size + 1UL)) + 1UL;
        st = png_unfilter_row(ws->row_cur,
                              src_row,
                              (y == 0UL) ? 0 : ws->row_prev,
                              row_size,
                              bpp,
                              filter);
        if (st != MFT_OK) {
            return st;
        }

        if (color_type == 3UL || color_type == 0UL) {
            mft_u8 *dst_row;
            dst_row = out->pixels + (y * out->stride);
            if (bit_depth == 8UL) {
                memcpy(dst_row, ws->row_cur, (size_t)width);
            } else {
                png_unpack_bits(dst_row, ws->row_cur, width, bit_depth);
            }
        } else if (color_type == 2UL) {
            const mft_u8 *s;
            mft_u32 x;
            s = ws->row_cur;
            if (out->format == MFT_PIXFMT_RGB24) {
                mft_u8 *dst_row;
                dst_row = out->pixels + (y * out->stride);
                memcpy(dst_row, s, (size_t)(width * 3UL));
            } else {
                mft_u8 *dst_row;
                dst_row = out->pixels + (y * out->stride);
                for (x = 0UL; x < width; ++x) {
                    dst_row[(x * 4UL) + 0UL] = s[(x * 3UL) + 0UL];
                    dst_row[(x * 4UL) + 1UL] = s[(x * 3UL) + 1UL];
                    dst_row[(x * 4UL) + 2UL] = s[(x * 3UL) + 2UL];
                    dst_row[(x * 4UL) + 3UL] = (s[(x * 3UL) + 0UL] == trns_r &&
                                                s[(x * 3UL) + 1UL] == trns_g &&
                                                s[(x * 3UL) + 2UL] == trns_b) ? 0U : 255U;
                }
            }
        } else if (color_type == 4UL) {
            mft_u8 *dst_row;
            mft_u32 x;
            dst_row = out->pixels + (y * out->stride);
            for (x = 0UL; x < width; ++x) {
                mft_u8 g;
                mft_u8 a;
                g = ws->row_cur[(x * 2UL) + 0UL];
                a = ws->row_cur[(x * 2UL) + 1UL];
                dst_row[(x * 4UL) + 0UL] = g;
                dst_row[(x * 4UL) + 1UL] = g;
                dst_row[(x * 4UL) + 2UL] = g;
                dst_row[(x * 4UL) + 3UL] = a;
            }
        } else {
            mft_u8 *dst_row;
            dst_row = out->pixels + (y * out->stride);
            memcpy(dst_row, ws->row_cur, (size_t)(width * 4UL));
        }

        memcpy(ws->row_prev, ws->row_cur, (size_t)row_size);
    }

    return MFT_OK;
}

static mft_status png_index_max_used(const mft_image *image, mft_u32 *out_max)
{
    mft_u32 y;
    mft_u32 max_index;
    max_index = 0UL;
    for (y = 0UL; y < image->height; ++y) {
        const mft_u8 *row;
        mft_u32 x;
        row = image->pixels + (y * image->stride);
        for (x = 0UL; x < image->width; ++x) {
            if ((mft_u32)row[x] > max_index) {
                max_index = (mft_u32)row[x];
            }
        }
    }
    *out_max = max_index;
    return MFT_OK;
}

mft_status fntpng_encode(mft_u8 *dst,
                         mft_u32 *dst_size,
                         fntpng_workspace *ws,
                         const mft_image *image)
{
    mft_u32 row_size;
    mft_u32 filtered_size;
    mft_u32 idat_size;
    mft_u32 pos;
    mft_u32 y;
    mft_u8 ihdr[13];
    mft_status st;
    mft_u32 palette_entries;
    mft_u8 plte[256 * 3];
    mft_u8 trns[256];
    mft_u32 trns_len;
    mft_u32 max_index;
    mft_u32 need;

    if (dst == 0 || dst_size == 0 || ws == 0 || image == 0) {
        return MFT_ERR_ARGS;
    }
    st = mft_image_validate(image);
    if (st != MFT_OK) {
        return st;
    }
    if (image->format != MFT_PIXFMT_INDEX8 &&
        image->format != MFT_PIXFMT_RGB24 &&
        image->format != MFT_PIXFMT_RGBA32) {
        return MFT_ERR_UNSUPPORTED;
    }

    if (image->format == MFT_PIXFMT_INDEX8) {
        row_size = image->width;
    } else if (image->format == MFT_PIXFMT_RGB24) {
        row_size = image->width * 3UL;
    } else {
        row_size = image->width * 4UL;
    }
    filtered_size = image->height * (row_size + 1UL);
    if (row_size > (mft_u32)sizeof(ws->row_cur) || filtered_size > (mft_u32)sizeof(ws->filtered)) {
        return MFT_ERR_CAPACITY;
    }

    pos = 0UL;
    for (y = 0UL; y < image->height; ++y) {
        const mft_u8 *src_row;
        ws->filtered[pos++] = 0U;
        src_row = image->pixels + (y * image->stride);
        if (image->format == MFT_PIXFMT_INDEX8) {
            memcpy(ws->filtered + pos, src_row, (size_t)image->width);
            pos += image->width;
        } else if (image->format == MFT_PIXFMT_RGB24) {
            memcpy(ws->filtered + pos, src_row, (size_t)(image->width * 3UL));
            pos += image->width * 3UL;
        } else {
            memcpy(ws->filtered + pos, src_row, (size_t)(image->width * 4UL));
            pos += image->width * 4UL;
        }
    }

    idat_size = (mft_u32)sizeof(ws->idat);
    st = zlf_deflate_stored_zlib(ws->idat, &idat_size, ws->filtered, filtered_size);
    if (st != MFT_OK) {
        return st;
    }

    mft_write_be32(ihdr + 0, image->width);
    mft_write_be32(ihdr + 4, image->height);
    if (image->format == MFT_PIXFMT_INDEX8) {
        ihdr[8] = 8U;
        ihdr[9] = 3U;
    } else if (image->format == MFT_PIXFMT_RGB24) {
        ihdr[8] = 8U;
        ihdr[9] = 2U;
    } else {
        ihdr[8] = 8U;
        ihdr[9] = 6U;
    }
    ihdr[10] = 0U;
    ihdr[11] = 0U;
    ihdr[12] = 0U;

    palette_entries = 0UL;
    trns_len = 0UL;
    mft_memzero(plte, (mft_u32)sizeof(plte));
    mft_memzero(trns, (mft_u32)sizeof(trns));
    if (image->format == MFT_PIXFMT_INDEX8) {
        png_index_max_used(image, &max_index);
        palette_entries = image->palette_count;
        if (palette_entries == 0UL) {
            return MFT_ERR_FORMAT;
        }
        if (max_index + 1UL > palette_entries) {
            palette_entries = max_index + 1UL;
        }
        if (palette_entries > 256UL) {
            return MFT_ERR_FORMAT;
        }
        for (y = 0UL; y < palette_entries; ++y) {
            plte[(y * 3UL) + 0UL] = image->palette[y].r;
            plte[(y * 3UL) + 1UL] = image->palette[y].g;
            plte[(y * 3UL) + 2UL] = image->palette[y].b;
            trns[y] = image->palette[y].a;
            if (image->palette[y].a != 255U) {
                trns_len = y + 1UL;
            }
        }
    }

    need = 8UL + (12UL + 13UL) + (12UL + idat_size) + 12UL;
    if (palette_entries > 0UL) {
        need += 12UL + (palette_entries * 3UL);
    }
    if (trns_len > 0UL) {
        need += 12UL + trns_len;
    }
    if (*dst_size < need) {
        *dst_size = need;
        return MFT_ERR_CAPACITY;
    }

    memcpy(dst, png_sig, 8U);
    pos = 8UL;
    st = png_write_chunk(dst, *dst_size, &pos, "IHDR", ihdr, 13UL);
    if (st != MFT_OK) {
        return st;
    }
    if (palette_entries > 0UL) {
        st = png_write_chunk(dst, *dst_size, &pos, "PLTE", plte, palette_entries * 3UL);
        if (st != MFT_OK) {
            return st;
        }
        if (trns_len > 0UL) {
            st = png_write_chunk(dst, *dst_size, &pos, "tRNS", trns, trns_len);
            if (st != MFT_OK) {
                return st;
            }
        }
    }
    st = png_write_chunk(dst, *dst_size, &pos, "IDAT", ws->idat, idat_size);
    if (st != MFT_OK) {
        return st;
    }
    st = png_write_chunk(dst, *dst_size, &pos, "IEND", 0, 0UL);
    if (st != MFT_OK) {
        return st;
    }

    *dst_size = pos;
    return MFT_OK;
}
