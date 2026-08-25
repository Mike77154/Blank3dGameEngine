#ifndef PNG_FUZZ_COMMON_H
#define PNG_FUZZ_COMMON_H

#include "../png_decoder.h"
#include "../png_mem89.h"

#include <string.h>
#include <stdio.h>

#define PNG_FUZZ_MAX_INPUT_BYTES   (8u * 1024u * 1024u)
#define PNG_FUZZ_MAX_DIMENSION     128u
#define PNG_FUZZ_MAX_PIXELS        (PNG_FUZZ_MAX_DIMENSION * PNG_FUZZ_MAX_DIMENSION)
#define PNG_FUZZ_MAX_IMAGE_BYTES   (8u * 1024u * 1024u)
#define PNG_FUZZ_MAX_FILE_BYTES    (8u * 1024u * 1024u)
#define PNG_FUZZ_MAX_TEMP_BYTES    (32u * 1024u * 1024u)
#define PNG_FUZZ_MAX_APNG_FRAMES   32u
#define PNG_FUZZ_MAX_UNKNOWN       64u
#define PNG_FUZZ_MAX_CHUNKS        1024u
#define PNG_FUZZ_MAX_TEXT_BYTES    (512u * 1024u)
#define PNG_FUZZ_MAX_CHUNK_BYTES   (512u * 1024u)

typedef struct fuzz_cursor_s {
    const png_u8* data;
    png_u32 size;
    png_u32 pos;
} fuzz_cursor;

static png_u8 fuzz_cycle_byte(const fuzz_cursor* cur, png_u32 offset)
{
    if (!cur || cur->size == 0u)
        return (png_u8)(offset * 131u + 17u);
    return cur->data[offset % cur->size];
}

static void fuzz_cursor_init(fuzz_cursor* cur, const png_u8* data, png_u32 size)
{
    if (!cur)
        return;
    cur->data = data;
    cur->size = size;
    cur->pos = 0u;
}

static png_u32 fuzz_remaining(const fuzz_cursor* cur)
{
    if (!cur || cur->pos >= cur->size)
        return 0u;
    return cur->size - cur->pos;
}

static png_u8 fuzz_take_u8(fuzz_cursor* cur)
{
    png_u8 v;
    if (!cur || cur->size == 0u)
        return 0u;
    v = cur->data[cur->pos % cur->size];
    if (cur->pos < cur->size)
        cur->pos += 1u;
    return v;
}

static png_u16 fuzz_take_u16(fuzz_cursor* cur)
{
    png_u16 a = fuzz_take_u8(cur);
    png_u16 b = fuzz_take_u8(cur);
    return (png_u16)((a << 8) | b);
}

static png_u32 fuzz_take_u32(fuzz_cursor* cur)
{
    png_u32 a = fuzz_take_u8(cur);
    png_u32 b = fuzz_take_u8(cur);
    png_u32 c = fuzz_take_u8(cur);
    png_u32 d = fuzz_take_u8(cur);
    return (a << 24) | (b << 16) | (c << 8) | d;
}

static png_u32 fuzz_range(fuzz_cursor* cur, png_u32 min_v, png_u32 max_v)
{
    png_u32 span;
    png_u32 raw;
    if (max_v <= min_v)
        return min_v;
    span = max_v - min_v + 1u;
    raw = fuzz_take_u32(cur);
    return min_v + (raw % span);
}

static int fuzz_bool(fuzz_cursor* cur)
{
    return (fuzz_take_u8(cur) & 1u) != 0u;
}

static png_u8 fuzz_pick_output_format(png_u8 selector)
{
    static const png_u8 formats[] = {
        PNG_OUTPUT_RGBA8, PNG_OUTPUT_BGRA8, PNG_OUTPUT_ARGB8,
        PNG_OUTPUT_RGB8, PNG_OUTPUT_BGR8, PNG_OUTPUT_GA8,
        PNG_OUTPUT_G8, PNG_OUTPUT_RGBA16, PNG_OUTPUT_BGRA16,
        PNG_OUTPUT_RGB16, PNG_OUTPUT_GA16, PNG_OUTPUT_G16
    };
    return formats[(unsigned int)selector % (sizeof(formats) / sizeof(formats[0]))];
}

static png_u8 fuzz_pick_public_input_format(png_u8 selector)
{
    static const png_u8 formats[] = {
        PNG_OUTPUT_RGBA8, PNG_OUTPUT_RGB8, PNG_OUTPUT_GA8,
        PNG_OUTPUT_G8, PNG_OUTPUT_RGBA16, PNG_OUTPUT_RGB16,
        PNG_OUTPUT_GA16, PNG_OUTPUT_G16
    };
    return formats[(unsigned int)selector % (sizeof(formats) / sizeof(formats[0]))];
}

static void fuzz_fill_bytes(fuzz_cursor* cur, png_u8* dst, png_u32 size)
{
    png_u32 i;
    if (!dst)
        return;
    for (i = 0u; i < size; ++i)
        dst[i] = fuzz_cycle_byte(cur, cur ? (cur->pos + i) : i);
    if (cur && cur->size != 0u)
        cur->pos += size;
}

static int fuzz_safe_mul_u32(png_u32 a, png_u32 b, png_u32* out)
{
    if (!out)
        return 0;
    if (a == 0u || b == 0u)
    {
        *out = 0u;
        return 1;
    }
    if (a > 0xFFFFFFFFu / b)
        return 0;
    *out = a * b;
    return 1;
}

static int fuzz_safe_add_u32(png_u32 a, png_u32 b, png_u32* out)
{
    if (!out)
        return 0;
    if (a > 0xFFFFFFFFu - b)
        return 0;
    *out = a + b;
    return 1;
}

static void fuzz_init_decode_options(png_decode_options* opt, fuzz_cursor* cur)
{
    if (!opt)
        return;
    png_decode_options_init(opt);
    opt->output_format = fuzz_pick_output_format(fuzz_take_u8(cur));
    opt->keep_text = 1;
    opt->keep_unknown_chunks = PNG_DEC_KEEP_UNKNOWN_ALL;
    opt->strict_trailing_data = fuzz_bool(cur);
    opt->transform_flags = 0u;
    if (fuzz_bool(cur)) opt->transform_flags |= PNG_DEC_TRANSFORM_APPLY_GAMMA;
    if (fuzz_bool(cur)) opt->transform_flags |= PNG_DEC_TRANSFORM_PREMULTIPLY_ALPHA;
    if (fuzz_bool(cur)) opt->transform_flags |= PNG_DEC_TRANSFORM_SWAP_RB;
    if (fuzz_bool(cur)) opt->transform_flags |= PNG_DEC_TRANSFORM_INVERT_ALPHA;
    if (fuzz_bool(cur)) opt->transform_flags |= PNG_DEC_TRANSFORM_STRIP_ALPHA;
    if (fuzz_bool(cur)) opt->transform_flags |= PNG_DEC_TRANSFORM_SWAP_16_ENDIAN;
    opt->max_width = fuzz_range(cur, 32u, PNG_FUZZ_MAX_DIMENSION);
    opt->max_height = fuzz_range(cur, 32u, PNG_FUZZ_MAX_DIMENSION);
    opt->max_pixels = PNG_FUZZ_MAX_PIXELS;
    opt->max_file_bytes = PNG_FUZZ_MAX_FILE_BYTES;
    opt->max_image_bytes = PNG_FUZZ_MAX_IMAGE_BYTES;
    opt->max_inflated_bytes = PNG_FUZZ_MAX_IMAGE_BYTES;
    opt->max_chunk_bytes = PNG_FUZZ_MAX_CHUNK_BYTES;
    opt->max_chunks = PNG_FUZZ_MAX_CHUNKS;
    opt->max_text_entries = 64u;
    opt->max_unknown_chunks = PNG_FUZZ_MAX_UNKNOWN;
    opt->max_text_bytes = PNG_FUZZ_MAX_TEXT_BYTES;
    opt->max_apng_frames = PNG_FUZZ_MAX_APNG_FRAMES;
    opt->max_temp_bytes = PNG_FUZZ_MAX_TEMP_BYTES;
    opt->max_conversion_expansion = 8u;
}

static void fuzz_init_progressive_control(png_progressive_control* ctl, fuzz_cursor* cur)
{
    if (!ctl)
        return;
    png_progressive_control_init(ctl);
    if (fuzz_bool(cur)) ctl->max_feed_bytes = fuzz_range(cur, 1u, 128u);
    if (fuzz_bool(cur)) ctl->max_buffered_bytes = fuzz_range(cur, 64u, 4096u);
    if (fuzz_bool(cur)) ctl->max_parse_bytes_per_call = fuzz_range(cur, 1u, 512u);
    if (fuzz_bool(cur)) ctl->max_row_callbacks_per_call = fuzz_range(cur, 1u, 16u);
    if (fuzz_bool(cur)) ctl->max_chunk_callbacks_per_call = fuzz_range(cur, 1u, 16u);
    if (fuzz_bool(cur)) ctl->max_chunk_bytes_per_call = fuzz_range(cur, 1u, 4096u);
    if (fuzz_bool(cur)) ctl->max_zlib_work_bytes_per_call = fuzz_range(cur, 1u, 4096u);
    if (fuzz_bool(cur)) ctl->max_zlib_steps_per_call = fuzz_range(cur, 1u, 16u);
}

static void fuzz_init_encode_options(png_encode_options* opt)
{
    if (!opt)
        return;
    png_encode_options_init(opt);
    opt->max_temp_bytes = PNG_FUZZ_MAX_TEMP_BYTES;
    opt->max_conversion_expansion = 8u;
    opt->max_apng_frames = PNG_FUZZ_MAX_APNG_FRAMES;
}

static int fuzz_alloc_public_pixels(fuzz_cursor* cur,
                                    png_u8 public_format,
                                    png_u32 width,
                                    png_u32 height,
                                    png_u8** out_pixels,
                                    png_u32* out_rowbytes)
{
    png_u32 rowbytes;
    png_u32 total;
    png_u8* pixels;

    if (!out_pixels || !out_rowbytes)
        return 0;
    *out_pixels = 0;
    *out_rowbytes = 0u;

    if (png_output_format_rowbytes(public_format, width, &rowbytes) != PNG_DEC_OK)
        return 0;
    if (!fuzz_safe_mul_u32(rowbytes, height, &total))
        return 0;
    if (total > PNG_FUZZ_MAX_IMAGE_BYTES)
        return 0;

    pixels = (png_u8*)png_mem89_alloc(total == 0u ? 1u : total);
    if (!pixels)
        return 0;
    fuzz_fill_bytes(cur, pixels, total);
    *out_pixels = pixels;
    *out_rowbytes = rowbytes;
    return 1;
}

static int fuzz_alloc_indexed_pixels(fuzz_cursor* cur,
                                     png_u32 width,
                                     png_u32 height,
                                     png_u32 palette_entries,
                                     png_u8** out_pixels,
                                     png_u32* out_stride)
{
    png_u32 total;
    png_u32 i;
    png_u8* pixels;

    if (!out_pixels || !out_stride || palette_entries == 0u)
        return 0;
    if (!fuzz_safe_mul_u32(width, height, &total))
        return 0;
    pixels = (png_u8*)png_mem89_alloc(total == 0u ? 1u : total);
    if (!pixels)
        return 0;
    for (i = 0u; i < total; ++i)
        pixels[i] = (png_u8)(fuzz_cycle_byte(cur, cur ? (cur->pos + i) : i) % palette_entries);
    if (cur && cur->size != 0u)
        cur->pos += total;
    *out_pixels = pixels;
    *out_stride = width;
    return 1;
}

static void fuzz_fill_palette(fuzz_cursor* cur, png_u8* palette_rgb, png_u32 palette_entries)
{
    png_u32 i;
    for (i = 0u; i < palette_entries * 3u; ++i)
        palette_rgb[i] = fuzz_cycle_byte(cur, cur ? (cur->pos + i) : i);
    if (cur && cur->size != 0u)
        cur->pos += palette_entries * 3u;
}

static void fuzz_free_many(void* p0, void* p1, void* p2, void* p3)
{
    if (p0) png_mem89_release(p0);
    if (p1) png_mem89_release(p1);
    if (p2) png_mem89_release(p2);
    if (p3) png_mem89_release(p3);
}

#endif
