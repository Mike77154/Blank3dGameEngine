#include <stdio.h>
#include <string.h>
#include "c89jpeg.h"

#define W 320
#define H 240
#define JPEG_CAP 524288UL
#define TABLES_CAP 16384UL

static c89jpeg_u8 g_rgb[3][W * H * 3UL];
static c89jpeg_u8 g_row_cache[W * 3UL * 16UL];
static c89jpeg_u8 g_window[W * 3UL * 16UL];
static c89jpeg_u8 g_stage[4096UL];
static c89jpeg_u8 g_tables[TABLES_CAP];
static c89jpeg_u8 g_frame[JPEG_CAP];
static c89jpeg_u8 g_decode[W * H * 3UL];

struct mem_rows {
    const c89jpeg_u8 *pixels;
    c89jpeg_u32 stride;
};

static void make_pattern(c89jpeg_u8 *dst, int variant)
{
    c89jpeg_u16 x;
    c89jpeg_u16 y;
    for (y = 0; y < H; ++y) {
        for (x = 0; x < W; ++x) {
            c89jpeg_u32 i;
            i = ((c89jpeg_u32)y * W + x) * 3UL;
            if (variant == 0) {
                dst[i + 0] = (c89jpeg_u8)(((x * 5U) + (y * 9U)) & 255U);
                dst[i + 1] = (c89jpeg_u8)(((x * 7U) ^ (y * 11U)) & 255U);
                dst[i + 2] = (c89jpeg_u8)(((x * 13U) + y) & 255U);
            } else if (variant == 1) {
                dst[i + 0] = (c89jpeg_u8)(((x * y) + 17U * x) & 255U);
                dst[i + 1] = (c89jpeg_u8)(((x * 3U) + (y * 19U)) & 255U);
                dst[i + 2] = (c89jpeg_u8)(((x * x) + (y * 5U) + 31U) & 255U);
            } else {
                dst[i + 0] = (c89jpeg_u8)(((x * 23U) + (y * 7U)) & 255U);
                dst[i + 1] = (c89jpeg_u8)(((x ^ y) * 15U) & 255U);
                dst[i + 2] = (c89jpeg_u8)(((x * 3U) + (y * y)) & 255U);
            }
        }
    }
}

static int read_row_mem(void *user, c89jpeg_u32 row_index, c89jpeg_u8 *row, c89jpeg_u32 row_size)
{
    struct mem_rows *ctx;
    ctx = (struct mem_rows *)user;
    memcpy(row, ctx->pixels + row_index * ctx->stride, (size_t)row_size);
    return 1;
}

static void fill_source_params(c89jpeg_encode_source_params *sp,
                               struct mem_rows *rows,
                               const c89jpeg_u8 *pixels)
{
    memset(sp, 0, sizeof(*sp));
    rows->pixels = pixels;
    rows->stride = W * 3UL;
    sp->width = W;
    sp->height = H;
    sp->stride_bytes = W * 3UL;
    sp->pixel_format = C89JPEG_PIXFMT_RGB24;
    sp->subsampling = C89JPEG_SUBSAMP_420;
    sp->quality = 82;
    sp->restart_interval = 4;
    sp->emit_jfif = 1;
    sp->huffman_mode = C89JPEG_HUFFMAN_DEFAULT;
    sp->read_row_fn = read_row_mem;
    sp->read_user = rows;
    sp->row_cache = g_row_cache;
    sp->row_cache_size = sizeof(g_row_cache);
}

static int write_file(const char *path, const c89jpeg_u8 *data, c89jpeg_u32 size)
{
    FILE *f;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    fwrite(data, 1, (size_t)size, f);
    fclose(f);
    return 1;
}

static int write_ppm(const char *path, const c89jpeg_u8 *rgb, c89jpeg_u16 w, c89jpeg_u16 h)
{
    FILE *f;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    fprintf(f, "P6\n%u %u\n255\n", (unsigned)w, (unsigned)h);
    fwrite(rgb, 1, (size_t)(w * h * 3UL), f);
    fclose(f);
    return 1;
}

int main(void)
{
    c89jpeg_encoder enc;
    c89jpeg_encoder_resume resume;
    c89jpeg_decoder dec;
    c89jpeg_tables_trainer trainer;
    c89jpeg_tables_session session;
    c89jpeg_tables trained_tables;
    c89jpeg_encode_source_params sp;
    c89jpeg_encode_source_resume_params srp;
    struct mem_rows rows;
    c89jpeg_decode_params dp;
    c89jpeg_image_info info;
    c89jpeg_status st;
    c89jpeg_u32 tables_size;
    c89jpeg_u32 frame_size;
    c89jpeg_u32 fed_rows;
    int i;

    for (i = 0; i < 3; ++i) make_pattern(g_rgb[i], i);

    fill_source_params(&sp, &rows, g_rgb[0]);
    c89jpeg_tables_trainer_init(&trainer);
    c89jpeg_encoder_init(&enc);
    st = c89jpeg_tables_trainer_prepare_source(&trainer, &enc, &sp);
    if (st != C89JPEG_OK) {
        printf("tables_trainer_prepare_source failed: %s\n", c89jpeg_status_string(st));
        return 1;
    }
    for (i = 0; i < 3; ++i) {
        fill_source_params(&sp, &rows, g_rgb[i]);
        c89jpeg_encoder_init(&enc);
        st = c89jpeg_tables_trainer_add_image_source(&trainer, &enc, &sp);
        if (st != C89JPEG_OK) {
            printf("tables_trainer_add_image_source[%d] failed: %s\n", i, c89jpeg_status_string(st));
            return 1;
        }
    }
    c89jpeg_tables_session_init(&session);
    st = c89jpeg_tables_session_prepare_trained(&session, &trainer, &trained_tables);
    if (st != C89JPEG_OK) {
        printf("tables_session_prepare_trained failed: %s\n", c89jpeg_status_string(st));
        return 1;
    }
    st = c89jpeg_tables_session_write_tables_memory(&session, g_tables, sizeof(g_tables), &tables_size);
    if (st != C89JPEG_OK) {
        printf("tables_session_write_tables_memory failed: %s\n", c89jpeg_status_string(st));
        return 1;
    }
    if (!write_file("build/example_source_resume_tables.jpg", g_tables, tables_size)) {
        printf("failed to write build/example_source_resume_tables.jpg\n");
        return 1;
    }

    memset(&srp, 0, sizeof(srp));
    srp.width = W;
    srp.height = H;
    srp.stride_bytes = W * 3UL;
    srp.pixel_format = C89JPEG_PIXFMT_RGB24;
    srp.subsampling = C89JPEG_SUBSAMP_420;
    srp.quality = 82;
    srp.restart_interval = 4;
    srp.emit_jfif = 1;
    srp.huffman_mode = C89JPEG_HUFFMAN_DEFAULT;
    srp.row_window = g_window;
    srp.row_window_size = sizeof(g_window);
    srp.staging_buffer = g_stage;
    srp.staging_capacity = sizeof(g_stage);
    srp.emit_chunk_size = 199;

    c89jpeg_encoder_init(&enc);
    st = c89jpeg_tables_session_begin_image_source(&session, &resume, &enc, &srp);
    if (st != C89JPEG_OK) {
        printf("tables_session_begin_image_source failed: %s\n", c89jpeg_status_string(st));
        return 1;
    }

    frame_size = 0;
    fed_rows = 0;
    for (;;) {
        int progressed;
        progressed = 0;
        for (;;) {
            c89jpeg_u32 chunk;
            chunk = 0;
            st = c89jpeg_encoder_resume_pull(&resume, g_frame + frame_size, sizeof(g_frame) - frame_size, &chunk);
            if (st != C89JPEG_OK && st != C89JPEG_SUSPENDED) {
                printf("session encoder_resume_pull failed: %s\n", c89jpeg_status_string(st));
                return 1;
            }
            frame_size += chunk;
            if (chunk != 0) progressed = 1;
            if (st == C89JPEG_OK) break;
            if (chunk == 0) break;
        }
        if (c89jpeg_encoder_resume_is_finished(&resume)) break;
        if (c89jpeg_encoder_resume_needs_input(&resume)) {
            c89jpeg_u32 need;
            c89jpeg_u32 accepted;
            need = c89jpeg_encoder_resume_source_rows_needed(&resume);
            if (need > 7) need = 7;
            st = c89jpeg_encoder_resume_feed_rows(&resume,
                                                  g_rgb[1] + fed_rows * (W * 3UL),
                                                  need,
                                                  W * 3UL,
                                                  &accepted);
            if (st != C89JPEG_OK) {
                printf("session encoder_resume_feed_rows failed: %s\n", c89jpeg_status_string(st));
                return 1;
            }
            fed_rows += accepted;
            if (fed_rows >= H) c89jpeg_encoder_resume_finish_input(&resume);
            progressed = 1;
        }
        if (!progressed) {
            printf("source-resume session driver stalled\n");
            return 1;
        }
    }

    if (!write_file("build/example_source_resume_frame1.abbr.jpg", g_frame, frame_size)) {
        printf("failed to write build/example_source_resume_frame1.abbr.jpg\n");
        return 1;
    }

    c89jpeg_decoder_init(&dec);
    memset(&dp, 0, sizeof(dp));
    dp.data = g_frame;
    dp.size = frame_size;
    dp.out_pixels = g_decode;
    dp.out_capacity = sizeof(g_decode);
    dp.out_format = C89JPEG_DECODE_RGB24;
    dp.upsampling = C89JPEG_UPSAMPLE_LINEAR;
    st = c89jpeg_decode_abbreviated(&dec, &trained_tables, &dp, &info);
    if (st != C89JPEG_OK) {
        printf("decode_abbreviated failed: %s\n", c89jpeg_status_string(st));
        return 1;
    }
    if (!write_ppm("build/example_source_resume_frame1.ppm", g_decode, info.width, info.height)) {
        printf("failed to write build/example_source_resume_frame1.ppm\n");
        return 1;
    }

    printf("source_resume_session tables=%lu frame=%lu rows=%lu\n",
           (unsigned long)tables_size,
           (unsigned long)frame_size,
           (unsigned long)c89jpeg_encoder_resume_source_rows_accepted(&resume));
    printf("wrote build/example_source_resume_tables.jpg\n");
    printf("wrote build/example_source_resume_frame1.abbr.jpg\n");
    printf("wrote build/example_source_resume_frame1.ppm\n");
    return 0;
}
