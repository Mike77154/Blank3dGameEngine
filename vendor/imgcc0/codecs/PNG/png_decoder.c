#include "png_mem89.h"
#include <stdio.h>
#include <string.h>

#ifdef PNG_DEC_USE_ZLIB
#include <zlib.h>
#endif

#include "png_decoder_internal.h"
#include "png_chunks.h"

static void png_free_text_entries_public(png_text_entry* entries, png_u32 count)
{
    png_u32 i;

    if (!entries)
        return;

    for (i = 0; i < count; ++i)
    {
        if (entries[i].keyword) png_mem89_release(entries[i].keyword);
        if (entries[i].text) png_mem89_release(entries[i].text);
        if (entries[i].language_tag) png_mem89_release(entries[i].language_tag);
        if (entries[i].translated_keyword) png_mem89_release(entries[i].translated_keyword);
    }

    png_mem89_release(entries);
}

static void png_free_unknown_chunks_public(png_unknown_chunk* chunks, png_u32 count)
{
    png_u32 i;

    if (!chunks)
        return;

    for (i = 0; i < count; ++i)
    {
        if (chunks[i].data)
            png_mem89_release(chunks[i].data);
    }

    png_mem89_release(chunks);
}

static void png_free_gifg_entries_public(png_gifg_entry* entries)
{
    if (entries)
        png_mem89_release(entries);
}

static void png_free_gifx_entries_public(png_gifx_entry* entries, png_u32 count)
{
    png_u32 i;

    if (!entries)
        return;

    for (i = 0u; i < count; ++i)
    {
        if (entries[i].application_data)
            png_mem89_release(entries[i].application_data);
    }

    png_mem89_release(entries);
}

static void png_free_gift_entries_public(png_gift_entry* entries, png_u32 count)
{
    png_u32 i;

    if (!entries)
        return;

    for (i = 0u; i < count; ++i)
    {
        if (entries[i].text_data)
            png_mem89_release(entries[i].text_data);
    }

    png_mem89_release(entries);
}

static void png_free_frac_entries_public(png_frac_entry* entries, png_u32 count)
{
    png_u32 i;

    if (!entries)
        return;

    for (i = 0u; i < count; ++i)
    {
        if (entries[i].data)
            png_mem89_release(entries[i].data);
    }

    png_mem89_release(entries);
}

static void png_free_splt_palettes_public(png_splt_palette* palettes, png_u32 count)
{
    png_u32 i;

    if (!palettes)
        return;

    for (i = 0; i < count; ++i)
    {
        if (palettes[i].name)
            png_mem89_release(palettes[i].name);
        if (palettes[i].entries)
            png_mem89_release(palettes[i].entries);
    }

    png_mem89_release(palettes);
}

static void png_free_string_array_public(char** strings, png_u32 count)
{
    png_u32 i;

    if (!strings)
        return;

    for (i = 0; i < count; ++i)
    {
        if (strings[i])
            png_mem89_release(strings[i]);
    }

    png_mem89_release(strings);
}

static void png_free_pcal_info_public(png_pcal_info* pcal)
{
    if (!pcal)
        return;

    if (pcal->name)
    {
        png_mem89_release(pcal->name);
        pcal->name = 0;
    }
    if (pcal->unit_name)
    {
        png_mem89_release(pcal->unit_name);
        pcal->unit_name = 0;
    }
    png_free_string_array_public(pcal->params, pcal->param_count);
    pcal->params = 0;
    pcal->param_count = 0u;
    pcal->equation_type = 0u;
    pcal->x0 = 0;
    pcal->x1 = 0;
}

void png_decode_options_init(png_decode_options* opt)
{
    if (!opt)
        return;

    memset(opt, 0, sizeof(*opt));
    opt->transform_flags = PNG_DEC_TRANSFORM_NONE;
    opt->output_format = PNG_OUTPUT_RGBA8;
    opt->keep_text = 1;
    opt->keep_unknown_chunks = PNG_DEC_KEEP_UNKNOWN_SAFE;
    opt->strict_trailing_data = 1;
    opt->max_pixels = PNG_DEC_MAX_PIXELS;
    opt->max_inflated_bytes = PNG_DEC_MAX_INFLATED_BYTES;
    opt->max_chunks = PNG_DEC_MAX_CHUNKS;
    opt->max_text_bytes = PNG_DEC_MAX_TEXT_BYTES;
    opt->max_apng_frames = PNG_DEC_MAX_APNG_FRAMES;
    opt->max_temp_bytes = PNG_DEC_MAX_TEMP_BYTES;
    opt->max_conversion_expansion = PNG_DEC_MAX_CONVERSION_EXPANSION;
}

void png_progressive_control_init(png_progressive_control* ctl)
{
    if (!ctl)
        return;

    memset(ctl, 0, sizeof(*ctl));
}

void png_progressive_poll_state_init(png_progressive_poll_state* st)
{
    if (!st)
        return;

    memset(st, 0, sizeof(*st));
}

void png_free_image(png_image* img)
{
    if (!img) return;

    if (img->pixels && img->pixels != img->rgba) {
        png_mem89_release(img->pixels);
        img->pixels = 0;
    }
    if (img->rgba) {
        png_mem89_release(img->rgba);
        img->rgba = 0;
    }
    if (img->iccp_name) {
        png_mem89_release(img->iccp_name);
        img->iccp_name = 0;
    }
    if (img->iccp_profile) {
        png_mem89_release(img->iccp_profile);
        img->iccp_profile = 0;
    }
    if (img->exif_profile) {
        png_mem89_release(img->exif_profile);
        img->exif_profile = 0;
    }
    if (img->dsig_chunks) {
        png_u32 i;
        for (i = 0u; i < img->dsig_chunk_count; ++i) {
            if (img->dsig_chunks[i].cms_data) {
                png_mem89_release(img->dsig_chunks[i].cms_data);
                img->dsig_chunks[i].cms_data = 0;
            }
        }
        png_mem89_release(img->dsig_chunks);
        img->dsig_chunks = 0;
        img->dsig_chunk_count = 0u;
    }
    png_free_gifg_entries_public(img->gifg_chunks);
    img->gifg_chunks = 0;
    img->gifg_chunk_count = 0u;
    png_free_gifx_entries_public(img->gifx_chunks, img->gifx_chunk_count);
    img->gifx_chunks = 0;
    img->gifx_chunk_count = 0u;
    png_free_gift_entries_public(img->gift_chunks, img->gift_chunk_count);
    img->gift_chunks = 0;
    img->gift_chunk_count = 0u;
    png_free_frac_entries_public(img->frac_chunks, img->frac_chunk_count);
    img->frac_chunks = 0;
    img->frac_chunk_count = 0u;
    if (img->scal_pixel_width) {
        png_mem89_release(img->scal_pixel_width);
        img->scal_pixel_width = 0;
    }
    if (img->scal_pixel_height) {
        png_mem89_release(img->scal_pixel_height);
        img->scal_pixel_height = 0;
    }
    if (img->hist_entries) {
        png_mem89_release(img->hist_entries);
        img->hist_entries = 0;
    }

    png_free_pcal_info_public(&img->pcal);
    png_free_splt_palettes_public(img->splt_palettes, img->splt_palette_count);
    png_free_text_entries_public(img->text_entries, img->text_count);
    png_free_unknown_chunks_public(img->unknown_chunks, img->unknown_chunk_count);

    memset(img, 0, sizeof(*img));
}

void png_free_file(png_u8* file_data)
{
    if (file_data)
        png_mem89_release(file_data);
}

static int safe_mul_u32(png_u32 a, png_u32 b, png_u32* out)
{
    png_u32 maxv;

    if (!out)
        return 0;

    maxv = (png_u32)~(png_u32)0;

    if (a != 0u && b > maxv / a)
        return 0;

    *out = a * b;
    return 1;
}

static int safe_add_u32_local(png_u32 a, png_u32 b, png_u32* out)
{
    if (!out)
        return 0;
    if (a > (png_u32)~(png_u32)0 - b)
        return 0;
    *out = a + b;
    return 1;
}

static png_u32 limit_or_default_local(png_u32 value, png_u32 fallback)
{
    return value != 0u ? value : fallback;
}

static int check_temp_limit2(png_u32 a, png_u32 b, png_u32 max_temp_bytes)
{
    png_u32 total;
    if (!safe_add_u32_local(a, b, &total))
        return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
    if (total > max_temp_bytes)
        return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
    return PNG_DEC_OK;
}

static int check_temp_limit3(png_u32 a, png_u32 b, png_u32 c, png_u32 max_temp_bytes)
{
    png_u32 total;
    if (!safe_add_u32_local(a, b, &total))
        return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
    if (!safe_add_u32_local(total, c, &total))
        return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
    if (total > max_temp_bytes)
        return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
    return PNG_DEC_OK;
}

static int check_conversion_limit_bytes(png_u32 input_size, png_u32 output_size, png_u32 max_output_bytes, png_u32 max_expansion)
{
    png_u32 limit;

    if (max_output_bytes != 0u && output_size > max_output_bytes)
        return PNG_DEC_ERR_CONVERSION_LIMIT;

    max_expansion = limit_or_default_local(max_expansion, PNG_DEC_MAX_CONVERSION_EXPANSION);
    if (input_size != 0u)
    {
        if (!safe_mul_u32(input_size, max_expansion, &limit))
            limit = (png_u32)~(png_u32)0;
        if (output_size > limit)
            return PNG_DEC_ERR_CONVERSION_LIMIT;
    }

    return PNG_DEC_OK;
}

int png_pcal_expected_param_count(png_u8 equation_type)
{
    switch (equation_type)
    {
        case 0u: return 2;
        case 1u: return 3;
        case 2u: return 3;
        case 3u: return 4;
        default: return -1;
    }
}

static int png_parse_ascii_fixed_local(const char* s, png_fixed89* out)
{
    return png_fixed89_parse_decimal(s, out);
}

int png_pcal_map_stored_to_original(png_u8 bit_depth,
                                    png_i32 x0,
                                    png_i32 x1,
                                    png_u32 stored_sample,
                                    png_i32* out_original)
{
    png_u32 max_sample;
    png_i32 delta;
    png_i32 q;
    png_i32 r;
    png_i32 add;

    if (!out_original)
        return PNG_DEC_ERR_FORMAT;
    if (!(bit_depth == 1u || bit_depth == 2u || bit_depth == 4u || bit_depth == 8u || bit_depth == 16u))
        return PNG_DEC_ERR_UNSUPPORTED;
    if (x0 == x1)
        return PNG_DEC_ERR_FORMAT;
    max_sample = (bit_depth == 16u) ? 65535u : ((1u << bit_depth) - 1u);
    if (stored_sample > max_sample)
        return PNG_DEC_ERR_FORMAT;

    delta = x1 - x0;
    q = delta / (png_i32)max_sample;
    r = delta % (png_i32)max_sample;
    add = (png_i32)stored_sample * q;
    add += ((png_i32)stored_sample * r + ((png_i32)max_sample / 2)) / (png_i32)max_sample;
    *out_original = x0 + add;
    return PNG_DEC_OK;
}

int png_pcal_map_original_to_physical(const png_pcal_info* pcal,
                                      png_i32 original_sample,
                                      png_fixed89* out_value)
{
    int expected_params;
    png_fixed89 params[4];
    png_fixed89 denom;
    png_fixed89 sample;
    png_fixed89 ratio;
    png_u32 i;
    png_fixed89 value;

    if (!pcal || !out_value)
        return PNG_DEC_ERR_FORMAT;
    expected_params = png_pcal_expected_param_count(pcal->equation_type);
    if (expected_params < 0 || pcal->param_count != (png_u32)expected_params)
        return PNG_DEC_ERR_FORMAT;
    if (pcal->x0 == pcal->x1)
        return PNG_DEC_ERR_FORMAT;
    for (i = 0u; i < pcal->param_count; ++i)
        if (!png_parse_ascii_fixed_local(pcal->params[i], &params[i]))
            return PNG_DEC_ERR_FORMAT;

    denom = (png_fixed89)((pcal->x1 - pcal->x0) * PNG_FIXED89_ONE);
    sample = (png_fixed89)(original_sample * PNG_FIXED89_ONE);
    ratio = png_fixed89_div(sample, denom);
    switch (pcal->equation_type)
    {
        case 0u:
            value = params[0] + png_fixed89_mul(params[1], ratio);
            break;
        case 1u:
            value = params[0] + png_fixed89_mul(params[1], png_fixed89_exp(png_fixed89_mul(params[2], ratio)));
            break;
        case 2u:
            value = params[0] + png_fixed89_mul(params[1], png_fixed89_pow_positive(params[2], ratio));
            break;
        case 3u:
            value = params[0] + png_fixed89_mul(params[1], png_fixed89_sinh(png_fixed89_mul(params[2], png_fixed89_div(sample - params[3], denom))));
            break;
        default:
            return PNG_DEC_ERR_FORMAT;
    }
    *out_value = value;
    return PNG_DEC_OK;
}

int png_pcal_map_stored_to_physical(png_u8 bit_depth,
                                    const png_pcal_info* pcal,
                                    png_u32 stored_sample,
                                    png_fixed89* out_value)
{
    png_i32 original_sample;
    int err;
    if (!pcal)
        return PNG_DEC_ERR_FORMAT;
    err = png_pcal_map_stored_to_original(bit_depth, pcal->x0, pcal->x1, stored_sample, &original_sample);
    if (err != PNG_DEC_OK)
        return err;
    return png_pcal_map_original_to_physical(pcal, original_sample, out_value);
}


static png_u16 read_be16_local(const png_u8* p)
{
    return (png_u16)(((png_u16)p[0] << 8) | (png_u16)p[1]);
}

static void write_be16_local(png_u8* p, png_u16 v)
{
    p[0] = (png_u8)(v >> 8);
    p[1] = (png_u8)(v & 0xFFu);
}

static png_u8 png_output_format_channels_local(png_u8 output_format)
{
    switch (output_format)
    {
        case PNG_OUTPUT_RGBA8:
        case PNG_OUTPUT_BGRA8:
        case PNG_OUTPUT_ARGB8:
        case PNG_OUTPUT_RGBA16:
        case PNG_OUTPUT_BGRA16:
        case PNG_OUTPUT_ARGB16:
            return 4u;
        case PNG_OUTPUT_RGB8:
        case PNG_OUTPUT_BGR8:
        case PNG_OUTPUT_RGB16:
        case PNG_OUTPUT_BGR16:
            return 3u;
        case PNG_OUTPUT_GA8:
        case PNG_OUTPUT_AG8:
        case PNG_OUTPUT_GA16:
        case PNG_OUTPUT_AG16:
            return 2u;
        case PNG_OUTPUT_G8:
        case PNG_OUTPUT_G16:
            return 1u;
        default:
            return 0u;
    }
}

static png_u8 png_output_format_bytes_per_channel_local(png_u8 output_format)
{
    switch (output_format)
    {
        case PNG_OUTPUT_RGBA16:
        case PNG_OUTPUT_BGRA16:
        case PNG_OUTPUT_ARGB16:
        case PNG_OUTPUT_RGB16:
        case PNG_OUTPUT_BGR16:
        case PNG_OUTPUT_GA16:
        case PNG_OUTPUT_AG16:
        case PNG_OUTPUT_G16:
            return 2u;
        case PNG_OUTPUT_RGBA8:
        case PNG_OUTPUT_BGRA8:
        case PNG_OUTPUT_ARGB8:
        case PNG_OUTPUT_RGB8:
        case PNG_OUTPUT_BGR8:
        case PNG_OUTPUT_GA8:
        case PNG_OUTPUT_AG8:
        case PNG_OUTPUT_G8:
            return 1u;
        default:
            return 0u;
    }
}

static png_u8 png_output_format_sample_depth_local(png_u8 output_format)
{
    png_u8 bpc = png_output_format_bytes_per_channel_local(output_format);
    return (png_u8)(bpc * 8u);
}

png_u8 png_output_format_channels(png_u8 output_format)
{
    return png_output_format_channels_local(output_format);
}

png_u8 png_output_format_bytes_per_channel(png_u8 output_format)
{
    return png_output_format_bytes_per_channel_local(output_format);
}

png_u8 png_output_format_sample_depth(png_u8 output_format)
{
    return png_output_format_sample_depth_local(output_format);
}

int png_output_format_rowbytes(png_u8 output_format, png_u32 width, png_u32* out_rowbytes)
{
    png_u8 channels;
    png_u8 bytes_per_channel;
    png_u32 pixel_bytes;

    if (!out_rowbytes)
        return PNG_DEC_ERR_FORMAT;

    channels = png_output_format_channels_local(output_format);
    bytes_per_channel = png_output_format_bytes_per_channel_local(output_format);
    if (channels == 0u || bytes_per_channel == 0u)
        return PNG_DEC_ERR_UNSUPPORTED;

    if (!safe_mul_u32((png_u32)channels, (png_u32)bytes_per_channel, &pixel_bytes))
        return PNG_DEC_ERR_UNSUPPORTED;

    if (!safe_mul_u32(width, pixel_bytes, out_rowbytes))
        return PNG_DEC_ERR_UNSUPPORTED;

    return PNG_DEC_OK;
}

static png_u8 png_gray_from_rgb(png_u8 r, png_u8 g, png_u8 b)
{
    png_u32 y;

    y = (png_u32)r * 2126u + (png_u32)g * 7152u + (png_u32)b * 722u + 5000u;
    return (png_u8)(y / 10000u);
}

static png_u16 png_gray_from_rgb16(png_u16 r, png_u16 g, png_u16 b)
{
    png_u32 y;

    y = (png_u32)r * 2126u + (png_u32)g * 7152u + (png_u32)b * 722u + 5000u;
    return (png_u16)(y / 10000u);
}

static png_u16 png_expand_8_to_16(png_u8 v)
{
    return (png_u16)(((png_u16)v << 8) | (png_u16)v);
}

static void png_unpack_pixel_rgba16(png_u8 format,
                                    const png_u8* src,
                                    png_u16* r, png_u16* g, png_u16* b, png_u16* a)
{
    switch (format)
    {
        case PNG_OUTPUT_RGBA8:
            *r = png_expand_8_to_16(src[0]); *g = png_expand_8_to_16(src[1]);
            *b = png_expand_8_to_16(src[2]); *a = png_expand_8_to_16(src[3]);
            break;
        case PNG_OUTPUT_BGRA8:
            *r = png_expand_8_to_16(src[2]); *g = png_expand_8_to_16(src[1]);
            *b = png_expand_8_to_16(src[0]); *a = png_expand_8_to_16(src[3]);
            break;
        case PNG_OUTPUT_ARGB8:
            *r = png_expand_8_to_16(src[1]); *g = png_expand_8_to_16(src[2]);
            *b = png_expand_8_to_16(src[3]); *a = png_expand_8_to_16(src[0]);
            break;
        case PNG_OUTPUT_RGB8:
            *r = png_expand_8_to_16(src[0]); *g = png_expand_8_to_16(src[1]);
            *b = png_expand_8_to_16(src[2]); *a = 65535u;
            break;
        case PNG_OUTPUT_BGR8:
            *r = png_expand_8_to_16(src[2]); *g = png_expand_8_to_16(src[1]);
            *b = png_expand_8_to_16(src[0]); *a = 65535u;
            break;
        case PNG_OUTPUT_GA8:
            *r = png_expand_8_to_16(src[0]); *g = *r; *b = *r; *a = png_expand_8_to_16(src[1]);
            break;
        case PNG_OUTPUT_AG8:
            *r = png_expand_8_to_16(src[1]); *g = *r; *b = *r; *a = png_expand_8_to_16(src[0]);
            break;
        case PNG_OUTPUT_G8:
            *r = png_expand_8_to_16(src[0]); *g = *r; *b = *r; *a = 65535u;
            break;
        case PNG_OUTPUT_RGBA16:
            *r = read_be16_local(src + 0); *g = read_be16_local(src + 2);
            *b = read_be16_local(src + 4); *a = read_be16_local(src + 6);
            break;
        case PNG_OUTPUT_BGRA16:
            *r = read_be16_local(src + 4); *g = read_be16_local(src + 2);
            *b = read_be16_local(src + 0); *a = read_be16_local(src + 6);
            break;
        case PNG_OUTPUT_ARGB16:
            *r = read_be16_local(src + 2); *g = read_be16_local(src + 4);
            *b = read_be16_local(src + 6); *a = read_be16_local(src + 0);
            break;
        case PNG_OUTPUT_RGB16:
            *r = read_be16_local(src + 0); *g = read_be16_local(src + 2);
            *b = read_be16_local(src + 4); *a = 65535u;
            break;
        case PNG_OUTPUT_BGR16:
            *r = read_be16_local(src + 4); *g = read_be16_local(src + 2);
            *b = read_be16_local(src + 0); *a = 65535u;
            break;
        case PNG_OUTPUT_GA16:
            *r = read_be16_local(src + 0); *g = *r; *b = *r; *a = read_be16_local(src + 2);
            break;
        case PNG_OUTPUT_AG16:
            *r = read_be16_local(src + 2); *g = *r; *b = *r; *a = read_be16_local(src + 0);
            break;
        case PNG_OUTPUT_G16:
            *r = read_be16_local(src + 0); *g = *r; *b = *r; *a = 65535u;
            break;
        default:
            *r = 0u; *g = 0u; *b = 0u; *a = 65535u;
            break;
    }
}

static void png_pack_pixel_rgba16(png_u8 format,
                                  png_u8* dst,
                                  png_u16 r, png_u16 g, png_u16 b, png_u16 a)
{
    png_u16 y;

    switch (format)
    {
        case PNG_OUTPUT_RGBA8:
            dst[0] = (png_u8)(r >> 8); dst[1] = (png_u8)(g >> 8); dst[2] = (png_u8)(b >> 8); dst[3] = (png_u8)(a >> 8);
            break;
        case PNG_OUTPUT_BGRA8:
            dst[0] = (png_u8)(b >> 8); dst[1] = (png_u8)(g >> 8); dst[2] = (png_u8)(r >> 8); dst[3] = (png_u8)(a >> 8);
            break;
        case PNG_OUTPUT_ARGB8:
            dst[0] = (png_u8)(a >> 8); dst[1] = (png_u8)(r >> 8); dst[2] = (png_u8)(g >> 8); dst[3] = (png_u8)(b >> 8);
            break;
        case PNG_OUTPUT_RGB8:
            dst[0] = (png_u8)(r >> 8); dst[1] = (png_u8)(g >> 8); dst[2] = (png_u8)(b >> 8);
            break;
        case PNG_OUTPUT_BGR8:
            dst[0] = (png_u8)(b >> 8); dst[1] = (png_u8)(g >> 8); dst[2] = (png_u8)(r >> 8);
            break;
        case PNG_OUTPUT_GA8:
        {
            png_u8 y8 = png_gray_from_rgb((png_u8)(r >> 8), (png_u8)(g >> 8), (png_u8)(b >> 8));
            dst[0] = y8; dst[1] = (png_u8)(a >> 8);
            break;
        }
        case PNG_OUTPUT_AG8:
        {
            png_u8 y8 = png_gray_from_rgb((png_u8)(r >> 8), (png_u8)(g >> 8), (png_u8)(b >> 8));
            dst[0] = (png_u8)(a >> 8); dst[1] = y8;
            break;
        }
        case PNG_OUTPUT_G8:
        {
            png_u8 y8 = png_gray_from_rgb((png_u8)(r >> 8), (png_u8)(g >> 8), (png_u8)(b >> 8));
            dst[0] = y8;
            break;
        }
        case PNG_OUTPUT_RGBA16:
            write_be16_local(dst + 0, r); write_be16_local(dst + 2, g); write_be16_local(dst + 4, b); write_be16_local(dst + 6, a);
            break;
        case PNG_OUTPUT_BGRA16:
            write_be16_local(dst + 0, b); write_be16_local(dst + 2, g); write_be16_local(dst + 4, r); write_be16_local(dst + 6, a);
            break;
        case PNG_OUTPUT_ARGB16:
            write_be16_local(dst + 0, a); write_be16_local(dst + 2, r); write_be16_local(dst + 4, g); write_be16_local(dst + 6, b);
            break;
        case PNG_OUTPUT_RGB16:
            write_be16_local(dst + 0, r); write_be16_local(dst + 2, g); write_be16_local(dst + 4, b);
            break;
        case PNG_OUTPUT_BGR16:
            write_be16_local(dst + 0, b); write_be16_local(dst + 2, g); write_be16_local(dst + 4, r);
            break;
        case PNG_OUTPUT_GA16:
            y = png_gray_from_rgb16(r, g, b);
            write_be16_local(dst + 0, y); write_be16_local(dst + 2, a);
            break;
        case PNG_OUTPUT_AG16:
            y = png_gray_from_rgb16(r, g, b);
            write_be16_local(dst + 0, a); write_be16_local(dst + 2, y);
            break;
        case PNG_OUTPUT_G16:
            y = png_gray_from_rgb16(r, g, b);
            write_be16_local(dst + 0, y);
            break;
        default:
            break;
    }
}

static int png_convert_row_between_formats(const png_u8* src,
                                           png_u8 src_format,
                                           png_u8* dst,
                                           png_u8 dst_format,
                                           png_u32 width)
{
    png_u8 src_channels;
    png_u8 dst_channels;
    png_u8 src_bpc;
    png_u8 dst_bpc;
    png_u32 x;

    if (!src || !dst)
        return PNG_DEC_ERR_FORMAT;

    src_channels = png_output_format_channels_local(src_format);
    dst_channels = png_output_format_channels_local(dst_format);
    src_bpc = png_output_format_bytes_per_channel_local(src_format);
    dst_bpc = png_output_format_bytes_per_channel_local(dst_format);
    if (src_channels == 0u || dst_channels == 0u || src_bpc == 0u || dst_bpc == 0u)
        return PNG_DEC_ERR_UNSUPPORTED;

    for (x = 0u; x < width; ++x)
    {
        png_u16 r, g, b, a;
        png_unpack_pixel_rgba16(src_format, src + (png_u32)x * src_channels * src_bpc, &r, &g, &b, &a);
        png_pack_pixel_rgba16(dst_format, dst + (png_u32)x * dst_channels * dst_bpc, r, g, b, a);
    }

    return PNG_DEC_OK;
}

static int png_convert_buffer_between_formats(const png_u8* src,
                                              png_u32 src_rowbytes,
                                              png_u8 src_format,
                                              png_u8* dst,
                                              png_u32 dst_rowbytes,
                                              png_u8 dst_format,
                                              png_u32 width,
                                              png_u32 height)
{
    png_u32 y;
    int err;

    if (!src || !dst)
        return PNG_DEC_ERR_FORMAT;

    for (y = 0u; y < height; ++y)
    {
        err = png_convert_row_between_formats(src + y * src_rowbytes,
                                              src_format,
                                              dst + y * dst_rowbytes,
                                              dst_format,
                                              width);
        if (err != PNG_DEC_OK)
            return err;
    }

    return PNG_DEC_OK;
}

static void png_assign_output_view(png_image* img,
                                   png_u8* pixels,
                                   png_u8 output_format,
                                   png_u32 rowbytes)
{
    if (!img)
        return;

    img->pixels = pixels;
    img->pixel_rowbytes = rowbytes;
    img->output_format = output_format;
    img->output_channels = png_output_format_channels_local(output_format);
    img->output_sample_depth = png_output_format_sample_depth_local(output_format);
    img->output_bytes_per_channel = png_output_format_bytes_per_channel_local(output_format);
    img->rgba = (output_format == PNG_OUTPUT_RGBA8) ? pixels : 0;
}

void png_conversion_limits_init(png_conversion_limits* limits)
{
    if (!limits)
        return;
    memset(limits, 0, sizeof(*limits));
    limits->max_output_bytes = PNG_DEC_MAX_IMAGE_BYTES;
    limits->max_expansion = PNG_DEC_MAX_CONVERSION_EXPANSION;
    limits->max_output_sample_depth = 16u;
}

int png_image_convert_format_ex(png_image* img, png_u8 output_format, const png_conversion_limits* limits)
{
    png_u32 new_rowbytes;
    png_u32 new_size;
    png_u8* dst;
    int err;
    png_conversion_limits local_limits;

    if (!img || !img->pixels)
        return PNG_DEC_ERR_FORMAT;

    if (img->output_format == output_format)
        return PNG_DEC_OK;

    png_conversion_limits_init(&local_limits);
    if (limits)
    {
        local_limits = *limits;
        if (local_limits.max_output_bytes == 0u)
            local_limits.max_output_bytes = PNG_DEC_MAX_IMAGE_BYTES;
        if (local_limits.max_expansion == 0u)
            local_limits.max_expansion = PNG_DEC_MAX_CONVERSION_EXPANSION;
    }

    if (local_limits.max_output_sample_depth != 0u &&
        png_output_format_sample_depth_local(output_format) > local_limits.max_output_sample_depth)
        return PNG_DEC_ERR_CONVERSION_LIMIT;

    err = png_output_format_rowbytes(output_format, img->width, &new_rowbytes);
    if (err != PNG_DEC_OK)
        return err;

    if (!safe_mul_u32(new_rowbytes, img->height, &new_size))
        return PNG_DEC_ERR_CONVERSION_LIMIT;

    err = check_conversion_limit_bytes(img->pixel_rowbytes * img->height, new_size,
                                       local_limits.max_output_bytes,
                                       local_limits.max_expansion);
    if (err != PNG_DEC_OK)
        return err;

    dst = (png_u8*)png_mem89_alloc(new_size == 0u ? 1u : new_size);
    if (!dst)
        return PNG_DEC_ERR_OOM;

    err = png_convert_buffer_between_formats(img->pixels,
                                             img->pixel_rowbytes,
                                             img->output_format,
                                             dst,
                                             new_rowbytes,
                                             output_format,
                                             img->width,
                                             img->height);
    if (err != PNG_DEC_OK)
    {
        png_mem89_release(dst);
        return err;
    }

    if (img->pixels && img->pixels != img->rgba)
        png_mem89_release(img->pixels);
    if (img->rgba)
        png_mem89_release(img->rgba);

    img->rgba = 0;
    png_assign_output_view(img, dst, output_format, new_rowbytes);
    return PNG_DEC_OK;
}

int png_image_convert_format(png_image* img, png_u8 output_format)
{
    return png_image_convert_format_ex(img, output_format, (const png_conversion_limits*)0);
}

static void png_apply_post_transforms_rgba(png_u8* p,
                                           png_u32 count,
                                           png_u32 flags)
{
    png_u32 i;
    png_u8 r;
    png_u8 g;
    png_u8 b;
    png_u8 a;

    if (!p)
        return;

    for (i = 0; i < count; ++i)
    {
        r = p[0];
        g = p[1];
        b = p[2];
        a = p[3];

        if (flags & PNG_DEC_TRANSFORM_PREMULTIPLY_ALPHA)
        {
            r = (png_u8)(((png_u32)r * (png_u32)a + 127u) / 255u);
            g = (png_u8)(((png_u32)g * (png_u32)a + 127u) / 255u);
            b = (png_u8)(((png_u32)b * (png_u32)a + 127u) / 255u);
        }

        if (flags & PNG_DEC_TRANSFORM_SWAP_RB)
        {
            png_u8 t = r;
            r = b;
            b = t;
        }

        if (flags & PNG_DEC_TRANSFORM_INVERT_ALPHA)
            a = (png_u8)(255u - a);

        if (flags & PNG_DEC_TRANSFORM_STRIP_ALPHA)
            a = 255u;

        p[0] = r;
        p[1] = g;
        p[2] = b;
        p[3] = a;
        p += 4;
    }
}

static void png_apply_post_transforms_rgba16(png_u8* p,
                                             png_u32 count,
                                             png_u32 flags)
{
    png_u32 i;

    if (!p)
        return;

    for (i = 0; i < count; ++i)
    {
        png_u16 r = read_be16_local(p + 0);
        png_u16 g = read_be16_local(p + 2);
        png_u16 b = read_be16_local(p + 4);
        png_u16 a = read_be16_local(p + 6);

        if (flags & PNG_DEC_TRANSFORM_PREMULTIPLY_ALPHA)
        {
            r = (png_u16)(((png_u32)r * (png_u32)a + 32767u) / 65535u);
            g = (png_u16)(((png_u32)g * (png_u32)a + 32767u) / 65535u);
            b = (png_u16)(((png_u32)b * (png_u32)a + 32767u) / 65535u);
        }

        if (flags & PNG_DEC_TRANSFORM_SWAP_RB)
        {
            png_u16 t = r;
            r = b;
            b = t;
        }

        if (flags & PNG_DEC_TRANSFORM_INVERT_ALPHA)
            a = (png_u16)(65535u - a);

        if (flags & PNG_DEC_TRANSFORM_STRIP_ALPHA)
            a = 65535u;

        write_be16_local(p + 0, r);
        write_be16_local(p + 2, g);
        write_be16_local(p + 4, b);
        write_be16_local(p + 6, a);
        p += 8;
    }
}

static void png_transfer_metadata_from_state(png_image* out_image, png_state* st)
{
    if (!out_image || !st)
        return;

    out_image->source_color_type = st->color_type;
    out_image->source_bit_depth = st->bit_depth;
    out_image->interlace_method = st->interlace_method;
    out_image->transform_flags_applied = st->transform_flags;

    if (st->have_gama || st->have_srgb)
        out_image->image_gamma = st->image_gamma;
    else
        out_image->image_gamma = 0;

    out_image->is_srgb = st->have_srgb ? 1 : 0;

    if (st->have_chrm) {
        out_image->has_cHRM = 1;
        out_image->white_x = st->white_x;
        out_image->white_y = st->white_y;
        out_image->red_x = st->red_x;
        out_image->red_y = st->red_y;
        out_image->green_x = st->green_x;
        out_image->green_y = st->green_y;
        out_image->blue_x = st->blue_x;
        out_image->blue_y = st->blue_y;
    }

    if (st->have_phys) {
        out_image->has_pHYs = 1;
        out_image->pHYs_ppu_x = st->phys_ppu_x;
        out_image->pHYs_ppu_y = st->phys_ppu_y;
        out_image->pHYs_unit  = st->phys_unit;
    }

    if (st->have_cicp) {
        out_image->has_cICP = 1;
        out_image->cicp = st->cicp;
    }

    if (st->have_mdcv) {
        out_image->has_mDCV = 1;
        out_image->mdcv = st->mdcv;
    }

    if (st->have_clli) {
        out_image->has_cLLI = 1;
        out_image->clli = st->clli;
    }

    if (st->have_offs) {
        out_image->has_oFFs = 1;
        out_image->offset_x = st->offs_x;
        out_image->offset_y = st->offs_y;
        out_image->offset_unit = st->offs_unit;
    }

    if (st->have_scal) {
        out_image->has_sCAL = 1;
        out_image->scal_unit = st->scal_unit;
        out_image->scal_pixel_width = st->scal_pixel_width;
        out_image->scal_pixel_height = st->scal_pixel_height;
        out_image->scal_width = st->scal_width;
        out_image->scal_height = st->scal_height;
        st->scal_pixel_width = 0;
        st->scal_pixel_height = 0;
    }

    if (st->have_ster) {
        out_image->has_sTER = 1;
        out_image->ster_mode = st->ster_mode;
    }

    if (st->have_pcal) {
        out_image->has_pCAL = 1;
        out_image->pcal = st->pcal;
        memset(&st->pcal, 0, sizeof(st->pcal));
        st->have_pcal = 0;
    }

    if (st->have_bkgd) {
        out_image->has_bKGD = 1;
        out_image->bkgd_r = st->bkgd_r;
        out_image->bkgd_g = st->bkgd_g;
        out_image->bkgd_b = st->bkgd_b;
    }

    if (st->have_time) {
        out_image->has_tIME = 1;
        out_image->time_year = st->time_year;
        out_image->time_month = st->time_month;
        out_image->time_day = st->time_day;
        out_image->time_hour = st->time_hour;
        out_image->time_minute = st->time_minute;
        out_image->time_second = st->time_second;
    }

    if (st->have_sbit) {
        out_image->has_sBIT = 1;
        out_image->sbit_r = st->sbit[0];
        out_image->sbit_g = st->sbit[1];
        out_image->sbit_b = st->sbit[2];
        out_image->sbit_a = st->sbit[3];
    }

    if (st->have_iccp) {
        out_image->has_iCCP = 1;
        out_image->iccp_name = st->iccp_name;
        out_image->iccp_compression_method = st->iccp_compression_method;
        out_image->iccp_profile = st->iccp_profile;
        out_image->iccp_profile_size = st->iccp_profile_size;
        st->iccp_name = 0;
        st->iccp_profile = 0;
        st->iccp_profile_size = 0u;
        st->have_iccp = 0;
    }

    if (st->have_exif) {
        out_image->has_eXIf = 1;
        out_image->exif_profile = st->exif_profile;
        out_image->exif_profile_size = st->exif_profile_size;
        st->exif_profile = 0;
        st->exif_profile_size = 0u;
        st->have_exif = 0;
    }

    out_image->dsig_chunks = st->dsig_chunks;
    out_image->dsig_chunk_count = st->dsig_chunk_count;
    st->dsig_chunks = 0;
    st->dsig_chunk_count = 0u;
    st->dsig_chunk_capacity = 0u;

    out_image->gifg_chunks = st->gifg_chunks;
    out_image->gifg_chunk_count = st->gifg_chunk_count;
    st->gifg_chunks = 0;
    st->gifg_chunk_count = 0u;
    st->gifg_chunk_capacity = 0u;

    out_image->gifx_chunks = st->gifx_chunks;
    out_image->gifx_chunk_count = st->gifx_chunk_count;
    st->gifx_chunks = 0;
    st->gifx_chunk_count = 0u;
    st->gifx_chunk_capacity = 0u;

    out_image->gift_chunks = st->gift_chunks;
    out_image->gift_chunk_count = st->gift_chunk_count;
    st->gift_chunks = 0;
    st->gift_chunk_count = 0u;
    st->gift_chunk_capacity = 0u;

    out_image->frac_chunks = st->frac_chunks;
    out_image->frac_chunk_count = st->frac_chunk_count;
    st->frac_chunks = 0;
    st->frac_chunk_count = 0u;
    st->frac_chunk_capacity = 0u;

    out_image->hist_entries = st->hist_entries;
    out_image->hist_count = st->hist_count;
    st->hist_entries = 0;
    st->hist_count = 0u;

    out_image->splt_palettes = st->splt_palettes;
    out_image->splt_palette_count = st->splt_palette_count;
    st->splt_palettes = 0;
    st->splt_palette_count = 0u;
    st->splt_palette_capacity = 0u;

    out_image->text_entries = st->text_entries;
    out_image->text_count = st->text_count;
    st->text_entries = 0;
    st->text_count = 0u;
    st->text_capacity = 0u;

    out_image->unknown_chunks = st->unknown_chunks;
    out_image->unknown_chunk_count = st->unknown_chunk_count;
    st->unknown_chunks = 0;
    st->unknown_chunk_count = 0u;
    st->unknown_chunk_capacity = 0u;
}

int png_decode_memory_ex(const png_u8* data,
                         png_u32 size,
                         png_zlib_decompress_func zfunc,
                         const png_decode_options* options,
                         png_image* out_image)
{
    png_state st;
    png_u8* img_data;
    png_u32 img_size;
    png_u32 pixels;
    png_u32 tmp_size;
    png_u32 tmp_rowbytes;
    png_u8 output_format;
    png_u8 output_depth;
    png_u32 output_rowbytes;
    png_u32 output_size;
    png_u8* tmp_pixels;
    png_u8* out_pixels;
    int err;

    if (!data || !out_image || !zfunc)
        return PNG_DEC_ERR_FORMAT;

    memset(out_image, 0, sizeof(*out_image));

    img_data = 0;
    img_size = 0u;
    tmp_pixels = 0;
    out_pixels = 0;
    output_format = options ? options->output_format : PNG_OUTPUT_RGBA8;
    output_depth = png_output_format_sample_depth_local(output_format);

    png_state_init(&st);
    png_state_set_decode_options(&st, zfunc, options, 0);

    err = png_output_format_rowbytes(output_format, 1u, &output_rowbytes);
    if (err != PNG_DEC_OK)
        goto cleanup;

    if (size > st.max_file_bytes) {
        err = PNG_DEC_ERR_CHUNK_TOO_LARGE;
        goto cleanup;
    }

    err = png_parse_png(data, size, zfunc, &st, &img_data, &img_size);
    if (err != PNG_DEC_OK)
        goto cleanup;

    out_image->width  = st.width;
    out_image->height = st.height;

    err = png_output_format_rowbytes(output_format, st.width, &output_rowbytes);
    if (err != PNG_DEC_OK)
        goto cleanup;

    if (!safe_mul_u32(st.width, st.height, &pixels)) {
        err = PNG_DEC_ERR_TOO_MANY_PIXELS;
        goto cleanup;
    }

    if (output_depth == 16u)
    {
        if (!safe_mul_u32(pixels, 8u, &tmp_size)) {
            err = PNG_DEC_ERR_TOO_MANY_PIXELS;
            goto cleanup;
        }
        tmp_rowbytes = st.width * 8u;
    }
    else
    {
        if (!safe_mul_u32(pixels, 4u, &tmp_size)) {
            err = PNG_DEC_ERR_TOO_MANY_PIXELS;
            goto cleanup;
        }
        tmp_rowbytes = st.width * 4u;
    }

    if (tmp_size > st.max_image_bytes) {
        err = PNG_DEC_ERR_UNSUPPORTED;
        goto cleanup;
    }

    err = check_temp_limit2(img_size, tmp_size, st.max_temp_bytes);
    if (err != PNG_DEC_OK)
        goto cleanup;

    tmp_pixels = (png_u8*)png_mem89_alloc(tmp_size == 0u ? 1u : tmp_size);
    if (!tmp_pixels) {
        err = PNG_DEC_ERR_OOM;
        goto cleanup;
    }

    png_transfer_metadata_from_state(out_image, &st);

    if (output_depth == 16u)
    {
        err = png_decode_image_data16(&st, img_data, img_size, tmp_pixels);
        if (err != PNG_DEC_OK)
            goto cleanup;

        png_apply_post_transforms_rgba16(tmp_pixels, pixels, st.transform_flags);

        if (output_format == PNG_OUTPUT_RGBA16)
        {
            png_assign_output_view(out_image, tmp_pixels, PNG_OUTPUT_RGBA16, output_rowbytes);
            tmp_pixels = 0;
        }
        else
        {
            if (!safe_mul_u32(output_rowbytes, st.height, &output_size)) {
                err = PNG_DEC_ERR_CONVERSION_LIMIT;
                goto cleanup;
            }
            err = check_conversion_limit_bytes(tmp_size, output_size, st.max_image_bytes, st.max_conversion_expansion);
            if (err != PNG_DEC_OK)
                goto cleanup;

            out_pixels = (png_u8*)png_mem89_alloc(output_size == 0u ? 1u : output_size);
            if (!out_pixels) {
                err = PNG_DEC_ERR_OOM;
                goto cleanup;
            }

            err = png_convert_buffer_between_formats(tmp_pixels,
                                                     tmp_rowbytes,
                                                     PNG_OUTPUT_RGBA16,
                                                     out_pixels,
                                                     output_rowbytes,
                                                     output_format,
                                                     st.width,
                                                     st.height);
            if (err != PNG_DEC_OK)
                goto cleanup;

            png_assign_output_view(out_image, out_pixels, output_format, output_rowbytes);
            out_pixels = 0;
        }

        if ((st.transform_flags & PNG_DEC_TRANSFORM_SWAP_16_ENDIAN) && out_image->pixels)
            png_swap_16_buffer(out_image->pixels, out_image->pixel_rowbytes * out_image->height);
    }
    else
    {
        err = png_decode_image_data(&st, img_data, img_size, tmp_pixels);
        if (err != PNG_DEC_OK)
            goto cleanup;

        png_apply_post_transforms_rgba(tmp_pixels, pixels, st.transform_flags);

        if (output_format == PNG_OUTPUT_RGBA8)
        {
            png_assign_output_view(out_image, tmp_pixels, PNG_OUTPUT_RGBA8, output_rowbytes);
            tmp_pixels = 0;
        }
        else
        {
            if (!safe_mul_u32(output_rowbytes, st.height, &output_size)) {
                err = PNG_DEC_ERR_CONVERSION_LIMIT;
                goto cleanup;
            }
            err = check_conversion_limit_bytes(tmp_size, output_size, st.max_image_bytes, st.max_conversion_expansion);
            if (err != PNG_DEC_OK)
                goto cleanup;

            out_pixels = (png_u8*)png_mem89_alloc(output_size == 0u ? 1u : output_size);
            if (!out_pixels) {
                err = PNG_DEC_ERR_OOM;
                goto cleanup;
            }

            err = png_convert_buffer_between_formats(tmp_pixels,
                                                     tmp_rowbytes,
                                                     PNG_OUTPUT_RGBA8,
                                                     out_pixels,
                                                     output_rowbytes,
                                                     output_format,
                                                     st.width,
                                                     st.height);
            if (err != PNG_DEC_OK)
                goto cleanup;

            png_assign_output_view(out_image, out_pixels, output_format, output_rowbytes);
            out_pixels = 0;
        }
    }

cleanup:
    if (img_data)
        png_mem89_release(img_data);
    if (tmp_pixels)
        png_mem89_release(tmp_pixels);
    if (out_pixels)
        png_mem89_release(out_pixels);

    png_state_free(&st);

    if (err != PNG_DEC_OK)
        png_free_image(out_image);

    return err;
}

int png_decode_memory(const png_u8* data,
                      png_u32 size,
                      png_zlib_decompress_func zfunc,
                      png_image* out_image)
{
    png_decode_options opt;

    png_decode_options_init(&opt);
    opt.transform_flags = PNG_DEC_TRANSFORM_APPLY_GAMMA;
    opt.keep_text = 0;
    opt.keep_unknown_chunks = PNG_DEC_KEEP_UNKNOWN_NEVER;
    opt.strict_trailing_data = 1;

    return png_decode_memory_ex(data, size, zfunc, &opt, out_image);
}

static int png_read_file_to_memory(const char* filename, png_u8** out_buf, png_u32* out_size)
{
    FILE* f;
    png_u8* buf;
    png_u32 size;
    png_u32 cap;

    if (!filename || !out_buf || !out_size)
        return PNG_DEC_ERR_FORMAT;
    *out_buf = 0;
    *out_size = 0u;

    f = fopen(filename, "rb");
    if (!f)
        return PNG_DEC_ERR_IO;

    cap = 4096u;
    if (cap > PNG_DEC_MAX_FILE_BYTES)
        cap = PNG_DEC_MAX_FILE_BYTES;
    buf = (png_u8*)png_mem89_alloc(cap == 0u ? 1u : cap);
    if (!buf)
    {
        fclose(f);
        return PNG_DEC_ERR_OOM;
    }

    size = 0u;
    for (;;)
    {
        png_u32 room;
        png_u32 got;
        if (size == cap)
        {
            png_u32 newcap;
            png_u8* nb;
            if (cap >= PNG_DEC_MAX_FILE_BYTES)
            {
                png_mem89_release(buf);
                fclose(f);
                return PNG_DEC_ERR_UNSUPPORTED;
            }
            newcap = cap <= PNG_DEC_MAX_FILE_BYTES / 2u ? cap * 2u : PNG_DEC_MAX_FILE_BYTES;
            nb = (png_u8*)png_mem89_resize(buf, newcap);
            if (!nb)
            {
                png_mem89_release(buf);
                fclose(f);
                return PNG_DEC_ERR_OOM;
            }
            buf = nb;
            cap = newcap;
        }
        room = cap - size;
        got = (png_u32)fread(buf + size, 1u, room, f);
        size += got;
        if (got < room)
        {
            if (ferror(f))
            {
                png_mem89_release(buf);
                fclose(f);
                return PNG_DEC_ERR_IO;
            }
            break;
        }
    }
    fclose(f);
    *out_buf = buf;
    *out_size = size;
    return PNG_DEC_OK;
}

int png_load_file_ex(const char* filename,
                     png_zlib_decompress_func zfunc,
                     const png_decode_options* options,
                     png_image* out_image)
{
    png_u8* buf;
    png_u32 sz;
    int err;

    buf = 0;
    sz = 0u;

    err = png_read_file_to_memory(filename, &buf, &sz);
    if (err != PNG_DEC_OK)
        return err;

    err = png_decode_memory_ex(buf, sz, zfunc, options, out_image);
    png_mem89_release(buf);
    return err;
}

int png_load_file(const char* filename,
                  png_zlib_decompress_func zfunc,
                  png_image* out_image)
{
    return png_load_file_ex(filename, zfunc, 0, out_image);
}

/* ---------------- Incremental decoder ---------------- */

struct png_decoder_s {
    png_zlib_decompress_func zfunc;

    png_u8* buffer;
    png_u32 size;
    png_u32 capacity;
    png_u32 parse_pos;
    png_u32 buffer_file_offset;

    int has_image;
    int parse_done;
    int sig_checked;
    png_image image;

    png_decode_options options;
    int options_set;

    png_progressive_callbacks callbacks;
    int info_emitted;
    png_progressive_control prog_ctl;
    png_u32 work_parse_bytes;
    png_u32 work_row_callbacks;
    png_u32 work_chunk_callbacks;
    png_u32 work_chunk_bytes;
    png_u32 work_zlib_bytes;
    png_u32 work_zlib_steps;

    png_state st;

    int pause_requested;
    int paused;
    int pause_save;
    png_u32 pause_unprocessed_bytes;
    png_u32 pause_resume_offset;
    png_u32 feed_start_offset;
    png_u32 feed_end_offset;
    int feed_active;

#ifdef PNG_DEC_USE_ZLIB
    int stream_initialized;
    int zstream_ended;
    int stream_interlaced;
    z_stream zstrm;
    png_u8* stream_scanline;
    png_u8* stream_row;
    png_u8* stream_prev_row;
    png_u8* stream_rgba_row;
    png_u8* stream_rgba16_row;
    png_u8* stream_public_row;
    png_u32 stream_rowbytes;
    png_u32 stream_bpp;
    png_u32 stream_row_fill;
    png_u32 stream_output_pixel_size;
    png_u32 next_row;
    png_u32 stream_pass;
    png_u32 stream_pass_width;
    png_u32 stream_pass_height;
    png_u32 stream_pass_row;
    png_u32 stream_pass_x_start;
    png_u32 stream_pass_y_start;
    png_u32 stream_pass_x_step;
    png_u32 stream_pass_y_step;
    int active_chunk_valid;
    png_u32 active_chunk_type;
    png_u32 active_chunk_start;
    png_u32 active_chunk_length;
    png_u32 active_chunk_data_offset;
    png_u32 active_chunk_next_pos;
    png_u32 active_chunk_data_consumed;
#endif
};

static png_u32 read_be32_local(const png_u8* p)
{
    return ((png_u32)p[0] << 24) |
           ((png_u32)p[1] << 16) |
           ((png_u32)p[2] <<  8) |
           (png_u32)p[3];
}

static png_u32 decoder_paused_cached_bytes(const png_decoder* dec)
{
    if (!dec)
        return 0u;
    if (dec->pause_resume_offset >= dec->size)
        return 0u;
    return dec->size - dec->pause_resume_offset;
}

static png_u32 decoder_compute_unprocessed_bytes(const png_decoder* dec, png_u32 unread_abs)
{
    if (!dec)
        return 0u;
    if (dec->feed_active)
    {
        if (unread_abs <= dec->feed_start_offset)
            return dec->feed_end_offset - dec->feed_start_offset;
        if (unread_abs < dec->feed_end_offset)
            return dec->feed_end_offset - unread_abs;
        return 0u;
    }
    if (unread_abs >= dec->size)
        return 0u;
    return dec->size - unread_abs;
}

static png_u32 decoder_current_unread_offset(const png_decoder* dec)
{
    if (!dec)
        return 0u;
#ifdef PNG_DEC_USE_ZLIB
    if (dec->active_chunk_valid)
    {
        png_u32 off = dec->active_chunk_data_offset + dec->active_chunk_data_consumed;
        if (off > dec->size)
            off = dec->size;
        return off;
    }
#endif
    if (dec->parse_pos > dec->size)
        return dec->size;
    return dec->parse_pos;
}

static void decoder_fill_poll_state(const png_decoder* dec, png_progressive_poll_state* st);

png_u32 png_decoder_unprocessed_bytes(const png_decoder* dec)
{
    if (!dec)
        return 0u;
    if (dec->paused)
        return decoder_paused_cached_bytes(dec);
    return decoder_compute_unprocessed_bytes(dec, decoder_current_unread_offset(dec));
}

png_u32 png_decoder_input_room(const png_decoder* dec)
{
    png_u32 unread;
    if (!dec)
        return 0u;
    if (dec->prog_ctl.max_buffered_bytes == 0u)
        return 0xFFFFFFFFu;
    unread = png_decoder_unprocessed_bytes(dec);
    if (unread >= dec->prog_ctl.max_buffered_bytes)
        return 0u;
    return dec->prog_ctl.max_buffered_bytes - unread;
}

int png_decoder_poll(const png_decoder* dec, png_progressive_poll_state* st)
{
    decoder_fill_poll_state(dec, st);
    return dec ? PNG_DEC_OK : PNG_DEC_ERR_FORMAT;
}

static void decoder_work_reset(png_decoder* dec)
{
    if (!dec)
        return;
    dec->work_parse_bytes = 0u;
    dec->work_row_callbacks = 0u;
    dec->work_chunk_callbacks = 0u;
    dec->work_chunk_bytes = 0u;
    dec->work_zlib_bytes = 0u;
    dec->work_zlib_steps = 0u;
}

static void decoder_fill_chunk_progress_info(png_chunk_progress_info* info,
                                             png_u32 type,
                                             png_u32 length,
                                             png_u32 file_offset)
{
    png_u8 c0;
    png_u8 c1;
    png_u8 c2;
    png_u8 c3;

    if (!info)
        return;

    memset(info, 0, sizeof(*info));
    info->type = type;
    info->length = length;
    info->file_offset = file_offset;
    info->data_offset = file_offset + 8u;
    info->total_size = length + 12u;
    c0 = (png_u8)(type >> 24);
    c1 = (png_u8)(type >> 16);
    c2 = (png_u8)(type >> 8);
    c3 = (png_u8)(type);
    info->ancillary = (png_u8)((c0 & 0x20u) ? 1u : 0u);
    info->private_bit = (png_u8)((c1 & 0x20u) ? 1u : 0u);
    info->reserved_bit = (png_u8)((c2 & 0x20u) ? 1u : 0u);
    info->safe_to_copy = (png_u8)((c3 & 0x20u) ? 1u : 0u);
}

static int decoder_budget_hit_result(const png_decoder* dec)
{
    if (dec && dec->prog_ctl.hard_fail_on_budget_exhaustion)
        return PNG_DEC_ERR_WORK_BUDGET;
    return PNG_DEC_YIELDED;
}

static int decoder_note_parse_bytes(png_decoder* dec, png_u32 bytes)
{
    if (!dec || bytes == 0u)
        return PNG_DEC_OK;
    if (0xFFFFFFFFu - dec->work_parse_bytes < bytes)
        dec->work_parse_bytes = 0xFFFFFFFFu;
    else
        dec->work_parse_bytes += bytes;
    if (dec->prog_ctl.max_parse_bytes_per_call != 0u &&
        dec->work_parse_bytes >= dec->prog_ctl.max_parse_bytes_per_call)
        return decoder_budget_hit_result(dec);
    return PNG_DEC_OK;
}

static int decoder_note_row_callback(png_decoder* dec)
{
    if (!dec)
        return PNG_DEC_OK;
    if (dec->work_row_callbacks != 0xFFFFFFFFu)
        dec->work_row_callbacks += 1u;
    if (dec->prog_ctl.max_row_callbacks_per_call != 0u &&
        dec->work_row_callbacks >= dec->prog_ctl.max_row_callbacks_per_call)
        return decoder_budget_hit_result(dec);
    return PNG_DEC_OK;
}

static int decoder_note_chunk_budget(png_decoder* dec, png_u32 chunk_total_size, int chunk_callback_emitted)
{
    if (!dec)
        return PNG_DEC_OK;

    if (chunk_callback_emitted && dec->work_chunk_callbacks != 0xFFFFFFFFu)
        dec->work_chunk_callbacks += 1u;

    if (chunk_total_size != 0u)
    {
        if (0xFFFFFFFFu - dec->work_chunk_bytes < chunk_total_size)
            dec->work_chunk_bytes = 0xFFFFFFFFu;
        else
            dec->work_chunk_bytes += chunk_total_size;
    }

    if (dec->prog_ctl.max_chunk_callbacks_per_call != 0u &&
        dec->work_chunk_callbacks >= dec->prog_ctl.max_chunk_callbacks_per_call)
        return decoder_budget_hit_result(dec);

    if (dec->prog_ctl.max_chunk_bytes_per_call != 0u &&
        dec->work_chunk_bytes >= dec->prog_ctl.max_chunk_bytes_per_call)
        return decoder_budget_hit_result(dec);

    return PNG_DEC_OK;
}

static int decoder_note_zlib_budget(png_decoder* dec, png_u32 produced_bytes)
{
    if (!dec)
        return PNG_DEC_OK;

    if (dec->work_zlib_steps != 0xFFFFFFFFu)
        dec->work_zlib_steps += 1u;

    if (produced_bytes != 0u)
    {
        if (0xFFFFFFFFu - dec->work_zlib_bytes < produced_bytes)
            dec->work_zlib_bytes = 0xFFFFFFFFu;
        else
            dec->work_zlib_bytes += produced_bytes;
    }

    if (dec->prog_ctl.max_zlib_steps_per_call != 0u &&
        dec->work_zlib_steps >= dec->prog_ctl.max_zlib_steps_per_call)
        return decoder_budget_hit_result(dec);

    if (dec->prog_ctl.max_zlib_work_bytes_per_call != 0u &&
        dec->work_zlib_bytes >= dec->prog_ctl.max_zlib_work_bytes_per_call)
        return decoder_budget_hit_result(dec);

    return PNG_DEC_OK;
}

static int decoder_emit_chunk_callback(png_decoder* dec, png_u32 type, png_u32 length, png_u32 file_offset)
{
    png_chunk_progress_info info;
    int emitted = 0;
    int err;
    png_u32 absolute_offset = file_offset;

    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    if (0xFFFFFFFFu - dec->buffer_file_offset < file_offset)
        absolute_offset = 0xFFFFFFFFu;
    else
        absolute_offset = dec->buffer_file_offset + file_offset;

    if (dec->callbacks.chunk_fn)
    {
        decoder_fill_chunk_progress_info(&info, type, length, absolute_offset);
        dec->callbacks.chunk_fn(dec->callbacks.user_ptr, &info);
        emitted = 1;
    }

    if (dec->pause_requested)
        return PNG_DEC_PAUSED;

    err = decoder_note_chunk_budget(dec, length + 12u, emitted);
    if (err != PNG_DEC_OK)
        return err;

    return PNG_DEC_OK;
}

static png_u32 decoder_suggest_read_bytes(const png_decoder* dec, png_u32 room)
{
    png_u32 suggested;

    if (!dec || room == 0u)
        return 0u;
    suggested = room;
    if (suggested == 0xFFFFFFFFu)
        suggested = dec->prog_ctl.max_feed_bytes ? dec->prog_ctl.max_feed_bytes : 65536u;
    else if (dec->prog_ctl.max_feed_bytes != 0u && suggested > dec->prog_ctl.max_feed_bytes)
        suggested = dec->prog_ctl.max_feed_bytes;
    if (suggested == 0u)
        suggested = 1u;
    return suggested;
}

static int decoder_can_drain_no_input(const png_decoder* dec)
{
    png_u32 length;
    png_u32 next_pos;

    if (!dec || dec->has_image || dec->parse_done)
        return 0;
#ifdef PNG_DEC_USE_ZLIB
    if (dec->active_chunk_valid)
        return 1;
#endif
    if (dec->st.seen_IEND && dec->parse_pos == dec->size)
        return 1;
    if (!dec->sig_checked)
        return dec->size >= 8u;
    if (dec->parse_pos + 8u > dec->size)
        return 0;
    length = read_be32_local(dec->buffer + dec->parse_pos);
    if (length > 0x7FFFFFFFu)
        return 1;
    next_pos = dec->parse_pos + 8u + length + 4u;
    return next_pos <= dec->size;
}

static void decoder_fill_poll_state(const png_decoder* dec, png_progressive_poll_state* st)
{
    png_u32 room;
    png_u32 unread;
    png_u32 pending;

    if (!st)
        return;

    png_progressive_poll_state_init(st);
    if (!dec)
        return;

    room = png_decoder_input_room(dec);
    unread = png_decoder_unprocessed_bytes(dec);
    pending = png_decoder_pending_bytes(dec);
    st->input_room = room;
    st->unprocessed_bytes = unread;
    st->pending_bytes = pending;

    if (dec->has_image)
    {
        st->events |= PNG_PROGRESSIVE_POLL_DONE;
        return;
    }

    if (unread != 0u || pending != 0u)
        st->events |= PNG_PROGRESSIVE_POLL_HAVE_BUFFERED;

    if (dec->paused)
    {
        st->events |= PNG_PROGRESSIVE_POLL_PAUSED;
        if (dec->pause_save && pending != 0u)
            st->events |= PNG_PROGRESSIVE_POLL_CAN_DRAIN;
        else if (pending != 0u)
            st->events |= PNG_PROGRESSIVE_POLL_WANT_INPUT;
    }
    else if (decoder_can_drain_no_input(dec))
        st->events |= PNG_PROGRESSIVE_POLL_CAN_DRAIN;

    if (room != 0u && !dec->has_image && (!dec->paused || (pending != 0u && !dec->pause_save) || !decoder_can_drain_no_input(dec)))
        st->events |= PNG_PROGRESSIVE_POLL_WANT_INPUT;

    if ((st->events & PNG_PROGRESSIVE_POLL_WANT_INPUT) != 0u)
        st->suggested_read_bytes = decoder_suggest_read_bytes(dec, room);
}

static void decoder_drop_prefix(png_decoder* dec, png_u32 drop)
{
    if (!dec || drop == 0u)
        return;
    if (drop > dec->size)
        drop = dec->size;
    if (drop < dec->size)
        memmove(dec->buffer, dec->buffer + drop, dec->size - drop);
    dec->size -= drop;
    if (0xFFFFFFFFu - dec->buffer_file_offset < drop)
        dec->buffer_file_offset = 0xFFFFFFFFu;
    else
        dec->buffer_file_offset += drop;
    if (dec->parse_pos >= drop) dec->parse_pos -= drop; else dec->parse_pos = 0u;
    if (dec->pause_resume_offset >= drop) dec->pause_resume_offset -= drop; else dec->pause_resume_offset = 0u;
    if (dec->feed_start_offset >= drop) dec->feed_start_offset -= drop; else dec->feed_start_offset = 0u;
    if (dec->feed_end_offset >= drop) dec->feed_end_offset -= drop; else dec->feed_end_offset = dec->size;
#ifdef PNG_DEC_USE_ZLIB
    if (dec->active_chunk_valid)
    {
        png_u32 old_data_offset = dec->active_chunk_data_offset;
        png_u32 old_length = dec->active_chunk_length;
        png_u32 old_consumed = dec->active_chunk_data_consumed;
        if (dec->active_chunk_start >= drop) dec->active_chunk_start -= drop; else dec->active_chunk_start = 0u;
        if (dec->active_chunk_next_pos >= drop) dec->active_chunk_next_pos -= drop; else dec->active_chunk_next_pos = 0u;
        if (drop <= old_data_offset)
        {
            dec->active_chunk_data_offset = old_data_offset - drop;
            dec->active_chunk_length = old_length;
            dec->active_chunk_data_consumed = old_consumed;
        }
        else
        {
            png_u32 skipped_in_data = drop - old_data_offset;
            if (skipped_in_data > old_length)
                skipped_in_data = old_length;
            dec->active_chunk_data_offset = 0u;
            dec->active_chunk_length = old_length - skipped_in_data;
            if (old_consumed > skipped_in_data)
                dec->active_chunk_data_consumed = old_consumed - skipped_in_data;
            else
                dec->active_chunk_data_consumed = 0u;
        }
    }
#endif
}

static void decoder_maybe_compact_buffer(png_decoder* dec)
{
    png_u32 unread_abs;
    png_u32 unread;
    if (!dec || !dec->buffer || dec->size == 0u)
        return;
#ifdef PNG_DEC_USE_ZLIB
    if (dec->active_chunk_valid)
        return;
#endif
    unread_abs = dec->paused ? dec->pause_resume_offset : decoder_current_unread_offset(dec);
    if (unread_abs == 0u || unread_abs > dec->size)
        return;
    unread = dec->size - unread_abs;
    if (unread_abs >= 65536u || unread_abs >= unread)
        decoder_drop_prefix(dec, unread_abs);
}

static int decoder_pause_here(png_decoder* dec, png_u32 unread_abs)
{
    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    if (unread_abs > dec->size)
        unread_abs = dec->size;

    dec->paused = 1;
    dec->pause_resume_offset = unread_abs;
    dec->pause_unprocessed_bytes = decoder_compute_unprocessed_bytes(dec, unread_abs);
    dec->pause_requested = 0;

    if (!dec->pause_save)
    {
        dec->size = unread_abs;
        if (dec->parse_pos > dec->size)
            dec->parse_pos = dec->size;
    }

    return PNG_DEC_PAUSED;
}

#ifdef PNG_DEC_USE_ZLIB
static int decoder_stream_process_idat(png_decoder* dec, const png_u8* data, png_u32 size, png_u32* consumed_out);
static int decoder_stream_finish_no_input(png_decoder* dec);
#endif

static int decoder_resume_active_chunk(png_decoder* dec)
{
#ifdef PNG_DEC_USE_ZLIB
    int err;
    png_u32 consumed = 0u;

    if (!dec || !dec->active_chunk_valid)
        return PNG_DEC_OK;

    if (dec->active_chunk_type != ((png_u32)'I'<<24 | (png_u32)'D'<<16 | (png_u32)'A'<<8 | (png_u32)'T'))
        return PNG_DEC_ERR_FORMAT;

    err = decoder_stream_process_idat(dec,
                                      dec->buffer + dec->active_chunk_data_offset + dec->active_chunk_data_consumed,
                                      dec->active_chunk_length - dec->active_chunk_data_consumed,
                                      &consumed);
    dec->active_chunk_data_consumed += consumed;
    if (err == PNG_DEC_PAUSED)
        return decoder_pause_here(dec, dec->active_chunk_data_offset + dec->active_chunk_data_consumed);
    if (err != PNG_DEC_OK)
        return err;
    if (dec->active_chunk_data_consumed == dec->active_chunk_length && !dec->zstream_ended)
    {
        err = decoder_stream_finish_no_input(dec);
        if (err != PNG_DEC_OK)
            return err;
    }
    if (dec->active_chunk_data_consumed != dec->active_chunk_length)
        return PNG_DEC_ERR_FORMAT;
    dec->parse_pos = dec->active_chunk_next_pos;
    dec->active_chunk_valid = 0;
    return PNG_DEC_OK;
#else
    (void)dec;
    return PNG_DEC_OK;
#endif
}

static const png_u8 PNG_SIG_LOCAL[8] = {
    0x89, 'P','N','G', 0x0D,0x0A,0x1A,0x0A
};

static void decoder_emit_header_from_image(png_decoder* dec, const png_image* img)
{
    png_image header_only;

    if (!dec || dec->info_emitted || !dec->callbacks.info_fn || !img)
        return;

    memset(&header_only, 0, sizeof(header_only));
    header_only.width = img->width;
    header_only.height = img->height;
    header_only.output_format = img->output_format;
    header_only.output_channels = img->output_channels;
    header_only.output_sample_depth = img->output_sample_depth;
    header_only.output_bytes_per_channel = img->output_bytes_per_channel;
    header_only.pixel_rowbytes = img->pixel_rowbytes;
    header_only.source_color_type = img->source_color_type;
    header_only.source_bit_depth = img->source_bit_depth;
    header_only.interlace_method = img->interlace_method;
    header_only.image_gamma = img->image_gamma;
    header_only.is_srgb = img->is_srgb;
    header_only.has_cHRM = img->has_cHRM;
    header_only.white_x = img->white_x;
    header_only.white_y = img->white_y;
    header_only.red_x = img->red_x;
    header_only.red_y = img->red_y;
    header_only.green_x = img->green_x;
    header_only.green_y = img->green_y;
    header_only.blue_x = img->blue_x;
    header_only.blue_y = img->blue_y;
    header_only.has_pHYs = img->has_pHYs;
    header_only.pHYs_ppu_x = img->pHYs_ppu_x;
    header_only.pHYs_ppu_y = img->pHYs_ppu_y;
    header_only.pHYs_unit = img->pHYs_unit;
    header_only.has_cICP = img->has_cICP;
    header_only.cicp = img->cicp;
    header_only.has_mDCV = img->has_mDCV;
    header_only.mdcv = img->mdcv;
    header_only.has_cLLI = img->has_cLLI;
    header_only.clli = img->clli;
    header_only.has_oFFs = img->has_oFFs;
    header_only.offset_x = img->offset_x;
    header_only.offset_y = img->offset_y;
    header_only.offset_unit = img->offset_unit;
    header_only.has_sCAL = img->has_sCAL;
    header_only.scal_unit = img->scal_unit;
    header_only.scal_pixel_width = img->scal_pixel_width;
    header_only.scal_pixel_height = img->scal_pixel_height;
    header_only.scal_width = img->scal_width;
    header_only.scal_height = img->scal_height;
    header_only.has_sTER = img->has_sTER;
    header_only.ster_mode = img->ster_mode;
    header_only.has_pCAL = img->has_pCAL;
    header_only.pcal = img->pcal;
    header_only.has_tIME = img->has_tIME;
    header_only.time_year = img->time_year;
    header_only.time_month = img->time_month;
    header_only.time_day = img->time_day;
    header_only.time_hour = img->time_hour;
    header_only.time_minute = img->time_minute;
    header_only.time_second = img->time_second;
    header_only.has_sBIT = img->has_sBIT;
    header_only.sbit_r = img->sbit_r;
    header_only.sbit_g = img->sbit_g;
    header_only.sbit_b = img->sbit_b;
    header_only.sbit_a = img->sbit_a;
    header_only.has_iCCP = img->has_iCCP;
    header_only.iccp_name = img->iccp_name;
    header_only.iccp_compression_method = img->iccp_compression_method;
    header_only.iccp_profile = img->iccp_profile;
    header_only.iccp_profile_size = img->iccp_profile_size;
    header_only.has_eXIf = img->has_eXIf;
    header_only.exif_profile = img->exif_profile;
    header_only.exif_profile_size = img->exif_profile_size;
    header_only.dsig_chunks = img->dsig_chunks;
    header_only.dsig_chunk_count = img->dsig_chunk_count;
    header_only.gifg_chunks = img->gifg_chunks;
    header_only.gifg_chunk_count = img->gifg_chunk_count;
    header_only.gifx_chunks = img->gifx_chunks;
    header_only.gifx_chunk_count = img->gifx_chunk_count;
    header_only.gift_chunks = img->gift_chunks;
    header_only.gift_chunk_count = img->gift_chunk_count;
    header_only.frac_chunks = img->frac_chunks;
    header_only.frac_chunk_count = img->frac_chunk_count;
    header_only.hist_entries = img->hist_entries;
    header_only.hist_count = img->hist_count;
    header_only.splt_palettes = img->splt_palettes;
    header_only.splt_palette_count = img->splt_palette_count;
    header_only.text_entries = img->text_entries;
    header_only.text_count = img->text_count;
    header_only.unknown_chunks = img->unknown_chunks;
    header_only.unknown_chunk_count = img->unknown_chunk_count;

    dec->callbacks.info_fn(dec->callbacks.user_ptr, &header_only);
    dec->info_emitted = 1;
}

static void decoder_emit_header_from_state(png_decoder* dec)
{
    png_image header_only;

    if (!dec || dec->info_emitted || !dec->callbacks.info_fn)
        return;

    if (!dec->st.seen_IHDR)
        return;

    memset(&header_only, 0, sizeof(header_only));
    header_only.width = dec->st.width;
    header_only.height = dec->st.height;
    header_only.output_format = dec->options.output_format;
    header_only.output_channels = png_output_format_channels_local(dec->options.output_format);
    header_only.output_sample_depth = png_output_format_sample_depth_local(dec->options.output_format);
    header_only.output_bytes_per_channel = png_output_format_bytes_per_channel_local(dec->options.output_format);
    if (png_output_format_rowbytes(dec->options.output_format, dec->st.width, &header_only.pixel_rowbytes) != PNG_DEC_OK)
        header_only.pixel_rowbytes = 0u;
    header_only.source_color_type = dec->st.color_type;
    header_only.source_bit_depth = dec->st.bit_depth;
    header_only.interlace_method = dec->st.interlace_method;
    header_only.image_gamma = (dec->st.have_gama || dec->st.have_srgb) ? dec->st.image_gamma : 0;
    header_only.is_srgb = dec->st.have_srgb ? 1 : 0;
    header_only.has_cHRM = dec->st.have_chrm ? 1 : 0;
    header_only.white_x = dec->st.white_x;
    header_only.white_y = dec->st.white_y;
    header_only.red_x = dec->st.red_x;
    header_only.red_y = dec->st.red_y;
    header_only.green_x = dec->st.green_x;
    header_only.green_y = dec->st.green_y;
    header_only.blue_x = dec->st.blue_x;
    header_only.blue_y = dec->st.blue_y;
    header_only.has_pHYs = dec->st.have_phys ? 1 : 0;
    header_only.pHYs_ppu_x = dec->st.phys_ppu_x;
    header_only.pHYs_ppu_y = dec->st.phys_ppu_y;
    header_only.pHYs_unit = dec->st.phys_unit;
    header_only.has_cICP = dec->st.have_cicp ? 1 : 0;
    header_only.cicp = dec->st.cicp;
    header_only.has_mDCV = dec->st.have_mdcv ? 1 : 0;
    header_only.mdcv = dec->st.mdcv;
    header_only.has_cLLI = dec->st.have_clli ? 1 : 0;
    header_only.clli = dec->st.clli;
    header_only.has_oFFs = dec->st.have_offs ? 1 : 0;
    header_only.offset_x = dec->st.offs_x;
    header_only.offset_y = dec->st.offs_y;
    header_only.offset_unit = dec->st.offs_unit;
    header_only.has_sCAL = dec->st.have_scal ? 1 : 0;
    header_only.scal_unit = dec->st.scal_unit;
    header_only.scal_pixel_width = dec->st.scal_pixel_width;
    header_only.scal_pixel_height = dec->st.scal_pixel_height;
    header_only.scal_width = dec->st.scal_width;
    header_only.scal_height = dec->st.scal_height;
    header_only.has_sTER = dec->st.have_ster ? 1 : 0;
    header_only.ster_mode = dec->st.ster_mode;
    header_only.has_pCAL = dec->st.have_pcal ? 1 : 0;
    header_only.pcal = dec->st.pcal;
    header_only.has_tIME = dec->st.have_time ? 1 : 0;
    header_only.time_year = dec->st.time_year;
    header_only.time_month = dec->st.time_month;
    header_only.time_day = dec->st.time_day;
    header_only.time_hour = dec->st.time_hour;
    header_only.time_minute = dec->st.time_minute;
    header_only.time_second = dec->st.time_second;
    header_only.has_sBIT = dec->st.have_sbit ? 1 : 0;
    header_only.sbit_r = dec->st.sbit[0];
    header_only.sbit_g = dec->st.sbit[1];
    header_only.sbit_b = dec->st.sbit[2];
    header_only.sbit_a = dec->st.sbit[3];
    header_only.has_iCCP = dec->st.have_iccp ? 1 : 0;
    header_only.iccp_name = dec->st.iccp_name;
    header_only.iccp_compression_method = dec->st.iccp_compression_method;
    header_only.iccp_profile = dec->st.iccp_profile;
    header_only.iccp_profile_size = dec->st.iccp_profile_size;
    header_only.has_eXIf = dec->st.have_exif ? 1 : 0;
    header_only.exif_profile = dec->st.exif_profile;
    header_only.exif_profile_size = dec->st.exif_profile_size;
    header_only.dsig_chunks = dec->st.dsig_chunks;
    header_only.dsig_chunk_count = dec->st.dsig_chunk_count;
    header_only.gifg_chunks = dec->st.gifg_chunks;
    header_only.gifg_chunk_count = dec->st.gifg_chunk_count;
    header_only.gifx_chunks = dec->st.gifx_chunks;
    header_only.gifx_chunk_count = dec->st.gifx_chunk_count;
    header_only.gift_chunks = dec->st.gift_chunks;
    header_only.gift_chunk_count = dec->st.gift_chunk_count;
    header_only.frac_chunks = dec->st.frac_chunks;
    header_only.frac_chunk_count = dec->st.frac_chunk_count;
    header_only.hist_entries = dec->st.hist_entries;
    header_only.hist_count = dec->st.hist_count;
    header_only.splt_palettes = dec->st.splt_palettes;
    header_only.splt_palette_count = dec->st.splt_palette_count;
    header_only.text_entries = dec->st.text_entries;
    header_only.text_count = dec->st.text_count;
    header_only.unknown_chunks = dec->st.unknown_chunks;
    header_only.unknown_chunk_count = dec->st.unknown_chunk_count;

    dec->callbacks.info_fn(dec->callbacks.user_ptr, &header_only);
    dec->info_emitted = 1;
}

int png_decoder_init(png_decoder** dec_out, png_zlib_decompress_func zfunc)
{
    png_decoder* dec;

    if (!dec_out || !zfunc)
        return PNG_DEC_ERR_FORMAT;

    *dec_out = 0;

    dec = (png_decoder*)png_mem89_alloc(sizeof(*dec));
    if (!dec)
        return PNG_DEC_ERR_OOM;

    memset(dec, 0, sizeof(*dec));
    dec->zfunc = zfunc;
    png_decode_options_init(&dec->options);
    png_progressive_control_init(&dec->prog_ctl);
    png_state_init(&dec->st);
    png_state_set_decode_options(&dec->st, zfunc, &dec->options, 0);

    *dec_out = dec;
    return PNG_DEC_OK;
}

int png_decoder_set_options(png_decoder* dec, const png_decode_options* options)
{
    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    if (dec->size != 0u || dec->parse_pos != 0u || dec->st.seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    if (options)
        dec->options = *options;
    else
        png_decode_options_init(&dec->options);

    png_state_set_decode_options(&dec->st, dec->zfunc, &dec->options, 0);
    dec->options_set = 1;
    return PNG_DEC_OK;
}

int png_decoder_set_callbacks(png_decoder* dec, const png_progressive_callbacks* callbacks)
{
    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    if (callbacks)
        dec->callbacks = *callbacks;
    else
        memset(&dec->callbacks, 0, sizeof(dec->callbacks));

    return PNG_DEC_OK;
}

int png_decoder_set_progressive_control(png_decoder* dec, const png_progressive_control* ctl)
{
    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    if (dec->size != 0u || dec->parse_pos != 0u || dec->st.seen_IHDR)
        return PNG_DEC_ERR_FORMAT;

    if (ctl)
        dec->prog_ctl = *ctl;
    else
        png_progressive_control_init(&dec->prog_ctl);

    return PNG_DEC_OK;
}

static int decoder_reserve(png_decoder* dec, png_u32 extra)
{
    png_u32 needed;
    png_u32 newcap;
    png_u8* nb;

    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    if (extra > dec->st.max_file_bytes)
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    if (dec->size > dec->st.max_file_bytes - extra)
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    needed = dec->size + extra;

    if (needed <= dec->capacity)
        return PNG_DEC_OK;

    newcap = (dec->capacity == 0u) ? 4096u : dec->capacity;
    while (newcap < needed) {
        if (newcap > dec->st.max_file_bytes / 2u)
            return PNG_DEC_ERR_CHUNK_TOO_LARGE;
        if (newcap > dec->st.max_temp_bytes)
            return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
        newcap *= 2u;
    }

    nb = (png_u8*)png_mem89_resize(dec->buffer, newcap);
    if (!nb)
        return PNG_DEC_ERR_OOM;

    dec->buffer = nb;
    dec->capacity = newcap;
    return PNG_DEC_OK;
}

#ifdef PNG_DEC_USE_ZLIB
static void decoder_stream_cleanup(png_decoder* dec)
{
    if (!dec)
        return;

    if (dec->stream_initialized)
        inflateEnd(&dec->zstrm);

    dec->stream_initialized = 0;
    dec->zstream_ended = 0;
    dec->stream_interlaced = 0;
    dec->stream_row_fill = 0u;
    dec->stream_output_pixel_size = 0u;
    dec->next_row = 0u;
    dec->stream_pass = 0u;
    dec->stream_pass_width = 0u;
    dec->stream_pass_height = 0u;
    dec->stream_pass_row = 0u;
    dec->stream_pass_x_start = 0u;
    dec->stream_pass_y_start = 0u;
    dec->stream_pass_x_step = 0u;
    dec->stream_pass_y_step = 0u;

    if (dec->stream_scanline) {
        png_mem89_release(dec->stream_scanline);
        dec->stream_scanline = 0;
    }
    if (dec->stream_row) {
        png_mem89_release(dec->stream_row);
        dec->stream_row = 0;
    }
    if (dec->stream_prev_row) {
        png_mem89_release(dec->stream_prev_row);
        dec->stream_prev_row = 0;
    }
    if (dec->stream_rgba_row) {
        png_mem89_release(dec->stream_rgba_row);
        dec->stream_rgba_row = 0;
    }
    if (dec->stream_rgba16_row) {
        png_mem89_release(dec->stream_rgba16_row);
        dec->stream_rgba16_row = 0;
    }
    if (dec->stream_public_row) {
        png_mem89_release(dec->stream_public_row);
        dec->stream_public_row = 0;
    }
    dec->stream_rowbytes = 0u;
    dec->stream_bpp = 0u;
}

static int decoder_stream_advance_pass(png_decoder* dec)
{
    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    while (dec->stream_pass < 7u)
    {
        png_u32 x_start;
        png_u32 y_start;
        png_u32 x_step;
        png_u32 y_step;
        png_u32 pw;
        png_u32 ph;
        png_u32 rowbytes;

        x_start = (png_u32)PNG_ADAM7_X_START[dec->stream_pass];
        y_start = (png_u32)PNG_ADAM7_Y_START[dec->stream_pass];
        x_step  = (png_u32)PNG_ADAM7_X_STEP[dec->stream_pass];
        y_step  = (png_u32)PNG_ADAM7_Y_STEP[dec->stream_pass];

        pw = (dec->st.width > x_start)
           ? ((dec->st.width - x_start + x_step - 1u) / x_step)
           : 0u;
        ph = (dec->st.height > y_start)
           ? ((dec->st.height - y_start + y_step - 1u) / y_step)
           : 0u;

        if (pw == 0u || ph == 0u)
        {
            dec->stream_pass += 1u;
            continue;
        }

        rowbytes = png_rowbytes_for_width_internal(&dec->st, pw);
        if (rowbytes == 0u)
            return PNG_DEC_ERR_UNSUPPORTED;

        dec->stream_pass_width = pw;
        dec->stream_pass_height = ph;
        dec->stream_pass_row = 0u;
        dec->stream_pass_x_start = x_start;
        dec->stream_pass_y_start = y_start;
        dec->stream_pass_x_step = x_step;
        dec->stream_pass_y_step = y_step;
        dec->stream_rowbytes = rowbytes;
        dec->stream_row_fill = 0u;
        memset(dec->stream_prev_row, 0, rowbytes);
        return PNG_DEC_OK;
    }

    dec->stream_pass_width = 0u;
    dec->stream_pass_height = 0u;
    dec->stream_rowbytes = 0u;
    dec->stream_row_fill = 0u;
    return PNG_DEC_OK;
}

static int decoder_stream_setup(png_decoder* dec)
{
    png_u32 output_rowbytes;
    png_u32 output_size;
    png_u32 full_rowbytes;
    png_u32 scratch_rowbytes;
    png_u32 scratch_rowbytes16;
    int zr;
    int err;

    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    if (dec->stream_initialized)
        return PNG_DEC_OK;

    png_build_gamma_lut(&dec->st);

    full_rowbytes = png_rowbytes_for_width_internal(&dec->st, dec->st.width);
    dec->stream_bpp = png_filter_bpp_bytes_internal(&dec->st);
    if (full_rowbytes == 0u || dec->stream_bpp == 0u)
        return PNG_DEC_ERR_CONVERSION_LIMIT;

    err = png_output_format_rowbytes(dec->options.output_format, dec->st.width, &output_rowbytes);
    if (err != PNG_DEC_OK)
        return err;

    if (!safe_mul_u32(output_rowbytes, dec->st.height, &output_size))
        return PNG_DEC_ERR_CONVERSION_LIMIT;
    if (output_size > dec->st.max_image_bytes)
        return PNG_DEC_ERR_CONVERSION_LIMIT;

    if (!safe_mul_u32(dec->st.width, 4u, &scratch_rowbytes))
        return PNG_DEC_ERR_CONVERSION_LIMIT;
    if (!safe_mul_u32(dec->st.width, 8u, &scratch_rowbytes16))
        return PNG_DEC_ERR_CONVERSION_LIMIT;

    err = check_temp_limit3(full_rowbytes + 1u, full_rowbytes + full_rowbytes, scratch_rowbytes + scratch_rowbytes16 + output_rowbytes, dec->st.max_temp_bytes);
    if (err != PNG_DEC_OK)
        return err;

    memset(&dec->image, 0, sizeof(dec->image));
    dec->image.width = dec->st.width;
    dec->image.height = dec->st.height;
    dec->image.source_color_type = dec->st.color_type;
    dec->image.source_bit_depth = dec->st.bit_depth;
    dec->image.interlace_method = dec->st.interlace_method;
    dec->image.transform_flags_applied = dec->st.transform_flags;
    dec->image.pixels = (png_u8*)png_mem89_alloc(output_size == 0u ? 1u : output_size);
    if (!dec->image.pixels)
        return PNG_DEC_ERR_OOM;
    memset(dec->image.pixels, 0, output_size);
    png_assign_output_view(&dec->image, dec->image.pixels, dec->options.output_format, output_rowbytes);

    dec->stream_scanline = (png_u8*)png_mem89_alloc(full_rowbytes + 1u);
    dec->stream_row = (png_u8*)png_mem89_alloc(full_rowbytes == 0u ? 1u : full_rowbytes);
    dec->stream_prev_row = (png_u8*)png_mem89_alloc(full_rowbytes == 0u ? 1u : full_rowbytes);
    dec->stream_rgba_row = (png_u8*)png_mem89_alloc(scratch_rowbytes == 0u ? 1u : scratch_rowbytes);
    dec->stream_rgba16_row = (png_u8*)png_mem89_alloc(scratch_rowbytes16 == 0u ? 1u : scratch_rowbytes16);
    dec->stream_public_row = (png_u8*)png_mem89_alloc(output_rowbytes == 0u ? 1u : output_rowbytes);
    if (!dec->stream_scanline || !dec->stream_row || !dec->stream_prev_row || !dec->stream_rgba_row || !dec->stream_rgba16_row || !dec->stream_public_row) {
        decoder_stream_cleanup(dec);
        png_free_image(&dec->image);
        return PNG_DEC_ERR_OOM;
    }

    dec->stream_output_pixel_size = output_rowbytes / dec->st.width;
    dec->stream_interlaced = (dec->st.interlace_method != 0u) ? 1 : 0;
    dec->stream_row_fill = 0u;
    dec->next_row = 0u;
    dec->stream_pass = 0u;
    dec->stream_pass_width = dec->st.width;
    dec->stream_pass_height = dec->st.height;
    dec->stream_pass_row = 0u;
    dec->stream_pass_x_start = 0u;
    dec->stream_pass_y_start = 0u;
    dec->stream_pass_x_step = 1u;
    dec->stream_pass_y_step = 1u;
    dec->stream_rowbytes = full_rowbytes;
    memset(dec->stream_prev_row, 0, full_rowbytes);

    if (dec->stream_interlaced)
    {
        err = decoder_stream_advance_pass(dec);
        if (err != PNG_DEC_OK) {
            decoder_stream_cleanup(dec);
            png_free_image(&dec->image);
            return err;
        }
    }

    memset(&dec->zstrm, 0, sizeof(dec->zstrm));
    zr = inflateInit(&dec->zstrm);
    if (zr != Z_OK) {
        decoder_stream_cleanup(dec);
        png_free_image(&dec->image);
        return PNG_DEC_ERR_ZLIB;
    }

    dec->stream_initialized = 1;
    dec->zstream_ended = 0;

    decoder_emit_header_from_state(dec);
    return PNG_DEC_OK;
}

static int decoder_stream_scatter_current_row(png_decoder* dec)
{
    png_u8* src_row;
    png_u8* dst_row;
    png_u32 src_pixel_size;
    png_u32 y_index;
    png_u32 row_pixels;
    int pass_number;
    int err;

    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    row_pixels = dec->stream_interlaced ? dec->stream_pass_width : dec->st.width;
    if (row_pixels == 0u)
        return PNG_DEC_ERR_FORMAT;

    if (dec->image.output_sample_depth == 16u)
    {
        err = png_render_scanline_rgba16(&dec->st, dec->stream_row, dec->stream_rgba16_row, row_pixels);
        if (err != PNG_DEC_OK)
            return err;

        png_apply_post_transforms_rgba16(dec->stream_rgba16_row,
                                         row_pixels,
                                         dec->st.transform_flags);

        if (dec->image.output_format == PNG_OUTPUT_RGBA16)
            src_row = dec->stream_rgba16_row;
        else
        {
            err = png_convert_row_between_formats(dec->stream_rgba16_row,
                                                  PNG_OUTPUT_RGBA16,
                                                  dec->stream_public_row,
                                                  dec->image.output_format,
                                                  row_pixels);
            if (err != PNG_DEC_OK)
                return err;
            src_row = dec->stream_public_row;
        }
    }
    else
    {
        err = png_render_scanline_rgba8(&dec->st, dec->stream_row, dec->stream_rgba_row, row_pixels);
        if (err != PNG_DEC_OK)
            return err;

        png_apply_post_transforms_rgba(dec->stream_rgba_row,
                                       row_pixels,
                                       dec->st.transform_flags);

        if (dec->image.output_format == PNG_OUTPUT_RGBA8)
            src_row = dec->stream_rgba_row;
        else
        {
            err = png_convert_row_between_formats(dec->stream_rgba_row,
                                                  PNG_OUTPUT_RGBA8,
                                                  dec->stream_public_row,
                                                  dec->image.output_format,
                                                  row_pixels);
            if (err != PNG_DEC_OK)
                return err;
            src_row = dec->stream_public_row;
        }
    }

    src_pixel_size = dec->stream_output_pixel_size;

    if (!dec->stream_interlaced)
    {
        dst_row = dec->image.pixels + dec->next_row * dec->image.pixel_rowbytes;
        memcpy(dst_row, src_row, dec->image.pixel_rowbytes);
        if (dec->image.output_sample_depth == 16u &&
            (dec->st.transform_flags & PNG_DEC_TRANSFORM_SWAP_16_ENDIAN))
            png_swap_16_buffer(dst_row, dec->image.pixel_rowbytes);
        y_index = dec->next_row;
        pass_number = 0;
        dec->next_row += 1u;
    }
    else
    {
        png_u32 px;
        y_index = dec->stream_pass_y_start + dec->stream_pass_row * dec->stream_pass_y_step;
        if (y_index >= dec->st.height)
            return PNG_DEC_ERR_FORMAT;
        dst_row = dec->image.pixels + y_index * dec->image.pixel_rowbytes;
        for (px = 0u; px < row_pixels; ++px)
        {
            png_u32 x_index = dec->stream_pass_x_start + px * dec->stream_pass_x_step;
            png_u8* dst_px;
            if (x_index >= dec->st.width)
                continue;
            dst_px = dst_row + x_index * dec->stream_output_pixel_size;
            memcpy(dst_px,
                   src_row + px * src_pixel_size,
                   src_pixel_size);
            if (dec->image.output_sample_depth == 16u &&
                (dec->st.transform_flags & PNG_DEC_TRANSFORM_SWAP_16_ENDIAN))
                png_swap_16_buffer(dst_px, src_pixel_size);
        }
        pass_number = (int)dec->stream_pass + 1;
        dec->stream_pass_row += 1u;
        if (dec->stream_pass_row == dec->stream_pass_height)
        {
            dec->stream_pass += 1u;
            err = decoder_stream_advance_pass(dec);
            if (err != PNG_DEC_OK)
                return err;
        }
    }

    if (dec->callbacks.row_fn)
        dec->callbacks.row_fn(dec->callbacks.user_ptr,
                              y_index,
                              dst_row,
                              dec->image.pixel_rowbytes,
                              pass_number);

    if (dec->pause_requested)
        return PNG_DEC_PAUSED;
    err = decoder_note_row_callback(dec);
    if (err != PNG_DEC_OK)
        return err;

    return PNG_DEC_OK;
}

static int decoder_stream_process_idat(png_decoder* dec, const png_u8* data, png_u32 size, png_u32* consumed_out)
{
    int err;

    if (!dec || (!data && size != 0u))
        return PNG_DEC_ERR_FORMAT;

    if (consumed_out)
        *consumed_out = 0u;

    if (!dec->stream_initialized)
        return PNG_DEC_OK;

    if (dec->zstream_ended)
        return PNG_DEC_OK;

    dec->zstrm.next_in = (Bytef*)data;
    dec->zstrm.avail_in = (uInt)size;

    while (dec->zstrm.avail_in != 0u && !dec->zstream_ended)
    {
        int zr;
        png_u32 need;
        png_u32 produced;

        if (dec->stream_interlaced && dec->stream_pass >= 7u)
        {
            png_u8 dummy = 0u;
            dec->zstrm.next_out = &dummy;
            dec->zstrm.avail_out = 1u;
            zr = inflate(&dec->zstrm, Z_NO_FLUSH);
            produced = 1u - (png_u32)dec->zstrm.avail_out;
            if (zr != Z_OK && zr != Z_STREAM_END && zr != Z_BUF_ERROR)
                return PNG_DEC_ERR_ZLIB;
            err = decoder_note_zlib_budget(dec, produced);
            if (err != PNG_DEC_OK)
                return err;
            if (produced != 0u)
                return PNG_DEC_ERR_FORMAT;
            if (zr == Z_STREAM_END) {
                dec->zstream_ended = 1;
                break;
            }
            if (produced == 0u && dec->zstrm.avail_in == 0u)
                break;
            if (zr == Z_BUF_ERROR)
                break;
            continue;
        }

        need = dec->stream_rowbytes + 1u - dec->stream_row_fill;
        dec->zstrm.next_out = dec->stream_scanline + dec->stream_row_fill;
        dec->zstrm.avail_out = (uInt)need;

        {
            uInt prev_avail_in = dec->zstrm.avail_in;
            zr = inflate(&dec->zstrm, Z_NO_FLUSH);
            produced = need - (png_u32)dec->zstrm.avail_out;
            dec->stream_row_fill += produced;
            err = decoder_note_parse_bytes(dec, (png_u32)(prev_avail_in - dec->zstrm.avail_in));
            if (err != PNG_DEC_OK)
            {
                if (consumed_out)
                    *consumed_out = size - (png_u32)dec->zstrm.avail_in;
                return err;
            }
            err = decoder_note_zlib_budget(dec, produced);
            if (err != PNG_DEC_OK)
            {
                if (consumed_out)
                    *consumed_out = size - (png_u32)dec->zstrm.avail_in;
                return err;
            }
        }

        if (zr != Z_OK && zr != Z_STREAM_END && zr != Z_BUF_ERROR)
            return PNG_DEC_ERR_ZLIB;

        if (dec->stream_row_fill == dec->stream_rowbytes + 1u)
        {
            int filter_type;
            int err;
            png_u32 completed_rowbytes;
            png_u32 completed_pass;

            if (!dec->stream_interlaced && dec->next_row >= dec->st.height)
                return PNG_DEC_ERR_FORMAT;

            filter_type = (int)dec->stream_scanline[0];
            if (filter_type < 0 || filter_type > 4)
                return PNG_DEC_ERR_FORMAT;

            memcpy(dec->stream_row, dec->stream_scanline + 1u, dec->stream_rowbytes);
            png_unfilter_scanline(dec->stream_row,
                                  dec->stream_prev_row,
                                  dec->stream_rowbytes,
                                  filter_type,
                                  dec->stream_bpp);

            completed_rowbytes = dec->stream_rowbytes;
            completed_pass = dec->stream_pass;
            err = decoder_stream_scatter_current_row(dec);

            /*
             * Preserve the "previous row" only when the next scanline stays inside
             * the same reduced image/pass. When Adam7 advances to a new pass the
             * predictor must restart from an all-zero previous row, just like the
             * non-streaming decoder does for each pass.
             */
            if (!dec->stream_interlaced || dec->stream_pass == completed_pass)
                memcpy(dec->stream_prev_row, dec->stream_row, completed_rowbytes);
            dec->stream_row_fill = 0u;
            if (err != PNG_DEC_OK)
            {
                if (consumed_out)
                    *consumed_out = size - (png_u32)dec->zstrm.avail_in;
                return err;
            }
        }

        if (zr == Z_STREAM_END) {
            dec->zstream_ended = 1;
            break;
        }

        if (produced == 0u && dec->zstrm.avail_in == 0u)
            break;

        if (zr == Z_BUF_ERROR && produced == 0u)
            break;
    }

    if (consumed_out)
        *consumed_out = size - (png_u32)dec->zstrm.avail_in;

    return PNG_DEC_OK;
}

static int decoder_stream_finish_no_input(png_decoder* dec)
{
    int err;

    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    while (!dec->zstream_ended)
    {
        int zr;
        png_u8 dummy = 0u;
        dec->zstrm.next_in = Z_NULL;
        dec->zstrm.avail_in = 0u;
        dec->zstrm.next_out = &dummy;
        dec->zstrm.avail_out = 1u;
        {
            png_u32 produced;
            zr = inflate(&dec->zstrm, Z_NO_FLUSH);
            produced = 1u - (png_u32)dec->zstrm.avail_out;
            err = decoder_note_zlib_budget(dec, produced);
            if (err != PNG_DEC_OK)
                return err;
        }
        if (zr == Z_STREAM_END)
        {
            dec->zstream_ended = 1;
            return PNG_DEC_OK;
        }
        if (zr == Z_BUF_ERROR)
            return PNG_DEC_ERR_FORMAT;
        if (zr != Z_OK)
            return PNG_DEC_ERR_ZLIB;
        if (dec->zstrm.avail_out == 0u)
            return PNG_DEC_ERR_FORMAT;
    }

    return PNG_DEC_OK;
}

static int decoder_finalize_stream_image(png_decoder* dec)
{
    if (!dec || !dec->stream_initialized)
        return PNG_DEC_ERR_FORMAT;

    if (!dec->zstream_ended)
        return PNG_DEC_ERR_FORMAT;

    if (dec->stream_row_fill != 0u)
        return PNG_DEC_ERR_FORMAT;

    if (!dec->stream_interlaced)
    {
        if (dec->next_row != dec->st.height)
            return PNG_DEC_ERR_FORMAT;
    }
    else if (dec->stream_pass < 7u)
        return PNG_DEC_ERR_FORMAT;

    png_transfer_metadata_from_state(&dec->image, &dec->st);
    dec->has_image = 1;

    if (dec->callbacks.end_fn)
        dec->callbacks.end_fn(dec->callbacks.user_ptr, &dec->image);

    decoder_stream_cleanup(dec);
    return PNG_DEC_DONE;
}
#endif

static int decoder_finish_full_decode(png_decoder* dec, png_u32 end_pos)
{
    int err;
    png_image img;
    png_u32 y;
    png_decode_options opt;

    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    memset(&img, 0, sizeof(img));

    if (dec->options_set)
        opt = dec->options;
    else
        png_decode_options_init(&opt);

    err = png_decode_memory_ex(dec->buffer, end_pos, dec->zfunc, &opt, &img);
    if (err != PNG_DEC_OK)
        return err;

    decoder_emit_header_from_image(dec, &img);

    if (dec->callbacks.row_fn)
    {
        for (y = 0u; y < img.height; ++y)
        {
            dec->callbacks.row_fn(dec->callbacks.user_ptr,
                                  y,
                                  img.pixels + y * img.pixel_rowbytes,
                                  img.pixel_rowbytes,
                                  0);
        }
    }

    if (dec->callbacks.end_fn)
        dec->callbacks.end_fn(dec->callbacks.user_ptr, &img);

    dec->image = img;
    dec->has_image = 1;
    return PNG_DEC_DONE;
}

static int decoder_parse_available(png_decoder* dec)
{
    if (!dec)
        return PNG_DEC_ERR_FORMAT;

    if (dec->has_image || dec->parse_done)
        return PNG_DEC_DONE;

    if (dec->paused)
        return PNG_DEC_PAUSED;

    if (dec->st.seen_IEND && !dec->parse_done && dec->parse_pos == dec->size)
    {
        int err;

#ifdef PNG_DEC_USE_ZLIB
        if (dec->stream_initialized)
            err = decoder_finalize_stream_image(dec);
        else
#endif
            err = decoder_finish_full_decode(dec, dec->parse_pos);

        if (err != PNG_DEC_DONE && err != PNG_DEC_OK)
            return err;
        dec->parse_done = 1;
        return PNG_DEC_DONE;
    }

    if (!dec->sig_checked)
    {
        if (dec->size < 8u)
            return PNG_DEC_OK;
        if (memcmp(dec->buffer, PNG_SIG_LOCAL, 8u) != 0)
            return PNG_DEC_ERR_SIG;
        dec->sig_checked = 1;
        dec->parse_pos = 8u;
    }

    while (1)
    {
        png_u32 chunk_pos;
        png_u32 length;
        png_u32 type;
        png_u32 next_pos;
        png_u32 crc_calc;
        png_u32 crc_read;
        const png_u8* chunk_data;
        int err;

#ifdef PNG_DEC_USE_ZLIB
        if (dec->active_chunk_valid)
        {
            err = decoder_resume_active_chunk(dec);
            if (err != PNG_DEC_OK)
                return err;
        }
#endif

        if (dec->prog_ctl.max_parse_bytes_per_call != 0u &&
            dec->work_parse_bytes >= dec->prog_ctl.max_parse_bytes_per_call)
            return decoder_budget_hit_result(dec);

        if (dec->parse_pos + 8u > dec->size)
            break;

        chunk_pos = dec->parse_pos;
        length = read_be32_local(dec->buffer + dec->parse_pos);
        type = read_be32_local(dec->buffer + dec->parse_pos + 4u);

        if (length > 0x7FFFFFFFu)
            return PNG_DEC_ERR_FORMAT;

        if (length > dec->st.max_chunk_bytes)
            return PNG_DEC_ERR_CHUNK_TOO_LARGE;

        if (dec->st.chunk_count >= dec->st.max_chunks)
            return PNG_DEC_ERR_TOO_MANY_CHUNKS;

        next_pos = dec->parse_pos + 8u + length + 4u;
        if (next_pos > dec->size)
            break;

        chunk_data = dec->buffer + dec->parse_pos + 8u;
        crc_calc = png_crc32(dec->buffer + dec->parse_pos + 4u, 4u + length);
        crc_read = read_be32_local(dec->buffer + dec->parse_pos + 8u + length);
        if (crc_calc != crc_read)
            return PNG_DEC_ERR_CRC;

        dec->st.chunk_count += 1u;
        err = png_handle_chunk(&dec->st, type, chunk_data, length);
        if (err != PNG_DEC_OK)
            return err;

        if (type == ((png_u32)'I'<<24 | (png_u32)'D'<<16 | (png_u32)'A'<<8 | (png_u32)'T'))
        {
#ifdef PNG_DEC_USE_ZLIB
            if (!dec->stream_initialized)
            {
                err = decoder_stream_setup(dec);
                if (err != PNG_DEC_OK)
                    return err;
            }

            dec->active_chunk_valid = 1;
            dec->active_chunk_type = type;
            dec->active_chunk_start = dec->parse_pos;
            dec->active_chunk_length = length;
            dec->active_chunk_data_offset = dec->parse_pos + 8u;
            dec->active_chunk_next_pos = next_pos;
            dec->active_chunk_data_consumed = 0u;

            if (dec->pause_requested)
                return decoder_pause_here(dec, dec->active_chunk_data_offset);

            err = decoder_resume_active_chunk(dec);
            if (err != PNG_DEC_OK)
                return err;
#else
            decoder_emit_header_from_state(dec);
            dec->parse_pos = next_pos;
            if (dec->pause_requested)
                return decoder_pause_here(dec, dec->parse_pos);
#endif

            err = decoder_emit_chunk_callback(dec, type, length, chunk_pos);
            if (err == PNG_DEC_PAUSED)
                return decoder_pause_here(dec, dec->parse_pos);
            if (err != PNG_DEC_OK)
                return err;
        }
        else
        {
            png_u32 consumed_bytes = next_pos - dec->parse_pos;
            dec->parse_pos = next_pos;
            if (dec->pause_requested)
                return decoder_pause_here(dec, dec->parse_pos);

            err = decoder_emit_chunk_callback(dec, type, length, chunk_pos);
            if (err == PNG_DEC_PAUSED)
                return decoder_pause_here(dec, dec->parse_pos);
            if (err != PNG_DEC_OK)
                return err;

            err = decoder_note_parse_bytes(dec, consumed_bytes);
            if (err != PNG_DEC_OK)
                return err;

            if (type == ((png_u32)'I'<<24 | (png_u32)'E'<<16 | (png_u32)'N'<<8 | (png_u32)'D'))
            {
                if (dec->options.strict_trailing_data && dec->size != dec->parse_pos)
                    return PNG_DEC_ERR_FORMAT;

#ifdef PNG_DEC_USE_ZLIB
                if (dec->stream_initialized)
                    err = decoder_finalize_stream_image(dec);
                else
#endif
                    err = decoder_finish_full_decode(dec, dec->parse_pos);

                if (err != PNG_DEC_DONE && err != PNG_DEC_OK)
                    return err;

                dec->parse_done = 1;
                return PNG_DEC_DONE;
            }
        }
    }

    return PNG_DEC_OK;
}

int png_decoder_feed_ex(png_decoder* dec, const png_u8* data, png_u32 size, png_u32* consumed_out)
{
    int err;
    png_u32 old_size;
    png_u32 accept = size;
    png_u32 room;

    if (consumed_out)
        *consumed_out = 0u;

    if (!dec || (!data && size != 0u))
        return PNG_DEC_ERR_FORMAT;

    if (dec->has_image)
    {
        if (size != 0u && dec->options.strict_trailing_data)
            return PNG_DEC_ERR_FORMAT;
        return PNG_DEC_DONE;
    }

    if (dec->paused && !dec->pause_save && size == 0u)
        return PNG_DEC_PAUSED;

    dec->paused = 0;
    decoder_work_reset(dec);
    decoder_maybe_compact_buffer(dec);

    room = png_decoder_input_room(dec);
    if (room == 0u && size != 0u && png_decoder_unprocessed_bytes(dec) != 0u)
    {
        dec->feed_active = 1;
        dec->feed_start_offset = dec->size;
        dec->feed_end_offset = dec->size;
        err = decoder_parse_available(dec);
        dec->feed_active = 0;
        decoder_maybe_compact_buffer(dec);
        if (err != PNG_DEC_OK)
            return err;
        room = png_decoder_input_room(dec);
    }

    old_size = dec->size;

    if (dec->prog_ctl.max_feed_bytes != 0u && accept > dec->prog_ctl.max_feed_bytes)
        accept = dec->prog_ctl.max_feed_bytes;
    if (room != 0xFFFFFFFFu && accept > room)
        accept = room;

    if (accept != 0u)
    {
        err = decoder_reserve(dec, accept);
        if (err != PNG_DEC_OK)
            return err;

        memcpy(dec->buffer + dec->size, data, accept);
        dec->size += accept;
    }

    if (consumed_out)
        *consumed_out = accept;

    dec->feed_active = 1;
    dec->feed_start_offset = old_size;
    dec->feed_end_offset = old_size + accept;
    err = decoder_parse_available(dec);
    dec->feed_active = 0;
    decoder_maybe_compact_buffer(dec);

    if (err == PNG_DEC_OK && accept < size)
        return PNG_DEC_YIELDED;
    return err;
}

int png_decoder_feed(png_decoder* dec, const png_u8* data, png_u32 size)
{
    return png_decoder_feed_ex(dec, data, size, (png_u32*)0);
}

png_u32 png_decoder_process_data_pause(png_decoder* dec, int save)
{
    png_u32 unread_abs;

    if (!dec)
        return 0u;

    dec->pause_requested = 1;
    dec->pause_save = save ? 1 : 0;

#ifdef PNG_DEC_USE_ZLIB
    if (dec->active_chunk_valid)
        unread_abs = dec->active_chunk_data_offset + dec->active_chunk_data_consumed;
    else
#endif
        unread_abs = dec->parse_pos;

    return decoder_compute_unprocessed_bytes(dec, unread_abs);
}

png_u32 png_decoder_process_data_skip(png_decoder* dec)
{
    png_u32 count;

    if (!dec || !dec->paused || !dec->pause_save)
        return 0u;

    count = decoder_paused_cached_bytes(dec);
    dec->pause_save = 0;
    dec->size = dec->pause_resume_offset;
    if (dec->parse_pos > dec->size)
        dec->parse_pos = dec->size;
    dec->pause_unprocessed_bytes = count;
    return count;
}

png_u32 png_decoder_pending_bytes(const png_decoder* dec)
{
    if (!dec)
        return 0u;
    if (dec->paused)
        return decoder_paused_cached_bytes(dec);
    return 0u;
}

int png_decoder_is_paused(const png_decoder* dec)
{
    return dec ? dec->paused : 0;
}

int png_decoder_take_image(png_decoder* dec, png_image* out_image)
{
    if (!dec || !out_image)
        return PNG_DEC_ERR_FORMAT;

    if (!dec->has_image)
        return PNG_DEC_DONE;

    *out_image = dec->image;
    memset(&dec->image, 0, sizeof(dec->image));
    dec->has_image = 0;
    return PNG_DEC_OK;
}

const char* png_strerror(int err)
{
    switch (err)
    {
        case PNG_DEC_OK: return "ok";
        case PNG_DEC_DONE: return "done";
        case PNG_DEC_PAUSED: return "paused";
        case PNG_DEC_YIELDED: return "yielded";
        case PNG_DEC_ERR_FORMAT: return "format error";
        case PNG_DEC_ERR_OOM: return "out of memory";
        case PNG_DEC_ERR_ZLIB: return "zlib error";
        case PNG_DEC_ERR_CRC: return "crc error";
        case PNG_DEC_ERR_SIG: return "bad png signature";
        case PNG_DEC_ERR_UNSUPPORTED: return "unsupported feature or value";
        case PNG_DEC_ERR_IO: return "i/o error";
        case PNG_DEC_ERR_DIMENSIONS_TOO_LARGE: return "dimensions exceed configured limit";
        case PNG_DEC_ERR_TOO_MANY_PIXELS: return "pixel count exceeds configured limit";
        case PNG_DEC_ERR_INFLATED_TOO_LARGE: return "inflated image data exceeds configured limit";
        case PNG_DEC_ERR_CHUNK_TOO_LARGE: return "chunk or input size exceeds configured limit";
        case PNG_DEC_ERR_TOO_MANY_CHUNKS: return "too many chunks";
        case PNG_DEC_ERR_TEXT_TOO_LARGE: return "text metadata exceeds configured limit";
        case PNG_DEC_ERR_FRAME_LIMIT: return "apng frame count exceeds configured limit";
        case PNG_DEC_ERR_TEMP_MEMORY_LIMIT: return "temporary memory exceeds configured limit";
        case PNG_DEC_ERR_WORK_BUDGET: return "work budget exhausted";
        case PNG_DEC_ERR_CONVERSION_LIMIT: return "pixel conversion exceeds configured limit";
        default: return "unknown png decoder error";
    }
}

void png_decoder_free(png_decoder* dec)
{
    if (!dec)
        return;

    if (dec->buffer)
        png_mem89_release(dec->buffer);

#ifdef PNG_DEC_USE_ZLIB
    decoder_stream_cleanup(dec);
#endif

    png_state_free(&dec->st);
    png_free_image(&dec->image);
    png_mem89_release(dec);
}
