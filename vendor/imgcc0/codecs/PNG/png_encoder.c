#include "png_mem89.h"
#include <string.h>

#include "png_decoder_internal.h"

static void write_be16(png_u8* p, png_u16 v)
{
    p[0] = (png_u8)((v >> 8) & 0xFFu);
    p[1] = (png_u8)(v & 0xFFu);
}

static void write_be32(png_u8* p, png_u32 v)
{
    p[0] = (png_u8)((v >> 24) & 0xFFu);
    p[1] = (png_u8)((v >> 16) & 0xFFu);
    p[2] = (png_u8)((v >>  8) & 0xFFu);
    p[3] = (png_u8)( v        & 0xFFu);
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

static int safe_add_u32(png_u32 a, png_u32 b, png_u32* out)
{
    png_u32 maxv;

    if (!out)
        return 0;

    maxv = (png_u32)~(png_u32)0;
    if (a > maxv - b)
        return 0;

    *out = a + b;
    return 1;
}

static int deflate_bound_u32(png_u32 src_len, png_u32* out_bound)
{
    png_u32 bound;

    if (!out_bound)
        return 0;

    bound = src_len;
    if (!safe_add_u32(bound, (src_len >> 12), &bound)) return 0;
    if (!safe_add_u32(bound, (src_len >> 14), &bound)) return 0;
    if (!safe_add_u32(bound, (src_len >> 25), &bound)) return 0;
    if (!safe_add_u32(bound, 13u, &bound)) return 0;

    *out_bound = bound;
    return 1;
}

static png_u16 png_scale_16_to_sample(png_u16 v16, png_u8 bit_depth);
static int png_i32_to_u32_checked(png_i32 v, png_u32* out);

static int png_fixed_to_scaled_u16(png_fixed89 value, png_u32 scale, png_u16* out)
{
    png_u32 whole;
    png_u32 frac;
    png_u32 v;
    if (!out || value < 0 || scale == 0u)
        return 0;
    whole = (png_u32)value >> 16;
    frac = (png_u32)value & 65535u;
    if (whole > 65535u / scale)
        return 0;
    v = whole * scale;
    if (frac != 0u)
    {
        png_u32 add;
        if (frac > (0xFFFFFFFFu - 32768u) / scale)
            return 0;
        add = (frac * scale + 32768u) >> 16;
        if (v > 65535u - add)
            return 0;
        v += add;
    }
    if (v > 65535u)
        return 0;
    *out = (png_u16)v;
    return 1;
}

static int png_fixed_to_scaled_u32(png_fixed89 value, png_u32 scale, png_u32* out)
{
    png_u32 whole;
    png_u32 frac;
    png_u32 v;
    png_u32 add;
    if (!out || value < 0 || scale == 0u)
        return 0;
    whole = (png_u32)value >> 16;
    frac = (png_u32)value & 65535u;
    if (whole != 0u && scale > 0xFFFFFFFFu / whole)
        return 0;
    v = whole * scale;
    if (frac > (0xFFFFFFFFu - 32768u) / scale)
        return 0;
    add = (frac * scale + 32768u) >> 16;
    if (v > 0xFFFFFFFFu - add)
        return 0;
    *out = v + add;
    return 1;
}


typedef struct png_write_buffer_s {
    png_u8* data;
    png_u32 size;
    png_u32 capacity;
} png_write_buffer;



static int wb_reserve(png_write_buffer* wb, png_u32 extra)
{
    png_u32 needed;
    png_u32 newcap;
    png_u8* nb;

    if (!wb)
        return PNG_DEC_ERR_FORMAT;

    if (!safe_add_u32(wb->size, extra, &needed))
        return PNG_DEC_ERR_UNSUPPORTED;

    if (needed <= wb->capacity)
        return PNG_DEC_OK;

    newcap = (wb->capacity == 0u) ? 4096u : wb->capacity;
    while (newcap < needed) {
        if (newcap > PNG_DEC_MAX_FILE_BYTES / 2u)
            return PNG_DEC_ERR_UNSUPPORTED;
        newcap *= 2u;
    }

    nb = (png_u8*)png_mem89_resize(wb->data, newcap);
    if (!nb)
        return PNG_DEC_ERR_OOM;

    wb->data = nb;
    wb->capacity = newcap;
    return PNG_DEC_OK;
}

static int wb_append(png_write_buffer* wb, const png_u8* data, png_u32 len)
{
    int err;

    if (!wb)
        return PNG_DEC_ERR_FORMAT;

    err = wb_reserve(wb, len);
    if (err != PNG_DEC_OK)
        return err;

    if (len != 0u && data)
        memcpy(wb->data + wb->size, data, len);
    wb->size += len;
    return PNG_DEC_OK;
}

static int write_chunk_buffer(png_write_buffer* wb,
                              const png_u8 type_bytes[4],
                              const png_u8* data,
                              png_u32 length)
{
    png_u8 hdr[8];
    png_u8 crc_buf[4];
    png_u32 crc;
    int err;

    if (!wb || !type_bytes)
        return PNG_DEC_ERR_FORMAT;

    write_be32(hdr, length);
    memcpy(hdr + 4, type_bytes, 4u);

    err = wb_append(wb, hdr, 8u);
    if (err != PNG_DEC_OK)
        return err;

    if (length != 0u)
    {
        if (!data)
            return PNG_DEC_ERR_FORMAT;
        err = wb_append(wb, data, length);
        if (err != PNG_DEC_OK)
            return err;
    }

    crc = png_crc32_start();
    crc = png_crc32_update(crc, type_bytes, 4u);
    crc = png_crc32_update(crc, data, length);
    crc = png_crc32_finish(crc);
    write_be32(crc_buf, crc);

    return wb_append(wb, crc_buf, 4u);
}

static int png_append_dsig_chunks(png_write_buffer* wb,
                                  const png_encode_options* opt,
                                  png_u8 location)
{
    png_u32 i;
    int err;

    if (!wb || !opt)
        return PNG_DEC_ERR_FORMAT;

    for (i = 0u; i < opt->dsig_chunk_count; ++i)
    {
        const png_dsig_entry* ds;
        if (!opt->dsig_chunks)
            break;
        ds = &opt->dsig_chunks[i];
        if (ds->location != location)
            continue;
        if (!ds->cms_data || ds->size == 0u)
            return PNG_DEC_ERR_FORMAT;
        err = write_chunk_buffer(wb, (const png_u8*)"dSIG", ds->cms_data, ds->size);
        if (err != PNG_DEC_OK)
            return err;
    }

    return PNG_DEC_OK;
}

static int png_validate_printable_ascii_bytes_enc(const png_u8* s, png_u32 len)
{
    png_u32 i;

    if (!s)
        return 0;

    for (i = 0u; i < len; ++i)
    {
        if (!(s[i] >= 0x20u && s[i] <= 0x7Eu))
            return 0;
    }

    return 1;
}

static int png_is_valid_generic_location(png_u8 location)
{
    return (location == PNG_CHUNK_POS_AFTER_IHDR ||
            location == PNG_CHUNK_POS_AFTER_PLTE ||
            location == PNG_CHUNK_POS_AFTER_IDAT) ? 1 : 0;
}

static int png_is_valid_iccp_name(const char* name)
{
    const unsigned char* p;
    unsigned int len;
    unsigned char prev_space;

    if (!name || !name[0])
        return 0;

    p = (const unsigned char*)name;
    len = 0u;
    prev_space = 0u;

    while (*p)
    {
        unsigned char c;
        c = *p++;
        ++len;

        if (len > 79u)
            return 0;

        if (!((c >= 0x20u && c <= 0x7Eu) || (c >= 0xA1u)))
            return 0;

        if (c == 0x20u)
        {
            if (len == 1u || prev_space)
                return 0;
            prev_space = 1u;
        }
        else
        {
            prev_space = 0u;
        }
    }

    if (len == 0u || prev_space)
        return 0;

    return 1;
}

static png_u32 png_channels_for_color_type_enc(png_u8 color_type)
{
    switch (color_type)
    {
        case PNG_COLOR_GRAYSCALE: return 1u;
        case PNG_COLOR_TRUECOLOR: return 3u;
        case PNG_COLOR_INDEXED: return 1u;
        case PNG_COLOR_GRAYSCALE_ALPHA: return 2u;
        case PNG_COLOR_TRUECOLOR_ALPHA: return 4u;
        default: return 0u;
    }
}

static int png_validate_color_depth_combo(png_u8 color_type, png_u8 bit_depth)
{
    switch (color_type)
    {
        case PNG_COLOR_GRAYSCALE:
            return (bit_depth == 1u || bit_depth == 2u || bit_depth == 4u ||
                    bit_depth == 8u || bit_depth == 16u) ? 1 : 0;
        case PNG_COLOR_TRUECOLOR:
            return (bit_depth == 8u || bit_depth == 16u) ? 1 : 0;
        case PNG_COLOR_INDEXED:
            return (bit_depth == 1u || bit_depth == 2u || bit_depth == 4u ||
                    bit_depth == 8u) ? 1 : 0;
        case PNG_COLOR_GRAYSCALE_ALPHA:
            return (bit_depth == 8u || bit_depth == 16u) ? 1 : 0;
        case PNG_COLOR_TRUECOLOR_ALPHA:
            return (bit_depth == 8u || bit_depth == 16u) ? 1 : 0;
        default:
            return 0;
    }
}

static int png_compute_rowbytes(png_u32 width,
                                png_u8 color_type,
                                png_u8 bit_depth,
                                png_u32* out_rowbytes)
{
    png_u32 channels;
    png_u32 rowbits;

    if (!out_rowbytes)
        return 0;

    channels = png_channels_for_color_type_enc(color_type);
    if (channels == 0u)
        return 0;

    if (!safe_mul_u32(width, channels * (png_u32)bit_depth, &rowbits))
        return 0;

    *out_rowbytes = (rowbits + 7u) / 8u;
    return 1;
}

static png_u32 png_filter_bpp_bytes_enc(png_u8 color_type, png_u8 bit_depth)
{
    png_u32 channels;
    png_u32 bits_per_pixel;

    channels = png_channels_for_color_type_enc(color_type);
    if (channels == 0u)
        return 0u;

    bits_per_pixel = channels * (png_u32)bit_depth;
    return (bits_per_pixel + 7u) / 8u;
}

static int png_compute_source_stride(const png_encode_options* opt,
                                     png_u32 width,
                                     png_u8 color_type,
                                     png_u8 bit_depth,
                                     png_u32 rowbytes,
                                     png_u32* out_stride)
{
    if (!opt || !out_stride)
        return 0;

    if (opt->stride_bytes != 0u) {
        *out_stride = opt->stride_bytes;
        return 1;
    }

    if (opt->input_format != PNG_ENC_INPUT_FORMAT_NATIVE)
        return png_output_format_rowbytes(opt->input_format, width, out_stride) == PNG_DEC_OK;

    if ((color_type == PNG_COLOR_GRAYSCALE || color_type == PNG_COLOR_INDEXED) && bit_depth < 8u)
    {
        if (opt->input_is_packed)
            *out_stride = rowbytes;
        else
            *out_stride = width;
        return 1;
    }

    *out_stride = rowbytes;
    return 1;
}

static png_u8 png_read_packed_sample(const png_u8* src_row,
                                      png_u32 x,
                                      png_u8 bit_depth)
{
    png_u32 per_byte;
    png_u32 byte_index;
    png_u32 pos_in_byte;
    png_u8 shift;
    png_u8 mask;

    per_byte = 8u / (png_u32)bit_depth;
    byte_index = x / per_byte;
    pos_in_byte = x % per_byte;
    shift = (png_u8)(8u - bit_depth * (pos_in_byte + 1u));
    mask = (png_u8)((1u << bit_depth) - 1u);
    return (png_u8)((src_row[byte_index] >> shift) & mask);
}

static void png_write_packed_sample(png_u8* dst_row,
                                    png_u32 x,
                                    png_u8 bit_depth,
                                    png_u8 value)
{
    png_u32 per_byte;
    png_u32 byte_index;
    png_u32 pos_in_byte;
    png_u8 shift;

    per_byte = 8u / (png_u32)bit_depth;
    byte_index = x / per_byte;
    pos_in_byte = x % per_byte;
    shift = (png_u8)(8u - bit_depth * (pos_in_byte + 1u));
    dst_row[byte_index] = (png_u8)(dst_row[byte_index] | (png_u8)(value << shift));
}

static png_u16 png_read_u16_input(const png_u8* p, int little_endian)
{
    if (little_endian)
        return (png_u16)(((png_u16)p[1] << 8) | (png_u16)p[0]);
    return (png_u16)(((png_u16)p[0] << 8) | (png_u16)p[1]);
}

static png_u8 png_scale_16_to_8_local(png_u16 v16)
{
    return (png_u8)(((png_u32)v16 * 255u + 32767u) / 65535u);
}

static png_u16 png_gray_from_rgb16_enc(png_u16 r, png_u16 g, png_u16 b)
{
    png_u32 y;
    y = (png_u32)r * 2126u + (png_u32)g * 7152u + (png_u32)b * 722u + 5000u;
    return (png_u16)(y / 10000u);
}

static int png_public_format_is_gray_local(png_u8 input_format)
{
    return (input_format == PNG_OUTPUT_G8 || input_format == PNG_OUTPUT_GA8 ||
            input_format == PNG_OUTPUT_AG8 || input_format == PNG_OUTPUT_G16 ||
            input_format == PNG_OUTPUT_GA16 || input_format == PNG_OUTPUT_AG16) ? 1 : 0;
}

static int png_public_format_valid_local(png_u8 input_format)
{
    return png_output_format_channels(input_format) != 0u;
}

static void png_read_public_pixel_rgba16(const png_u8* src_row,
                                         png_u32 x,
                                         png_u8 input_format,
                                         png_u8 little_endian_16,
                                         png_u16* out_r,
                                         png_u16* out_g,
                                         png_u16* out_b,
                                         png_u16* out_a)
{
    png_u8 channels;
    png_u8 bpc;
    const png_u8* p;
    png_u16 c0, c1, c2, c3;

    channels = png_output_format_channels(input_format);
    bpc = png_output_format_bytes_per_channel(input_format);
    p = src_row + x * (png_u32)channels * (png_u32)bpc;
    c0 = c1 = c2 = 0u;
    c3 = 65535u;

    if (bpc == 1u)
    {
        if (channels > 0u) c0 = (png_u16)((png_u16)p[0] * 257u);
        if (channels > 1u) c1 = (png_u16)((png_u16)p[1] * 257u);
        if (channels > 2u) c2 = (png_u16)((png_u16)p[2] * 257u);
        if (channels > 3u) c3 = (png_u16)((png_u16)p[3] * 257u);
    }
    else
    {
        if (channels > 0u) c0 = png_read_u16_input(p + 0u, little_endian_16);
        if (channels > 1u) c1 = png_read_u16_input(p + 2u, little_endian_16);
        if (channels > 2u) c2 = png_read_u16_input(p + 4u, little_endian_16);
        if (channels > 3u) c3 = png_read_u16_input(p + 6u, little_endian_16);
    }

    switch (input_format)
    {
        case PNG_OUTPUT_RGBA8:
        case PNG_OUTPUT_RGBA16:
            *out_r = c0; *out_g = c1; *out_b = c2; *out_a = c3; break;
        case PNG_OUTPUT_BGRA8:
        case PNG_OUTPUT_BGRA16:
            *out_r = c2; *out_g = c1; *out_b = c0; *out_a = c3; break;
        case PNG_OUTPUT_ARGB8:
        case PNG_OUTPUT_ARGB16:
            *out_r = c1; *out_g = c2; *out_b = c3; *out_a = c0; break;
        case PNG_OUTPUT_RGB8:
        case PNG_OUTPUT_RGB16:
            *out_r = c0; *out_g = c1; *out_b = c2; *out_a = 65535u; break;
        case PNG_OUTPUT_BGR8:
        case PNG_OUTPUT_BGR16:
            *out_r = c2; *out_g = c1; *out_b = c0; *out_a = 65535u; break;
        case PNG_OUTPUT_GA8:
        case PNG_OUTPUT_GA16:
            *out_r = c0; *out_g = c0; *out_b = c0; *out_a = c1; break;
        case PNG_OUTPUT_AG8:
        case PNG_OUTPUT_AG16:
            *out_r = c1; *out_g = c1; *out_b = c1; *out_a = c0; break;
        case PNG_OUTPUT_G8:
        case PNG_OUTPUT_G16:
            *out_r = c0; *out_g = c0; *out_b = c0; *out_a = 65535u; break;
        default:
            *out_r = *out_g = *out_b = 0u;
            *out_a = 65535u;
            break;
    }
}

static void png_apply_input_transforms_rgba16(png_u16* r,
                                              png_u16* g,
                                              png_u16* b,
                                              png_u16* a,
                                              png_u32 flags)
{
    if (!r || !g || !b || !a)
        return;

    if (flags & PNG_ENC_TRANSFORM_INVERT_ALPHA)
        *a = (png_u16)(65535u - *a);

    if (flags & PNG_ENC_TRANSFORM_UNPREMULTIPLY_ALPHA)
    {
        if (*a == 0u)
        {
            *r = *g = *b = 0u;
        }
        else if (*a < 65535u)
        {
            png_u32 rv = ((png_u32)(*r) * 65535u + (*a / 2u)) / (png_u32)(*a);
            png_u32 gv = ((png_u32)(*g) * 65535u + (*a / 2u)) / (png_u32)(*a);
            png_u32 bv = ((png_u32)(*b) * 65535u + (*a / 2u)) / (png_u32)(*a);
            *r = (png_u16)(rv > 65535u ? 65535u : rv);
            *g = (png_u16)(gv > 65535u ? 65535u : gv);
            *b = (png_u16)(bv > 65535u ? 65535u : bv);
        }
    }
}

static int png_write_target_pixel_from_public(const png_u8* src_row,
                                              png_u32 src_x,
                                              const png_encode_options* opt,
                                              png_u8* dst_row,
                                              png_u32 dst_x)
{
    png_u16 r, g, b, a;
    png_u16 gray;
    png_u8 color_type;
    png_u8 bit_depth;
    png_u32 off;
    png_u8 sample8;

    if (!src_row || !opt || !dst_row)
        return PNG_DEC_ERR_FORMAT;

    color_type = opt->color_type;
    bit_depth = opt->bit_depth;
    png_read_public_pixel_rgba16(src_row, src_x, opt->input_format,
                                 opt->input_16bit_little_endian ? 1u : 0u,
                                 &r, &g, &b, &a);
    png_apply_input_transforms_rgba16(&r, &g, &b, &a, opt->input_transform_flags);
    gray = png_gray_from_rgb16_enc(r, g, b);

    switch (color_type)
    {
        case PNG_COLOR_GRAYSCALE:
            if (bit_depth < 8u)
            {
                png_write_packed_sample(dst_row, dst_x, bit_depth,
                                        (png_u8)png_scale_16_to_sample(gray, bit_depth));
            }
            else if (bit_depth == 8u)
            {
                dst_row[dst_x] = png_scale_16_to_8_local(gray);
            }
            else
            {
                write_be16(dst_row + dst_x * 2u, gray);
            }
            return PNG_DEC_OK;

        case PNG_COLOR_INDEXED:
            if (!png_public_format_is_gray_local(opt->input_format))
                return PNG_DEC_ERR_UNSUPPORTED;
            if (bit_depth < 8u)
            {
                png_u8 sample = (png_u8)png_scale_16_to_sample(gray, bit_depth);
                if (opt->palette_entries != 0u && sample >= opt->palette_entries)
                    return PNG_DEC_ERR_FORMAT;
                png_write_packed_sample(dst_row, dst_x, bit_depth, sample);
            }
            else
            {
                sample8 = png_scale_16_to_8_local(gray);
                if (opt->palette_entries != 0u && sample8 >= opt->palette_entries)
                    return PNG_DEC_ERR_FORMAT;
                dst_row[dst_x] = sample8;
            }
            return PNG_DEC_OK;

        case PNG_COLOR_GRAYSCALE_ALPHA:
            if (bit_depth == 8u)
            {
                dst_row[dst_x * 2u + 0u] = png_scale_16_to_8_local(gray);
                dst_row[dst_x * 2u + 1u] = png_scale_16_to_8_local(a);
            }
            else
            {
                write_be16(dst_row + dst_x * 4u + 0u, gray);
                write_be16(dst_row + dst_x * 4u + 2u, a);
            }
            return PNG_DEC_OK;

        case PNG_COLOR_TRUECOLOR:
            if (bit_depth == 8u)
            {
                off = dst_x * 3u;
                dst_row[off + 0u] = png_scale_16_to_8_local(r);
                dst_row[off + 1u] = png_scale_16_to_8_local(g);
                dst_row[off + 2u] = png_scale_16_to_8_local(b);
            }
            else
            {
                off = dst_x * 6u;
                write_be16(dst_row + off + 0u, r);
                write_be16(dst_row + off + 2u, g);
                write_be16(dst_row + off + 4u, b);
            }
            return PNG_DEC_OK;

        case PNG_COLOR_TRUECOLOR_ALPHA:
            if (bit_depth == 8u)
            {
                off = dst_x * 4u;
                dst_row[off + 0u] = png_scale_16_to_8_local(r);
                dst_row[off + 1u] = png_scale_16_to_8_local(g);
                dst_row[off + 2u] = png_scale_16_to_8_local(b);
                dst_row[off + 3u] = png_scale_16_to_8_local(a);
            }
            else
            {
                off = dst_x * 8u;
                write_be16(dst_row + off + 0u, r);
                write_be16(dst_row + off + 2u, g);
                write_be16(dst_row + off + 4u, b);
                write_be16(dst_row + off + 6u, a);
            }
            return PNG_DEC_OK;

        default:
            return PNG_DEC_ERR_UNSUPPORTED;
    }
}

static int png_build_row_from_public(const png_u8* src_row,
                                     png_u32 width,
                                     const png_encode_options* opt,
                                     png_u8* dst,
                                     png_u32 rowbytes)
{
    png_u32 x;

    memset(dst, 0, rowbytes);
    for (x = 0u; x < width; ++x)
    {
        int err = png_write_target_pixel_from_public(src_row, x, opt, dst, x);
        if (err != PNG_DEC_OK)
            return err;
    }
    return PNG_DEC_OK;
}

static int png_build_pass_row_from_public(const png_u8* src_row,
                                          png_u32 x_start,
                                          png_u32 x_step,
                                          png_u32 pass_width,
                                          const png_encode_options* opt,
                                          png_u8* dst,
                                          png_u32 rowbytes)
{
    png_u32 i;

    memset(dst, 0, rowbytes);
    for (i = 0u; i < pass_width; ++i)
    {
        png_u32 src_x;
        int err;

        src_x = x_start + i * x_step;
        err = png_write_target_pixel_from_public(src_row, src_x, opt, dst, i);
        if (err != PNG_DEC_OK)
            return err;
    }
    return PNG_DEC_OK;
}

static int png_copy_source_row(const png_u8* pixels,
                               png_u32 y,
                               png_u32 stride,
                               png_u32 width,
                               const png_encode_options* opt,
                               png_u8* dst,
                               png_u32 rowbytes)
{
    const png_u8* src_row;
    png_u8 color_type;
    png_u8 bit_depth;

    if (!pixels || !opt || !dst)
        return PNG_DEC_ERR_FORMAT;

    color_type = opt->color_type;
    bit_depth = opt->bit_depth;
    src_row = pixels + y * stride;

    if (opt->input_format != PNG_ENC_INPUT_FORMAT_NATIVE)
        return png_build_row_from_public(src_row, width, opt, dst, rowbytes);

    if ((color_type == PNG_COLOR_GRAYSCALE || color_type == PNG_COLOR_INDEXED) && bit_depth < 8u)
    {
        if (opt->input_is_packed)
        {
            memcpy(dst, src_row, rowbytes);
            return PNG_DEC_OK;
        }
        else
        {
            png_u32 x;
            png_u8 maxv;
            png_u8 per_byte;

            maxv = (png_u8)((1u << bit_depth) - 1u);
            per_byte = (png_u8)(8u / bit_depth);
            memset(dst, 0, rowbytes);

            for (x = 0; x < width; ++x)
            {
                png_u8 v;
                png_u32 byte_index;
                png_u32 pos_in_byte;
                png_u8 shift;

                v = src_row[x];
                if (v > maxv)
                    return PNG_DEC_ERR_FORMAT;

                byte_index = x / per_byte;
                pos_in_byte = x % per_byte;
                shift = (png_u8)(8u - bit_depth * (pos_in_byte + 1u));
                dst[byte_index] = (png_u8)(dst[byte_index] | (png_u8)(v << shift));
            }
            return PNG_DEC_OK;
        }
    }

    memcpy(dst, src_row, rowbytes);
    if (bit_depth == 16u && opt->input_16bit_little_endian)
        png_swap_16_buffer(dst, rowbytes);
    return PNG_DEC_OK;
}

static int png_copy_pass_row(const png_u8* pixels,
                             png_u32 y,
                             png_u32 stride,
                             png_u32 x_start,
                             png_u32 x_step,
                             png_u32 pass_width,
                             const png_encode_options* opt,
                             png_u8* dst,
                             png_u32 rowbytes)
{
    const png_u8* src_row;
    png_u8 color_type;
    png_u8 bit_depth;

    if (!pixels || !opt || !dst)
        return PNG_DEC_ERR_FORMAT;

    color_type = opt->color_type;
    bit_depth = opt->bit_depth;
    src_row = pixels + y * stride;

    if (opt->input_format != PNG_ENC_INPUT_FORMAT_NATIVE)
        return png_build_pass_row_from_public(src_row, x_start, x_step, pass_width, opt, dst, rowbytes);

    if ((color_type == PNG_COLOR_GRAYSCALE || color_type == PNG_COLOR_INDEXED) && bit_depth < 8u)
    {
        png_u32 i;
        png_u8 maxv;

        memset(dst, 0, rowbytes);
        maxv = (png_u8)((1u << bit_depth) - 1u);

        for (i = 0u; i < pass_width; ++i)
        {
            png_u32 src_x;
            png_u8 v;

            src_x = x_start + i * x_step;
            if (opt->input_is_packed)
                v = png_read_packed_sample(src_row, src_x, bit_depth);
            else
                v = src_row[src_x];

            if (v > maxv)
                return PNG_DEC_ERR_FORMAT;

            png_write_packed_sample(dst, i, bit_depth, v);
        }

        return PNG_DEC_OK;
    }
    else
    {
        png_u32 bytes_per_pixel;
        png_u32 i;

        bytes_per_pixel = png_filter_bpp_bytes_enc(color_type, bit_depth);
        if (bytes_per_pixel == 0u)
            return PNG_DEC_ERR_UNSUPPORTED;

        for (i = 0u; i < pass_width; ++i)
        {
            png_u32 src_x;
            png_u8* dst_px;
            src_x = x_start + i * x_step;
            dst_px = dst + i * bytes_per_pixel;
            memcpy(dst_px,
                   src_row + src_x * bytes_per_pixel,
                   bytes_per_pixel);
            if (bit_depth == 16u && opt->input_16bit_little_endian)
                png_swap_16_buffer(dst_px, bytes_per_pixel);
        }

        return PNG_DEC_OK;
    }
}

static int png_compute_adam7_raw_size(png_u32 width,
                                      png_u32 height,
                                      png_u8 color_type,
                                      png_u8 bit_depth,
                                      png_u32* out_size)
{
    png_u32 total;
    int pass;

    if (!out_size)
        return 0;

    total = 0u;
    for (pass = 0; pass < 7; ++pass)
    {
        png_u32 x_start;
        png_u32 y_start;
        png_u32 x_step;
        png_u32 y_step;
        png_u32 pw;
        png_u32 ph;
        png_u32 rowbytes;
        png_u32 per_row;
        png_u32 pass_size;

        x_start = (png_u32)PNG_ADAM7_X_START[pass];
        y_start = (png_u32)PNG_ADAM7_Y_START[pass];
        x_step  = (png_u32)PNG_ADAM7_X_STEP[pass];
        y_step  = (png_u32)PNG_ADAM7_Y_STEP[pass];

        pw = (width > x_start) ? ((width - x_start + x_step - 1u) / x_step) : 0u;
        ph = (height > y_start) ? ((height - y_start + y_step - 1u) / y_step) : 0u;
        if (pw == 0u || ph == 0u)
            continue;

        if (!png_compute_rowbytes(pw, color_type, bit_depth, &rowbytes))
            return 0;
        if (!safe_add_u32(rowbytes, 1u, &per_row))
            return 0;
        if (!safe_mul_u32(per_row, ph, &pass_size))
            return 0;
        if (!safe_add_u32(total, pass_size, &total))
            return 0;
    }

    *out_size = total;
    return 1;
}

static png_u8 png_choose_filter(const png_encode_options* opt,
                                const png_u8* row,
                                const png_u8* prev_row,
                                png_u32 rowbytes,
                                png_u32 bpp,
                                png_u8* filtered_row,
                                png_u8* scratch);

static int png_build_filtered_data(const png_u8* pixels,
                                   png_u32 width,
                                   png_u32 height,
                                   const png_encode_options* opt,
                                   png_u32 src_stride,
                                   png_u8** out_raw,
                                   png_u32* out_raw_size)
{
    png_u32 rowbytes;
    png_u32 raw_row;
    png_u32 raw_size;
    png_u8* raw;
    png_u8* rowbuf;
    png_u8* prev_row;
    png_u8* filtbuf;
    png_u8* scratch;
    png_u32 bpp;
    int err;

    if (!pixels || !opt || !out_raw || !out_raw_size)
        return PNG_DEC_ERR_FORMAT;

    *out_raw = 0;
    *out_raw_size = 0u;

    if (opt->interlace_method == 0u)
    {
        if (!png_compute_rowbytes(width, opt->color_type, opt->bit_depth, &rowbytes))
            return PNG_DEC_ERR_UNSUPPORTED;
        if (!safe_add_u32(rowbytes, 1u, &raw_row))
            return PNG_DEC_ERR_UNSUPPORTED;
        if (!safe_mul_u32(raw_row, height, &raw_size))
            return PNG_DEC_ERR_UNSUPPORTED;
    }
    else if (opt->interlace_method == 1u)
    {
        if (!png_compute_adam7_raw_size(width, height, opt->color_type, opt->bit_depth, &raw_size))
            return PNG_DEC_ERR_UNSUPPORTED;
        if (!png_compute_rowbytes(width, opt->color_type, opt->bit_depth, &rowbytes))
            return PNG_DEC_ERR_UNSUPPORTED;
        raw_row = rowbytes + 1u;
    }
    else
    {
        return PNG_DEC_ERR_UNSUPPORTED;
    }

    if (raw_size > PNG_DEC_MAX_IMAGE_BYTES)
        return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
    if (opt->max_temp_bytes != 0u)
    {
        png_u32 temp_total;
        if (!safe_mul_u32(rowbytes, 4u, &temp_total))
            return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
        if (!safe_add_u32(raw_size, temp_total, &temp_total))
            return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
        if (temp_total > opt->max_temp_bytes)
            return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
    }

    raw = (png_u8*)png_mem89_alloc(raw_size == 0u ? 1u : raw_size);
    rowbuf = (png_u8*)png_mem89_alloc(rowbytes == 0u ? 1u : rowbytes);
    prev_row = (png_u8*)png_mem89_alloc(rowbytes == 0u ? 1u : rowbytes);
    filtbuf = (png_u8*)png_mem89_alloc(rowbytes == 0u ? 1u : rowbytes);
    scratch = (png_u8*)png_mem89_alloc(rowbytes == 0u ? 1u : rowbytes);
    if (!raw || !rowbuf || !prev_row || !filtbuf || !scratch) {
        if (raw) png_mem89_release(raw);
        if (rowbuf) png_mem89_release(rowbuf);
        if (prev_row) png_mem89_release(prev_row);
        if (filtbuf) png_mem89_release(filtbuf);
        if (scratch) png_mem89_release(scratch);
        return PNG_DEC_ERR_OOM;
    }

    bpp = png_filter_bpp_bytes_enc(opt->color_type, opt->bit_depth);
    if (bpp == 0u) {
        png_mem89_release(raw); png_mem89_release(rowbuf); png_mem89_release(prev_row); png_mem89_release(filtbuf); png_mem89_release(scratch);
        return PNG_DEC_ERR_UNSUPPORTED;
    }

    if (opt->interlace_method == 0u)
    {
        png_u32 y;

        memset(prev_row, 0, rowbytes);
        for (y = 0u; y < height; ++y)
        {
            png_u8 filter_type;
            png_u32 off;

            err = png_copy_source_row(pixels, y, src_stride, width, opt, rowbuf, rowbytes);
            if (err != PNG_DEC_OK) {
                png_mem89_release(raw); png_mem89_release(rowbuf); png_mem89_release(prev_row); png_mem89_release(filtbuf); png_mem89_release(scratch);
                return err;
            }

            filter_type = png_choose_filter(opt, rowbuf, prev_row, rowbytes, bpp, filtbuf, scratch);
            off = y * raw_row;
            raw[off] = filter_type;
            memcpy(raw + off + 1u, filtbuf, rowbytes);
            memcpy(prev_row, rowbuf, rowbytes);
        }
    }
    else
    {
        png_u32 raw_off;
        int pass;

        raw_off = 0u;
        for (pass = 0; pass < 7; ++pass)
        {
            png_u32 x_start;
            png_u32 y_start;
            png_u32 x_step;
            png_u32 y_step;
            png_u32 pw;
            png_u32 ph;
            png_u32 pass_rowbytes;
            png_u32 y;

            x_start = (png_u32)PNG_ADAM7_X_START[pass];
            y_start = (png_u32)PNG_ADAM7_Y_START[pass];
            x_step  = (png_u32)PNG_ADAM7_X_STEP[pass];
            y_step  = (png_u32)PNG_ADAM7_Y_STEP[pass];

            pw = (width > x_start) ? ((width - x_start + x_step - 1u) / x_step) : 0u;
            ph = (height > y_start) ? ((height - y_start + y_step - 1u) / y_step) : 0u;
            if (pw == 0u || ph == 0u)
                continue;

            if (!png_compute_rowbytes(pw, opt->color_type, opt->bit_depth, &pass_rowbytes)) {
                png_mem89_release(raw); png_mem89_release(rowbuf); png_mem89_release(prev_row); png_mem89_release(filtbuf); png_mem89_release(scratch);
                return PNG_DEC_ERR_UNSUPPORTED;
            }

            memset(prev_row, 0, pass_rowbytes);
            for (y = 0u; y < ph; ++y)
            {
                png_u8 filter_type;
                png_u32 src_y;

                src_y = y_start + y * y_step;
                err = png_copy_pass_row(pixels,
                                        src_y,
                                        src_stride,
                                        x_start,
                                        x_step,
                                        pw,
                                        opt,
                                        rowbuf,
                                        pass_rowbytes);
                if (err != PNG_DEC_OK) {
                    png_mem89_release(raw); png_mem89_release(rowbuf); png_mem89_release(prev_row); png_mem89_release(filtbuf); png_mem89_release(scratch);
                    return err;
                }

                filter_type = png_choose_filter(opt, rowbuf, prev_row, pass_rowbytes, bpp, filtbuf, scratch);
                raw[raw_off++] = filter_type;
                memcpy(raw + raw_off, filtbuf, pass_rowbytes);
                raw_off += pass_rowbytes;
                memcpy(prev_row, rowbuf, pass_rowbytes);
            }
        }

        raw_size = raw_off;
    }

    png_mem89_release(rowbuf);
    png_mem89_release(prev_row);
    png_mem89_release(filtbuf);
    png_mem89_release(scratch);

    *out_raw = raw;
    *out_raw_size = raw_size;
    return PNG_DEC_OK;
}

static int png_abs_signed_byte(png_u8 v)
{
    int sv;
    sv = (int)(signed char)v;
    return (sv < 0) ? -sv : sv;
}

static int png_paeth_predictor_enc(int a, int b, int c)
{
    int p;
    int pa;
    int pb;
    int pc;

    p = a + b - c;
    pa = p - a; if (pa < 0) pa = -pa;
    pb = p - b; if (pb < 0) pb = -pb;
    pc = p - c; if (pc < 0) pc = -pc;

    if (pa <= pb && pa <= pc) return a;
    if (pb <= pc) return b;
    return c;
}

static png_u32 png_filter_row_compute(png_u8 filter_type,
                                      const png_u8* row,
                                      const png_u8* prev_row,
                                      png_u32 rowbytes,
                                      png_u32 bpp,
                                      png_u8* out)
{
    png_u32 i;
    png_u32 score;

    score = 0u;
    for (i = 0; i < rowbytes; ++i)
    {
        png_u8 left;
        png_u8 up;
        png_u8 upleft;
        png_u8 v;

        left = (i >= bpp) ? row[i - bpp] : 0u;
        up = prev_row ? prev_row[i] : 0u;
        upleft = (prev_row && i >= bpp) ? prev_row[i - bpp] : 0u;

        switch (filter_type)
        {
            case 0:
                v = row[i];
                break;
            case 1:
                v = (png_u8)(row[i] - left);
                break;
            case 2:
                v = (png_u8)(row[i] - up);
                break;
            case 3:
                v = (png_u8)(row[i] - (png_u8)(((png_u32)left + (png_u32)up) >> 1));
                break;
            case 4:
                v = (png_u8)(row[i] - (png_u8)png_paeth_predictor_enc((int)left, (int)up, (int)upleft));
                break;
            default:
                v = row[i];
                break;
        }

        out[i] = v;
        score += (png_u32)png_abs_signed_byte(v);
    }

    return score;
}

static png_u8 png_choose_filter(const png_encode_options* opt,
                                const png_u8* row,
                                const png_u8* prev_row,
                                png_u32 rowbytes,
                                png_u32 bpp,
                                png_u8* filtered_row,
                                png_u8* scratch)
{
    png_u8 strategy;
    png_u8 mask;
    png_u8 best_filter;
    png_u32 best_score;
    png_u8 f;

    strategy = opt->filter_strategy;
    mask = opt->filter_mask ? opt->filter_mask : PNG_ENC_FILTER_MASK_ALL;

    if (strategy <= PNG_ENC_FILTER_PAETH_FIXED)
    {
        png_filter_row_compute(strategy, row, prev_row, rowbytes, bpp, filtered_row);
        return strategy;
    }

    best_filter = 0u;
    best_score = 0xFFFFFFFFu;

    for (f = 0u; f <= 4u; ++f)
    {
        png_u8 bit;
        png_u32 score;

        bit = (png_u8)(1u << f);
        if ((mask & bit) == 0u)
            continue;

        score = png_filter_row_compute(f, row, prev_row, rowbytes, bpp, scratch);
        if (score < best_score)
        {
            best_score = score;
            best_filter = f;
            memcpy(filtered_row, scratch, rowbytes);
        }
    }

    return best_filter;
}

void png_encode_options_init(png_encode_options* opt)
{
    if (!opt)
        return;

    memset(opt, 0, sizeof(*opt));
    opt->color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt->bit_depth = 8u;
    opt->interlace_method = 0u;
    opt->filter_strategy = PNG_ENC_FILTER_ADAPTIVE;
    opt->filter_mask = PNG_ENC_FILTER_MASK_ALL;
    opt->input_format = PNG_ENC_INPUT_FORMAT_NATIVE;
    opt->input_transform_flags = PNG_ENC_TRANSFORM_NONE;
    opt->zlevel = 6;
    opt->max_temp_bytes = PNG_DEC_MAX_TEMP_BYTES;
    opt->max_conversion_expansion = PNG_DEC_MAX_CONVERSION_EXPANSION;
    opt->max_apng_frames = PNG_DEC_MAX_APNG_FRAMES;
}

static int png_q16_to_png_chunk_fixed(png_fixed89 v, png_u32* out)
{
    return png_fixed_to_scaled_u32(v, 100000u, out);
}


static int png_build_chrm_chunk(const png_encode_options* opt, png_u8 out[32])
{
    png_u32 v;

    if (!opt || !out)
        return 0;

    if (!png_q16_to_png_chunk_fixed(opt->white_x, &v)) return 0;
    write_be32(out + 0, v);
    if (!png_q16_to_png_chunk_fixed(opt->white_y, &v)) return 0;
    write_be32(out + 4, v);
    if (!png_q16_to_png_chunk_fixed(opt->red_x, &v)) return 0;
    write_be32(out + 8, v);
    if (!png_q16_to_png_chunk_fixed(opt->red_y, &v)) return 0;
    write_be32(out + 12, v);
    if (!png_q16_to_png_chunk_fixed(opt->green_x, &v)) return 0;
    write_be32(out + 16, v);
    if (!png_q16_to_png_chunk_fixed(opt->green_y, &v)) return 0;
    write_be32(out + 20, v);
    if (!png_q16_to_png_chunk_fixed(opt->blue_x, &v)) return 0;
    write_be32(out + 24, v);
    if (!png_q16_to_png_chunk_fixed(opt->blue_y, &v)) return 0;
    write_be32(out + 28, v);
    return 1;
}

static png_u16 png_scale_16_to_sample(png_u16 v16, png_u8 bit_depth)
{
    png_u32 maxv;
    png_u32 scaled;

    if (bit_depth >= 16u)
        return v16;

    if (bit_depth == 0u)
        return 0u;

    maxv = (1u << bit_depth) - 1u;
    scaled = ((png_u32)v16 * maxv + 32767u) / 65535u;
    return (png_u16)scaled;
}

static png_u8 png_scale_16_to_8_nearest(png_u16 v16)
{
    return (png_u8)(((png_u32)v16 * 255u + 32767u) / 65535u);
}

static int png_validate_latin1_name_enc(const char* name)
{
    const unsigned char* p;
    unsigned int len;
    unsigned char prev_space;

    if (!name || !name[0])
        return 0;

    p = (const unsigned char*)name;
    len = 0u;
    prev_space = 1u;
    while (*p)
    {
        unsigned char c = *p++;
        ++len;
        if (len > 79u)
            return 0;
        if (!((c >= 0x20u && c <= 0x7Eu) || c >= 0xA1u))
            return 0;
        if (c == 0x20u)
        {
            if (prev_space)
                return 0;
            prev_space = 1u;
        }
        else
            prev_space = 0u;
    }

    return prev_space ? 0 : 1;
}

static int png_validate_latin1_text_enc(const char* s, int allow_empty)
{
    const unsigned char* p;

    if (!s)
        return 0;
    if (!allow_empty && !s[0])
        return 0;

    p = (const unsigned char*)s;
    while (*p)
    {
        unsigned char c = *p++;
        if (!((c >= 0x20u && c <= 0x7Eu) || c >= 0xA1u))
            return 0;
    }
    return 1;
}

static int png_parse_fixed_string_enc(const char* s, png_fixed89* out_value)
{
    return png_fixed89_parse_decimal(s, out_value);
}

static int png_parse_positive_fixed_string_enc(const char* s)
{
    png_fixed89 v;
    if (!png_parse_fixed_string_enc(s, &v))
        return 0;
    return (v > 0) ? 1 : 0;
}


static int png_i32_to_u32_checked(png_i32 v, png_u32* out)
{
    if (!out)
        return 0;
    if (v < 0)
        *out = ((png_u32)(v - (-2147483647 - 1))) | 0x80000000u;
    else
        *out = (png_u32)v;
    return 1;
}

static int png_should_write_plte(const png_encode_options* opt)
{
    if (!opt || !opt->palette || opt->palette_entries == 0u)
        return 0;

    return (opt->color_type == PNG_COLOR_INDEXED ||
            opt->color_type == PNG_COLOR_TRUECOLOR ||
            opt->color_type == PNG_COLOR_TRUECOLOR_ALPHA) ? 1 : 0;
}

static int png_build_sbit_chunk(const png_encode_options* opt, png_u8 out[4], png_u32* out_len)
{
    if (!opt || !out || !out_len)
        return 0;

    switch (opt->color_type)
    {
        case PNG_COLOR_GRAYSCALE:
            if (opt->sbit_r == 0u || opt->sbit_r > opt->bit_depth) return 0;
            out[0] = opt->sbit_r;
            *out_len = 1u;
            return 1;
        case PNG_COLOR_TRUECOLOR:
        case PNG_COLOR_INDEXED:
            if (opt->sbit_r == 0u || opt->sbit_g == 0u || opt->sbit_b == 0u) return 0;
            if (opt->sbit_r > 8u || opt->sbit_g > 8u || opt->sbit_b > 8u) return 0;
            if (opt->color_type == PNG_COLOR_TRUECOLOR &&
                (opt->sbit_r > opt->bit_depth || opt->sbit_g > opt->bit_depth || opt->sbit_b > opt->bit_depth))
                return 0;
            out[0] = opt->sbit_r; out[1] = opt->sbit_g; out[2] = opt->sbit_b;
            *out_len = 3u;
            return 1;
        case PNG_COLOR_GRAYSCALE_ALPHA:
            if (opt->sbit_r == 0u || opt->sbit_a == 0u) return 0;
            if (opt->sbit_r > opt->bit_depth || opt->sbit_a > opt->bit_depth) return 0;
            out[0] = opt->sbit_r; out[1] = opt->sbit_a;
            *out_len = 2u;
            return 1;
        case PNG_COLOR_TRUECOLOR_ALPHA:
            if (opt->sbit_r == 0u || opt->sbit_g == 0u || opt->sbit_b == 0u || opt->sbit_a == 0u) return 0;
            if (opt->sbit_r > opt->bit_depth || opt->sbit_g > opt->bit_depth ||
                opt->sbit_b > opt->bit_depth || opt->sbit_a > opt->bit_depth)
                return 0;
            out[0] = opt->sbit_r; out[1] = opt->sbit_g; out[2] = opt->sbit_b; out[3] = opt->sbit_a;
            *out_len = 4u;
            return 1;
        default:
            return 0;
    }
}

static int png_build_bkgd_chunk(const png_encode_options* opt, png_u8 out[6], png_u32* out_len)
{
    png_u16 s;

    if (!opt || !out || !out_len)
        return 0;

    switch (opt->color_type)
    {
        case PNG_COLOR_GRAYSCALE:
        case PNG_COLOR_GRAYSCALE_ALPHA:
            s = png_scale_16_to_sample(opt->bkgd_r, opt->bit_depth);
            write_be16(out + 0, s);
            *out_len = 2u;
            return 1;
        case PNG_COLOR_TRUECOLOR:
        case PNG_COLOR_TRUECOLOR_ALPHA:
            write_be16(out + 0, png_scale_16_to_sample(opt->bkgd_r, opt->bit_depth));
            write_be16(out + 2, png_scale_16_to_sample(opt->bkgd_g, opt->bit_depth));
            write_be16(out + 4, png_scale_16_to_sample(opt->bkgd_b, opt->bit_depth));
            *out_len = 6u;
            return 1;
        case PNG_COLOR_INDEXED:
            out[0] = opt->bkgd_palette_index;
            *out_len = 1u;
            return 1;
        default:
            return 0;
    }
}

static int png_append_legacy_extension_chunks(png_write_buffer* wb,
                                              const png_encode_options* opt,
                                              png_u8 location)
{
    png_u32 i;
    int err;

    if (!wb || !opt || !png_is_valid_generic_location(location))
        return PNG_DEC_ERR_FORMAT;

    if (opt->gifg_chunk_count != 0u)
    {
        if (!opt->gifg_chunks)
            return PNG_DEC_ERR_FORMAT;
        for (i = 0u; i < opt->gifg_chunk_count; ++i)
        {
            png_u8 chunk[4];
            const png_gifg_entry* ge = &opt->gifg_chunks[i];
            if (ge->location != location)
                continue;
            if (!png_is_valid_generic_location(ge->location) || ge->user_input_flag > 1u || ge->disposal_method > 7u)
                return PNG_DEC_ERR_FORMAT;
            chunk[0] = ge->disposal_method;
            chunk[1] = ge->user_input_flag;
            write_be16(chunk + 2u, ge->delay_time_cs);
            err = write_chunk_buffer(wb, (const png_u8*)"gIFg", chunk, 4u);
            if (err != PNG_DEC_OK) return err;
        }
    }

    if (opt->gifx_chunk_count != 0u)
    {
        if (!opt->gifx_chunks)
            return PNG_DEC_ERR_FORMAT;
        for (i = 0u; i < opt->gifx_chunk_count; ++i)
        {
            png_write_buffer tmp;
            const png_gifx_entry* gx = &opt->gifx_chunks[i];
            if (gx->location != location)
                continue;
            if (!png_is_valid_generic_location(gx->location) ||
                !png_validate_printable_ascii_bytes_enc((const png_u8*)gx->application_identifier, 8u))
                return PNG_DEC_ERR_FORMAT;
            memset(&tmp, 0, sizeof(tmp));
            err = wb_append(&tmp, (const png_u8*)gx->application_identifier, 8u);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            err = wb_append(&tmp, gx->authentication_code, 3u);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            if (gx->application_data_size != 0u)
            {
                if (!gx->application_data)
                {
                    if (tmp.data) png_mem89_release(tmp.data);
                    return PNG_DEC_ERR_FORMAT;
                }
                err = wb_append(&tmp, gx->application_data, gx->application_data_size);
                if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            }
            err = write_chunk_buffer(wb, (const png_u8*)"gIFx", tmp.data, tmp.size);
            png_mem89_release(tmp.data);
            if (err != PNG_DEC_OK) return err;
        }
    }

    if (opt->gift_chunk_count != 0u)
    {
        if (!opt->gift_chunks)
            return PNG_DEC_ERR_FORMAT;
        for (i = 0u; i < opt->gift_chunk_count; ++i)
        {
            png_write_buffer tmp;
            png_u8 hdr[24];
            const png_gift_entry* gt = &opt->gift_chunks[i];
            png_u32 ux;
            png_u32 uy;
            if (gt->location != location)
                continue;
            if (!png_is_valid_generic_location(gt->location) ||
                !png_i32_to_u32_checked(gt->grid_left, &ux) ||
                !png_i32_to_u32_checked(gt->grid_top, &uy))
                return PNG_DEC_ERR_FORMAT;
            memset(&tmp, 0, sizeof(tmp));
            write_be32(hdr + 0u, ux);
            write_be32(hdr + 4u, uy);
            write_be32(hdr + 8u, gt->grid_width);
            write_be32(hdr + 12u, gt->grid_height);
            hdr[16] = gt->cell_width;
            hdr[17] = gt->cell_height;
            memcpy(hdr + 18u, gt->foreground_rgb, 3u);
            memcpy(hdr + 21u, gt->background_rgb, 3u);
            err = wb_append(&tmp, hdr, 24u);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            if (gt->text_data_size != 0u)
            {
                if (!gt->text_data)
                {
                    if (tmp.data) png_mem89_release(tmp.data);
                    return PNG_DEC_ERR_FORMAT;
                }
                err = wb_append(&tmp, gt->text_data, gt->text_data_size);
                if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            }
            err = write_chunk_buffer(wb, (const png_u8*)"gIFt", tmp.data, tmp.size);
            png_mem89_release(tmp.data);
            if (err != PNG_DEC_OK) return err;
        }
    }

    if (opt->frac_chunk_count != 0u)
    {
        if (!opt->frac_chunks)
            return PNG_DEC_ERR_FORMAT;
        for (i = 0u; i < opt->frac_chunk_count; ++i)
        {
            const png_frac_entry* fr = &opt->frac_chunks[i];
            if (fr->location != location)
                continue;
            if (!png_is_valid_generic_location(fr->location))
                return PNG_DEC_ERR_FORMAT;
            if (fr->size != 0u && !fr->data)
                return PNG_DEC_ERR_FORMAT;
            err = write_chunk_buffer(wb, (const png_u8*)"fRAc", fr->data, fr->size);
            if (err != PNG_DEC_OK) return err;
        }
    }

    return PNG_DEC_OK;
}

static int png_append_standard_chunks(png_write_buffer* wb,
                                      png_zlib_compress_func zfunc,
                                      const png_encode_options* opt,
                                      png_u8 before_plte)
{
    png_u32 i;
    int err;
    png_u8 chunk[32];

    if (!wb || !opt)
        return PNG_DEC_ERR_FORMAT;

    if (before_plte)
    {
        if (opt->write_gAMA && !opt->write_sRGB)
        {
            png_u32 g;
            if (opt->image_gamma > 0)
            {
                if (!png_fixed_to_scaled_u32(opt->image_gamma, 100000u, &g))
                    return PNG_DEC_ERR_FORMAT;
                write_be32(chunk, g);
                err = write_chunk_buffer(wb, (const png_u8*)"gAMA", chunk, 4u);
                if (err != PNG_DEC_OK) return err;
            }
        }

        if (opt->write_cHRM)
        {
            if (!png_build_chrm_chunk(opt, chunk))
                return PNG_DEC_ERR_FORMAT;
            err = write_chunk_buffer(wb, (const png_u8*)"cHRM", chunk, 32u);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_sBIT)
        {
            png_u32 sbit_len;
            if (!png_build_sbit_chunk(opt, chunk, &sbit_len))
                return PNG_DEC_ERR_FORMAT;
            err = write_chunk_buffer(wb, (const png_u8*)"sBIT", chunk, sbit_len);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_sRGB)
        {
            chunk[0] = opt->srgb_intent;
            err = write_chunk_buffer(wb, (const png_u8*)"sRGB", chunk, 1u);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_cICP)
        {
            chunk[0] = opt->cicp.colour_primaries;
            chunk[1] = opt->cicp.transfer_function;
            chunk[2] = opt->cicp.matrix_coefficients;
            chunk[3] = opt->cicp.full_range_flag;
            err = write_chunk_buffer(wb, (const png_u8*)"cICP", chunk, 4u);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_mDCV)
        {
            png_u16 vals16[8];
            png_u32 max_lum;
            png_u32 min_lum;
            if (!png_fixed_to_scaled_u16(opt->mdcv.display_primaries_x[0], 50000u, &vals16[0]) ||
                !png_fixed_to_scaled_u16(opt->mdcv.display_primaries_y[0], 50000u, &vals16[1]) ||
                !png_fixed_to_scaled_u16(opt->mdcv.display_primaries_x[1], 50000u, &vals16[2]) ||
                !png_fixed_to_scaled_u16(opt->mdcv.display_primaries_y[1], 50000u, &vals16[3]) ||
                !png_fixed_to_scaled_u16(opt->mdcv.display_primaries_x[2], 50000u, &vals16[4]) ||
                !png_fixed_to_scaled_u16(opt->mdcv.display_primaries_y[2], 50000u, &vals16[5]) ||
                !png_fixed_to_scaled_u16(opt->mdcv.white_point_x, 50000u, &vals16[6]) ||
                !png_fixed_to_scaled_u16(opt->mdcv.white_point_y, 50000u, &vals16[7]) ||
                !png_fixed_to_scaled_u32(opt->mdcv.max_luminance, 10000u, &max_lum) ||
                !png_fixed_to_scaled_u32(opt->mdcv.min_luminance, 10000u, &min_lum))
                return PNG_DEC_ERR_FORMAT;
            write_be16(chunk + 0, vals16[0]);
            write_be16(chunk + 2, vals16[1]);
            write_be16(chunk + 4, vals16[2]);
            write_be16(chunk + 6, vals16[3]);
            write_be16(chunk + 8, vals16[4]);
            write_be16(chunk + 10, vals16[5]);
            write_be16(chunk + 12, vals16[6]);
            write_be16(chunk + 14, vals16[7]);
            write_be32(chunk + 16, max_lum);
            write_be32(chunk + 20, min_lum);
            err = write_chunk_buffer(wb, (const png_u8*)"mDCV", chunk, 24u);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_cLLI)
        {
            png_u32 max_cll;
            png_u32 max_fall;
            if (!png_fixed_to_scaled_u32(opt->clli.max_content_light_level, 10000u, &max_cll) ||
                !png_fixed_to_scaled_u32(opt->clli.max_frame_average_light_level, 10000u, &max_fall))
                return PNG_DEC_ERR_FORMAT;
            write_be32(chunk + 0, max_cll);
            write_be32(chunk + 4, max_fall);
            err = write_chunk_buffer(wb, (const png_u8*)"cLLI", chunk, 8u);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_iCCP)
        {
            png_write_buffer tmp;
            png_u32 name_len;
            png_u32 comp_bound;
            png_u32 comp_len;
            png_u8* comp;

            if (!png_is_valid_iccp_name(opt->iccp_name) ||
                !opt->iccp_profile ||
                opt->iccp_profile_size == 0u)
                return PNG_DEC_ERR_FORMAT;

            name_len = (png_u32)strlen(opt->iccp_name);
            memset(&tmp, 0, sizeof(tmp));
            err = wb_append(&tmp, (const png_u8*)opt->iccp_name, name_len);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            chunk[0] = 0u;
            chunk[1] = 0u;
            err = wb_append(&tmp, chunk, 2u);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }

            if (!deflate_bound_u32(opt->iccp_profile_size, &comp_bound)) {
                if (tmp.data) png_mem89_release(tmp.data);
                return PNG_DEC_ERR_UNSUPPORTED;
            }
            comp = (png_u8*)png_mem89_alloc(comp_bound == 0u ? 1u : comp_bound);
            if (!comp) { if (tmp.data) png_mem89_release(tmp.data); return PNG_DEC_ERR_OOM; }
            comp_len = comp_bound;
            if (zfunc(comp, &comp_len, opt->iccp_profile, opt->iccp_profile_size, opt->zlevel) != 0) {
                png_mem89_release(comp); if (tmp.data) png_mem89_release(tmp.data); return PNG_DEC_ERR_ZLIB;
            }
            err = wb_append(&tmp, comp, comp_len);
            png_mem89_release(comp);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            err = write_chunk_buffer(wb, (const png_u8*)"iCCP", tmp.data, tmp.size);
            png_mem89_release(tmp.data);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_pHYs)
        {
            write_be32(chunk + 0, opt->pHYs_ppu_x);
            write_be32(chunk + 4, opt->pHYs_ppu_y);
            chunk[8] = opt->pHYs_unit;
            err = write_chunk_buffer(wb, (const png_u8*)"pHYs", chunk, 9u);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_oFFs)
        {
            png_u32 ux, uy;
            if (!png_i32_to_u32_checked(opt->offset_x, &ux) ||
                !png_i32_to_u32_checked(opt->offset_y, &uy) ||
                !(opt->offset_unit == 0u || opt->offset_unit == 1u))
                return PNG_DEC_ERR_FORMAT;
            write_be32(chunk + 0, ux);
            write_be32(chunk + 4, uy);
            chunk[8] = opt->offset_unit;
            err = write_chunk_buffer(wb, (const png_u8*)"oFFs", chunk, 9u);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_sCAL)
        {
            png_write_buffer tmp;
            png_u32 sx_len;
            png_u32 sy_len;
            if (!(opt->scal_unit == 1u || opt->scal_unit == 2u) ||
                !png_parse_positive_fixed_string_enc(opt->scal_pixel_width) ||
                !png_parse_positive_fixed_string_enc(opt->scal_pixel_height))
                return PNG_DEC_ERR_FORMAT;
            memset(&tmp, 0, sizeof(tmp));
            chunk[0] = opt->scal_unit;
            err = wb_append(&tmp, chunk, 1u);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            sx_len = (png_u32)strlen(opt->scal_pixel_width);
            sy_len = (png_u32)strlen(opt->scal_pixel_height);
            err = wb_append(&tmp, (const png_u8*)opt->scal_pixel_width, sx_len);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            chunk[0] = 0u;
            err = wb_append(&tmp, chunk, 1u);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            err = wb_append(&tmp, (const png_u8*)opt->scal_pixel_height, sy_len);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            err = write_chunk_buffer(wb, (const png_u8*)"sCAL", tmp.data, tmp.size);
            png_mem89_release(tmp.data);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_pCAL)
        {
            png_write_buffer tmp;
            png_u32 ux0;
            png_u32 ux1;
            int expected_params;
            png_u32 j;

            expected_params = png_pcal_expected_param_count(opt->pcal.equation_type);
            if (!png_validate_latin1_name_enc(opt->pcal.name) ||
                !png_validate_latin1_text_enc(opt->pcal.unit_name ? opt->pcal.unit_name : "", 1) ||
                expected_params < 0 ||
                opt->pcal.param_count != (png_u32)expected_params ||
                !opt->pcal.params ||
                opt->pcal.x0 == opt->pcal.x1 ||
                !png_i32_to_u32_checked(opt->pcal.x0, &ux0) ||
                !png_i32_to_u32_checked(opt->pcal.x1, &ux1))
                return PNG_DEC_ERR_FORMAT;

            memset(&tmp, 0, sizeof(tmp));
            err = wb_append(&tmp, (const png_u8*)opt->pcal.name, (png_u32)strlen(opt->pcal.name));
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            chunk[0] = 0u;
            err = wb_append(&tmp, chunk, 1u);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            write_be32(chunk + 0, ux0);
            write_be32(chunk + 4, ux1);
            chunk[8] = opt->pcal.equation_type;
            chunk[9] = (png_u8)opt->pcal.param_count;
            err = wb_append(&tmp, chunk, 10u);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            if (opt->pcal.unit_name && opt->pcal.unit_name[0])
            {
                err = wb_append(&tmp,
                                (const png_u8*)opt->pcal.unit_name,
                                (png_u32)strlen(opt->pcal.unit_name));
                if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            }
            chunk[0] = 0u;
            err = wb_append(&tmp, chunk, 1u);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            for (j = 0u; j < opt->pcal.param_count; ++j)
            {
                const char* param = opt->pcal.params[j];
                if (!png_parse_fixed_string_enc(param, 0)) { if (tmp.data) png_mem89_release(tmp.data); return PNG_DEC_ERR_FORMAT; }
                err = wb_append(&tmp, (const png_u8*)param, (png_u32)strlen(param));
                if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
                if (j + 1u != opt->pcal.param_count)
                {
                    chunk[0] = 0u;
                    err = wb_append(&tmp, chunk, 1u);
                    if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
                }
            }
            err = write_chunk_buffer(wb, (const png_u8*)"pCAL", tmp.data, tmp.size);
            png_mem89_release(tmp.data);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_sTER)
        {
            if (!(opt->ster_mode == 0u || opt->ster_mode == 1u))
                return PNG_DEC_ERR_FORMAT;
            chunk[0] = opt->ster_mode;
            err = write_chunk_buffer(wb, (const png_u8*)"sTER", chunk, 1u);
            if (err != PNG_DEC_OK) return err;
        }

        for (i = 0; i < opt->splt_palette_count; ++i)
        {
            const png_splt_palette* sp;
            png_write_buffer tmp;
            png_u32 name_len;
            png_u32 j;

            if (!opt->splt_palettes)
                break;

            sp = &opt->splt_palettes[i];
            if (!sp->name || !png_validate_latin1_name_enc(sp->name) ||
                !(sp->sample_depth == 8u || sp->sample_depth == 16u) ||
                !sp->entries || sp->entry_count == 0u)
                return PNG_DEC_ERR_FORMAT;

            memset(&tmp, 0, sizeof(tmp));
            name_len = (png_u32)strlen(sp->name);
            err = wb_append(&tmp, (const png_u8*)sp->name, name_len);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            chunk[0] = 0u;
            chunk[1] = sp->sample_depth;
            err = wb_append(&tmp, chunk, 2u);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }

            for (j = 0u; j < sp->entry_count; ++j)
            {
                png_u8 entry[10];
                const png_splt_entry* e = &sp->entries[j];
                if (sp->sample_depth == 8u)
                {
                    entry[0] = png_scale_16_to_8_nearest(e->red);
                    entry[1] = png_scale_16_to_8_nearest(e->green);
                    entry[2] = png_scale_16_to_8_nearest(e->blue);
                    entry[3] = png_scale_16_to_8_nearest(e->alpha);
                    write_be16(entry + 4, e->frequency);
                    err = wb_append(&tmp, entry, 6u);
                }
                else
                {
                    write_be16(entry + 0, e->red);
                    write_be16(entry + 2, e->green);
                    write_be16(entry + 4, e->blue);
                    write_be16(entry + 6, e->alpha);
                    write_be16(entry + 8, e->frequency);
                    err = wb_append(&tmp, entry, 10u);
                }
                if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            }

            err = write_chunk_buffer(wb, (const png_u8*)"sPLT", tmp.data, tmp.size);
            png_mem89_release(tmp.data);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_eXIf)
        {
            if (!opt->exif_profile || opt->exif_profile_size == 0u ||
                !png_exif_has_valid_tiff_header(opt->exif_profile, opt->exif_profile_size))
                return PNG_DEC_ERR_FORMAT;
            err = write_chunk_buffer(wb, (const png_u8*)"eXIf", opt->exif_profile, opt->exif_profile_size);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->write_tIME)
        {
            write_be16(chunk + 0, opt->time_year);
            chunk[2] = opt->time_month;
            chunk[3] = opt->time_day;
            chunk[4] = opt->time_hour;
            chunk[5] = opt->time_minute;
            chunk[6] = opt->time_second;
            err = write_chunk_buffer(wb, (const png_u8*)"tIME", chunk, 7u);
            if (err != PNG_DEC_OK) return err;
        }

        for (i = 0; i < opt->text_count; ++i)
        {
            const png_text_entry* te;
            png_u32 key_len;
            png_u32 text_len;
            png_u32 lang_len;
            png_u32 trans_len;
            png_write_buffer tmp;

            te = &opt->text_entries[i];
            if (!te->keyword || !te->text)
                continue;

            key_len = (png_u32)strlen(te->keyword);
            if (key_len == 0u || key_len > 79u)
                continue;

            text_len = (png_u32)strlen(te->text);
            lang_len = te->language_tag ? (png_u32)strlen(te->language_tag) : 0u;
            trans_len = te->translated_keyword ? (png_u32)strlen(te->translated_keyword) : 0u;

            memset(&tmp, 0, sizeof(tmp));
            err = wb_append(&tmp, (const png_u8*)te->keyword, key_len);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
            chunk[0] = 0u;
            err = wb_append(&tmp, chunk, 1u);
            if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }

            if (te->compression == 1u)
            {
                png_u32 bound;
                png_u8* comp;
                png_u32 comp_len;

                chunk[0] = 0u;
                err = wb_append(&tmp, chunk, 1u);
                if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }

                if (!deflate_bound_u32(text_len, &bound)) {
                    if (tmp.data) png_mem89_release(tmp.data);
                    return PNG_DEC_ERR_UNSUPPORTED;
                }
                comp = (png_u8*)png_mem89_alloc(bound == 0u ? 1u : bound);
                if (!comp) { if (tmp.data) png_mem89_release(tmp.data); return PNG_DEC_ERR_OOM; }
                comp_len = bound;
                if (zfunc(comp, &comp_len, (const png_u8*)te->text, text_len, opt->zlevel) != 0) {
                    png_mem89_release(comp); if (tmp.data) png_mem89_release(tmp.data); return PNG_DEC_ERR_ZLIB;
                }
                err = wb_append(&tmp, comp, comp_len);
                png_mem89_release(comp);
                if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
                err = write_chunk_buffer(wb, (const png_u8*)"zTXt", tmp.data, tmp.size);
                png_mem89_release(tmp.data);
                if (err != PNG_DEC_OK) return err;
            }
            else if (te->compression == 2u || lang_len != 0u || trans_len != 0u)
            {
                chunk[0] = 0u; /* compression flag */
                chunk[1] = 0u; /* compression method */
                err = wb_append(&tmp, chunk, 2u);
                if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
                if (lang_len != 0u) {
                    err = wb_append(&tmp, (const png_u8*)te->language_tag, lang_len);
                    if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
                }
                chunk[0] = 0u;
                err = wb_append(&tmp, chunk, 1u);
                if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
                if (trans_len != 0u) {
                    err = wb_append(&tmp, (const png_u8*)te->translated_keyword, trans_len);
                    if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
                }
                chunk[0] = 0u;
                err = wb_append(&tmp, chunk, 1u);
                if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
                if (text_len != 0u) {
                    err = wb_append(&tmp, (const png_u8*)te->text, text_len);
                    if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
                }
                err = write_chunk_buffer(wb, (const png_u8*)"iTXt", tmp.data, tmp.size);
                png_mem89_release(tmp.data);
                if (err != PNG_DEC_OK) return err;
            }
            else
            {
                if (text_len != 0u) {
                    err = wb_append(&tmp, (const png_u8*)te->text, text_len);
                    if (err != PNG_DEC_OK) { if (tmp.data) png_mem89_release(tmp.data); return err; }
                }
                err = write_chunk_buffer(wb, (const png_u8*)"tEXt", tmp.data, tmp.size);
                png_mem89_release(tmp.data);
                if (err != PNG_DEC_OK) return err;
            }
        }

        for (i = 0; i < opt->unknown_chunk_count; ++i)
        {
            const png_unknown_chunk* uc;
            if (!opt->unknown_chunks)
                break;
            uc = &opt->unknown_chunks[i];
            if (uc->location != PNG_CHUNK_POS_AFTER_IHDR)
                continue;
            err = write_chunk_buffer(wb, uc->type, uc->data, uc->size);
            if (err != PNG_DEC_OK) return err;
        }
    }
    else
    {
        if (opt->write_bKGD)
        {
            png_u32 bkgd_len;
            if (!png_build_bkgd_chunk(opt, chunk, &bkgd_len))
                return PNG_DEC_ERR_FORMAT;
            err = write_chunk_buffer(wb, (const png_u8*)"bKGD", chunk, bkgd_len);
            if (err != PNG_DEC_OK) return err;
        }

        if (opt->hist_entries && opt->hist_count != 0u)
        {
            png_u8* hist_buf;
            png_u32 j;
            hist_buf = (png_u8*)png_mem89_alloc(opt->hist_count * 2u);
            if (!hist_buf)
                return PNG_DEC_ERR_OOM;
            for (j = 0u; j < opt->hist_count; ++j)
                write_be16(hist_buf + j * 2u, opt->hist_entries[j]);
            err = write_chunk_buffer(wb, (const png_u8*)"hIST", hist_buf, opt->hist_count * 2u);
            png_mem89_release(hist_buf);
            if (err != PNG_DEC_OK) return err;
        }

        for (i = 0; i < opt->unknown_chunk_count; ++i)
        {
            const png_unknown_chunk* uc;
            if (!opt->unknown_chunks)
                break;
            uc = &opt->unknown_chunks[i];
            if (uc->location != PNG_CHUNK_POS_AFTER_PLTE)
                continue;
            err = write_chunk_buffer(wb, uc->type, uc->data, uc->size);
            if (err != PNG_DEC_OK) return err;
        }
    }

    return PNG_DEC_OK;
}

static int png_append_after_idat_chunks(png_write_buffer* wb,
                                        const png_encode_options* opt)
{
    png_u32 i;
    int err;

    if (!wb || !opt)
        return PNG_DEC_ERR_FORMAT;

    for (i = 0; i < opt->unknown_chunk_count; ++i)
    {
        const png_unknown_chunk* uc;
        if (!opt->unknown_chunks)
            break;
        uc = &opt->unknown_chunks[i];
        if (uc->location != PNG_CHUNK_POS_AFTER_IDAT)
            continue;
        err = write_chunk_buffer(wb, uc->type, uc->data, uc->size);
        if (err != PNG_DEC_OK) return err;
    }

    return PNG_DEC_OK;
}

int png_encode_memory_ex(const png_u8* pixels,
                         png_u32 width,
                         png_u32 height,
                         png_zlib_compress_func zfunc,
                         const png_encode_options* options,
                         png_u8** out_png,
                         png_u32* out_png_size)
{
    png_encode_options opt_local;
    const png_encode_options* opt;
    png_u32 rowbytes;
    png_u32 src_stride;
    png_u8* raw;
    png_u32 raw_size;
    png_u8* comp;
    png_u32 comp_bound;
    png_u32 comp_len;
    png_write_buffer wb;
    png_u8 ihdr[13];
    int err;
    static const png_u8 sig[8] = { 0x89,'P','N','G',0x0D,0x0A,0x1A,0x0A };

    if (!out_png || !out_png_size)
        return PNG_DEC_ERR_FORMAT;
    *out_png = 0;
    *out_png_size = 0u;

    if (!pixels || !zfunc)
        return PNG_DEC_ERR_FORMAT;

    png_encode_options_init(&opt_local);
    if (options)
        opt_local = *options;
    opt = &opt_local;

    if (width == 0u || height == 0u)
        return PNG_DEC_ERR_FORMAT;
    if (width > PNG_DEC_MAX_WIDTH || height > PNG_DEC_MAX_HEIGHT)
        return PNG_DEC_ERR_DIMENSIONS_TOO_LARGE;
    if (width > 0u && height > 0u && width > (PNG_DEC_MAX_PIXELS / height))
        return PNG_DEC_ERR_TOO_MANY_PIXELS;

    if (!png_validate_color_depth_combo(opt->color_type, opt->bit_depth))
        return PNG_DEC_ERR_UNSUPPORTED;

    if (!(opt->interlace_method == 0u || opt->interlace_method == 1u))
        return PNG_DEC_ERR_UNSUPPORTED;

    if (opt->input_format != PNG_ENC_INPUT_FORMAT_NATIVE && !png_public_format_valid_local(opt->input_format))
        return PNG_DEC_ERR_FORMAT;

    if (opt->write_sRGB && opt->write_iCCP)
        return PNG_DEC_ERR_FORMAT;

    if (opt->write_iCCP &&
        (!png_is_valid_iccp_name(opt->iccp_name) ||
         !opt->iccp_profile ||
         opt->iccp_profile_size == 0u))
        return PNG_DEC_ERR_FORMAT;

    if (opt->write_mDCV && !opt->write_cICP)
        return PNG_DEC_ERR_FORMAT;

    if (opt->write_eXIf &&
        (!opt->exif_profile || opt->exif_profile_size == 0u ||
         !png_exif_has_valid_tiff_header(opt->exif_profile, opt->exif_profile_size)))
        return PNG_DEC_ERR_FORMAT;

    if (opt->dsig_chunk_count != 0u)
    {
        png_u32 dsig_before = 0u;
        png_u32 dsig_after = 0u;
        png_u32 i;
        if (!opt->dsig_chunks)
            return PNG_DEC_ERR_FORMAT;
        for (i = 0u; i < opt->dsig_chunk_count; ++i)
        {
            const png_dsig_entry* ds = &opt->dsig_chunks[i];
            if (!ds->cms_data || ds->size == 0u)
                return PNG_DEC_ERR_FORMAT;
            if (ds->location == PNG_CHUNK_POS_AFTER_IHDR)
                dsig_before += 1u;
            else if (ds->location == PNG_CHUNK_POS_BEFORE_IEND)
                dsig_after += 1u;
            else
                return PNG_DEC_ERR_FORMAT;
        }
        if (dsig_before != dsig_after)
            return PNG_DEC_ERR_FORMAT;
    }

    if (opt->gifg_chunk_count != 0u && !opt->gifg_chunks)
        return PNG_DEC_ERR_FORMAT;
    if (opt->gifx_chunk_count != 0u && !opt->gifx_chunks)
        return PNG_DEC_ERR_FORMAT;
    if (opt->gift_chunk_count != 0u && !opt->gift_chunks)
        return PNG_DEC_ERR_FORMAT;
    if (opt->frac_chunk_count != 0u && !opt->frac_chunks)
        return PNG_DEC_ERR_FORMAT;

    if (width > PNG_DEC_MAX_WIDTH || height > PNG_DEC_MAX_HEIGHT)
        return PNG_DEC_ERR_DIMENSIONS_TOO_LARGE;

    if (!png_compute_rowbytes(width, opt->color_type, opt->bit_depth, &rowbytes))
        return PNG_DEC_ERR_UNSUPPORTED;

    if (!png_compute_source_stride(opt, width, opt->color_type, opt->bit_depth, rowbytes, &src_stride))
        return PNG_DEC_ERR_UNSUPPORTED;

    if (opt->input_format != PNG_ENC_INPUT_FORMAT_NATIVE &&
        opt->color_type == PNG_COLOR_INDEXED &&
        !png_public_format_is_gray_local(opt->input_format))
        return PNG_DEC_ERR_UNSUPPORTED;

    if (opt->color_type == PNG_COLOR_INDEXED)
    {
        if (!opt->palette || opt->palette_entries == 0u || opt->palette_entries > 256u)
            return PNG_DEC_ERR_FORMAT;
        if (opt->palette_entries > (1u << opt->bit_depth))
            return PNG_DEC_ERR_FORMAT;
        if (opt->trns_size > opt->palette_entries)
            return PNG_DEC_ERR_FORMAT;
        if (opt->write_bKGD && opt->bkgd_palette_index >= opt->palette_entries)
            return PNG_DEC_ERR_FORMAT;
    }
    else if ((opt->color_type == PNG_COLOR_TRUECOLOR || opt->color_type == PNG_COLOR_TRUECOLOR_ALPHA) &&
             opt->palette_entries != 0u)
    {
        if (!opt->palette || opt->palette_entries > 256u)
            return PNG_DEC_ERR_FORMAT;
    }
    else if (opt->palette_entries != 0u)
    {
        return PNG_DEC_ERR_FORMAT;
    }
    else if (opt->color_type == PNG_COLOR_GRAYSCALE)
    {
        if (opt->trns_size != 0u && opt->trns_size != 2u)
            return PNG_DEC_ERR_FORMAT;
    }
    else if (opt->color_type == PNG_COLOR_TRUECOLOR)
    {
        if (opt->trns_size != 0u && opt->trns_size != 6u)
            return PNG_DEC_ERR_FORMAT;
    }
    else if (opt->trns_size != 0u)
    {
        return PNG_DEC_ERR_FORMAT;
    }

    if (opt->hist_entries && opt->hist_count != 0u)
    {
        if (!png_should_write_plte(opt) || opt->hist_count != opt->palette_entries)
            return PNG_DEC_ERR_FORMAT;
    }

    if (opt->write_oFFs && !(opt->offset_unit == 0u || opt->offset_unit == 1u))
        return PNG_DEC_ERR_FORMAT;

    if (opt->write_sCAL)
    {
        if (!(opt->scal_unit == 1u || opt->scal_unit == 2u) ||
            !png_parse_positive_fixed_string_enc(opt->scal_pixel_width) ||
            !png_parse_positive_fixed_string_enc(opt->scal_pixel_height))
            return PNG_DEC_ERR_FORMAT;
    }

    if (opt->write_pCAL)
    {
        int expected_params = png_pcal_expected_param_count(opt->pcal.equation_type);
        png_u32 pj;
        if (!png_validate_latin1_name_enc(opt->pcal.name) ||
            !png_validate_latin1_text_enc(opt->pcal.unit_name ? opt->pcal.unit_name : "", 1) ||
            expected_params < 0 ||
            opt->pcal.param_count != (png_u32)expected_params ||
            !opt->pcal.params ||
            opt->pcal.x0 == opt->pcal.x1)
            return PNG_DEC_ERR_FORMAT;
        for (pj = 0u; pj < opt->pcal.param_count; ++pj)
        {
            if (!opt->pcal.params[pj] || !png_parse_fixed_string_enc(opt->pcal.params[pj], 0))
                return PNG_DEC_ERR_FORMAT;
        }
    }

    if (opt->write_sTER && !(opt->ster_mode == 0u || opt->ster_mode == 1u))
        return PNG_DEC_ERR_FORMAT;

    if (opt->splt_palette_count != 0u)
    {
        png_u32 si, sj;
        if (!opt->splt_palettes)
            return PNG_DEC_ERR_FORMAT;
        for (si = 0u; si < opt->splt_palette_count; ++si)
        {
            const png_splt_palette* sp = &opt->splt_palettes[si];
            if (!sp->name || !png_validate_latin1_name_enc(sp->name) ||
                !(sp->sample_depth == 8u || sp->sample_depth == 16u) ||
                !sp->entries || sp->entry_count == 0u)
                return PNG_DEC_ERR_FORMAT;
            for (sj = si + 1u; sj < opt->splt_palette_count; ++sj)
            {
                if (opt->splt_palettes[sj].name && strcmp(sp->name, opt->splt_palettes[sj].name) == 0)
                    return PNG_DEC_ERR_FORMAT;
            }
        }
    }

    raw = 0;
    raw_size = 0u;
    err = png_build_filtered_data(pixels, width, height, opt, src_stride, &raw, &raw_size);
    if (err != PNG_DEC_OK)
        return err;

    if (!deflate_bound_u32(raw_size, &comp_bound)) {
        png_mem89_release(raw);
        return PNG_DEC_ERR_UNSUPPORTED;
    }

    comp = (png_u8*)png_mem89_alloc(comp_bound == 0u ? 1u : comp_bound);
    if (!comp) {
        png_mem89_release(raw);
        return PNG_DEC_ERR_OOM;
    }

    comp_len = comp_bound;
    if (zfunc(comp, &comp_len, raw, raw_size, opt->zlevel) != 0) {
        png_mem89_release(raw);
        png_mem89_release(comp);
        return PNG_DEC_ERR_ZLIB;
    }
    png_mem89_release(raw);

    memset(&wb, 0, sizeof(wb));
    err = wb_append(&wb, sig, 8u);
    if (err != PNG_DEC_OK) goto fail;

    write_be32(ihdr + 0, width);
    write_be32(ihdr + 4, height);
    ihdr[8] = opt->bit_depth;
    ihdr[9] = opt->color_type;
    ihdr[10] = 0u;
    ihdr[11] = 0u;
    ihdr[12] = opt->interlace_method;

    err = write_chunk_buffer(&wb, (const png_u8*)"IHDR", ihdr, 13u);
    if (err != PNG_DEC_OK) goto fail;

    err = png_append_dsig_chunks(&wb, opt, PNG_CHUNK_POS_AFTER_IHDR);
    if (err != PNG_DEC_OK) goto fail;

    err = png_append_legacy_extension_chunks(&wb, opt, PNG_CHUNK_POS_AFTER_IHDR);
    if (err != PNG_DEC_OK) goto fail;

    err = png_append_standard_chunks(&wb, zfunc, opt, 1u);
    if (err != PNG_DEC_OK) goto fail;

    if (png_should_write_plte(opt))
    {
        err = write_chunk_buffer(&wb, (const png_u8*)"PLTE",
                                 opt->palette,
                                 opt->palette_entries * 3u);
        if (err != PNG_DEC_OK) goto fail;
    }

    err = png_append_standard_chunks(&wb, zfunc, opt, 0u);
    if (err != PNG_DEC_OK) goto fail;

    err = png_append_legacy_extension_chunks(&wb, opt, PNG_CHUNK_POS_AFTER_PLTE);
    if (err != PNG_DEC_OK) goto fail;

    if (opt->trns_size != 0u)
    {
        err = write_chunk_buffer(&wb, (const png_u8*)"tRNS", opt->trns_data, opt->trns_size);
        if (err != PNG_DEC_OK) goto fail;
    }

    err = write_chunk_buffer(&wb, (const png_u8*)"IDAT", comp, comp_len);
    if (err != PNG_DEC_OK) goto fail;

    err = png_append_legacy_extension_chunks(&wb, opt, PNG_CHUNK_POS_AFTER_IDAT);
    if (err != PNG_DEC_OK) goto fail;

    err = png_append_after_idat_chunks(&wb, opt);
    if (err != PNG_DEC_OK) goto fail;

    err = png_append_dsig_chunks(&wb, opt, PNG_CHUNK_POS_BEFORE_IEND);
    if (err != PNG_DEC_OK) goto fail;

    err = write_chunk_buffer(&wb, (const png_u8*)"IEND", 0, 0u);
    if (err != PNG_DEC_OK) goto fail;

    png_mem89_release(comp);
    *out_png = wb.data;
    *out_png_size = wb.size;
    return PNG_DEC_OK;

fail:
    png_mem89_release(comp);
    if (wb.data) png_mem89_release(wb.data);
    return err;
}

int png_encode_rgba8_memory(const png_u8* rgba,
                            png_u32 width,
                            png_u32 height,
                            png_zlib_compress_func zfunc,
                            int zlevel,
                            png_u8** out_png,
                            png_u32* out_png_size)
{
    png_encode_options opt;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.zlevel = zlevel;
    opt.filter_strategy = PNG_ENC_FILTER_ADAPTIVE;

    return png_encode_memory_ex(rgba, width, height, zfunc, &opt, out_png, out_png_size);
}


int png_encode_image_ex(const png_image* image,
                        png_zlib_compress_func zfunc,
                        const png_encode_options* options,
                        png_u8** out_png,
                        png_u32* out_png_size)
{
    png_encode_options opt_local;

    if (!image || !image->pixels)
        return PNG_DEC_ERR_FORMAT;

    png_encode_options_init(&opt_local);
    if (options)
        opt_local = *options;
    opt_local.stride_bytes = image->pixel_rowbytes;
    opt_local.input_format = image->output_format;
    if (image->output_sample_depth == 16u &&
        (image->transform_flags_applied & PNG_DEC_TRANSFORM_SWAP_16_ENDIAN) != 0u)
        opt_local.input_16bit_little_endian = 1u;

    return png_encode_memory_ex(image->pixels,
                                image->width,
                                image->height,
                                zfunc,
                                &opt_local,
                                out_png,
                                out_png_size);
}
