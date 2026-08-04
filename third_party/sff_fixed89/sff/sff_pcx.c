#include "sff_pcx.h"
#include "../sublibs/pcx89/pcx89.h"
#include <string.h>

int sff_pcx_is_8bpp_rle(const sff_u8 *raw, sff_u32 raw_len)
{
    Pcx89Info info;
    int rc;
    if (!raw) return 0;
    rc = pcx89_read_info(raw, raw_len, &info);
    if (rc != PCX89_OK) return 0;
    if (info.hdr.encoding != 1u) return 0;
    return (info.hdr.bits_per_pixel == 8u && info.hdr.num_planes == 1u) ? 1 : 0;
}

int sff_pcx_peek_dims(const sff_u8 *raw, sff_u32 raw_len, sff_u16 *out_w, sff_u16 *out_h)
{
    Pcx89Info info;
    int rc;
    if (!raw) return 0;
    rc = pcx89_read_info(raw, raw_len, &info);
    if (rc != PCX89_OK) return 0;
    if (info.width == 0u || info.height == 0u || info.width > 0xFFFFu || info.height > 0xFFFFu) return 0;
    if (out_w) *out_w = (sff_u16)info.width;
    if (out_h) *out_h = (sff_u16)info.height;
    return 1;
}

int sff_pcx_has_palette(const sff_u8 *raw, sff_u32 raw_len)
{
    Pcx89Info info;
    int rc;
    if (!raw) return 0;
    rc = pcx89_read_info(raw, raw_len, &info);
    if (rc != PCX89_OK) return 0;
    return info.has_vga_palette ? 1 : 0;
}

int sff_pcx_extract_palette_rgb(const sff_u8 *raw, sff_u32 raw_len, sff_u8 out_pal_rgb[768])
{
    Pcx89Info info;
    int rc;
    if (!raw || !out_pal_rgb) return 0;
    rc = pcx89_read_info(raw, raw_len, &info);
    if (rc != PCX89_OK) return 0;
    if (info.has_vga_palette) {
        return pcx89_extract_vga_palette_rgb(raw, raw_len, out_pal_rgb) == PCX89_OK ? 1 : 0;
    }
    return 0;
}

int sff_pcx_decode_8bpp_into(const sff_u8 *raw, sff_u32 raw_len,
                             sff_u8 *out_pixels, sff_u32 out_pixels_size,
                             sff_u16 *out_w, sff_u16 *out_h,
                             sff_u8 out_pal_rgb[768], int *out_has_pal)
{
    Pcx89Info info;
    sff_u32 need;
    sff_u32 row_bytes;
    sff_u32 i;
    sff_u32 y;
    sff_u32 limit;
    int rc;

    if (out_has_pal) *out_has_pal = 0;
    if (!raw || !out_pixels) return 0;
    rc = pcx89_read_info(raw, raw_len, &info);
    if (rc != PCX89_OK) return 0;
    if (info.hdr.bits_per_pixel != 8u || info.hdr.num_planes != 1u) return 0;

    need = (sff_u32)info.width * (sff_u32)info.height;
    if (info.width != 0u && need / (sff_u32)info.width != (sff_u32)info.height) return 0;
    if (out_pixels_size < need) return 0;
    memset(out_pixels, 0, (size_t)need);

    row_bytes = (sff_u32)info.hdr.bytes_per_line;
    if (row_bytes == 0u) row_bytes = info.width;
    if (row_bytes < info.width) row_bytes = info.width;

    limit = raw_len;
    if (info.has_vga_palette) {
        if (limit < (128u + 769u)) return 0;
        limit -= 769u;
    }

    i = 128u;
    for (y = 0u; y < info.height; ++y) {
        sff_u32 col;
        col = 0u;
        while (col < row_bytes) {
            sff_u8 byte_;
            if (i >= limit) return 0;
            byte_ = raw[i++];
            if ((byte_ & 0xC0u) == 0xC0u) {
                sff_u32 count;
                sff_u8 val;
                if (i >= limit) return 0;
                count = (sff_u32)(byte_ & 0x3Fu);
                val = raw[i++];
                while (count-- != 0u) {
                    if (col >= row_bytes) return 0;
                    if (col < info.width) {
                        out_pixels[y * info.width + col] = val;
                    }
                    ++col;
                }
            } else {
                if (col < info.width) out_pixels[y * info.width + col] = byte_;
                ++col;
            }
        }
    }

    if (info.has_vga_palette) {
        if (out_pal_rgb) {
            memcpy(out_pal_rgb, raw + raw_len - 768u, 768u);
        }
        if (out_has_pal) *out_has_pal = 1;
    }

    if (out_w) *out_w = (sff_u16)info.width;
    if (out_h) *out_h = (sff_u16)info.height;
    return 1;
}
