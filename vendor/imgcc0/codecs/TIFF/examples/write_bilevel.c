/* SPDX-License-Identifier: CC0-1.0 */
#include "tifx.h"

#include <stdio.h>
#include <string.h>

#define W 128UL
#define H 96UL
#define STRIDE ((W + 7UL) / 8UL)

static unsigned char bilevel_pixels[STRIDE * H];
static unsigned char tiff_buffer[65536UL];

static int write_file(const char *path, const unsigned char *data, unsigned long size)
{
    FILE *fp;
    fp = fopen(path, "wb");
    if (fp == 0) {
        return 0;
    }
    if (fwrite(data, 1U, (size_t)size, fp) != (size_t)size) {
        fclose(fp);
        return 0;
    }
    fclose(fp);
    return 1;
}

static void set_black(unsigned long x, unsigned long y)
{
    bilevel_pixels[y * STRIDE + (x >> 3)] =
        (unsigned char)(bilevel_pixels[y * STRIDE + (x >> 3)] |
                        (unsigned char)(0x80U >> (unsigned int)(x & 7UL)));
}

static void build_pattern(void)
{
    unsigned long x;
    unsigned long y;

    memset(bilevel_pixels, 0, sizeof(bilevel_pixels));
    for (y = 0UL; y < H; ++y) {
        for (x = 0UL; x < W; ++x) {
            unsigned long block;
            block = (x / 8UL) + (y / 8UL);
            if ((block & 1UL) == 0UL || x == y || x + y + 1UL == W) {
                set_black(x, y);
            }
        }
    }
}

static int write_variant(const char *path,
                         unsigned short container_format,
                         unsigned short compression,
                         unsigned long t4_options,
                         unsigned long rows_per_strip,
                         unsigned short fill_order)
{
    tifx_write_params params;
    unsigned long written;
    int rc;

    tifx_write_params_init(&params);
    params.container_format = container_format;
    params.pixel_format = TIFX_PIXEL_BILEVEL;
    params.compression = compression;
    params.photometric = 0U;
    params.width = W;
    params.height = H;
    params.stride = STRIDE;
    params.rows_per_strip = rows_per_strip;
    params.t4_options = t4_options;
    params.fill_order = fill_order;
    params.pixels = bilevel_pixels;
    params.x_resolution = tifx_fp_from_ratio(204UL, 1UL);
    params.y_resolution = tifx_fp_from_ratio(196UL, 1UL);

    rc = tifx_write_memory(tiff_buffer, sizeof(tiff_buffer), &params, &written);
    if (rc != TIFX_OK) {
        printf("%s write failed: %s\n", path, tifx_strerror(rc));
        return 0;
    }
    if (!write_file(path, tiff_buffer, written)) {
        printf("could not write %s\n", path);
        return 0;
    }
    printf("wrote %s (%lu bytes)\n", path, written);
    return 1;
}

int main(void)
{
    build_pattern();

    if (!write_variant("bilevel_mh.tif", TIFX_CONTAINER_CLASSIC, 2U, 0UL, 16UL, 1U)) {
        return 1;
    }
    if (!write_variant("bilevel_t4_1d.tif", TIFX_CONTAINER_CLASSIC, 3U, 4UL, 16UL, 1U)) {
        return 1;
    }
    if (!write_variant("bilevel_t4_mr.tif", TIFX_CONTAINER_CLASSIC, 3U, 5UL, 16UL, 1U)) {
        return 1;
    }
    if (!write_variant("bilevel_t6.tif", TIFX_CONTAINER_CLASSIC, 4U, 0UL, 16UL, 1U)) {
        return 1;
    }
    if (!write_variant("bilevel_t6_fill2.tif", TIFX_CONTAINER_CLASSIC, 4U, 0UL, 16UL, 2U)) {
        return 1;
    }
    if (!write_variant("bilevel_t6_big.tif", TIFX_CONTAINER_BIGTIFF, 4U, 0UL, 16UL, 1U)) {
        return 1;
    }
    if (!write_variant("bilevel_t6_big_fill2.tif", TIFX_CONTAINER_BIGTIFF, 4U, 0UL, 16UL, 2U)) {
        return 1;
    }

    return 0;
}
