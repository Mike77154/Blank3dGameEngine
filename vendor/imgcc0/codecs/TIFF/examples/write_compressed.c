/* SPDX-License-Identifier: CC0-1.0 */
#include "tifx.h"

#include <stdio.h>
#include <string.h>

#define W 96
#define H 64

static unsigned char gray_pixels[W * H];
static unsigned char rgb_pixels[W * H * 3];
static unsigned char tiff_buffer[W * H * 4 * 2 + 8192];

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

static void build_gray_gradient(void)
{
    unsigned long x;
    unsigned long y;
    for (y = 0UL; y < H; ++y) {
        for (x = 0UL; x < W; ++x) {
            gray_pixels[y * W + x] = (unsigned char)(((x * 5UL) + (y * 11UL)) & 255UL);
        }
    }
}

static void build_rgb_gradient(void)
{
    unsigned long x;
    unsigned long y;
    for (y = 0UL; y < H; ++y) {
        for (x = 0UL; x < W; ++x) {
            unsigned long idx;
            idx = (y * W + x) * 3UL;
            rgb_pixels[idx + 0UL] = (unsigned char)(((x * 3UL) + (y * 7UL)) & 255UL);
            rgb_pixels[idx + 1UL] = (unsigned char)(((x * 13UL) + (y * 5UL)) & 255UL);
            rgb_pixels[idx + 2UL] = (unsigned char)(((x * 17UL) + (y * 9UL)) & 255UL);
        }
    }
}

static int write_variant(const char *path,
                         unsigned short container_format,
                         unsigned short pixel_format,
                         unsigned short compression,
                         unsigned short predictor,
                         unsigned short deflate_mode,
                         unsigned long tile_width,
                         unsigned long tile_length,
                         const unsigned char *pixels,
                         unsigned long stride,
                         unsigned long rows_per_strip)
{
    tifx_write_params params;
    unsigned long written;
    int rc;

    memset(tiff_buffer, 0, sizeof(tiff_buffer));
    tifx_write_params_init(&params);
    params.container_format = container_format;
    params.pixel_format = pixel_format;
    params.compression = compression;
    params.predictor = predictor;
    params.deflate_mode = deflate_mode;
    params.tile_width = tile_width;
    params.tile_length = tile_length;
    params.width = W;
    params.height = H;
    params.stride = stride;
    params.rows_per_strip = rows_per_strip;
    params.pixels = pixels;
    params.x_resolution = tifx_fp_from_ratio(300UL, 1UL);
    params.y_resolution = tifx_fp_from_ratio(300UL, 1UL);

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
    build_gray_gradient();
    if (!write_variant("gradient_gray_lzw.tif", TIFX_CONTAINER_CLASSIC,
                       TIFX_PIXEL_GRAY8, 5U, TIFX_PREDICTOR_NONE,
                       TIFX_DEFLATE_STORED, 0UL, 0UL,
                       gray_pixels, W, 16UL)) {
        return 1;
    }
    if (!write_variant("gradient_gray_zip_legacy.tif", TIFX_CONTAINER_CLASSIC,
                       TIFX_PIXEL_GRAY8, 32946U, TIFX_PREDICTOR_HORIZONTAL,
                       TIFX_DEFLATE_FIXED, 0UL, 0UL,
                       gray_pixels, W, 16UL)) {
        return 1;
    }

    build_rgb_gradient();
    if (!write_variant("gradient_rgb_deflate.tif", TIFX_CONTAINER_CLASSIC,
                       TIFX_PIXEL_RGB24, 8U, TIFX_PREDICTOR_HORIZONTAL,
                       TIFX_DEFLATE_DYNAMIC, 0UL, 0UL,
                       rgb_pixels, W * 3UL, 16UL)) {
        return 1;
    }
    if (!write_variant("gradient_rgb_lzw_big.tif", TIFX_CONTAINER_BIGTIFF,
                       TIFX_PIXEL_RGB24, 5U, TIFX_PREDICTOR_HORIZONTAL,
                       TIFX_DEFLATE_STORED, 0UL, 0UL,
                       rgb_pixels, W * 3UL, 16UL)) {
        return 1;
    }
    if (!write_variant("gradient_rgb_deflate_tiled.tif", TIFX_CONTAINER_CLASSIC,
                       TIFX_PIXEL_RGB24, 8U, TIFX_PREDICTOR_HORIZONTAL,
                       TIFX_DEFLATE_DYNAMIC, 32UL, 32UL,
                       rgb_pixels, W * 3UL, 0UL)) {
        return 1;
    }
    if (!write_variant("gradient_rgb_deflate_auto.tif", TIFX_CONTAINER_CLASSIC,
                       TIFX_PIXEL_RGB24, 8U, TIFX_PREDICTOR_HORIZONTAL,
                       TIFX_DEFLATE_AUTO, 0UL, 0UL,
                       rgb_pixels, W * 3UL, 16UL)) {
        return 1;
    }

    return 0;
}
