#include "c89jpeg.h"
#include "fixtures/progressive_pair89.h"

#include <stdio.h>
#include <string.h>

#define TEST_W 32U
#define TEST_H 24U
#define TEST_RGB_BYTES (TEST_W * TEST_H * 3U)
#define TEST_WS_BYTES (128U * 1024U)

static unsigned char baseline_rgb[TEST_RGB_BYTES];
static unsigned char progressive_rgb[TEST_RGB_BYTES];
static unsigned char progressive_ws[TEST_WS_BYTES];

static int decode_one(const unsigned char *jpg,
                      unsigned long jpg_size,
                      unsigned char *rgb,
                      int expect_progressive)
{
    c89jpeg_decoder dec;
    c89jpeg_image_info info;
    c89jpeg_decode_params p;
    c89jpeg_status st;
    c89jpeg_u32 ws_need;

    c89jpeg_decoder_init(&dec);
    memset(&info, 0, sizeof(info));
    st = c89jpeg_probe(&dec, jpg, (c89jpeg_u32)jpg_size, &info);
    if (st != C89JPEG_OK) return 0;
    if (info.width != TEST_W || info.height != TEST_H || info.components != 3U)
        return 0;
    if (!!info.progressive != !!expect_progressive) return 0;

    memset(&p, 0, sizeof(p));
    p.data = jpg;
    p.size = (c89jpeg_u32)jpg_size;
    p.out_pixels = rgb;
    p.out_capacity = (c89jpeg_u32)TEST_RGB_BYTES;
    p.out_stride = TEST_W * 3U;
    p.out_format = C89JPEG_DECODE_RGB24;
    p.upsampling = C89JPEG_UPSAMPLE_LINEAR;

    ws_need = c89jpeg_decoder_progressive_workspace_size(&info);
    if (expect_progressive) {
        if (!ws_need || ws_need > (c89jpeg_u32)sizeof(progressive_ws)) return 0;
        memset(progressive_ws, 0, sizeof(progressive_ws));
        p.progressive_workspace = progressive_ws;
        p.progressive_workspace_size = (c89jpeg_u32)sizeof(progressive_ws);
    } else if (ws_need != 0U) {
        return 0;
    }

    st = c89jpeg_decode(&dec, &p, &info);
    return st == C89JPEG_OK;
}

int main(void)
{
    if (!decode_one(c89jpeg_fixture_baseline420,
                    c89jpeg_fixture_baseline420_size,
                    baseline_rgb, 0)) {
        fprintf(stderr, "baseline fixture decode failed\n");
        return 1;
    }
    if (!decode_one(c89jpeg_fixture_progressive420,
                    c89jpeg_fixture_progressive420_size,
                    progressive_rgb, 1)) {
        fprintf(stderr, "progressive fixture decode failed\n");
        return 1;
    }
    if (memcmp(baseline_rgb, progressive_rgb, TEST_RGB_BYTES) != 0) {
        fprintf(stderr, "progressive coefficients differ from matched baseline fixture\n");
        return 1;
    }
    puts("progressive 4:2:0 matched-baseline decode: PASS");
    return 0;
}
