/* SPDX-License-Identifier: CC0-1.0 */
#include "tifx.h"

#include <stdio.h>
#include <string.h>

#define W 128
#define H 96

static unsigned char gray_pixels[W * H];
static unsigned char rgb_pixels[W * H * 3];
static unsigned char rgba_pixels[W * H * 4];
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
            gray_pixels[y * W + x] = (unsigned char)((x * 255UL) / (W - 1UL));
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
            rgb_pixels[idx + 0UL] = (unsigned char)((x * 255UL) / (W - 1UL));
            rgb_pixels[idx + 1UL] = (unsigned char)((y * 255UL) / (H - 1UL));
            rgb_pixels[idx + 2UL] = (unsigned char)(((x + y) * 255UL) / (W + H - 2UL));
        }
    }
}

static void build_rgba_gradient(void)
{
    unsigned long x;
    unsigned long y;
    for (y = 0UL; y < H; ++y) {
        for (x = 0UL; x < W; ++x) {
            unsigned long idx;
            unsigned char a;
            idx = (y * W + x) * 4UL;
            a = (unsigned char)(((x + y) * 255UL) / (W + H - 2UL));
            rgba_pixels[idx + 0UL] = (unsigned char)(((x * 255UL) / (W - 1UL) * (unsigned long)a) / 255UL);
            rgba_pixels[idx + 1UL] = (unsigned char)(((y * 255UL) / (H - 1UL) * (unsigned long)a) / 255UL);
            rgba_pixels[idx + 2UL] = (unsigned char)((((W - 1UL - x) * 255UL) / (W - 1UL) * (unsigned long)a) / 255UL);
            rgba_pixels[idx + 3UL] = a;
        }
    }
}

static int write_variant(const char *path,
                         unsigned short container_format,
                         unsigned short pixel_format,
                         unsigned short alpha_mode,
                         unsigned short planar_config,
                         const unsigned char *pixels,
                         unsigned long stride)
{
    tifx_write_params params;
    unsigned long written;
    int rc;

    memset(tiff_buffer, 0, sizeof(tiff_buffer));
    tifx_write_params_init(&params);
    params.container_format = container_format;
    params.pixel_format = pixel_format;
    params.width = W;
    params.height = H;
    params.alpha_mode = alpha_mode;
    params.planar_config = planar_config;
    params.stride = stride;
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
    if (!write_variant("gradient_gray.tif", TIFX_CONTAINER_CLASSIC,
                       TIFX_PIXEL_GRAY8, TIFX_ALPHA_NONE, 1U, gray_pixels, W)) {
        return 1;
    }
    if (!write_variant("gradient_gray_big.tif", TIFX_CONTAINER_BIGTIFF,
                       TIFX_PIXEL_GRAY8, TIFX_ALPHA_NONE, 1U, gray_pixels, W)) {
        return 1;
    }

    build_rgb_gradient();
    if (!write_variant("gradient_rgb.tif", TIFX_CONTAINER_CLASSIC,
                       TIFX_PIXEL_RGB24, TIFX_ALPHA_NONE, 1U, rgb_pixels, W * 3UL)) {
        return 1;
    }
    if (!write_variant("gradient_rgb_big.tif", TIFX_CONTAINER_BIGTIFF,
                       TIFX_PIXEL_RGB24, TIFX_ALPHA_NONE, 1U, rgb_pixels, W * 3UL)) {
        return 1;
    }
    if (!write_variant("gradient_rgb_planar.tif", TIFX_CONTAINER_CLASSIC,
                       TIFX_PIXEL_RGB24, TIFX_ALPHA_NONE, 2U, rgb_pixels, W * 3UL)) {
        return 1;
    }
    if (!write_variant("gradient_rgb_planar_big.tif", TIFX_CONTAINER_BIGTIFF,
                       TIFX_PIXEL_RGB24, TIFX_ALPHA_NONE, 2U, rgb_pixels, W * 3UL)) {
        return 1;
    }

    build_rgba_gradient();
    if (!write_variant("gradient_rgba.tif", TIFX_CONTAINER_CLASSIC,
                       TIFX_PIXEL_RGBA32, TIFX_ALPHA_ASSOCIATED, 1U, rgba_pixels, W * 4UL)) {
        return 1;
    }
    if (!write_variant("gradient_rgba_big.tif", TIFX_CONTAINER_BIGTIFF,
                       TIFX_PIXEL_RGBA32, TIFX_ALPHA_ASSOCIATED, 1U, rgba_pixels, W * 4UL)) {
        return 1;
    }
    if (!write_variant("gradient_rgba_planar.tif", TIFX_CONTAINER_CLASSIC,
                       TIFX_PIXEL_RGBA32, TIFX_ALPHA_ASSOCIATED, 2U, rgba_pixels, W * 4UL)) {
        return 1;
    }
    if (!write_variant("gradient_rgba_planar_big.tif", TIFX_CONTAINER_BIGTIFF,
                       TIFX_PIXEL_RGBA32, TIFX_ALPHA_ASSOCIATED, 2U, rgba_pixels, W * 4UL)) {
        return 1;
    }

    return 0;
}
