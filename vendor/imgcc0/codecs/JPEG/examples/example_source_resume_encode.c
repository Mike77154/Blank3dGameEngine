#include <stdio.h>
#include <string.h>
#include "c89jpeg.h"

#define W 320
#define H 240
#define JPEG_CAP 524288UL

static c89jpeg_u8 g_rgb[W * H * 3UL];
static c89jpeg_u8 g_window[W * 3UL * 16UL];
static c89jpeg_u8 g_replay[W * H * 3UL];
static c89jpeg_u8 g_stage[4096UL];
static c89jpeg_u8 g_jpeg[JPEG_CAP];
static c89jpeg_u8 g_decode[W * H * 3UL];

static void make_pattern(void)
{
    c89jpeg_u16 x;
    c89jpeg_u16 y;
    for (y = 0; y < H; ++y) {
        for (x = 0; x < W; ++x) {
            c89jpeg_u32 i;
            i = ((c89jpeg_u32)y * W + x) * 3UL;
            g_rgb[i + 0] = (c89jpeg_u8)((x * 255U) / (W - 1));
            g_rgb[i + 1] = (c89jpeg_u8)((y * 255U) / (H - 1));
            g_rgb[i + 2] = (c89jpeg_u8)(((x * 9U) + (y * 17U) + ((x ^ y) * 5U)) & 255U);
        }
    }
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
    c89jpeg_encode_source_resume_params srp;
    c89jpeg_decode_params dp;
    c89jpeg_image_info info;
    c89jpeg_status st;
    c89jpeg_u32 out_size;
    c89jpeg_u32 fed_rows;
    c89jpeg_u32 prefix_bytes;

    make_pattern();
    memset(&srp, 0, sizeof(srp));
    srp.width = W;
    srp.height = H;
    srp.stride_bytes = W * 3UL;
    srp.pixel_format = C89JPEG_PIXFMT_RGB24;
    srp.subsampling = C89JPEG_SUBSAMP_420;
    srp.quality = 84;
    srp.restart_interval = 6;
    srp.emit_jfif = 1;
    srp.huffman_mode = C89JPEG_HUFFMAN_OPTIMAL;
    srp.row_window = g_window;
    srp.row_window_size = sizeof(g_window);
    srp.replay_buffer = g_replay;
    srp.replay_buffer_size = sizeof(g_replay);
    srp.staging_buffer = g_stage;
    srp.staging_capacity = sizeof(g_stage);
    srp.emit_chunk_size = 211;

    c89jpeg_encoder_init(&enc);
    st = c89jpeg_encoder_resume_begin_source(&resume, &enc, &srp);
    if (st != C89JPEG_OK) {
        printf("encoder_resume_begin_source failed: %s\n", c89jpeg_status_string(st));
        return 1;
    }

    out_size = 0;
    prefix_bytes = 0;
    for (;;) {
        c89jpeg_u32 chunk;
        chunk = 0;
        st = c89jpeg_encoder_resume_pull(&resume, g_jpeg + out_size, sizeof(g_jpeg) - out_size, &chunk);
        if (st != C89JPEG_OK && st != C89JPEG_SUSPENDED) {
            printf("initial encoder_resume_pull failed: %s\n", c89jpeg_status_string(st));
            return 1;
        }
        out_size += chunk;
        prefix_bytes += chunk;
        if (st == C89JPEG_OK || chunk == 0) break;
    }

    fed_rows = 0;
    for (;;) {
        int progressed;
        progressed = 0;
        for (;;) {
            c89jpeg_u32 chunk;
            chunk = 0;
            st = c89jpeg_encoder_resume_pull(&resume, g_jpeg + out_size, sizeof(g_jpeg) - out_size, &chunk);
            if (st != C89JPEG_OK && st != C89JPEG_SUSPENDED) {
                printf("encoder_resume_pull failed: %s\n", c89jpeg_status_string(st));
                return 1;
            }
            out_size += chunk;
            if (chunk != 0) progressed = 1;
            if (st == C89JPEG_OK) break;
            if (chunk == 0) break;
        }
        if (c89jpeg_encoder_resume_is_finished(&resume)) break;
        if (c89jpeg_encoder_resume_needs_input(&resume)) {
            c89jpeg_u32 need;
            c89jpeg_u32 accepted;
            need = c89jpeg_encoder_resume_source_rows_needed(&resume);
            if (need > 5) need = 5;
            st = c89jpeg_encoder_resume_feed_rows(&resume,
                                                  g_rgb + fed_rows * (W * 3UL),
                                                  need,
                                                  W * 3UL,
                                                  &accepted);
            if (st != C89JPEG_OK) {
                printf("encoder_resume_feed_rows failed: %s\n", c89jpeg_status_string(st));
                return 1;
            }
            fed_rows += accepted;
            if (fed_rows >= H) c89jpeg_encoder_resume_finish_input(&resume);
            progressed = 1;
        }
        if (!progressed) {
            printf("source-resume driver stalled\n");
            return 1;
        }
    }

    if (!write_file("build/example_source_resume_optimal_replay.jpg", g_jpeg, out_size)) {
        printf("failed to write build/example_source_resume_optimal_replay.jpg\n");
        return 1;
    }

    c89jpeg_decoder_init(&dec);
    memset(&dp, 0, sizeof(dp));
    dp.data = g_jpeg;
    dp.size = out_size;
    dp.out_pixels = g_decode;
    dp.out_capacity = sizeof(g_decode);
    dp.out_format = C89JPEG_DECODE_RGB24;
    dp.upsampling = C89JPEG_UPSAMPLE_LINEAR;
    st = c89jpeg_decode(&dec, &dp, &info);
    if (st != C89JPEG_OK) {
        printf("decode failed: %s\n", c89jpeg_status_string(st));
        return 1;
    }
    if (!write_ppm("build/example_source_resume_optimal_replay.ppm", g_decode, info.width, info.height)) {
        printf("failed to write build/example_source_resume_optimal_replay.ppm\n");
        return 1;
    }

    printf("source_resume_optimal_replay jpeg=%lu prefix=%lu rows=%lu replay=%lu\n",
           (unsigned long)out_size,
           (unsigned long)prefix_bytes,
           (unsigned long)c89jpeg_encoder_resume_source_rows_accepted(&resume),
           (unsigned long)c89jpeg_encoder_resume_replay_buffer_size(W, H, W * 3UL));
    printf("wrote build/example_source_resume_optimal_replay.jpg\n");
    printf("wrote build/example_source_resume_optimal_replay.ppm\n");
    return 0;
}
