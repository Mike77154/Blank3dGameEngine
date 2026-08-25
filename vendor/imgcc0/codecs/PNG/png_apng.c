#include "png_mem89.h"
#include "png_decoder_internal.h"

#include <stdio.h>
#include <string.h>

typedef struct png_raw_chunk_blob_s {
    png_u8* bytes; /* length+type+data+crc */
    png_u32 size;
    png_u32 type;
} png_raw_chunk_blob;

typedef struct png_static_png_parts_s {
    png_u32 width;
    png_u32 height;
    png_u8  bit_depth;
    png_u8  color_type;
    png_u8  compression_method;
    png_u8  filter_method;
    png_u8  interlace_method;
    png_raw_chunk_blob* pre_chunks;
    png_u32 pre_chunk_count;
    png_u32 pre_chunk_capacity;
    png_raw_chunk_blob* post_chunks;
    png_u32 post_chunk_count;
    png_u32 post_chunk_capacity;
    png_u8* idat_data;
    png_u32 idat_size;
    png_u32 idat_capacity;
} png_static_png_parts;

typedef struct png_apng_frame_build_s {
    png_apng_frame_control control;
    png_u8* compressed;
    png_u32 compressed_size;
    png_u32 compressed_capacity;
} png_apng_frame_build;

typedef struct png_apng_parse_s {
    png_u32 width;
    png_u32 height;
    png_u8  bit_depth;
    png_u8  color_type;
    png_u8  compression_method;
    png_u8  filter_method;
    png_u8  interlace_method;
    png_u32 num_frames_declared;
    png_u32 num_plays;
    int     has_actl;
    int     default_image_is_first_frame;
    png_u32 next_sequence_number;
    png_raw_chunk_blob* pre_chunks;
    png_u32 pre_chunk_count;
    png_u32 pre_chunk_capacity;
    png_apng_frame_build* frames;
    png_u32 frame_count;
    png_u32 frame_capacity;
    png_u32 current_frame_index;
    png_u32 max_frames;
    png_u32 chunk_count;
    png_u32 max_chunks;
    png_u32 max_chunk_bytes;
} png_apng_parse;

static const png_u8 PNG_SIG_BYTES[8] = { 137u, 80u, 78u, 71u, 13u, 10u, 26u, 10u };

static png_u32 read_be32_apng(const png_u8* p)
{
    return ((png_u32)p[0] << 24) |
           ((png_u32)p[1] << 16) |
           ((png_u32)p[2] << 8) |
           (png_u32)p[3];
}

static png_u16 read_be16_apng(const png_u8* p)
{
    return (png_u16)(((png_u16)p[0] << 8) | (png_u16)p[1]);
}

static void write_be32_apng(png_u8* p, png_u32 v)
{
    p[0] = (png_u8)(v >> 24);
    p[1] = (png_u8)(v >> 16);
    p[2] = (png_u8)(v >> 8);
    p[3] = (png_u8)(v & 255u);
}

static void write_be16_apng(png_u8* p, png_u16 v)
{
    p[0] = (png_u8)(v >> 8);
    p[1] = (png_u8)(v & 255u);
}

static int safe_add_u32_apng(png_u32 a, png_u32 b, png_u32* out)
{
    if (a > 0xFFFFFFFFu - b)
        return 0;
    *out = a + b;
    return 1;
}

static int safe_mul_u32_apng(png_u32 a, png_u32 b, png_u32* out)
{
    if (a != 0u && b > 0xFFFFFFFFu / a)
        return 0;
    *out = a * b;
    return 1;
}

static int chunk_is_type(png_u32 type, char a, char b, char c, char d)
{
    return type == (((png_u32)(png_u8)a << 24) |
                    ((png_u32)(png_u8)b << 16) |
                    ((png_u32)(png_u8)c << 8) |
                    (png_u32)(png_u8)d);
}

static int buffer_append(png_u8** buf, png_u32* size, png_u32* cap,
                         const png_u8* data, png_u32 data_size)
{
    png_u32 need;
    png_u32 new_cap;
    png_u8* new_buf;

    if (data_size == 0u)
        return PNG_DEC_OK;
    if (!buf || !size || !cap || (!data && data_size != 0u))
        return PNG_DEC_ERR_FORMAT;
    if (!safe_add_u32_apng(*size, data_size, &need))
        return PNG_DEC_ERR_UNSUPPORTED;
    if (need > *cap)
    {
        new_cap = (*cap == 0u) ? 256u : *cap;
        while (new_cap < need)
        {
            if (new_cap > 0x7FFFFFFFu)
                return PNG_DEC_ERR_UNSUPPORTED;
            new_cap *= 2u;
        }
        new_buf = (png_u8*)png_mem89_resize(*buf, new_cap);
        if (!new_buf)
            return PNG_DEC_ERR_OOM;
        *buf = new_buf;
        *cap = new_cap;
    }
    memcpy(*buf + *size, data, data_size);
    *size = need;
    return PNG_DEC_OK;
}

static int raw_chunk_array_push(png_raw_chunk_blob** arr,
                                png_u32* count,
                                png_u32* cap,
                                const png_u8* data,
                                png_u32 size,
                                png_u32 type)
{
    png_u32 new_cap;
    png_raw_chunk_blob* new_arr;
    png_raw_chunk_blob* dst;

    if (!arr || !count || !cap || (!data && size != 0u))
        return PNG_DEC_ERR_FORMAT;

    if (*count == *cap)
    {
        new_cap = (*cap == 0u) ? 8u : (*cap * 2u);
        new_arr = (png_raw_chunk_blob*)png_mem89_resize(*arr, new_cap * sizeof(png_raw_chunk_blob));
        if (!new_arr)
            return PNG_DEC_ERR_OOM;
        *arr = new_arr;
        *cap = new_cap;
    }

    dst = &(*arr)[*count];
    memset(dst, 0, sizeof(*dst));
    dst->bytes = (png_u8*)png_mem89_alloc(size == 0u ? 1u : size);
    if (!dst->bytes)
        return PNG_DEC_ERR_OOM;
    memcpy(dst->bytes, data, size);
    dst->size = size;
    dst->type = type;
    *count += 1u;
    return PNG_DEC_OK;
}

static void raw_chunk_array_free(png_raw_chunk_blob* arr, png_u32 count)
{
    png_u32 i;
    if (!arr)
        return;
    for (i = 0u; i < count; ++i)
        png_mem89_release(arr[i].bytes);
    png_mem89_release(arr);
}

static void static_png_parts_free(png_static_png_parts* parts)
{
    if (!parts)
        return;
    raw_chunk_array_free(parts->pre_chunks, parts->pre_chunk_count);
    raw_chunk_array_free(parts->post_chunks, parts->post_chunk_count);
    png_mem89_release(parts->idat_data);
    memset(parts, 0, sizeof(*parts));
}

static void apng_parse_free(png_apng_parse* apng)
{
    png_u32 i;
    if (!apng)
        return;
    raw_chunk_array_free(apng->pre_chunks, apng->pre_chunk_count);
    if (apng->frames)
    {
        for (i = 0u; i < apng->frame_count; ++i)
            png_mem89_release(apng->frames[i].compressed);
        png_mem89_release(apng->frames);
    }
    memset(apng, 0, sizeof(*apng));
}

static int validate_chunk_crc(const png_u8* type_and_data, png_u32 length, const png_u8* crc_bytes)
{
    png_u32 crc = png_crc32_start();
    png_u32 stored;
    crc = png_crc32_update(crc, type_and_data, 4u + length);
    crc = png_crc32_finish(crc);
    stored = read_be32_apng(crc_bytes);
    return (crc == stored) ? PNG_DEC_OK : PNG_DEC_ERR_CRC;
}

static int parse_static_png_parts(const png_u8* data, png_u32 size, png_static_png_parts* out)
{
    png_u32 off;
    int seen_ihdr;
    int seen_idat;
    int seen_iend;

    if (!data || !out || size < 8u)
        return PNG_DEC_ERR_FORMAT;
    memset(out, 0, sizeof(*out));
    if (memcmp(data, PNG_SIG_BYTES, 8u) != 0)
        return PNG_DEC_ERR_SIG;

    off = 8u;
    seen_ihdr = 0;
    seen_idat = 0;
    seen_iend = 0;

    while (off + 12u <= size)
    {
        png_u32 length = read_be32_apng(data + off);
        png_u32 type = read_be32_apng(data + off + 4u);
        png_u32 chunk_total;
        int err;

        if (length > 0x7FFFFFFFu)
        {
            static_png_parts_free(out);
            return PNG_DEC_ERR_UNSUPPORTED;
        }
        if (!safe_add_u32_apng(12u, length, &chunk_total) || off + chunk_total > size)
        {
            static_png_parts_free(out);
            return PNG_DEC_ERR_FORMAT;
        }
        err = validate_chunk_crc(data + off + 4u, length, data + off + 8u + length);
        if (err != PNG_DEC_OK)
        {
            static_png_parts_free(out);
            return err;
        }

        if (chunk_is_type(type, 'I', 'H', 'D', 'R'))
        {
            if (seen_ihdr || length != 13u)
            {
                static_png_parts_free(out);
                return PNG_DEC_ERR_FORMAT;
            }
            out->width = read_be32_apng(data + off + 8u);
            out->height = read_be32_apng(data + off + 12u);
            out->bit_depth = data[off + 16u];
            out->color_type = data[off + 17u];
            out->compression_method = data[off + 18u];
            out->filter_method = data[off + 19u];
            out->interlace_method = data[off + 20u];
            seen_ihdr = 1;
        }
        else if (chunk_is_type(type, 'I', 'D', 'A', 'T'))
        {
            err = buffer_append(&out->idat_data, &out->idat_size, &out->idat_capacity,
                                data + off + 8u, length);
            if (err != PNG_DEC_OK)
            {
                static_png_parts_free(out);
                return err;
            }
            seen_idat = 1;
        }
        else if (chunk_is_type(type, 'I', 'E', 'N', 'D'))
        {
            seen_iend = 1;
            off += chunk_total;
            break;
        }
        else if (!seen_idat)
        {
            err = raw_chunk_array_push(&out->pre_chunks, &out->pre_chunk_count, &out->pre_chunk_capacity,
                                       data + off, chunk_total, type);
            if (err != PNG_DEC_OK)
            {
                static_png_parts_free(out);
                return err;
            }
        }
        else
        {
            err = raw_chunk_array_push(&out->post_chunks, &out->post_chunk_count, &out->post_chunk_capacity,
                                       data + off, chunk_total, type);
            if (err != PNG_DEC_OK)
            {
                static_png_parts_free(out);
                return err;
            }
        }

        off += chunk_total;
    }

    if (!seen_ihdr || !seen_iend)
    {
        static_png_parts_free(out);
        return PNG_DEC_ERR_FORMAT;
    }

    return PNG_DEC_OK;
}

static int apng_push_frame(png_apng_parse* apng, const png_apng_frame_control* ctl)
{
    png_u32 new_cap;
    png_apng_frame_build* new_arr;
    png_apng_frame_build* dst;

    if (!apng || !ctl)
        return PNG_DEC_ERR_FORMAT;
    if (apng->frame_count >= apng->max_frames)
        return PNG_DEC_ERR_FRAME_LIMIT;
    if (apng->frame_count == apng->frame_capacity)
    {
        new_cap = (apng->frame_capacity == 0u) ? 4u : (apng->frame_capacity * 2u);
        new_arr = (png_apng_frame_build*)png_mem89_resize(apng->frames, new_cap * sizeof(png_apng_frame_build));
        if (!new_arr)
            return PNG_DEC_ERR_OOM;
        apng->frames = new_arr;
        apng->frame_capacity = new_cap;
    }
    dst = &apng->frames[apng->frame_count];
    memset(dst, 0, sizeof(*dst));
    dst->control = *ctl;
    apng->current_frame_index = apng->frame_count;
    apng->frame_count += 1u;
    return PNG_DEC_OK;
}

static int apng_append_current_data(png_apng_parse* apng, const png_u8* data, png_u32 size)
{
    png_apng_frame_build* frame;
    if (!apng || apng->current_frame_index >= apng->frame_count)
        return PNG_DEC_ERR_FORMAT;
    frame = &apng->frames[apng->current_frame_index];
    return buffer_append(&frame->compressed, &frame->compressed_size, &frame->compressed_capacity, data, size);
}

static int parse_apng(const png_u8* data, png_u32 size, const png_decode_options* options, png_apng_parse* out)
{
    png_u32 off;
    int seen_ihdr;
    int seen_idat;
    int seen_iend;
    png_decode_options dopt;
    png_u32 max_width;
    png_u32 max_height;
    png_u32 max_pixels;
    png_u32 max_file_bytes;

    if (!data || !out || size < 8u)
        return PNG_DEC_ERR_FORMAT;

    if (options)
        dopt = *options;
    else
        png_decode_options_init(&dopt);

    max_width = dopt.max_width ? dopt.max_width : PNG_DEC_MAX_WIDTH;
    max_height = dopt.max_height ? dopt.max_height : PNG_DEC_MAX_HEIGHT;
    max_pixels = dopt.max_pixels ? dopt.max_pixels : PNG_DEC_MAX_PIXELS;
    max_file_bytes = dopt.max_file_bytes ? dopt.max_file_bytes : PNG_DEC_MAX_FILE_BYTES;

    if (size > max_file_bytes)
        return PNG_DEC_ERR_CHUNK_TOO_LARGE;

    memset(out, 0, sizeof(*out));
    out->current_frame_index = 0xFFFFFFFFu;
    out->max_frames = dopt.max_apng_frames ? dopt.max_apng_frames : PNG_DEC_MAX_APNG_FRAMES;
    out->max_chunks = dopt.max_chunks ? dopt.max_chunks : PNG_DEC_MAX_CHUNKS;
    out->max_chunk_bytes = dopt.max_chunk_bytes ? dopt.max_chunk_bytes : (1024u * 1024u * 16u);
    if (memcmp(data, PNG_SIG_BYTES, 8u) != 0)
        return PNG_DEC_ERR_SIG;

    off = 8u;
    seen_ihdr = 0;
    seen_idat = 0;
    seen_iend = 0;

    while (off + 12u <= size)
    {
        png_u32 length = read_be32_apng(data + off);
        png_u32 type = read_be32_apng(data + off + 4u);
        png_u32 chunk_total;
        int err;

        if (length > 0x7FFFFFFFu)
        {
            apng_parse_free(out);
            return PNG_DEC_ERR_UNSUPPORTED;
        }
        if (length > out->max_chunk_bytes)
        {
            apng_parse_free(out);
            return PNG_DEC_ERR_CHUNK_TOO_LARGE;
        }
        if (out->chunk_count >= out->max_chunks)
        {
            apng_parse_free(out);
            return PNG_DEC_ERR_TOO_MANY_CHUNKS;
        }
        if (!safe_add_u32_apng(12u, length, &chunk_total) || off + chunk_total > size)
        {
            apng_parse_free(out);
            return PNG_DEC_ERR_FORMAT;
        }
        err = validate_chunk_crc(data + off + 4u, length, data + off + 8u + length);
        if (err != PNG_DEC_OK)
        {
            apng_parse_free(out);
            return err;
        }

        out->chunk_count += 1u;

        if (chunk_is_type(type, 'I', 'H', 'D', 'R'))
        {
            if (seen_ihdr || length != 13u)
            {
                apng_parse_free(out);
                return PNG_DEC_ERR_FORMAT;
            }
            out->width = read_be32_apng(data + off + 8u);
            out->height = read_be32_apng(data + off + 12u);
            out->bit_depth = data[off + 16u];
            out->color_type = data[off + 17u];
            out->compression_method = data[off + 18u];
            out->filter_method = data[off + 19u];
            out->interlace_method = data[off + 20u];
            if (out->width == 0u || out->height == 0u ||
                out->width > max_width || out->height > max_height)
            {
                apng_parse_free(out);
                return PNG_DEC_ERR_DIMENSIONS_TOO_LARGE;
            }
            if (out->width > 0u && out->height > 0u && out->width > (max_pixels / out->height))
            {
                apng_parse_free(out);
                return PNG_DEC_ERR_TOO_MANY_PIXELS;
            }
            seen_ihdr = 1;
        }
        else if (chunk_is_type(type, 'a', 'c', 'T', 'L'))
        {
            if (!seen_ihdr || seen_idat || length != 8u || out->has_actl)
            {
                apng_parse_free(out);
                return PNG_DEC_ERR_FORMAT;
            }
            out->num_frames_declared = read_be32_apng(data + off + 8u);
            out->num_plays = read_be32_apng(data + off + 12u);
            if (out->num_frames_declared > out->max_frames)
            {
                apng_parse_free(out);
                return PNG_DEC_ERR_FRAME_LIMIT;
            }
            if (out->num_frames_declared == 0u)
            {
                apng_parse_free(out);
                return PNG_DEC_ERR_FORMAT;
            }
            out->has_actl = 1;
        }
        else if (chunk_is_type(type, 'f', 'c', 'T', 'L'))
        {
            png_apng_frame_control ctl;
            png_u32 seq;
            if (!out->has_actl || length != 26u)
            {
                apng_parse_free(out);
                return PNG_DEC_ERR_FORMAT;
            }
            seq = read_be32_apng(data + off + 8u);
            if (seq != out->next_sequence_number)
            {
                apng_parse_free(out);
                return PNG_DEC_ERR_FORMAT;
            }
            out->next_sequence_number += 1u;
            memset(&ctl, 0, sizeof(ctl));
            ctl.width = read_be32_apng(data + off + 12u);
            ctl.height = read_be32_apng(data + off + 16u);
            ctl.x_offset = read_be32_apng(data + off + 20u);
            ctl.y_offset = read_be32_apng(data + off + 24u);
            ctl.delay_num = read_be16_apng(data + off + 28u);
            ctl.delay_den = read_be16_apng(data + off + 30u);
            ctl.dispose_op = data[off + 32u];
            ctl.blend_op = data[off + 33u];
            if (ctl.width == 0u || ctl.height == 0u ||
                ctl.x_offset + ctl.width > out->width ||
                ctl.y_offset + ctl.height > out->height ||
                ctl.dispose_op > PNG_APNG_DISPOSE_OP_PREVIOUS ||
                ctl.blend_op > PNG_APNG_BLEND_OP_OVER)
            {
                apng_parse_free(out);
                return PNG_DEC_ERR_FORMAT;
            }
            if (out->frame_count == 0u && !seen_idat)
            {
                out->default_image_is_first_frame = 1;
                if (ctl.x_offset != 0u || ctl.y_offset != 0u || ctl.width != out->width || ctl.height != out->height)
                {
                    apng_parse_free(out);
                    return PNG_DEC_ERR_FORMAT;
                }
            }
            err = apng_push_frame(out, &ctl);
            if (err != PNG_DEC_OK)
            {
                apng_parse_free(out);
                return err;
            }
        }
        else if (chunk_is_type(type, 'I', 'D', 'A', 'T'))
        {
            seen_idat = 1;
            if (out->has_actl && out->default_image_is_first_frame)
            {
                if (out->frame_count == 0u || out->current_frame_index != 0u)
                {
                    apng_parse_free(out);
                    return PNG_DEC_ERR_FORMAT;
                }
                err = apng_append_current_data(out, data + off + 8u, length);
                if (err != PNG_DEC_OK)
                {
                    apng_parse_free(out);
                    return err;
                }
            }
        }
        else if (chunk_is_type(type, 'f', 'd', 'A', 'T'))
        {
            png_u32 seq;
            if (!out->has_actl || length < 4u || out->current_frame_index == 0xFFFFFFFFu)
            {
                apng_parse_free(out);
                return PNG_DEC_ERR_FORMAT;
            }
            seq = read_be32_apng(data + off + 8u);
            if (seq != out->next_sequence_number)
            {
                apng_parse_free(out);
                return PNG_DEC_ERR_FORMAT;
            }
            out->next_sequence_number += 1u;
            err = apng_append_current_data(out, data + off + 12u, length - 4u);
            if (err != PNG_DEC_OK)
            {
                apng_parse_free(out);
                return err;
            }
        }
        else if (chunk_is_type(type, 'I', 'E', 'N', 'D'))
        {
            seen_iend = 1;
            off += chunk_total;
            break;
        }
        else if (!seen_idat)
        {
            if (!chunk_is_type(type, 'a', 'c', 'T', 'L') && !chunk_is_type(type, 'f', 'c', 'T', 'L'))
            {
                err = raw_chunk_array_push(&out->pre_chunks, &out->pre_chunk_count, &out->pre_chunk_capacity,
                                           data + off, chunk_total, type);
                if (err != PNG_DEC_OK)
                {
                    apng_parse_free(out);
                    return err;
                }
            }
        }

        off += chunk_total;
    }

    if (!seen_ihdr || !seen_iend || !out->has_actl || out->frame_count != out->num_frames_declared)
    {
        apng_parse_free(out);
        return PNG_DEC_ERR_FORMAT;
    }

    return PNG_DEC_OK;
}

static int append_chunk_bytes(png_u8** out, png_u32* size, png_u32* cap,
                              const png_u8 type_bytes[4], const png_u8* data, png_u32 data_size)
{
    png_u8 header[8];
    png_u8 crc_buf[4];
    png_u32 crc;
    int err;

    write_be32_apng(header, data_size);
    memcpy(header + 4u, type_bytes, 4u);
    err = buffer_append(out, size, cap, header, 8u);
    if (err != PNG_DEC_OK)
        return err;
    err = buffer_append(out, size, cap, data, data_size);
    if (err != PNG_DEC_OK)
        return err;
    crc = png_crc32_start();
    crc = png_crc32_update(crc, type_bytes, 4u);
    if (data_size != 0u)
        crc = png_crc32_update(crc, data, data_size);
    crc = png_crc32_finish(crc);
    write_be32_apng(crc_buf, crc);
    return buffer_append(out, size, cap, crc_buf, 4u);
}

static int build_frame_png(const png_apng_parse* parse,
                           const png_apng_frame_build* frame,
                           png_u8** out_png,
                           png_u32* out_png_size)
{
    png_u8* out;
    png_u32 out_size;
    png_u32 out_cap;
    png_u8 ihdr[13];
    png_u32 i;
    int err;
    static const png_u8 ihdr_type[4] = { 'I', 'H', 'D', 'R' };
    static const png_u8 idat_type[4] = { 'I', 'D', 'A', 'T' };
    static const png_u8 iend_type[4] = { 'I', 'E', 'N', 'D' };

    if (!parse || !frame || !out_png || !out_png_size)
        return PNG_DEC_ERR_FORMAT;

    *out_png = 0;
    *out_png_size = 0u;
    out = 0;
    out_size = 0u;
    out_cap = 0u;

    err = buffer_append(&out, &out_size, &out_cap, PNG_SIG_BYTES, 8u);
    if (err != PNG_DEC_OK)
        goto fail;

    write_be32_apng(ihdr + 0u, frame->control.width);
    write_be32_apng(ihdr + 4u, frame->control.height);
    ihdr[8] = parse->bit_depth;
    ihdr[9] = parse->color_type;
    ihdr[10] = parse->compression_method;
    ihdr[11] = parse->filter_method;
    ihdr[12] = parse->interlace_method;
    err = append_chunk_bytes(&out, &out_size, &out_cap, ihdr_type, ihdr, 13u);
    if (err != PNG_DEC_OK)
        goto fail;

    for (i = 0u; i < parse->pre_chunk_count; ++i)
    {
        err = buffer_append(&out, &out_size, &out_cap, parse->pre_chunks[i].bytes, parse->pre_chunks[i].size);
        if (err != PNG_DEC_OK)
            goto fail;
    }

    err = append_chunk_bytes(&out, &out_size, &out_cap, idat_type, frame->compressed, frame->compressed_size);
    if (err != PNG_DEC_OK)
        goto fail;
    err = append_chunk_bytes(&out, &out_size, &out_cap, iend_type, 0, 0u);
    if (err != PNG_DEC_OK)
        goto fail;

    *out_png = out;
    *out_png_size = out_size;
    return PNG_DEC_OK;

fail:
    png_mem89_release(out);
    return err;
}

static void apply_post_transforms_rgba8_apng(png_u8* p, png_u32 count, png_u32 flags)
{
    png_u32 i;
    for (i = 0u; i < count; ++i)
    {
        png_u8 r = p[0];
        png_u8 g = p[1];
        png_u8 b = p[2];
        png_u8 a = p[3];
        if (flags & PNG_DEC_TRANSFORM_PREMULTIPLY_ALPHA)
        {
            r = (png_u8)(((png_u32)r * (png_u32)a + 127u) / 255u);
            g = (png_u8)(((png_u32)g * (png_u32)a + 127u) / 255u);
            b = (png_u8)(((png_u32)b * (png_u32)a + 127u) / 255u);
        }
        if (flags & PNG_DEC_TRANSFORM_SWAP_RB)
        {
            png_u8 t = r; r = b; b = t;
        }
        if (flags & PNG_DEC_TRANSFORM_INVERT_ALPHA)
            a = (png_u8)(255u - a);
        if (flags & PNG_DEC_TRANSFORM_STRIP_ALPHA)
            a = 255u;
        p[0] = r; p[1] = g; p[2] = b; p[3] = a;
        p += 4;
    }
}

static void apply_post_transforms_rgba16_apng(png_u8* p, png_u32 count, png_u32 flags)
{
    png_u32 i;
    for (i = 0u; i < count; ++i)
    {
        png_u16 r = read_be16_apng(p + 0u);
        png_u16 g = read_be16_apng(p + 2u);
        png_u16 b = read_be16_apng(p + 4u);
        png_u16 a = read_be16_apng(p + 6u);
        if (flags & PNG_DEC_TRANSFORM_PREMULTIPLY_ALPHA)
        {
            r = (png_u16)(((png_u32)r * (png_u32)a + 32767u) / 65535u);
            g = (png_u16)(((png_u32)g * (png_u32)a + 32767u) / 65535u);
            b = (png_u16)(((png_u32)b * (png_u32)a + 32767u) / 65535u);
        }
        if (flags & PNG_DEC_TRANSFORM_SWAP_RB)
        {
            png_u16 t = r; r = b; b = t;
        }
        if (flags & PNG_DEC_TRANSFORM_INVERT_ALPHA)
            a = (png_u16)(65535u - a);
        if (flags & PNG_DEC_TRANSFORM_STRIP_ALPHA)
            a = 65535u;
        write_be16_apng(p + 0u, r);
        write_be16_apng(p + 2u, g);
        write_be16_apng(p + 4u, b);
        write_be16_apng(p + 6u, a);
        p += 8;
    }
}

static void clear_region_rgba8(png_u8* canvas, png_u32 canvas_rowbytes,
                               png_u32 x, png_u32 y, png_u32 w, png_u32 h)
{
    png_u32 row;
    for (row = 0u; row < h; ++row)
        memset(canvas + (y + row) * canvas_rowbytes + x * 4u, 0, (w * 4u));
}

static void clear_region_rgba16(png_u8* canvas, png_u32 canvas_rowbytes,
                                png_u32 x, png_u32 y, png_u32 w, png_u32 h)
{
    png_u32 row;
    for (row = 0u; row < h; ++row)
        memset(canvas + (y + row) * canvas_rowbytes + x * 8u, 0, (w * 8u));
}

static void blend_over_rgba8_pixel(png_u8* dst, const png_u8* src)
{
    png_u32 sa = src[3];
    png_u32 da = dst[3];
    png_u32 out_a = sa + (da * (255u - sa) + 127u) / 255u;
    png_u32 i;
    if (out_a == 0u)
    {
        dst[0] = dst[1] = dst[2] = dst[3] = 0u;
        return;
    }
    for (i = 0u; i < 3u; ++i)
    {
        png_u32 src_p = (png_u32)src[i] * sa;
        png_u32 dst_p = ((png_u32)dst[i] * da * (255u - sa) + 127u) / 255u;
        dst[i] = (png_u8)((src_p + dst_p + out_a / 2u) / out_a);
    }
    dst[3] = (png_u8)out_a;
}

static void blend_over_rgba16_pixel(png_u8* dst, const png_u8* src)
{
    png_u32 sa = read_be16_apng(src + 6u);
    png_u32 da = read_be16_apng(dst + 6u);
    png_u32 out_a = sa + (da * (65535u - sa) + 32767u) / 65535u;
    png_u32 src_c, dst_c;
    png_u16 out_c;
    if (out_a == 0u)
    {
        memset(dst, 0, 8u);
        return;
    }
    src_c = (png_u32)read_be16_apng(src + 0u) * sa;
    dst_c = ((png_u32)read_be16_apng(dst + 0u) * da * (65535u - sa) + 32767u) / 65535u;
    out_c = (png_u16)((src_c + dst_c + out_a / 2u) / out_a);
    write_be16_apng(dst + 0u, out_c);
    src_c = (png_u32)read_be16_apng(src + 2u) * sa;
    dst_c = ((png_u32)read_be16_apng(dst + 2u) * da * (65535u - sa) + 32767u) / 65535u;
    out_c = (png_u16)((src_c + dst_c + out_a / 2u) / out_a);
    write_be16_apng(dst + 2u, out_c);
    src_c = (png_u32)read_be16_apng(src + 4u) * sa;
    dst_c = ((png_u32)read_be16_apng(dst + 4u) * da * (65535u - sa) + 32767u) / 65535u;
    out_c = (png_u16)((src_c + dst_c + out_a / 2u) / out_a);
    write_be16_apng(dst + 4u, out_c);
    write_be16_apng(dst + 6u, (png_u16)out_a);
}

static void composite_frame_rgba8(png_u8* canvas, png_u32 canvas_rowbytes,
                                  const png_apng_frame_control* ctl,
                                  const png_u8* frame, png_u32 frame_rowbytes)
{
    png_u32 row, col;
    for (row = 0u; row < ctl->height; ++row)
    {
        png_u8* dst = canvas + (ctl->y_offset + row) * canvas_rowbytes + ctl->x_offset * 4u;
        const png_u8* src = frame + row * frame_rowbytes;
        if (ctl->blend_op == PNG_APNG_BLEND_OP_SOURCE)
            memcpy(dst, src, (ctl->width * 4u));
        else
            for (col = 0u; col < ctl->width; ++col)
                blend_over_rgba8_pixel(dst + col * 4u, src + col * 4u);
    }
}

static void composite_frame_rgba16(png_u8* canvas, png_u32 canvas_rowbytes,
                                   const png_apng_frame_control* ctl,
                                   const png_u8* frame, png_u32 frame_rowbytes)
{
    png_u32 row, col;
    for (row = 0u; row < ctl->height; ++row)
    {
        png_u8* dst = canvas + (ctl->y_offset + row) * canvas_rowbytes + ctl->x_offset * 8u;
        const png_u8* src = frame + row * frame_rowbytes;
        if (ctl->blend_op == PNG_APNG_BLEND_OP_SOURCE)
            memcpy(dst, src, (ctl->width * 8u));
        else
            for (col = 0u; col < ctl->width; ++col)
                blend_over_rgba16_pixel(dst + col * 8u, src + col * 8u);
    }
}

static int canvas_to_public_frame(const png_u8* canvas,
                                  png_u32 canvas_rowbytes,
                                  png_u32 width,
                                  png_u32 height,
                                  int internal_16,
                                  png_u32 post_flags,
                                  png_u8 output_format,
                                  png_apng_frame* out_frame)
{
    png_image tmp;
    png_u32 size;
    int err;

    if (!canvas || !out_frame)
        return PNG_DEC_ERR_FORMAT;

    memset(&tmp, 0, sizeof(tmp));
    memset(out_frame, 0, sizeof(*out_frame));
    if (!safe_mul_u32_apng(canvas_rowbytes, height, &size))
        return PNG_DEC_ERR_UNSUPPORTED;
    tmp.pixels = (png_u8*)png_mem89_alloc(size == 0u ? 1u : size);
    if (!tmp.pixels)
        return PNG_DEC_ERR_OOM;
    memcpy(tmp.pixels, canvas, size);
    tmp.width = width;
    tmp.height = height;
    tmp.pixel_rowbytes = canvas_rowbytes;
    tmp.output_format = internal_16 ? PNG_OUTPUT_RGBA16 : PNG_OUTPUT_RGBA8;
    tmp.output_channels = 4u;
    tmp.output_sample_depth = internal_16 ? 16u : 8u;
    tmp.output_bytes_per_channel = internal_16 ? 2u : 1u;
    tmp.rgba = internal_16 ? 0 : tmp.pixels;

    if (internal_16)
        apply_post_transforms_rgba16_apng(tmp.pixels, width * height, post_flags);
    else
        apply_post_transforms_rgba8_apng(tmp.pixels, width * height, post_flags);

    if (output_format != tmp.output_format)
    {
        err = png_image_convert_format(&tmp, output_format);
        if (err != PNG_DEC_OK)
        {
            png_free_image(&tmp);
            return err;
        }
    }

    if ((post_flags & PNG_DEC_TRANSFORM_SWAP_16_ENDIAN) &&
        tmp.output_sample_depth == 16u && tmp.pixels)
    {
        png_u32 swap_size;
        if (!safe_mul_u32_apng(tmp.pixel_rowbytes, tmp.height, &swap_size))
        {
            png_free_image(&tmp);
            return PNG_DEC_ERR_UNSUPPORTED;
        }
        png_swap_16_buffer(tmp.pixels, swap_size);
    }

    out_frame->pixels = tmp.pixels;
    out_frame->pixel_rowbytes = tmp.pixel_rowbytes;
    out_frame->output_format = tmp.output_format;
    out_frame->output_channels = tmp.output_channels;
    out_frame->output_sample_depth = tmp.output_sample_depth;
    out_frame->output_bytes_per_channel = tmp.output_bytes_per_channel;
    tmp.pixels = 0;
    tmp.rgba = 0;
    png_free_image(&tmp);
    return PNG_DEC_OK;
}

static int read_file_into_memory_apng(const char* filename, png_u8** out_buf, png_u32* out_size)
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

void png_free_apng(png_apng* apng)
{
    png_u32 i;
    if (!apng)
        return;
    if (apng->frames)
    {
        for (i = 0u; i < apng->frame_count; ++i)
            png_mem89_release(apng->frames[i].pixels);
        png_mem89_release(apng->frames);
    }
    png_free_image(&apng->default_image);
    memset(apng, 0, sizeof(*apng));
}

int png_decode_apng_memory_ex(const png_u8* data,
                              png_u32 size,
                              png_zlib_decompress_func zfunc,
                              const png_decode_options* options,
                              png_apng* out_apng)
{
    png_apng_parse parse;
    png_decode_options dopt_user;
    png_decode_options dopt_internal;
    png_u8 requested_format;
    png_u32 internal_rowbytes;
    png_u32 canvas_size;
    png_u8* canvas;
    png_u8* restore_region_unused;
    png_u32 restore_rowbytes_unused;
    int internal_16;
    png_u32 i;
    int err;

    if (!data || !out_apng)
        return PNG_DEC_ERR_FORMAT;

    memset(out_apng, 0, sizeof(*out_apng));
    err = parse_apng(data, size, options, &parse);
    if (err != PNG_DEC_OK)
        return err;

    if (options)
        dopt_user = *options;
    else
        png_decode_options_init(&dopt_user);
    requested_format = dopt_user.output_format;
    if (png_output_format_channels(requested_format) == 0u)
        requested_format = PNG_OUTPUT_RGBA8;

    internal_16 = (parse.bit_depth == 16u) ? 1 : 0;
    err = png_output_format_rowbytes(internal_16 ? PNG_OUTPUT_RGBA16 : PNG_OUTPUT_RGBA8,
                                     parse.width, &internal_rowbytes);
    if (err != PNG_DEC_OK)
    {
        apng_parse_free(&parse);
        return err;
    }
    if (!safe_mul_u32_apng(internal_rowbytes, parse.height, &canvas_size))
    {
        apng_parse_free(&parse);
        return PNG_DEC_ERR_TOO_MANY_PIXELS;
    }
    if (dopt_user.max_temp_bytes != 0u && canvas_size > dopt_user.max_temp_bytes)
    {
        apng_parse_free(&parse);
        return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
    }
    canvas = (png_u8*)png_mem89_alloc(canvas_size == 0u ? 1u : canvas_size);
    restore_region_unused = 0;
    restore_rowbytes_unused = internal_16 ? parse.width * 8u : parse.width * 4u;
    (void)restore_region_unused;
    (void)restore_rowbytes_unused;
    if (!canvas)
    {
        apng_parse_free(&parse);
        return PNG_DEC_ERR_OOM;
    }
    memset(canvas, 0, canvas_size);

    out_apng->frames = (png_apng_frame*)png_mem89_alloc_zero(parse.frame_count, sizeof(png_apng_frame));
    if (!out_apng->frames)
    {
        png_mem89_release(canvas);
        apng_parse_free(&parse);
        return PNG_DEC_ERR_OOM;
    }
    out_apng->frame_count = parse.frame_count;
    out_apng->width = parse.width;
    out_apng->height = parse.height;
    out_apng->num_plays = parse.num_plays;

    if (png_decode_memory_ex(data, size, zfunc, &dopt_user, &out_apng->default_image) == PNG_DEC_OK)
        out_apng->has_default_image = 1;

    for (i = 0u; i < parse.frame_count; ++i)
    {
        png_u8* frame_png = 0;
        png_u32 frame_png_size = 0u;
        png_image frame_img;
        png_apng_frame_control ctl = parse.frames[i].control;
        png_u8* saved = 0;
        png_u32 saved_size = 0u;
        png_u32 row;
        png_u32 pixel_bytes;
        png_u32 post_flags;
        png_u8* frame_src;

        memset(&frame_img, 0, sizeof(frame_img));
        err = build_frame_png(&parse, &parse.frames[i], &frame_png, &frame_png_size);
        if (err != PNG_DEC_OK)
        {
            png_free_apng(out_apng);
            png_mem89_release(canvas);
            apng_parse_free(&parse);
            return err;
        }

        png_decode_options_init(&dopt_internal);
        dopt_internal.keep_text = dopt_user.keep_text;
        dopt_internal.keep_unknown_chunks = dopt_user.keep_unknown_chunks;
        dopt_internal.strict_trailing_data = dopt_user.strict_trailing_data;
        dopt_internal.max_width = dopt_user.max_width;
        dopt_internal.max_height = dopt_user.max_height;
        dopt_internal.max_image_bytes = dopt_user.max_image_bytes;
        dopt_internal.max_file_bytes = dopt_user.max_file_bytes;
        dopt_internal.max_text_entries = dopt_user.max_text_entries;
        dopt_internal.max_unknown_chunks = dopt_user.max_unknown_chunks;
        dopt_internal.max_chunk_bytes = dopt_user.max_chunk_bytes;
        dopt_internal.max_pixels = dopt_user.max_pixels;
        dopt_internal.max_inflated_bytes = dopt_user.max_inflated_bytes;
        dopt_internal.max_chunks = dopt_user.max_chunks;
        dopt_internal.max_text_bytes = dopt_user.max_text_bytes;
        dopt_internal.max_apng_frames = dopt_user.max_apng_frames;
        dopt_internal.max_temp_bytes = dopt_user.max_temp_bytes;
        dopt_internal.max_conversion_expansion = dopt_user.max_conversion_expansion;
        dopt_internal.transform_flags = dopt_user.transform_flags & PNG_DEC_TRANSFORM_APPLY_GAMMA;
        dopt_internal.output_format = internal_16 ? PNG_OUTPUT_RGBA16 : PNG_OUTPUT_RGBA8;

        err = png_decode_memory_ex(frame_png, frame_png_size, zfunc, &dopt_internal, &frame_img);
        png_mem89_release(frame_png);
        if (err != PNG_DEC_OK)
        {
            png_free_apng(out_apng);
            png_mem89_release(canvas);
            apng_parse_free(&parse);
            return err;
        }

        if ((i == 0u) && ctl.dispose_op == PNG_APNG_DISPOSE_OP_PREVIOUS)
            ctl.dispose_op = PNG_APNG_DISPOSE_OP_BACKGROUND;

        pixel_bytes = internal_16 ? 8u : 4u;
        if (ctl.dispose_op == PNG_APNG_DISPOSE_OP_PREVIOUS)
        {
            if (!safe_mul_u32_apng(ctl.width * pixel_bytes, ctl.height, &saved_size))
            {
                png_free_image(&frame_img);
                png_free_apng(out_apng);
                png_mem89_release(canvas);
                apng_parse_free(&parse);
                return PNG_DEC_ERR_UNSUPPORTED;
            }
            if (dopt_user.max_temp_bytes != 0u && saved_size > dopt_user.max_temp_bytes)
            {
                png_free_image(&frame_img);
                png_free_apng(out_apng);
                png_mem89_release(canvas);
                apng_parse_free(&parse);
                return PNG_DEC_ERR_TEMP_MEMORY_LIMIT;
            }
            saved = (png_u8*)png_mem89_alloc(saved_size == 0u ? 1u : saved_size);
            if (!saved)
            {
                png_free_image(&frame_img);
                png_free_apng(out_apng);
                png_mem89_release(canvas);
                apng_parse_free(&parse);
                return PNG_DEC_ERR_OOM;
            }
            for (row = 0u; row < ctl.height; ++row)
            {
                memcpy(saved + row * ctl.width * pixel_bytes,
                       canvas + (ctl.y_offset + row) * internal_rowbytes + ctl.x_offset * pixel_bytes,
                       (ctl.width * pixel_bytes));
            }
        }

        frame_src = frame_img.pixels;
        if (internal_16)
            composite_frame_rgba16(canvas, internal_rowbytes, &ctl, frame_src, frame_img.pixel_rowbytes);
        else
            composite_frame_rgba8(canvas, internal_rowbytes, &ctl, frame_src, frame_img.pixel_rowbytes);

        out_apng->frames[i].control = parse.frames[i].control;
        post_flags = dopt_user.transform_flags & ~PNG_DEC_TRANSFORM_APPLY_GAMMA;
        err = canvas_to_public_frame(canvas, internal_rowbytes, parse.width, parse.height,
                                     internal_16, post_flags, requested_format, &out_apng->frames[i]);
        png_free_image(&frame_img);
        if (err != PNG_DEC_OK)
        {
            png_mem89_release(saved);
            png_free_apng(out_apng);
            png_mem89_release(canvas);
            apng_parse_free(&parse);
            return err;
        }

        if (ctl.dispose_op == PNG_APNG_DISPOSE_OP_BACKGROUND)
        {
            if (internal_16)
                clear_region_rgba16(canvas, internal_rowbytes, ctl.x_offset, ctl.y_offset, ctl.width, ctl.height);
            else
                clear_region_rgba8(canvas, internal_rowbytes, ctl.x_offset, ctl.y_offset, ctl.width, ctl.height);
        }
        else if (ctl.dispose_op == PNG_APNG_DISPOSE_OP_PREVIOUS)
        {
            for (row = 0u; row < ctl.height; ++row)
            {
                memcpy(canvas + (ctl.y_offset + row) * internal_rowbytes + ctl.x_offset * pixel_bytes,
                       saved + row * ctl.width * pixel_bytes,
                       (ctl.width * pixel_bytes));
            }
        }
        png_mem89_release(saved);
    }

    out_apng->output_format = out_apng->frames[0].output_format;
    out_apng->output_channels = out_apng->frames[0].output_channels;
    out_apng->output_sample_depth = out_apng->frames[0].output_sample_depth;
    out_apng->output_bytes_per_channel = out_apng->frames[0].output_bytes_per_channel;

    png_mem89_release(canvas);
    apng_parse_free(&parse);
    return PNG_DEC_OK;
}

int png_decode_apng_memory(const png_u8* data,
                           png_u32 size,
                           png_zlib_decompress_func zfunc,
                           png_apng* out_apng)
{
    png_decode_options opt;
    png_decode_options_init(&opt);
    return png_decode_apng_memory_ex(data, size, zfunc, &opt, out_apng);
}

int png_load_apng_file_ex(const char* filename,
                          png_zlib_decompress_func zfunc,
                          const png_decode_options* options,
                          png_apng* out_apng)
{
    png_u8* data;
    png_u32 size;
    int err;
    err = read_file_into_memory_apng(filename, &data, &size);
    if (err != PNG_DEC_OK)
        return err;
    err = png_decode_apng_memory_ex(data, size, zfunc, options, out_apng);
    png_mem89_release(data);
    return err;
}

int png_load_apng_file(const char* filename,
                       png_zlib_decompress_func zfunc,
                       png_apng* out_apng)
{
    png_decode_options opt;
    png_decode_options_init(&opt);
    return png_load_apng_file_ex(filename, zfunc, &opt, out_apng);
}

static int write_fctl_chunk(png_u8** out, png_u32* size, png_u32* cap,
                            png_u32 sequence_number,
                            const png_apng_encode_frame* frame)
{
    png_u8 data[26];
    static const png_u8 type_bytes[4] = { 'f', 'c', 'T', 'L' };
    write_be32_apng(data + 0u, sequence_number);
    write_be32_apng(data + 4u, frame->width);
    write_be32_apng(data + 8u, frame->height);
    write_be32_apng(data + 12u, frame->x_offset);
    write_be32_apng(data + 16u, frame->y_offset);
    write_be16_apng(data + 20u, frame->delay_num);
    write_be16_apng(data + 22u, frame->delay_den);
    data[24] = frame->dispose_op;
    data[25] = frame->blend_op;
    return append_chunk_bytes(out, size, cap, type_bytes, data, 26u);
}

static png_u32 apng_native_channels(png_u8 color_type)
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

static int apng_frame_pixel_bytes(const png_encode_options* options, png_u32* out_pixel_bytes)
{
    png_u32 channels;
    png_u32 bpc;

    if (!options || !out_pixel_bytes)
        return PNG_DEC_ERR_FORMAT;

    if (options->input_format != PNG_ENC_INPUT_FORMAT_NATIVE)
    {
        channels = (png_u32)png_output_format_channels(options->input_format);
        bpc = (png_u32)png_output_format_bytes_per_channel(options->input_format);
        if (channels == 0u || bpc == 0u)
            return PNG_DEC_ERR_FORMAT;
        *out_pixel_bytes = channels * bpc;
        return PNG_DEC_OK;
    }

    if (options->bit_depth != 8u && options->bit_depth != 16u)
        return PNG_DEC_ERR_UNSUPPORTED;

    channels = apng_native_channels(options->color_type);
    if (channels == 0u)
        return PNG_DEC_ERR_FORMAT;
    *out_pixel_bytes = channels * (options->bit_depth == 16u ? 2u : 1u);
    return PNG_DEC_OK;
}

static int apng_frame_default_stride(const png_encode_options* options,
                                     png_u32 width,
                                     png_u32* out_stride)
{
    png_u32 pixel_bytes;

    if (!options || !out_stride)
        return PNG_DEC_ERR_FORMAT;
    if (options->stride_bytes != 0u)
    {
        *out_stride = options->stride_bytes;
        return PNG_DEC_OK;
    }
    if (apng_frame_pixel_bytes(options, &pixel_bytes) != PNG_DEC_OK)
        return PNG_DEC_ERR_UNSUPPORTED;
    if (!safe_mul_u32_apng(width, pixel_bytes, out_stride))
        return PNG_DEC_ERR_UNSUPPORTED;
    return PNG_DEC_OK;
}

static int apng_frame_stride_for_encode(const png_apng_encode_frame* frame,
                                        const png_encode_options* options,
                                        png_u32* out_stride)
{
    if (!frame || !out_stride)
        return PNG_DEC_ERR_FORMAT;
    if (frame->stride_bytes != 0u)
    {
        *out_stride = frame->stride_bytes;
        return PNG_DEC_OK;
    }
    return apng_frame_default_stride(options, frame->width, out_stride);
}

static int apng_find_diff_rect(const png_u8* prev,
                               png_u32 prev_stride,
                               const png_u8* curr,
                               png_u32 curr_stride,
                               png_u32 width,
                               png_u32 height,
                               png_u32 pixel_bytes,
                               png_u32* out_x,
                               png_u32* out_y,
                               png_u32* out_w,
                               png_u32* out_h)
{
    png_u32 x, y;
    png_u32 min_x = width;
    png_u32 min_y = height;
    png_u32 max_x = 0u;
    png_u32 max_y = 0u;
    int found = 0;

    if (!prev || !curr || !out_x || !out_y || !out_w || !out_h || pixel_bytes == 0u)
        return PNG_DEC_ERR_FORMAT;

    for (y = 0u; y < height; ++y)
    {
        const png_u8* prev_row = prev + y * prev_stride;
        const png_u8* curr_row = curr + y * curr_stride;
        if (memcmp(prev_row, curr_row, (width * pixel_bytes)) == 0)
            continue;
        for (x = 0u; x < width; ++x)
        {
            if (memcmp(prev_row + x * pixel_bytes, curr_row + x * pixel_bytes, pixel_bytes) != 0)
            {
                if (!found)
                {
                    min_x = max_x = x;
                    min_y = max_y = y;
                    found = 1;
                }
                else
                {
                    if (x < min_x) min_x = x;
                    if (x > max_x) max_x = x;
                    if (y < min_y) min_y = y;
                    if (y > max_y) max_y = y;
                }
            }
        }
    }

    if (!found)
    {
        *out_x = 0u;
        *out_y = 0u;
        *out_w = 1u;
        *out_h = 1u;
        return PNG_DEC_DONE;
    }

    *out_x = min_x;
    *out_y = min_y;
    *out_w = max_x - min_x + 1u;
    *out_h = max_y - min_y + 1u;
    return PNG_DEC_OK;
}

static int apng_copy_rect(const png_u8* src,
                          png_u32 src_stride,
                          png_u32 x,
                          png_u32 y,
                          png_u32 width,
                          png_u32 height,
                          png_u32 pixel_bytes,
                          png_u8** out_buf,
                          png_u32* out_stride)
{
    png_u32 row;
    png_u32 dst_stride;
    png_u32 size;
    png_u8* buf;

    if (!src || !out_buf || !out_stride || width == 0u || height == 0u || pixel_bytes == 0u)
        return PNG_DEC_ERR_FORMAT;
    if (!safe_mul_u32_apng(width, pixel_bytes, &dst_stride))
        return PNG_DEC_ERR_UNSUPPORTED;
    if (!safe_mul_u32_apng(dst_stride, height, &size))
        return PNG_DEC_ERR_UNSUPPORTED;

    buf = (png_u8*)png_mem89_alloc(size == 0u ? 1u : size);
    if (!buf)
        return PNG_DEC_ERR_OOM;

    for (row = 0u; row < height; ++row)
    {
        memcpy(buf + row * dst_stride,
               src + (y + row) * src_stride + x * pixel_bytes,
               dst_stride);
    }

    *out_buf = buf;
    *out_stride = dst_stride;
    return PNG_DEC_OK;
}

int png_encode_apng_memory_ex(const png_apng_encode_frame* frames,
                              png_u32 frame_count,
                              png_u32 canvas_width,
                              png_u32 canvas_height,
                              png_u32 num_plays,
                              png_zlib_compress_func zfunc,
                              const png_encode_options* options,
                              png_u8** out_png,
                              png_u32* out_png_size)
{
    png_encode_options opt_local;
    png_static_png_parts first_parts;
    png_u8* first_png;
    png_u32 first_png_size;
    png_u8* out;
    png_u32 out_size;
    png_u32 out_cap;
    png_u8 actl[8];
    png_u32 seq;
    png_u32 i;
    int err;
    static const png_u8 actl_type[4] = { 'a', 'c', 'T', 'L' };
    static const png_u8 fdAT_type[4] = { 'f', 'd', 'A', 'T' };
    static const png_u8 iend_type[4] = { 'I', 'E', 'N', 'D' };

    if (!frames || !out_png || !out_png_size || !zfunc || frame_count == 0u)
        return PNG_DEC_ERR_FORMAT;
    if (frames[0].width != canvas_width || frames[0].height != canvas_height ||
        frames[0].x_offset != 0u || frames[0].y_offset != 0u)
        return PNG_DEC_ERR_UNSUPPORTED;

    memset(&first_parts, 0, sizeof(first_parts));
    first_png = 0;
    first_png_size = 0u;
    out = 0;
    out_size = 0u;
    out_cap = 0u;

    if (options)
        opt_local = *options;
    else
        png_encode_options_init(&opt_local);

    if (opt_local.max_apng_frames != 0u && frame_count > opt_local.max_apng_frames)
        return PNG_DEC_ERR_FRAME_LIMIT;

    {
        png_encode_options frame_opt = opt_local;
        if (frames[0].stride_bytes != 0u)
            frame_opt.stride_bytes = frames[0].stride_bytes;
        err = png_encode_memory_ex(frames[0].pixels, frames[0].width, frames[0].height,
                                   zfunc, &frame_opt, &first_png, &first_png_size);
    }
    if (err != PNG_DEC_OK)
        return err;

    err = parse_static_png_parts(first_png, first_png_size, &first_parts);
    if (err != PNG_DEC_OK)
    {
        png_mem89_release(first_png);
        return err;
    }

    if (memcmp(first_png, PNG_SIG_BYTES, 8u) != 0)
    {
        png_mem89_release(first_png);
        static_png_parts_free(&first_parts);
        return PNG_DEC_ERR_FORMAT;
    }

    err = buffer_append(&out, &out_size, &out_cap, PNG_SIG_BYTES, 8u);
    if (err != PNG_DEC_OK)
        goto fail;
    /* Copy IHDR from the first encoded PNG. */
    err = buffer_append(&out, &out_size, &out_cap, first_png + 8u, 25u);
    if (err != PNG_DEC_OK)
        goto fail;

    write_be32_apng(actl + 0u, frame_count);
    write_be32_apng(actl + 4u, num_plays);
    err = append_chunk_bytes(&out, &out_size, &out_cap, actl_type, actl, 8u);
    if (err != PNG_DEC_OK)
        goto fail;

    seq = 0u;
    err = write_fctl_chunk(&out, &out_size, &out_cap, seq++, &frames[0]);
    if (err != PNG_DEC_OK)
        goto fail;

    for (i = 0u; i < first_parts.pre_chunk_count; ++i)
    {
        err = buffer_append(&out, &out_size, &out_cap,
                            first_parts.pre_chunks[i].bytes, first_parts.pre_chunks[i].size);
        if (err != PNG_DEC_OK)
            goto fail;
    }

    err = append_chunk_bytes(&out, &out_size, &out_cap,
                             (const png_u8*)"IDAT", first_parts.idat_data, first_parts.idat_size);
    if (err != PNG_DEC_OK)
        goto fail;

    for (i = 1u; i < frame_count; ++i)
    {
        png_u8* frame_png = 0;
        png_u32 frame_png_size = 0u;
        png_static_png_parts frame_parts;
        png_u8* fdat_buf = 0;
        png_u32 fdat_size = 0u;
        png_u32 fdat_cap = 0u;

        memset(&frame_parts, 0, sizeof(frame_parts));
        if (frames[i].width == 0u || frames[i].height == 0u ||
            frames[i].x_offset + frames[i].width > canvas_width ||
            frames[i].y_offset + frames[i].height > canvas_height ||
            frames[i].dispose_op > PNG_APNG_DISPOSE_OP_PREVIOUS ||
            frames[i].blend_op > PNG_APNG_BLEND_OP_OVER)
        {
            err = PNG_DEC_ERR_FORMAT;
            goto fail;
        }

        {
            png_encode_options frame_opt = opt_local;
            if (frames[i].stride_bytes != 0u)
                frame_opt.stride_bytes = frames[i].stride_bytes;
            err = png_encode_memory_ex(frames[i].pixels, frames[i].width, frames[i].height,
                                       zfunc, &frame_opt, &frame_png, &frame_png_size);
        }
        if (err != PNG_DEC_OK)
            goto fail;
        err = parse_static_png_parts(frame_png, frame_png_size, &frame_parts);
        png_mem89_release(frame_png);
        if (err != PNG_DEC_OK)
        {
            static_png_parts_free(&frame_parts);
            goto fail;
        }

        err = write_fctl_chunk(&out, &out_size, &out_cap, seq++, &frames[i]);
        if (err != PNG_DEC_OK)
        {
            static_png_parts_free(&frame_parts);
            goto fail;
        }

        write_be32_apng(actl, seq++);
        err = buffer_append(&fdat_buf, &fdat_size, &fdat_cap, actl, 4u);
        if (err != PNG_DEC_OK)
        {
            static_png_parts_free(&frame_parts);
            png_mem89_release(fdat_buf);
            goto fail;
        }
        err = buffer_append(&fdat_buf, &fdat_size, &fdat_cap, frame_parts.idat_data, frame_parts.idat_size);
        if (err != PNG_DEC_OK)
        {
            static_png_parts_free(&frame_parts);
            png_mem89_release(fdat_buf);
            goto fail;
        }
        err = append_chunk_bytes(&out, &out_size, &out_cap, fdAT_type, fdat_buf, fdat_size);
        png_mem89_release(fdat_buf);
        static_png_parts_free(&frame_parts);
        if (err != PNG_DEC_OK)
            goto fail;
    }

    for (i = 0u; i < first_parts.post_chunk_count; ++i)
    {
        err = buffer_append(&out, &out_size, &out_cap,
                            first_parts.post_chunks[i].bytes, first_parts.post_chunks[i].size);
        if (err != PNG_DEC_OK)
            goto fail;
    }

    err = append_chunk_bytes(&out, &out_size, &out_cap, iend_type, 0, 0u);
    if (err != PNG_DEC_OK)
        goto fail;

    png_mem89_release(first_png);
    static_png_parts_free(&first_parts);
    *out_png = out;
    *out_png_size = out_size;
    return PNG_DEC_OK;

fail:
    png_mem89_release(first_png);
    static_png_parts_free(&first_parts);
    png_mem89_release(out);
    return err;
}

int png_encode_apng_auto_memory_ex(const png_apng_encode_frame* full_canvas_frames,
                                   png_u32 frame_count,
                                   png_u32 canvas_width,
                                   png_u32 canvas_height,
                                   png_u32 num_plays,
                                   png_zlib_compress_func zfunc,
                                   const png_encode_options* options,
                                   png_u8** out_png,
                                   png_u32* out_png_size)
{
    png_encode_options opt_local;
    png_apng_encode_frame* optimized = 0;
    png_u8** owned_buffers = 0;
    png_u32 pixel_bytes;
    png_u32 i;
    int err;

    if (!full_canvas_frames || !out_png || !out_png_size || !zfunc || frame_count == 0u)
        return PNG_DEC_ERR_FORMAT;

    if (options)
        opt_local = *options;
    else
        png_encode_options_init(&opt_local);

    err = apng_frame_pixel_bytes(&opt_local, &pixel_bytes);
    if (err != PNG_DEC_OK)
        return err;

    optimized = (png_apng_encode_frame*)png_mem89_alloc_zero(frame_count, sizeof(png_apng_encode_frame));
    owned_buffers = (png_u8**)png_mem89_alloc_zero(frame_count, sizeof(png_u8*));
    if (!optimized || !owned_buffers)
    {
        png_mem89_release(optimized);
        png_mem89_release(owned_buffers);
        return PNG_DEC_ERR_OOM;
    }

    for (i = 0u; i < frame_count; ++i)
    {
        if (!full_canvas_frames[i].pixels ||
            full_canvas_frames[i].width != canvas_width ||
            full_canvas_frames[i].height != canvas_height ||
            full_canvas_frames[i].x_offset != 0u ||
            full_canvas_frames[i].y_offset != 0u)
        {
            err = PNG_DEC_ERR_UNSUPPORTED;
            goto cleanup;
        }
    }

    optimized[0] = full_canvas_frames[0];
    optimized[0].width = canvas_width;
    optimized[0].height = canvas_height;
    optimized[0].x_offset = 0u;
    optimized[0].y_offset = 0u;
    optimized[0].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    optimized[0].blend_op = PNG_APNG_BLEND_OP_SOURCE;
    err = apng_frame_stride_for_encode(&full_canvas_frames[0], &opt_local, &optimized[0].stride_bytes);
    if (err != PNG_DEC_OK)
        goto cleanup;

    for (i = 1u; i < frame_count; ++i)
    {
        png_u32 prev_stride;
        png_u32 curr_stride;
        png_u32 x, y, w, h;
        int diff_err;

        err = apng_frame_stride_for_encode(&full_canvas_frames[i - 1u], &opt_local, &prev_stride);
        if (err != PNG_DEC_OK)
            goto cleanup;
        err = apng_frame_stride_for_encode(&full_canvas_frames[i], &opt_local, &curr_stride);
        if (err != PNG_DEC_OK)
            goto cleanup;

        diff_err = apng_find_diff_rect(full_canvas_frames[i - 1u].pixels, prev_stride,
                                       full_canvas_frames[i].pixels, curr_stride,
                                       canvas_width, canvas_height, pixel_bytes,
                                       &x, &y, &w, &h);
        if (diff_err != PNG_DEC_OK && diff_err != PNG_DEC_DONE)
        {
            err = diff_err;
            goto cleanup;
        }

        err = apng_copy_rect(full_canvas_frames[i].pixels, curr_stride, x, y, w, h,
                             pixel_bytes, &owned_buffers[i], &optimized[i].stride_bytes);
        if (err != PNG_DEC_OK)
            goto cleanup;

        optimized[i].pixels = owned_buffers[i];
        optimized[i].width = w;
        optimized[i].height = h;
        optimized[i].x_offset = x;
        optimized[i].y_offset = y;
        optimized[i].delay_num = full_canvas_frames[i].delay_num;
        optimized[i].delay_den = full_canvas_frames[i].delay_den;
        optimized[i].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
        optimized[i].blend_op = PNG_APNG_BLEND_OP_SOURCE;
    }

    err = png_encode_apng_memory_ex(optimized, frame_count, canvas_width, canvas_height,
                                    num_plays, zfunc, &opt_local, out_png, out_png_size);

cleanup:
    if (owned_buffers)
    {
        for (i = 0u; i < frame_count; ++i)
            png_mem89_release(owned_buffers[i]);
    }
    png_mem89_release(owned_buffers);
    png_mem89_release(optimized);
    return err;
}
