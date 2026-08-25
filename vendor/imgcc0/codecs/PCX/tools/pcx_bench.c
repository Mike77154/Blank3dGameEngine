#include <stdio.h>
#include <stdlib.h>

#include "../pcx.h"

#define BENCH_WIDTH 256
#define BENCH_HEIGHT 256
#define BENCH_STRIDE (BENCH_WIDTH * 3)
#define BENCH_RGB_BYTES (BENCH_STRIDE * BENCH_HEIGHT)

static pcx_u8 g_bench_rgb[BENCH_RGB_BYTES];

static void fill_rgb_pattern(void)
{
    int x;
    int y;
    int idx;

    for (y = 0; y < BENCH_HEIGHT; ++y)
    {
        for (x = 0; x < BENCH_WIDTH; ++x)
        {
            idx = (y * BENCH_WIDTH + x) * 3;
            g_bench_rgb[idx + 0] = (pcx_u8)((x * 5 + y * 3) & 0xFF);
            g_bench_rgb[idx + 1] = (pcx_u8)((x * 11 + y * 7) & 0xFF);
            g_bench_rgb[idx + 2] = (pcx_u8)((x * 13 + y * 17) & 0xFF);
        }
    }
}

static int inspect_loops(const pcx_u8 *encoded, pcx_size encoded_size, int loops)
{
    int i;
    int rc;
    PCXHeader hdr;
    int width;
    int height;

    for (i = 0; i < loops; ++i)
    {
        rc = pcx_inspect_memory(encoded, encoded_size, &hdr, &width, &height);
        if (rc != PCX_OK)
        {
            return rc;
        }
    }
    return PCX_OK;
}

static int decode_loops(const pcx_u8 *encoded, pcx_size encoded_size, int loops)
{
    int i;
    int rc;
    PCXImage img;

    for (i = 0; i < loops; ++i)
    {
        pcx_image_init(&img);
        rc = pcx_load_memory_strict(encoded, encoded_size, &img);
        if (rc != PCX_OK)
        {
            pcx_image_release(&img);
            return rc;
        }
        pcx_image_release(&img);
    }
    return PCX_OK;
}

static int encode_loops(int loops, pcx_u32 *checksum)
{
    int i;
    int rc;
    pcx_u8 *encoded;
    pcx_size encoded_size;
    PCXEncodeOptions opt;

    pcx_encode_options_default(&opt);
    *checksum = 0U;
    for (i = 0; i < loops; ++i)
    {
        encoded = NULL;
        encoded_size = 0U;
        rc = pcx_encode_rgb24(g_bench_rgb,
                              BENCH_WIDTH,
                              BENCH_HEIGHT,
                              BENCH_STRIDE,
                              &opt,
                              &encoded,
                              &encoded_size);
        if (rc != PCX_OK)
        {
            return rc;
        }
        if (encoded_size != 0U)
        {
            *checksum ^= (pcx_u32)encoded[0];
            *checksum ^= (pcx_u32)encoded[encoded_size - 1U] << 8;
            *checksum ^= (pcx_u32)encoded_size;
        }
    }
    return PCX_OK;
}

int main(int argc, char **argv)
{
    const int default_loops = 200;
    int loops;
    pcx_u8 *encoded;
    pcx_size encoded_size;
    PCXEncodeOptions opt;
    pcx_u32 checksum;
    int rc;

    loops = default_loops;
    if (argc > 1)
    {
        loops = atoi(argv[1]);
        if (loops <= 0)
        {
            loops = default_loops;
        }
    }

    fill_rgb_pattern();
    pcx_encode_options_default(&opt);
    encoded = NULL;
    encoded_size = 0U;
    rc = pcx_encode_rgb24(g_bench_rgb,
                          BENCH_WIDTH,
                          BENCH_HEIGHT,
                          BENCH_STRIDE,
                          &opt,
                          &encoded,
                          &encoded_size);
    if (rc != PCX_OK)
    {
        fprintf(stderr, "pcx_bench: setup encode failed: %s\n",
                pcx_result_to_string((PCXResult)rc));
        return 1;
    }

    printf("pcx89 deterministic stress benchmark\n");
    printf("  image=%dx%d rgb24\n", BENCH_WIDTH, BENCH_HEIGHT);
    printf("  encodedBytes=%u\n", (unsigned int)encoded_size);
    printf("  loops=%d\n", loops);

    rc = inspect_loops(encoded, encoded_size, loops * 2);
    if (rc != PCX_OK)
    {
        fprintf(stderr, "pcx_bench: inspect failed: %s\n",
                pcx_result_to_string((PCXResult)rc));
        return 1;
    }
    printf("  inspect_ops=%d PASS\n", loops * 2);

    rc = decode_loops(encoded, encoded_size, loops);
    if (rc != PCX_OK)
    {
        fprintf(stderr, "pcx_bench: decode failed: %s\n",
                pcx_result_to_string((PCXResult)rc));
        return 1;
    }
    printf("  decode_ops=%d PASS\n", loops);

    rc = encode_loops(loops, &checksum);
    if (rc != PCX_OK)
    {
        fprintf(stderr, "pcx_bench: encode failed: %s\n",
                pcx_result_to_string((PCXResult)rc));
        return 1;
    }
    printf("  encode_ops=%d PASS checksum=%u\n", loops, (unsigned int)checksum);
    return 0;
}
