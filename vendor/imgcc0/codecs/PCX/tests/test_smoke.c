#include <stdio.h>
#include <string.h>

#include "../pcx.h"

static int test_rgb_roundtrip(void)
{
    static const pcx_u8 src[24] = {
        255,0,0, 0,255,0, 0,0,255, 255,255,255,
        0,0,0, 128,64,32, 10,20,30, 200,150,100
    };
    pcx_u8 *encoded;
    pcx_size encoded_size;
    PCXEncodeOptions opt;
    PCXImage decoded;
    int rc;

    encoded = NULL;
    encoded_size = 0U;
    pcx_encode_options_default(&opt);
    rc = pcx_encode_rgb24(src, 4, 2, 12, &opt, &encoded, &encoded_size);
    if (rc != PCX_OK || encoded == NULL || encoded_size == 0U)
    {
        return 1;
    }

    pcx_image_init(&decoded);
    rc = pcx_load_memory(encoded, encoded_size, &decoded);
    if (rc != PCX_OK)
    {
        pcx_image_release(&decoded);
        return 2;
    }
    if (decoded.width != 4 || decoded.height != 2 || decoded.channels != 3)
    {
        pcx_image_release(&decoded);
        return 3;
    }
    if (memcmp(decoded.pixels, src, 24U) != 0)
    {
        pcx_image_release(&decoded);
        return 4;
    }
    pcx_image_release(&decoded);
    return 0;
}

static int test_indexed_roundtrip(void)
{
    static const pcx_u8 indices[8] = {0,1,2,3,3,2,1,0};
    PCXPalette pal;
    PCXEncodeOptions opt;
    pcx_u8 *encoded;
    pcx_size encoded_size;
    PCXIndexedImage decoded;
    int rc;

    pcx_palette_init(&pal);
    pal.isValid = 1;
    pal.colors[0][0] = 0U; pal.colors[0][1] = 0U; pal.colors[0][2] = 0U;
    pal.colors[1][0] = 255U; pal.colors[1][1] = 0U; pal.colors[1][2] = 0U;
    pal.colors[2][0] = 0U; pal.colors[2][1] = 255U; pal.colors[2][2] = 0U;
    pal.colors[3][0] = 0U; pal.colors[3][1] = 0U; pal.colors[3][2] = 255U;

    pcx_encode_options_default(&opt);
    encoded = NULL;
    encoded_size = 0U;
    rc = pcx_encode_indexed(indices, 4, 2, 4, 8, &pal, &opt, &encoded, &encoded_size);
    if (rc != PCX_OK || encoded == NULL || encoded_size == 0U)
    {
        return 1;
    }

    pcx_indexed_image_init(&decoded);
    rc = pcx_load_indexed_memory(encoded, encoded_size, &decoded);
    if (rc != PCX_OK)
    {
        pcx_indexed_image_release(&decoded);
        return 2;
    }
    if (decoded.width != 4 || decoded.height != 2 || decoded.totalBitsPerPixel != 8)
    {
        pcx_indexed_image_release(&decoded);
        return 3;
    }
    if (memcmp(decoded.indices, indices, 8U) != 0)
    {
        pcx_indexed_image_release(&decoded);
        return 4;
    }
    if (memcmp(decoded.palette.colors, pal.colors, 256U * 3U) != 0)
    {
        pcx_indexed_image_release(&decoded);
        return 5;
    }
    pcx_indexed_image_release(&decoded);
    return 0;
}

static int test_corpus_file(void)
{
    PCXImage img;
    PCXFileInfo info;
    int rc;

    pcx_image_init(&img);
    rc = pcx_load_with_info("tests/generated_corpus/valid_indexed8_palette.pcx", &img, &info);
    if (rc != PCX_OK)
    {
        pcx_image_release(&img);
        return 1;
    }
    if (img.width <= 0 || img.height <= 0 || img.channels != 3)
    {
        pcx_image_release(&img);
        return 2;
    }
    pcx_image_release(&img);
    return 0;
}

int main(void)
{
    int rc;

    rc = test_rgb_roundtrip();
    if (rc != 0)
    {
        fprintf(stderr, "rgb roundtrip failed: %d\n", rc);
        return 1;
    }
    rc = test_indexed_roundtrip();
    if (rc != 0)
    {
        fprintf(stderr, "indexed roundtrip failed: %d\n", rc);
        return 1;
    }
    rc = test_corpus_file();
    if (rc != 0)
    {
        fprintf(stderr, "corpus decode failed: %d\n", rc);
        return 1;
    }
    printf("pcx89 smoke: PASS\n");
    return 0;
}
