/* SPDX-License-Identifier: CC0-1.0 */
#include "tifx.h"

#include <stdio.h>
#include <string.h>

static unsigned char out_buffer[131072UL];
static unsigned char root_pixels[64UL * 64UL];
static unsigned char reduced_pixels[32UL * 32UL];
static unsigned char tiny_pixels[16UL * 16UL];
static unsigned char layer_pixels[64UL * 64UL];
static unsigned char root_rgb_pixels[64UL * 64UL * 3UL];
static unsigned char reduced_rgb_pixels[32UL * 32UL * 3UL];

static void fill_gray(unsigned char *dst,
                      unsigned long width,
                      unsigned long height,
                      unsigned long stride,
                      unsigned long seed)
{
    unsigned long x;
    unsigned long y;
    for (y = 0UL; y < height; ++y) {
        for (x = 0UL; x < width; ++x) {
            dst[y * stride + x] = (unsigned char)((x * 3UL + y * 5UL + seed) & 255UL);
        }
    }
}

static void init_gray_node(tifx_tiff_node *node,
                           unsigned short container_format,
                           unsigned long width,
                           unsigned long height,
                           unsigned long stride,
                           unsigned long new_subfile_type,
                           const unsigned char *pixels,
                           const tifx_tiff_node *children,
                           unsigned long child_count)
{
    tifx_write_params_init(&node->image);
    node->image.container_format = container_format;
    node->image.pixel_format = TIFX_PIXEL_GRAY8;
    node->image.width = width;
    node->image.height = height;
    node->image.stride = stride;
    node->image.compression = 32773U;
    node->image.tile_width = 16UL;
    node->image.tile_length = 16UL;
    node->image.new_subfile_type = new_subfile_type;
    node->image.pixels = pixels;
    node->children = children;
    node->child_count = child_count;
}


static void fill_rgb(unsigned char *dst,
                     unsigned long width,
                     unsigned long height,
                     unsigned long stride,
                     unsigned long seed)
{
    unsigned long x;
    unsigned long y;
    for (y = 0UL; y < height; ++y) {
        for (x = 0UL; x < width; ++x) {
            unsigned long idx;
            idx = y * stride + x * 3UL;
            dst[idx + 0UL] = (unsigned char)((x * 11UL + y * 3UL + seed) & 255UL);
            dst[idx + 1UL] = (unsigned char)((x * 5UL + y * 9UL + seed * 3UL) & 255UL);
            dst[idx + 2UL] = (unsigned char)((x * 13UL + y * 7UL + seed * 5UL) & 255UL);
        }
    }
}

static void init_rgb_node(tifx_tiff_node *node,
                          unsigned short container_format,
                          unsigned long width,
                          unsigned long height,
                          unsigned long stride,
                          unsigned short deflate_mode,
                          unsigned long new_subfile_type,
                          const unsigned char *pixels,
                          const tifx_tiff_node *children,
                          unsigned long child_count)
{
    tifx_write_params_init(&node->image);
    node->image.container_format = container_format;
    node->image.pixel_format = TIFX_PIXEL_RGB24;
    node->image.width = width;
    node->image.height = height;
    node->image.stride = stride;
    node->image.compression = 8U;
    node->image.predictor = TIFX_PREDICTOR_HORIZONTAL;
    node->image.deflate_mode = deflate_mode;
    node->image.tile_width = 16UL;
    node->image.tile_length = 16UL;
    node->image.new_subfile_type = new_subfile_type;
    node->image.pixels = pixels;
    node->children = children;
    node->child_count = child_count;
}
static int write_tree_file(const char *path,
                           unsigned short container_format,
                           tifx_tiff_node *pages,
                           unsigned long page_count,
                           unsigned short subifd_style)
{
    FILE *fp;
    unsigned long written;
    int rc;

    pages[0].image.subifd_style = subifd_style;

    if (container_format == TIFX_CONTAINER_BIGTIFF) {
        rc = tifx_write_bigtiff_tree_memory(out_buffer,
                                            sizeof(out_buffer),
                                            pages,
                                            page_count,
                                            &written);
    } else {
        rc = tifx_write_classic_tree_memory(out_buffer,
                                            sizeof(out_buffer),
                                            pages,
                                            page_count,
                                            &written);
    }
    if (rc != TIFX_OK) {
        printf("write failed for %s: %s\n", path, tifx_strerror(rc));
        return 0;
    }

    fp = fopen(path, "wb");
    if (fp == 0) {
        printf("could not open %s\n", path);
        return 0;
    }
    if (fwrite(out_buffer, 1U, (size_t)written, fp) != (size_t)written) {
        fclose(fp);
        printf("could not write %s\n", path);
        return 0;
    }
    fclose(fp);
    printf("wrote %s (%lu bytes)\n", path, written);
    return 1;
}

int main(void)
{
    tifx_tiff_node classic_pages[1];
    tifx_tiff_node classic_root_children[2];
    tifx_tiff_node classic_reduced_children[1];
    tifx_tiff_node big_pages[1];
    tifx_tiff_node big_root_children[2];
    tifx_tiff_node big_reduced_children[1];
    tifx_tiff_node big_deflate_pages[1];
    tifx_tiff_node big_deflate_children[1];

    fill_gray(root_pixels, 64UL, 64UL, 64UL, 0UL);
    fill_gray(reduced_pixels, 32UL, 32UL, 32UL, 17UL);
    fill_gray(tiny_pixels, 16UL, 16UL, 16UL, 77UL);
    fill_gray(layer_pixels, 64UL, 64UL, 64UL, 131UL);
    fill_rgb(root_rgb_pixels, 64UL, 64UL, 64UL * 3UL, 9UL);
    fill_rgb(reduced_rgb_pixels, 32UL, 32UL, 32UL * 3UL, 41UL);

    init_gray_node(&classic_reduced_children[0],
                   TIFX_CONTAINER_CLASSIC,
                   16UL, 16UL, 16UL,
                   1UL,
                   tiny_pixels,
                   0,
                   0UL);
    init_gray_node(&classic_root_children[0],
                   TIFX_CONTAINER_CLASSIC,
                   32UL, 32UL, 32UL,
                   1UL,
                   reduced_pixels,
                   classic_reduced_children,
                   1UL);
    init_gray_node(&classic_root_children[1],
                   TIFX_CONTAINER_CLASSIC,
                   64UL, 64UL, 64UL,
                   0UL,
                   layer_pixels,
                   0,
                   0UL);
    init_gray_node(&classic_pages[0],
                   TIFX_CONTAINER_CLASSIC,
                   64UL, 64UL, 64UL,
                   0UL,
                   root_pixels,
                   classic_root_children,
                   2UL);

    init_gray_node(&big_reduced_children[0],
                   TIFX_CONTAINER_BIGTIFF,
                   16UL, 16UL, 16UL,
                   1UL,
                   tiny_pixels,
                   0,
                   0UL);
    init_gray_node(&big_root_children[0],
                   TIFX_CONTAINER_BIGTIFF,
                   32UL, 32UL, 32UL,
                   1UL,
                   reduced_pixels,
                   big_reduced_children,
                   1UL);
    init_gray_node(&big_root_children[1],
                   TIFX_CONTAINER_BIGTIFF,
                   64UL, 64UL, 64UL,
                   0UL,
                   layer_pixels,
                   0,
                   0UL);
    init_gray_node(&big_pages[0],
                   TIFX_CONTAINER_BIGTIFF,
                   64UL, 64UL, 64UL,
                   0UL,
                   root_pixels,
                   big_root_children,
                   2UL);

    init_rgb_node(&big_deflate_children[0],
                  TIFX_CONTAINER_BIGTIFF,
                  32UL, 32UL, 32UL * 3UL,
                  TIFX_DEFLATE_FIXED,
                  1UL,
                  reduced_rgb_pixels,
                  0,
                  0UL);
    init_rgb_node(&big_deflate_pages[0],
                  TIFX_CONTAINER_BIGTIFF,
                  64UL, 64UL, 64UL * 3UL,
                  TIFX_DEFLATE_DYNAMIC,
                  0UL,
                  root_rgb_pixels,
                  big_deflate_children,
                  1UL);

    if (!write_tree_file("subifd_tree_classic.tif",
                         TIFX_CONTAINER_CLASSIC,
                         classic_pages,
                         1UL,
                         TIFX_SUBIFD_STYLE_TREE)) {
        return 1;
    }
    if (!write_tree_file("subifd_chain_classic.tif",
                         TIFX_CONTAINER_CLASSIC,
                         classic_pages,
                         1UL,
                         TIFX_SUBIFD_STYLE_ADOBE_CHAIN)) {
        return 1;
    }
    if (!write_tree_file("subifd_tree_big.tif",
                         TIFX_CONTAINER_BIGTIFF,
                         big_pages,
                         1UL,
                         TIFX_SUBIFD_STYLE_TREE)) {
        return 1;
    }
    if (!write_tree_file("subifd_chain_big.tif",
                         TIFX_CONTAINER_BIGTIFF,
                         big_pages,
                         1UL,
                         TIFX_SUBIFD_STYLE_ADOBE_CHAIN)) {
        return 1;
    }
    if (!write_tree_file("subifd_pyramid_big_deflate.tif",
                         TIFX_CONTAINER_BIGTIFF,
                         big_deflate_pages,
                         1UL,
                         TIFX_SUBIFD_STYLE_TREE)) {
        return 1;
    }

    return 0;
}
