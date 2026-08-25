#include <stdio.h>
#include <string.h>

#include "giffany_dds.h"
#include "test_support89.h"

#define FOURCC_DX10 0x30315844u
#define FOURCC_DXT2 0x32545844u
#define DDSCAPS2_CUBEMAP 0x00000200u
#define DXGI_R8G8B8A8_UNORM 28u
#define DDS_DIMENSION_TEXTURE2D 3u

#define DX10_SCRATCH_CAP 4096u
static gdds_u8 g_dx10_scratch[DX10_SCRATCH_CAP];
static gdds_info g_scratch_info;

static void write_u32le(gdds_u8* p, gdds_u32 v) {
    p[0] = (gdds_u8)(v & 0xFFu);
    p[1] = (gdds_u8)((v >> 8) & 0xFFu);
    p[2] = (gdds_u8)((v >> 16) & 0xFFu);
    p[3] = (gdds_u8)((v >> 24) & 0xFFu);
}

static void make_opaque_pattern(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_u32 x, y;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            gdds_u8* p = rgba + (((gdds_size)y * w + x) * 4u);
            p[0] = (gdds_u8)((x * 255u) / (w > 1u ? (w - 1u) : 1u));
            p[1] = (gdds_u8)((y * 255u) / (h > 1u ? (h - 1u) : 1u));
            p[2] = (gdds_u8)(((x ^ y) * 255u) / (((w > h ? w : h) > 1u) ? ((w > h ? w : h) - 1u) : 1u));
            p[3] = 255u;
        }
    }
}

static void make_alpha_pattern(gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_u32 x, y;
    for (y = 0; y < h; ++y) {
        for (x = 0; x < w; ++x) {
            gdds_u8* p = rgba + (((gdds_size)y * w + x) * 4u);
            p[0] = (gdds_u8)((x * 255u) / (w > 1u ? (w - 1u) : 1u));
            p[1] = (gdds_u8)(255u - ((y * 255u) / (h > 1u ? (h - 1u) : 1u)));
            {
                gdds_u32 denom = (w > 1u && h > 1u) ? ((w - 1u) * (h - 1u)) : 1u;
                p[2] = (gdds_u8)(((x * y) * 255u) / denom);
            }
            p[3] = (gdds_u8)(((x + y) * 255u) / (((w - 1u) + (h - 1u)) ? ((w - 1u) + (h - 1u)) : 1u));
        }
    }
}

static int expect_result(const char* label, gdds_result got, gdds_result expected) {
    if (got != expected) {
        fprintf(stderr, "%s: expected %s, got %s\n",
                label,
                gdds_result_string(expected),
                gdds_result_string(got));
        return 0;
    }
    return 1;
}

static gdds_parse_options strict_options(void) {
    gdds_parse_options options = gdds_parse_options_default();
    options.mode = GDDS_PARSE_MODE_STRICT;
    return options;
}

static int roundtrip_exact(const gdds_u8* src, gdds_u32 w, gdds_u32 h, gdds_format fmt) {
    gdds_buffer dds = {0};
    gdds_image decoded = {0};
    gdds_encode_options options = gdds_encode_options_default(fmt);
    gdds_result rc = gdds_encode_memory_rgba8(src, w, h, &options, &dds);
    if (rc != GDDS_RESULT_OK) {
        fprintf(stderr, "encode failed for %d: %s\n", (int)fmt, gdds_result_string(rc));
        return 0;
    }
    rc = gdds_decode_memory(dds.data, dds.size, &decoded);
    if (rc != GDDS_RESULT_OK) {
        fprintf(stderr, "decode failed for %d: %s\n", (int)fmt, gdds_result_string(rc));
        gdds_buffer_release(&dds);
        return 0;
    }
    if (decoded.width != w || decoded.height != h) {
        fprintf(stderr, "size mismatch for %d\n", (int)fmt);
        gdds_buffer_release(&dds);
        gdds_image_release(&decoded);
        return 0;
    }
    if (memcmp(src, decoded.pixels, (gdds_size)w * h * 4u) != 0) {
        fprintf(stderr, "exact mismatch for %d\n", (int)fmt);
        gdds_buffer_release(&dds);
        gdds_image_release(&decoded);
        return 0;
    }
    if (decoded.source_format != fmt) {
        fprintf(stderr, "source format mismatch for %d\n", (int)fmt);
        gdds_buffer_release(&dds);
        gdds_image_release(&decoded);
        return 0;
    }
    if (decoded.alpha_mode != GDDS_ALPHA_MODE_UNKNOWN) {
        fprintf(stderr, "unexpected alpha mode for exact format %d\n", (int)fmt);
        gdds_buffer_release(&dds);
        gdds_image_release(&decoded);
        return 0;
    }
    gdds_buffer_release(&dds);
    gdds_image_release(&decoded);
    return 1;
}

static int roundtrip_lossy(const gdds_u8* src, gdds_u32 w, gdds_u32 h, gdds_format fmt, gdds_u32 max_mae) {
    gdds_buffer dds = {0};
    gdds_image decoded = {0};
    gdds_encode_options options = gdds_encode_options_default(fmt);
    gdds_result rc = gdds_encode_memory_rgba8(src, w, h, &options, &dds);
    if (rc != GDDS_RESULT_OK) {
        fprintf(stderr, "encode failed for %d: %s\n", (int)fmt, gdds_result_string(rc));
        return 0;
    }
    rc = gdds_decode_memory(dds.data, dds.size, &decoded);
    if (rc != GDDS_RESULT_OK) {
        fprintf(stderr, "decode failed for %d: %s\n", (int)fmt, gdds_result_string(rc));
        gdds_buffer_release(&dds);
        return 0;
    }
    if (decoded.width != w || decoded.height != h) {
        fprintf(stderr, "size mismatch for %d\n", (int)fmt);
        gdds_buffer_release(&dds);
        gdds_image_release(&decoded);
        return 0;
    }
    if (!gdds_test_mae_le(src, decoded.pixels, (gdds_size)(w * h * 4u), max_mae)) {
        fprintf(stderr, "MAE too high for %d (limit %u)\n", (int)fmt, max_mae);
        gdds_buffer_release(&dds);
        gdds_image_release(&decoded);
        return 0;
    }
    gdds_buffer_release(&dds);
    gdds_image_release(&decoded);
    return 1;
}

static int test_missing_optional_flags_permissive_and_strict(const gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_buffer dds = {0};
    gdds_image image = {0};
    gdds_info info = {0};
    gdds_encode_options options = gdds_encode_options_default(GDDS_FORMAT_RGBA8);
    gdds_parse_options strict = strict_options();
    gdds_result rc = gdds_encode_memory_rgba8(rgba, w, h, &options, &dds);
    if (!expect_result("encode base", rc, GDDS_RESULT_OK)) return 0;

    write_u32le((gdds_u8*)dds.data + 8u, 0u);
    write_u32le((gdds_u8*)dds.data + 108u, 0u);

    rc = gdds_inspect_memory(dds.data, dds.size, &info);
    if (!expect_result("inspect flags-tolerant", rc, GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }
    if ((info.warning_flags & (GDDS_WARNING_MISSING_HEADER_FLAGS | GDDS_WARNING_MISSING_TEXTURE_CAPS)) !=
        (GDDS_WARNING_MISSING_HEADER_FLAGS | GDDS_WARNING_MISSING_TEXTURE_CAPS)) {
        fprintf(stderr, "missing warning bits for permissive header parse\n");
        gdds_buffer_release(&dds);
        return 0;
    }

    rc = gdds_inspect_memory_ex(dds.data, dds.size, &strict, &g_scratch_info);
    if (!expect_result("inspect strict missing flags", rc, GDDS_RESULT_UNSUPPORTED)) {
        gdds_buffer_release(&dds);
        return 0;
    }

    rc = gdds_decode_memory(dds.data, dds.size, &image);
    if (!expect_result("decode flags-tolerant", rc, GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }
    if (memcmp(image.pixels, rgba, (gdds_size)w * h * 4u) != 0) {
        fprintf(stderr, "decoded pixels mismatch on optional flag tolerance\n");
        gdds_image_release(&image);
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_image_release(&image);
    gdds_buffer_release(&dds);
    return 1;
}

static int test_noncanonical_pitch_warned(const gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_buffer dds = {0};
    gdds_info info = {0};
    gdds_encode_options options = gdds_encode_options_default(GDDS_FORMAT_RGBA8);
    gdds_result rc = gdds_encode_memory_rgba8(rgba, w, h, &options, &dds);
    if (!expect_result("encode pitch warning base", rc, GDDS_RESULT_OK)) return 0;

    write_u32le((gdds_u8*)dds.data + 20u, (w * 4u) - 1u);
    rc = gdds_inspect_memory(dds.data, dds.size, &info);
    if (!expect_result("inspect noncanonical pitch", rc, GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }
    if ((info.warning_flags & GDDS_WARNING_INCONSISTENT_PITCH) == 0u) {
        fprintf(stderr, "missing pitch warning bit\n");
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_buffer_release(&dds);
    return 1;
}

static int test_truncated_mip_chain(const gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_buffer dds = {0};
    gdds_encode_options options = gdds_encode_options_default(GDDS_FORMAT_RGBA8);
    gdds_result rc = gdds_encode_memory_rgba8(rgba, w, h, &options, &dds);
    if (!expect_result("encode mip truncation base", rc, GDDS_RESULT_OK)) return 0;

    write_u32le((gdds_u8*)dds.data + 28u, 4u);
    rc = gdds_inspect_memory(dds.data, dds.size, &g_scratch_info);
    if (!expect_result("inspect truncated mip chain", rc, GDDS_RESULT_TRUNCATED)) {
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_buffer_release(&dds);
    return 1;
}

static int test_invalid_mask_rejected(const gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_buffer dds = {0};
    gdds_encode_options options = gdds_encode_options_default(GDDS_FORMAT_RGBA8);
    gdds_result rc = gdds_encode_memory_rgba8(rgba, w, h, &options, &dds);
    if (!expect_result("encode invalid mask base", rc, GDDS_RESULT_OK)) return 0;

    write_u32le((gdds_u8*)dds.data + 92u, 0x00F000F0u);
    rc = gdds_inspect_memory(dds.data, dds.size, &g_scratch_info);
    if (!expect_result("inspect invalid mask", rc, GDDS_RESULT_UNSUPPORTED)) {
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_buffer_release(&dds);
    return 1;
}

static int test_legacy_cubemap_rejected(const gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_buffer dds = {0};
    gdds_encode_options options = gdds_encode_options_default(GDDS_FORMAT_RGBA8);
    gdds_result rc = gdds_encode_memory_rgba8(rgba, w, h, &options, &dds);
    if (!expect_result("encode cubemap base", rc, GDDS_RESULT_OK)) return 0;

    write_u32le((gdds_u8*)dds.data + 112u, DDSCAPS2_CUBEMAP);
    rc = gdds_inspect_memory(dds.data, dds.size, &g_scratch_info);
    if (!expect_result("inspect legacy cubemap", rc, GDDS_RESULT_UNSUPPORTED)) {
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_buffer_release(&dds);
    return 1;
}

static int make_dx10_rgba8_dds(const gdds_u8* rgba,
                               gdds_u32 w,
                               gdds_u32 h,
                               gdds_buffer* out_buffer) {
    gdds_buffer legacy = {0};
    gdds_encode_options options = gdds_encode_options_default(GDDS_FORMAT_RGBA8);
    gdds_u8* dx10;
    gdds_size payload_size;

    gdds_result rc = gdds_encode_memory_rgba8(rgba, w, h, &options, &legacy);
    if (rc != GDDS_RESULT_OK) return 0;
    if (legacy.size < 128u) {
        gdds_buffer_release(&legacy);
        return 0;
    }

    payload_size = legacy.size - 128u;
    if (148u + payload_size > DX10_SCRATCH_CAP) {
        gdds_buffer_release(&legacy);
        return 0;
    }
    dx10 = g_dx10_scratch;

    memcpy(dx10, legacy.data, 128u);
    memset(dx10 + 128u, 0, 20u);
    memcpy(dx10 + 148u, (const gdds_u8*)legacy.data + 128u, payload_size);

    write_u32le(dx10 + 80u, 0x00000004u);
    write_u32le(dx10 + 84u, FOURCC_DX10);
    write_u32le(dx10 + 88u, 0u);
    write_u32le(dx10 + 92u, 0u);
    write_u32le(dx10 + 96u, 0u);
    write_u32le(dx10 + 100u, 0u);
    write_u32le(dx10 + 104u, 0u);

    write_u32le(dx10 + 128u, DXGI_R8G8B8A8_UNORM);
    write_u32le(dx10 + 132u, DDS_DIMENSION_TEXTURE2D);
    write_u32le(dx10 + 136u, 0u);
    write_u32le(dx10 + 140u, 1u);
    write_u32le(dx10 + 144u, 0u);

    out_buffer->data = dx10;
    out_buffer->size = 148u + payload_size;
    gdds_buffer_release(&legacy);
    return 1;
}

static int test_dx10_rgba8_decode_exact(const gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_buffer dds = {0};
    gdds_info info = {0};
    gdds_image image = {0};
    gdds_result rc;

    if (!make_dx10_rgba8_dds(rgba, w, h, &dds)) {
        fprintf(stderr, "failed to build synthetic DX10 DDS\n");
        return 0;
    }

    rc = gdds_inspect_memory(dds.data, dds.size, &info);
    if (!expect_result("inspect dx10 rgba8", rc, GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }
    if (!info.has_dx10_header || info.source_format != GDDS_FORMAT_RGBA8 || info.alpha_mode != GDDS_ALPHA_MODE_UNKNOWN) {
        fprintf(stderr, "DX10 inspect metadata mismatch\n");
        gdds_buffer_release(&dds);
        return 0;
    }
    if (info.data_offset != 148u || info.top_level_size != (gdds_size)w * h * 4u) {
        fprintf(stderr, "DX10 size metadata mismatch\n");
        gdds_buffer_release(&dds);
        return 0;
    }

    rc = gdds_decode_memory(dds.data, dds.size, &image);
    if (!expect_result("decode dx10 rgba8", rc, GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }
    if (memcmp(image.pixels, rgba, (gdds_size)w * h * 4u) != 0 || image.alpha_mode != GDDS_ALPHA_MODE_UNKNOWN) {
        fprintf(stderr, "DX10 decode pixels or metadata mismatch\n");
        gdds_image_release(&image);
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_image_release(&image);
    gdds_buffer_release(&dds);
    return 1;
}

static int test_dx10_array_rejected(const gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_buffer dds = {0};
    gdds_result rc;

    if (!make_dx10_rgba8_dds(rgba, w, h, &dds)) {
        fprintf(stderr, "failed to build synthetic DX10 DDS for array rejection\n");
        return 0;
    }

    write_u32le((gdds_u8*)dds.data + 140u, 2u);
    rc = gdds_inspect_memory(dds.data, dds.size, &g_scratch_info);
    if (!expect_result("inspect dx10 array", rc, GDDS_RESULT_UNSUPPORTED)) {
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_buffer_release(&dds);
    return 1;
}

static int test_dx10_reserved_bits_permissive_and_strict(const gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_buffer dds = {0};
    gdds_info info = {0};
    gdds_parse_options strict = strict_options();
    gdds_result rc;

    if (!make_dx10_rgba8_dds(rgba, w, h, &dds)) {
        fprintf(stderr, "failed to build synthetic DX10 DDS for reserved bit test\n");
        return 0;
    }

    write_u32le((gdds_u8*)dds.data + 144u, 0x20u);
    rc = gdds_inspect_memory(dds.data, dds.size, &info);
    if (!expect_result("inspect dx10 reserved permissive", rc, GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }
    if ((info.warning_flags & GDDS_WARNING_DX10_RESERVED_BITS) == 0u) {
        fprintf(stderr, "missing DX10 reserved-bit warning\n");
        gdds_buffer_release(&dds);
        return 0;
    }

    rc = gdds_inspect_memory_ex(dds.data, dds.size, &strict, &g_scratch_info);
    if (!expect_result("inspect dx10 reserved strict", rc, GDDS_RESULT_UNSUPPORTED)) {
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_buffer_release(&dds);
    return 1;
}

static int test_dx10_alpha_mode_reported(const gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_buffer dds = {0};
    gdds_info info = {0};
    gdds_image image = {0};
    gdds_result rc;

    if (!make_dx10_rgba8_dds(rgba, w, h, &dds)) {
        fprintf(stderr, "failed to build synthetic DX10 DDS for alpha mode test\n");
        return 0;
    }

    write_u32le((gdds_u8*)dds.data + 144u, GDDS_ALPHA_MODE_PREMULTIPLIED);
    rc = gdds_inspect_memory(dds.data, dds.size, &info);
    if (!expect_result("inspect dx10 alpha mode", rc, GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }
    if (info.alpha_mode != GDDS_ALPHA_MODE_PREMULTIPLIED) {
        fprintf(stderr, "DX10 alpha mode not reported\n");
        gdds_buffer_release(&dds);
        return 0;
    }

    rc = gdds_decode_memory(dds.data, dds.size, &image);
    if (!expect_result("decode dx10 alpha mode", rc, GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }
    if (image.alpha_mode != GDDS_ALPHA_MODE_PREMULTIPLIED) {
        fprintf(stderr, "DX10 decoded alpha mode mismatch\n");
        gdds_image_release(&image);
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_image_release(&image);
    gdds_buffer_release(&dds);
    return 1;
}

static int test_legacy_dxt2_alpha_mode_reported(const gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_buffer dds = {0};
    gdds_info info = {0};
    gdds_image image = {0};
    gdds_encode_options options = gdds_encode_options_default(GDDS_FORMAT_DXT3);
    gdds_result rc = gdds_encode_memory_rgba8(rgba, w, h, &options, &dds);
    if (!expect_result("encode dxt2 synthetic base", rc, GDDS_RESULT_OK)) return 0;

    write_u32le((gdds_u8*)dds.data + 84u, FOURCC_DXT2);
    rc = gdds_inspect_memory(dds.data, dds.size, &info);
    if (!expect_result("inspect dxt2 synthetic", rc, GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }
    if (info.source_format != GDDS_FORMAT_DXT3 || info.alpha_mode != GDDS_ALPHA_MODE_PREMULTIPLIED) {
        fprintf(stderr, "legacy DXT2 alpha metadata mismatch\n");
        gdds_buffer_release(&dds);
        return 0;
    }

    rc = gdds_decode_memory(dds.data, dds.size, &image);
    if (!expect_result("decode dxt2 synthetic", rc, GDDS_RESULT_OK)) {
        gdds_buffer_release(&dds);
        return 0;
    }
    if (image.alpha_mode != GDDS_ALPHA_MODE_PREMULTIPLIED) {
        fprintf(stderr, "legacy DXT2 decoded alpha mode mismatch\n");
        gdds_image_release(&image);
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_image_release(&image);
    gdds_buffer_release(&dds);
    return 1;
}

static int test_dx10_header_truncation_detected(const gdds_u8* rgba, gdds_u32 w, gdds_u32 h) {
    gdds_buffer dds = {0};
    gdds_encode_options options = gdds_encode_options_default(GDDS_FORMAT_RGBA8);
    gdds_result rc = gdds_encode_memory_rgba8(rgba, w, h, &options, &dds);
    if (!expect_result("encode dx10 trunc base", rc, GDDS_RESULT_OK)) return 0;

    write_u32le((gdds_u8*)dds.data + 80u, 0x00000004u);
    write_u32le((gdds_u8*)dds.data + 84u, FOURCC_DX10);
    rc = gdds_inspect_memory(dds.data, 128u, &g_scratch_info);
    if (!expect_result("inspect truncated dx10 header", rc, GDDS_RESULT_TRUNCATED)) {
        gdds_buffer_release(&dds);
        return 0;
    }

    gdds_buffer_release(&dds);
    return 1;
}

int main(void) {
    const gdds_u32 w = 11u;
    const gdds_u32 h = 7u;
    const gdds_u32 sw = 8u;
    const gdds_u32 sh = 8u;
    gdds_u8 opaque[11u * 7u * 4u];
    gdds_u8 alpha[11u * 7u * 4u];
    gdds_u8 square[8u * 8u * 4u];
    gdds_u8 tiny_dds[128] = {0};
    gdds_info info;


    make_opaque_pattern(opaque, w, h);
    make_alpha_pattern(alpha, w, h);
    make_opaque_pattern(square, sw, sh);

    if (!roundtrip_exact(opaque, w, h, GDDS_FORMAT_RGBA8)) return 1;
    if (!roundtrip_exact(opaque, w, h, GDDS_FORMAT_BGRA8)) return 1;
    if (!roundtrip_lossy(opaque, w, h, GDDS_FORMAT_DXT1, 28u)) return 1;
    if (!roundtrip_lossy(alpha, w, h, GDDS_FORMAT_DXT3, 26u)) return 1;
    if (!roundtrip_lossy(alpha, w, h, GDDS_FORMAT_DXT5, 24u)) return 1;

    memset(&info, 0, sizeof(info));
    if (!expect_result("not-dds detection", gdds_inspect_memory(tiny_dds, sizeof(tiny_dds), &info), GDDS_RESULT_NOT_DDS)) {
        return 1;
    }

    if (!test_missing_optional_flags_permissive_and_strict(opaque, w, h)) return 1;
    if (!test_noncanonical_pitch_warned(opaque, w, h)) return 1;
    if (!test_truncated_mip_chain(square, sw, sh)) return 1;
    if (!test_invalid_mask_rejected(opaque, w, h)) return 1;
    if (!test_legacy_cubemap_rejected(opaque, w, h)) return 1;
    if (!test_dx10_rgba8_decode_exact(opaque, w, h)) return 1;
    if (!test_dx10_array_rejected(opaque, w, h)) return 1;
    if (!test_dx10_reserved_bits_permissive_and_strict(opaque, w, h)) return 1;
    if (!test_dx10_alpha_mode_reported(opaque, w, h)) return 1;
    if (!test_legacy_dxt2_alpha_mode_reported(alpha, w, h)) return 1;
    if (!test_dx10_header_truncation_detected(opaque, w, h)) return 1;

    puts("ok");
    return 0;
}
