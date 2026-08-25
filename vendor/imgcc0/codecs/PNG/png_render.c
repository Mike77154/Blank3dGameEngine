/* png_render.c
 * Convert inflated PNG image data (filtered scanlines) + png_state into RGBA8.
 */

#include "png_mem89.h"
#include <string.h>

#include "png_decoder_internal.h"

static int png_compute_uncompressed_size(const png_state* st, png_u32* out_size);

static png_u16 read_be16(const png_u8* p)
{
    return (png_u16)(((png_u16)p[0] << 8) | (png_u16)p[1]);
}

static png_u32 png_channels_for_color_type(png_u8 color_type)
{
    switch (color_type)
    {
        case PNG_COLOR_GRAYSCALE:       return 1u;
        case PNG_COLOR_TRUECOLOR:       return 3u;
        case PNG_COLOR_INDEXED:         return 1u;
        case PNG_COLOR_GRAYSCALE_ALPHA: return 2u;
        case PNG_COLOR_TRUECOLOR_ALPHA: return 4u;
        default:                        return 0u;
    }
}

png_u32 png_rowbytes_for_width_internal(const png_state* st, png_u32 width)
{
    png_u32 channels;

    if (!st)
        return 0u;

    channels = png_channels_for_color_type(st->color_type);
    if (channels == 0u)
        return 0u;

    /* ceil(width * channels * bit_depth / 8) */
    return ((width * channels * (png_u32)st->bit_depth) + 7u) / 8u;
}

png_u32 png_filter_bpp_bytes_internal(const png_state* st)
{
    png_u32 channels;
    png_u32 bits_per_pixel;

    if (!st)
        return 0u;

    channels = png_channels_for_color_type(st->color_type);
    if (channels == 0u)
        return 0u;

    bits_per_pixel = (png_u32)st->bit_depth * channels;
    return (bits_per_pixel + 7u) / 8u; /* ceil */
}

static png_u8 expand_sub8(png_u8 byte, png_u8 shift, png_u8 depth, png_u8* out_raw)
{
    png_u8 mask;
    png_u8 s;

    mask = (png_u8)((1u << depth) - 1u);
    s = (png_u8)((byte >> shift) & mask);

    if (out_raw)
        *out_raw = s;

    if (depth == 1u) return s ? 255u : 0u;
    if (depth == 2u) return (png_u8)(s * 85u);
    if (depth == 4u) return (png_u8)(s * 17u);

    return s;
}

static png_u8 from_16_to_8(png_u16 v)
{
    return (png_u8)(v >> 8);
}


static png_u16 expand_8_to_16(png_u8 v)
{
    return (png_u16)(((png_u16)v << 8) | (png_u16)v);
}

static void write_be16_out(png_u8* p, png_u16 v)
{
    p[0] = (png_u8)(v >> 8);
    p[1] = (png_u8)(v & 0xFFu);
}

#if !PNG_DEC_DISABLE_GAMMA
static png_u16 png_apply_gamma_16_scalar(const png_state* st, png_u16 v)
{
    png_fixed89 img_gamma;
    png_fixed89 exponent;
    png_fixed89 in_v;
    png_fixed89 outv;

    if (!st || !st->use_gamma_lut)
        return v;

    img_gamma = st->have_srgb ? png_fixed89_from_ratio(45455, 100000) : st->image_gamma;
    if (img_gamma <= 0 || img_gamma < png_fixed89_from_ratio(1, 100) || img_gamma > (10 * PNG_FIXED89_ONE))
        return v;

    exponent = png_fixed89_div(img_gamma, png_fixed89_from_ratio(22, 10));
    in_v = png_fixed89_from_ratio((signed int)v, 65535);
    outv = png_fixed89_pow_unit(in_v, exponent);
    return (png_u16)png_fixed89_unit_to_u16(outv);
}
#else
static png_u16 png_apply_gamma_16_scalar(const png_state* st, png_u16 v)
{
    (void)st;
    return v;
}
#endif


static int palette_lookup(const png_state* st, png_u8 idx,
                          png_u8* r, png_u8* g, png_u8* b, png_u8* a)
{
    png_u32 pos;

    if (!st || !r || !g || !b || !a)
        return PNG_DEC_ERR_FORMAT;

    if (!st->have_palette || !st->palette)
        return PNG_DEC_ERR_FORMAT;

    if (idx >= st->palette_entries)
        return PNG_DEC_ERR_FORMAT;

    pos = (png_u32)idx * 3u;

    *r = st->palette[pos + 0u];
    *g = st->palette[pos + 1u];
    *b = st->palette[pos + 2u];

    if (st->has_trns && st->trns_data && st->trns_size > (png_u32)idx)
        *a = st->trns_data[idx];
    else
        *a = 255u;

    return PNG_DEC_OK;
}

int png_render_scanline_rgba8(const png_state* st,
                           const png_u8* scan,
                           png_u8* out_rgba,
                           png_u32 width)
{
    png_u32 x;
    const png_u8* p;

    if (!st || !scan || !out_rgba)
        return PNG_DEC_ERR_FORMAT;

    p = scan;

    /* -------- Truecolor (RGB) -------- */
    if (st->color_type == PNG_COLOR_TRUECOLOR && st->bit_depth == 8u)
    {
        png_u8 tr, tg, tb;
        int have_trns;

        have_trns = (st->has_trns && st->trns_data && st->trns_size >= 6u) ? 1 : 0;
        tr = have_trns ? st->trns_data[1] : 0;
        tg = have_trns ? st->trns_data[3] : 0;
        tb = have_trns ? st->trns_data[5] : 0;

        for (x = 0; x < width; ++x)
        {
            png_u8 r = p[0];
            png_u8 g = p[1];
            png_u8 b = p[2];
            png_u8 a = 255u;
            p += 3;

            if (have_trns && r == tr && g == tg && b == tb)
                a = 0u;

            if (st->use_gamma_lut) {
                r = st->gamma_lut[r];
                g = st->gamma_lut[g];
                b = st->gamma_lut[b];
            }

            out_rgba[0] = r;
            out_rgba[1] = g;
            out_rgba[2] = b;
            out_rgba[3] = a;
            out_rgba += 4;
        }
        return PNG_DEC_OK;
    }

    if (st->color_type == PNG_COLOR_TRUECOLOR && st->bit_depth == 16u)
    {
        png_u16 tr16, tg16, tb16;
        int have_trns;

        have_trns = (st->has_trns && st->trns_data && st->trns_size >= 6u) ? 1 : 0;
        tr16 = have_trns ? read_be16(st->trns_data + 0) : 0;
        tg16 = have_trns ? read_be16(st->trns_data + 2) : 0;
        tb16 = have_trns ? read_be16(st->trns_data + 4) : 0;

        for (x = 0; x < width; ++x)
        {
            png_u16 r16 = read_be16(p + 0);
            png_u16 g16 = read_be16(p + 2);
            png_u16 b16 = read_be16(p + 4);
            png_u8 a = 255u;
            png_u8 r;
            png_u8 g;
            png_u8 b;
            p += 6;

            if (have_trns && r16 == tr16 && g16 == tg16 && b16 == tb16)
                a = 0u;

            r = from_16_to_8(r16);
            g = from_16_to_8(g16);
            b = from_16_to_8(b16);

            if (st->use_gamma_lut) {
                r = st->gamma_lut[r];
                g = st->gamma_lut[g];
                b = st->gamma_lut[b];
            }

            out_rgba[0] = r;
            out_rgba[1] = g;
            out_rgba[2] = b;
            out_rgba[3] = a;
            out_rgba += 4;
        }
        return PNG_DEC_OK;
    }

    /* -------- Truecolor with alpha (RGBA) -------- */
    if (st->color_type == PNG_COLOR_TRUECOLOR_ALPHA && st->bit_depth == 8u)
    {
        for (x = 0; x < width; ++x)
        {
            png_u8 r = p[0];
            png_u8 g = p[1];
            png_u8 b = p[2];
            png_u8 a = p[3];
            p += 4;

            if (st->use_gamma_lut) {
                r = st->gamma_lut[r];
                g = st->gamma_lut[g];
                b = st->gamma_lut[b];
            }

            out_rgba[0] = r;
            out_rgba[1] = g;
            out_rgba[2] = b;
            out_rgba[3] = a;
            out_rgba += 4;
        }
        return PNG_DEC_OK;
    }

    if (st->color_type == PNG_COLOR_TRUECOLOR_ALPHA && st->bit_depth == 16u)
    {
        for (x = 0; x < width; ++x)
        {
            png_u16 r16 = read_be16(p + 0);
            png_u16 g16 = read_be16(p + 2);
            png_u16 b16 = read_be16(p + 4);
            png_u16 a16 = read_be16(p + 6);
            png_u8 r;
            png_u8 g;
            png_u8 b;
            png_u8 a;
            p += 8;

            r = from_16_to_8(r16);
            g = from_16_to_8(g16);
            b = from_16_to_8(b16);
            a = from_16_to_8(a16);

            if (st->use_gamma_lut) {
                r = st->gamma_lut[r];
                g = st->gamma_lut[g];
                b = st->gamma_lut[b];
            }

            out_rgba[0] = r;
            out_rgba[1] = g;
            out_rgba[2] = b;
            out_rgba[3] = a;
            out_rgba += 4;
        }
        return PNG_DEC_OK;
    }

    /* -------- Grayscale -------- */
    if (st->color_type == PNG_COLOR_GRAYSCALE)
    {
        if (st->bit_depth == 16u)
        {
            png_u16 t16;
            int have_trns;

            have_trns = (st->has_trns && st->trns_data && st->trns_size >= 2u) ? 1 : 0;
            t16 = have_trns ? read_be16(st->trns_data) : 0;

            for (x = 0; x < width; ++x)
            {
                png_u16 v16 = read_be16(p);
                png_u8 g = from_16_to_8(v16);
                png_u8 a = 255u;
                p += 2;

                if (have_trns && v16 == t16)
                    a = 0u;

                if (st->use_gamma_lut)
                    g = st->gamma_lut[g];

                out_rgba[0] = g;
                out_rgba[1] = g;
                out_rgba[2] = g;
                out_rgba[3] = a;
                out_rgba += 4;
            }
            return PNG_DEC_OK;
        }

        if (st->bit_depth == 8u)
        {
            png_u8 t8;
            int have_trns;

            have_trns = (st->has_trns && st->trns_data && st->trns_size >= 2u) ? 1 : 0;
            t8 = have_trns ? (png_u8)(read_be16(st->trns_data) & 0xFFu) : 0;

            for (x = 0; x < width; ++x)
            {
                png_u8 g = *p++;
                png_u8 a = 255u;

                if (have_trns && g == t8)
                    a = 0u;

                if (st->use_gamma_lut)
                    g = st->gamma_lut[g];

                out_rgba[0] = g;
                out_rgba[1] = g;
                out_rgba[2] = g;
                out_rgba[3] = a;
                out_rgba += 4;
            }
            return PNG_DEC_OK;
        }

        if (st->bit_depth == 1u || st->bit_depth == 2u || st->bit_depth == 4u)
        {
            png_u8 shift_max;
            png_u8 shift;
            png_u8 depth;
            png_u8 raw;
            png_u16 trns16;
            png_u8 trns_raw;
            int have_trns;
            png_u8 mask;

            depth = st->bit_depth;
            shift_max = (png_u8)(8u - depth);
            shift = shift_max;

            have_trns = (st->has_trns && st->trns_data && st->trns_size >= 2u) ? 1 : 0;
            trns16 = have_trns ? read_be16(st->trns_data) : 0;
            mask = (png_u8)((1u << depth) - 1u);
            trns_raw = have_trns ? (png_u8)(trns16 & mask) : 0;

            for (x = 0; x < width; ++x)
            {
                png_u8 g;
                png_u8 a;

                g = expand_sub8(*p, shift, depth, &raw);
                a = 255u;

                if (have_trns && raw == trns_raw)
                    a = 0u;

                if (st->use_gamma_lut)
                    g = st->gamma_lut[g];

                out_rgba[0] = g;
                out_rgba[1] = g;
                out_rgba[2] = g;
                out_rgba[3] = a;
                out_rgba += 4;

                if (shift == 0u) {
                    ++p;
                    shift = shift_max;
                } else {
                    shift = (png_u8)(shift - depth);
                }
            }
            return PNG_DEC_OK;
        }
    }

    /* -------- Grayscale with alpha -------- */
    if (st->color_type == PNG_COLOR_GRAYSCALE_ALPHA)
    {
        if (st->bit_depth == 8u)
        {
            for (x = 0; x < width; ++x)
            {
                png_u8 g = p[0];
                png_u8 a = p[1];
                p += 2;

                if (st->use_gamma_lut)
                    g = st->gamma_lut[g];

                out_rgba[0] = g;
                out_rgba[1] = g;
                out_rgba[2] = g;
                out_rgba[3] = a;
                out_rgba += 4;
            }
            return PNG_DEC_OK;
        }

        if (st->bit_depth == 16u)
        {
            for (x = 0; x < width; ++x)
            {
                png_u16 g16 = read_be16(p + 0);
                png_u16 a16 = read_be16(p + 2);
                png_u8 g;
                png_u8 a;
                p += 4;

                g = from_16_to_8(g16);
                a = from_16_to_8(a16);

                if (st->use_gamma_lut)
                    g = st->gamma_lut[g];

                out_rgba[0] = g;
                out_rgba[1] = g;
                out_rgba[2] = g;
                out_rgba[3] = a;
                out_rgba += 4;
            }
            return PNG_DEC_OK;
        }
    }

    /* -------- Indexed-color -------- */
    if (st->color_type == PNG_COLOR_INDEXED &&
        (st->bit_depth == 1u || st->bit_depth == 2u || st->bit_depth == 4u || st->bit_depth == 8u))
    {
        if (st->bit_depth == 8u)
        {
            for (x = 0; x < width; ++x)
            {
                png_u8 idx = *p++;
                png_u8 r, g, b, a;
                int err;

                err = palette_lookup(st, idx, &r, &g, &b, &a);
                if (err != PNG_DEC_OK)
                    return err;

                if (st->use_gamma_lut) {
                    r = st->gamma_lut[r];
                    g = st->gamma_lut[g];
                    b = st->gamma_lut[b];
                }

                out_rgba[0] = r;
                out_rgba[1] = g;
                out_rgba[2] = b;
                out_rgba[3] = a;
                out_rgba += 4;
            }
            return PNG_DEC_OK;
        }
        else
        {
            png_u8 depth;
            png_u8 shift_max;
            png_u8 shift;

            depth = st->bit_depth;
            shift_max = (png_u8)(8u - depth);
            shift = shift_max;

            for (x = 0; x < width; ++x)
            {
                png_u8 idx;
                png_u8 r, g, b, a;
                int err;

                idx = (png_u8)(((*p) >> shift) & ((1u << depth) - 1u));

                if (shift == 0u) {
                    ++p;
                    shift = shift_max;
                } else {
                    shift = (png_u8)(shift - depth);
                }

                err = palette_lookup(st, idx, &r, &g, &b, &a);
                if (err != PNG_DEC_OK)
                    return err;

                if (st->use_gamma_lut) {
                    r = st->gamma_lut[r];
                    g = st->gamma_lut[g];
                    b = st->gamma_lut[b];
                }

                out_rgba[0] = r;
                out_rgba[1] = g;
                out_rgba[2] = b;
                out_rgba[3] = a;
                out_rgba += 4;
            }
            return PNG_DEC_OK;
        }
    }

    return PNG_DEC_ERR_UNSUPPORTED;
}


int png_render_scanline_rgba16(const png_state* st,
                               const png_u8* scan,
                               png_u8* out_rgba16,
                               png_u32 width)
{
    png_u32 x;
    const png_u8* p;

    if (!st || !scan || !out_rgba16)
        return PNG_DEC_ERR_FORMAT;

    p = scan;

    if (st->color_type == PNG_COLOR_TRUECOLOR && st->bit_depth == 8u)
    {
        png_u8 tr, tg, tb;
        int have_trns;

        have_trns = (st->has_trns && st->trns_data && st->trns_size >= 6u) ? 1 : 0;
        tr = have_trns ? st->trns_data[1] : 0;
        tg = have_trns ? st->trns_data[3] : 0;
        tb = have_trns ? st->trns_data[5] : 0;

        for (x = 0; x < width; ++x)
        {
            png_u8 r8 = p[0];
            png_u8 g8 = p[1];
            png_u8 b8 = p[2];
            png_u16 a16 = 65535u;
            png_u16 r16, g16, b16;
            p += 3;

            if (have_trns && r8 == tr && g8 == tg && b8 == tb)
                a16 = 0u;

            if (st->use_gamma_lut) {
                r8 = st->gamma_lut[r8];
                g8 = st->gamma_lut[g8];
                b8 = st->gamma_lut[b8];
            }

            r16 = expand_8_to_16(r8);
            g16 = expand_8_to_16(g8);
            b16 = expand_8_to_16(b8);

            write_be16_out(out_rgba16 + 0, r16);
            write_be16_out(out_rgba16 + 2, g16);
            write_be16_out(out_rgba16 + 4, b16);
            write_be16_out(out_rgba16 + 6, a16);
            out_rgba16 += 8;
        }
        return PNG_DEC_OK;
    }

    if (st->color_type == PNG_COLOR_TRUECOLOR && st->bit_depth == 16u)
    {
        png_u16 tr16, tg16, tb16;
        int have_trns;

        have_trns = (st->has_trns && st->trns_data && st->trns_size >= 6u) ? 1 : 0;
        tr16 = have_trns ? read_be16(st->trns_data + 0) : 0;
        tg16 = have_trns ? read_be16(st->trns_data + 2) : 0;
        tb16 = have_trns ? read_be16(st->trns_data + 4) : 0;

        for (x = 0; x < width; ++x)
        {
            png_u16 r16 = read_be16(p + 0);
            png_u16 g16 = read_be16(p + 2);
            png_u16 b16 = read_be16(p + 4);
            png_u16 a16 = 65535u;
            p += 6;

            if (have_trns && r16 == tr16 && g16 == tg16 && b16 == tb16)
                a16 = 0u;

            if (st->use_gamma_lut) {
                r16 = png_apply_gamma_16_scalar(st, r16);
                g16 = png_apply_gamma_16_scalar(st, g16);
                b16 = png_apply_gamma_16_scalar(st, b16);
            }

            write_be16_out(out_rgba16 + 0, r16);
            write_be16_out(out_rgba16 + 2, g16);
            write_be16_out(out_rgba16 + 4, b16);
            write_be16_out(out_rgba16 + 6, a16);
            out_rgba16 += 8;
        }
        return PNG_DEC_OK;
    }

    if (st->color_type == PNG_COLOR_TRUECOLOR_ALPHA && st->bit_depth == 8u)
    {
        for (x = 0; x < width; ++x)
        {
            png_u8 r8 = p[0];
            png_u8 g8 = p[1];
            png_u8 b8 = p[2];
            png_u8 a8 = p[3];
            p += 4;

            if (st->use_gamma_lut) {
                r8 = st->gamma_lut[r8];
                g8 = st->gamma_lut[g8];
                b8 = st->gamma_lut[b8];
            }

            write_be16_out(out_rgba16 + 0, expand_8_to_16(r8));
            write_be16_out(out_rgba16 + 2, expand_8_to_16(g8));
            write_be16_out(out_rgba16 + 4, expand_8_to_16(b8));
            write_be16_out(out_rgba16 + 6, expand_8_to_16(a8));
            out_rgba16 += 8;
        }
        return PNG_DEC_OK;
    }

    if (st->color_type == PNG_COLOR_TRUECOLOR_ALPHA && st->bit_depth == 16u)
    {
        for (x = 0; x < width; ++x)
        {
            png_u16 r16 = read_be16(p + 0);
            png_u16 g16 = read_be16(p + 2);
            png_u16 b16 = read_be16(p + 4);
            png_u16 a16 = read_be16(p + 6);
            p += 8;

            if (st->use_gamma_lut) {
                r16 = png_apply_gamma_16_scalar(st, r16);
                g16 = png_apply_gamma_16_scalar(st, g16);
                b16 = png_apply_gamma_16_scalar(st, b16);
            }

            write_be16_out(out_rgba16 + 0, r16);
            write_be16_out(out_rgba16 + 2, g16);
            write_be16_out(out_rgba16 + 4, b16);
            write_be16_out(out_rgba16 + 6, a16);
            out_rgba16 += 8;
        }
        return PNG_DEC_OK;
    }

    if (st->color_type == PNG_COLOR_GRAYSCALE)
    {
        if (st->bit_depth == 16u)
        {
            png_u16 t16;
            int have_trns;

            have_trns = (st->has_trns && st->trns_data && st->trns_size >= 2u) ? 1 : 0;
            t16 = have_trns ? read_be16(st->trns_data) : 0;

            for (x = 0; x < width; ++x)
            {
                png_u16 g16 = read_be16(p);
                png_u16 a16 = 65535u;
                p += 2;

                if (have_trns && g16 == t16)
                    a16 = 0u;

                if (st->use_gamma_lut)
                    g16 = png_apply_gamma_16_scalar(st, g16);

                write_be16_out(out_rgba16 + 0, g16);
                write_be16_out(out_rgba16 + 2, g16);
                write_be16_out(out_rgba16 + 4, g16);
                write_be16_out(out_rgba16 + 6, a16);
                out_rgba16 += 8;
            }
            return PNG_DEC_OK;
        }

        if (st->bit_depth == 8u)
        {
            png_u8 t8;
            int have_trns;

            have_trns = (st->has_trns && st->trns_data && st->trns_size >= 2u) ? 1 : 0;
            t8 = have_trns ? (png_u8)(read_be16(st->trns_data) & 0xFFu) : 0;

            for (x = 0; x < width; ++x)
            {
                png_u8 g8 = *p++;
                png_u16 a16 = 65535u;
                png_u16 g16;

                if (have_trns && g8 == t8)
                    a16 = 0u;

                if (st->use_gamma_lut)
                    g8 = st->gamma_lut[g8];
                g16 = expand_8_to_16(g8);

                write_be16_out(out_rgba16 + 0, g16);
                write_be16_out(out_rgba16 + 2, g16);
                write_be16_out(out_rgba16 + 4, g16);
                write_be16_out(out_rgba16 + 6, a16);
                out_rgba16 += 8;
            }
            return PNG_DEC_OK;
        }

        if (st->bit_depth == 1u || st->bit_depth == 2u || st->bit_depth == 4u)
        {
            png_u8 shift_max;
            png_u8 shift;
            png_u8 depth;
            png_u8 raw;
            png_u16 trns16;
            png_u8 trns_raw;
            int have_trns;
            png_u8 mask;

            depth = st->bit_depth;
            shift_max = (png_u8)(8u - depth);
            shift = shift_max;

            have_trns = (st->has_trns && st->trns_data && st->trns_size >= 2u) ? 1 : 0;
            trns16 = have_trns ? read_be16(st->trns_data) : 0;
            mask = (png_u8)((1u << depth) - 1u);
            trns_raw = have_trns ? (png_u8)(trns16 & mask) : 0;

            for (x = 0; x < width; ++x)
            {
                png_u8 g8;
                png_u16 g16;
                png_u16 a16 = 65535u;

                g8 = expand_sub8(*p, shift, depth, &raw);
                if (have_trns && raw == trns_raw)
                    a16 = 0u;

                if (st->use_gamma_lut)
                    g8 = st->gamma_lut[g8];
                g16 = expand_8_to_16(g8);

                write_be16_out(out_rgba16 + 0, g16);
                write_be16_out(out_rgba16 + 2, g16);
                write_be16_out(out_rgba16 + 4, g16);
                write_be16_out(out_rgba16 + 6, a16);
                out_rgba16 += 8;

                if (shift == 0u) {
                    ++p;
                    shift = shift_max;
                } else {
                    shift = (png_u8)(shift - depth);
                }
            }
            return PNG_DEC_OK;
        }
    }

    if (st->color_type == PNG_COLOR_GRAYSCALE_ALPHA)
    {
        if (st->bit_depth == 8u)
        {
            for (x = 0; x < width; ++x)
            {
                png_u8 g8 = p[0];
                png_u8 a8 = p[1];
                png_u16 g16;
                p += 2;

                if (st->use_gamma_lut)
                    g8 = st->gamma_lut[g8];
                g16 = expand_8_to_16(g8);

                write_be16_out(out_rgba16 + 0, g16);
                write_be16_out(out_rgba16 + 2, g16);
                write_be16_out(out_rgba16 + 4, g16);
                write_be16_out(out_rgba16 + 6, expand_8_to_16(a8));
                out_rgba16 += 8;
            }
            return PNG_DEC_OK;
        }

        if (st->bit_depth == 16u)
        {
            for (x = 0; x < width; ++x)
            {
                png_u16 g16 = read_be16(p + 0);
                png_u16 a16 = read_be16(p + 2);
                p += 4;

                if (st->use_gamma_lut)
                    g16 = png_apply_gamma_16_scalar(st, g16);

                write_be16_out(out_rgba16 + 0, g16);
                write_be16_out(out_rgba16 + 2, g16);
                write_be16_out(out_rgba16 + 4, g16);
                write_be16_out(out_rgba16 + 6, a16);
                out_rgba16 += 8;
            }
            return PNG_DEC_OK;
        }
    }

    if (st->color_type == PNG_COLOR_INDEXED &&
        (st->bit_depth == 1u || st->bit_depth == 2u || st->bit_depth == 4u || st->bit_depth == 8u))
    {
        if (st->bit_depth == 8u)
        {
            for (x = 0; x < width; ++x)
            {
                png_u8 idx = *p++;
                png_u8 r8, g8, b8, a8;
                int err;

                err = palette_lookup(st, idx, &r8, &g8, &b8, &a8);
                if (err != PNG_DEC_OK)
                    return err;

                if (st->use_gamma_lut) {
                    r8 = st->gamma_lut[r8];
                    g8 = st->gamma_lut[g8];
                    b8 = st->gamma_lut[b8];
                }

                write_be16_out(out_rgba16 + 0, expand_8_to_16(r8));
                write_be16_out(out_rgba16 + 2, expand_8_to_16(g8));
                write_be16_out(out_rgba16 + 4, expand_8_to_16(b8));
                write_be16_out(out_rgba16 + 6, expand_8_to_16(a8));
                out_rgba16 += 8;
            }
            return PNG_DEC_OK;
        }
        else
        {
            png_u8 depth;
            png_u8 shift_max;
            png_u8 shift;

            depth = st->bit_depth;
            shift_max = (png_u8)(8u - depth);
            shift = shift_max;

            for (x = 0; x < width; ++x)
            {
                png_u8 idx;
                png_u8 r8, g8, b8, a8;
                int err;

                idx = (png_u8)(((*p) >> shift) & ((1u << depth) - 1u));

                if (shift == 0u) {
                    ++p;
                    shift = shift_max;
                } else {
                    shift = (png_u8)(shift - depth);
                }

                err = palette_lookup(st, idx, &r8, &g8, &b8, &a8);
                if (err != PNG_DEC_OK)
                    return err;

                if (st->use_gamma_lut) {
                    r8 = st->gamma_lut[r8];
                    g8 = st->gamma_lut[g8];
                    b8 = st->gamma_lut[b8];
                }

                write_be16_out(out_rgba16 + 0, expand_8_to_16(r8));
                write_be16_out(out_rgba16 + 2, expand_8_to_16(g8));
                write_be16_out(out_rgba16 + 4, expand_8_to_16(b8));
                write_be16_out(out_rgba16 + 6, expand_8_to_16(a8));
                out_rgba16 += 8;
            }
            return PNG_DEC_OK;
        }
    }

    return PNG_DEC_ERR_UNSUPPORTED;
}

int png_decode_image_data_rows16(const png_state* st,
                                 const png_u8* img_data,
                                 png_u32 img_size,
                                 png_u8* out_rgba16,
                                 png_row_callback_fn row_fn,
                                 void* row_user)
{
    png_u32 expected_size;
    const png_u8* p;
    int err_size;

    if (!st || !img_data || !out_rgba16)
        return PNG_DEC_ERR_FORMAT;

    err_size = png_compute_uncompressed_size(st, &expected_size);
    if (err_size != PNG_DEC_OK)
        return err_size;

    if (img_size != expected_size)
        return PNG_DEC_ERR_FORMAT;

    p = img_data;

    if (st->interlace_method == 0u)
    {
        png_u32 rowbytes;
        png_u32 bpp_bytes;
        png_u8* row;
        png_u8* prev;
        png_u32 y;

        rowbytes = png_rowbytes_for_width_internal(st, st->width);
        if (rowbytes == 0u)
            return PNG_DEC_ERR_UNSUPPORTED;

        bpp_bytes = png_filter_bpp_bytes_internal(st);
        if (bpp_bytes == 0u)
            return PNG_DEC_ERR_UNSUPPORTED;

        row = (png_u8*)png_mem89_alloc(rowbytes == 0u ? 1u : rowbytes);
        prev = (png_u8*)png_mem89_alloc(rowbytes == 0u ? 1u : rowbytes);
        if (!row || !prev) {
            if (row) png_mem89_release(row);
            if (prev) png_mem89_release(prev);
            return PNG_DEC_ERR_OOM;
        }

        memset(prev, 0, rowbytes);

        for (y = 0; y < st->height; ++y)
        {
            int filter;
            int err;
            png_u8* dst_row;

            filter = (int)(*p++);
            if (filter < 0 || filter > 4) {
                png_mem89_release(row);
                png_mem89_release(prev);
                return PNG_DEC_ERR_FORMAT;
            }

            memcpy(row, p, rowbytes);
            p += rowbytes;

            png_unfilter_scanline(row, prev, rowbytes, filter, bpp_bytes);

            dst_row = out_rgba16 + (png_u32)y * st->width * 8u;
            err = png_render_scanline_rgba16(st, row, dst_row, st->width);
            if (err != PNG_DEC_OK) {
                png_mem89_release(row);
                png_mem89_release(prev);
                return err;
            }

            if (row_fn)
                row_fn(row_user, y, dst_row, st->width * 8u, 0);

            {
                png_u8* tmp = prev;
                prev = row;
                row = tmp;
            }
        }

        png_mem89_release(row);
        png_mem89_release(prev);
        return PNG_DEC_OK;
    }

    {
        int pass;

        for (pass = 0; pass < 7; ++pass)
        {
            png_u32 x_start;
            png_u32 y_start;
            png_u32 x_step;
            png_u32 y_step;
            png_u32 pw;
            png_u32 ph;
            png_u32 rp;
            png_u32 bpp_bytes;
            png_u8* row;
            png_u8* prev;
            png_u8* tmp_rgba;
            png_u32 y;

            x_start = (png_u32)PNG_ADAM7_X_START[pass];
            y_start = (png_u32)PNG_ADAM7_Y_START[pass];
            x_step  = (png_u32)PNG_ADAM7_X_STEP[pass];
            y_step  = (png_u32)PNG_ADAM7_Y_STEP[pass];

            pw = (st->width > x_start)
               ? ((st->width - x_start + x_step - 1u) / x_step)
               : 0u;
            ph = (st->height > y_start)
               ? ((st->height - y_start + y_step - 1u) / y_step)
               : 0u;

            if (pw == 0u || ph == 0u)
                continue;

            rp = png_rowbytes_for_width_internal(st, pw);
            if (rp == 0u)
                return PNG_DEC_ERR_UNSUPPORTED;

            bpp_bytes = png_filter_bpp_bytes_internal(st);
            if (bpp_bytes == 0u)
                return PNG_DEC_ERR_UNSUPPORTED;

            row = (png_u8*)png_mem89_alloc(rp == 0u ? 1u : rp);
            prev = (png_u8*)png_mem89_alloc(rp == 0u ? 1u : rp);
            tmp_rgba = (png_u8*)png_mem89_alloc(pw * 8u == 0u ? 1u : pw * 8u);
            if (!row || !prev || !tmp_rgba) {
                if (row) png_mem89_release(row);
                if (prev) png_mem89_release(prev);
                if (tmp_rgba) png_mem89_release(tmp_rgba);
                return PNG_DEC_ERR_OOM;
            }

            memset(prev, 0, rp);

            for (y = 0; y < ph; ++y)
            {
                int filter;
                int err;
                png_u32 px;
                png_u32 last_row_index;

                filter = (int)(*p++);
                if (filter < 0 || filter > 4) {
                    png_mem89_release(row);
                    png_mem89_release(prev);
                    png_mem89_release(tmp_rgba);
                    return PNG_DEC_ERR_FORMAT;
                }

                memcpy(row, p, rp);
                p += rp;

                png_unfilter_scanline(row, prev, rp, filter, bpp_bytes);

                err = png_render_scanline_rgba16(st, row, tmp_rgba, pw);
                if (err != PNG_DEC_OK) {
                    png_mem89_release(row);
                    png_mem89_release(prev);
                    png_mem89_release(tmp_rgba);
                    return err;
                }

                last_row_index = y_start + y * y_step;
                for (px = 0; px < pw; ++px)
                {
                    png_u32 xr;
                    png_u32 yr;
                    png_u8* dst;
                    png_u8* src;

                    xr = x_start + px * x_step;
                    yr = y_start + y * y_step;
                    if (xr >= st->width || yr >= st->height)
                        continue;

                    dst = out_rgba16 + (yr * st->width + xr) * 8u;
                    src = tmp_rgba + px * 8u;
                    memcpy(dst, src, 8u);
                }

                if (row_fn && last_row_index < st->height)
                    row_fn(row_user,
                           last_row_index,
                           out_rgba16 + last_row_index * st->width * 8u,
                           st->width * 8u,
                           pass + 1);

                {
                    png_u8* tmp = prev;
                    prev = row;
                    row = tmp;
                }
            }

            png_mem89_release(row);
            png_mem89_release(prev);
            png_mem89_release(tmp_rgba);
        }
    }

    return PNG_DEC_OK;
}

int png_decode_image_data16(const png_state* st,
                            const png_u8* img_data,
                            png_u32 img_size,
                            png_u8* out_rgba16)
{
    return png_decode_image_data_rows16(st, img_data, img_size, out_rgba16, 0, 0);
}

int png_decode_image_data_rows(const png_state* st,
                               const png_u8* img_data,
                               png_u32 img_size,
                               png_u8* out_rgba,
                               png_row_callback_fn row_fn,
                               void* row_user)
{
    png_u32 expected_size;
    const png_u8* p;
    int err_size;

    if (!st || !img_data || !out_rgba)
        return PNG_DEC_ERR_FORMAT;

    err_size = png_compute_uncompressed_size(st, &expected_size);
    if (err_size != PNG_DEC_OK)
        return err_size;

    if (img_size != expected_size)
        return PNG_DEC_ERR_FORMAT;

    p = img_data;

    if (st->interlace_method == 0u)
    {
        png_u32 rowbytes;
        png_u32 bpp_bytes;
        png_u8* row;
        png_u8* prev;
        png_u32 y;

        rowbytes = png_rowbytes_for_width_internal(st, st->width);
        if (rowbytes == 0u)
            return PNG_DEC_ERR_UNSUPPORTED;

        bpp_bytes = png_filter_bpp_bytes_internal(st);
        if (bpp_bytes == 0u)
            return PNG_DEC_ERR_UNSUPPORTED;

        row = (png_u8*)png_mem89_alloc(rowbytes);
        prev = (png_u8*)png_mem89_alloc(rowbytes);
        if (!row || !prev) {
            if (row) png_mem89_release(row);
            if (prev) png_mem89_release(prev);
            return PNG_DEC_ERR_OOM;
        }

        memset(prev, 0, rowbytes);

        for (y = 0; y < st->height; ++y)
        {
            int filter;
            int err;
            png_u8* dst_row;

            filter = (int)(*p++);
            if (filter < 0 || filter > 4) {
                png_mem89_release(row);
                png_mem89_release(prev);
                return PNG_DEC_ERR_FORMAT;
            }

            memcpy(row, p, rowbytes);
            p += rowbytes;

            png_unfilter_scanline(row, prev, rowbytes, filter, bpp_bytes);

            dst_row = out_rgba + (png_u32)y * st->width * 4u;
            err = png_render_scanline_rgba8(st, row, dst_row, st->width);
            if (err != PNG_DEC_OK) {
                png_mem89_release(row);
                png_mem89_release(prev);
                return err;
            }

            if (row_fn)
                row_fn(row_user, y, dst_row, st->width * 4u, 0);

            {
                png_u8* tmp = prev;
                prev = row;
                row = tmp;
            }
        }

        png_mem89_release(row);
        png_mem89_release(prev);
        return PNG_DEC_OK;
    }

    {
        int pass;

        for (pass = 0; pass < 7; ++pass)
        {
            png_u32 x_start;
            png_u32 y_start;
            png_u32 x_step;
            png_u32 y_step;
            png_u32 pw;
            png_u32 ph;
            png_u32 rp;
            png_u32 bpp_bytes;
            png_u8* row;
            png_u8* prev;
            png_u8* tmp_rgba;
            png_u32 y;

            x_start = (png_u32)PNG_ADAM7_X_START[pass];
            y_start = (png_u32)PNG_ADAM7_Y_START[pass];
            x_step  = (png_u32)PNG_ADAM7_X_STEP[pass];
            y_step  = (png_u32)PNG_ADAM7_Y_STEP[pass];

            pw = (st->width > x_start)
               ? ((st->width - x_start + x_step - 1u) / x_step)
               : 0u;
            ph = (st->height > y_start)
               ? ((st->height - y_start + y_step - 1u) / y_step)
               : 0u;

            if (pw == 0u || ph == 0u)
                continue;

            rp = png_rowbytes_for_width_internal(st, pw);
            if (rp == 0u)
                return PNG_DEC_ERR_UNSUPPORTED;

            bpp_bytes = png_filter_bpp_bytes_internal(st);
            if (bpp_bytes == 0u)
                return PNG_DEC_ERR_UNSUPPORTED;

            row = (png_u8*)png_mem89_alloc(rp);
            prev = (png_u8*)png_mem89_alloc(rp);
            tmp_rgba = (png_u8*)png_mem89_alloc(pw * 4u);
            if (!row || !prev || !tmp_rgba) {
                if (row) png_mem89_release(row);
                if (prev) png_mem89_release(prev);
                if (tmp_rgba) png_mem89_release(tmp_rgba);
                return PNG_DEC_ERR_OOM;
            }

            memset(prev, 0, rp);

            for (y = 0; y < ph; ++y)
            {
                int filter;
                int err;
                png_u32 px;
                png_u32 last_row_index;

                filter = (int)(*p++);
                if (filter < 0 || filter > 4) {
                    png_mem89_release(row);
                    png_mem89_release(prev);
                    png_mem89_release(tmp_rgba);
                    return PNG_DEC_ERR_FORMAT;
                }

                memcpy(row, p, rp);
                p += rp;

                png_unfilter_scanline(row, prev, rp, filter, bpp_bytes);

                err = png_render_scanline_rgba8(st, row, tmp_rgba, pw);
                if (err != PNG_DEC_OK) {
                    png_mem89_release(row);
                    png_mem89_release(prev);
                    png_mem89_release(tmp_rgba);
                    return err;
                }

                last_row_index = y_start + y * y_step;
                for (px = 0; px < pw; ++px)
                {
                    png_u32 xr;
                    png_u32 yr;
                    png_u8* dst;
                    png_u8* src;

                    xr = x_start + px * x_step;
                    yr = y_start + y * y_step;
                    if (xr >= st->width || yr >= st->height)
                        continue;

                    dst = out_rgba + (yr * st->width + xr) * 4u;
                    src = tmp_rgba + px * 4u;

                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                    dst[3] = src[3];
                }

                if (row_fn && last_row_index < st->height)
                    row_fn(row_user,
                           last_row_index,
                           out_rgba + last_row_index * st->width * 4u,
                           st->width * 4u,
                           pass + 1);

                {
                    png_u8* tmp = prev;
                    prev = row;
                    row = tmp;
                }
            }

            png_mem89_release(row);
            png_mem89_release(prev);
            png_mem89_release(tmp_rgba);
        }
    }

    return PNG_DEC_OK;
}

int png_decode_image_data(const png_state* st,
                          const png_u8* img_data,
                          png_u32 img_size,
                          png_u8* out_rgba)
{
    return png_decode_image_data_rows(st, img_data, img_size, out_rgba, 0, 0);
}

static int png_compute_uncompressed_size(const png_state* st, png_u32* out_size)
{
    png_u32 maxv;
    png_u32 bpp_bits;
    png_u32 total;
    int pass;

    if (!st || !out_size)
        return PNG_DEC_ERR_FORMAT;

    if (st->width == 0u || st->height == 0u)
        return PNG_DEC_ERR_FORMAT;

    switch (st->color_type)
    {
        case PNG_COLOR_GRAYSCALE:
        case PNG_COLOR_INDEXED:
            bpp_bits = (png_u32)st->bit_depth;
            break;

        case PNG_COLOR_TRUECOLOR:
            bpp_bits = (png_u32)st->bit_depth * 3u;
            break;

        case PNG_COLOR_GRAYSCALE_ALPHA:
            bpp_bits = (png_u32)st->bit_depth * 2u;
            break;

        case PNG_COLOR_TRUECOLOR_ALPHA:
            bpp_bits = (png_u32)st->bit_depth * 4u;
            break;

        default:
            return PNG_DEC_ERR_UNSUPPORTED;
    }

    if (bpp_bits == 0u)
        return PNG_DEC_ERR_FORMAT;

    maxv = (png_u32)~(png_u32)0;
    total = 0u;

    if (st->interlace_method == 0u)
    {
        png_u32 rowbits;
        png_u32 rowbytes;
        png_u32 per_row;

        rowbits = st->width * bpp_bits;
        if (st->width != 0u && rowbits / st->width != bpp_bits)
            return PNG_DEC_ERR_FORMAT;

        rowbytes = (rowbits + 7u) / 8u;

        if (rowbytes > maxv - 1u)
            return PNG_DEC_ERR_FORMAT;

        per_row = rowbytes + 1u;

        if (st->height != 0u && per_row != 0u && st->height > maxv / per_row)
            return PNG_DEC_ERR_FORMAT;

        total = st->height * per_row;
    }
    else
    {
        for (pass = 0; pass < 7; ++pass)
        {
            png_u32 x_start;
            png_u32 y_start;
            png_u32 x_step;
            png_u32 y_step;
            png_u32 pw;
            png_u32 ph;
            png_u32 rowbits;
            png_u32 rowbytes;
            png_u32 per_row;
            png_u32 this_pass;

            x_start = (png_u32)PNG_ADAM7_X_START[pass];
            y_start = (png_u32)PNG_ADAM7_Y_START[pass];
            x_step  = (png_u32)PNG_ADAM7_X_STEP[pass];
            y_step  = (png_u32)PNG_ADAM7_Y_STEP[pass];

            pw = (st->width > x_start)
               ? ((st->width - x_start + x_step - 1u) / x_step)
               : 0u;
            ph = (st->height > y_start)
               ? ((st->height - y_start + y_step - 1u) / y_step)
               : 0u;

            if (pw == 0u || ph == 0u)
                continue;

            rowbits = pw * bpp_bits;
            if (pw != 0u && rowbits / pw != bpp_bits)
                return PNG_DEC_ERR_FORMAT;

            rowbytes = (rowbits + 7u) / 8u;

            if (rowbytes > maxv - 1u)
                return PNG_DEC_ERR_FORMAT;

            per_row = rowbytes + 1u;

            if (ph != 0u && per_row != 0u && ph > maxv / per_row)
                return PNG_DEC_ERR_FORMAT;

            this_pass = ph * per_row;

            if (total > maxv - this_pass)
                return PNG_DEC_ERR_FORMAT;

            total += this_pass;
        }
    }

    if (total == 0u)
        return PNG_DEC_ERR_FORMAT;

    if (total > PNG_DEC_MAX_IMAGE_BYTES)
        return PNG_DEC_ERR_UNSUPPORTED;

    *out_size = total;
    return PNG_DEC_OK;
}
