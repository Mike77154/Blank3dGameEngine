#include "png89.h"
#include <string.h>

#define PNG89_SIG_SIZE 8u
#define PNG89_IHDR_LEN 13u
#define PNG89_CHUNK_HDR 8u
#define PNG89_CHUNK_CRC 4u

static int png89_compute_requirements(Png89Info *info, Png89Requirements *req);

static png89_u32 png89_rd_be32(const png89_u8 *p)
{
    return ((png89_u32)p[0] << 24) | ((png89_u32)p[1] << 16) | ((png89_u32)p[2] << 8) | (png89_u32)p[3];
}

static png89_u16 png89_rd_be16(const png89_u8 *p)
{
    return (png89_u16)(((png89_u16)p[0] << 8) | (png89_u16)p[1]);
}

static int png89_mul_u32(png89_u32 a, png89_u32 b, png89_u32 *out)
{
    if (!out) return 0;
    if (a == 0u || b == 0u) { *out = 0u; return 1; }
    if (a > (png89_u32)(~(png89_u32)0) / b) return 0;
    *out = a * b;
    return 1;
}

static int png89_add_u32(png89_u32 a, png89_u32 b, png89_u32 *out)
{
    if (!out) return 0;
    if (a > (png89_u32)(~(png89_u32)0) - b) return 0;
    *out = a + b;
    return 1;
}

static png89_u8 png89_paeth(png89_u8 a, png89_u8 b, png89_u8 c)
{
    int p = (int)a + (int)b - (int)c;
    int pa = p > (int)a ? p - (int)a : (int)a - p;
    int pb = p > (int)b ? p - (int)b : (int)b - p;
    int pc = p > (int)c ? p - (int)c : (int)c - p;
    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

static png89_u32 png89_crc32_update(png89_u32 crc, const png89_u8 *buf, png89_u32 len)
{
    png89_u32 i;
    png89_u32 j;
    crc = crc ^ 0xFFFFFFFFul;
    for (i = 0u; i < len; ++i) {
        crc ^= (png89_u32)buf[i];
        for (j = 0u; j < 8u; ++j) {
            png89_u32 mask = (png89_u32)(0u - (crc & 1u));
            crc = (crc >> 1) ^ (0xEDB88320ul & mask);
        }
    }
    return crc ^ 0xFFFFFFFFul;
}

static int png89_bytes_per_pixel(png89_u8 color_type, png89_u8 bit_depth, png89_u32 *out_bpp)
{
    png89_u32 channels;
    if (!out_bpp) return 0;
    switch (color_type) {
        case 0: channels = 1u; break;
        case 2: channels = 3u; break;
        case 3: channels = 1u; break;
        case 4: channels = 2u; break;
        case 6: channels = 4u; break;
        default: return 0;
    }
    if (bit_depth >= 8u) {
        if (!png89_mul_u32(channels, (png89_u32)(bit_depth >> 3), out_bpp)) return 0;
    } else {
        *out_bpp = 1u;
    }
    return 1;
}

static int png89_scanline_bytes(png89_u32 width, png89_u8 color_type, png89_u8 bit_depth, png89_u32 *out_bytes)
{
    png89_u32 channels;
    png89_u32 bits;
    switch (color_type) {
        case 0: channels = 1u; break;
        case 2: channels = 3u; break;
        case 3: channels = 1u; break;
        case 4: channels = 2u; break;
        case 6: channels = 4u; break;
        default: return 0;
    }
    if (!png89_mul_u32(width, channels, &bits)) return 0;
    if (!png89_mul_u32(bits, bit_depth, &bits)) return 0;
    *out_bytes = (bits + 7u) >> 3;
    return 1;
}

static int png89_allowed_combo(png89_u8 color_type, png89_u8 bit_depth)
{
    switch (color_type) {
        case 0: return (bit_depth == 1u || bit_depth == 2u || bit_depth == 4u || bit_depth == 8u || bit_depth == 16u);
        case 2: return (bit_depth == 8u || bit_depth == 16u);
        case 3: return (bit_depth == 1u || bit_depth == 2u || bit_depth == 4u || bit_depth == 8u);
        case 4: return (bit_depth == 8u || bit_depth == 16u);
        case 6: return (bit_depth == 8u || bit_depth == 16u);
        default: return 0;
    }
}

static int png89_parse(const png89_u8 *data, png89_u32 size, png89_u32 flags,
                       Png89Info *info,
                       png89_u8 *idat_copy, png89_u32 idat_copy_cap)
{
    png89_u32 pos;
    int seen_ihdr = 0;
    int seen_iend = 0;
    int seen_plte = 0;
    int seen_idat = 0;

    if (!data || !info) return PNG89_EINVAL;
    if (size < PNG89_SIG_SIZE + PNG89_CHUNK_HDR + PNG89_IHDR_LEN + PNG89_CHUNK_CRC) return PNG89_EFORMAT;
    if (memcmp(data, "\x89PNG\r\n\x1a\n", 8) != 0) return PNG89_EFORMAT;

    memset(info, 0, sizeof(*info));
    pos = PNG89_SIG_SIZE;
    while (pos + PNG89_CHUNK_HDR + PNG89_CHUNK_CRC <= size) {
        png89_u32 length = png89_rd_be32(data + pos);
        const png89_u8 *type = data + pos + 4u;
        const png89_u8 *payload = data + pos + 8u;
        png89_u32 crc_expect;
        png89_u32 crc_calc;
        png89_u32 chunk_end;

        if (!png89_add_u32(pos, PNG89_CHUNK_HDR, &chunk_end)) return PNG89_ERANGE;
        if (!png89_add_u32(chunk_end, length, &chunk_end)) return PNG89_ERANGE;
        if (!png89_add_u32(chunk_end, PNG89_CHUNK_CRC, &chunk_end)) return PNG89_ERANGE;
        if (chunk_end > size) return PNG89_EFORMAT;

        crc_expect = png89_rd_be32(data + pos + 8u + length);
        crc_calc = png89_crc32_update(0u, type, 4u);
        crc_calc = png89_crc32_update(crc_calc, payload, length);
        if (!(flags & PNG89_FLAG_IGNORE_CRC) && crc_calc != crc_expect) return PNG89_ECRC;

        if (memcmp(type, "IHDR", 4) == 0) {
            if (seen_ihdr || length != PNG89_IHDR_LEN || seen_plte || seen_idat) return PNG89_EFORMAT;
            info->width = png89_rd_be32(payload + 0u);
            info->height = png89_rd_be32(payload + 4u);
            info->bit_depth = payload[8];
            info->color_type = payload[9];
            info->compression_method = payload[10];
            info->filter_method = payload[11];
            info->interlace_method = payload[12];
            if (info->width == 0u || info->height == 0u) return PNG89_EFORMAT;
            if (!png89_allowed_combo(info->color_type, info->bit_depth)) return PNG89_EUNSUPPORTED;
            if (info->compression_method != 0u || info->filter_method != 0u) return PNG89_EUNSUPPORTED;
            if (info->interlace_method > 1u) return PNG89_EUNSUPPORTED;
            seen_ihdr = 1;
        } else if (memcmp(type, "PLTE", 4) == 0) {
            png89_u32 n;
            if (!seen_ihdr || seen_plte || seen_idat) return PNG89_EFORMAT;
            if ((length % 3u) != 0u || length == 0u) return PNG89_EFORMAT;
            n = length / 3u;
            if (n > 256u) return PNG89_EFORMAT;
            info->palette_entries = (png89_u16)n;
            memset(info->palette_rgba, 255, sizeof(info->palette_rgba));
            {
                png89_u32 i;
                for (i = 0u; i < n; ++i) {
                    info->palette_rgba[i * 4u + 0u] = payload[i * 3u + 0u];
                    info->palette_rgba[i * 4u + 1u] = payload[i * 3u + 1u];
                    info->palette_rgba[i * 4u + 2u] = payload[i * 3u + 2u];
                }
            }
            info->has_plte = 1;
            seen_plte = 1;
        } else if (memcmp(type, "tRNS", 4) == 0) {
            if (!seen_ihdr || seen_idat) return PNG89_EFORMAT;
            info->has_trns = 1;
            if (info->color_type == 3u) {
                png89_u32 i;
                if (!seen_plte) return PNG89_EFORMAT;
                if (length > (png89_u32)info->palette_entries) return PNG89_EFORMAT;
                for (i = 0u; i < length; ++i) info->palette_rgba[i * 4u + 3u] = payload[i];
            } else if (info->color_type == 0u) {
                if (length != 2u) return PNG89_EFORMAT;
                info->transparent_gray = png89_rd_be16(payload);
            } else if (info->color_type == 2u) {
                if (length != 6u) return PNG89_EFORMAT;
                info->transparent_red = png89_rd_be16(payload + 0u);
                info->transparent_green = png89_rd_be16(payload + 2u);
                info->transparent_blue = png89_rd_be16(payload + 4u);
            } else {
                return PNG89_EFORMAT;
            }
        } else if (memcmp(type, "IDAT", 4) == 0) {
            if (!seen_ihdr) return PNG89_EFORMAT;
            if (info->color_type == 3u && !seen_plte && !(flags & PNG89_FLAG_ALLOW_MISSING_PLTE)) return PNG89_EFORMAT;
            if (idat_copy) {
                if (info->idat_size > idat_copy_cap || length > idat_copy_cap - info->idat_size) return PNG89_ENOSPC;
                memcpy(idat_copy + info->idat_size, payload, length);
            }
            if (!png89_add_u32(info->idat_size, length, &info->idat_size)) return PNG89_ERANGE;
            seen_idat = 1;
        } else if (memcmp(type, "IEND", 4) == 0) {
            if (!seen_ihdr || length != 0u) return PNG89_EFORMAT;
            seen_iend = 1;
            break;
        }
        pos = chunk_end;
    }
    if (!seen_ihdr || !seen_iend || !seen_idat) return PNG89_EFORMAT;
    return PNG89_OK;
}

int png89_read_info(const png89_u8 *data, png89_u32 size, png89_u32 flags, Png89Info *out_info)
{
    return png89_parse(data, size, flags, out_info, 0, 0u);
}

int png89_get_requirements(const png89_u8 *data, png89_u32 size, png89_u32 flags,
                           Png89Info *out_info, Png89Requirements *out_req)
{
    Png89Info info;
    int rc;
    if (!out_req) return PNG89_EINVAL;
    rc = png89_parse(data, size, flags, &info, 0, 0u);
    if (rc != PNG89_OK) return rc;
    rc = png89_compute_requirements(&info, out_req);
    if (rc != PNG89_OK) return rc;
    if (out_info) *out_info = info;
    return PNG89_OK;
}

static int png89_unfilter_row(png89_u8 filter, png89_u8 *row, const png89_u8 *prev, png89_u32 row_bytes, png89_u32 bpp)
{
    png89_u32 i;
    switch (filter) {
        case 0: return PNG89_OK;
        case 1:
            for (i = 0u; i < row_bytes; ++i) {
                png89_u8 left = (i >= bpp) ? row[i - bpp] : 0u;
                row[i] = (png89_u8)(row[i] + left);
            }
            return PNG89_OK;
        case 2:
            for (i = 0u; i < row_bytes; ++i) {
                png89_u8 up = prev ? prev[i] : 0u;
                row[i] = (png89_u8)(row[i] + up);
            }
            return PNG89_OK;
        case 3:
            for (i = 0u; i < row_bytes; ++i) {
                png89_u8 left = (i >= bpp) ? row[i - bpp] : 0u;
                png89_u8 up = prev ? prev[i] : 0u;
                row[i] = (png89_u8)(row[i] + (png89_u8)(((unsigned)left + (unsigned)up) >> 1));
            }
            return PNG89_OK;
        case 4:
            for (i = 0u; i < row_bytes; ++i) {
                png89_u8 left = (i >= bpp) ? row[i - bpp] : 0u;
                png89_u8 up = prev ? prev[i] : 0u;
                png89_u8 ul = (prev && i >= bpp) ? prev[i - bpp] : 0u;
                row[i] = (png89_u8)(row[i] + png89_paeth(left, up, ul));
            }
            return PNG89_OK;
        default:
            return PNG89_EFORMAT;
    }
}

static void png89_put_rgba(png89_u8 *dst, png89_u8 r, png89_u8 g, png89_u8 b, png89_u8 a)
{
    dst[0] = r; dst[1] = g; dst[2] = b; dst[3] = a;
}

static png89_u8 png89_scale_sample(png89_u32 v, png89_u8 bits)
{
    if (bits == 8u) return (png89_u8)v;
    if (bits == 1u) return (png89_u8)(v ? 255u : 0u);
    if (bits == 2u) return (png89_u8)(v * 85u);
    if (bits == 4u) return (png89_u8)(v * 17u);
    if (bits == 16u) return (png89_u8)(v >> 8);
    return 0u;
}

static png89_u32 png89_get_packed(const png89_u8 *row, png89_u32 x, png89_u8 bits)
{
    png89_u32 ppb = 8u / (png89_u32)bits;
    png89_u32 byte_index = x / ppb;
    png89_u32 shift = (ppb - 1u - (x % ppb)) * (png89_u32)bits;
    return ((png89_u32)row[byte_index] >> shift) & ((1u << bits) - 1u);
}

static void png89_write_pixel_from_row(const Png89Info *info, const png89_u8 *row, png89_u32 x, png89_u8 *dst_rgba)
{
    png89_u8 ct = info->color_type;
    png89_u8 bd = info->bit_depth;
    if (ct == 0u) {
        png89_u32 gray = (bd < 8u) ? png89_get_packed(row, x, bd) : (bd == 8u ? row[x] : png89_rd_be16(row + x * 2u));
        png89_u8 g = png89_scale_sample(gray, bd);
        png89_u8 a = 255u;
        if (info->has_trns) {
            if (bd == 16u) {
                if ((png89_u16)gray == info->transparent_gray) a = 0u;
            } else {
                png89_u16 tr = info->transparent_gray;
                if (bd < 8u) tr = (png89_u16)(tr & ((1u << bd) - 1u));
                if ((png89_u16)gray == tr) a = 0u;
            }
        }
        png89_put_rgba(dst_rgba, g, g, g, a);
    } else if (ct == 2u) {
        png89_u32 off = (bd == 8u) ? x * 3u : x * 6u;
        png89_u16 r16, g16, b16;
        png89_u8 a = 255u;
        if (bd == 8u) {
            r16 = row[off + 0u]; g16 = row[off + 1u]; b16 = row[off + 2u];
        } else {
            r16 = png89_rd_be16(row + off + 0u);
            g16 = png89_rd_be16(row + off + 2u);
            b16 = png89_rd_be16(row + off + 4u);
        }
        if (info->has_trns && r16 == info->transparent_red && g16 == info->transparent_green && b16 == info->transparent_blue) a = 0u;
        png89_put_rgba(dst_rgba, png89_scale_sample(r16, bd), png89_scale_sample(g16, bd), png89_scale_sample(b16, bd), a);
    } else if (ct == 3u) {
        png89_u8 idx = (png89_u8)((bd < 8u) ? png89_get_packed(row, x, bd) : row[x]);
        if (idx < info->palette_entries) {
            png89_put_rgba(dst_rgba,
                           info->palette_rgba[idx * 4u + 0u], info->palette_rgba[idx * 4u + 1u],
                           info->palette_rgba[idx * 4u + 2u], info->palette_rgba[idx * 4u + 3u]);
        } else {
            png89_put_rgba(dst_rgba, 255u, 0u, 255u, 255u);
        }
    } else if (ct == 4u) {
        png89_u32 off = (bd == 8u) ? x * 2u : x * 4u;
        png89_u16 g16 = (bd == 8u) ? row[off + 0u] : png89_rd_be16(row + off + 0u);
        png89_u16 a16 = (bd == 8u) ? row[off + 1u] : png89_rd_be16(row + off + 2u);
        png89_u8 g = png89_scale_sample(g16, bd);
        png89_u8 a = png89_scale_sample(a16, bd);
        png89_put_rgba(dst_rgba, g, g, g, a);
    } else if (ct == 6u) {
        png89_u32 off = (bd == 8u) ? x * 4u : x * 8u;
        png89_u16 r16 = (bd == 8u) ? row[off + 0u] : png89_rd_be16(row + off + 0u);
        png89_u16 g16 = (bd == 8u) ? row[off + 1u] : png89_rd_be16(row + off + 2u);
        png89_u16 b16 = (bd == 8u) ? row[off + 2u] : png89_rd_be16(row + off + 4u);
        png89_u16 a16 = (bd == 8u) ? row[off + 3u] : png89_rd_be16(row + off + 6u);
        png89_put_rgba(dst_rgba, png89_scale_sample(r16, bd), png89_scale_sample(g16, bd), png89_scale_sample(b16, bd), png89_scale_sample(a16, bd));
    }
}

static const png89_u8 png89_adam7_x0[7] = {0u, 4u, 0u, 2u, 0u, 1u, 0u};
static const png89_u8 png89_adam7_y0[7] = {0u, 0u, 4u, 0u, 2u, 0u, 1u};
static const png89_u8 png89_adam7_dx[7] = {8u, 8u, 4u, 4u, 2u, 2u, 1u};
static const png89_u8 png89_adam7_dy[7] = {8u, 8u, 8u, 4u, 4u, 2u, 2u};

static png89_u32 png89_pass_dim(png89_u32 full, png89_u8 start, png89_u8 step)
{
    if (full <= (png89_u32)start) return 0u;
    return (full - (png89_u32)start + (png89_u32)step - 1u) / (png89_u32)step;
}

static int png89_compute_requirements(Png89Info *info, Png89Requirements *req)
{
    png89_u32 row_bytes;
    png89_u32 bpp;
    png89_u32 raw_need;
    png89_u32 pass_need;

    if (!info || !req) return PNG89_EINVAL;
    if (!png89_scanline_bytes(info->width, info->color_type, info->bit_depth, &row_bytes)) return PNG89_EUNSUPPORTED;
    if (!png89_bytes_per_pixel(info->color_type, info->bit_depth, &bpp)) return PNG89_EUNSUPPORTED;

    raw_need = 0u;
    pass_need = row_bytes;
    if (info->interlace_method == 0u) {
        if (!png89_add_u32(row_bytes, 1u, &raw_need)) return PNG89_ERANGE;
        if (!png89_mul_u32(raw_need, info->height, &raw_need)) return PNG89_ERANGE;
    } else {
        png89_u32 total = 0u;
        png89_u32 p;
        pass_need = 0u;
        for (p = 0u; p < 7u; ++p) {
            png89_u32 pw = png89_pass_dim(info->width, png89_adam7_x0[p], png89_adam7_dx[p]);
            png89_u32 ph = png89_pass_dim(info->height, png89_adam7_y0[p], png89_adam7_dy[p]);
            png89_u32 pb;
            png89_u32 part;
            if (pw == 0u || ph == 0u) continue;
            if (!png89_scanline_bytes(pw, info->color_type, info->bit_depth, &pb)) return PNG89_EUNSUPPORTED;
            if (pb > pass_need) pass_need = pb;
            if (!png89_add_u32(pb, 1u, &part)) return PNG89_ERANGE;
            if (!png89_mul_u32(part, ph, &part)) return PNG89_ERANGE;
            if (!png89_add_u32(total, part, &total)) return PNG89_ERANGE;
        }
        raw_need = total;
    }

    info->raw_size = raw_need;
    req->row_bytes = row_bytes;
    req->bytes_per_pixel = bpp;
    req->idat_size = info->idat_size;
    req->inflate_out_size = raw_need;
    req->prev_row_size = pass_need;
    req->cur_row_size = row_bytes;
    req->pass_row_size = pass_need;
    req->scratch_size = info->idat_size + raw_need + pass_need + row_bytes + pass_need;
    return PNG89_OK;
}

static png89_u8 png89_get_index_from_row(const Png89Info *info, const png89_u8 *row, png89_u32 x)
{
    if (info->bit_depth < 8u) return (png89_u8)png89_get_packed(row, x, info->bit_depth);
    return row[x];
}

int png89_decode_indexed8(const png89_u8 *data, png89_u32 size,
                           png89_u32 flags,
                           const Png89InflateHooks *inflate,
                           const Png89Scratch *scratch,
                           png89_u8 *out_pixels, png89_u32 out_pixels_size,
                           png89_u8 out_pal_rgb[768], int *out_has_palette,
                           Png89Info *out_info)
{
    Png89Info info;
    Png89Requirements req;
    png89_u32 pixels;
    png89_u32 written;
    int rc;

    if (out_has_palette) *out_has_palette = 0;
    if (!data || !inflate || !inflate->inflate_zlib || !scratch || !out_pixels) return PNG89_EINVAL;
    rc = png89_parse(data, size, flags, &info, scratch->idat_data, scratch->idat_size);
    if (rc != PNG89_OK) return rc;
    if (info.color_type != 3u) return PNG89_EUNSUPPORTED;
    rc = png89_compute_requirements(&info, &req);
    if (rc != PNG89_OK) return rc;
    if (!png89_mul_u32(info.width, info.height, &pixels)) return PNG89_ERANGE;
    if (out_pixels_size < pixels) return PNG89_ENOSPC;
    if (scratch->cur_row_size < req.cur_row_size || scratch->prev_row_size < req.prev_row_size) return PNG89_ENOSPC;
    if (info.interlace_method != 0u && scratch->pass_row_size < req.pass_row_size) return PNG89_ENOSPC;
    if (scratch->inflate_out_size < req.inflate_out_size) return PNG89_ENOSPC;

    rc = inflate->inflate_zlib(inflate->user,
                               scratch->idat_data, info.idat_size,
                               scratch->inflate_out, req.inflate_out_size,
                               &written);
    if (rc != 0) return PNG89_EINFLATE;
    if (written != req.inflate_out_size) return PNG89_EFORMAT;

    if (info.interlace_method == 0u) {
        png89_u32 y;
        png89_u32 pos = 0u;
        memset(scratch->prev_row, 0, (size_t)req.prev_row_size);
        for (y = 0u; y < info.height; ++y) {
            png89_u32 x;
            png89_u8 filter = scratch->inflate_out[pos++];
            memcpy(scratch->cur_row, scratch->inflate_out + pos, (size_t)req.row_bytes);
            pos += req.row_bytes;
            rc = png89_unfilter_row(filter, scratch->cur_row, y ? scratch->prev_row : 0, req.row_bytes, req.bytes_per_pixel);
            if (rc != PNG89_OK) return rc;
            for (x = 0u; x < info.width; ++x) out_pixels[y * info.width + x] = png89_get_index_from_row(&info, scratch->cur_row, x);
            memcpy(scratch->prev_row, scratch->cur_row, (size_t)req.row_bytes);
        }
    } else {
        png89_u32 pos = 0u;
        png89_u32 p;
        for (p = 0u; p < 7u; ++p) {
            png89_u32 pw = png89_pass_dim(info.width, png89_adam7_x0[p], png89_adam7_dx[p]);
            png89_u32 ph = png89_pass_dim(info.height, png89_adam7_y0[p], png89_adam7_dy[p]);
            png89_u32 pb;
            png89_u32 y;
            if (pw == 0u || ph == 0u) continue;
            if (!png89_scanline_bytes(pw, info.color_type, info.bit_depth, &pb)) return PNG89_EUNSUPPORTED;
            memset(scratch->prev_row, 0, (size_t)pb);
            for (y = 0u; y < ph; ++y) {
                png89_u32 x;
                png89_u32 dst_y = (png89_u32)png89_adam7_y0[p] + y * (png89_u32)png89_adam7_dy[p];
                png89_u8 filter = scratch->inflate_out[pos++];
                memcpy(scratch->pass_row, scratch->inflate_out + pos, (size_t)pb);
                pos += pb;
                rc = png89_unfilter_row(filter, scratch->pass_row, y ? scratch->prev_row : 0, pb, req.bytes_per_pixel);
                if (rc != PNG89_OK) return rc;
                for (x = 0u; x < pw; ++x) {
                    png89_u32 dst_x = (png89_u32)png89_adam7_x0[p] + x * (png89_u32)png89_adam7_dx[p];
                    out_pixels[dst_y * info.width + dst_x] = png89_get_index_from_row(&info, scratch->pass_row, x);
                }
                memcpy(scratch->prev_row, scratch->pass_row, (size_t)pb);
            }
        }
    }

    if (out_pal_rgb) {
        png89_u32 i;
        memset(out_pal_rgb, 0, 768u);
        for (i = 0u; i < (png89_u32)info.palette_entries && i < 256u; ++i) {
            out_pal_rgb[i * 3u + 0u] = info.palette_rgba[i * 4u + 0u];
            out_pal_rgb[i * 3u + 1u] = info.palette_rgba[i * 4u + 1u];
            out_pal_rgb[i * 3u + 2u] = info.palette_rgba[i * 4u + 2u];
        }
    }
    if (out_has_palette) *out_has_palette = info.has_plte ? 1 : 0;
    if (out_info) *out_info = info;
    return PNG89_OK;
}

int png89_decode_rgba8888(const png89_u8 *data, png89_u32 size,
                          png89_u32 flags,
                          const Png89InflateHooks *inflate,
                          const Png89Scratch *scratch,
                          png89_u8 *out_rgba, png89_u32 out_rgba_size,
                          Png89Info *out_info)
{
    Png89Info info;
    Png89Requirements req;
    png89_u32 pixels;
    png89_u32 written;
    int rc;

    if (!data || !inflate || !inflate->inflate_zlib || !scratch || !out_rgba) return PNG89_EINVAL;
    rc = png89_parse(data, size, flags, &info, scratch->idat_data, scratch->idat_size);
    if (rc != PNG89_OK) return rc;
    rc = png89_compute_requirements(&info, &req);
    if (rc != PNG89_OK) return rc;
    if (!png89_mul_u32(info.width, info.height, &pixels)) return PNG89_ERANGE;
    if (!png89_mul_u32(pixels, 4u, &pixels)) return PNG89_ERANGE;
    if (out_rgba_size < pixels) return PNG89_ENOSPC;
    if (scratch->cur_row_size < req.cur_row_size || scratch->prev_row_size < req.prev_row_size) return PNG89_ENOSPC;
    if (info.interlace_method != 0u && scratch->pass_row_size < req.pass_row_size) return PNG89_ENOSPC;
    if (scratch->inflate_out_size < req.inflate_out_size) return PNG89_ENOSPC;

    rc = inflate->inflate_zlib(inflate->user,
                               scratch->idat_data, info.idat_size,
                               scratch->inflate_out, req.inflate_out_size,
                               &written);
    if (rc != 0) return PNG89_EINFLATE;
    if (written != req.inflate_out_size) return PNG89_EFORMAT;

    memset(out_rgba, 0, (size_t)pixels);
    if (info.interlace_method == 0u) {
        png89_u32 y;
        png89_u32 pos = 0u;
        memset(scratch->prev_row, 0, (size_t)req.prev_row_size);
        for (y = 0u; y < info.height; ++y) {
            png89_u8 filter = scratch->inflate_out[pos++];
            memcpy(scratch->cur_row, scratch->inflate_out + pos, (size_t)req.row_bytes);
            pos += req.row_bytes;
            rc = png89_unfilter_row(filter, scratch->cur_row, y ? scratch->prev_row : 0, req.row_bytes, req.bytes_per_pixel);
            if (rc != PNG89_OK) return rc;
            {
                png89_u32 x;
                for (x = 0u; x < info.width; ++x) png89_write_pixel_from_row(&info, scratch->cur_row, x, out_rgba + (y * info.width + x) * 4u);
            }
            memcpy(scratch->prev_row, scratch->cur_row, (size_t)req.row_bytes);
        }
    } else {
        png89_u32 pos = 0u;
        png89_u32 p;
        for (p = 0u; p < 7u; ++p) {
            png89_u32 pw = png89_pass_dim(info.width, png89_adam7_x0[p], png89_adam7_dx[p]);
            png89_u32 ph = png89_pass_dim(info.height, png89_adam7_y0[p], png89_adam7_dy[p]);
            png89_u32 pb;
            png89_u32 y;
            if (pw == 0u || ph == 0u) continue;
            if (!png89_scanline_bytes(pw, info.color_type, info.bit_depth, &pb)) return PNG89_EUNSUPPORTED;
            memset(scratch->prev_row, 0, (size_t)pb);
            for (y = 0u; y < ph; ++y) {
                png89_u32 x;
                png89_u32 dst_y = (png89_u32)png89_adam7_y0[p] + y * (png89_u32)png89_adam7_dy[p];
                png89_u8 filter = scratch->inflate_out[pos++];
                memcpy(scratch->pass_row, scratch->inflate_out + pos, (size_t)pb);
                pos += pb;
                rc = png89_unfilter_row(filter, scratch->pass_row, y ? scratch->prev_row : 0, pb, req.bytes_per_pixel);
                if (rc != PNG89_OK) return rc;
                for (x = 0u; x < pw; ++x) {
                    png89_u32 dst_x = (png89_u32)png89_adam7_x0[p] + x * (png89_u32)png89_adam7_dx[p];
                    png89_write_pixel_from_row(&info, scratch->pass_row, x, out_rgba + (dst_y * info.width + dst_x) * 4u);
                }
                memcpy(scratch->prev_row, scratch->pass_row, (size_t)pb);
            }
        }
    }

    if (out_info) *out_info = info;
    return PNG89_OK;
}
