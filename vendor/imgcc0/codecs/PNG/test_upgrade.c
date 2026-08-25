#include "png_mem89.h"
#include <stdio.h>
#include <string.h>

#include "png_decoder.h"

static int g_rows = 0;
static int g_end = 0;
static png_u32 g_last_rowbytes = 0u;
static png_u8 g_first_row[64];
static png_u32 g_first_row_size = 0u;
static int g_last_pass = 0;

static int g_apng_info = 0;
static int g_apng_frame_infos = 0;
static int g_apng_frame_rows = 0;
static int g_apng_frame_ends = 0;
static int g_apng_end = 0;
static png_u32 g_apng_rows_per_frame[16];
static png_u32 g_apng_last_rowbytes = 0u;
static png_u8 g_apng_first_row[64];
static png_u32 g_apng_first_row_size = 0u;
static png_u32 g_apng_info_width = 0u;
static png_u32 g_apng_info_height = 0u;
static png_u32 g_apng_declared_frames = 0u;
static int g_apng_last_pass = 0;
static int g_apng_max_pass = 0;
static int g_apng_pass_rows = 0;

static png_decoder* g_pause_dec = 0;
static png_apng_decoder* g_pause_apng_dec = 0;
static int g_pause_requested_once = 0;
static int g_apng_pause_requested_once = 0;
static png_u32 g_pause_requested_bytes = 0u;
static png_u32 g_apng_pause_requested_bytes = 0u;
static int g_chunk_events = 0;
static png_u32 g_chunk_types[64];
static png_u32 g_chunk_offsets[64];
static png_u32 g_chunk_total_sizes[64];
static int g_apng_chunk_events = 0;
static png_u32 g_apng_chunk_types[96];
static png_i32 g_apng_chunk_frame_indices[96];

static png_u8 gray_from_rgb(png_u8 r, png_u8 g, png_u8 b)
{
    png_u32 y = (png_u32)r * 2126u + (png_u32)g * 7152u + (png_u32)b * 722u + 5000u;
    return (png_u8)(y / 10000u);
}

static void rgba_to_bgra(const png_u8* src, png_u8* dst, png_u32 pixels)
{
    png_u32 i;
    for (i = 0u; i < pixels; ++i)
    {
        dst[i * 4u + 0u] = src[i * 4u + 2u];
        dst[i * 4u + 1u] = src[i * 4u + 1u];
        dst[i * 4u + 2u] = src[i * 4u + 0u];
        dst[i * 4u + 3u] = src[i * 4u + 3u];
    }
}


static png_u16 read_be16_test(const png_u8* p)
{
    return (png_u16)(((png_u16)p[0] << 8) | (png_u16)p[1]);
}

static void write_be16_test(png_u8* p, png_u16 v)
{
    p[0] = (png_u8)(v >> 8);
    p[1] = (png_u8)(v & 0xFFu);
}

static png_u16 gray_from_rgb16(png_u16 r, png_u16 g, png_u16 b)
{
    png_u32 y = (png_u32)r * 2126u + (png_u32)g * 7152u + (png_u32)b * 722u + 5000u;
    return (png_u16)(y / 10000u);
}

static void rgba16_to_bgra16(const png_u8* src, png_u8* dst, png_u32 pixels)
{
    png_u32 i;
    for (i = 0u; i < pixels; ++i)
    {
        png_u16 r = read_be16_test(src + i * 8u + 0u);
        png_u16 g = read_be16_test(src + i * 8u + 2u);
        png_u16 b = read_be16_test(src + i * 8u + 4u);
        png_u16 a = read_be16_test(src + i * 8u + 6u);
        write_be16_test(dst + i * 8u + 0u, b);
        write_be16_test(dst + i * 8u + 2u, g);
        write_be16_test(dst + i * 8u + 4u, r);
        write_be16_test(dst + i * 8u + 6u, a);
    }
}

static void blend_over_rgba8_pixels(const png_u8* dst, const png_u8* src, png_u8* out, png_u32 pixels)
{
    png_u32 i;
    for (i = 0u; i < pixels; ++i)
    {
        png_u32 sa = src[i * 4u + 3u];
        png_u32 da = dst[i * 4u + 3u];
        png_u32 out_a = sa + (da * (255u - sa) + 127u) / 255u;
        png_u32 c;
        if (out_a == 0u)
        {
            out[i * 4u + 0u] = 0u;
            out[i * 4u + 1u] = 0u;
            out[i * 4u + 2u] = 0u;
            out[i * 4u + 3u] = 0u;
            continue;
        }
        for (c = 0u; c < 3u; ++c)
        {
            png_u32 src_p = (png_u32)src[i * 4u + c] * sa;
            png_u32 dst_p = ((png_u32)dst[i * 4u + c] * da * (255u - sa) + 127u) / 255u;
            out[i * 4u + c] = (png_u8)((src_p + dst_p + out_a / 2u) / out_a);
        }
        out[i * 4u + 3u] = (png_u8)out_a;
    }
}

static void fill_rgba16_pattern(png_u8* rgba16, png_u32 width, png_u32 height)
{
    png_u32 x, y;
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            png_u16 r = (png_u16)((x * 5003u + y * 1901u + 1000u) & 0xFFFFu);
            png_u16 g = (png_u16)((y * 7001u + x * 1307u + 2000u) & 0xFFFFu);
            png_u16 b = (png_u16)((x * 2309u + y * 3301u + 3000u) & 0xFFFFu);
            png_u16 a = (png_u16)(65535u - ((x * 911u + y * 613u) & 0x7FFFu));
            png_u8* p = rgba16 + (y * width + x) * 8u;
            write_be16_test(p + 0u, r);
            write_be16_test(p + 2u, g);
            write_be16_test(p + 4u, b);
            write_be16_test(p + 6u, a);
        }
    }
}

static void row_cb(void* user_ptr, png_u32 row_index, const png_u8* rgba_row, png_u32 rowbytes, int pass)
{
    (void)user_ptr;
    (void)row_index;
    (void)rgba_row;
    g_last_pass = pass;
    g_rows += 1;
    g_last_rowbytes = rowbytes;
    if (row_index == 0u && rowbytes <= (png_u32)sizeof(g_first_row))
    {
        memcpy(g_first_row, rgba_row, rowbytes);
        g_first_row_size = rowbytes;
    }
}

static void pause_row_cb(void* user_ptr, png_u32 row_index, const png_u8* rgba_row, png_u32 rowbytes, int pass)
{
    (void)user_ptr;
    row_cb(user_ptr, row_index, rgba_row, rowbytes, pass);
    if (!g_pause_requested_once && g_pause_dec)
    {
        g_pause_requested_once = 1;
        g_pause_requested_bytes = png_decoder_process_data_pause(g_pause_dec, 1);
    }
}

static void end_cb(void* user_ptr, const png_image* image)
{
    (void)user_ptr;
    (void)image;
    g_end += 1;
}

static void chunk_cb(void* user_ptr, const png_chunk_progress_info* info)
{
    (void)user_ptr;
    if (!info)
        return;
    if (g_chunk_events < (int)(sizeof(g_chunk_types) / sizeof(g_chunk_types[0])))
    {
        g_chunk_types[g_chunk_events] = info->type;
        g_chunk_offsets[g_chunk_events] = info->file_offset;
        g_chunk_total_sizes[g_chunk_events] = info->total_size;
    }
    g_chunk_events += 1;
}

static void apng_chunk_cb(void* user_ptr, const png_chunk_progress_info* info, png_i32 frame_index)
{
    (void)user_ptr;
    if (!info)
        return;
    if (g_apng_chunk_events < (int)(sizeof(g_apng_chunk_types) / sizeof(g_apng_chunk_types[0])))
    {
        g_apng_chunk_types[g_apng_chunk_events] = info->type;
        g_apng_chunk_frame_indices[g_apng_chunk_events] = frame_index;
    }
    g_apng_chunk_events += 1;
}

static void apng_info_cb(void* user_ptr, const png_apng_info* info)
{
    (void)user_ptr;
    g_apng_info += 1;
    if (info)
    {
        g_apng_info_width = info->width;
        g_apng_info_height = info->height;
        g_apng_declared_frames = info->num_frames_declared;
    }
}

static void apng_frame_info_cb(void* user_ptr, png_u32 frame_index, const png_apng_frame_control* control, png_u32 rowbytes)
{
    (void)user_ptr;
    (void)frame_index;
    (void)control;
    g_apng_frame_infos += 1;
    g_apng_last_rowbytes = rowbytes;
}

static void apng_frame_row_cb(void* user_ptr, png_u32 frame_index, png_u32 row_index, const png_u8* row_data, png_u32 rowbytes)
{
    (void)user_ptr;
    g_apng_frame_rows += 1;
    if (frame_index < 16u)
        g_apng_rows_per_frame[frame_index] += 1u;
    if (frame_index == 0u && row_index == 0u && rowbytes <= (png_u32)sizeof(g_apng_first_row))
    {
        memcpy(g_apng_first_row, row_data, rowbytes);
        g_apng_first_row_size = rowbytes;
    }
}

static void apng_frame_row_pass_cb(void* user_ptr, png_u32 frame_index, png_u32 row_index, const png_u8* row_data, png_u32 rowbytes, int pass)
{
    (void)user_ptr;
    (void)frame_index;
    (void)row_index;
    (void)row_data;
    (void)rowbytes;
    g_apng_last_pass = pass;
    if (pass > g_apng_max_pass)
        g_apng_max_pass = pass;
    g_apng_pass_rows += 1;
}

static void apng_pause_frame_row_cb(void* user_ptr, png_u32 frame_index, png_u32 row_index, const png_u8* row_data, png_u32 rowbytes)
{
    (void)user_ptr;
    apng_frame_row_cb(user_ptr, frame_index, row_index, row_data, rowbytes);
    if (!g_apng_pause_requested_once && g_pause_apng_dec)
    {
        g_apng_pause_requested_once = 1;
        g_apng_pause_requested_bytes = png_apng_decoder_process_data_pause(g_pause_apng_dec, 1);
    }
}

static void apng_frame_end_cb(void* user_ptr, png_u32 frame_index, const png_apng_frame* frame)
{
    (void)user_ptr;
    (void)frame_index;
    (void)frame;
    g_apng_frame_ends += 1;
}

static void apng_end_cb(void* user_ptr, const png_apng* apng)
{
    (void)user_ptr;
    (void)apng;
    g_apng_end += 1;
}

static png_u32 read_be32_test(const png_u8* p)
{
    return ((png_u32)p[0] << 24) | ((png_u32)p[1] << 16) | ((png_u32)p[2] << 8) | (png_u32)p[3];
}

static png_u32 find_nth_chunk_end(const png_u8* png_data, png_u32 png_size, const char type[4], int nth)
{
    png_u32 off = 8u;
    int count = 0;
    while (png_data && off + 8u <= png_size)
    {
        png_u32 length = read_be32_test(png_data + off);
        png_u32 chunk_end = off + 12u + length;
        if (chunk_end > png_size)
            return 0u;
        if (png_data[off + 4u] == (png_u8)type[0] && png_data[off + 5u] == (png_u8)type[1] &&
            png_data[off + 6u] == (png_u8)type[2] && png_data[off + 7u] == (png_u8)type[3])
        {
            count += 1;
            if (count == nth)
                return chunk_end;
        }
        off = chunk_end;
    }
    return 0u;
}

static void fill_rgba_pattern(png_u8* rgba, png_u32 width, png_u32 height)
{
    png_u32 x, y;
    for (y = 0; y < height; ++y)
    {
        for (x = 0; x < width; ++x)
        {
            png_u8* p = rgba + (y * width + x) * 4u;
            p[0] = (png_u8)(x * 17u + y * 3u);
            p[1] = (png_u8)(y * 23u + x * 5u);
            p[2] = (png_u8)(x * 9u + y * 11u);
            p[3] = (png_u8)(255u - (x * 7u + y * 13u));
        }
    }
}


static int make_basic_rgba_png(png_u32 width, png_u32 height, png_u8** out_png, png_u32* out_png_size)
{
    png_u32 pixels;
    png_u8* rgba;
    png_encode_options opt;
    int err;

    if (!out_png || !out_png_size || width == 0u || height == 0u)
        return PNG_DEC_ERR_FORMAT;

    *out_png = 0;
    *out_png_size = 0u;
    pixels = width * height;
    rgba = (png_u8*)png_mem89_alloc(pixels * 4u);
    if (!rgba)
        return PNG_DEC_ERR_OOM;

    fill_rgba_pattern(rgba, width, height);
    png_encode_options_init(&opt);
    err = png_encode_memory_ex_zlib(rgba, width, height, &opt, out_png, out_png_size);
    png_mem89_release(rgba);
    return err;
}

static int make_text_png(png_u8** out_png, png_u32* out_png_size)
{
    png_u8 rgba[4u * 2u * 2u];
    png_encode_options opt;
    png_text_entry entry;
    int err;
    char keyword[] = "Comment";
    char text_data[] = "This text chunk is intentionally longer than the metadata budget.";

    if (!out_png || !out_png_size)
        return PNG_DEC_ERR_FORMAT;

    *out_png = 0;
    *out_png_size = 0u;
    fill_rgba_pattern(rgba, 2u, 2u);
    png_encode_options_init(&opt);
    memset(&entry, 0, sizeof(entry));
    entry.keyword = keyword;
    entry.text = text_data;
    entry.compression = 0u;
    opt.text_entries = &entry;
    opt.text_count = 1u;
    err = png_encode_memory_ex_zlib(rgba, 2u, 2u, &opt, out_png, out_png_size);
    return err;
}

static int make_unknown_chunk_png(png_u8** out_png, png_u32* out_png_size)
{
    png_u8 rgba[4u * 2u * 2u];
    png_u8 chunk_data[16u];
    png_unknown_chunk chunk;
    png_encode_options opt;
    png_u32 i;
    int err;

    if (!out_png || !out_png_size)
        return PNG_DEC_ERR_FORMAT;

    *out_png = 0;
    *out_png_size = 0u;
    fill_rgba_pattern(rgba, 2u, 2u);
    for (i = 0u; i < (png_u32)sizeof(chunk_data); ++i)
        chunk_data[i] = (png_u8)(i * 9u + 3u);

    memset(&chunk, 0, sizeof(chunk));
    chunk.type[0] = (png_u8)'r';
    chunk.type[1] = (png_u8)'a';
    chunk.type[2] = (png_u8)'R';
    chunk.type[3] = (png_u8)'e';
    chunk.type[4] = 0u;
    chunk.data = chunk_data;
    chunk.size = (png_u32)sizeof(chunk_data);
    chunk.location = PNG_CHUNK_POS_AFTER_IHDR;
    chunk.safe_to_copy = 1u;

    png_encode_options_init(&opt);
    opt.unknown_chunks = &chunk;
    opt.unknown_chunk_count = 1u;
    err = png_encode_memory_ex_zlib(rgba, 2u, 2u, &opt, out_png, out_png_size);
    return err;
}

static int make_two_frame_apng(png_u8** out_png, png_u32* out_png_size)
{
    png_u8 frame0[3u * 2u * 4u];
    png_u8 frame1[3u * 2u * 4u];
    png_apng_encode_frame frames[2u];
    png_encode_options opt;
    png_u32 i;
    int err;

    if (!out_png || !out_png_size)
        return PNG_DEC_ERR_FORMAT;

    *out_png = 0;
    *out_png_size = 0u;
    fill_rgba_pattern(frame0, 3u, 2u);
    for (i = 0u; i < (png_u32)sizeof(frame1); ++i)
        frame1[i] = (png_u8)(255u - frame0[i]);

    memset(frames, 0, sizeof(frames));
    frames[0].pixels = frame0;
    frames[0].width = 3u;
    frames[0].height = 2u;
    frames[0].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[0].blend_op = PNG_APNG_BLEND_OP_SOURCE;
    frames[1].pixels = frame1;
    frames[1].width = 3u;
    frames[1].height = 2u;
    frames[1].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[1].blend_op = PNG_APNG_BLEND_OP_SOURCE;

    png_encode_options_init(&opt);
    err = png_encode_apng_memory_ex_zlib(frames, 2u, 3u, 2u, 0u, &opt, out_png, out_png_size);
    return err;
}

static int test_adam7_iccp(void)
{
    png_u32 width = 13u, height = 9u;
    png_u32 pixels = width * height;
    png_u8* rgba = 0;
    png_encode_options opt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_image prog_img;
    png_decode_options dopt;
    png_decoder* dec = 0;
    png_progressive_poll_state poll;
    png_u32 offset;
    int guard;
    int err;
    const png_u8 icc_bytes[] = { 'F','A','K','E','I','C','C','0','1','2','3','4' };

    rgba = (png_u8*)png_mem89_alloc(pixels * 4u);
    if (!rgba)
        return 100;
    fill_rgba_pattern(rgba, width, height);

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.interlace_method = 1u;
    opt.write_iCCP = 1;
    opt.iccp_name = "Profile Name";
    opt.iccp_profile = icc_bytes;
    opt.iccp_profile_size = (png_u32)sizeof(icc_bytes);
    opt.write_pHYs = 1;
    opt.pHYs_ppu_x = 3779u;
    opt.pHYs_ppu_y = 3779u;
    opt.pHYs_unit = 1u;

    err = png_encode_memory_ex_zlib(rgba, width, height, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba);
        return 101;
    }

    memset(&img, 0, sizeof(img));
    memset(&prog_img, 0, sizeof(prog_img));
    png_decode_options_init(&dopt);
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba);
        png_free_file(png_data);
        return 102;
    }

    if (img.width != width || img.height != height || img.interlace_method != 1u) {
        png_mem89_release(rgba);
        png_free_file(png_data);
        png_free_image(&img);
        return 103;
    }

    if (!img.has_iCCP || !img.iccp_profile || img.iccp_profile_size != (png_u32)sizeof(icc_bytes) ||
        memcmp(img.iccp_profile, icc_bytes, sizeof(icc_bytes)) != 0) {
        png_mem89_release(rgba);
        png_free_file(png_data);
        png_free_image(&img);
        return 104;
    }

    if (memcmp(img.rgba, rgba, pixels * 4u) != 0) {
        png_mem89_release(rgba);
        png_free_file(png_data);
        png_free_image(&img);
        return 105;
    }

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba);
        png_free_file(png_data);
        png_free_image(&img);
        return 106;
    }

    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba);
        png_free_file(png_data);
        png_free_image(&img);
        png_decoder_free(dec);
        return 107;
    }

    offset = 0u;
    guard = 0;
    while (offset < png_size && guard++ < 4096)
    {
        png_u32 consumed = 0u;
        png_u32 take = png_size - offset;
        if (take > 11u)
            take = 11u;
        err = png_decoder_feed_ex(dec, png_data + offset, take, &consumed);
        offset += consumed;
        if (err == PNG_DEC_OK)
            continue;
        if (err == PNG_DEC_DONE)
            break;
        png_mem89_release(rgba);
        png_free_file(png_data);
        png_free_image(&img);
        png_decoder_free(dec);
        return 108;
    }

    while (guard++ < 4096)
    {
        png_u32 consumed = 0u;
        err = png_decoder_poll(dec, &poll);
        if (err != PNG_DEC_OK) {
            png_mem89_release(rgba);
            png_free_file(png_data);
            png_free_image(&img);
            png_decoder_free(dec);
            return 109;
        }
        if ((poll.events & PNG_PROGRESSIVE_POLL_DONE) != 0u)
            break;
        err = png_decoder_feed_ex(dec, (const png_u8*)0, 0u, &consumed);
        if (err == PNG_DEC_OK)
            continue;
        if (err == PNG_DEC_DONE)
            break;
        png_mem89_release(rgba);
        png_free_file(png_data);
        png_free_image(&img);
        png_decoder_free(dec);
        return 110;
    }

    if (guard >= 4096) {
        png_mem89_release(rgba);
        png_free_file(png_data);
        png_free_image(&img);
        png_decoder_free(dec);
        return 111;
    }

    err = png_decoder_take_image(dec, &prog_img);
    png_decoder_free(dec);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba);
        png_free_file(png_data);
        png_free_image(&img);
        return 112;
    }

    if (prog_img.width != width || prog_img.height != height || prog_img.interlace_method != 1u ||
        !prog_img.has_iCCP || !prog_img.iccp_profile || prog_img.iccp_profile_size != (png_u32)sizeof(icc_bytes) ||
        memcmp(prog_img.iccp_profile, icc_bytes, sizeof(icc_bytes)) != 0 ||
        !prog_img.has_pHYs || prog_img.pHYs_ppu_x != 3779u || prog_img.pHYs_ppu_y != 3779u || prog_img.pHYs_unit != 1u ||
        memcmp(prog_img.rgba, rgba, pixels * 4u) != 0) {
        png_mem89_release(rgba);
        png_free_file(png_data);
        png_free_image(&img);
        png_free_image(&prog_img);
        return 113;
    }

    {
        FILE* f = fopen("test_adam7_iccp.png", "wb");
        if (f) {
            fwrite(png_data, 1, png_size, f);
            fclose(f);
        }
    }

    png_mem89_release(rgba);
    png_free_file(png_data);
    png_free_image(&img);
    png_free_image(&prog_img);
    return 0;
}

static int test_runtime_limits(void)
{
    png_u8 rgba[4 * 6 * 3];
    png_encode_options opt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    int err;

    fill_rgba_pattern(rgba, 6u, 3u);
    png_encode_options_init(&opt);
    err = png_encode_memory_ex_zlib(rgba, 6u, 3u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 201;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.max_width = 4u;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_ERR_DIMENSIONS_TOO_LARGE)
        return 202;

    return 0;
}


static int test_error_too_many_pixels(void)
{
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    int err;

    err = make_basic_rgba_png(6u, 3u, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 211;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.max_pixels = 8u;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_ERR_TOO_MANY_PIXELS)
    {
        png_free_image(&img);
        return 212;
    }

    png_free_image(&img);
    return 0;
}

static int test_error_inflated_too_large(void)
{
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    int err;

    err = make_basic_rgba_png(6u, 3u, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 221;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.max_inflated_bytes = 32u;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_ERR_INFLATED_TOO_LARGE)
    {
        png_free_image(&img);
        return 222;
    }

    png_free_image(&img);
    return 0;
}

static int test_error_chunk_too_large(void)
{
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    int err;

    err = make_unknown_chunk_png(&png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 231;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.max_chunk_bytes = 8u;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_ERR_CHUNK_TOO_LARGE)
    {
        png_free_image(&img);
        return 232;
    }

    png_free_image(&img);
    return 0;
}

static int test_error_too_many_chunks(void)
{
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    int err;

    err = make_basic_rgba_png(2u, 2u, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 241;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.max_chunks = 1u;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_ERR_TOO_MANY_CHUNKS)
    {
        png_free_image(&img);
        return 242;
    }

    png_free_image(&img);
    return 0;
}

static int test_error_text_too_large(void)
{
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    int err;

    err = make_text_png(&png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 251;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.max_text_bytes = 8u;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_ERR_TEXT_TOO_LARGE)
    {
        png_free_image(&img);
        return 252;
    }

    png_free_image(&img);
    return 0;
}

static int test_error_frame_limit(void)
{
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_apng apng;
    png_decode_options dopt;
    int err;

    err = make_two_frame_apng(&png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 261;

    memset(&apng, 0, sizeof(apng));
    png_decode_options_init(&dopt);
    dopt.max_apng_frames = 1u;
    err = png_decode_apng_memory_ex_zlib(png_data, png_size, &dopt, &apng);
    png_free_file(png_data);
    if (err != PNG_DEC_ERR_FRAME_LIMIT)
    {
        png_free_apng(&apng);
        return 262;
    }

    png_free_apng(&apng);
    return 0;
}

static int test_error_temp_memory_limit(void)
{
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    int err;

    err = make_basic_rgba_png(6u, 3u, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 271;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.max_temp_bytes = 120u;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_ERR_TEMP_MEMORY_LIMIT)
    {
        png_free_image(&img);
        return 272;
    }

    png_free_image(&img);
    return 0;
}

static int test_error_work_budget(void)
{
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_decoder* dec = 0;
    png_progressive_control ctl;
    int err;

    err = make_basic_rgba_png(8u, 8u, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 281;

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK)
    {
        png_free_file(png_data);
        return 282;
    }

    png_progressive_control_init(&ctl);
    ctl.max_zlib_steps_per_call = 1u;
    err = png_decoder_set_progressive_control(dec, &ctl);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 283;
    }

    err = png_decoder_feed(dec, png_data, png_size);
    png_decoder_free(dec);
    if (err != PNG_DEC_YIELDED)
    {
        png_free_file(png_data);
        return 284;
    }

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK)
    {
        png_free_file(png_data);
        return 285;
    }

    png_progressive_control_init(&ctl);
    ctl.max_zlib_steps_per_call = 1u;
    ctl.hard_fail_on_budget_exhaustion = 1;
    err = png_decoder_set_progressive_control(dec, &ctl);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 286;
    }

    err = png_decoder_feed(dec, png_data, png_size);
    png_decoder_free(dec);
    png_free_file(png_data);
    if (err != PNG_DEC_ERR_WORK_BUDGET)
        return 287;

    return 0;
}

static int test_error_conversion_limit(void)
{
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    png_conversion_limits limits;
    int err;

    err = make_basic_rgba_png(6u, 3u, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 291;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
    {
        png_free_image(&img);
        return 292;
    }

    png_conversion_limits_init(&limits);
    limits.max_expansion = 1u;
    err = png_image_convert_format_ex(&img, PNG_OUTPUT_RGBA16, &limits);
    if (err != PNG_DEC_ERR_CONVERSION_LIMIT)
    {
        png_free_image(&img);
        return 293;
    }

    png_free_image(&img);
    return 0;
}

static int test_incremental_rows(void)
{
    png_u32 width = 8u, height = 4u;
    png_u8 rgba[8u * 4u * 4u];
    png_encode_options opt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_decoder* dec = 0;
    png_progressive_callbacks cb;
    png_image out;
    int err;

    fill_rgba_pattern(rgba, width, height);
    png_encode_options_init(&opt);
    opt.interlace_method = 0u;
    err = png_encode_memory_ex_zlib(rgba, width, height, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 301;

    if (png_size <= 12u) {
        png_free_file(png_data);
        return 302;
    }

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK) {
        png_free_file(png_data);
        return 303;
    }

    memset(&cb, 0, sizeof(cb));
    cb.row_fn = row_cb;
    cb.end_fn = end_cb;
    png_decoder_set_callbacks(dec, &cb);

    g_rows = 0;
    g_end = 0;
    err = png_decoder_feed(dec, png_data, png_size - 12u);
    if (err != PNG_DEC_OK) {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 304;
    }

    if (g_rows != (int)height || g_end != 0) {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 305;
    }

    err = png_decoder_feed(dec, png_data + png_size - 12u, 12u);
    if (err != PNG_DEC_DONE) {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 306;
    }

    if (g_end != 1) {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 307;
    }

    memset(&out, 0, sizeof(out));
    err = png_decoder_take_image(dec, &out);
    if (err != PNG_DEC_OK) {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 308;
    }

    if (memcmp(out.rgba, rgba, width * height * 4u) != 0) {
        png_free_image(&out);
        png_decoder_free(dec);
        png_free_file(png_data);
        return 309;
    }

    png_free_image(&out);
    png_decoder_free(dec);
    png_free_file(png_data);
    return 0;
}

static int test_iccp_srgb_conflict(void)
{
    png_u8 rgba[4 * 2 * 2];
    png_encode_options opt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    int err;
    const png_u8 icc_bytes[] = { 'A', 'B', 'C' };

    fill_rgba_pattern(rgba, 2u, 2u);
    png_encode_options_init(&opt);
    opt.write_sRGB = 1;
    opt.write_iCCP = 1;
    opt.iccp_name = "Profile Name";
    opt.iccp_profile = icc_bytes;
    opt.iccp_profile_size = (png_u32)sizeof(icc_bytes);

    err = png_encode_memory_ex_zlib(rgba, 2u, 2u, &opt, &png_data, &png_size);
    if (png_data)
        png_free_file(png_data);
    if (err != PNG_DEC_ERR_FORMAT)
        return 401;
    return 0;
}

static int test_output_formats_and_convert(void)
{
    png_u8 rgba[4u * 3u];
    png_encode_options opt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    png_u8 expected_bgra[4u * 3u];
    png_u8 expected_gray[3u];
    int err;

    rgba[0] = 10u; rgba[1] = 20u; rgba[2] = 30u; rgba[3] = 40u;
    rgba[4] = 50u; rgba[5] = 60u; rgba[6] = 70u; rgba[7] = 80u;
    rgba[8] = 90u; rgba[9] = 100u; rgba[10] = 110u; rgba[11] = 120u;

    rgba_to_bgra(rgba, expected_bgra, 3u);
    expected_gray[0] = gray_from_rgb(rgba[0], rgba[1], rgba[2]);
    expected_gray[1] = gray_from_rgb(rgba[4], rgba[5], rgba[6]);
    expected_gray[2] = gray_from_rgb(rgba[8], rgba[9], rgba[10]);

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    err = png_encode_memory_ex_zlib(rgba, 3u, 1u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 501;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_BGRA8;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    if (err != PNG_DEC_OK) { png_free_file(png_data); return 502; }
    if (img.output_format != PNG_OUTPUT_BGRA8 || img.output_channels != 4u || img.pixel_rowbytes != 12u) {
        png_free_image(&img); png_free_file(png_data); return 503;
    }
    if (img.rgba != 0 || !img.pixels || memcmp(img.pixels, expected_bgra, 12u) != 0) {
        png_free_image(&img); png_free_file(png_data); return 504;
    }

    err = png_image_convert_format(&img, PNG_OUTPUT_RGBA8);
    if (err != PNG_DEC_OK) { png_free_image(&img); png_free_file(png_data); return 505; }
    if (img.output_format != PNG_OUTPUT_RGBA8 || img.rgba != img.pixels || memcmp(img.rgba, rgba, 12u) != 0) {
        png_free_image(&img); png_free_file(png_data); return 506;
    }
    png_free_image(&img);

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_G8;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 507;
    if (img.pixel_rowbytes != 3u || img.output_channels != 1u || memcmp(img.pixels, expected_gray, 3u) != 0) {
        png_free_image(&img); return 508;
    }

    png_free_image(&img);
    return 0;
}

static int test_streaming_output_format(void)
{
    png_u32 width = 5u, height = 2u;
    png_u8 rgba[5u * 2u * 4u];
    png_u8 expected_bgra[5u * 2u * 4u];
    png_encode_options opt;
    png_decode_options dopt;
    png_progressive_callbacks cb;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_decoder* dec = 0;
    png_image out;
    int err;

    fill_rgba_pattern(rgba, width, height);
    rgba_to_bgra(rgba, expected_bgra, width * height);

    png_encode_options_init(&opt);
    err = png_encode_memory_ex_zlib(rgba, width, height, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 601;

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK) { png_free_file(png_data); return 602; }

    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_BGRA8;
    err = png_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK) { png_decoder_free(dec); png_free_file(png_data); return 603; }

    memset(&cb, 0, sizeof(cb));
    cb.row_fn = row_cb;
    cb.end_fn = end_cb;
    png_decoder_set_callbacks(dec, &cb);

    g_rows = 0; g_end = 0; g_last_rowbytes = 0u; g_first_row_size = 0u;
    err = png_decoder_feed(dec, png_data, png_size);
    if (err != PNG_DEC_DONE) { png_decoder_free(dec); png_free_file(png_data); return 604; }
    if (g_rows != (int)height || g_end != 1 || g_last_rowbytes != width * 4u || g_first_row_size != width * 4u) {
        png_decoder_free(dec); png_free_file(png_data); return 605;
    }
    if (memcmp(g_first_row, expected_bgra, width * 4u) != 0) {
        png_decoder_free(dec); png_free_file(png_data); return 606;
    }

    memset(&out, 0, sizeof(out));
    err = png_decoder_take_image(dec, &out);
    if (err != PNG_DEC_OK) { png_decoder_free(dec); png_free_file(png_data); return 607; }
    if (out.output_format != PNG_OUTPUT_BGRA8 || memcmp(out.pixels, expected_bgra, width * height * 4u) != 0) {
        png_free_image(&out); png_decoder_free(dec); png_free_file(png_data); return 608;
    }

    png_free_image(&out);
    png_decoder_free(dec);
    png_free_file(png_data);
    return 0;
}

static int test_write_chrm_bkgd_sbit(void)
{
    png_u8 rgba[4u * 4u];
    png_encode_options opt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    int err;

    fill_rgba_pattern(rgba, 2u, 2u);
    png_encode_options_init(&opt);
    opt.write_gAMA = 1;
    opt.image_gamma = png_fixed89_from_ratio(45455, 100000);
    opt.write_cHRM = 1;
    opt.white_x = png_fixed89_from_ratio(3127, 10000); opt.white_y = png_fixed89_from_ratio(3290, 10000);
    opt.red_x = png_fixed89_from_ratio(6400, 10000); opt.red_y = png_fixed89_from_ratio(3300, 10000);
    opt.green_x = png_fixed89_from_ratio(3000, 10000); opt.green_y = png_fixed89_from_ratio(6000, 10000);
    opt.blue_x = png_fixed89_from_ratio(1500, 10000); opt.blue_y = png_fixed89_from_ratio(600, 10000);
    opt.write_bKGD = 1;
    opt.bkgd_r = 0x3333u; opt.bkgd_g = 0x6666u; opt.bkgd_b = 0x9999u;
    opt.write_sBIT = 1;
    opt.sbit_r = 5u; opt.sbit_g = 6u; opt.sbit_b = 7u; opt.sbit_a = 8u;

    err = png_encode_memory_ex_zlib(rgba, 2u, 2u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 701;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 702;

    if (!img.has_cHRM) { png_free_image(&img); return 703; }
    if (img.white_x < png_fixed89_from_ratio(31269, 100000) || img.white_x > png_fixed89_from_ratio(31271, 100000) ||
        img.white_y < png_fixed89_from_ratio(32899, 100000) || img.white_y > png_fixed89_from_ratio(32901, 100000)) { png_free_image(&img); return 704; }
    if (!img.has_bKGD || img.bkgd_r != 0x3333u || img.bkgd_g != 0x6666u || img.bkgd_b != 0x9999u) { png_free_image(&img); return 705; }
    if (!img.has_sBIT || img.sbit_r != 5u || img.sbit_g != 6u || img.sbit_b != 7u || img.sbit_a != 8u) { png_free_image(&img); return 706; }

    png_free_image(&img);
    return 0;
}



static int test_extended_ancillary_platform_chunks(void)
{
    png_u8 rgb[2u * 3u];
    png_u8 palette[4u * 3u];
    png_u16 hist[4u];
    png_splt_entry splt_entries[3u];
    png_splt_palette splt;
    png_encode_options opt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    int err;
    char splt_name[] = "Preview Palette";

    rgb[0] = 11u; rgb[1] = 22u; rgb[2] = 33u;
    rgb[3] = 44u; rgb[4] = 55u; rgb[5] = 66u;

    palette[0] = 10u; palette[1] = 20u; palette[2] = 30u;
    palette[3] = 40u; palette[4] = 50u; palette[5] = 60u;
    palette[6] = 70u; palette[7] = 80u; palette[8] = 90u;
    palette[9] = 100u; palette[10] = 110u; palette[11] = 120u;

    hist[0] = 100u;
    hist[1] = 300u;
    hist[2] = 50u;
    hist[3] = 10u;

    splt_entries[0].red = 0x1010u;
    splt_entries[0].green = 0x2020u;
    splt_entries[0].blue = 0x3030u;
    splt_entries[0].alpha = 0xFFFFu;
    splt_entries[0].frequency = 200u;
    splt_entries[1].red = 0x4040u;
    splt_entries[1].green = 0x5050u;
    splt_entries[1].blue = 0x6060u;
    splt_entries[1].alpha = 0xFFFFu;
    splt_entries[1].frequency = 150u;
    splt_entries[2].red = 0x7070u;
    splt_entries[2].green = 0x8080u;
    splt_entries[2].blue = 0x9090u;
    splt_entries[2].alpha = 0x8080u;
    splt_entries[2].frequency = 75u;

    memset(&splt, 0, sizeof(splt));
    splt.name = splt_name;
    splt.sample_depth = 8u;
    splt.entries = splt_entries;
    splt.entry_count = 3u;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR;
    opt.bit_depth = 8u;
    opt.palette = palette;
    opt.palette_entries = 4u;
    opt.hist_entries = hist;
    opt.hist_count = 4u;
    opt.splt_palettes = &splt;
    opt.splt_palette_count = 1u;
    opt.write_oFFs = 1;
    opt.offset_x = -25;
    opt.offset_y = 40;
    opt.offset_unit = 1u;
    opt.write_sCAL = 1;
    opt.scal_unit = 1u;
    opt.scal_pixel_width = "0.125";
    opt.scal_pixel_height = "0.5";
    opt.write_sTER = 1;
    opt.ster_mode = 1u;

    err = png_encode_memory_ex_zlib(rgb, 2u, 1u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 751;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 752;

    if (!img.pixels || img.pixel_rowbytes != 8u || img.output_format != PNG_OUTPUT_RGBA8)
    {
        png_free_image(&img);
        return 753;
    }

    if (img.rgba[0] != 11u || img.rgba[1] != 22u || img.rgba[2] != 33u || img.rgba[3] != 255u ||
        img.rgba[4] != 44u || img.rgba[5] != 55u || img.rgba[6] != 66u || img.rgba[7] != 255u)
    {
        png_free_image(&img);
        return 754;
    }

    if (!img.has_oFFs || img.offset_x != -25 || img.offset_y != 40 || img.offset_unit != 1u)
    {
        png_free_image(&img);
        return 755;
    }

    if (!img.has_sCAL || img.scal_unit != 1u || !img.scal_pixel_width || !img.scal_pixel_height ||
        strcmp(img.scal_pixel_width, "0.125") != 0 || strcmp(img.scal_pixel_height, "0.5") != 0)
    {
        png_free_image(&img);
        return 756;
    }

    if (!img.has_sTER || img.ster_mode != 1u)
    {
        png_free_image(&img);
        return 757;
    }

    if (!img.hist_entries || img.hist_count != 4u ||
        img.hist_entries[0] != 100u || img.hist_entries[1] != 300u ||
        img.hist_entries[2] != 50u || img.hist_entries[3] != 10u)
    {
        png_free_image(&img);
        return 758;
    }

    if (!img.splt_palettes || img.splt_palette_count != 1u ||
        !img.splt_palettes[0].name || strcmp(img.splt_palettes[0].name, "Preview Palette") != 0 ||
        img.splt_palettes[0].sample_depth != 8u || img.splt_palettes[0].entry_count != 3u)
    {
        png_free_image(&img);
        return 759;
    }

    if (img.splt_palettes[0].entries[0].red != 0x1010u ||
        img.splt_palettes[0].entries[0].green != 0x2020u ||
        img.splt_palettes[0].entries[0].blue != 0x3030u ||
        img.splt_palettes[0].entries[0].frequency != 200u ||
        img.splt_palettes[0].entries[2].alpha != 0x8080u)
    {
        png_free_image(&img);
        return 760;
    }

    png_free_image(&img);
    return 0;
}

static int test_output_formats_16bit_and_convert(void)
{
    png_u8 rgba16[2u * 1u * 8u];
    png_u8 expected_bgra16[2u * 1u * 8u];
    png_u8 expected_gray16[2u * 2u];
    png_encode_options opt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    png_decode_options dopt;
    int err;
    png_u16 g0, g1;

    write_be16_test(rgba16 + 0u, 0x1234u); write_be16_test(rgba16 + 2u, 0x5678u);
    write_be16_test(rgba16 + 4u, 0x9ABCu); write_be16_test(rgba16 + 6u, 0xDEF0u);
    write_be16_test(rgba16 + 8u, 0x0102u); write_be16_test(rgba16 + 10u, 0x0304u);
    write_be16_test(rgba16 + 12u, 0x0506u); write_be16_test(rgba16 + 14u, 0x0708u);

    rgba16_to_bgra16(rgba16, expected_bgra16, 2u);
    g0 = gray_from_rgb16(0x1234u, 0x5678u, 0x9ABCu);
    g1 = gray_from_rgb16(0x0102u, 0x0304u, 0x0506u);
    write_be16_test(expected_gray16 + 0u, g0);
    write_be16_test(expected_gray16 + 2u, g1);

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 16u;
    err = png_encode_memory_ex_zlib(rgba16, 2u, 1u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 801;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_BGRA16;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    if (err != PNG_DEC_OK) { png_free_file(png_data); return 802; }
    if (img.output_format != PNG_OUTPUT_BGRA16 || img.output_channels != 4u ||
        img.output_sample_depth != 16u || img.output_bytes_per_channel != 2u ||
        img.pixel_rowbytes != 16u) {
        png_free_image(&img); png_free_file(png_data); return 803;
    }
    if (img.rgba != 0 || !img.pixels || memcmp(img.pixels, expected_bgra16, 16u) != 0) {
        png_free_image(&img); png_free_file(png_data); return 804;
    }

    err = png_image_convert_format(&img, PNG_OUTPUT_RGBA16);
    if (err != PNG_DEC_OK) { png_free_image(&img); png_free_file(png_data); return 805; }
    if (img.output_format != PNG_OUTPUT_RGBA16 || img.output_sample_depth != 16u ||
        memcmp(img.pixels, rgba16, 16u) != 0) {
        png_free_image(&img); png_free_file(png_data); return 806;
    }
    png_free_image(&img);

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_G16;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 807;
    if (img.pixel_rowbytes != 4u || img.output_channels != 1u || img.output_sample_depth != 16u ||
        memcmp(img.pixels, expected_gray16, 4u) != 0) {
        png_free_image(&img); return 808;
    }

    png_free_image(&img);
    return 0;
}

static int test_streaming_output_format_16bit(void)
{
    png_u32 width = 4u, height = 2u;
    png_u8 rgba16[4u * 2u * 8u];
    png_u8 expected_bgra16[4u * 2u * 8u];
    png_encode_options opt;
    png_decode_options dopt;
    png_progressive_callbacks cb;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_decoder* dec = 0;
    png_image out;
    int err;

    fill_rgba16_pattern(rgba16, width, height);
    rgba16_to_bgra16(rgba16, expected_bgra16, width * height);

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 16u;
    err = png_encode_memory_ex_zlib(rgba16, width, height, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 901;

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK) { png_free_file(png_data); return 902; }

    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_BGRA16;
    err = png_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK) { png_decoder_free(dec); png_free_file(png_data); return 903; }

    memset(&cb, 0, sizeof(cb));
    cb.row_fn = row_cb;
    cb.end_fn = end_cb;
    png_decoder_set_callbacks(dec, &cb);

    g_rows = 0; g_end = 0; g_last_rowbytes = 0u; g_first_row_size = 0u;
    err = png_decoder_feed(dec, png_data, png_size);
    if (err != PNG_DEC_DONE) { png_decoder_free(dec); png_free_file(png_data); return 904; }
    if (g_rows != (int)height || g_end != 1 || g_last_rowbytes != width * 8u || g_first_row_size != width * 8u) {
        png_decoder_free(dec); png_free_file(png_data); return 905;
    }
    if (memcmp(g_first_row, expected_bgra16, width * 8u) != 0) {
        png_decoder_free(dec); png_free_file(png_data); return 906;
    }

    memset(&out, 0, sizeof(out));
    err = png_decoder_take_image(dec, &out);
    if (err != PNG_DEC_OK) { png_decoder_free(dec); png_free_file(png_data); return 907; }
    if (out.output_format != PNG_OUTPUT_BGRA16 || out.output_sample_depth != 16u ||
        memcmp(out.pixels, expected_bgra16, width * height * 8u) != 0) {
        png_free_image(&out); png_decoder_free(dec); png_free_file(png_data); return 908;
    }

    png_free_image(&out);
    png_decoder_free(dec);
    png_free_file(png_data);
    return 0;
}


static int test_incremental_adam7_rows(void)
{
    png_u32 width = 9u, height = 7u;
    png_u32 pixels = width * height;
    png_u8* rgba = 0;
    png_u8* expected_bgra = 0;
    png_encode_options opt;
    png_decode_options dopt;
    png_progressive_callbacks cb;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_decoder* dec = 0;
    png_image out;
    int err;

    rgba = (png_u8*)png_mem89_alloc(pixels * 4u);
    expected_bgra = (png_u8*)png_mem89_alloc(pixels * 4u);
    if (!rgba || !expected_bgra) {
        if (rgba) png_mem89_release(rgba);
        if (expected_bgra) png_mem89_release(expected_bgra);
        return 951;
    }

    fill_rgba_pattern(rgba, width, height);
    rgba_to_bgra(rgba, expected_bgra, pixels);

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.interlace_method = 1u;
    err = png_encode_memory_ex_zlib(rgba, width, height, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba); png_mem89_release(expected_bgra); return 952;
    }

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba); png_mem89_release(expected_bgra); png_free_file(png_data); return 953;
    }

    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_BGRA8;
    err = png_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba); png_mem89_release(expected_bgra); png_decoder_free(dec); png_free_file(png_data); return 954;
    }

    memset(&cb, 0, sizeof(cb));
    cb.row_fn = row_cb;
    cb.end_fn = end_cb;
    png_decoder_set_callbacks(dec, &cb);

    g_rows = 0; g_end = 0; g_last_rowbytes = 0u; g_first_row_size = 0u; g_last_pass = 0;
    err = png_decoder_feed(dec, png_data, png_size - 12u);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba); png_mem89_release(expected_bgra); png_decoder_free(dec); png_free_file(png_data); return 955;
    }
    if (g_rows <= 0 || g_end != 0 || g_last_rowbytes != width * 4u || g_last_pass <= 0) {
        png_mem89_release(rgba); png_mem89_release(expected_bgra); png_decoder_free(dec); png_free_file(png_data); return 956;
    }

    err = png_decoder_feed(dec, png_data + png_size - 12u, 12u);
    if (err != PNG_DEC_DONE || g_end != 1) {
        png_mem89_release(rgba); png_mem89_release(expected_bgra); png_decoder_free(dec); png_free_file(png_data); return 957;
    }

    memset(&out, 0, sizeof(out));
    err = png_decoder_take_image(dec, &out);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba); png_mem89_release(expected_bgra); png_decoder_free(dec); png_free_file(png_data); return 958;
    }

    if (out.output_format != PNG_OUTPUT_BGRA8 || out.pixel_rowbytes != width * 4u ||
        memcmp(out.pixels, expected_bgra, pixels * 4u) != 0) {
        png_free_image(&out); png_mem89_release(rgba); png_mem89_release(expected_bgra); png_decoder_free(dec); png_free_file(png_data); return 959;
    }

    png_free_image(&out);
    png_decoder_free(dec);
    png_free_file(png_data);
    png_mem89_release(rgba);
    png_mem89_release(expected_bgra);
    return 0;
}

static int test_pcal_roundtrip(void)
{
    png_u8 gray[4u] = { 0u, 64u, 128u, 255u };
    png_encode_options opt;
    png_decode_options dopt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    int err;
    png_fixed89 physical;
    static const char* params[2] = { "273.15", "100.0" };

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_GRAYSCALE;
    opt.bit_depth = 8u;
    opt.write_pCAL = 1;
    opt.pcal.name = "Temperature";
    opt.pcal.x0 = 0;
    opt.pcal.x1 = 255;
    opt.pcal.equation_type = 0u;
    opt.pcal.unit_name = "K";
    opt.pcal.params = (char**)params;
    opt.pcal.param_count = 2u;

    err = png_encode_memory_ex_zlib(gray, 4u, 1u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 961;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 962;

    if (!img.has_pCAL || !img.pcal.name || !img.pcal.unit_name || !img.pcal.params || img.pcal.param_count != 2u ||
        strcmp(img.pcal.name, "Temperature") != 0 || strcmp(img.pcal.unit_name, "K") != 0 ||
        strcmp(img.pcal.params[0], "273.15") != 0 || strcmp(img.pcal.params[1], "100.0") != 0 ||
        img.pcal.x0 != 0 || img.pcal.x1 != 255 || img.pcal.equation_type != 0u) {
        png_free_image(&img);
        return 963;
    }

    err = png_pcal_map_stored_to_physical(img.source_bit_depth, &img.pcal, 128u, &physical);
    if (err != PNG_DEC_OK || physical < png_fixed89_from_ratio(32334, 100) ||
        physical > png_fixed89_from_ratio(32335, 100)) {
        png_free_image(&img);
        return 964;
    }

    png_free_image(&img);
    return 0;
}


static int test_hdr_exif_dsig_roundtrip(void)
{
    png_u8 rgba[8u] = { 1u, 2u, 3u, 255u, 4u, 5u, 6u, 128u };
    png_encode_options opt;
    png_decode_options dopt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    int err;
    png_dsig_entry dsig[2];
    static const png_u8 exif_data[] = { 'I','I',0x2Au,0x00u,0x08u,0x00u,0x00u,0x00u };
    static const png_u8 dsig_begin[] = { 0x30u,0x82u,0x01u,0x0Au,0xAAu };
    static const png_u8 dsig_end[]   = { 0x30u,0x82u,0x01u,0x0Bu,0xBBu,0xCCu };

    memset(dsig, 0, sizeof(dsig));
    dsig[0].cms_data = (png_u8*)dsig_begin;
    dsig[0].size = (png_u32)sizeof(dsig_begin);
    dsig[0].location = PNG_CHUNK_POS_AFTER_IHDR;
    dsig[1].cms_data = (png_u8*)dsig_end;
    dsig[1].size = (png_u32)sizeof(dsig_end);
    dsig[1].location = PNG_CHUNK_POS_BEFORE_IEND;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.write_cICP = 1;
    opt.cicp.colour_primaries = 1u;
    opt.cicp.transfer_function = 13u;
    opt.cicp.matrix_coefficients = 0u;
    opt.cicp.full_range_flag = 1u;
    opt.write_mDCV = 1;
    opt.mdcv.display_primaries_x[0] = png_fixed89_from_ratio(6400, 10000);
    opt.mdcv.display_primaries_y[0] = png_fixed89_from_ratio(3300, 10000);
    opt.mdcv.display_primaries_x[1] = png_fixed89_from_ratio(3000, 10000);
    opt.mdcv.display_primaries_y[1] = png_fixed89_from_ratio(6000, 10000);
    opt.mdcv.display_primaries_x[2] = png_fixed89_from_ratio(1500, 10000);
    opt.mdcv.display_primaries_y[2] = png_fixed89_from_ratio(600, 10000);
    opt.mdcv.white_point_x = png_fixed89_from_ratio(3127, 10000);
    opt.mdcv.white_point_y = png_fixed89_from_ratio(3290, 10000);
    opt.mdcv.max_luminance = png_fixed89_from_ratio(1000, 1);
    opt.mdcv.min_luminance = png_fixed89_from_ratio(5, 100);
    opt.write_cLLI = 1;
    opt.clli.max_content_light_level = png_fixed89_from_ratio(1000, 1);
    opt.clli.max_frame_average_light_level = png_fixed89_from_ratio(400, 1);
    opt.write_eXIf = 1;
    opt.exif_profile = exif_data;
    opt.exif_profile_size = (png_u32)sizeof(exif_data);
    opt.dsig_chunks = dsig;
    opt.dsig_chunk_count = 2u;

    err = png_encode_memory_ex_zlib(rgba, 2u, 1u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 971;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.keep_unknown_chunks = PNG_DEC_KEEP_UNKNOWN_ALL;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 972;

    if (!img.has_cICP || img.cicp.colour_primaries != 1u || img.cicp.transfer_function != 13u ||
        img.cicp.matrix_coefficients != 0u || img.cicp.full_range_flag != 1u) {
        png_free_image(&img);
        return 973;
    }
    if (!img.has_mDCV || !img.has_cLLI || !img.has_eXIf) {
        png_free_image(&img);
        return 974;
    }
    if (img.mdcv.max_luminance < png_fixed89_from_ratio(99999, 100) ||
        img.mdcv.max_luminance > png_fixed89_from_ratio(100001, 100) ||
        img.mdcv.min_luminance < png_fixed89_from_ratio(49, 1000) ||
        img.mdcv.min_luminance > png_fixed89_from_ratio(51, 1000) ||
        img.clli.max_content_light_level < png_fixed89_from_ratio(99999, 100) ||
        img.clli.max_content_light_level > png_fixed89_from_ratio(100001, 100) ||
        img.clli.max_frame_average_light_level < png_fixed89_from_ratio(39999, 100) ||
        img.clli.max_frame_average_light_level > png_fixed89_from_ratio(40001, 100)) {
        png_free_image(&img);
        return 975;
    }
    if (img.exif_profile_size != (png_u32)sizeof(exif_data) ||
        memcmp(img.exif_profile, exif_data, sizeof(exif_data)) != 0) {
        png_free_image(&img);
        return 976;
    }
    if (img.dsig_chunk_count != 2u || !img.dsig_chunks) {
        png_free_image(&img);
        return 977;
    }
    if (img.dsig_chunks[0].location != PNG_CHUNK_POS_AFTER_IHDR ||
        img.dsig_chunks[1].location != PNG_CHUNK_POS_BEFORE_IEND ||
        img.dsig_chunks[0].size != (png_u32)sizeof(dsig_begin) ||
        img.dsig_chunks[1].size != (png_u32)sizeof(dsig_end) ||
        memcmp(img.dsig_chunks[0].cms_data, dsig_begin, sizeof(dsig_begin)) != 0 ||
        memcmp(img.dsig_chunks[1].cms_data, dsig_end, sizeof(dsig_end)) != 0) {
        png_free_image(&img);
        return 978;
    }

    png_free_image(&img);
    return 0;
}

static int test_modern_chunk_validation(void)
{
    png_u8 rgba[4u] = { 0u, 0u, 0u, 255u };
    png_encode_options opt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    int err;
    png_dsig_entry dsig_one;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.write_mDCV = 1;
    opt.mdcv.display_primaries_x[0] = png_fixed89_from_ratio(64, 100);
    opt.mdcv.display_primaries_y[0] = png_fixed89_from_ratio(33, 100);
    opt.mdcv.display_primaries_x[1] = png_fixed89_from_ratio(30, 100);
    opt.mdcv.display_primaries_y[1] = png_fixed89_from_ratio(60, 100);
    opt.mdcv.display_primaries_x[2] = png_fixed89_from_ratio(15, 100);
    opt.mdcv.display_primaries_y[2] = png_fixed89_from_ratio(6, 100);
    opt.mdcv.white_point_x = png_fixed89_from_ratio(3127, 10000);
    opt.mdcv.white_point_y = png_fixed89_from_ratio(3290, 10000);
    opt.mdcv.max_luminance = png_fixed89_from_ratio(1000, 1);
    opt.mdcv.min_luminance = png_fixed89_from_ratio(1, 100);
    err = png_encode_memory_ex_zlib(rgba, 1u, 1u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_ERR_FORMAT)
        return 979;

    memset(&dsig_one, 0, sizeof(dsig_one));
    dsig_one.cms_data = rgba;
    dsig_one.size = 4u;
    dsig_one.location = PNG_CHUNK_POS_AFTER_IHDR;
    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.dsig_chunks = &dsig_one;
    opt.dsig_chunk_count = 1u;
    err = png_encode_memory_ex_zlib(rgba, 1u, 1u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_ERR_FORMAT)
        return 980;

    return 0;
}

static int test_endian_16bit_paths(void)
{
    png_u32 width = 3u, height = 2u;
    png_u8 rgba16_be[3u * 2u * 8u];
    png_u8 rgba16_le[3u * 2u * 8u];
    png_encode_options opt;
    png_decode_options dopt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    int err;

    fill_rgba16_pattern(rgba16_be, width, height);
    memcpy(rgba16_le, rgba16_be, sizeof(rgba16_be));
    png_swap_16_buffer(rgba16_le, (png_u32)sizeof(rgba16_le));

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 16u;
    opt.input_16bit_little_endian = 1u;
    err = png_encode_memory_ex_zlib(rgba16_le, width, height, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 981;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA16;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    if (err != PNG_DEC_OK) {
        png_free_file(png_data);
        return 982;
    }
    if (img.pixel_rowbytes != width * 8u || memcmp(img.pixels, rgba16_be, sizeof(rgba16_be)) != 0) {
        png_free_image(&img);
        png_free_file(png_data);
        return 983;
    }
    png_free_image(&img);

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA16;
    dopt.transform_flags = PNG_DEC_TRANSFORM_SWAP_16_ENDIAN;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 984;
    if (img.pixel_rowbytes != width * 8u || memcmp(img.pixels, rgba16_le, sizeof(rgba16_le)) != 0) {
        png_free_image(&img);
        return 985;
    }

    png_free_image(&img);
    return 0;
}


static int test_legacy_extension_chunks(void)
{
    png_u8 rgba[8u] = { 10u, 20u, 30u, 255u, 40u, 50u, 60u, 128u };
    png_encode_options opt;
    png_decode_options dopt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    int err;
    png_gifg_entry gifg;
    png_gifx_entry gifx;
    png_gift_entry gift;
    png_frac_entry frac;
    static const png_u8 gifx_data[] = { 'D','A','T','A' };
    static const png_u8 gift_text[] = { 'H','E','L','L','O' };
    static const png_u8 frac_data[] = { 0x01u, 0x23u, 0x45u, 0x67u, 0x89u };
    static const png_u8 suggested_plte[] = { 10u, 20u, 30u };

    memset(&gifg, 0, sizeof(gifg));
    gifg.disposal_method = 3u;
    gifg.user_input_flag = 1u;
    gifg.delay_time_cs = 25u;
    gifg.location = PNG_CHUNK_POS_AFTER_IHDR;

    memset(&gifx, 0, sizeof(gifx));
    memcpy(gifx.application_identifier, "APPNAME1", 8u);
    gifx.application_identifier[8] = '\0';
    gifx.authentication_code[0] = 1u;
    gifx.authentication_code[1] = 2u;
    gifx.authentication_code[2] = 3u;
    gifx.application_data = (png_u8*)gifx_data;
    gifx.application_data_size = (png_u32)sizeof(gifx_data);
    gifx.location = PNG_CHUNK_POS_AFTER_PLTE;

    memset(&gift, 0, sizeof(gift));
    gift.grid_left = 1;
    gift.grid_top = 2;
    gift.grid_width = 3u;
    gift.grid_height = 4u;
    gift.cell_width = 8u;
    gift.cell_height = 9u;
    gift.foreground_rgb[0] = 10u;
    gift.foreground_rgb[1] = 20u;
    gift.foreground_rgb[2] = 30u;
    gift.background_rgb[0] = 200u;
    gift.background_rgb[1] = 210u;
    gift.background_rgb[2] = 220u;
    gift.text_data = (png_u8*)gift_text;
    gift.text_data_size = (png_u32)sizeof(gift_text);
    gift.location = PNG_CHUNK_POS_AFTER_IDAT;

    memset(&frac, 0, sizeof(frac));
    frac.data = (png_u8*)frac_data;
    frac.size = (png_u32)sizeof(frac_data);
    frac.location = PNG_CHUNK_POS_AFTER_IDAT;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.gifg_chunks = &gifg;
    opt.gifg_chunk_count = 1u;
    opt.palette = suggested_plte;
    opt.palette_entries = 1u;
    opt.gifx_chunks = &gifx;
    opt.gifx_chunk_count = 1u;
    opt.gift_chunks = &gift;
    opt.gift_chunk_count = 1u;
    opt.frac_chunks = &frac;
    opt.frac_chunk_count = 1u;

    err = png_encode_memory_ex_zlib(rgba, 2u, 1u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 986;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 987;

    if (img.gifg_chunk_count != 1u || img.gifx_chunk_count != 1u ||
        img.gift_chunk_count != 1u || img.frac_chunk_count != 1u) {
        png_free_image(&img);
        return 988;
    }

    if (img.gifg_chunks[0].disposal_method != 3u || img.gifg_chunks[0].user_input_flag != 1u ||
        img.gifg_chunks[0].delay_time_cs != 25u || img.gifg_chunks[0].location != PNG_CHUNK_POS_AFTER_IHDR) {
        png_free_image(&img);
        return 989;
    }

    if (strcmp(img.gifx_chunks[0].application_identifier, "APPNAME1") != 0 ||
        img.gifx_chunks[0].authentication_code[0] != 1u ||
        img.gifx_chunks[0].authentication_code[1] != 2u ||
        img.gifx_chunks[0].authentication_code[2] != 3u ||
        img.gifx_chunks[0].application_data_size != (png_u32)sizeof(gifx_data) ||
        img.gifx_chunks[0].location != PNG_CHUNK_POS_AFTER_PLTE ||
        memcmp(img.gifx_chunks[0].application_data, gifx_data, sizeof(gifx_data)) != 0) {
        png_free_image(&img);
        return 990;
    }

    if (img.gift_chunks[0].grid_left != 1 || img.gift_chunks[0].grid_top != 2 ||
        img.gift_chunks[0].grid_width != 3u || img.gift_chunks[0].grid_height != 4u ||
        img.gift_chunks[0].cell_width != 8u || img.gift_chunks[0].cell_height != 9u ||
        img.gift_chunks[0].foreground_rgb[0] != 10u || img.gift_chunks[0].foreground_rgb[1] != 20u || img.gift_chunks[0].foreground_rgb[2] != 30u ||
        img.gift_chunks[0].background_rgb[0] != 200u || img.gift_chunks[0].background_rgb[1] != 210u || img.gift_chunks[0].background_rgb[2] != 220u ||
        img.gift_chunks[0].text_data_size != (png_u32)sizeof(gift_text) ||
        img.gift_chunks[0].location != PNG_CHUNK_POS_AFTER_IDAT ||
        memcmp(img.gift_chunks[0].text_data, gift_text, sizeof(gift_text)) != 0) {
        png_free_image(&img);
        return 991;
    }

    if (img.frac_chunks[0].size != (png_u32)sizeof(frac_data) ||
        img.frac_chunks[0].location != PNG_CHUNK_POS_AFTER_IDAT ||
        memcmp(img.frac_chunks[0].data, frac_data, sizeof(frac_data)) != 0) {
        png_free_image(&img);
        return 992;
    }

    if (memcmp(img.rgba, rgba, sizeof(rgba)) != 0) {
        png_free_image(&img);
        return 993;
    }

    png_free_image(&img);
    return 0;
}

static int test_public_input_encode_pipeline(void)
{
    png_u8 bgra[8u] = { 30u, 20u, 10u, 40u, 70u, 60u, 50u, 80u };
    png_u8 expected_rgba[8u] = { 10u, 20u, 30u, 215u, 50u, 60u, 70u, 175u };
    png_encode_options opt;
    png_decode_options dopt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_image img;
    int err;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.input_format = PNG_OUTPUT_BGRA8;
    opt.input_transform_flags = PNG_ENC_TRANSFORM_INVERT_ALPHA;

    err = png_encode_memory_ex_zlib(bgra, 2u, 1u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 994;

    memset(&img, 0, sizeof(img));
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_decode_memory_ex_zlib(png_data, png_size, &dopt, &img);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 995;

    if (img.output_format != PNG_OUTPUT_RGBA8 || img.pixel_rowbytes != 8u || memcmp(img.rgba, expected_rgba, sizeof(expected_rgba)) != 0) {
        png_free_image(&img);
        return 996;
    }

    png_free_image(&img);
    return 0;
}

static int test_encode_image_ex_wrapper(void)
{
    png_u8 rgba[12u] = { 9u, 19u, 29u, 39u, 49u, 59u, 69u, 79u, 89u, 99u, 109u, 119u };
    png_encode_options opt;
    png_decode_options dopt;
    png_u8* png_data0 = 0;
    png_u32 png_size0 = 0u;
    png_u8* png_data1 = 0;
    png_u32 png_size1 = 0u;
    png_image mid;
    png_image out;
    int err;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    err = png_encode_memory_ex_zlib(rgba, 3u, 1u, &opt, &png_data0, &png_size0);
    if (err != PNG_DEC_OK)
        return 997;

    memset(&mid, 0, sizeof(mid));
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_BGRA8;
    err = png_decode_memory_ex_zlib(png_data0, png_size0, &dopt, &mid);
    png_free_file(png_data0);
    if (err != PNG_DEC_OK)
        return 998;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    err = png_encode_image_ex_zlib(&mid, &opt, &png_data1, &png_size1);
    png_free_image(&mid);
    if (err != PNG_DEC_OK)
        return 999;

    memset(&out, 0, sizeof(out));
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_decode_memory_ex_zlib(png_data1, png_size1, &dopt, &out);
    png_free_file(png_data1);
    if (err != PNG_DEC_OK)
        return 1000;

    if (out.pixel_rowbytes != 12u || memcmp(out.rgba, rgba, sizeof(rgba)) != 0) {
        png_free_image(&out);
        return 1001;
    }

    png_free_image(&out);
    return 0;
}


static void put_pixel_rgba(png_u8* rgba, png_u32 width, png_u32 x, png_u32 y,
                           png_u8 r, png_u8 g, png_u8 b, png_u8 a)
{
    png_u8* p = rgba + (y * width + x) * 4u;
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = a;
}

static void put_pixel_rgba_stride(png_u8* rgba, png_u32 stride, png_u32 x, png_u32 y,
                                  png_u8 r, png_u8 g, png_u8 b, png_u8 a)
{
    png_u8* p = rgba + y * stride + x * 4u;
    p[0] = r;
    p[1] = g;
    p[2] = b;
    p[3] = a;
}

static int test_progressive_pause_resume_and_skip(void)
{
    png_u8 rgba[2u * 2u * 4u];
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_progressive_callbacks cb;
    png_decode_options dopt;
    png_decoder* dec = 0;
    png_image img;
    png_u32 pending;
    png_u32 replay_off;
    int err;

    memset(rgba, 0, sizeof(rgba));
    put_pixel_rgba(rgba, 2u, 0u, 0u, 10u, 20u, 30u, 255u);
    put_pixel_rgba(rgba, 2u, 1u, 0u, 40u, 50u, 60u, 255u);
    put_pixel_rgba(rgba, 2u, 0u, 1u, 70u, 80u, 90u, 255u);
    put_pixel_rgba(rgba, 2u, 1u, 1u, 100u, 110u, 120u, 255u);

    err = png_encode_rgba8_memory_zlib(rgba, 2u, 2u, 6, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 1520;

    g_rows = 0;
    g_end = 0;
    g_first_row_size = 0u;
    g_pause_requested_once = 0;
    g_pause_requested_bytes = 0u;
    g_pause_dec = 0;

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK)
    {
        png_free_file(png_data);
        return 1521;
    }
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 1522;
    }

    memset(&cb, 0, sizeof(cb));
    cb.row_fn = pause_row_cb;
    cb.end_fn = end_cb;
    err = png_decoder_set_callbacks(dec, &cb);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 1523;
    }

    g_pause_dec = dec;
    err = png_decoder_feed(dec, png_data, png_size);
    if (err != PNG_DEC_PAUSED || !png_decoder_is_paused(dec) || png_decoder_pending_bytes(dec) == 0u ||
        g_rows != 1 || g_end != 0 || g_pause_requested_bytes == 0u)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 1524;
    }

    err = png_decoder_feed(dec, 0, 0u);
    if (err != PNG_DEC_DONE || png_decoder_is_paused(dec) || g_rows != 2 || g_end != 1)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 1525;
    }

    memset(&img, 0, sizeof(img));
    err = png_decoder_take_image(dec, &img);
    png_decoder_free(dec);
    g_pause_dec = 0;
    if (err != PNG_DEC_OK || img.width != 2u || img.height != 2u ||
        img.pixel_rowbytes != 8u || memcmp(img.pixels, rgba, sizeof(rgba)) != 0)
    {
        png_free_image(&img);
        png_free_file(png_data);
        return 1526;
    }
    png_free_image(&img);

    g_rows = 0;
    g_end = 0;
    g_first_row_size = 0u;
    g_pause_requested_once = 0;
    g_pause_requested_bytes = 0u;
    g_pause_dec = 0;

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK)
    {
        png_free_file(png_data);
        return 1527;
    }
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 1528;
    }
    memset(&cb, 0, sizeof(cb));
    cb.row_fn = pause_row_cb;
    cb.end_fn = end_cb;
    err = png_decoder_set_callbacks(dec, &cb);
    if (err != PNG_DEC_OK)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 1529;
    }

    g_pause_dec = dec;
    err = png_decoder_feed(dec, png_data, png_size);
    if (err != PNG_DEC_PAUSED || !png_decoder_is_paused(dec))
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 1530;
    }

    pending = png_decoder_pending_bytes(dec);
    if (pending == 0u || pending > png_size)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 1531;
    }
    replay_off = png_size - pending;
    if (png_decoder_process_data_skip(dec) != pending || png_decoder_pending_bytes(dec) != 0u)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 1532;
    }

    err = png_decoder_feed(dec, png_data + replay_off, pending);
    if (err != PNG_DEC_DONE || g_rows != 2 || g_end != 1)
    {
        png_decoder_free(dec);
        png_free_file(png_data);
        return 1533;
    }

    memset(&img, 0, sizeof(img));
    err = png_decoder_take_image(dec, &img);
    png_decoder_free(dec);
    g_pause_dec = 0;
    png_free_file(png_data);
    if (err != PNG_DEC_OK || img.width != 2u || img.height != 2u ||
        img.pixel_rowbytes != 8u || memcmp(img.pixels, rgba, sizeof(rgba)) != 0)
    {
        png_free_image(&img);
        return 1534;
    }
    png_free_image(&img);
    return 0;
}

static int test_apng_progressive_pause_resume(void)
{
    png_u8 frame0[3u * 2u * 4u];
    png_u8 frame1[1u * 2u * 4u];
    png_u8 expect0[3u * 2u * 4u];
    png_u8 expect1[3u * 2u * 4u];
    png_apng_encode_frame frames[2];
    png_encode_options opt;
    png_decode_options dopt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_apng_decoder* dec = 0;
    png_apng_progressive_callbacks cb;
    png_apng apng;
    int err;

    memset(frame0, 0, sizeof(frame0));
    memset(frame1, 0, sizeof(frame1));
    memset(expect0, 0, sizeof(expect0));
    memset(expect1, 0, sizeof(expect1));
    memset(frames, 0, sizeof(frames));
    memset(&apng, 0, sizeof(apng));

    put_pixel_rgba(frame0, 3u, 0u, 0u, 10u, 20u, 30u, 255u);
    put_pixel_rgba(frame0, 3u, 1u, 0u, 40u, 50u, 60u, 255u);
    put_pixel_rgba(frame0, 3u, 2u, 0u, 70u, 80u, 90u, 255u);
    put_pixel_rgba(frame0, 3u, 0u, 1u, 15u, 25u, 35u, 255u);
    put_pixel_rgba(frame0, 3u, 1u, 1u, 45u, 55u, 65u, 255u);
    put_pixel_rgba(frame0, 3u, 2u, 1u, 75u, 85u, 95u, 255u);
    put_pixel_rgba(frame1, 1u, 0u, 0u, 0u, 0u, 255u, 255u);
    put_pixel_rgba(frame1, 1u, 0u, 1u, 0u, 255u, 0u, 255u);

    memcpy(expect0, frame0, sizeof(expect0));
    memcpy(expect1, frame0, sizeof(expect1));
    put_pixel_rgba(expect1, 3u, 1u, 0u, 0u, 0u, 255u, 255u);
    put_pixel_rgba(expect1, 3u, 1u, 1u, 0u, 255u, 0u, 255u);

    frames[0].pixels = frame0;
    frames[0].width = 3u;
    frames[0].height = 2u;
    frames[0].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[0].blend_op = PNG_APNG_BLEND_OP_SOURCE;
    frames[1].pixels = frame1;
    frames[1].width = 1u;
    frames[1].height = 2u;
    frames[1].x_offset = 1u;
    frames[1].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[1].blend_op = PNG_APNG_BLEND_OP_SOURCE;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    err = png_encode_apng_memory_ex_zlib(frames, 2u, 3u, 2u, 0u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 1535;

    g_apng_info = 0;
    g_apng_frame_infos = 0;
    g_apng_frame_rows = 0;
    g_apng_frame_ends = 0;
    g_apng_end = 0;
    memset(g_apng_rows_per_frame, 0, sizeof(g_apng_rows_per_frame));
    g_apng_pause_requested_once = 0;
    g_apng_pause_requested_bytes = 0u;
    g_pause_apng_dec = 0;

    err = png_apng_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK)
    {
        png_free_file(png_data);
        return 1536;
    }
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_apng_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1537;
    }

    memset(&cb, 0, sizeof(cb));
    cb.info_fn = apng_info_cb;
    cb.frame_info_fn = apng_frame_info_cb;
    cb.frame_row_fn = apng_pause_frame_row_cb;
    cb.frame_end_fn = apng_frame_end_cb;
    cb.end_fn = apng_end_cb;
    err = png_apng_decoder_set_callbacks(dec, &cb);
    if (err != PNG_DEC_OK)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1538;
    }

    g_pause_apng_dec = dec;
    err = png_apng_decoder_feed(dec, png_data, png_size);
    if (err != PNG_DEC_PAUSED || !png_apng_decoder_is_paused(dec) ||
        png_apng_decoder_pending_bytes(dec) == 0u || g_apng_frame_rows != 1 ||
        g_apng_frame_ends != 0 || g_apng_pause_requested_bytes == 0u)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1539;
    }

    err = png_apng_decoder_feed(dec, 0, 0u);
    if (err != PNG_DEC_DONE || png_apng_decoder_is_paused(dec) ||
        g_apng_info != 1 || g_apng_frame_infos != 2 || g_apng_frame_ends != 2 || g_apng_end != 1)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1540;
    }

    err = png_apng_decoder_take_animation(dec, &apng);
    png_apng_decoder_free(dec);
    g_pause_apng_dec = 0;
    png_free_file(png_data);
    if (err != PNG_DEC_OK || apng.frame_count != 2u || !apng.has_default_image ||
        memcmp(apng.frames[0].pixels, expect0, sizeof(expect0)) != 0 ||
        memcmp(apng.frames[1].pixels, expect1, sizeof(expect1)) != 0)
    {
        png_free_apng(&apng);
        return 1541;
    }

    png_free_apng(&apng);
    return 0;
}

static int test_apng_platform_roundtrip(void)
{
    png_u8 frame0[3u * 2u * 4u];
    png_u8 frame1[1u * 2u * 4u];
    png_u8 frame2[1u * 1u * 4u];
    png_u8 frame3[1u * 1u * 4u];
    png_u8 expect0[3u * 2u * 4u];
    png_u8 expect1[3u * 2u * 4u];
    png_u8 expect2[3u * 2u * 4u];
    png_apng_encode_frame frames[4];
    png_encode_options opt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_apng apng;
    png_decode_options dopt;
    int err;

    memset(frame0, 0, sizeof(frame0));
    memset(frame1, 0, sizeof(frame1));
    memset(frame2, 0, sizeof(frame2));
    memset(frame3, 0, sizeof(frame3));
    memset(&apng, 0, sizeof(apng));

    put_pixel_rgba(frame0, 3u, 0u, 0u, 10u, 20u, 30u, 255u);
    put_pixel_rgba(frame0, 3u, 1u, 0u, 40u, 50u, 60u, 255u);
    put_pixel_rgba(frame0, 3u, 2u, 0u, 70u, 80u, 90u, 255u);
    put_pixel_rgba(frame0, 3u, 0u, 1u, 15u, 25u, 35u, 255u);
    put_pixel_rgba(frame0, 3u, 1u, 1u, 45u, 55u, 65u, 255u);
    put_pixel_rgba(frame0, 3u, 2u, 1u, 75u, 85u, 95u, 255u);

    put_pixel_rgba(frame1, 1u, 0u, 0u, 0u, 0u, 255u, 255u);
    put_pixel_rgba(frame1, 1u, 0u, 1u, 0u, 255u, 0u, 255u);

    put_pixel_rgba(frame2, 1u, 0u, 0u, 255u, 0u, 0u, 128u);
    put_pixel_rgba(frame3, 1u, 0u, 0u, 0u, 0u, 0u, 0u);

    memcpy(expect0, frame0, sizeof(expect0));
    memcpy(expect1, frame0, sizeof(expect1));
    put_pixel_rgba(expect1, 3u, 1u, 0u, 0u, 0u, 255u, 255u);
    put_pixel_rgba(expect1, 3u, 1u, 1u, 0u, 255u, 0u, 255u);
    memcpy(expect2, expect1, sizeof(expect2));
    put_pixel_rgba(expect2, 3u, 2u, 1u, 165u, 42u, 47u, 255u);

    memset(frames, 0, sizeof(frames));
    frames[0].pixels = frame0;
    frames[0].width = 3u;
    frames[0].height = 2u;
    frames[0].delay_num = 1u;
    frames[0].delay_den = 10u;
    frames[0].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[0].blend_op = PNG_APNG_BLEND_OP_SOURCE;

    frames[1].pixels = frame1;
    frames[1].width = 1u;
    frames[1].height = 2u;
    frames[1].x_offset = 1u;
    frames[1].y_offset = 0u;
    frames[1].delay_num = 1u;
    frames[1].delay_den = 10u;
    frames[1].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[1].blend_op = PNG_APNG_BLEND_OP_SOURCE;

    frames[2].pixels = frame2;
    frames[2].width = 1u;
    frames[2].height = 1u;
    frames[2].x_offset = 2u;
    frames[2].y_offset = 1u;
    frames[2].delay_num = 1u;
    frames[2].delay_den = 10u;
    frames[2].dispose_op = PNG_APNG_DISPOSE_OP_PREVIOUS;
    frames[2].blend_op = PNG_APNG_BLEND_OP_OVER;

    frames[3].pixels = frame3;
    frames[3].width = 1u;
    frames[3].height = 1u;
    frames[3].x_offset = 2u;
    frames[3].y_offset = 1u;
    frames[3].delay_num = 1u;
    frames[3].delay_den = 10u;
    frames[3].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[3].blend_op = PNG_APNG_BLEND_OP_OVER;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;

    err = png_encode_apng_memory_ex_zlib(frames, 4u, 3u, 2u, 7u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 1801;

    {
        FILE* f = fopen("test_apng_anim.png", "wb");
        if (f) {
            fwrite(png_data, 1, png_size, f);
            fclose(f);
        }
    }

    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_decode_apng_memory_ex_zlib(png_data, png_size, &dopt, &apng);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 1802;

    if (apng.frame_count != 4u || apng.num_plays != 7u || !apng.has_default_image)
    {
        png_free_apng(&apng);
        return 1803;
    }

    if (!apng.default_image.rgba || memcmp(apng.default_image.rgba, expect0, sizeof(expect0)) != 0)
    {
        png_free_apng(&apng);
        return 1804;
    }

    if (!apng.frames[0].pixels || memcmp(apng.frames[0].pixels, expect0, sizeof(expect0)) != 0)
    {
        png_free_apng(&apng);
        return 1805;
    }

    if (memcmp(apng.frames[1].pixels, expect1, sizeof(expect1)) != 0)
    {
        png_free_apng(&apng);
        return 1806;
    }

    if (memcmp(apng.frames[2].pixels, expect2, sizeof(expect2)) != 0)
    {
        png_free_apng(&apng);
        return 1807;
    }

    if (memcmp(apng.frames[3].pixels, expect1, sizeof(expect1)) != 0)
    {
        png_free_apng(&apng);
        return 1808;
    }

    png_free_apng(&apng);
    return 0;
}

static int test_apng_progressive_callbacks(void)
{
    png_u8 frame0[3u * 2u * 4u];
    png_u8 frame1[1u * 2u * 4u];
    png_u8 frame2[1u * 1u * 4u];
    png_u8 frame3[1u * 1u * 4u];
    png_u8 expect0[3u * 2u * 4u];
    png_u8 expect1[3u * 2u * 4u];
    png_u8 expect2[3u * 2u * 4u];
    png_apng_encode_frame frames[4];
    png_encode_options opt;
    png_decode_options dopt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_u32 cut;
    png_u32 pos;
    int saw_live_before_frame_end;
    png_apng_decoder* dec = 0;
    png_apng_progressive_callbacks cb;
    png_apng apng;
    int err;

    memset(frame0, 0, sizeof(frame0));
    memset(frame1, 0, sizeof(frame1));
    memset(frame2, 0, sizeof(frame2));
    memset(frame3, 0, sizeof(frame3));
    memset(frames, 0, sizeof(frames));
    memset(&apng, 0, sizeof(apng));

    put_pixel_rgba(frame0, 3u, 0u, 0u, 10u, 20u, 30u, 255u);
    put_pixel_rgba(frame0, 3u, 1u, 0u, 40u, 50u, 60u, 255u);
    put_pixel_rgba(frame0, 3u, 2u, 0u, 70u, 80u, 90u, 255u);
    put_pixel_rgba(frame0, 3u, 0u, 1u, 15u, 25u, 35u, 255u);
    put_pixel_rgba(frame0, 3u, 1u, 1u, 45u, 55u, 65u, 255u);
    put_pixel_rgba(frame0, 3u, 2u, 1u, 75u, 85u, 95u, 255u);

    put_pixel_rgba(frame1, 1u, 0u, 0u, 0u, 0u, 255u, 255u);
    put_pixel_rgba(frame1, 1u, 0u, 1u, 0u, 255u, 0u, 255u);

    put_pixel_rgba(frame2, 1u, 0u, 0u, 255u, 0u, 0u, 128u);
    put_pixel_rgba(frame3, 1u, 0u, 0u, 0u, 0u, 0u, 0u);

    memcpy(expect0, frame0, sizeof(expect0));
    memcpy(expect1, frame0, sizeof(expect1));
    put_pixel_rgba(expect1, 3u, 1u, 0u, 0u, 0u, 255u, 255u);
    put_pixel_rgba(expect1, 3u, 1u, 1u, 0u, 255u, 0u, 255u);
    memcpy(expect2, expect1, sizeof(expect2));
    put_pixel_rgba(expect2, 3u, 2u, 1u, 165u, 42u, 47u, 255u);

    frames[0].pixels = frame0;
    frames[0].width = 3u;
    frames[0].height = 2u;
    frames[0].delay_num = 1u;
    frames[0].delay_den = 10u;
    frames[0].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[0].blend_op = PNG_APNG_BLEND_OP_SOURCE;

    frames[1].pixels = frame1;
    frames[1].width = 1u;
    frames[1].height = 2u;
    frames[1].x_offset = 1u;
    frames[1].y_offset = 0u;
    frames[1].delay_num = 1u;
    frames[1].delay_den = 10u;
    frames[1].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[1].blend_op = PNG_APNG_BLEND_OP_SOURCE;

    frames[2].pixels = frame2;
    frames[2].width = 1u;
    frames[2].height = 1u;
    frames[2].x_offset = 2u;
    frames[2].y_offset = 1u;
    frames[2].delay_num = 1u;
    frames[2].delay_den = 10u;
    frames[2].dispose_op = PNG_APNG_DISPOSE_OP_PREVIOUS;
    frames[2].blend_op = PNG_APNG_BLEND_OP_OVER;

    frames[3].pixels = frame3;
    frames[3].width = 1u;
    frames[3].height = 1u;
    frames[3].x_offset = 2u;
    frames[3].y_offset = 1u;
    frames[3].delay_num = 1u;
    frames[3].delay_den = 10u;
    frames[3].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[3].blend_op = PNG_APNG_BLEND_OP_OVER;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    err = png_encode_apng_memory_ex_zlib(frames, 4u, 3u, 2u, 7u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 1851;

    cut = find_nth_chunk_end(png_data, png_size, "fcTL", 2);
    if (cut == 0u || cut >= png_size)
    {
        png_free_file(png_data);
        return 1852;
    }

    g_apng_info = 0;
    g_apng_frame_infos = 0;
    g_apng_frame_rows = 0;
    g_apng_frame_ends = 0;
    g_apng_end = 0;
    memset(g_apng_rows_per_frame, 0, sizeof(g_apng_rows_per_frame));
    g_apng_last_rowbytes = 0u;
    g_apng_first_row_size = 0u;
    g_apng_info_width = 0u;
    g_apng_info_height = 0u;
    g_apng_declared_frames = 0u;
    g_apng_last_pass = 0;
    g_apng_max_pass = 0;
    g_apng_pass_rows = 0;

    err = png_apng_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK)
    {
        png_free_file(png_data);
        return 1853;
    }

    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_apng_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1854;
    }

    memset(&cb, 0, sizeof(cb));
    cb.info_fn = apng_info_cb;
    cb.frame_info_fn = apng_frame_info_cb;
    cb.frame_row_fn = apng_frame_row_cb;
    cb.frame_row_pass_fn = apng_frame_row_pass_cb;
    cb.frame_end_fn = apng_frame_end_cb;
    cb.end_fn = apng_end_cb;
    err = png_apng_decoder_set_callbacks(dec, &cb);
    if (err != PNG_DEC_OK)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1855;
    }

    saw_live_before_frame_end = 0;
    pos = 0u;
    while (pos < cut && g_apng_frame_rows == 0)
    {
        err = png_apng_decoder_feed(dec, png_data + pos, 1u);
        if (err != PNG_DEC_OK)
        {
            png_apng_decoder_free(dec);
            png_free_file(png_data);
            return 1856;
        }
        pos += 1u;
        if (g_apng_frame_rows != 0)
            saw_live_before_frame_end = (g_apng_frame_ends == 0);
    }

    if (!saw_live_before_frame_end || g_apng_info != 1 || g_apng_info_width != 3u ||
        g_apng_info_height != 2u || g_apng_declared_frames != 4u ||
        g_apng_first_row_size != 12u || memcmp(g_apng_first_row, expect0, 12u) != 0)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1857;
    }

    if (pos < cut)
    {
        err = png_apng_decoder_feed(dec, png_data + pos, cut - pos);
        if (err != PNG_DEC_OK)
        {
            png_apng_decoder_free(dec);
            png_free_file(png_data);
            return 1858;
        }
    }

    if (g_apng_frame_ends != 1 || g_apng_rows_per_frame[0] != 2u || g_apng_end != 0)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1859;
    }

    err = png_apng_decoder_feed(dec, png_data + cut, png_size - cut);
    if (err != PNG_DEC_DONE)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1858;
    }

    if (g_apng_frame_infos != 4 || g_apng_frame_ends != 4 || g_apng_frame_rows != 8 || g_apng_end != 1 || g_apng_last_rowbytes != 12u)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1859;
    }

    err = png_apng_decoder_take_animation(dec, &apng);
    png_apng_decoder_free(dec);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 1860;

    if (apng.frame_count != 4u || !apng.has_default_image ||
        memcmp(apng.frames[0].pixels, expect0, sizeof(expect0)) != 0 ||
        memcmp(apng.frames[1].pixels, expect1, sizeof(expect1)) != 0 ||
        memcmp(apng.frames[2].pixels, expect2, sizeof(expect2)) != 0 ||
        memcmp(apng.frames[3].pixels, expect1, sizeof(expect1)) != 0)
    {
        png_free_apng(&apng);
        return 1861;
    }

    png_free_apng(&apng);
    return 0;
}

static int test_apng_progressive_adam7_pass_callbacks(void)
{
    enum { W = 8, H = 8 };
    png_u8 frame0[W * H * 4u];
    png_u8 frame1[W * H * 4u];
    png_u8 expect1[W * H * 4u];
    png_apng_encode_frame frames[2];
    png_encode_options opt;
    png_decode_options dopt;
    png_apng_progressive_callbacks cb;
    png_apng_decoder* dec = 0;
    png_apng apng;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_u32 x, y;
    int err;

    memset(frame0, 0, sizeof(frame0));
    memset(frame1, 0, sizeof(frame1));
    memset(frames, 0, sizeof(frames));
    memset(&apng, 0, sizeof(apng));

    for (y = 0u; y < H; ++y)
    {
        for (x = 0u; x < W; ++x)
        {
            put_pixel_rgba(frame0, W, x, y,
                           (png_u8)(10u + x * 21u),
                           (png_u8)(20u + y * 17u),
                           (png_u8)(30u + x * 7u + y * 5u),
                           255u);
            put_pixel_rgba(frame1, W, x, y,
                           (png_u8)(200u - x * 13u),
                           (png_u8)(40u + y * 19u),
                           (png_u8)(80u + x * 9u),
                           (png_u8)(32u + ((x + y) % 6u) * 32u));
        }
    }

    blend_over_rgba8_pixels(frame0, frame1, expect1, W * H);

    frames[0].pixels = frame0;
    frames[0].width = W;
    frames[0].height = H;
    frames[0].delay_num = 1u;
    frames[0].delay_den = 30u;
    frames[0].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[0].blend_op = PNG_APNG_BLEND_OP_SOURCE;

    frames[1].pixels = frame1;
    frames[1].width = W;
    frames[1].height = H;
    frames[1].delay_num = 1u;
    frames[1].delay_den = 30u;
    frames[1].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[1].blend_op = PNG_APNG_BLEND_OP_OVER;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    opt.interlace_method = 1u;
    err = png_encode_apng_memory_ex_zlib(frames, 2u, W, H, 0u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 1862;

    g_apng_info = 0;
    g_apng_frame_infos = 0;
    g_apng_frame_rows = 0;
    g_apng_frame_ends = 0;
    g_apng_end = 0;
    memset(g_apng_rows_per_frame, 0, sizeof(g_apng_rows_per_frame));
    g_apng_last_rowbytes = 0u;
    g_apng_first_row_size = 0u;
    g_apng_info_width = 0u;
    g_apng_info_height = 0u;
    g_apng_declared_frames = 0u;
    g_apng_last_pass = 0;
    g_apng_max_pass = 0;
    g_apng_pass_rows = 0;

    err = png_apng_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK)
    {
        png_free_file(png_data);
        return 1863;
    }

    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_apng_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1864;
    }

    memset(&cb, 0, sizeof(cb));
    cb.info_fn = apng_info_cb;
    cb.frame_info_fn = apng_frame_info_cb;
    cb.frame_row_fn = apng_frame_row_cb;
    cb.frame_row_pass_fn = apng_frame_row_pass_cb;
    cb.frame_end_fn = apng_frame_end_cb;
    cb.end_fn = apng_end_cb;
    err = png_apng_decoder_set_callbacks(dec, &cb);
    if (err != PNG_DEC_OK)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1865;
    }

    err = png_apng_decoder_feed(dec, png_data, png_size);
    if (err != PNG_DEC_DONE)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1866;
    }

    if (g_apng_info != 1 || g_apng_info_width != W || g_apng_info_height != H ||
        g_apng_declared_frames != 2u || g_apng_frame_infos != 2 || g_apng_frame_ends != 2 ||
        g_apng_end != 1 || g_apng_pass_rows != g_apng_frame_rows || g_apng_max_pass < 7 ||
        g_apng_rows_per_frame[0] <= H || g_apng_rows_per_frame[1] <= H)
    {
        png_apng_decoder_free(dec);
        png_free_file(png_data);
        return 1867;
    }

    err = png_apng_decoder_take_animation(dec, &apng);
    png_apng_decoder_free(dec);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 1868;

    if (apng.frame_count != 2u ||
        memcmp(apng.frames[0].pixels, frame0, sizeof(frame0)) != 0 ||
        memcmp(apng.frames[1].pixels, expect1, sizeof(expect1)) != 0)
    {
        png_free_apng(&apng);
        return 1869;
    }

    png_free_apng(&apng);
    return 0;
}

static int test_apng_auto_optimizer(void)
{
    enum { W = 4, H = 3, STRIDE = 20 };
    png_u8 frame0[H * STRIDE];
    png_u8 frame1[H * STRIDE];
    png_u8 frame2[H * STRIDE];
    png_u8 frame3[H * STRIDE];
    png_u8 expect0[W * H * 4u];
    png_u8 expect1[W * H * 4u];
    png_u8 expect2[W * H * 4u];
    png_apng_encode_frame frames[4];
    png_encode_options opt;
    png_u8* png_naive = 0;
    png_u32 png_naive_size = 0u;
    png_u8* png_auto = 0;
    png_u32 png_auto_size = 0u;
    png_apng apng;
    png_decode_options dopt;
    int err;
    int i;

    memset(frame0, 0, sizeof(frame0));
    memset(frame1, 0, sizeof(frame1));
    memset(frame2, 0, sizeof(frame2));
    memset(frame3, 0, sizeof(frame3));
    memset(&apng, 0, sizeof(apng));

    put_pixel_rgba_stride(frame0, STRIDE, 0u, 0u,  10u,  20u,  30u, 255u);
    put_pixel_rgba_stride(frame0, STRIDE, 1u, 0u,  40u,  50u,  60u, 255u);
    put_pixel_rgba_stride(frame0, STRIDE, 2u, 0u,  70u,  80u,  90u, 255u);
    put_pixel_rgba_stride(frame0, STRIDE, 3u, 0u, 100u, 110u, 120u, 255u);
    put_pixel_rgba_stride(frame0, STRIDE, 0u, 1u,  15u,  25u,  35u, 255u);
    put_pixel_rgba_stride(frame0, STRIDE, 1u, 1u,  45u,  55u,  65u, 255u);
    put_pixel_rgba_stride(frame0, STRIDE, 2u, 1u,  75u,  85u,  95u, 255u);
    put_pixel_rgba_stride(frame0, STRIDE, 3u, 1u, 105u, 115u, 125u, 255u);
    put_pixel_rgba_stride(frame0, STRIDE, 0u, 2u,  12u,  22u,  32u, 255u);
    put_pixel_rgba_stride(frame0, STRIDE, 1u, 2u,  42u,  52u,  62u, 255u);
    put_pixel_rgba_stride(frame0, STRIDE, 2u, 2u,  72u,  82u,  92u, 255u);
    put_pixel_rgba_stride(frame0, STRIDE, 3u, 2u, 102u, 112u, 122u, 255u);

    memcpy(frame1, frame0, sizeof(frame0));
    memcpy(frame2, frame1, sizeof(frame1));

    put_pixel_rgba_stride(frame1, STRIDE, 2u, 1u, 255u,   0u,   0u, 255u);
    put_pixel_rgba_stride(frame2, STRIDE, 1u, 0u,   0u, 255u,   0u, 255u);
    put_pixel_rgba_stride(frame2, STRIDE, 2u, 0u,   0u,   0u, 255u, 255u);
    memcpy(frame3, frame2, sizeof(frame2));

    {
        png_u32 row;
        for (row = 0u; row < H; ++row)
        {
            memcpy(expect0 + row * W * 4u, frame0 + row * STRIDE, W * 4u);
            memcpy(expect1 + row * W * 4u, frame1 + row * STRIDE, W * 4u);
            memcpy(expect2 + row * W * 4u, frame2 + row * STRIDE, W * 4u);
        }
    }

    memset(frames, 0, sizeof(frames));
    frames[0].pixels = frame0;
    frames[1].pixels = frame1;
    frames[2].pixels = frame2;
    frames[3].pixels = frame3;
    for (i = 0; i < 4; ++i)
    {
        frames[i].width = W;
        frames[i].height = H;
        frames[i].stride_bytes = STRIDE;
        frames[i].delay_num = 1u;
        frames[i].delay_den = 12u;
        frames[i].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
        frames[i].blend_op = PNG_APNG_BLEND_OP_SOURCE;
    }

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;

    err = png_encode_apng_memory_ex_zlib(frames, 4u, W, H, 0u, &opt, &png_naive, &png_naive_size);
    if (err != PNG_DEC_OK)
        return 1901;

    err = png_encode_apng_auto_memory_ex_zlib(frames, 4u, W, H, 0u, &opt, &png_auto, &png_auto_size);
    if (err != PNG_DEC_OK)
    {
        png_free_file(png_naive);
        return 1902;
    }

    if (png_auto_size >= png_naive_size)
    {
        png_free_file(png_naive);
        png_free_file(png_auto);
        return 1903;
    }

    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_decode_apng_memory_ex_zlib(png_auto, png_auto_size, &dopt, &apng);
    png_free_file(png_naive);
    png_free_file(png_auto);
    if (err != PNG_DEC_OK)
        return 1904;

    if (apng.frame_count != 4u ||
        memcmp(apng.frames[0].pixels, expect0, sizeof(expect0)) != 0 ||
        memcmp(apng.frames[1].pixels, expect1, sizeof(expect1)) != 0 ||
        memcmp(apng.frames[2].pixels, expect2, sizeof(expect2)) != 0 ||
        memcmp(apng.frames[3].pixels, expect2, sizeof(expect2)) != 0)
    {
        png_free_apng(&apng);
        return 1905;
    }

    png_free_apng(&apng);
    return 0;
}


static int test_progressive_feed_ex_backpressure(void)
{
    png_u32 width = 5u, height = 4u;
    png_u32 pixels = width * height;
    png_u8* rgba = 0;
    png_encode_options opt;
    png_decode_options dopt;
    png_progressive_control ctl;
    png_progressive_callbacks cb;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_decoder* dec = 0;
    png_image img;
    png_u32 offset = 0u;
    int saw_partial = 0;
    int saw_yield = 0;
    int err;
    int guard = 0;

    rgba = (png_u8*)png_mem89_alloc(pixels * 4u);
    if (!rgba)
        return 3001;
    fill_rgba_pattern(rgba, width, height);

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    err = png_encode_memory_ex_zlib(rgba, width, height, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba);
        return 3002;
    }

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba);
        png_free_file(png_data);
        return 3003;
    }

    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba);
        png_decoder_free(dec);
        png_free_file(png_data);
        return 3004;
    }

    png_progressive_control_init(&ctl);
    ctl.max_feed_bytes = 11u;
    ctl.max_buffered_bytes = 128u;
    err = png_decoder_set_progressive_control(dec, &ctl);
    if (err != PNG_DEC_OK) {
        png_mem89_release(rgba);
        png_decoder_free(dec);
        png_free_file(png_data);
        return 3005;
    }

    memset(&cb, 0, sizeof(cb));
    cb.row_fn = row_cb;
    cb.end_fn = end_cb;
    png_decoder_set_callbacks(dec, &cb);
    g_rows = 0; g_end = 0; g_last_rowbytes = 0u; g_first_row_size = 0u;

    while (guard++ < 2048)
    {
        png_u32 consumed = 0u;
        png_u32 remaining = (offset < png_size) ? (png_size - offset) : 0u;
        err = png_decoder_feed_ex(dec,
                                  remaining ? (png_data + offset) : 0,
                                  remaining,
                                  &consumed);
        if (remaining != 0u && consumed > remaining) {
            png_mem89_release(rgba); png_decoder_free(dec); png_free_file(png_data); return 3006;
        }
        if (remaining != 0u && consumed < remaining)
            saw_partial = 1;
        offset += consumed;
        if (err == PNG_DEC_DONE)
            break;
        if (err == PNG_DEC_YIELDED)
            saw_yield = 1;
        else if (err != PNG_DEC_OK) {
            png_mem89_release(rgba); png_decoder_free(dec); png_free_file(png_data); return 3007;
        }
        if (remaining == 0u && consumed == 0u && err == PNG_DEC_OK) {
            png_mem89_release(rgba); png_decoder_free(dec); png_free_file(png_data); return 3008;
        }
    }

    if (guard >= 2048 || !saw_partial || !saw_yield || g_rows != (int)height || g_end != 1 || offset != png_size) {
        png_mem89_release(rgba); png_decoder_free(dec); png_free_file(png_data); return 3009;
    }

    memset(&img, 0, sizeof(img));
    err = png_decoder_take_image(dec, &img);
    png_decoder_free(dec);
    png_free_file(png_data);
    if (err != PNG_DEC_OK || img.width != width || img.height != height ||
        img.pixel_rowbytes != width * 4u || memcmp(img.pixels, rgba, pixels * 4u) != 0) {
        png_free_image(&img); png_mem89_release(rgba); return 3010;
    }

    png_free_image(&img);
    png_mem89_release(rgba);
    return 0;
}

static int test_progressive_zero_feed_yield_resume(void)
{
    png_u8 rgba[4u * 3u * 4u];
    png_encode_options opt;
    png_decode_options dopt;
    png_progressive_control ctl;
    png_progressive_callbacks cb;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_decoder* dec = 0;
    png_image img;
    png_u32 consumed = 0u;
    int err;
    int resumes = 0;

    fill_rgba_pattern(rgba, 4u, 3u);
    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    err = png_encode_memory_ex_zlib(rgba, 4u, 3u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 3011;

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK) { png_free_file(png_data); return 3012; }
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK) { png_decoder_free(dec); png_free_file(png_data); return 3013; }

    png_progressive_control_init(&ctl);
    ctl.max_row_callbacks_per_call = 1u;
    err = png_decoder_set_progressive_control(dec, &ctl);
    if (err != PNG_DEC_OK) { png_decoder_free(dec); png_free_file(png_data); return 3014; }

    memset(&cb, 0, sizeof(cb));
    cb.row_fn = row_cb;
    cb.end_fn = end_cb;
    png_decoder_set_callbacks(dec, &cb);
    g_rows = 0; g_end = 0; g_last_rowbytes = 0u;

    err = png_decoder_feed_ex(dec, png_data, png_size, &consumed);
    if (err != PNG_DEC_YIELDED || consumed != png_size || g_rows != 1 || g_end != 0) {
        png_decoder_free(dec); png_free_file(png_data); return 3015;
    }

    while (err == PNG_DEC_YIELDED && resumes++ < 16)
    {
        consumed = 123u;
        err = png_decoder_feed_ex(dec, 0, 0u, &consumed);
        if (consumed != 0u) {
            png_decoder_free(dec); png_free_file(png_data); return 3016;
        }
    }

    if (err != PNG_DEC_DONE || g_rows != 3 || g_end != 1 || resumes < 2) {
        png_decoder_free(dec); png_free_file(png_data); return 3017;
    }

    memset(&img, 0, sizeof(img));
    err = png_decoder_take_image(dec, &img);
    png_decoder_free(dec);
    png_free_file(png_data);
    if (err != PNG_DEC_OK || img.pixel_rowbytes != 16u || memcmp(img.pixels, rgba, sizeof(rgba)) != 0) {
        png_free_image(&img); return 3018;
    }
    png_free_image(&img);
    return 0;
}

static int test_apng_feed_ex_backpressure(void)
{
    png_u8 frame0[3u * 2u * 4u];
    png_u8 frame1[1u * 2u * 4u];
    png_u8 expect0[3u * 2u * 4u];
    png_u8 expect1[3u * 2u * 4u];
    png_apng_encode_frame frames[2];
    png_encode_options opt;
    png_decode_options dopt;
    png_progressive_control ctl;
    png_apng_progressive_callbacks cb;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_u32 offset = 0u;
    png_apng_decoder* dec = 0;
    png_apng apng;
    int saw_partial = 0;
    int saw_yield = 0;
    int err;
    int guard = 0;

    memset(frame0, 0, sizeof(frame0));
    memset(frame1, 0, sizeof(frame1));
    memset(expect0, 0, sizeof(expect0));
    memset(expect1, 0, sizeof(expect1));
    memset(frames, 0, sizeof(frames));
    memset(&apng, 0, sizeof(apng));

    put_pixel_rgba(frame0, 3u, 0u, 0u, 10u, 20u, 30u, 255u);
    put_pixel_rgba(frame0, 3u, 1u, 0u, 40u, 50u, 60u, 255u);
    put_pixel_rgba(frame0, 3u, 2u, 0u, 70u, 80u, 90u, 255u);
    put_pixel_rgba(frame0, 3u, 0u, 1u, 15u, 25u, 35u, 255u);
    put_pixel_rgba(frame0, 3u, 1u, 1u, 45u, 55u, 65u, 255u);
    put_pixel_rgba(frame0, 3u, 2u, 1u, 75u, 85u, 95u, 255u);
    put_pixel_rgba(frame1, 1u, 0u, 0u, 0u, 0u, 255u, 255u);
    put_pixel_rgba(frame1, 1u, 0u, 1u, 0u, 255u, 0u, 255u);
    memcpy(expect0, frame0, sizeof(expect0));
    memcpy(expect1, frame0, sizeof(expect1));
    put_pixel_rgba(expect1, 3u, 1u, 0u, 0u, 0u, 255u, 255u);
    put_pixel_rgba(expect1, 3u, 1u, 1u, 0u, 255u, 0u, 255u);

    frames[0].pixels = frame0;
    frames[0].width = 3u;
    frames[0].height = 2u;
    frames[0].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[0].blend_op = PNG_APNG_BLEND_OP_SOURCE;
    frames[1].pixels = frame1;
    frames[1].width = 1u;
    frames[1].height = 2u;
    frames[1].x_offset = 1u;
    frames[1].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[1].blend_op = PNG_APNG_BLEND_OP_SOURCE;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    err = png_encode_apng_memory_ex_zlib(frames, 2u, 3u, 2u, 0u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 3019;

    err = png_apng_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK) { png_free_file(png_data); return 3020; }
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_apng_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK) { png_apng_decoder_free(dec); png_free_file(png_data); return 3021; }

    png_progressive_control_init(&ctl);
    ctl.max_feed_bytes = 13u;
    ctl.max_row_callbacks_per_call = 1u;
    ctl.max_buffered_bytes = 256u;
    err = png_apng_decoder_set_progressive_control(dec, &ctl);
    if (err != PNG_DEC_OK) { png_apng_decoder_free(dec); png_free_file(png_data); return 3022; }

    memset(&cb, 0, sizeof(cb));
    cb.info_fn = apng_info_cb;
    cb.frame_info_fn = apng_frame_info_cb;
    cb.frame_row_fn = apng_frame_row_cb;
    cb.frame_end_fn = apng_frame_end_cb;
    cb.end_fn = apng_end_cb;
    err = png_apng_decoder_set_callbacks(dec, &cb);
    if (err != PNG_DEC_OK) { png_apng_decoder_free(dec); png_free_file(png_data); return 3023; }

    g_apng_info = 0; g_apng_frame_infos = 0; g_apng_frame_rows = 0; g_apng_frame_ends = 0; g_apng_end = 0;
    memset(g_apng_rows_per_frame, 0, sizeof(g_apng_rows_per_frame));

    while (guard++ < 4096)
    {
        png_u32 consumed = 0u;
        png_u32 remaining = (offset < png_size) ? (png_size - offset) : 0u;
        err = png_apng_decoder_feed_ex(dec,
                                       remaining ? (png_data + offset) : 0,
                                       remaining,
                                       &consumed);
        if (remaining != 0u && consumed > remaining) {
            png_apng_decoder_free(dec); png_free_file(png_data); return 3024;
        }
        if (remaining != 0u && consumed < remaining)
            saw_partial = 1;
        offset += consumed;
        if (err == PNG_DEC_DONE)
            break;
        if (err == PNG_DEC_YIELDED)
            saw_yield = 1;
        else if (err != PNG_DEC_OK) {
            png_apng_decoder_free(dec); png_free_file(png_data); return 3025;
        }
        if (remaining == 0u && consumed == 0u && err == PNG_DEC_OK) {
            png_apng_decoder_free(dec); png_free_file(png_data); return 3026;
        }
    }

    if (guard >= 4096 || !saw_partial || !saw_yield || offset != png_size) {
        png_apng_decoder_free(dec); png_free_file(png_data); return 3027;
    }

    err = png_apng_decoder_take_animation(dec, &apng);
    png_apng_decoder_free(dec);
    png_free_file(png_data);
    if (err != PNG_DEC_OK || apng.frame_count != 2u || !apng.has_default_image ||
        memcmp(apng.frames[0].pixels, expect0, sizeof(expect0)) != 0 ||
        memcmp(apng.frames[1].pixels, expect1, sizeof(expect1)) != 0 ||
        g_apng_frame_rows < 2 || g_apng_frame_ends != 2 || g_apng_end != 1) {
        png_free_apng(&apng); return 3028;
    }

    png_free_apng(&apng);
    return 0;
}

static int test_progressive_poll_and_chunk_callbacks(void)
{
    png_u8 rgba[3u * 2u * 4u];
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_encode_options opt;
    png_decoder* dec = 0;
    png_decode_options dopt;
    png_progressive_control ctl;
    png_progressive_callbacks cb;
    png_progressive_poll_state poll;
    png_image img;
    png_u32 offset = 0u;
    int err;
    int guard = 0;
    int saw_want = 0;
    int saw_drain = 0;
    int saw_yield = 0;
    int saw_idat = 0;
    png_u32 i;

    fill_rgba_pattern(rgba, 3u, 2u);
    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    err = png_encode_memory_ex_zlib(rgba, 3u, 2u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 3029;

    err = png_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK) { png_free_file(png_data); return 3030; }
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK) { png_decoder_free(dec); png_free_file(png_data); return 3031; }

    png_progressive_control_init(&ctl);
    ctl.max_feed_bytes = 7u;
    ctl.max_buffered_bytes = 128u;
    ctl.max_chunk_callbacks_per_call = 1u;
    err = png_decoder_set_progressive_control(dec, &ctl);
    if (err != PNG_DEC_OK) { png_decoder_free(dec); png_free_file(png_data); return 3032; }

    memset(&cb, 0, sizeof(cb));
    cb.row_fn = row_cb;
    cb.end_fn = end_cb;
    cb.chunk_fn = chunk_cb;
    err = png_decoder_set_callbacks(dec, &cb);
    if (err != PNG_DEC_OK) { png_decoder_free(dec); png_free_file(png_data); return 3033; }

    g_rows = 0; g_end = 0; g_last_pass = 0;
    g_chunk_events = 0;
    memset(g_chunk_types, 0, sizeof(g_chunk_types));
    memset(g_chunk_offsets, 0, sizeof(g_chunk_offsets));
    memset(g_chunk_total_sizes, 0, sizeof(g_chunk_total_sizes));
    memset(&img, 0, sizeof(img));

    while (guard++ < 4096)
    {
        png_u32 consumed = 0u;
        png_u32 take;

        err = png_decoder_poll(dec, &poll);
        if (err != PNG_DEC_OK) { png_decoder_free(dec); png_free_file(png_data); return 3034; }
        if ((poll.events & PNG_PROGRESSIVE_POLL_DONE) != 0u)
            break;

        if ((poll.events & PNG_PROGRESSIVE_POLL_CAN_DRAIN) != 0u)
        {
            saw_drain = 1;
            err = png_decoder_feed_ex(dec, 0, 0u, &consumed);
        }
        else if ((poll.events & PNG_PROGRESSIVE_POLL_WANT_INPUT) != 0u && offset < png_size)
        {
            saw_want = 1;
            take = poll.suggested_read_bytes;
            if (take == 0u || take > png_size - offset)
                take = png_size - offset;
            if (take > 9u)
                take = 9u;
            err = png_decoder_feed_ex(dec, png_data + offset, take, &consumed);
            offset += consumed;
        }
        else
        {
            png_decoder_free(dec); png_free_file(png_data); return 3035;
        }

        if (err == PNG_DEC_YIELDED)
            saw_yield = 1;
        else if (err != PNG_DEC_OK && err != PNG_DEC_DONE)
        {
            png_decoder_free(dec); png_free_file(png_data); return 3036;
        }
    }

    if (guard >= 4096) { png_decoder_free(dec); png_free_file(png_data); return 3037; }
    err = png_decoder_poll(dec, &poll);
    if (err != PNG_DEC_OK || (poll.events & PNG_PROGRESSIVE_POLL_DONE) == 0u)
    { png_decoder_free(dec); png_free_file(png_data); return 3038; }

    err = png_decoder_take_image(dec, &img);
    png_decoder_free(dec);
    png_free_file(png_data);
    if (err != PNG_DEC_OK)
        return 3039;

    if (!saw_want || !saw_drain || !saw_yield || offset != png_size ||
        g_chunk_events < 3 || g_chunk_types[0] != ((png_u32)'I'<<24 | (png_u32)'H'<<16 | (png_u32)'D'<<8 | (png_u32)'R') ||
        g_chunk_types[g_chunk_events - 1] != ((png_u32)'I'<<24 | (png_u32)'E'<<16 | (png_u32)'N'<<8 | (png_u32)'D') ||
        g_chunk_offsets[0] != 8u || g_chunk_total_sizes[0] != 25u ||
        g_rows != 2 || g_end != 1 || img.width != 3u || img.height != 2u)
    {
        png_free_image(&img);
        return 3040;
    }

    for (i = 0u; i < (png_u32)g_chunk_events; ++i)
        if (g_chunk_types[i] == ((png_u32)'I'<<24 | (png_u32)'D'<<16 | (png_u32)'A'<<8 | (png_u32)'T'))
            saw_idat = 1;
    png_free_image(&img);
    return saw_idat ? 0 : 3041;
}

static int test_apng_poll_and_chunk_callbacks(void)
{
    png_u8 frame0[3u * 2u * 4u];
    png_u8 frame1[1u * 2u * 4u];
    png_apng_encode_frame frames[2];
    png_encode_options opt;
    png_u8* png_data = 0;
    png_u32 png_size = 0u;
    png_apng_decoder* dec = 0;
    png_decode_options dopt;
    png_progressive_control ctl;
    png_apng_progressive_callbacks cb;
    png_progressive_poll_state poll;
    png_apng apng;
    png_u32 offset = 0u;
    int err;
    int guard = 0;
    int saw_want = 0;
    int saw_drain = 0;
    int saw_yield = 0;
    int saw_actl = 0;
    int saw_frame_chunk = 0;
    png_u32 i;

    memset(frame0, 0, sizeof(frame0));
    memset(frame1, 0, sizeof(frame1));
    memset(frames, 0, sizeof(frames));
    memset(&apng, 0, sizeof(apng));

    put_pixel_rgba(frame0, 3u, 0u, 0u, 10u, 20u, 30u, 255u);
    put_pixel_rgba(frame0, 3u, 1u, 0u, 40u, 50u, 60u, 255u);
    put_pixel_rgba(frame0, 3u, 2u, 0u, 70u, 80u, 90u, 255u);
    put_pixel_rgba(frame0, 3u, 0u, 1u, 15u, 25u, 35u, 255u);
    put_pixel_rgba(frame0, 3u, 1u, 1u, 45u, 55u, 65u, 255u);
    put_pixel_rgba(frame0, 3u, 2u, 1u, 75u, 85u, 95u, 255u);
    put_pixel_rgba(frame1, 1u, 0u, 0u, 0u, 0u, 255u, 255u);
    put_pixel_rgba(frame1, 1u, 0u, 1u, 0u, 255u, 0u, 255u);

    frames[0].pixels = frame0;
    frames[0].width = 3u;
    frames[0].height = 2u;
    frames[0].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[0].blend_op = PNG_APNG_BLEND_OP_SOURCE;
    frames[1].pixels = frame1;
    frames[1].width = 1u;
    frames[1].height = 2u;
    frames[1].x_offset = 1u;
    frames[1].dispose_op = PNG_APNG_DISPOSE_OP_NONE;
    frames[1].blend_op = PNG_APNG_BLEND_OP_SOURCE;

    png_encode_options_init(&opt);
    opt.color_type = PNG_COLOR_TRUECOLOR_ALPHA;
    opt.bit_depth = 8u;
    err = png_encode_apng_memory_ex_zlib(frames, 2u, 3u, 2u, 0u, &opt, &png_data, &png_size);
    if (err != PNG_DEC_OK)
        return 3042;

    err = png_apng_decoder_init(&dec, png_zlib_decompress);
    if (err != PNG_DEC_OK) { png_free_file(png_data); return 3043; }
    png_decode_options_init(&dopt);
    dopt.output_format = PNG_OUTPUT_RGBA8;
    err = png_apng_decoder_set_options(dec, &dopt);
    if (err != PNG_DEC_OK) { png_apng_decoder_free(dec); png_free_file(png_data); return 3044; }

    png_progressive_control_init(&ctl);
    ctl.max_feed_bytes = 11u;
    ctl.max_buffered_bytes = 256u;
    ctl.max_chunk_callbacks_per_call = 1u;
    err = png_apng_decoder_set_progressive_control(dec, &ctl);
    if (err != PNG_DEC_OK) { png_apng_decoder_free(dec); png_free_file(png_data); return 3045; }

    memset(&cb, 0, sizeof(cb));
    cb.chunk_fn = apng_chunk_cb;
    cb.end_fn = apng_end_cb;
    err = png_apng_decoder_set_callbacks(dec, &cb);
    if (err != PNG_DEC_OK) { png_apng_decoder_free(dec); png_free_file(png_data); return 3046; }

    g_apng_end = 0;
    g_apng_chunk_events = 0;
    memset(g_apng_chunk_types, 0, sizeof(g_apng_chunk_types));
    memset(g_apng_chunk_frame_indices, 0xFF, sizeof(g_apng_chunk_frame_indices));

    while (guard++ < 4096)
    {
        png_u32 consumed = 0u;
        png_u32 take;

        err = png_apng_decoder_poll(dec, &poll);
        if (err != PNG_DEC_OK) { png_apng_decoder_free(dec); png_free_file(png_data); return 3047; }
        if ((poll.events & PNG_PROGRESSIVE_POLL_DONE) != 0u)
            break;

        if ((poll.events & PNG_PROGRESSIVE_POLL_CAN_DRAIN) != 0u)
        {
            saw_drain = 1;
            err = png_apng_decoder_feed_ex(dec, 0, 0u, &consumed);
        }
        else if ((poll.events & PNG_PROGRESSIVE_POLL_WANT_INPUT) != 0u && offset < png_size)
        {
            saw_want = 1;
            take = poll.suggested_read_bytes;
            if (take == 0u || take > png_size - offset)
                take = png_size - offset;
            if (take > 13u)
                take = 13u;
            err = png_apng_decoder_feed_ex(dec, png_data + offset, take, &consumed);
            offset += consumed;
        }
        else
        {
            png_apng_decoder_free(dec); png_free_file(png_data); return 3048;
        }

        if (err == PNG_DEC_YIELDED)
            saw_yield = 1;
        else if (err != PNG_DEC_OK && err != PNG_DEC_DONE)
        {
            png_apng_decoder_free(dec); png_free_file(png_data); return 3049;
        }
    }

    if (guard >= 4096) { png_apng_decoder_free(dec); png_free_file(png_data); return 3050; }
    err = png_apng_decoder_poll(dec, &poll);
    if (err != PNG_DEC_OK || (poll.events & PNG_PROGRESSIVE_POLL_DONE) == 0u)
    { png_apng_decoder_free(dec); png_free_file(png_data); return 3051; }

    err = png_apng_decoder_take_animation(dec, &apng);
    png_apng_decoder_free(dec);
    png_free_file(png_data);
    if (err != PNG_DEC_OK || apng.frame_count != 2u || g_apng_end != 1)
    { png_free_apng(&apng); return 3052; }

    if (!saw_want || !saw_drain || !saw_yield || offset != png_size ||
        g_apng_chunk_events < 6 || g_apng_chunk_types[0] != ((png_u32)'I'<<24 | (png_u32)'H'<<16 | (png_u32)'D'<<8 | (png_u32)'R') ||
        g_apng_chunk_types[g_apng_chunk_events - 1] != ((png_u32)'I'<<24 | (png_u32)'E'<<16 | (png_u32)'N'<<8 | (png_u32)'D'))
    {
        png_free_apng(&apng);
        return 3053;
    }

    for (i = 0u; i < (png_u32)g_apng_chunk_events; ++i)
    {
        if (g_apng_chunk_types[i] == ((png_u32)'a'<<24 | (png_u32)'c'<<16 | (png_u32)'T'<<8 | (png_u32)'L'))
            saw_actl = 1;
        if (g_apng_chunk_frame_indices[i] >= 0)
            saw_frame_chunk = 1;
    }

    png_free_apng(&apng);
    return (saw_actl && saw_frame_chunk) ? 0 : 3054;
}

int main(void)
{
    int r;

    r = test_adam7_iccp();
    if (r != 0) {
        printf("FAIL adam7_iccp %d\n", r);
        return r;
    }

    r = test_runtime_limits();
    if (r != 0) {
        printf("FAIL runtime_limits %d\n", r);
        return r;
    }

    r = test_error_too_many_pixels();
    if (r != 0) {
        printf("FAIL error_too_many_pixels %d\n", r);
        return r;
    }

    r = test_error_inflated_too_large();
    if (r != 0) {
        printf("FAIL error_inflated_too_large %d\n", r);
        return r;
    }

    r = test_error_chunk_too_large();
    if (r != 0) {
        printf("FAIL error_chunk_too_large %d\n", r);
        return r;
    }

    r = test_error_too_many_chunks();
    if (r != 0) {
        printf("FAIL error_too_many_chunks %d\n", r);
        return r;
    }

    r = test_error_text_too_large();
    if (r != 0) {
        printf("FAIL error_text_too_large %d\n", r);
        return r;
    }

    r = test_error_frame_limit();
    if (r != 0) {
        printf("FAIL error_frame_limit %d\n", r);
        return r;
    }

    r = test_error_temp_memory_limit();
    if (r != 0) {
        printf("FAIL error_temp_memory_limit %d\n", r);
        return r;
    }

    r = test_error_work_budget();
    if (r != 0) {
        printf("FAIL error_work_budget %d\n", r);
        return r;
    }

    r = test_error_conversion_limit();
    if (r != 0) {
        printf("FAIL error_conversion_limit %d\n", r);
        return r;
    }

    r = test_incremental_rows();
    if (r != 0) {
        printf("FAIL incremental_rows %d\n", r);
        return r;
    }

    r = test_iccp_srgb_conflict();
    if (r != 0) {
        printf("FAIL iccp_srgb_conflict %d\n", r);
        return r;
    }

    r = test_output_formats_and_convert();
    if (r != 0) {
        printf("FAIL output_formats_and_convert %d\n", r);
        return r;
    }

    r = test_streaming_output_format();
    if (r != 0) {
        printf("FAIL streaming_output_format %d\n", r);
        return r;
    }

    r = test_write_chrm_bkgd_sbit();
    if (r != 0) {
        printf("FAIL write_chrm_bkgd_sbit %d\n", r);
        return r;
    }

    r = test_extended_ancillary_platform_chunks();
    if (r != 0) {
        printf("FAIL extended_ancillary_platform_chunks %d\n", r);
        return r;
    }

    r = test_output_formats_16bit_and_convert();
    if (r != 0) {
        printf("FAIL output_formats_16bit_and_convert %d\n", r);
        return r;
    }

    r = test_streaming_output_format_16bit();
    if (r != 0) {
        printf("FAIL streaming_output_format_16bit %d\n", r);
        return r;
    }

    r = test_incremental_adam7_rows();
    if (r != 0) {
        printf("FAIL incremental_adam7_rows %d\n", r);
        return r;
    }

    r = test_pcal_roundtrip();
    if (r != 0) {
        printf("FAIL pcal_roundtrip %d\n", r);
        return r;
    }

    r = test_hdr_exif_dsig_roundtrip();
    if (r != 0) {
        printf("FAIL hdr_exif_dsig_roundtrip %d\n", r);
        return r;
    }

    r = test_modern_chunk_validation();
    if (r != 0) {
        printf("FAIL modern_chunk_validation %d\n", r);
        return r;
    }

    r = test_endian_16bit_paths();
    if (r != 0) {
        printf("FAIL endian_16bit_paths %d\n", r);
        return r;
    }

    r = test_legacy_extension_chunks();
    if (r != 0) {
        printf("FAIL legacy_extension_chunks %d\n", r);
        return r;
    }

    r = test_public_input_encode_pipeline();
    if (r != 0) {
        printf("FAIL public_input_encode_pipeline %d\n", r);
        return r;
    }

    r = test_encode_image_ex_wrapper();
    if (r != 0) {
        printf("FAIL encode_image_ex_wrapper %d\n", r);
        return r;
    }

    r = test_progressive_pause_resume_and_skip();
    if (r != 0) {
        printf("FAIL progressive_pause_resume_and_skip %d\n", r);
        return r;
    }

    r = test_apng_progressive_pause_resume();
    if (r != 0) {
        printf("FAIL apng_progressive_pause_resume %d\n", r);
        return r;
    }

    r = test_apng_platform_roundtrip();
    if (r != 0) {
        printf("FAIL apng_platform_roundtrip %d\n", r);
        return r;
    }

    r = test_apng_progressive_callbacks();
    if (r != 0) {
        printf("FAIL apng_progressive_callbacks %d\n", r);
        return r;
    }

    r = test_apng_progressive_adam7_pass_callbacks();
    if (r != 0) {
        printf("FAIL apng_progressive_adam7_pass_callbacks %d\n", r);
        return r;
    }

    r = test_apng_auto_optimizer();
    if (r != 0) {
        printf("FAIL apng_auto_optimizer %d\n", r);
        return r;
    }

    r = test_progressive_feed_ex_backpressure();
    if (r != 0) {
        printf("FAIL progressive_feed_ex_backpressure %d\n", r);
        return r;
    }

    r = test_progressive_zero_feed_yield_resume();
    if (r != 0) {
        printf("FAIL progressive_zero_feed_yield_resume %d\n", r);
        return r;
    }

    r = test_apng_feed_ex_backpressure();
    if (r != 0) {
        printf("FAIL apng_feed_ex_backpressure %d\n", r);
        return r;
    }

    r = test_progressive_poll_and_chunk_callbacks();
    if (r != 0) {
        printf("FAIL progressive_poll_and_chunk_callbacks %d\n", r);
        return r;
    }

    r = test_apng_poll_and_chunk_callbacks();
    if (r != 0) {
        printf("FAIL apng_poll_and_chunk_callbacks %d\n", r);
        return r;
    }

    printf("ALL_TESTS_OK\n");
    return 0;
}
