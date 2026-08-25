/* png_parser.c
 * High-level parse of a PNG datastream:
 *  - validate signature
 *  - parse chunks into png_state
 *  - validate minimal conformance
 *  - build gamma LUT (optional)
 *  - inflate concatenated IDAT into filtered scanline bytes
 */

#include "png_mem89.h"
#include <string.h>

#include "png_decoder_internal.h"
#include "png_chunks.h"

/* ADAM7 (shared) */
const int PNG_ADAM7_X_START[7] = {0,4,0,2,0,1,0};
const int PNG_ADAM7_Y_START[7] = {0,0,4,0,2,0,1};
const int PNG_ADAM7_X_STEP[7]  = {8,8,4,4,2,2,1};
const int PNG_ADAM7_Y_STEP[7]  = {8,8,8,4,4,2,2};

static const png_u8 PNG_PARSER_SIGNATURE[8] = {
    0x89, 'P','N','G', 0x0D,0x0A,0x1A,0x0A
};

static int png_check_signature_local(const png_u8* data, png_u32 size)
{
    if (!data || size < 8u)
        return PNG_DEC_ERR_SIG;

    if (memcmp(data, PNG_PARSER_SIGNATURE, 8u) != 0)
        return PNG_DEC_ERR_SIG;

    return PNG_DEC_OK;
}

static int png_validate_state(const png_state* st)
{
    png_u32 max_entries;

    if (!st)
        return PNG_DEC_ERR_FORMAT;

    /* Critical chunks required */
    if (!st->seen_IHDR || !st->seen_IDAT || !st->seen_IEND)
        return PNG_DEC_ERR_FORMAT;

    if (st->width == 0u || st->height == 0u)
        return PNG_DEC_ERR_FORMAT;

    if (st->width > st->max_width || st->height > st->max_height)
        return PNG_DEC_ERR_DIMENSIONS_TOO_LARGE;

    if (st->height != 0u && st->width > st->max_pixels / st->height)
        return PNG_DEC_ERR_TOO_MANY_PIXELS;

    /* Validate color type + bit depth (PNG spec Table 12). */
    switch (st->color_type)
    {
        case PNG_COLOR_GRAYSCALE:
            if (!(st->bit_depth == 1u || st->bit_depth == 2u || st->bit_depth == 4u ||
                  st->bit_depth == 8u || st->bit_depth == 16u))
                return PNG_DEC_ERR_UNSUPPORTED;
            break;

        case PNG_COLOR_TRUECOLOR:
            if (!(st->bit_depth == 8u || st->bit_depth == 16u))
                return PNG_DEC_ERR_UNSUPPORTED;
            break;

        case PNG_COLOR_INDEXED:
            if (!(st->bit_depth == 1u || st->bit_depth == 2u || st->bit_depth == 4u ||
                  st->bit_depth == 8u))
                return PNG_DEC_ERR_UNSUPPORTED;

            if (!st->have_palette || !st->palette || st->palette_entries == 0u)
                return PNG_DEC_ERR_FORMAT;

            if (st->palette_entries > 256u)
                return PNG_DEC_ERR_FORMAT;

            max_entries = 1u << st->bit_depth;
            if (st->palette_entries > max_entries)
                return PNG_DEC_ERR_FORMAT;
            break;

        case PNG_COLOR_GRAYSCALE_ALPHA:
            if (!(st->bit_depth == 8u || st->bit_depth == 16u))
                return PNG_DEC_ERR_UNSUPPORTED;
            break;

        case PNG_COLOR_TRUECOLOR_ALPHA:
            if (!(st->bit_depth == 8u || st->bit_depth == 16u))
                return PNG_DEC_ERR_UNSUPPORTED;
            break;

        default:
            return PNG_DEC_ERR_UNSUPPORTED;
    }

    if (!(st->interlace_method == 0u || st->interlace_method == 1u))
        return PNG_DEC_ERR_UNSUPPORTED;

    if (st->compression_method != 0u)
        return PNG_DEC_ERR_UNSUPPORTED;

    if (st->filter_method != 0u)
        return PNG_DEC_ERR_UNSUPPORTED;

    if (st->idat_size == 0u || !st->idat_data)
        return PNG_DEC_ERR_FORMAT;

    if (st->idat_size > st->max_file_bytes)
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    if (st->have_mdcv && !st->have_cicp)
        return PNG_DEC_ERR_FORMAT;

    if (st->dsig_chunk_count != 0u)
    {
        png_u32 dsig_before = 0u;
        png_u32 dsig_after = 0u;
        png_u32 i;
        for (i = 0u; i < st->dsig_chunk_count; ++i)
        {
            if (st->dsig_chunks[i].location == PNG_CHUNK_POS_AFTER_IHDR)
                dsig_before += 1u;
            else if (st->dsig_chunks[i].location == PNG_CHUNK_POS_BEFORE_IEND)
                dsig_after += 1u;
            else
                return PNG_DEC_ERR_FORMAT;
        }
        if (dsig_before != dsig_after)
            return PNG_DEC_ERR_FORMAT;
    }

    return PNG_DEC_OK;
}

#if !PNG_DEC_DISABLE_GAMMA
void png_build_gamma_lut(png_state* s)
{
    png_fixed89 img_gamma;
    png_fixed89 exponent;
    png_u32 i;

    if (!s)
        return;

    img_gamma = s->image_gamma;

    if (!(s->transform_flags & PNG_DEC_TRANSFORM_APPLY_GAMMA)) {
        s->use_gamma_lut = 0;
        return;
    }

    if (!s->have_gama && !s->have_srgb) {
        s->use_gamma_lut = 0;
        return;
    }

    if (s->have_srgb)
        img_gamma = png_fixed89_from_ratio(45455, 100000);

    if (img_gamma <= 0) {
        s->use_gamma_lut = 0;
        return;
    }

    if (img_gamma < png_fixed89_from_ratio(1, 100) || img_gamma > (10 * PNG_FIXED89_ONE)) {
        s->use_gamma_lut = 0;
        return;
    }

    exponent = png_fixed89_div(img_gamma, png_fixed89_from_ratio(22, 10));
    for (i = 0u; i < 256u; ++i)
    {
        png_fixed89 v;
        png_fixed89 outv;
        v = png_fixed89_from_ratio((signed int)i, 255);
        outv = png_fixed89_pow_unit(v, exponent);
        s->gamma_lut[i] = (png_u8)png_fixed89_unit_to_u8(outv);
    }

    s->use_gamma_lut = 1;
}
#else
void png_build_gamma_lut(png_state* s)
{
    (void)s;
}
#endif

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

    if (st->width > st->max_width || st->height > st->max_height)
        return PNG_DEC_ERR_DIMENSIONS_TOO_LARGE;

    if (st->height != 0u && st->width > st->max_pixels / st->height)
        return PNG_DEC_ERR_TOO_MANY_PIXELS;

    /* Bits per pixel for the *compressed* image stream (before filtering).
       For indexed, bpp is the palette index size (bit_depth).
     */
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

        per_row = rowbytes + 1u; /* filter byte */

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

    if (total > st->max_inflated_bytes)
        return PNG_DEC_ERR_INFLATED_TOO_LARGE;

    *out_size = total;
    return PNG_DEC_OK;
}

int png_parse_png(const png_u8* data,
                  png_u32 size,
                  png_zlib_decompress_func zfunc,
                  png_state* st,
                  png_u8** out_img_data,
                  png_u32* out_img_size)
{
    png_u32 pos;
    int err;
    png_u8* decompressed;
    png_u32 decompressed_size;

    if (!data || !zfunc || !st || !out_img_data || !out_img_size)
        return PNG_DEC_ERR_FORMAT;

    if (size > st->max_file_bytes)
        return PNG_DEC_ERR_UNSUPPORTED;

    err = png_check_signature_local(data, size);
    if (err != PNG_DEC_OK)
        return err;

    pos = 8u;
    decompressed = 0;
    decompressed_size = 0u;

    st->zfunc = zfunc;
    *out_img_data = 0;
    *out_img_size = 0u;

    /* 1) Parse chunks into state */
    err = png_parse_chunks(data, size, &pos, st);
    if (err != PNG_DEC_OK)
        goto cleanup;

    if (st->strict_trailing_data && pos != size) {
        err = PNG_DEC_ERR_FORMAT;
        goto cleanup;
    }

    /* 2) Validate state */
    err = png_validate_state(st);
    if (err != PNG_DEC_OK)
        goto cleanup;

    /* 3) Build gamma LUT if needed */
    png_build_gamma_lut(st);

    /* 4) Compute exact expected inflated size */
    err = png_compute_uncompressed_size(st, &decompressed_size);
    if (err != PNG_DEC_OK)
        goto cleanup;

    if (decompressed_size > st->max_inflated_bytes) {
        err = PNG_DEC_ERR_INFLATED_TOO_LARGE;
        goto cleanup;
    }

    decompressed = (png_u8*)png_mem89_alloc(decompressed_size == 0u ? 1u : decompressed_size);
    if (!decompressed) {
        err = PNG_DEC_ERR_OOM;
        goto cleanup;
    }

    /* 5) Inflate IDAT into decompressed buffer */
    {
        png_u32 out_len;
        int zerr;

        out_len = decompressed_size;
        zerr = zfunc(decompressed, &out_len, st->idat_data, st->idat_size);
        if (zerr != 0) {
            err = PNG_DEC_ERR_ZLIB;
            goto cleanup;
        }

        if (out_len != decompressed_size) {
            err = PNG_DEC_ERR_FORMAT;
            goto cleanup;
        }
    }

    *out_img_data = decompressed;
    *out_img_size = decompressed_size;
    return PNG_DEC_OK;

cleanup:
    if (decompressed)
        png_mem89_release(decompressed);

    png_state_free(st);
    return err;
}
