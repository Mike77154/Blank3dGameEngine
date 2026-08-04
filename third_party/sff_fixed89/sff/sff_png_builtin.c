#include "sff_png_builtin.h"
#include "../sublibs/png89/png89_zlib89.h"

static png89_u32 sff_png_builtin_flags(int tolerant)
{
    png89_u32 flags;
    flags = 0u;
    if (tolerant) flags |= PNG89_FLAG_ALLOW_MISSING_PLTE;
    return flags;
}

int sff_png_builtin_decode_indexed(const sff_u8 *blob, sff_u32 blob_len,
                                   int tolerant,
                                   sff_u8 *work_buf, sff_u32 work_buf_size,
                                   sff_u8 *out_pixels, sff_u32 out_pixels_size,
                                   sff_u16 *out_w, sff_u16 *out_h,
                                   sff_u8 out_pal_rgb[768], int *out_has_palette)
{
    Png89Info info;
    Zlib89Scratch inflate_scratch;
    int rc;

    if (!blob || !work_buf || !out_pixels) return 0;
    rc = png89_zlib89_decode_indexed8(blob, blob_len,
                                      sff_png_builtin_flags(tolerant),
                                      &inflate_scratch,
                                      work_buf, work_buf_size,
                                      out_pixels, out_pixels_size,
                                      out_pal_rgb, out_has_palette,
                                      &info);
    if (rc != PNG89_OK) return 0;
    if (info.width == 0u || info.height == 0u || info.width > 0xFFFFu || info.height > 0xFFFFu) return 0;
    if (out_w) *out_w = (sff_u16)info.width;
    if (out_h) *out_h = (sff_u16)info.height;
    return 1;
}

int sff_png_builtin_decode_rgba(const sff_u8 *blob, sff_u32 blob_len,
                                int tolerant,
                                sff_u8 *work_buf, sff_u32 work_buf_size,
                                sff_u8 *out_rgba, sff_u32 out_rgba_size,
                                sff_u16 *out_w, sff_u16 *out_h)
{
    Png89Info info;
    Zlib89Scratch inflate_scratch;
    int rc;

    if (!blob || !work_buf || !out_rgba) return 0;
    rc = png89_zlib89_decode_rgba8888(blob, blob_len,
                                      sff_png_builtin_flags(tolerant),
                                      &inflate_scratch,
                                      work_buf, work_buf_size,
                                      out_rgba, out_rgba_size,
                                      &info);
    if (rc != PNG89_OK) return 0;
    if (info.width == 0u || info.height == 0u || info.width > 0xFFFFu || info.height > 0xFFFFu) return 0;
    if (out_w) *out_w = (sff_u16)info.width;
    if (out_h) *out_h = (sff_u16)info.height;
    return 1;
}
