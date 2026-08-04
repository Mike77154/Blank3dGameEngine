#include "pcx89.h"
#include <string.h>

#define PCX89_HEADER_SIZE 128u

static pcx89_u16 pcx89_rd_le16(const pcx89_u8 *p)
{
    return (pcx89_u16)((pcx89_u16)p[0] | ((pcx89_u16)p[1] << 8));
}

static int pcx89_mul_u32(pcx89_u32 a, pcx89_u32 b, pcx89_u32 *out)
{
    if (!out) return 0;
    if (a == 0u || b == 0u) {
        *out = 0u;
        return 1;
    }
    if (a > (pcx89_u32)(~(pcx89_u32)0) / b) return 0;
    *out = a * b;
    return 1;
}

static void pcx89_expand_ega6_to_8(const pcx89_u8 *in48, pcx89_u8 out16x3[48])
{
    pcx89_u32 i;
    for (i = 0u; i < 16u; ++i) {
        out16x3[i * 3u + 0u] = (pcx89_u8)(in48[i * 3u + 0u] << 2);
        out16x3[i * 3u + 1u] = (pcx89_u8)(in48[i * 3u + 1u] << 2);
        out16x3[i * 3u + 2u] = (pcx89_u8)(in48[i * 3u + 2u] << 2);
    }
}

static int pcx89_measure_image_end(const pcx89_u8 *data, pcx89_u32 size,
                                   const Pcx89Info *info, pcx89_u32 limit,
                                   pcx89_u32 *out_end)
{
    pcx89_u32 required;
    pcx89_u32 produced;
    pcx89_u32 pos;

    if (!data || !info || !out_end) return 0;
    if (limit > size || limit < PCX89_HEADER_SIZE) return 0;
    if (!pcx89_mul_u32(info->decoded_scanline_bytes, info->height, &required)) return 0;

    if (info->hdr.encoding == 0u) {
        if (required > limit - PCX89_HEADER_SIZE) return 0;
        *out_end = PCX89_HEADER_SIZE + required;
        return 1;
    }
    if (info->hdr.encoding != 1u) return 0;

    produced = 0u;
    pos = PCX89_HEADER_SIZE;
    while (produced < required) {
        pcx89_u8 b;
        pcx89_u32 count;
        if (pos >= limit) return 0;
        b = data[pos++];
        count = 1u;
        if ((b & 0xC0u) == 0xC0u) {
            count = (pcx89_u32)(b & 0x3Fu);
            if (count == 0u || pos >= limit) return 0;
            ++pos;
        }
        if (count > required - produced) return 0;
        produced += count;
    }

    *out_end = pos;
    return 1;
}

static int pcx89_has_vga_palette(const pcx89_u8 *data, pcx89_u32 size, const Pcx89Info *info)
{
    pcx89_u32 marker;
    pcx89_u32 image_end;

    if (!data || !info) return 0;
    if (size < PCX89_HEADER_SIZE + 769u) return 0;
    if (info->hdr.bits_per_pixel != 8u || info->hdr.num_planes != 1u) return 0;

    marker = size - 769u;
    if (data[marker] != 12u) return 0;

    /* A bare marker check is not enough for SFF v1. Palette-less PCX blobs
       frequently contain byte 0x0C exactly 769 bytes from the end by chance.
       Confirm that a complete image stream ends before the candidate marker. */
    if (!pcx89_measure_image_end(data, size, info, marker, &image_end)) return 0;
    return image_end <= marker ? 1 : 0;
}

int pcx89_read_info(const pcx89_u8 *data, pcx89_u32 size, Pcx89Info *out_info)
{
    Pcx89Info info;
    pcx89_u32 width;
    pcx89_u32 height;
    pcx89_u32 scanline;

    if (!data || !out_info) return PCX89_EINVAL;
    if (size < PCX89_HEADER_SIZE) return PCX89_EFORMAT;
    if (data[0] != 0x0Au) return PCX89_EFORMAT;
    if (data[2] != 0x00u && data[2] != 0x01u) return PCX89_EUNSUPPORTED;

    memset(&info, 0, sizeof(info));
    info.hdr.manufacturer = data[0];
    info.hdr.version = data[1];
    info.hdr.encoding = data[2];
    info.hdr.bits_per_pixel = data[3];
    info.hdr.xmin = pcx89_rd_le16(data + 4u);
    info.hdr.ymin = pcx89_rd_le16(data + 6u);
    info.hdr.xmax = pcx89_rd_le16(data + 8u);
    info.hdr.ymax = pcx89_rd_le16(data + 10u);
    info.hdr.hdpi = pcx89_rd_le16(data + 12u);
    info.hdr.vdpi = pcx89_rd_le16(data + 14u);
    memcpy(info.hdr.ega_palette, data + 16u, 48u);
    info.hdr.reserved0 = data[64];
    info.hdr.num_planes = data[65];
    info.hdr.bytes_per_line = pcx89_rd_le16(data + 66u);
    info.hdr.palette_info = pcx89_rd_le16(data + 68u);
    info.hdr.hscreen_size = pcx89_rd_le16(data + 70u);
    info.hdr.vscreen_size = pcx89_rd_le16(data + 72u);

    if (info.hdr.xmax < info.hdr.xmin || info.hdr.ymax < info.hdr.ymin) return PCX89_EFORMAT;
    width = (pcx89_u32)(info.hdr.xmax - info.hdr.xmin) + 1u;
    height = (pcx89_u32)(info.hdr.ymax - info.hdr.ymin) + 1u;
    if (width == 0u || height == 0u) return PCX89_EFORMAT;
    if (info.hdr.num_planes == 0u) return PCX89_EFORMAT;
    if (info.hdr.bytes_per_line == 0u) return PCX89_EFORMAT;
    if (!pcx89_mul_u32((pcx89_u32)info.hdr.num_planes, (pcx89_u32)info.hdr.bytes_per_line, &scanline)) return PCX89_ERANGE;

    info.width = width;
    info.height = height;
    info.decoded_scanline_bytes = scanline;
    info.has_vga_palette = pcx89_has_vga_palette(data, size, &info);
    info.is_supported = 0;

    if (info.hdr.bits_per_pixel == 1u &&
        (info.hdr.num_planes == 1u || info.hdr.num_planes == 2u || info.hdr.num_planes == 3u || info.hdr.num_planes == 4u)) {
        info.is_supported = 1;
    } else if ((info.hdr.bits_per_pixel == 2u || info.hdr.bits_per_pixel == 4u) && info.hdr.num_planes == 1u) {
        info.is_supported = 1;
    } else if (info.hdr.bits_per_pixel == 8u &&
               (info.hdr.num_planes == 1u || info.hdr.num_planes == 3u || info.hdr.num_planes == 4u)) {
        info.is_supported = 1;
    }

    *out_info = info;
    return info.is_supported ? PCX89_OK : PCX89_EUNSUPPORTED;
}

int pcx89_extract_vga_palette_rgb(const pcx89_u8 *data, pcx89_u32 size, pcx89_u8 out_rgb[768])
{
    Pcx89Info info;
    int rc;
    if (!out_rgb) return PCX89_EINVAL;
    rc = pcx89_read_info(data, size, &info);
    if (rc != PCX89_OK) return rc;
    if (!pcx89_has_vga_palette(data, size, &info)) return PCX89_EFORMAT;
    memcpy(out_rgb, data + size - 768u, 768u);
    return PCX89_OK;
}

static int pcx89_decode_scanline(const pcx89_u8 *data, pcx89_u32 data_len,
                                 pcx89_u32 *io_pos,
                                 pcx89_u8 *scanline, pcx89_u32 scanline_size)
{
    pcx89_u32 out_pos = 0u;
    pcx89_u32 pos;
    if (!data || !io_pos || !scanline) return PCX89_EINVAL;
    pos = *io_pos;
    while (out_pos < scanline_size) {
        pcx89_u8 b;
        if (pos >= data_len) return PCX89_EIO;
        b = data[pos++];
        if ((b & 0xC0u) == 0xC0u) {
            pcx89_u32 count = (pcx89_u32)(b & 0x3Fu);
            pcx89_u8 val;
            if (pos >= data_len) return PCX89_EIO;
            val = data[pos++];
            while (count--) {
                if (out_pos >= scanline_size) return PCX89_EFORMAT;
                scanline[out_pos++] = val;
            }
        } else {
            scanline[out_pos++] = b;
        }
    }
    *io_pos = pos;
    return PCX89_OK;
}

static void pcx89_put_rgba(pcx89_u8 *dst, pcx89_u8 r, pcx89_u8 g, pcx89_u8 b, pcx89_u8 a)
{
    dst[0] = r;
    dst[1] = g;
    dst[2] = b;
    dst[3] = a;
}

static void pcx89_decode_row_rgba(const Pcx89Info *info, const pcx89_u8 *scanline, pcx89_u8 *dst_row,
                                  const pcx89_u8 *pal16, const pcx89_u8 *pal256)
{
    pcx89_u32 x;
    pcx89_u32 w = info->width;
    pcx89_u32 bpl = info->hdr.bytes_per_line;
    pcx89_u8 bpp = info->hdr.bits_per_pixel;
    pcx89_u8 planes = info->hdr.num_planes;

    if (bpp == 8u && planes == 3u) {
        const pcx89_u8 *r = scanline + (bpl * 0u);
        const pcx89_u8 *g = scanline + (bpl * 1u);
        const pcx89_u8 *b = scanline + (bpl * 2u);
        for (x = 0u; x < w; ++x) pcx89_put_rgba(dst_row + x * 4u, r[x], g[x], b[x], 255u);
        return;
    }
    if (bpp == 8u && planes == 4u) {
        const pcx89_u8 *r = scanline + (bpl * 0u);
        const pcx89_u8 *g = scanline + (bpl * 1u);
        const pcx89_u8 *b = scanline + (bpl * 2u);
        const pcx89_u8 *a = scanline + (bpl * 3u);
        for (x = 0u; x < w; ++x) pcx89_put_rgba(dst_row + x * 4u, r[x], g[x], b[x], a[x]);
        return;
    }
    if (bpp == 8u && planes == 1u) {
        for (x = 0u; x < w; ++x) {
            pcx89_u8 idx = scanline[x];
            if (pal256) {
                pcx89_put_rgba(dst_row + x * 4u,
                               pal256[idx * 3u + 0u], pal256[idx * 3u + 1u], pal256[idx * 3u + 2u], 255u);
            } else if (pal16) {
                pcx89_u8 i16 = (pcx89_u8)(idx & 15u);
                pcx89_put_rgba(dst_row + x * 4u,
                               pal16[i16 * 3u + 0u], pal16[i16 * 3u + 1u], pal16[i16 * 3u + 2u], 255u);
            } else {
                pcx89_put_rgba(dst_row + x * 4u, idx, idx, idx, 255u);
            }
        }
        return;
    }

    if (bpp == 1u && (planes >= 1u && planes <= 4u)) {
        for (x = 0u; x < w; ++x) {
            pcx89_u8 idx = 0u;
            pcx89_u8 p;
            pcx89_u8 bit = (pcx89_u8)(7u - (x & 7u));
            pcx89_u32 byte_index = x >> 3;
            for (p = 0u; p < planes; ++p) {
                idx |= (pcx89_u8)(((scanline[(pcx89_u32)p * bpl + byte_index] >> bit) & 1u) << p);
            }
            pcx89_put_rgba(dst_row + x * 4u,
                           pal16[idx * 3u + 0u], pal16[idx * 3u + 1u], pal16[idx * 3u + 2u], 255u);
        }
        return;
    }

    if ((bpp == 2u || bpp == 4u) && planes == 1u) {
        pcx89_u32 ppb = 8u / (pcx89_u32)bpp;
        pcx89_u32 mask = (1u << bpp) - 1u;
        for (x = 0u; x < w; ++x) {
            pcx89_u32 byte_index = x / ppb;
            pcx89_u32 shift = (ppb - 1u - (x % ppb)) * (pcx89_u32)bpp;
            pcx89_u8 idx = (pcx89_u8)((scanline[byte_index] >> shift) & mask);
            pcx89_put_rgba(dst_row + x * 4u,
                           pal16[idx * 3u + 0u], pal16[idx * 3u + 1u], pal16[idx * 3u + 2u], 255u);
        }
        return;
    }

    for (x = 0u; x < w; ++x) pcx89_put_rgba(dst_row + x * 4u, 255u, 0u, 255u, 255u);
}

int pcx89_decode_indexed8(const pcx89_u8 *data, pcx89_u32 size,
                          pcx89_u8 *out_pixels, pcx89_u32 out_pixels_size,
                          pcx89_u8 out_pal_rgb[768], int *out_has_palette,
                          const Pcx89Scratch *scratch,
                          Pcx89Info *out_info)
{
    Pcx89Info info;
    pcx89_u8 ega_pal[48];
    const pcx89_u8 *vga_pal = 0;
    pcx89_u32 y;
    pcx89_u32 pos;
    int rc;

    if (out_has_palette) *out_has_palette = 0;
    if (!data || !out_pixels || !scratch || !scratch->scanline) return PCX89_EINVAL;
    rc = pcx89_read_info(data, size, &info);
    if (rc != PCX89_OK) return rc;
    if (info.hdr.bits_per_pixel != 8u || info.hdr.num_planes != 1u) return PCX89_EUNSUPPORTED;
    if (out_pixels_size < info.width * info.height) return PCX89_ENOSPC;
    if (scratch->scanline_size < info.decoded_scanline_bytes) return PCX89_ENOSPC;

    pcx89_expand_ega6_to_8(info.hdr.ega_palette, ega_pal);
    if (info.has_vga_palette) vga_pal = data + size - 768u;

    pos = PCX89_HEADER_SIZE;
    for (y = 0u; y < info.height; ++y) {
        rc = pcx89_decode_scanline(data, size - (info.has_vga_palette ? 769u : 0u), &pos,
                                   scratch->scanline, info.decoded_scanline_bytes);
        if (rc != PCX89_OK) return rc;
        memcpy(out_pixels + y * info.width, scratch->scanline, (size_t)info.width);
    }

    if (out_pal_rgb) {
        if (vga_pal) {
            memcpy(out_pal_rgb, vga_pal, 768u);
            if (out_has_palette) *out_has_palette = 1;
        } else {
            pcx89_u32 i;
            for (i = 0u; i < 256u; ++i) {
                pcx89_u8 idx = (pcx89_u8)(i & 15u);
                out_pal_rgb[i * 3u + 0u] = ega_pal[idx * 3u + 0u];
                out_pal_rgb[i * 3u + 1u] = ega_pal[idx * 3u + 1u];
                out_pal_rgb[i * 3u + 2u] = ega_pal[idx * 3u + 2u];
            }
            if (out_has_palette) *out_has_palette = 1;
        }
    }

    if (out_info) *out_info = info;
    return PCX89_OK;
}

int pcx89_decode_rgba8888(const pcx89_u8 *data, pcx89_u32 size,
                          pcx89_u8 *out_rgba, pcx89_u32 out_rgba_size,
                          const Pcx89Scratch *scratch,
                          Pcx89Info *out_info)
{
    Pcx89Info info;
    pcx89_u32 need;
    pcx89_u8 ega_pal[48];
    const pcx89_u8 *vga_pal = 0;
    pcx89_u32 y;
    pcx89_u32 pos;
    int rc;

    if (!data || !out_rgba || !scratch || !scratch->scanline) return PCX89_EINVAL;
    rc = pcx89_read_info(data, size, &info);
    if (rc != PCX89_OK) return rc;
    if (!pcx89_mul_u32(info.width, info.height, &need)) return PCX89_ERANGE;
    if (!pcx89_mul_u32(need, 4u, &need)) return PCX89_ERANGE;
    if (out_rgba_size < need) return PCX89_ENOSPC;
    if (scratch->scanline_size < info.decoded_scanline_bytes) return PCX89_ENOSPC;

    pcx89_expand_ega6_to_8(info.hdr.ega_palette, ega_pal);
    if (info.has_vga_palette) vga_pal = data + size - 768u;

    pos = PCX89_HEADER_SIZE;
    for (y = 0u; y < info.height; ++y) {
        rc = pcx89_decode_scanline(data, size - (info.has_vga_palette ? 769u : 0u), &pos,
                                   scratch->scanline, info.decoded_scanline_bytes);
        if (rc != PCX89_OK) return rc;
        pcx89_decode_row_rgba(&info, scratch->scanline, out_rgba + y * info.width * 4u, ega_pal, vga_pal);
    }

    if (out_info) *out_info = info;
    return PCX89_OK;
}
