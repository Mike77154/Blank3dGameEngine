/* SPDX-License-Identifier: CC0-1.0 */
#include "tifx.h"

#include <stdio.h>
#include <string.h>

#define G0_W 96UL
#define G0_H 64UL
#define R1_W 64UL
#define R1_H 48UL
#define B2_W 64UL
#define B2_H 64UL
#define B2_STRIDE ((B2_W + 7UL) / 8UL)

static unsigned char gray_page[G0_W * G0_H];
static unsigned char rgb_page[R1_W * R1_H * 3UL];
static unsigned char bilevel_page[B2_STRIDE * B2_H];
static unsigned char tiff_buffer[262144UL];

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

static void build_gray_page(void)
{
    unsigned long x;
    unsigned long y;
    for (y = 0UL; y < G0_H; ++y) {
        for (x = 0UL; x < G0_W; ++x) {
            gray_page[y * G0_W + x] = (unsigned char)(((x + y) * 255UL) / (G0_W + G0_H - 2UL));
        }
    }
}

static void build_rgb_page(void)
{
    unsigned long x;
    unsigned long y;
    for (y = 0UL; y < R1_H; ++y) {
        for (x = 0UL; x < R1_W; ++x) {
            unsigned long idx;
            idx = (y * R1_W + x) * 3UL;
            rgb_page[idx + 0UL] = (unsigned char)((x * 255UL) / (R1_W - 1UL));
            rgb_page[idx + 1UL] = (unsigned char)((y * 255UL) / (R1_H - 1UL));
            rgb_page[idx + 2UL] = (unsigned char)(((x ^ y) * 255UL) / (R1_W > R1_H ? (R1_W - 1UL) : (R1_H - 1UL)));
        }
    }
}

static void set_black(unsigned long x, unsigned long y)
{
    bilevel_page[y * B2_STRIDE + (x >> 3)] =
        (unsigned char)(bilevel_page[y * B2_STRIDE + (x >> 3)] |
                        (unsigned char)(0x80U >> (unsigned int)(x & 7UL)));
}

static void build_bilevel_page(void)
{
    unsigned long x;
    unsigned long y;

    memset(bilevel_page, 0, sizeof(bilevel_page));
    for (y = 0UL; y < B2_H; ++y) {
        for (x = 0UL; x < B2_W; ++x) {
            if ((x / 8UL + y / 8UL) & 1UL) {
                set_black(x, y);
            }
            if (x == y || x + y + 1UL == B2_W) {
                set_black(x, y);
            }
        }
    }
}

int main(void)
{
    tifx_write_params pages[3];
    unsigned long written;
    int rc;

    build_gray_page();
    build_rgb_page();
    build_bilevel_page();

    tifx_write_params_init(&pages[0]);
    pages[0].container_format = TIFX_CONTAINER_BIGTIFF;
    pages[0].pixel_format = TIFX_PIXEL_GRAY8;
    pages[0].width = G0_W;
    pages[0].height = G0_H;
    pages[0].stride = G0_W;
    pages[0].pixels = gray_page;
    pages[0].x_resolution = tifx_fp_from_ratio(300UL, 1UL);
    pages[0].y_resolution = tifx_fp_from_ratio(300UL, 1UL);

    tifx_write_params_init(&pages[1]);
    pages[1].container_format = TIFX_CONTAINER_BIGTIFF;
    pages[1].pixel_format = TIFX_PIXEL_RGB24;
    pages[1].width = R1_W;
    pages[1].height = R1_H;
    pages[1].stride = R1_W * 3UL;
    pages[1].pixels = rgb_page;
    pages[1].x_resolution = tifx_fp_from_ratio(300UL, 1UL);
    pages[1].y_resolution = tifx_fp_from_ratio(300UL, 1UL);

    tifx_write_params_init(&pages[2]);
    pages[2].container_format = TIFX_CONTAINER_BIGTIFF;
    pages[2].pixel_format = TIFX_PIXEL_BILEVEL;
    pages[2].compression = 4U;
    pages[2].photometric = 0U;
    pages[2].fill_order = 1U;
    pages[2].width = B2_W;
    pages[2].height = B2_H;
    pages[2].stride = B2_STRIDE;
    pages[2].rows_per_strip = 16UL;
    pages[2].pixels = bilevel_page;
    pages[2].x_resolution = tifx_fp_from_ratio(204UL, 1UL);
    pages[2].y_resolution = tifx_fp_from_ratio(196UL, 1UL);

    rc = tifx_write_bigtiff_pages_memory(tiff_buffer,
                                         sizeof(tiff_buffer),
                                         pages,
                                         3UL,
                                         &written);
    if (rc != TIFX_OK) {
        printf("write failed: %s\n", tifx_strerror(rc));
        return 1;
    }

    if (!write_file("multipage_big.tif", tiff_buffer, written)) {
        printf("could not write multipage_big.tif\n");
        return 1;
    }

    printf("wrote multipage_big.tif (%lu bytes, 3 pages)\n", written);
    return 0;
}
