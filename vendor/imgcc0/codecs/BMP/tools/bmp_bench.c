#include <stdio.h>
#include <time.h>

#include "../bmp/bmp.h"

#define BENCH_WIDTH 256U
#define BENCH_HEIGHT 256U
#define BENCH_STRIDE (BENCH_WIDTH * 4U)
#define BENCH_RGBA_CAPACITY (BENCH_STRIDE * BENCH_HEIGHT)
#define BENCH_ENCODE_CAPACITY (512U * 1024U)
#define BENCH_WORK_CAPACITY (512U * 1024U)
#define BENCH_DEFAULT_LOOPS 200U
#define BENCH_MAX_LOOPS 100000U

static bmp_u8 g_rgba[BENCH_RGBA_CAPACITY];
static bmp_u8 g_decoded[BENCH_RGBA_CAPACITY];
static bmp_u8 g_encoded[BENCH_ENCODE_CAPACITY];
static bmp_u8 g_workspace[BENCH_WORK_CAPACITY];

static bmp_u32 parse_positive_u32(const char *s, bmp_u32 fallback)
{
    bmp_u32 v = 0U;
    bmp_u32 d;
    if (!s || !*s) return fallback;
    while (*s) {
        if (*s < '0' || *s > '9') return fallback;
        d = (bmp_u32)(*s - '0');
        if (v > (4294967295U - d) / 10U) return fallback;
        v = v * 10U + d;
        ++s;
    }
    if (v == 0U || v > BENCH_MAX_LOOPS) return fallback;
    return v;
}

static bmp_u32 now_ticks(void)
{
    return (bmp_u32)clock();
}

static void print_measure(const char *name, bmp_u32 ticks, bmp_u32 operations)
{
    bmp_u32 cps = (bmp_u32)CLOCKS_PER_SEC;
    bmp_u32 sec_whole;
    bmp_u32 sec_frac_milli;
    bmp_u32 us_whole_total;
    bmp_u32 us_frac_total;
    bmp_u32 us_total;
    bmp_u32 us_op_whole;
    bmp_u32 us_op_frac_milli;

    if (cps == 0U || operations == 0U) {
        printf("  %s: timing unavailable\n", name);
        return;
    }

    sec_whole = ticks / cps;
    sec_frac_milli = ((ticks % cps) * 1000U) / cps;

    if (sec_whole > 4000U) {
        printf("  %s: %u.%03u s total\n", name,
               (unsigned)sec_whole, (unsigned)sec_frac_milli);
        return;
    }
    us_whole_total = sec_whole * 1000000U;
    us_frac_total = ((ticks % cps) * 1000000U) / cps;
    us_total = us_whole_total + us_frac_total;
    us_op_whole = us_total / operations;
    us_op_frac_milli = ((us_total % operations) * 1000U) / operations;

    printf("  %s: %u.%03u s total, %u.%03u us/op\n",
           name,
           (unsigned)sec_whole, (unsigned)sec_frac_milli,
           (unsigned)us_op_whole, (unsigned)us_op_frac_milli);
}

static void fill_rgba_pattern(void)
{
    bmp_u32 x;
    bmp_u32 y;
    for (y = 0U; y < BENCH_HEIGHT; ++y) {
        bmp_u8 *row = g_rgba + y * BENCH_STRIDE;
        for (x = 0U; x < BENCH_WIDTH; ++x) {
            row[x * 4U + 0U] = (bmp_u8)((x * 3U + y * 5U) & 0xFFU);
            row[x * 4U + 1U] = (bmp_u8)((x * 7U + y * 11U) & 0xFFU);
            row[x * 4U + 2U] = (bmp_u8)((x * 13U + y * 17U) & 0xFFU);
            row[x * 4U + 3U] = 255U;
        }
    }
}

static int parse_loops(const bmp_u8 *encoded, bmp_u32 encoded_size, bmp_u32 loops)
{
    bmp_u32 i;
    int rc;
    bmp_image img;
    for (i = 0U; i < loops; ++i) {
        rc = bmp_parse_memory(encoded, encoded_size, &img);
        if (rc != BMP_OK) return rc;
    }
    return BMP_OK;
}

static int decode_loops(const bmp_u8 *encoded, bmp_u32 encoded_size, bmp_u32 loops)
{
    bmp_u32 i;
    int rc;
    bmp_image img;
    for (i = 0U; i < loops; ++i) {
        rc = bmp_parse_memory(encoded, encoded_size, &img);
        if (rc != BMP_OK) return rc;
        rc = bmp_decode_to_rgba32(&img, g_decoded, BENCH_STRIDE);
        if (rc != BMP_OK) return rc;
    }
    return BMP_OK;
}

static int encode_loops(bmp_u32 loops, const bmp_encode_options *opt)
{
    bmp_u32 i;
    bmp_u32 encoded_size;
    int rc;
    for (i = 0U; i < loops; ++i) {
        encoded_size = 0U;
        rc = bmp_encode_rgba32_into(g_rgba, BENCH_WIDTH, BENCH_HEIGHT, BENCH_STRIDE,
                                    opt, g_encoded, BENCH_ENCODE_CAPACITY, &encoded_size,
                                    g_workspace, BENCH_WORK_CAPACITY);
        if (rc != BMP_OK) return rc;
    }
    return BMP_OK;
}

int main(int argc, char **argv)
{
    bmp_u32 loops = BENCH_DEFAULT_LOOPS;
    bmp_u32 parse_ops;
    bmp_u32 encoded_size = 0U;
    bmp_u32 workspace_need = 0U;
    bmp_u32 t0;
    bmp_u32 t1;
    bmp_encode_options opt;
    int rc;

    if (argc > 1) loops = parse_positive_u32(argv[1], BENCH_DEFAULT_LOOPS);
    parse_ops = loops <= (BENCH_MAX_LOOPS / 2U) ? loops * 2U : loops;

    fill_rgba_pattern();
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    rc = bmp_encode_rgba32_workspace_size(BENCH_WIDTH, BENCH_HEIGHT, &opt, &workspace_need);
    if (rc != BMP_OK || workspace_need > BENCH_WORK_CAPACITY) {
        fprintf(stderr, "bmp_bench: static workspace too small\n");
        return 1;
    }
    rc = bmp_encode_rgba32_into(g_rgba, BENCH_WIDTH, BENCH_HEIGHT, BENCH_STRIDE,
                                &opt, g_encoded, BENCH_ENCODE_CAPACITY, &encoded_size,
                                g_workspace, BENCH_WORK_CAPACITY);
    if (rc != BMP_OK) {
        fprintf(stderr, "bmp_bench: setup encode failed: %s\n", bmp_error_string(rc));
        return 1;
    }

    printf("bmp benchmark (C89 integer timing)\n");
    printf("  image=%ux%u rgba32 source\n", (unsigned)BENCH_WIDTH, (unsigned)BENCH_HEIGHT);
    printf("  encodedBytes=%u\n", (unsigned)encoded_size);
    printf("  loops=%u\n", (unsigned)loops);

    t0 = now_ticks();
    rc = parse_loops(g_encoded, encoded_size, parse_ops);
    t1 = now_ticks();
    if (rc != BMP_OK) { fprintf(stderr, "bmp_bench: parse failed: %s\n", bmp_error_string(rc)); return 1; }
    print_measure("parse", t1 - t0, parse_ops);

    t0 = now_ticks();
    rc = decode_loops(g_encoded, encoded_size, loops);
    t1 = now_ticks();
    if (rc != BMP_OK) { fprintf(stderr, "bmp_bench: decode failed: %s\n", bmp_error_string(rc)); return 1; }
    print_measure("decode", t1 - t0, loops);

    t0 = now_ticks();
    rc = encode_loops(loops, &opt);
    t1 = now_ticks();
    if (rc != BMP_OK) { fprintf(stderr, "bmp_bench: encode failed: %s\n", bmp_error_string(rc)); return 1; }
    print_measure("encode", t1 - t0, loops);
    return 0;
}
