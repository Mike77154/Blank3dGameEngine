#include <stdio.h>
#include <string.h>
#include "c89jpeg.h"

#define W 64
#define H 48
#define JPEG_CAP 65536UL

static c89jpeg_u8 g_rgb[W * H * 3UL];
static c89jpeg_u8 g_row_cache[W * 3UL * 16UL];
static c89jpeg_u8 g_row_window[W * 3UL * 16UL];
static c89jpeg_u8 g_replay[W * H * 3UL];
static c89jpeg_u8 g_stage[2048UL];
static c89jpeg_u8 g_ref[JPEG_CAP];
static c89jpeg_u8 g_out[JPEG_CAP];
static c89jpeg_u8 g_decode[W * H * 3UL];

struct mem_rows {
    const c89jpeg_u8 *pixels;
    c89jpeg_u32 stride;
};

static void make_pattern(void)
{
    c89jpeg_u16 x;
    c89jpeg_u16 y;
    for (y = 0; y < H; ++y) {
        for (x = 0; x < W; ++x) {
            c89jpeg_u32 i;
            i = ((c89jpeg_u32)y * W + x) * 3UL;
            g_rgb[i + 0] = (c89jpeg_u8)(((x * 13U) + (y * 5U)) & 255U);
            g_rgb[i + 1] = (c89jpeg_u8)(((x * 7U) ^ (y * 19U)) & 255U);
            g_rgb[i + 2] = (c89jpeg_u8)(((x * x) + (y * 11U) + 37U) & 255U);
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

static int test_source_resume_optimal_replay(void)
{
    c89jpeg_encoder enc;
    c89jpeg_encoder_resume resume;
    c89jpeg_decoder dec;
    c89jpeg_encode_source_params sp;
    c89jpeg_encode_source_resume_params srp;
    c89jpeg_decode_params dp;
    c89jpeg_image_info info;
    struct mem_rows rows;
    c89jpeg_status st;
    c89jpeg_u32 ref_size;
    c89jpeg_u32 out_size;
    c89jpeg_u32 chunk;
    c89jpeg_u32 need;
    c89jpeg_u32 accepted;
    c89jpeg_u32 fed_rows;
    c89jpeg_u32 prefix_bytes;

    rows.pixels = g_rgb;
    rows.stride = W * 3UL;

    memset(&sp, 0, sizeof(sp));
    sp.width = W;
    sp.height = H;
    sp.stride_bytes = W * 3UL;
    sp.pixel_format = C89JPEG_PIXFMT_RGB24;
    sp.subsampling = C89JPEG_SUBSAMP_420;
    sp.quality = 83;
    sp.restart_interval = 4;
    sp.emit_jfif = 1;
    sp.huffman_mode = C89JPEG_HUFFMAN_OPTIMAL;
    sp.read_row_fn = read_row_mem;
    sp.read_user = &rows;
    sp.row_cache = g_row_cache;
    sp.row_cache_size = sizeof(g_row_cache);

    c89jpeg_encoder_init(&enc);
    st = c89jpeg_encode_source_memory(&enc, &sp, g_ref, sizeof(g_ref), &ref_size);
    if (st != C89JPEG_OK) {
        printf("source_optimal_ref status=%s\n", c89jpeg_status_string(st));
        return 1;
    }

    memset(&srp, 0, sizeof(srp));
    srp.width = W;
    srp.height = H;
    srp.stride_bytes = W * 3UL;
    srp.pixel_format = C89JPEG_PIXFMT_RGB24;
    srp.subsampling = C89JPEG_SUBSAMP_420;
    srp.quality = 83;
    srp.restart_interval = 4;
    srp.emit_jfif = 1;
    srp.huffman_mode = C89JPEG_HUFFMAN_OPTIMAL;
    srp.row_window = g_row_window;
    srp.row_window_size = sizeof(g_row_window);
    srp.replay_buffer = g_replay;
    srp.replay_buffer_size = sizeof(g_replay);
    srp.staging_buffer = g_stage;
    srp.staging_capacity = sizeof(g_stage);
    srp.emit_chunk_size = 137;

    c89jpeg_encoder_init(&enc);
    st = c89jpeg_encoder_resume_begin_source(&resume, &enc, &srp);
    if (st != C89JPEG_OK) {
        printf("resume_source_optimal begin=%s\n", c89jpeg_status_string(st));
        return 1;
    }

    out_size = 0;
    prefix_bytes = 0;
    for (;;) {
        chunk = 0;
        st = c89jpeg_encoder_resume_pull(&resume, g_out + out_size, sizeof(g_out) - out_size, &chunk);
        if (st != C89JPEG_OK && st != C89JPEG_SUSPENDED) {
            printf("resume_source_optimal prefix_pull=%s\n", c89jpeg_status_string(st));
            return 1;
        }
        out_size += chunk;
        prefix_bytes += chunk;
        if (st == C89JPEG_OK || chunk == 0) break;
    }

    fed_rows = 0;
    while (!c89jpeg_encoder_resume_is_finished(&resume)) {
        if (c89jpeg_encoder_resume_needs_input(&resume)) {
            need = c89jpeg_encoder_resume_source_rows_needed(&resume);
            if (need > 5) need = 5;
            st = c89jpeg_encoder_resume_feed_rows(&resume,
                                                  g_rgb + fed_rows * (W * 3UL),
                                                  need,
                                                  W * 3UL,
                                                  &accepted);
            if (st != C89JPEG_OK) {
                printf("resume_source_optimal feed=%s\n", c89jpeg_status_string(st));
                return 1;
            }
            fed_rows += accepted;
            if (fed_rows >= H) c89jpeg_encoder_resume_finish_input(&resume);
        }
        for (;;) {
            chunk = 0;
            st = c89jpeg_encoder_resume_pull(&resume, g_out + out_size, sizeof(g_out) - out_size, &chunk);
            if (st != C89JPEG_OK && st != C89JPEG_SUSPENDED) {
                printf("resume_source_optimal pull=%s\n", c89jpeg_status_string(st));
                return 1;
            }
            out_size += chunk;
            if (st == C89JPEG_OK || chunk == 0) break;
        }
    }

    if (prefix_bytes == 0) {
        printf("resume_source_optimal prefix=0\n");
        return 1;
    }
    if (fed_rows != H) {
        printf("resume_source_optimal rows=%lu expected=%u\n",
               (unsigned long)fed_rows,
               (unsigned)H);
        return 1;
    }
    if (ref_size != out_size || memcmp(g_ref, g_out, (size_t)ref_size) != 0) {
        printf("resume_source_optimal mismatch ref=%lu out=%lu\n",
               (unsigned long)ref_size,
               (unsigned long)out_size);
        return 1;
    }

    c89jpeg_decoder_init(&dec);
    memset(&dp, 0, sizeof(dp));
    dp.data = g_out;
    dp.size = out_size;
    dp.out_pixels = g_decode;
    dp.out_capacity = sizeof(g_decode);
    dp.out_format = C89JPEG_DECODE_RGB24;
    dp.upsampling = C89JPEG_UPSAMPLE_LINEAR;
    st = c89jpeg_decode(&dec, &dp, &info);
    if (st != C89JPEG_OK) {
        printf("resume_source_optimal decode=%s\n", c89jpeg_status_string(st));
        return 1;
    }
    if (info.width != W || info.height != H) {
        printf("resume_source_optimal decoded=%ux%u\n",
               (unsigned)info.width,
               (unsigned)info.height);
        return 1;
    }

    printf("resume_source_optimal ref=%lu out=%lu prefix=%lu replay=%lu\n",
           (unsigned long)ref_size,
           (unsigned long)out_size,
           (unsigned long)prefix_bytes,
           (unsigned long)c89jpeg_encoder_resume_replay_buffer_size(W, H, W * 3UL));
    return 0;
}

static int test_source_resume_optimal_requires_replay(void)
{
    c89jpeg_encoder enc;
    c89jpeg_encoder_resume resume;
    c89jpeg_encode_source_resume_params srp;
    c89jpeg_status st;

    memset(&srp, 0, sizeof(srp));
    srp.width = W;
    srp.height = H;
    srp.stride_bytes = W * 3UL;
    srp.pixel_format = C89JPEG_PIXFMT_RGB24;
    srp.subsampling = C89JPEG_SUBSAMP_420;
    srp.quality = 83;
    srp.emit_jfif = 1;
    srp.huffman_mode = C89JPEG_HUFFMAN_OPTIMAL;
    srp.row_window = g_row_window;
    srp.row_window_size = sizeof(g_row_window);
    srp.staging_buffer = g_stage;
    srp.staging_capacity = sizeof(g_stage);
    srp.emit_chunk_size = 137;

    c89jpeg_encoder_init(&enc);
    st = c89jpeg_encoder_resume_begin_source(&resume, &enc, &srp);
    if (st != C89JPEG_ERR_SHORT_BUFFER) {
        printf("resume_source_optimal_requires_replay status=%s\n", c89jpeg_status_string(st));
        return 1;
    }
    printf("resume_source_optimal_requires_replay status=%s\n", c89jpeg_status_string(st));
    return 0;
}

static int test_source_resume_custom_replay(void)
{
    c89jpeg_encoder enc;
    c89jpeg_encoder_resume resume;
    c89jpeg_encode_source_resume_params srp;
    c89jpeg_huffman_table dc_luma;
    c89jpeg_huffman_table ac_luma;
    c89jpeg_huffman_table dc_chroma;
    c89jpeg_huffman_table ac_chroma;
    c89jpeg_status st;
    c89jpeg_u32 out_size;
    c89jpeg_u32 chunk;
    c89jpeg_u32 need;
    c89jpeg_u32 accepted;
    c89jpeg_u32 fed_rows;

    if (c89jpeg_huffman_table_init_std(&dc_luma, 0, 0) != C89JPEG_OK) return 1;
    if (c89jpeg_huffman_table_init_std(&ac_luma, 1, 0) != C89JPEG_OK) return 1;
    if (c89jpeg_huffman_table_init_std(&dc_chroma, 0, 1) != C89JPEG_OK) return 1;
    if (c89jpeg_huffman_table_init_std(&ac_chroma, 1, 1) != C89JPEG_OK) return 1;

    memset(&srp, 0, sizeof(srp));
    srp.width = W;
    srp.height = H;
    srp.stride_bytes = W * 3UL;
    srp.pixel_format = C89JPEG_PIXFMT_RGB24;
    srp.subsampling = C89JPEG_SUBSAMP_420;
    srp.quality = 75;
    srp.emit_jfif = 1;
    srp.huffman_mode = C89JPEG_HUFFMAN_CUSTOM;
    srp.custom_dc_luma = &dc_luma;
    srp.custom_ac_luma = &ac_luma;
    srp.custom_dc_chroma = &dc_chroma;
    srp.custom_ac_chroma = &ac_chroma;
    srp.row_window = g_row_window;
    srp.row_window_size = sizeof(g_row_window);
    srp.replay_buffer = g_replay;
    srp.replay_buffer_size = sizeof(g_replay);
    srp.staging_buffer = g_stage;
    srp.staging_capacity = sizeof(g_stage);
    srp.emit_chunk_size = 127;

    c89jpeg_encoder_init(&enc);
    st = c89jpeg_encoder_resume_begin_source(&resume, &enc, &srp);
    if (st != C89JPEG_OK) {
        printf("resume_source_custom begin=%s\n", c89jpeg_status_string(st));
        return 1;
    }

    out_size = 0;
    fed_rows = 0;
    while (!c89jpeg_encoder_resume_is_finished(&resume)) {
        if (c89jpeg_encoder_resume_needs_input(&resume)) {
            need = c89jpeg_encoder_resume_source_rows_needed(&resume);
            if (need > 7) need = 7;
            st = c89jpeg_encoder_resume_feed_rows(&resume,
                                                  g_rgb + fed_rows * (W * 3UL),
                                                  need,
                                                  W * 3UL,
                                                  &accepted);
            if (st != C89JPEG_OK) {
                printf("resume_source_custom feed=%s\n", c89jpeg_status_string(st));
                return 1;
            }
            fed_rows += accepted;
            if (fed_rows >= H) c89jpeg_encoder_resume_finish_input(&resume);
        }
        for (;;) {
            chunk = 0;
            st = c89jpeg_encoder_resume_pull(&resume, g_out + out_size, sizeof(g_out) - out_size, &chunk);
            if (st != C89JPEG_OK && st != C89JPEG_SUSPENDED) {
                printf("resume_source_custom pull=%s\n", c89jpeg_status_string(st));
                return 1;
            }
            out_size += chunk;
            if (st == C89JPEG_OK || chunk == 0) break;
        }
    }

    if (out_size == 0 || fed_rows != H) {
        printf("resume_source_custom out=%lu rows=%lu\n",
               (unsigned long)out_size,
               (unsigned long)fed_rows);
        return 1;
    }
    printf("resume_source_custom out=%lu rows=%lu\n",
           (unsigned long)out_size,
           (unsigned long)fed_rows);
    return 0;
}

int main(void)
{
    int failed;
    make_pattern();
    failed = 0;
    failed |= test_source_resume_optimal_replay();
    failed |= test_source_resume_optimal_requires_replay();
    failed |= test_source_resume_custom_replay();
    if (failed) return 1;
    printf("all tests passed\n");
    return 0;
}
