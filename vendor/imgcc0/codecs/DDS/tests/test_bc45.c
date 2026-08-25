#include <stdio.h>
#include <string.h>

#include "giffany_dds.h"
#include "test_support89.h"

#define FOURCC_ATI1 0x31495441u
#define FOURCC_ATI2 0x32495441u
#define LEGACY_BUFFER_CAP 4096u

static gdds_u8 g_legacy_buffer[LEGACY_BUFFER_CAP];

static int expect_result(const char* label, gdds_result got, gdds_result expected) {
    if (got != expected) {
        fprintf(stderr, "%s: expected %s, got %s\n",
                label, gdds_result_string(expected), gdds_result_string(got));
        return 0;
    }
    return 1;
}

static void write_u32le(gdds_u8* p, gdds_u32 v) {
    p[0] = (gdds_u8)(v & 0xFFu);
    p[1] = (gdds_u8)((v >> 8) & 0xFFu);
    p[2] = (gdds_u8)((v >> 16) & 0xFFu);
    p[3] = (gdds_u8)((v >> 24) & 0xFFu);
}

static void make_grayscale_rgba(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_u32 x;
    gdds_u32 y;
    gdds_u32 denom;
    gdds_u8 v;
    gdds_u8* p;
    denom = (w - 1u) + (h - 1u);
    if (denom == 0u) denom = 1u;
    for (y = 0u; y < h; ++y) {
        for (x = 0u; x < w; ++x) {
            v = (gdds_u8)(((x + y) * 255u) / denom);
            p = rgba + ((gdds_size)(y * w + x) * 4u);
            p[0] = v;
            p[1] = v;
            p[2] = v;
            p[3] = (gdds_u8)(255u - v / 3u);
        }
    }
}

static void make_rg_field_rgba(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_u32 x;
    gdds_u32 y;
    gdds_u32 denom;
    gdds_u8* p;
    denom = (w - 1u) + (h - 1u);
    if (denom == 0u) denom = 1u;
    for (y = 0u; y < h; ++y) {
        for (x = 0u; x < w; ++x) {
            p = rgba + ((gdds_size)(y * w + x) * 4u);
            p[0] = (gdds_u8)((x * 255u) / (w > 1u ? w - 1u : 1u));
            p[1] = (gdds_u8)((y * 255u) / (h > 1u ? h - 1u : 1u));
            p[2] = (gdds_u8)(((x * 19u) ^ (y * 37u)) & 255u);
            p[3] = (gdds_u8)(255u - ((x + y) * 255u) / denom);
        }
    }
}

static int convert_dx10_to_legacy_ati(const gdds_buffer* dx10_dds,
                                      gdds_u32 fourcc,
                                      gdds_buffer* out_legacy_dds) {
    const gdds_u8* src;
    gdds_size payload_size;
    if (dx10_dds == NULL || dx10_dds->data == NULL || out_legacy_dds == NULL || dx10_dds->size < 148u) return 0;
    payload_size = dx10_dds->size - 148u;
    if (128u + payload_size > LEGACY_BUFFER_CAP) return 0;
    src = (const gdds_u8*)dx10_dds->data;
    memcpy(g_legacy_buffer, src, 128u);
    memcpy(g_legacy_buffer + 128u, src + 148u, payload_size);
    write_u32le(g_legacy_buffer + 84u, fourcc);
    out_legacy_dds->data = g_legacy_buffer;
    out_legacy_dds->size = 128u + payload_size;
    return 1;
}

static int test_bc4_dx10_roundtrip(void) {
    enum { W = 8, H = 8 };
    gdds_u8 rgba[W * H * 4u];
    gdds_encode_options enc;
    gdds_buffer dds;
    gdds_info info;
    gdds_image image;
    enc = gdds_encode_options_default(GDDS_FORMAT_BC4_UNORM);
    memset(&dds, 0, sizeof(dds)); memset(&info, 0, sizeof(info)); memset(&image, 0, sizeof(image));
    make_grayscale_rgba(rgba, W, H);
    if (!expect_result("encode BC4", gdds_encode_memory_rgba8(rgba, W, H, &enc, &dds), GDDS_RESULT_OK)) return 0;
    if (!expect_result("inspect BC4", gdds_inspect_memory(dds.data, dds.size, &info), GDDS_RESULT_OK)) return 0;
    if (info.source_format != GDDS_FORMAT_BC4_UNORM || !info.has_dx10_header || info.data_offset != 148u || info.is_srgb) return 0;
    if (!expect_result("decode BC4", gdds_decode_memory(dds.data, dds.size, &image), GDDS_RESULT_OK)) return 0;
    if (!gdds_test_channel_mae_le_x2(rgba, image.pixels, W, H, 0u, 0u, 15u)) return 0;
    if (gdds_test_channel_error_sum(image.pixels, image.pixels, W, H, 0u, 1u) != 0u ||
        gdds_test_channel_error_sum(image.pixels, image.pixels, W, H, 0u, 2u) != 0u) return 0;
    if (!gdds_test_channel_constant(image.pixels, W, H, 3u, 255u)) return 0;
    gdds_image_release(&image); gdds_buffer_release(&dds);
    return 1;
}

static int test_bc5_dx10_roundtrip(void) {
    enum { W = 8, H = 8 };
    gdds_u8 rgba[W * H * 4u];
    gdds_encode_options enc;
    gdds_buffer dds;
    gdds_info info;
    gdds_image image;
    enc = gdds_encode_options_default(GDDS_FORMAT_BC5_UNORM);
    memset(&dds, 0, sizeof(dds)); memset(&info, 0, sizeof(info)); memset(&image, 0, sizeof(image));
    make_rg_field_rgba(rgba, W, H);
    if (!expect_result("encode BC5", gdds_encode_memory_rgba8(rgba, W, H, &enc, &dds), GDDS_RESULT_OK)) return 0;
    if (!expect_result("inspect BC5", gdds_inspect_memory(dds.data, dds.size, &info), GDDS_RESULT_OK)) return 0;
    if (info.source_format != GDDS_FORMAT_BC5_UNORM || !info.has_dx10_header || info.data_offset != 148u || info.is_srgb) return 0;
    if (!expect_result("decode BC5", gdds_decode_memory(dds.data, dds.size, &image), GDDS_RESULT_OK)) return 0;
    if (!gdds_test_channel_mae_le_x2(rgba, image.pixels, W, H, 0u, 0u, 15u) ||
        !gdds_test_channel_mae_le_x2(rgba, image.pixels, W, H, 1u, 1u, 15u)) return 0;
    if (!gdds_test_channel_constant(image.pixels, W, H, 2u, 0u) ||
        !gdds_test_channel_constant(image.pixels, W, H, 3u, 255u)) return 0;
    gdds_image_release(&image); gdds_buffer_release(&dds);
    return 1;
}

static int test_legacy_ati_read(gdds_format fmt, gdds_u32 fourcc, int channels) {
    enum { W = 8, H = 8 };
    gdds_u8 rgba[W * H * 4u];
    gdds_encode_options enc;
    gdds_buffer dx10;
    gdds_buffer legacy;
    gdds_info info;
    enc = gdds_encode_options_default(fmt);
    memset(&dx10, 0, sizeof(dx10)); memset(&legacy, 0, sizeof(legacy)); memset(&info, 0, sizeof(info));
    if (channels == 1) make_grayscale_rgba(rgba, W, H); else make_rg_field_rgba(rgba, W, H);
    if (!expect_result("encode ATI base", gdds_encode_memory_rgba8(rgba, W, H, &enc, &dx10), GDDS_RESULT_OK)) return 0;
    if (!convert_dx10_to_legacy_ati(&dx10, fourcc, &legacy)) return 0;
    if (!expect_result("inspect ATI legacy", gdds_inspect_memory(legacy.data, legacy.size, &info), GDDS_RESULT_OK)) return 0;
    if (info.source_format != fmt || info.has_dx10_header || info.data_offset != 128u) return 0;
    gdds_buffer_release(&legacy); gdds_buffer_release(&dx10);
    return 1;
}

static int test_legacy_ati1_read(void) { return test_legacy_ati_read(GDDS_FORMAT_BC4_UNORM, FOURCC_ATI1, 1); }
static int test_legacy_ati2_read(void) { return test_legacy_ati_read(GDDS_FORMAT_BC5_UNORM, FOURCC_ATI2, 2); }

int main(void) {
    if (!test_bc4_dx10_roundtrip()) return 1;
    if (!test_bc5_dx10_roundtrip()) return 1;
    if (!test_legacy_ati1_read()) return 1;
    if (!test_legacy_ati2_read()) return 1;
    puts("ok");
    return 0;
}
