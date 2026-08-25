#include <stdio.h>
#include <string.h>

#include "../bmp/bmp.h"

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed: %s at %s:%d\n", #expr, __FILE__, __LINE__); \
    return 1; \
} } while (0)


#define TEST_BMP_CAPACITY       (1024U * 1024U)
#define TEST_WORK_CAPACITY      (1024U * 1024U)
#define TEST_RGBA_CAPACITY      (1024U * 1024U)
#define TEST_BUILD_CAPACITY     (1024U * 1024U)
#define TEST_PAYLOAD_CAPACITY   (1024U * 1024U)
#define TEST_TRAILER_CAPACITY   (1024U * 1024U)

static bmp_u8 g_test_bmp[TEST_BMP_CAPACITY];
static bmp_u8 g_test_work[TEST_WORK_CAPACITY];
static bmp_u8 g_test_rgba[TEST_RGBA_CAPACITY];
static bmp_u8 g_test_build[TEST_BUILD_CAPACITY];
static bmp_u8 g_test_payload[TEST_PAYLOAD_CAPACITY];
static bmp_u8 g_test_trailer[TEST_TRAILER_CAPACITY];

static int test_encode_rgba32(const bmp_u8 *rgba,
                              bmp_u32 width,
                              bmp_u32 height,
                              bmp_u32 stride,
                              const bmp_encode_options *opt,
                              bmp_u8 **out_bmp,
                              bmp_u32 *out_size)
{
    bmp_u32 need = 0U;
    int rc;
    if (!out_bmp || !out_size) return BMP_ERR_ARGUMENT;
    rc = bmp_encode_rgba32_workspace_size(width, height, opt, &need);
    if (rc != BMP_OK) return rc;
    if (need > TEST_WORK_CAPACITY) return BMP_ERR_BUFFER_TOO_SMALL;
    rc = bmp_encode_rgba32_into(rgba, width, height, stride, opt,
                                g_test_bmp, TEST_BMP_CAPACITY, out_size,
                                g_test_work, TEST_WORK_CAPACITY);
    if (rc == BMP_OK) *out_bmp = g_test_bmp;
    return rc;
}

static int test_encode_indexed(const bmp_u8 *indices,
                               bmp_u32 width,
                               bmp_u32 height,
                               bmp_u32 stride,
                               const bmp_palette_entry *palette,
                               bmp_u32 palette_size,
                               const bmp_encode_options *opt,
                               bmp_u8 **out_bmp,
                               bmp_u32 *out_size)
{
    bmp_u32 need = 0U;
    int rc;
    if (!out_bmp || !out_size) return BMP_ERR_ARGUMENT;
    rc = bmp_encode_indexed_workspace_size_bound(width, height, palette_size, opt, &need);
    if (rc != BMP_OK) return rc;
    if (need > TEST_WORK_CAPACITY) return BMP_ERR_BUFFER_TOO_SMALL;
    rc = bmp_encode_indexed_into(indices, width, height, stride, palette, palette_size, opt,
                                 g_test_bmp, TEST_BMP_CAPACITY, out_size,
                                 g_test_work, TEST_WORK_CAPACITY);
    if (rc == BMP_OK) *out_bmp = g_test_bmp;
    return rc;
}

static int test_decode_to_rgba32_with_limits(const bmp_image *img,
                                             const bmp_limits *limits,
                                             bmp_u8 **out_rgba,
                                             bmp_u32 *out_stride)
{
    bmp_u32 total = 0U;
    bmp_u32 width;
    int rc;
    if (!img || !out_rgba || !out_stride) return BMP_ERR_ARGUMENT;
    if (img->meta.width <= 0) return BMP_ERR_FORMAT;
    width = (bmp_u32)img->meta.width;
    if (width > 0x3FFFFFFFU) return BMP_ERR_LIMITS;
    *out_stride = width * 4U;
    rc = bmp_calc_rgba32_buffer_size(img, &total);
    if (rc != BMP_OK) return rc;
    if (total > TEST_RGBA_CAPACITY) return BMP_ERR_BUFFER_TOO_SMALL;
    rc = bmp_decode_to_rgba32_with_limits(img, limits, g_test_rgba, *out_stride);
    if (rc == BMP_OK) *out_rgba = g_test_rgba;
    return rc;
}

static int test_decode_to_rgba32(const bmp_image *img,
                                 bmp_u8 **out_rgba,
                                 bmp_u32 *out_stride)
{
    return test_decode_to_rgba32_with_limits(img, 0, out_rgba, out_stride);
}

static int test_copy_embedded_payload(const bmp_image *img,
                                      bmp_u32 *out_kind,
                                      bmp_u8 **out_copy,
                                      bmp_u32 *out_size)
{
    int rc;
    bmp_u32 need = 0U;
    const bmp_u8 *view = 0;
    if (!out_copy) return BMP_ERR_ARGUMENT;
    rc = bmp_get_embedded_payload(img, out_kind, &view, &need);
    if (rc != BMP_OK) return rc;
    if (need > TEST_PAYLOAD_CAPACITY) return BMP_ERR_BUFFER_TOO_SMALL;
    rc = bmp_copy_embedded_payload_into(img, out_kind, g_test_payload,
                                        TEST_PAYLOAD_CAPACITY, out_size);
    if (rc == BMP_OK) *out_copy = g_test_payload;
    return rc;
}

static void test_release(void *ptr)
{
    (void)ptr;
}

static int parse_and_decode(const bmp_u8 *data,
                            bmp_u32 size,
                            bmp_image *img,
                            bmp_u8 **rgba,
                            bmp_u32 *stride)
{
    int rc = bmp_parse_memory(data, size, img);
    if (rc != BMP_OK) {
        fprintf(stderr, "bmp_parse_memory failed: %s\n", bmp_error_string(rc));
        return rc;
    }
    rc = test_decode_to_rgba32(img, rgba, stride);
    if (rc != BMP_OK) {
        fprintf(stderr, "test_decode_to_rgba32 failed: %s\n", bmp_error_string(rc));
        return rc;
    }
    return BMP_OK;
}

static int parse_only(const bmp_u8 *data, bmp_u32 size)
{
    bmp_image img;
    return bmp_parse_memory(data, size, &img);
}

static void set_u32le(bmp_u8 *dst, bmp_u32 v)
{
    dst[0] = (bmp_u8)(v & 0xFFU);
    dst[1] = (bmp_u8)((v >> 8) & 0xFFU);
    dst[2] = (bmp_u8)((v >> 16) & 0xFFU);
    dst[3] = (bmp_u8)((v >> 24) & 0xFFU);
}

static void set_u16le(bmp_u8 *dst, bmp_u16 v)
{
    dst[0] = (bmp_u8)(v & 0xFFU);
    dst[1] = (bmp_u8)((v >> 8) & 0xFFU);
}

static void set_s32le(bmp_u8 *dst, bmp_s32 v)
{
    set_u32le(dst, (bmp_u32)v);
}

static bmp_u8 *build_embedded_payload_bmp(bmp_u32 compression,
                                          const bmp_u8 *payload,
                                          bmp_u32 payload_size,
                                          bmp_u32 *out_size)
{
    bmp_u32 total_size;
    bmp_u8 *data;

    if (!payload || !out_size) return 0;

    total_size = 54U + payload_size;
    if (total_size > TEST_BUILD_CAPACITY) return 0;
    data = g_test_build;
    memset(data, 0, total_size);

    data[0] = 'B';
    data[1] = 'M';
    set_u32le(data + 2, total_size);
    set_u32le(data + 10, 54U);

    set_u32le(data + 14, 40U);     /* BITMAPINFOHEADER */
    set_s32le(data + 18, 3);       /* width of decompressed image */
    set_s32le(data + 22, 2);       /* height of decompressed image */
    data[26] = 1U;                 /* planes */
    data[27] = 0U;
    data[28] = 0U;                 /* bpp = 0 for JPEG/PNG payload */
    data[29] = 0U;
    set_u32le(data + 30, compression);
    set_u32le(data + 34, payload_size);

    memcpy(data + 54, payload, payload_size);
    *out_size = total_size;
    return data;
}


static bmp_u8 *build_indexed2_bmp(const bmp_palette_entry *palette,
                                  const bmp_u8 *top_row,
                                  const bmp_u8 *bottom_row,
                                  bmp_u32 width,
                                  bmp_u32 height,
                                  bmp_u32 *out_size)
{
    bmp_u32 row_stride = 4U;
    bmp_u32 palette_bytes = 16U;
    bmp_u32 pixel_offset = 14U + 40U + palette_bytes;
    bmp_u32 total_size = pixel_offset + row_stride * height;
    bmp_u8 *data;
    bmp_u32 x;
    bmp_u8 *row;

    if (!palette || !top_row || !bottom_row || !out_size) return 0;
    if (total_size > TEST_BUILD_CAPACITY) return 0;
    data = g_test_build;
    memset(data, 0, total_size);

    data[0] = 'B';
    data[1] = 'M';
    set_u32le(data + 2, total_size);
    set_u32le(data + 10, pixel_offset);

    set_u32le(data + 14, 40U);
    set_s32le(data + 18, (bmp_s32)width);
    set_s32le(data + 22, (bmp_s32)height);
    set_u16le(data + 26, 1U);
    set_u16le(data + 28, 2U);
    set_u32le(data + 30, BMP_COMP_RGB);
    set_u32le(data + 34, row_stride * height);
    set_s32le(data + 38, 2835);
    set_s32le(data + 42, 2835);
    set_u32le(data + 46, 4U);
    set_u32le(data + 50, 4U);

    for (x = 0; x < 4U; ++x) {
        const bmp_palette_entry *pe = &palette[x];
        bmp_u8 *dst = data + 54U + x * 4U;
        dst[0] = pe->b;
        dst[1] = pe->g;
        dst[2] = pe->r;
        dst[3] = pe->a;
    }

    row = data + pixel_offset;
    for (x = 0; x < width; ++x) {
        row[x >> 2] = (bmp_u8)(row[x >> 2] | ((bottom_row[x] & 0x03U) << (6U - ((x & 3U) * 2U))));
    }
    row += row_stride;
    for (x = 0; x < width; ++x) {
        row[x >> 2] = (bmp_u8)(row[x >> 2] | ((top_row[x] & 0x03U) << (6U - ((x & 3U) * 2U))));
    }

    *out_size = total_size;
    return data;
}

static bmp_u8 *build_os2v2_64_bmp(const bmp_palette_entry *palette,
                                  const bmp_u8 *top_row,
                                  const bmp_u8 *bottom_row,
                                  bmp_u32 width,
                                  bmp_u32 height,
                                  bmp_u32 *out_size)
{
    bmp_u32 row_stride = 4U;
    bmp_u32 palette_bytes = 16U;
    bmp_u32 pixel_offset = 14U + 64U + palette_bytes;
    bmp_u32 total_size = pixel_offset + row_stride * height;
    bmp_u8 *data;
    bmp_u32 x;
    bmp_u8 *row;

    if (!palette || !top_row || !bottom_row || !out_size) return 0;
    if (total_size > TEST_BUILD_CAPACITY) return 0;
    data = g_test_build;
    memset(data, 0, total_size);

    data[0] = 'B';
    data[1] = 'M';
    set_u32le(data + 2, total_size);
    set_u32le(data + 10, pixel_offset);

    set_u32le(data + 14, 64U);
    set_s32le(data + 18, (bmp_s32)width);
    set_s32le(data + 22, (bmp_s32)height);
    set_u16le(data + 26, 1U);
    set_u16le(data + 28, 8U);
    set_u32le(data + 30, BMP_COMP_RGB);
    set_u32le(data + 34, row_stride * height);
    set_s32le(data + 38, 2835);
    set_s32le(data + 42, 2835);
    set_u32le(data + 46, 4U);
    set_u32le(data + 50, 4U);
    /* The remaining 24 bytes are OS/2 v2 extensions; keep them zeroed for a baseline sample. */

    for (x = 0; x < 4U; ++x) {
        const bmp_palette_entry *pe = &palette[x];
        bmp_u8 *dst = data + 78U + x * 4U;
        dst[0] = pe->b;
        dst[1] = pe->g;
        dst[2] = pe->r;
        dst[3] = pe->a;
    }

    row = data + pixel_offset;
    for (x = 0; x < width; ++x) row[x] = bottom_row[x];
    row += row_stride;
    for (x = 0; x < width; ++x) row[x] = top_row[x];

    *out_size = total_size;
    return data;
}

static void build_expected_from_palette(const bmp_u8 *indices,
                                        bmp_u32 count,
                                        const bmp_palette_entry *palette,
                                        bmp_u8 *out_rgba)
{
    bmp_u32 i;
    for (i = 0; i < count; ++i) {
        const bmp_palette_entry *pe = &palette[indices[i]];
        out_rgba[i * 4U + 0U] = pe->r;
        out_rgba[i * 4U + 1U] = pe->g;
        out_rgba[i * 4U + 2U] = pe->b;
        out_rgba[i * 4U + 3U] = 255U;
    }
}

static bmp_u8 q5(bmp_u8 v)
{
    unsigned q = (unsigned)((v * 31U + 127U) / 255U);
    return (bmp_u8)((q * 255U + 15U) / 31U);
}

static bmp_u8 q6(bmp_u8 v)
{
    unsigned q = (unsigned)((v * 63U + 127U) / 255U);
    return (bmp_u8)((q * 255U + 31U) / 63U);
}


static int check_security_limits(void)
{
    static const bmp_u8 rgba_small[24] = {
        255, 0,   0,   255,
        0,   255, 0,   255,
        0,   0,   255, 255,
        255, 255, 0,   255,
        255, 0,   255, 255,
        0,   255, 255, 255
    };
    bmp_encode_options opt;
    bmp_limits saved;
    bmp_limits limits;
    bmp_u8 *bmp_bytes = 0;
    bmp_u32 bmp_size = 0;
    int rc;

    bmp_get_global_limits(&saved);
    bmp_limits_default(&limits);
    limits.max_pixels = 4U;
    bmp_set_global_limits(&limits);

    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    rc = test_encode_rgba32(rgba_small, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);

    rc = parse_only(bmp_bytes, bmp_size);
    CHECK(rc == BMP_ERR_LIMITS);
    CHECK(strcmp(bmp_error_string(rc), "BMP_ERR_LIMITS") == 0);
    CHECK(strcmp(bmp_error_description(rc),
                 "the BMP object exceeds the configured security limits") == 0);

    test_release(bmp_bytes);
    bmp_set_global_limits(&saved);
    return 0;
}

static int check_per_call_limits(void)
{
    static const bmp_u8 rgba_small[24] = {
        255, 0,   0,   255,
        0,   255, 0,   255,
        0,   0,   255, 255,
        255, 255, 0,   255,
        255, 0,   255, 255,
        0,   255, 255, 255
    };
    bmp_encode_options opt;
    bmp_limits limits;
    bmp_u8 *bmp_bytes = 0;
    bmp_u32 bmp_size = 0;
    bmp_image img;
    bmp_u8 *decoded = 0;
    bmp_u32 stride = 0;
    int rc;

    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    rc = test_encode_rgba32(rgba_small, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);

    bmp_limits_default(&limits);
    limits.max_pixels = 4U;
    rc = bmp_parse_memory_with_limits(bmp_bytes, bmp_size, &limits, &img);
    CHECK(rc == BMP_ERR_LIMITS);

    rc = bmp_parse_memory(bmp_bytes, bmp_size, &img);
    CHECK(rc == BMP_OK);

    bmp_limits_default(&limits);
    limits.max_decoded_bytes = 8U;
    rc = test_decode_to_rgba32_with_limits(&img, &limits, &decoded, &stride);
    CHECK(rc == BMP_ERR_LIMITS);

    rc = test_decode_to_rgba32(&img, &decoded, &stride);
    CHECK(rc == BMP_OK);
    CHECK(stride == 12U);
    test_release(decoded);
    test_release(bmp_bytes);
    return 0;
}

int main(void)
{
    static const bmp_u8 rgba[24] = {
        255, 0,   0,   255,
        0,   255, 0,   255,
        0,   0,   255, 255,
        255, 255, 0,   255,
        255, 0,   255, 128,
        0,   255, 255, 64
    };
    static const bmp_u8 expected_opaque[24] = {
        255, 0,   0,   255,
        0,   255, 0,   255,
        0,   0,   255, 255,
        255, 255, 0,   255,
        255, 0,   255, 255,
        0,   255, 255, 255
    };
    static const bmp_u8 indices4[6] = { 1, 2, 3, 4, 5, 6 };
    static const bmp_u8 indices8[12] = { 1, 1, 1, 2, 3, 4, 4, 4, 4, 5, 6, 6 };
    static const bmp_palette_entry palette[7] = {
        {   0,   0,   0, 0 },
        {   0,   0, 255, 0 },
        {   0, 255,   0, 0 },
        { 255,   0,   0, 0 },
        {   0, 255, 255, 0 },
        { 255,   0, 255, 0 },
        { 255, 255,   0, 0 }
    };
    bmp_encode_options opt;
    bmp_u8 *bmp_bytes = 0;
    bmp_u32 bmp_size = 0;
    bmp_image img;
    bmp_u8 *decoded = 0;
    bmp_u32 stride = 0;
    bmp_u8 expected[48];
    bmp_diagnostics diag;
    int rc;

    /* 24-bit */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    rc = test_encode_rgba32(rgba, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    rc = parse_and_decode(bmp_bytes, bmp_size, &img, &decoded, &stride);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(diag.warning_mask == 0U);
    CHECK(diag.payload_signature_ok == -1);
    CHECK(stride == 12);
    CHECK(memcmp(decoded, expected_opaque, 24) == 0);
    test_release(decoded);
    test_release(bmp_bytes);

    /* 24-bit top-down */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    opt.top_down = 1;
    rc = test_encode_rgba32(rgba, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    rc = parse_and_decode(bmp_bytes, bmp_size, &img, &decoded, &stride);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(bmp_warning_mask_has(diag.warning_mask, BMP_WARN_TOP_DOWN) == 1);
    CHECK(memcmp(decoded, expected_opaque, 24) == 0);
    test_release(decoded);
    test_release(bmp_bytes);

    /* 32-bit BGRA */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGRA32;
    rc = test_encode_rgba32(rgba, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    rc = parse_and_decode(bmp_bytes, bmp_size, &img, &decoded, &stride);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(diag.warning_mask == 0U);
    CHECK(img.meta.alpha_mask == 0xFF000000U);
    CHECK(memcmp(decoded, rgba, 24) == 0);
    test_release(decoded);
    test_release(bmp_bytes);

    /* 16-bit 565 */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_RGB565;
    rc = test_encode_rgba32(rgba, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    rc = parse_and_decode(bmp_bytes, bmp_size, &img, &decoded, &stride);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(diag.warning_mask == 0U);
    expected[0] = q5(255); expected[1] = q6(0);   expected[2] = q5(0);   expected[3] = 255;
    expected[4] = q5(0);   expected[5] = q6(255); expected[6] = q5(0);   expected[7] = 255;
    expected[8] = q5(0);   expected[9] = q6(0);   expected[10] = q5(255);expected[11] = 255;
    expected[12]= q5(255); expected[13]= q6(255); expected[14]= q5(0);   expected[15] = 255;
    expected[16]= q5(255); expected[17]= q6(0);   expected[18]= q5(255); expected[19] = 255;
    expected[20]= q5(0);   expected[21]= q6(255); expected[22]= q5(255); expected[23] = 255;
    CHECK(memcmp(decoded, expected, 24) == 0);
    test_release(decoded);
    test_release(bmp_bytes);

    /* Indexed 4bpp */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_INDEXED4;
    rc = test_encode_indexed(indices4, 3, 2, 3, palette, 7, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    rc = parse_and_decode(bmp_bytes, bmp_size, &img, &decoded, &stride);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(diag.warning_mask == 0U);
    build_expected_from_palette(indices4, 6, palette, expected);
    CHECK(memcmp(decoded, expected, 24) == 0);
    test_release(decoded);
    test_release(bmp_bytes);

    /* Indexed 2bpp rare-case coverage */
    {
        static const bmp_palette_entry pal2[4] = {
            {   0,   0,   0, 0 },
            {   0,   0, 255, 0 },
            {   0, 255,   0, 0 },
            { 255,   0,   0, 0 }
        };
        static const bmp_u8 top_row2[5] = { 0, 1, 2, 3, 1 };
        static const bmp_u8 bottom_row2[5] = { 3, 2, 1, 0, 2 };
        bmp_bytes = build_indexed2_bmp(pal2, top_row2, bottom_row2, 5U, 2U, &bmp_size);
        CHECK(bmp_bytes != 0);
        rc = parse_and_decode(bmp_bytes, bmp_size, &img, &decoded, &stride);
        CHECK(rc == BMP_OK);
        CHECK(img.meta.bpp == 2U);
        CHECK(img.meta.dib_type == BMP_DIB_INFO);
        build_expected_from_palette(top_row2, 5, pal2, expected);
        build_expected_from_palette(bottom_row2, 5, pal2, expected + 20);
        CHECK(stride == 20U);
        CHECK(memcmp(decoded, expected, 40) == 0);
        test_release(decoded);
        test_release(bmp_bytes);
    }

    /* OS/2 v2 64-byte header rare-case coverage */
    {
        static const bmp_palette_entry pal_os2[4] = {
            {   0,   0,   0, 0 },
            {   0,   0, 255, 0 },
            {   0, 255,   0, 0 },
            { 255, 255,   0, 0 }
        };
        static const bmp_u8 top_row_os2[4] = { 0, 1, 2, 3 };
        static const bmp_u8 bottom_row_os2[4] = { 3, 2, 1, 0 };
        bmp_bytes = build_os2v2_64_bmp(pal_os2, top_row_os2, bottom_row_os2, 4U, 2U, &bmp_size);
        CHECK(bmp_bytes != 0);
        rc = parse_and_decode(bmp_bytes, bmp_size, &img, &decoded, &stride);
        CHECK(rc == BMP_OK);
        CHECK(img.meta.dib_header_size == 64U);
        CHECK(img.meta.dib_type == BMP_DIB_OS2V2);
        build_expected_from_palette(top_row_os2, 4, pal_os2, expected);
        build_expected_from_palette(bottom_row_os2, 4, pal_os2, expected + 16);
        CHECK(stride == 16U);
        CHECK(memcmp(decoded, expected, 32) == 0);
        test_release(decoded);
        test_release(bmp_bytes);
    }

    /* Indexed 4bpp RLE4 */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_INDEXED4;
    opt.compression = BMP_ENC_COMP_RLE4;
    rc = test_encode_indexed(indices4, 3, 2, 3, palette, 7, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    rc = parse_and_decode(bmp_bytes, bmp_size, &img, &decoded, &stride);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(diag.warning_mask == 0U);
    build_expected_from_palette(indices4, 6, palette, expected);
    CHECK(memcmp(decoded, expected, 24) == 0);
    test_release(decoded);
    test_release(bmp_bytes);

    /* Indexed 8bpp RLE8 */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_INDEXED8;
    opt.compression = BMP_ENC_COMP_RLE8;
    rc = test_encode_indexed(indices8, 6, 2, 6, palette, 7, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    rc = parse_and_decode(bmp_bytes, bmp_size, &img, &decoded, &stride);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(diag.warning_mask == 0U);
    build_expected_from_palette(indices8, 12, palette, expected);
    CHECK(memcmp(decoded, expected, 48) == 0);
    test_release(decoded);

    /* File header reserved fields must be zero. */
    bmp_bytes[6] = 1U;
    CHECK(parse_only(bmp_bytes, bmp_size) == BMP_ERR_FORMAT);
    bmp_bytes[6] = 0U;

    /* Top-down + RLE8 is invalid. */
    set_s32le(bmp_bytes + 22, -2);
    CHECK(parse_only(bmp_bytes, bmp_size) == BMP_ERR_FORMAT);
    test_release(bmp_bytes);

    /* colors_used must fit indexed depth. */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_INDEXED4;
    rc = test_encode_indexed(indices4, 3, 2, 3, palette, 7, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    set_u32le(bmp_bytes + 46, 17U);
    CHECK(parse_only(bmp_bytes, bmp_size) == BMP_ERR_PALETTE);
    test_release(bmp_bytes);

    /* Pixel offset cannot point inside the palette/masks area. */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_INDEXED4;
    rc = test_encode_indexed(indices4, 3, 2, 3, palette, 7, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    set_u32le(bmp_bytes + 10, 54U);
    CHECK(parse_only(bmp_bytes, bmp_size) == BMP_ERR_FORMAT);
    test_release(bmp_bytes);

    /* BITFIELDS masks must not overlap. */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_RGB565;
    rc = test_encode_rgba32(rgba, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    set_u32le(bmp_bytes + 54, 0x07E0U);
    CHECK(parse_only(bmp_bytes, bmp_size) == BMP_ERR_MASKS);
    test_release(bmp_bytes);

    /* ICC profile size without data must be rejected at encode time. */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    opt.header_size = 124U;
    opt.icc_profile_size = 4U;
    opt.icc_profile_data = 0;
    rc = test_encode_rgba32(rgba, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_ERR_ARGUMENT);

    /* BI_PNG payload extraction */
    {
        static const bmp_u8 png_payload[16] = {
            0x89U, 'P', 'N', 'G', 0x0DU, 0x0AU, 0x1AU, 0x0AU,
            0x00U, 0x00U, 0x00U, 0x0DU, 'I', 'H', 'D', 'R'
        };
        const bmp_u8 *payload_view = 0;
        bmp_u8 *payload_copy = 0;
        bmp_u32 payload_size = 0;
        bmp_u32 payload_kind = 0;

        bmp_bytes = build_embedded_payload_bmp(BMP_COMP_PNG,
                                               png_payload,
                                               (bmp_u32)sizeof(png_payload),
                                               &bmp_size);
        CHECK(bmp_bytes != 0);
        rc = bmp_parse_memory(bmp_bytes, bmp_size, &img);
        CHECK(rc == BMP_OK);
        rc = bmp_collect_diagnostics(&img, &diag);
        CHECK(rc == BMP_OK);
        CHECK(bmp_warning_mask_has(diag.warning_mask, BMP_WARN_EMBEDDED_PAYLOAD) == 1);
        CHECK(diag.payload_signature_ok == 1);
        CHECK(img.palette == 0);
        CHECK(img.meta.palette_entries == 0U);
        CHECK(bmp_image_has_embedded_payload(&img) == 1);
        CHECK(bmp_validate_embedded_payload_signature(&img) == BMP_OK);
        CHECK(bmp_embedded_payload_signature_matches(&img) == 1);
        CHECK(strcmp(bmp_embedded_payload_extension(&img), "png") == 0);
        rc = bmp_get_embedded_payload(&img, &payload_kind, &payload_view, &payload_size);
        CHECK(rc == BMP_OK);
        CHECK(payload_kind == BMP_COMP_PNG);
        CHECK(payload_size == (bmp_u32)sizeof(png_payload));
        CHECK(memcmp(payload_view, png_payload, sizeof(png_payload)) == 0);
        rc = test_copy_embedded_payload(&img, &payload_kind, &payload_copy, &payload_size);
        CHECK(rc == BMP_OK);
        CHECK(payload_kind == BMP_COMP_PNG);
        CHECK(payload_size == (bmp_u32)sizeof(png_payload));
        CHECK(memcmp(payload_copy, png_payload, sizeof(png_payload)) == 0);
        test_release(payload_copy);
        rc = test_decode_to_rgba32(&img, &decoded, &stride);
        CHECK(rc == BMP_ERR_UNSUPPORTED);
        test_release(bmp_bytes);
    }

    /* BI_JPEG payload extraction */
    {
        static const bmp_u8 jpeg_payload[10] = {
            0xFFU, 0xD8U, 0xFFU, 0xE0U, 0x00U, 0x10U, 'J', 'F', 'I', 'F'
        };
        const bmp_u8 *payload_view = 0;
        bmp_u32 payload_size = 0;
        bmp_u32 payload_kind = 0;

        bmp_bytes = build_embedded_payload_bmp(BMP_COMP_JPEG,
                                               jpeg_payload,
                                               (bmp_u32)sizeof(jpeg_payload),
                                               &bmp_size);
        CHECK(bmp_bytes != 0);
        rc = bmp_parse_memory(bmp_bytes, bmp_size, &img);
        CHECK(rc == BMP_OK);
        rc = bmp_collect_diagnostics(&img, &diag);
        CHECK(rc == BMP_OK);
        CHECK(bmp_warning_mask_has(diag.warning_mask, BMP_WARN_EMBEDDED_PAYLOAD) == 1);
        CHECK(diag.payload_signature_ok == 1);
        CHECK(bmp_image_has_embedded_payload(&img) == 1);
        CHECK(bmp_validate_embedded_payload_signature(&img) == BMP_OK);
        CHECK(bmp_embedded_payload_signature_matches(&img) == 1);
        CHECK(strcmp(bmp_embedded_payload_extension(&img), "jpg") == 0);
        rc = bmp_get_embedded_payload(&img, &payload_kind, &payload_view, &payload_size);
        CHECK(rc == BMP_OK);
        CHECK(payload_kind == BMP_COMP_JPEG);
        CHECK(payload_size == (bmp_u32)sizeof(jpeg_payload));
        CHECK(memcmp(payload_view, jpeg_payload, sizeof(jpeg_payload)) == 0);

        /* Invalid bpp for JPEG/PNG must be rejected. */
        set_u32le(bmp_bytes + 28, 3U);
        CHECK(parse_only(bmp_bytes, bmp_size) == BMP_ERR_UNSUPPORTED);
        test_release(bmp_bytes);
    }

    /* BI_PNG payload with wrong signature must be rejected at parse time. */
    {
        static const bmp_u8 bad_png_payload[16] = {
            0x89U, 'B', 'A', 'D', 0x0DU, 0x0AU, 0x1AU, 0x0AU,
            0x00U, 0x00U, 0x00U, 0x0DU, 'I', 'H', 'D', 'R'
        };
        bmp_bytes = build_embedded_payload_bmp(BMP_COMP_PNG,
                                               bad_png_payload,
                                               (bmp_u32)sizeof(bad_png_payload),
                                               &bmp_size);
        CHECK(bmp_bytes != 0);
        CHECK(parse_only(bmp_bytes, bmp_size) == BMP_ERR_FORMAT);
        test_release(bmp_bytes);
    }

    /* BI_JPEG payload too short must be treated as truncation. */
    {
        static const bmp_u8 short_jpeg_payload[2] = { 0xFFU, 0xD8U };
        bmp_bytes = build_embedded_payload_bmp(BMP_COMP_JPEG,
                                               short_jpeg_payload,
                                               (bmp_u32)sizeof(short_jpeg_payload),
                                               &bmp_size);
        CHECK(bmp_bytes != 0);
        CHECK(parse_only(bmp_bytes, bmp_size) == BMP_ERR_STREAM);
        test_release(bmp_bytes);
    }

    /* file_size = 0 is tolerated but reported. */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    rc = test_encode_rgba32(rgba, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    set_u32le(bmp_bytes + 2, 0U);
    rc = bmp_parse_memory(bmp_bytes, bmp_size, &img);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(bmp_warning_mask_has(diag.warning_mask,
                               BMP_WARN_FILE_SIZE_HEADER_ZERO) == 1);
    test_release(bmp_bytes);

    /* file_size smaller than actual is tolerated when only trailing garbage exceeds it. */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    rc = test_encode_rgba32(rgba, 1, 1, 4, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    {
        bmp_u8 *with_trailer = g_test_trailer;
        CHECK(bmp_size + 4U <= TEST_TRAILER_CAPACITY);
        memset(with_trailer, 0, bmp_size + 4U);
        memcpy(with_trailer, bmp_bytes, bmp_size);
        rc = bmp_parse_memory(with_trailer, bmp_size + 4U, &img);
        CHECK(rc == BMP_OK);
        rc = bmp_collect_diagnostics(&img, &diag);
        CHECK(rc == BMP_OK);
        CHECK(bmp_warning_mask_has(diag.warning_mask,
                                   BMP_WARN_FILE_SIZE_HEADER_SMALLER_ACTUAL) == 1);
        test_release(with_trailer);
    }
    test_release(bmp_bytes);

    /* colors_used on truecolor images is tolerated but reported. */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    rc = test_encode_rgba32(rgba, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    set_u32le(bmp_bytes + 46, 3U);
    rc = bmp_parse_memory(bmp_bytes, bmp_size, &img);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(bmp_warning_mask_has(diag.warning_mask,
                               BMP_WARN_TRUECOLOR_COLORS_USED_NONZERO) == 1);
    test_release(bmp_bytes);

    /* colors_important beyond the declared palette is tolerated but reported. */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_INDEXED4;
    rc = test_encode_indexed(indices4, 3, 2, 3, palette, 7, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    set_u32le(bmp_bytes + 50, 17U);
    rc = bmp_parse_memory(bmp_bytes, bmp_size, &img);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(bmp_warning_mask_has(diag.warning_mask,
                               BMP_WARN_COLORS_IMPORTANT_EXCEEDS_PALETTE) == 1);
    test_release(bmp_bytes);

    /* compressed image_size = 0 falls back to EOF and is reported. */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_INDEXED8;
    opt.compression = BMP_ENC_COMP_RLE8;
    rc = test_encode_indexed(indices8, 6, 2, 6, palette, 7, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    set_u32le(bmp_bytes + 34, 0U);
    rc = bmp_parse_memory(bmp_bytes, bmp_size, &img);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(bmp_warning_mask_has(diag.warning_mask,
                               BMP_WARN_IMAGE_SIZE_ZERO_COMPRESSED) == 1);
    test_release(bmp_bytes);

    /* top-down BI_RGB is valid and reported as such. */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    rc = test_encode_rgba32(rgba, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    set_s32le(bmp_bytes + 22, -2);
    rc = bmp_parse_memory(bmp_bytes, bmp_size, &img);
    CHECK(rc == BMP_OK);
    rc = bmp_collect_diagnostics(&img, &diag);
    CHECK(rc == BMP_OK);
    CHECK(bmp_warning_mask_has(diag.warning_mask,
                               BMP_WARN_TOP_DOWN) == 1);
    test_release(bmp_bytes);


    /* Round 11: report helpers + runtime version macros */
    bmp_encode_options_default(&opt);
    opt.format = BMP_ENC_FMT_BGR24;
    rc = test_encode_rgba32(rgba, 3, 2, 12, &opt, &bmp_bytes, &bmp_size);
    CHECK(rc == BMP_OK);
    rc = bmp_parse_memory(bmp_bytes, bmp_size, &img);
    CHECK(rc == BMP_OK);
    CHECK(strcmp(bmp_version_string(), BMP_VERSION_STRING) == 0);
    CHECK(bmp_version_number() == BMP_VERSION_NUMBER);
    CHECK(strcmp(bmp_dib_type_string(img.meta.dib_type), "BMP_DIB_INFO") == 0);
    CHECK(strcmp(bmp_compression_string(img.meta.compression), "BMP_COMP_RGB") == 0);
    {
        char textBuf[1024];
        char jsonBuf[1024];
        rc = bmp_format_diagnostics_text(&img, textBuf, sizeof(textBuf));
        CHECK(rc > 0);
        CHECK(strstr(textBuf, "format=bmp") != 0);
        CHECK(strstr(textBuf, "libraryVersion=" BMP_VERSION_STRING) != 0);
        CHECK(strstr(textBuf, "compression=BMP_COMP_RGB") != 0);

        rc = bmp_format_diagnostics_json(&img, jsonBuf, sizeof(jsonBuf));
        CHECK(rc > 0);
        CHECK(strstr(jsonBuf, "\"format\":\"bmp\"") != 0);
        CHECK(strstr(jsonBuf, "\"libraryVersion\":\"" BMP_VERSION_STRING "\"") != 0);
        CHECK(strstr(jsonBuf, "\"warnings\":[]") != 0);
    }
    test_release(bmp_bytes);

    CHECK(check_security_limits() == 0);
    CHECK(check_per_call_limits() == 0);

    puts("test_smoke: OK");
    return 0;
}
