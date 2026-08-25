#ifndef BMP_FUZZ_C89_COMMON_H
#define BMP_FUZZ_C89_COMMON_H

#include "../bmp/bmp.h"

#ifndef BMP_FUZZ_INPUT_CAPACITY
#define BMP_FUZZ_INPUT_CAPACITY (16U * 1024U * 1024U)
#endif
#ifndef BMP_FUZZ_RGBA_CAPACITY
#define BMP_FUZZ_RGBA_CAPACITY (64U * 1024U * 1024U)
#endif

#define BMP_FUZZ_MODE_ALL     0
#define BMP_FUZZ_MODE_PARSE   1
#define BMP_FUZZ_MODE_DECODE  2
#define BMP_FUZZ_MODE_PAYLOAD 3

static bmp_u8 bmp_fuzz_input[BMP_FUZZ_INPUT_CAPACITY];
static bmp_u8 bmp_fuzz_rgba[BMP_FUZZ_RGBA_CAPACITY];
static volatile bmp_u32 bmp_fuzz_sink = 0U;

static void bmp_fuzz_touch(const bmp_u8 *data, bmp_u32 size)
{
    if (data && size != 0U) {
        bmp_fuzz_sink ^= (bmp_u32)data[0];
        bmp_fuzz_sink ^= (bmp_u32)data[size - 1U] << 8;
    }
}

static void bmp_fuzz_process(const bmp_u8 *data, bmp_u32 size, int mode)
{
    bmp_image img;
    bmp_diagnostics diag;
    const bmp_u8 *payload = 0;
    bmp_u32 payload_kind = 0U;
    bmp_u32 payload_size = 0U;
    bmp_u32 rgba_size = 0U;
    bmp_u32 stride;
    bmp_u32 width;
    int rc;

    rc = bmp_parse_memory(data, size, &img);
    if (rc != BMP_OK) return;
    bmp_fuzz_sink ^= (bmp_u32)img.meta.bpp;

    if (mode == BMP_FUZZ_MODE_PARSE) return;

    if (mode == BMP_FUZZ_MODE_PAYLOAD || mode == BMP_FUZZ_MODE_ALL) {
        if (bmp_image_has_embedded_payload(&img)) {
            rc = bmp_get_embedded_payload(&img, &payload_kind, &payload, &payload_size);
            if (rc == BMP_OK) {
                bmp_fuzz_sink ^= payload_kind;
                bmp_fuzz_touch(payload, payload_size);
            }
        }
        if (mode == BMP_FUZZ_MODE_PAYLOAD) return;
    }

    if (mode == BMP_FUZZ_MODE_DECODE || mode == BMP_FUZZ_MODE_ALL) {
        if (img.meta.width > 0) {
            width = (bmp_u32)img.meta.width;
            if (width <= 0x3FFFFFFFU) {
                stride = width * 4U;
                rc = bmp_calc_rgba32_buffer_size(&img, &rgba_size);
                if (rc == BMP_OK && rgba_size <= BMP_FUZZ_RGBA_CAPACITY) {
                    rc = bmp_decode_to_rgba32(&img, bmp_fuzz_rgba, stride);
                    if (rc == BMP_OK) bmp_fuzz_touch(bmp_fuzz_rgba, rgba_size);
                }
            }
        }
    }

    bmp_diagnostics_default(&diag);
    rc = bmp_collect_diagnostics(&img, &diag);
    if (rc == BMP_OK) bmp_fuzz_sink ^= diag.warning_mask;
}

#endif
