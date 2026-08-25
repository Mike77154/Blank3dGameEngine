/* SPDX-License-Identifier: CC0-1.0 */
#include "tifx.h"

#include <stdio.h>
#include <string.h>

static void write_u16le(unsigned char *p, unsigned short v)
{
    p[0] = (unsigned char)(v & 0xFFU);
    p[1] = (unsigned char)((v >> 8) & 0xFFU);
}

static void write_u32le(unsigned char *p, unsigned long v)
{
    p[0] = (unsigned char)(v & 0xFFUL);
    p[1] = (unsigned char)((v >> 8) & 0xFFUL);
    p[2] = (unsigned char)((v >> 16) & 0xFFUL);
    p[3] = (unsigned char)((v >> 24) & 0xFFUL);
}

static unsigned short read_u16le_test(const unsigned char *p)
{
    return (unsigned short)((unsigned short)p[0] | ((unsigned short)p[1] << 8));
}

static unsigned long read_u32le_test(const unsigned char *p)
{
    return ((unsigned long)p[0]) |
           ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24);
}

#if ULONG_MAX > 0xFFFFFFFFUL
static unsigned long read_u64le_test(const unsigned char *p)
{
    return ((unsigned long)p[0]) |
           ((unsigned long)p[1] << 8) |
           ((unsigned long)p[2] << 16) |
           ((unsigned long)p[3] << 24) |
           ((unsigned long)p[4] << 32) |
           ((unsigned long)p[5] << 40) |
           ((unsigned long)p[6] << 48) |
           ((unsigned long)p[7] << 56);
}
#endif

static unsigned short get_packed_bit_test(const unsigned char *src, unsigned long x)
{
    return (unsigned short)((src[x >> 3] >> (7U - (unsigned int)(x & 7UL))) & 1U);
}

static void set_packed_bit_test(unsigned char *dst, unsigned long x, unsigned short bit)
{
    unsigned char mask;

    mask = (unsigned char)(0x80U >> (unsigned int)(x & 7UL));
    if (bit != 0U) {
        dst[x >> 3] = (unsigned char)(dst[x >> 3] | mask);
    } else {
        dst[x >> 3] = (unsigned char)(dst[x >> 3] & (unsigned char)(~mask));
    }
}

static void fill_gray_pattern(unsigned char *dst,
                              unsigned long width,
                              unsigned long height,
                              unsigned long stride,
                              unsigned long seed)
{
    unsigned long x;
    unsigned long y;

    for (y = 0UL; y < height; ++y) {
        for (x = 0UL; x < width; ++x) {
            dst[y * stride + x] = (unsigned char)((x * 5UL + y * 7UL + seed) & 255UL);
        }
    }
}

static void fill_rgb_pattern(unsigned char *dst,
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
            dst[idx + 0UL] = (unsigned char)((x * 13UL + y * 3UL + seed) & 255UL);
            dst[idx + 1UL] = (unsigned char)((x * 5UL + y * 11UL + seed * 3UL) & 255UL);
            dst[idx + 2UL] = (unsigned char)((x * 17UL + y * 7UL + seed * 5UL) & 255UL);
        }
    }
}

static void fill_rgb_mixed_pattern(unsigned char *dst,
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
            unsigned long band_x;
            unsigned long band_y;

            idx = y * stride + x * 3UL;
            band_x = x >> 3;
            band_y = y >> 3;
            dst[idx + 0UL] = (unsigned char)(((band_x * 19UL) + (seed * 3UL)) & 255UL);
            dst[idx + 1UL] = (unsigned char)((((band_y * 23UL) + (band_x * 7UL)) + seed) & 255UL);
            dst[idx + 2UL] = (unsigned char)(((((band_x ^ band_y) * 29UL) + seed * 5UL)) & 255UL);
            if ((y & 7UL) >= 4UL) {
                dst[idx + 0UL] = dst[((y - 4UL) * stride) + x * 3UL + 0UL];
                dst[idx + 1UL] = dst[((y - 4UL) * stride) + x * 3UL + 1UL];
                dst[idx + 2UL] = dst[((y - 4UL) * stride) + x * 3UL + 2UL];
            }
        }
    }
}

static void fill_rgba_pattern(unsigned char *dst,
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

            idx = y * stride + x * 4UL;
            dst[idx + 0UL] = (unsigned char)((x * 9UL + y * 5UL + seed) & 255UL);
            dst[idx + 1UL] = (unsigned char)((x * 7UL + y * 13UL + seed * 3UL) & 255UL);
            dst[idx + 2UL] = (unsigned char)((x * 19UL + y * 3UL + seed * 5UL) & 255UL);
            dst[idx + 3UL] = (unsigned char)((x * 11UL + y * 17UL + seed * 7UL) & 255UL);
        }
    }
}

static int find_classic_tag_value(const unsigned char *buf,
                                  unsigned long size,
                                  unsigned long ifd_offset,
                                  unsigned short wanted_tag,
                                  unsigned long *out_count,
                                  unsigned long *out_value,
                                  unsigned long *out_next_ifd)
{
    unsigned long tag_count;
    unsigned long entry_offset;
    unsigned long i;

    if (buf == 0 || out_count == 0 || out_value == 0 || out_next_ifd == 0) {
        return 0;
    }
    if (ifd_offset > size || size - ifd_offset < 2UL) {
        return 0;
    }

    tag_count = (unsigned long)read_u16le_test(buf + ifd_offset);
    entry_offset = ifd_offset + 2UL;
    if (entry_offset > size || (tag_count * 12UL) > size - entry_offset ||
        4UL > size - (entry_offset + (tag_count * 12UL))) {
        return 0;
    }

    *out_next_ifd = read_u32le_test(buf + entry_offset + (tag_count * 12UL));
    for (i = 0UL; i < tag_count; ++i) {
        const unsigned char *entry;
        unsigned short tag;

        entry = buf + entry_offset + (i * 12UL);
        tag = read_u16le_test(entry);
        if (tag == wanted_tag) {
            *out_count = read_u32le_test(entry + 4UL);
            *out_value = read_u32le_test(entry + 8UL);
            return 1;
        }
    }
    return 0;
}

static int patch_classic_tag_value(unsigned char *buf,
                                   unsigned long size,
                                   unsigned long ifd_offset,
                                   unsigned short wanted_tag,
                                   unsigned long new_value)
{
    unsigned long tag_count;
    unsigned long entry_offset;
    unsigned long i;

    if (buf == 0) {
        return 0;
    }
    if (ifd_offset > size || size - ifd_offset < 2UL) {
        return 0;
    }

    tag_count = (unsigned long)read_u16le_test(buf + ifd_offset);
    entry_offset = ifd_offset + 2UL;
    if (entry_offset > size || (tag_count * 12UL) > size - entry_offset ||
        4UL > size - (entry_offset + (tag_count * 12UL))) {
        return 0;
    }

    for (i = 0UL; i < tag_count; ++i) {
        unsigned char *entry;
        unsigned short tag;

        entry = buf + entry_offset + (i * 12UL);
        tag = read_u16le_test(entry);
        if (tag == wanted_tag) {
            write_u32le(entry + 8UL, new_value);
            return 1;
        }
    }
    return 0;
}

#if ULONG_MAX > 0xFFFFFFFFUL
static int find_bigtiff_tag_value(const unsigned char *buf,
                                  unsigned long size,
                                  unsigned long ifd_offset,
                                  unsigned short wanted_tag,
                                  unsigned long *out_count,
                                  unsigned long *out_value,
                                  unsigned long *out_next_ifd)
{
    unsigned long tag_count;
    unsigned long entry_offset;
    unsigned long i;

    if (buf == 0 || out_count == 0 || out_value == 0 || out_next_ifd == 0) {
        return 0;
    }
    if (ifd_offset > size || size - ifd_offset < 8UL) {
        return 0;
    }

    tag_count = read_u64le_test(buf + ifd_offset);
    entry_offset = ifd_offset + 8UL;
    if (entry_offset > size || (tag_count * 20UL) > size - entry_offset ||
        8UL > size - (entry_offset + (tag_count * 20UL))) {
        return 0;
    }

    *out_next_ifd = read_u64le_test(buf + entry_offset + (tag_count * 20UL));
    for (i = 0UL; i < tag_count; ++i) {
        const unsigned char *entry;
        unsigned short tag;

        entry = buf + entry_offset + (i * 20UL);
        tag = read_u16le_test(entry);
        if (tag == wanted_tag) {
            *out_count = read_u64le_test(entry + 4UL);
            *out_value = read_u64le_test(entry + 12UL);
            return 1;
        }
    }
    return 0;
}
#endif

static int check_equal(const unsigned char *a,
                       const unsigned char *b,
                       unsigned long size,
                       const char *label)
{
    unsigned long i;
    for (i = 0UL; i < size; ++i) {
        if (a[i] != b[i]) {
            printf("FAIL: %s mismatch at %lu: %u != %u\n",
                   label,
                   i,
                   (unsigned int)a[i],
                   (unsigned int)b[i]);
            return 0;
        }
    }
    return 1;
}

static int test_gray_roundtrip(void)
{
    static const unsigned char src_pixels[6] = { 0U, 64U, 128U, 255U, 7U, 99U };
    unsigned char file_buf[512];
    unsigned char decode_buf[6];
    unsigned char workspace[16];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    int rc;

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_GRAY8;
    params.width = 3UL;
    params.height = 2UL;
    params.stride = 3UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: gray write %s\n", tifx_strerror(rc));
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: gray parse %s\n", tifx_strerror(rc));
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 3UL) {
        printf("FAIL: gray decode buffer size wrong\n");
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: gray decode %s\n", tifx_strerror(rc));
        return 0;
    }

    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "gray roundtrip");
}

static int test_rgb_roundtrip(void)
{
    static const unsigned char src_pixels[12] = {
        255U, 0U, 0U,
        0U, 255U, 0U,
        0U, 0U, 255U,
        12U, 34U, 56U
    };
    unsigned char file_buf[512];
    unsigned char decode_buf[12];
    unsigned char workspace[32];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    int rc;

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_RGB24;
    params.width = 2UL;
    params.height = 2UL;
    params.stride = 6UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: rgb write %s\n", tifx_strerror(rc));
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: rgb parse %s\n", tifx_strerror(rc));
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 6UL) {
        printf("FAIL: rgb decode buffer size wrong\n");
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: rgb decode %s\n", tifx_strerror(rc));
        return 0;
    }

    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "rgb roundtrip");
}


static int test_rgba_roundtrip(void)
{
    static const unsigned char src_pixels[8] = {
        10U, 20U, 30U, 40U,
        200U, 150U, 100U, 50U
    };
    unsigned char file_buf[1024];
    unsigned char decode_buf[8];
    unsigned char workspace[32];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long tag_count;
    unsigned long tag_value;
    unsigned long next_ifd;
    int rc;

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_RGBA32;
    params.alpha_mode = TIFX_ALPHA_ASSOCIATED;
    params.width = 2UL;
    params.height = 1UL;
    params.stride = 8UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: rgba write %s\n", tifx_strerror(rc));
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: rgba parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.pixel_format != TIFX_PIXEL_RGBA32 || info.alpha_mode != TIFX_ALPHA_ASSOCIATED ||
        info.samples_per_pixel != 4U || info.extra_samples_count != 1U) {
        printf("FAIL: rgba metadata wrong\n");
        return 0;
    }
    if (!find_classic_tag_value(file_buf, written, info.current_ifd_offset, 338U,
                                &tag_count, &tag_value, &next_ifd) ||
        tag_count != 1UL || tag_value != 1UL) {
        printf("FAIL: rgba extra samples tag wrong\n");
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 8UL) {
        printf("FAIL: rgba decode buffer size wrong\n");
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: rgba decode %s\n", tifx_strerror(rc));
        return 0;
    }

    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "rgba roundtrip");
}

static unsigned long make_packbits_gray_tiff(unsigned char *buf)
{
    static const unsigned char packed_rows[10] = {
        3U, 1U, 2U, 3U, 4U,
        3U, 5U, 6U, 7U, 8U
    };
    unsigned long ifd_offset;
    unsigned long xres_offset;
    unsigned long yres_offset;
    unsigned short tags;
    unsigned short i;

    memset(buf, 0, 256U);
    buf[0] = 'I';
    buf[1] = 'I';
    write_u16le(buf + 2, 42U);
    ifd_offset = 18UL;
    write_u32le(buf + 4, ifd_offset);
    memcpy(buf + 8, packed_rows, sizeof(packed_rows));

    tags = 12U;
    write_u16le(buf + ifd_offset, tags);
    xres_offset = ifd_offset + 2UL + (unsigned long)tags * 12UL + 4UL;
    yres_offset = xres_offset + 8UL;
    i = 0U;
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 256U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 4UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 257U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 2UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 258U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 8U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 259U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 32773U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 262U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 1U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 273U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 8UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 274U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 1U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 278U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 2UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 279U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, sizeof(packed_rows));

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 282U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 5U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, xres_offset);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 283U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 5U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, yres_offset);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 296U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 2U);

    write_u32le(buf + ifd_offset + 2UL + (unsigned long)tags * 12UL, 0UL);
    write_u32le(buf + xres_offset + 0UL, 72UL);
    write_u32le(buf + xres_offset + 4UL, 1UL);
    write_u32le(buf + yres_offset + 0UL, 72UL);
    write_u32le(buf + yres_offset + 4UL, 1UL);

    return yres_offset + 8UL;
}

static int test_packbits_gray(void)
{
    static const unsigned char expected[8] = { 1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U };
    unsigned char file_buf[256];
    unsigned char decode_buf[8];
    unsigned char workspace[16];
    unsigned long file_size;
    unsigned long stride;
    tifx_image_info info;
    int rc;

    file_size = make_packbits_gray_tiff(file_buf);
    rc = tifx_parse_memory(&info, file_buf, file_size);
    if (rc != TIFX_OK) {
        printf("FAIL: packbits parse %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_decode_memory(&info,
                            file_buf,
                            file_size,
                            decode_buf,
                            sizeof(decode_buf),
                            4UL,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: packbits decode %s\n", tifx_strerror(rc));
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 4UL) {
        printf("FAIL: packbits size wrong\n");
        return 0;
    }
    return check_equal(expected, decode_buf, sizeof(expected), "packbits gray");
}

static unsigned long make_palette4_tiff(unsigned char *buf)
{
    unsigned long image_size;
    unsigned long ifd_offset;
    unsigned long xres_offset;
    unsigned long yres_offset;
    unsigned long cmap_offset;
    unsigned short tags;
    unsigned short i;
    unsigned short idx;

    memset(buf, 0, 512U);
    buf[0] = 'I';
    buf[1] = 'I';
    write_u16le(buf + 2, 42U);
    image_size = 1UL;
    ifd_offset = 10UL;
    write_u32le(buf + 4, ifd_offset);
    buf[8] = 0x01U;

    tags = 13U;
    write_u16le(buf + ifd_offset, tags);
    xres_offset = ifd_offset + 2UL + (unsigned long)tags * 12UL + 4UL;
    yres_offset = xres_offset + 8UL;
    cmap_offset = yres_offset + 8UL;
    i = 0U;

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 256U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 2UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 257U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 1UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 258U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 4U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 259U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 1U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 262U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 3U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 273U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 8UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 274U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 1U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 278U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 1UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 279U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, image_size);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 282U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 5U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, xres_offset);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 283U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 5U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, yres_offset);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 296U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 2U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 320U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 48UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, cmap_offset);

    write_u32le(buf + ifd_offset + 2UL + (unsigned long)tags * 12UL, 0UL);
    write_u32le(buf + xres_offset + 0UL, 72UL);
    write_u32le(buf + xres_offset + 4UL, 1UL);
    write_u32le(buf + yres_offset + 0UL, 72UL);
    write_u32le(buf + yres_offset + 4UL, 1UL);

    for (idx = 0U; idx < 16U; ++idx) {
        write_u16le(buf + cmap_offset + (unsigned long)idx * 2UL, (idx == 1U) ? 65535U : 0U);
        write_u16le(buf + cmap_offset + 32UL + (unsigned long)idx * 2UL, 0U);
        write_u16le(buf + cmap_offset + 64UL + (unsigned long)idx * 2UL, 0U);
    }

    return cmap_offset + 96UL;
}

static int test_palette4(void)
{
    static const unsigned char expected[6] = { 0U, 0U, 0U, 255U, 0U, 0U };
    unsigned char file_buf[512];
    unsigned char decode_buf[6];
    unsigned char workspace[16];
    unsigned long file_size;
    tifx_image_info info;
    int rc;

    file_size = make_palette4_tiff(file_buf);
    rc = tifx_parse_memory(&info, file_buf, file_size);
    if (rc != TIFX_OK) {
        printf("FAIL: palette parse %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_decode_memory(&info,
                            file_buf,
                            file_size,
                            decode_buf,
                            sizeof(decode_buf),
                            6UL,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: palette decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(expected, decode_buf, sizeof(expected), "palette 4-bit");
}


static unsigned long make_mh_bilevel_tiff(unsigned char *buf,
                                          unsigned short fill_order)
{
    static const unsigned char mh_rows_fill1[6] = {
        0xDFU, 0x04U, 0x35U, 0x03U, 0xC8U, 0xE0U
    };
    static const unsigned char mh_rows_fill2[6] = {
        0xFBU, 0x20U, 0xACU, 0xC0U, 0x13U, 0x07U
    };
    const unsigned char *compressed_rows;
    unsigned long ifd_offset;
    unsigned long xres_offset;
    unsigned long yres_offset;
    unsigned short tags;
    unsigned short i;

    compressed_rows = (fill_order == 2U) ? mh_rows_fill2 : mh_rows_fill1;

    memset(buf, 0, 256U);
    buf[0] = 'I';
    buf[1] = 'I';
    write_u16le(buf + 2, 42U);
    ifd_offset = 14UL;
    write_u32le(buf + 4, ifd_offset);
    memcpy(buf + 8, compressed_rows, 6U);

    tags = 13U;
    write_u16le(buf + ifd_offset, tags);
    xres_offset = ifd_offset + 2UL + (unsigned long)tags * 12UL + 4UL;
    yres_offset = xres_offset + 8UL;
    i = 0U;

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 256U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 80UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 257U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 2UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 258U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 1U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 259U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 2U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 262U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 0U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 266U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, fill_order);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 273U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 8UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 274U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 1U);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 278U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 2UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 279U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 4U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 6UL);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 282U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 5U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, xres_offset);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 283U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 5U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, yres_offset);

    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i++) * 12UL + 0UL, 296U);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 2UL, 3U);
    write_u32le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 4UL, 1UL);
    write_u16le(buf + ifd_offset + 2UL + (unsigned long)(i - 1U) * 12UL + 8UL, 2U);

    write_u32le(buf + ifd_offset + 2UL + (unsigned long)tags * 12UL, 0UL);
    write_u32le(buf + xres_offset + 0UL, 72UL);
    write_u32le(buf + xres_offset + 4UL, 1UL);
    write_u32le(buf + yres_offset + 0UL, 72UL);
    write_u32le(buf + yres_offset + 4UL, 1UL);

    return yres_offset + 8UL;
}

static void fill_mh_expected(unsigned char *expected)
{
    memset(expected, 255, 160U);
    memset(expected + 70UL, 0, 10U);
    memset(expected + 80UL, 0, 70U);
}

static int test_mh_bilevel(unsigned short fill_order, const char *label)
{
    unsigned char file_buf[256];
    unsigned char decode_buf[160];
    unsigned char expected[160];
    unsigned char workspace[16];
    unsigned long file_size;
    unsigned long stride;
    tifx_image_info info;
    int rc;

    fill_mh_expected(expected);
    file_size = make_mh_bilevel_tiff(file_buf, fill_order);

    rc = tifx_parse_memory(&info, file_buf, file_size);
    if (rc != TIFX_OK) {
        printf("FAIL: %s parse %s\n", label, tifx_strerror(rc));
        return 0;
    }

    if (tifx_decode_workspace_size(&info) != 10UL) {
        printf("FAIL: %s workspace size wrong\n", label);
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 80UL) {
        printf("FAIL: %s decode buffer size wrong\n", label);
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            file_size,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: %s decode %s\n", label, tifx_strerror(rc));
        return 0;
    }

    return check_equal(expected, decode_buf, sizeof(expected), label);
}


static int read_file(const char *path,
                     unsigned char *buf,
                     unsigned long buf_capacity,
                     unsigned long *out_size)
{
    FILE *fp;
    unsigned long size;

    fp = fopen(path, "rb");
    if (fp == 0) {
        printf("FAIL: could not open %s\n", path);
        return 0;
    }
    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        printf("FAIL: could not seek %s\n", path);
        return 0;
    }
    size = (unsigned long)ftell(fp);
    if (size > buf_capacity) {
        fclose(fp);
        printf("FAIL: %s too large for test buffer\n", path);
        return 0;
    }
    if (fseek(fp, 0L, SEEK_SET) != 0) {
        fclose(fp);
        printf("FAIL: could not rewind %s\n", path);
        return 0;
    }
    if (fread(buf, 1U, (size_t)size, fp) != (size_t)size) {
        fclose(fp);
        printf("FAIL: could not read %s\n", path);
        return 0;
    }
    fclose(fp);
    *out_size = size;
    return 1;
}

static void fill_ccitt_expected(unsigned char *expected)
{
    static const unsigned char rows[6][2] = {
        { 0x00U, 0x00U },
        { 0x0FU, 0x0FU },
        { 0x0FU, 0x0FU },
        { 0x1EU, 0x1EU },
        { 0xF0U, 0xF0U },
        { 0x03U, 0xF0U }
    };
    unsigned long y;
    unsigned long x;

    for (y = 0UL; y < 6UL; ++y) {
        for (x = 0UL; x < 16UL; ++x) {
            unsigned char packed;
            unsigned char bit;
            packed = rows[y][x >> 3];
            bit = (unsigned char)((packed >> (7U - (unsigned int)(x & 7UL))) & 1U);
            expected[y * 16UL + x] = bit ? 0U : 255U;
        }
    }
}

static int test_ccitt_fixture(const char *path,
                              unsigned short expected_compression,
                              unsigned long expected_t4_options,
                              unsigned long expected_t6_options,
                              unsigned short expected_fill_order,
                              const char *label)
{
    unsigned char file_buf[512];
    unsigned char decode_buf[96];
    unsigned char expected[96];
    unsigned char workspace[16];
    unsigned long file_size;
    unsigned long stride;
    tifx_image_info info;
    int rc;

    if (!read_file(path, file_buf, sizeof(file_buf), &file_size)) {
        return 0;
    }
    fill_ccitt_expected(expected);

    rc = tifx_parse_memory(&info, file_buf, file_size);
    if (rc != TIFX_OK) {
        printf("FAIL: %s parse %s\n", label, tifx_strerror(rc));
        return 0;
    }
    if (info.compression != expected_compression ||
        info.t4_options != expected_t4_options ||
        info.t6_options != expected_t6_options ||
        info.fill_order != expected_fill_order) {
        printf("FAIL: %s metadata mismatch\n", label);
        return 0;
    }
    if (tifx_decode_workspace_size(&info) != 4UL) {
        printf("FAIL: %s workspace size wrong\n", label);
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 16UL) {
        printf("FAIL: %s decode buffer size wrong\n", label);
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            file_size,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: %s decode %s\n", label, tifx_strerror(rc));
        return 0;
    }

    return check_equal(expected, decode_buf, sizeof(expected), label);
}


static int test_small_bilevel_fixture(const char *path,
                                      unsigned long expected_width,
                                      unsigned long expected_height,
                                      unsigned short expected_compression,
                                      unsigned long expected_t4_options,
                                      unsigned long expected_t6_options,
                                      unsigned short expected_fill_order,
                                      const unsigned char *expected_pixels,
                                      unsigned long expected_size,
                                      const char *label)
{
    unsigned char file_buf[512];
    unsigned char decode_buf[128];
    unsigned char workspace[32];
    unsigned long file_size;
    unsigned long stride;
    unsigned long decode_size;
    tifx_image_info info;
    int rc;

    if (!read_file(path, file_buf, sizeof(file_buf), &file_size)) {
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, file_size);
    if (rc != TIFX_OK) {
        printf("FAIL: %s parse %s\n", label, tifx_strerror(rc));
        return 0;
    }
    if (info.width != expected_width ||
        info.height != expected_height ||
        info.compression != expected_compression ||
        info.t4_options != expected_t4_options ||
        info.t6_options != expected_t6_options ||
        info.fill_order != expected_fill_order) {
        printf("FAIL: %s metadata mismatch\n", label);
        return 0;
    }
    if (tifx_decode_workspace_size(&info) > sizeof(workspace)) {
        printf("FAIL: %s workspace size too large for test buffer\n", label);
        return 0;
    }

    stride = 0UL;
    decode_size = tifx_decode_buffer_size(&info, &stride);
    if (decode_size != expected_size || stride != expected_width) {
        printf("FAIL: %s decode buffer size wrong\n", label);
        return 0;
    }
    if (decode_size > sizeof(decode_buf)) {
        printf("FAIL: %s decode buffer too small\n", label);
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            file_size,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: %s decode %s\n", label, tifx_strerror(rc));
        return 0;
    }

    return check_equal(expected_pixels, decode_buf, expected_size, label);
}

static void expand_packed_bilevel_expected(const unsigned char *src,
                                           unsigned long width,
                                           unsigned long height,
                                           unsigned long stride,
                                           unsigned short photometric,
                                           unsigned char *dst)
{
    unsigned long y;
    unsigned long x;

    for (y = 0UL; y < height; ++y) {
        for (x = 0UL; x < width; ++x) {
            unsigned char packed;
            unsigned char bit;

            packed = src[y * stride + (x >> 3)];
            bit = (unsigned char)((packed >> (7U - (unsigned int)(x & 7UL))) & 1U);
            if (photometric == 0U) {
                dst[y * width + x] = bit ? 0U : 255U;
            } else {
                dst[y * width + x] = bit ? 255U : 0U;
            }
        }
    }
}

static int test_write_bilevel_ccitt_roundtrip(unsigned short compression,
                                             unsigned long t4_options,
                                             unsigned long rows_per_strip,
                                             unsigned short fill_order,
                                             const char *label)
{
    static const unsigned char src_pixels[10] = {
        0x00U, 0x00U,
        0xFFU, 0xF8U,
        0xAAU, 0xA8U,
        0x1EU, 0x38U,
        0xC1U, 0xF0U
    };
    unsigned char file_buf[4096];
    unsigned char decode_buf[128];
    unsigned char expected[128];
    unsigned char workspace[32];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long expected_written;
    unsigned long stride;
    unsigned long expected_strip_count;
    unsigned long i;
    int rc;

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_BILEVEL;
    params.compression = compression;
    params.photometric = 0U;
    params.width = 13UL;
    params.height = 5UL;
    params.stride = 2UL;
    params.rows_per_strip = rows_per_strip;
    params.t4_options = t4_options;
    params.fill_order = fill_order;
    params.pixels = src_pixels;

    expected_written = tifx_write_buffer_size(&params);
    if (expected_written == 0UL) {
        printf("FAIL: %s write buffer size is zero\n", label);
        return 0;
    }

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: %s write %s\n", label, tifx_strerror(rc));
        return 0;
    }
    if (written != expected_written) {
        printf("FAIL: %s write size mismatch\n", label);
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: %s parse %s\n", label, tifx_strerror(rc));
        return 0;
    }

    expected_strip_count = (13UL + 1UL); /* quiet old compilers about constant folding */
    expected_strip_count = (params.height + rows_per_strip - 1UL) / rows_per_strip;
    if (info.width != 13UL || info.height != 5UL ||
        info.compression != compression ||
        info.photometric != 0U ||
        info.bits_per_sample[0] != 1U ||
        info.fill_order != fill_order ||
        info.rows_per_strip != rows_per_strip ||
        info.strip_count != expected_strip_count) {
        printf("FAIL: %s metadata mismatch\n", label);
        return 0;
    }
    if (compression == 3U && info.t4_options != t4_options) {
        printf("FAIL: %s T4Options mismatch\n", label);
        return 0;
    }
    if (compression == 4U && info.t6_options != 0UL) {
        printf("FAIL: %s T6Options mismatch\n", label);
        return 0;
    }

    for (i = 0UL; i < info.strip_count; ++i) {
        if (info.strip_byte_counts[i] == 0UL) {
            printf("FAIL: %s empty strip\n", label);
            return 0;
        }
        if (i > 0UL && info.strip_offsets[i] <= info.strip_offsets[i - 1UL]) {
            printf("FAIL: %s strip offsets not increasing\n", label);
            return 0;
        }
    }

    expand_packed_bilevel_expected(src_pixels,
                                   13UL,
                                   5UL,
                                   2UL,
                                   0U,
                                   expected);

    if (tifx_decode_workspace_size(&info) > sizeof(workspace)) {
        printf("FAIL: %s workspace too small for test buffer\n", label);
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != 65UL || stride != 13UL) {
        printf("FAIL: %s decode buffer size wrong\n", label);
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: %s decode %s\n", label, tifx_strerror(rc));
        return 0;
    }

    return check_equal(expected, decode_buf, 65UL, label);
}

static int test_write_bilevel_t6_roundtrip(void)
{
    static const unsigned char src_pixels[6] = {
        0x80U, 0x01U,
        0xF0U, 0x0FU,
        0x18U, 0xC0U
    };
    unsigned char file_buf[1024];
    unsigned char decode_buf[48];
    unsigned char expected[48];
    unsigned char workspace[16];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long expected_written;
    unsigned long stride;
    int rc;

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_BILEVEL;
    params.compression = 4U;
    params.photometric = 0U;
    params.width = 16UL;
    params.height = 3UL;
    params.stride = 2UL;
    params.pixels = src_pixels;

    expected_written = tifx_write_buffer_size(&params);
    if (expected_written == 0UL) {
        printf("FAIL: bilevel t6 write buffer size is zero\n");
        return 0;
    }

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bilevel t6 write %s\n", tifx_strerror(rc));
        return 0;
    }
    if (written != expected_written) {
        printf("FAIL: bilevel t6 write size mismatch\n");
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bilevel t6 parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.width != 16UL || info.height != 3UL || info.compression != 4U ||
        info.photometric != 0U || info.bits_per_sample[0] != 1U ||
        info.t6_options != 0UL || info.fill_order != 1U) {
        printf("FAIL: bilevel t6 metadata mismatch\n");
        return 0;
    }

    expand_packed_bilevel_expected(src_pixels,
                                   16UL,
                                   3UL,
                                   2UL,
                                   0U,
                                   expected);

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 16UL) {
        printf("FAIL: bilevel t6 decode buffer size wrong\n");
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bilevel t6 decode %s\n", tifx_strerror(rc));
        return 0;
    }

    return check_equal(expected, decode_buf, sizeof(expected), "bilevel t6 roundtrip");
}


static int test_bigtiff_gray_roundtrip(void)
{
    static const unsigned char src_pixels[6] = { 0U, 64U, 128U, 255U, 7U, 99U };
    unsigned char file_buf[1024];
    unsigned char decode_buf[6];
    unsigned char workspace[16];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    int rc;

    tifx_write_params_init(&params);
    params.container_format = TIFX_CONTAINER_BIGTIFF;
    params.pixel_format = TIFX_PIXEL_GRAY8;
    params.width = 3UL;
    params.height = 2UL;
    params.stride = 3UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff gray write %s\n", tifx_strerror(rc));
        return 0;
    }
    if (written < 16UL || file_buf[0] != 'I' || file_buf[1] != 'I' || file_buf[2] != 43U || file_buf[3] != 0U) {
        printf("FAIL: bigtiff gray header wrong\n");
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff gray parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.container_format != TIFX_CONTAINER_BIGTIFF) {
        printf("FAIL: bigtiff gray parse did not detect BigTIFF\n");
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 3UL) {
        printf("FAIL: bigtiff gray decode buffer size wrong\n");
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff gray decode %s\n", tifx_strerror(rc));
        return 0;
    }

    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "bigtiff gray roundtrip");
}

static int test_bigtiff_rgb_roundtrip(void)
{
    static const unsigned char src_pixels[12] = {
        255U, 0U, 0U,
        0U, 255U, 0U,
        0U, 0U, 255U,
        12U, 34U, 56U
    };
    unsigned char file_buf[2048];
    unsigned char decode_buf[12];
    unsigned char workspace[32];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    int rc;

    tifx_write_params_init(&params);
    params.container_format = TIFX_CONTAINER_BIGTIFF;
    params.pixel_format = TIFX_PIXEL_RGB24;
    params.width = 2UL;
    params.height = 2UL;
    params.stride = 6UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff rgb write %s\n", tifx_strerror(rc));
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff rgb parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.container_format != TIFX_CONTAINER_BIGTIFF) {
        printf("FAIL: bigtiff rgb parse did not detect BigTIFF\n");
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 6UL) {
        printf("FAIL: bigtiff rgb decode buffer size wrong\n");
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff rgb decode %s\n", tifx_strerror(rc));
        return 0;
    }

    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "bigtiff rgb roundtrip");
}


static int test_bigtiff_rgba_roundtrip(void)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    return 1;
#else
    static const unsigned char src_pixels[8] = {
        1U, 2U, 3U, 4U,
        250U, 200U, 150U, 100U
    };
    unsigned char file_buf[4096];
    unsigned char decode_buf[8];
    unsigned char workspace[32];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long tag_count;
    unsigned long tag_value;
    unsigned long next_ifd;
    int rc;

    tifx_write_params_init(&params);
    params.container_format = TIFX_CONTAINER_BIGTIFF;
    params.pixel_format = TIFX_PIXEL_RGBA32;
    params.alpha_mode = TIFX_ALPHA_UNASSOCIATED;
    params.width = 2UL;
    params.height = 1UL;
    params.stride = 8UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff rgba write %s\n", tifx_strerror(rc));
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff rgba parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.container_format != TIFX_CONTAINER_BIGTIFF ||
        info.pixel_format != TIFX_PIXEL_RGBA32 ||
        info.alpha_mode != TIFX_ALPHA_UNASSOCIATED ||
        info.samples_per_pixel != 4U || info.extra_samples_count != 1U) {
        printf("FAIL: bigtiff rgba metadata wrong\n");
        return 0;
    }
    if (!find_bigtiff_tag_value(file_buf, written, info.current_ifd_offset, 338U,
                                &tag_count, &tag_value, &next_ifd) ||
        tag_count != 1UL || tag_value != 2UL) {
        printf("FAIL: bigtiff rgba extra samples tag wrong\n");
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 8UL) {
        printf("FAIL: bigtiff rgba decode buffer size wrong\n");
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff rgba decode %s\n", tifx_strerror(rc));
        return 0;
    }

    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "bigtiff rgba roundtrip");
#endif
}

static int test_bigtiff_t6_roundtrip(void)
{
    static const unsigned char src_bits[4] = {
        0xAAU, 0x55U,
        0xF0U, 0x0FU
    };
    static const unsigned char expected[32] = {
        0U, 255U, 0U, 255U, 0U, 255U, 0U, 255U,
        255U, 0U, 255U, 0U, 255U, 0U, 255U, 0U,
        0U, 0U, 0U, 0U, 255U, 255U, 255U, 255U,
        255U, 255U, 255U, 255U, 0U, 0U, 0U, 0U
    };
    unsigned char file_buf[4096];
    unsigned char decode_buf[32];
    unsigned char workspace[64];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    int rc;

    tifx_write_params_init(&params);
    params.container_format = TIFX_CONTAINER_BIGTIFF;
    params.pixel_format = TIFX_PIXEL_BILEVEL;
    params.compression = 4U;
    params.photometric = 0U;
    params.width = 8UL;
    params.height = 4UL;
    params.stride = 1UL;
    params.rows_per_strip = 2UL;
    params.pixels = src_bits;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff t6 write %s\n", tifx_strerror(rc));
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff t6 parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.container_format != TIFX_CONTAINER_BIGTIFF || info.compression != 4U || info.strip_count != 2UL) {
        printf("FAIL: bigtiff t6 metadata wrong\n");
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 8UL) {
        printf("FAIL: bigtiff t6 decode buffer size wrong\n");
        return 0;
    }

    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff t6 decode %s\n", tifx_strerror(rc));
        return 0;
    }

    return check_equal(expected, decode_buf, sizeof(expected), "bigtiff t6 roundtrip");
}


static int test_bigtiff_multipage_roundtrip(void)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    return 1;
#else
    static const unsigned char gray_pixels[6] = { 0U, 64U, 128U, 255U, 7U, 99U };
    static const unsigned char rgb_pixels[12] = {
        255U, 0U, 0U,
        0U, 255U, 0U,
        0U, 0U, 255U,
        12U, 34U, 56U
    };
    static const unsigned char bilevel_bits[2] = { 0xAAU, 0x0FU };
    static const unsigned char bilevel_expected[16] = {
        0U, 255U, 0U, 255U, 0U, 255U, 0U, 255U,
        255U, 255U, 255U, 255U, 0U, 0U, 0U, 0U
    };
    unsigned char file_buf[16384];
    unsigned char decode_buf[32];
    unsigned char workspace[128];
    tifx_write_params pages[3];
    tifx_image_info info;
    unsigned long written;
    unsigned long page_count;
    unsigned long stride;
    int rc;

    tifx_write_params_init(&pages[0]);
    pages[0].container_format = TIFX_CONTAINER_BIGTIFF;
    pages[0].pixel_format = TIFX_PIXEL_GRAY8;
    pages[0].width = 3UL;
    pages[0].height = 2UL;
    pages[0].stride = 3UL;
    pages[0].pixels = gray_pixels;

    tifx_write_params_init(&pages[1]);
    pages[1].container_format = TIFX_CONTAINER_BIGTIFF;
    pages[1].pixel_format = TIFX_PIXEL_RGB24;
    pages[1].width = 2UL;
    pages[1].height = 2UL;
    pages[1].stride = 6UL;
    pages[1].pixels = rgb_pixels;

    tifx_write_params_init(&pages[2]);
    pages[2].container_format = TIFX_CONTAINER_BIGTIFF;
    pages[2].pixel_format = TIFX_PIXEL_BILEVEL;
    pages[2].compression = 4U;
    pages[2].photometric = 0U;
    pages[2].width = 8UL;
    pages[2].height = 2UL;
    pages[2].stride = 1UL;
    pages[2].rows_per_strip = 1UL;
    pages[2].pixels = bilevel_bits;

    rc = tifx_write_bigtiff_pages_memory(file_buf,
                                         sizeof(file_buf),
                                         pages,
                                         3UL,
                                         &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff multipage write %s\n", tifx_strerror(rc));
        return 0;
    }
    if (written < 16UL || file_buf[0] != 'I' || file_buf[1] != 'I' || file_buf[2] != 43U || file_buf[3] != 0U) {
        printf("FAIL: bigtiff multipage header wrong\n");
        return 0;
    }

    page_count = 0UL;
    rc = tifx_bigtiff_page_count_memory(file_buf, written, &page_count);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff multipage count %s\n", tifx_strerror(rc));
        return 0;
    }
    if (page_count != 3UL) {
        printf("FAIL: bigtiff multipage count wrong\n");
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff multipage parse page0 %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.container_format != TIFX_CONTAINER_BIGTIFF ||
        info.page_count != 3UL ||
        info.page_index != 0UL ||
        info.new_subfile_type != 2UL ||
        info.page_number[0] != 0U ||
        info.page_number[1] != 3U ||
        info.next_ifd_offset == 0UL) {
        printf("FAIL: bigtiff multipage page0 metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(gray_pixels) || stride != 3UL) {
        printf("FAIL: bigtiff multipage page0 buffer size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff multipage page0 decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(gray_pixels, decode_buf, sizeof(gray_pixels), "bigtiff multipage gray")) {
        return 0;
    }

    rc = tifx_parse_bigtiff_page_memory(&info, file_buf, written, 1UL);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff multipage parse page1 %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.page_count != 3UL ||
        info.page_index != 1UL ||
        info.new_subfile_type != 2UL ||
        info.page_number[0] != 1U ||
        info.page_number[1] != 3U ||
        info.next_ifd_offset == 0UL) {
        printf("FAIL: bigtiff multipage page1 metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(rgb_pixels) || stride != 6UL) {
        printf("FAIL: bigtiff multipage page1 buffer size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff multipage page1 decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(rgb_pixels, decode_buf, sizeof(rgb_pixels), "bigtiff multipage rgb")) {
        return 0;
    }

    rc = tifx_parse_bigtiff_page_memory(&info, file_buf, written, 2UL);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff multipage parse page2 %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.page_count != 3UL ||
        info.page_index != 2UL ||
        info.new_subfile_type != 2UL ||
        info.page_number[0] != 2U ||
        info.page_number[1] != 3U ||
        info.next_ifd_offset != 0UL ||
        info.compression != 4U ||
        info.strip_count != 2UL) {
        printf("FAIL: bigtiff multipage page2 metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(bilevel_expected) || stride != 8UL) {
        printf("FAIL: bigtiff multipage page2 buffer size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff multipage page2 decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(bilevel_expected, decode_buf, sizeof(bilevel_expected), "bigtiff multipage bilevel")) {
        return 0;
    }

    rc = tifx_parse_bigtiff_page_memory(&info, file_buf, written, 3UL);
    if (rc != TIFX_ERR_BAD_ARGUMENT) {
        printf("FAIL: bigtiff multipage out-of-range page index\n");
        return 0;
    }

    return 1;
#endif
}


static int test_bigtiff_subifd_tree_roundtrip(void)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    return 1;
#else
    static const unsigned char root_pixels[16] = {
        0U, 1U, 2U, 3U,
        4U, 5U, 6U, 7U,
        8U, 9U, 10U, 11U,
        12U, 13U, 14U, 15U
    };
    static const unsigned char reduced_pixels[4] = { 10U, 20U, 30U, 40U };
    static const unsigned char tiny_pixels[1] = { 77U };
    static const unsigned char layer_pixels[16] = {
        255U, 240U, 225U, 210U,
        195U, 180U, 165U, 150U,
        135U, 120U, 105U, 90U,
        75U, 60U, 45U, 30U
    };
    tifx_bigtiff_node tree_pages[1];
    tifx_bigtiff_node root_children[2];
    tifx_bigtiff_node reduced_children[1];
    unsigned char file_buf[4096];
    unsigned char decode_buf[64];
    unsigned char workspace[64];
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long subifd_count;
    unsigned long path_one[1];
    unsigned long path_two[2];
    unsigned long root_ifd_offset;
    unsigned long reduced_ifd_offset;
    int rc;

    tifx_write_params_init(&tree_pages[0].image);
    tree_pages[0].image.container_format = TIFX_CONTAINER_BIGTIFF;
    tree_pages[0].image.pixel_format = TIFX_PIXEL_GRAY8;
    tree_pages[0].image.width = 4UL;
    tree_pages[0].image.height = 4UL;
    tree_pages[0].image.stride = 4UL;
    tree_pages[0].image.pixels = root_pixels;
    tree_pages[0].children = root_children;
    tree_pages[0].child_count = 2UL;

    tifx_write_params_init(&root_children[0].image);
    root_children[0].image.container_format = TIFX_CONTAINER_BIGTIFF;
    root_children[0].image.pixel_format = TIFX_PIXEL_GRAY8;
    root_children[0].image.width = 2UL;
    root_children[0].image.height = 2UL;
    root_children[0].image.stride = 2UL;
    root_children[0].image.new_subfile_type = 1UL;
    root_children[0].image.pixels = reduced_pixels;
    root_children[0].children = reduced_children;
    root_children[0].child_count = 1UL;

    tifx_write_params_init(&reduced_children[0].image);
    reduced_children[0].image.container_format = TIFX_CONTAINER_BIGTIFF;
    reduced_children[0].image.pixel_format = TIFX_PIXEL_GRAY8;
    reduced_children[0].image.width = 1UL;
    reduced_children[0].image.height = 1UL;
    reduced_children[0].image.stride = 1UL;
    reduced_children[0].image.new_subfile_type = 1UL;
    reduced_children[0].image.pixels = tiny_pixels;
    reduced_children[0].children = 0;
    reduced_children[0].child_count = 0UL;

    tifx_write_params_init(&root_children[1].image);
    root_children[1].image.container_format = TIFX_CONTAINER_BIGTIFF;
    root_children[1].image.pixel_format = TIFX_PIXEL_GRAY8;
    root_children[1].image.width = 4UL;
    root_children[1].image.height = 4UL;
    root_children[1].image.stride = 4UL;
    root_children[1].image.pixels = layer_pixels;
    root_children[1].children = 0;
    root_children[1].child_count = 0UL;

    rc = tifx_write_bigtiff_tree_memory(file_buf, sizeof(file_buf), tree_pages, 1UL, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff subifd tree write %s\n", tifx_strerror(rc));
        return 0;
    }
    if (written < 16UL || file_buf[0] != 'I' || file_buf[1] != 'I' ||
        file_buf[2] != 43U || file_buf[3] != 0U) {
        printf("FAIL: bigtiff subifd tree header wrong\n");
        return 0;
    }

    rc = tifx_bigtiff_page_count_memory(file_buf, written, &subifd_count);
    if (rc != TIFX_OK || subifd_count != 1UL) {
        printf("FAIL: bigtiff subifd tree main page count wrong\n");
        return 0;
    }

    rc = tifx_bigtiff_subifd_count_memory(file_buf, written, 0UL, 0, 0UL, &subifd_count);
    if (rc != TIFX_OK || subifd_count != 2UL) {
        printf("FAIL: bigtiff subifd tree child count wrong\n");
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff subifd tree parse root %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.subifd_count != 2UL || info.subifd_depth != 0UL || info.parent_ifd_offset != 0UL) {
        printf("FAIL: bigtiff subifd tree root metadata wrong\n");
        return 0;
    }
    root_ifd_offset = info.current_ifd_offset;
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(root_pixels) || stride != 4UL) {
        printf("FAIL: bigtiff subifd tree root buffer size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff subifd tree root decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(root_pixels, decode_buf, sizeof(root_pixels), "bigtiff subifd tree root")) {
        return 0;
    }

    path_one[0] = 0UL;
    rc = tifx_bigtiff_subifd_count_memory(file_buf, written, 0UL, path_one, 1UL, &subifd_count);
    if (rc != TIFX_OK || subifd_count != 1UL) {
        printf("FAIL: bigtiff subifd tree reduced child count wrong\n");
        return 0;
    }
    rc = tifx_parse_bigtiff_node_memory(&info, file_buf, written, 0UL, path_one, 1UL);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff subifd tree parse reduced %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.width != 2UL || info.height != 2UL || info.new_subfile_type != 1UL ||
        info.subifd_depth != 1UL || info.subifd_index != 0UL ||
        info.parent_ifd_offset != root_ifd_offset || info.subifd_count != 1UL) {
        printf("FAIL: bigtiff subifd tree reduced metadata wrong\n");
        return 0;
    }
    reduced_ifd_offset = info.current_ifd_offset;
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(reduced_pixels) || stride != 2UL) {
        printf("FAIL: bigtiff subifd tree reduced buffer size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff subifd tree reduced decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(reduced_pixels, decode_buf, sizeof(reduced_pixels), "bigtiff subifd tree reduced")) {
        return 0;
    }

    path_two[0] = 0UL;
    path_two[1] = 0UL;
    rc = tifx_bigtiff_subifd_count_memory(file_buf, written, 0UL, path_two, 2UL, &subifd_count);
    if (rc != TIFX_OK || subifd_count != 0UL) {
        printf("FAIL: bigtiff subifd tree grandchild count wrong\n");
        return 0;
    }
    rc = tifx_parse_bigtiff_node_memory(&info, file_buf, written, 0UL, path_two, 2UL);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff subifd tree parse grandchild %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.width != 1UL || info.height != 1UL || info.new_subfile_type != 1UL ||
        info.subifd_depth != 2UL || info.subifd_index != 0UL ||
        info.parent_ifd_offset != reduced_ifd_offset || info.subifd_count != 0UL) {
        printf("FAIL: bigtiff subifd tree grandchild metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(tiny_pixels) || stride != 1UL) {
        printf("FAIL: bigtiff subifd tree grandchild buffer size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff subifd tree grandchild decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(tiny_pixels, decode_buf, sizeof(tiny_pixels), "bigtiff subifd tree grandchild")) {
        return 0;
    }

    path_one[0] = 1UL;
    rc = tifx_parse_bigtiff_node_memory(&info, file_buf, written, 0UL, path_one, 1UL);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff subifd tree parse related layer %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.width != 4UL || info.height != 4UL || info.new_subfile_type != 0UL ||
        info.subifd_depth != 1UL || info.subifd_index != 1UL ||
        info.parent_ifd_offset != root_ifd_offset) {
        printf("FAIL: bigtiff subifd tree related layer metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(layer_pixels) || stride != 4UL) {
        printf("FAIL: bigtiff subifd tree related layer buffer size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff subifd tree related layer decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(layer_pixels, decode_buf, sizeof(layer_pixels), "bigtiff subifd tree related layer")) {
        return 0;
    }

    path_two[0] = 2UL;
    rc = tifx_parse_bigtiff_node_memory(&info, file_buf, written, 0UL, path_two, 1UL);
    if (rc != TIFX_ERR_BAD_ARGUMENT) {
        printf("FAIL: bigtiff subifd tree out-of-range child index\n");
        return 0;
    }

    return 1;
#endif
}


static int test_classic_subifd_tiled_tree_roundtrip(void)
{
    static const unsigned char root_pixels[32UL * 32UL] = {
        0U
    };
    static const unsigned char child_pixels[16UL * 16UL] = {
        0U
    };
    tifx_tiff_node pages[1];
    tifx_tiff_node children[1];
    unsigned char mutable_root[32UL * 32UL];
    unsigned char mutable_child[16UL * 16UL];
    unsigned char file_buf[16384];
    unsigned char decode_buf[32UL * 32UL];
    unsigned char workspace[64];
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long subifd_count;
    unsigned long path[1];
    unsigned long root_ifd_offset;
    unsigned long i;
    int rc;

    (void)root_pixels;
    (void)child_pixels;
    for (i = 0UL; i < 32UL * 32UL; ++i) {
        mutable_root[i] = (unsigned char)((i * 7UL + 3UL) & 255UL);
    }
    for (i = 0UL; i < 16UL * 16UL; ++i) {
        mutable_child[i] = (unsigned char)((i * 11UL + 29UL) & 255UL);
    }

    tifx_write_params_init(&children[0].image);
    children[0].image.container_format = TIFX_CONTAINER_CLASSIC;
    children[0].image.pixel_format = TIFX_PIXEL_GRAY8;
    children[0].image.width = 16UL;
    children[0].image.height = 16UL;
    children[0].image.stride = 16UL;
    children[0].image.tile_width = 16UL;
    children[0].image.tile_length = 16UL;
    children[0].image.new_subfile_type = 1UL;
    children[0].image.pixels = mutable_child;
    children[0].children = 0;
    children[0].child_count = 0UL;

    tifx_write_params_init(&pages[0].image);
    pages[0].image.container_format = TIFX_CONTAINER_CLASSIC;
    pages[0].image.pixel_format = TIFX_PIXEL_GRAY8;
    pages[0].image.width = 32UL;
    pages[0].image.height = 32UL;
    pages[0].image.stride = 32UL;
    pages[0].image.tile_width = 16UL;
    pages[0].image.tile_length = 16UL;
    pages[0].image.pixels = mutable_root;
    pages[0].children = children;
    pages[0].child_count = 1UL;

    rc = tifx_write_classic_tree_memory(file_buf, sizeof(file_buf), pages, 1UL, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: classic tiled subifd tree write %s\n", tifx_strerror(rc));
        return 0;
    }
    if (written < 8UL || file_buf[0] != 'I' || file_buf[1] != 'I' ||
        file_buf[2] != 42U || file_buf[3] != 0U) {
        printf("FAIL: classic tiled subifd tree header wrong\n");
        return 0;
    }

    rc = tifx_classic_page_count_memory(file_buf, written, &subifd_count);
    if (rc != TIFX_OK || subifd_count != 1UL) {
        printf("FAIL: classic tiled subifd tree page count wrong\n");
        return 0;
    }
    rc = tifx_classic_subifd_count_memory(file_buf, written, 0UL, 0, 0UL, &subifd_count);
    if (rc != TIFX_OK || subifd_count != 1UL) {
        printf("FAIL: classic tiled subifd tree child count wrong\n");
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: classic tiled subifd tree parse root %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.container_format != TIFX_CONTAINER_CLASSIC ||
        info.storage_layout != TIFX_LAYOUT_TILES ||
        info.tile_width != 16UL || info.tile_length != 16UL || info.tile_count != 4UL ||
        info.subifd_count != 1UL) {
        printf("FAIL: classic tiled subifd tree root metadata wrong\n");
        return 0;
    }
    root_ifd_offset = info.current_ifd_offset;
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(mutable_root) || stride != 32UL) {
        printf("FAIL: classic tiled subifd tree root buffer size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: classic tiled subifd tree root decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(mutable_root, decode_buf, sizeof(mutable_root), "classic tiled subifd root")) {
        return 0;
    }

    path[0] = 0UL;
    rc = tifx_parse_classic_node_memory(&info, file_buf, written, 0UL, path, 1UL);
    if (rc != TIFX_OK) {
        printf("FAIL: classic tiled subifd tree parse child %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.width != 16UL || info.height != 16UL || info.new_subfile_type != 1UL ||
        info.storage_layout != TIFX_LAYOUT_TILES || info.tile_count != 1UL ||
        info.parent_ifd_offset != root_ifd_offset) {
        printf("FAIL: classic tiled subifd tree child metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(mutable_child) || stride != 16UL) {
        printf("FAIL: classic tiled subifd tree child buffer size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: classic tiled subifd tree child decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(mutable_child, decode_buf, sizeof(mutable_child), "classic tiled subifd child")) {
        return 0;
    }

    path[0] = 1UL;
    rc = tifx_parse_classic_node_memory(&info, file_buf, written, 0UL, path, 1UL);
    if (rc != TIFX_ERR_BAD_ARGUMENT) {
        printf("FAIL: classic tiled subifd tree out-of-range child index\n");
        return 0;
    }

    return 1;
}

static int test_bigtiff_subifd_tiled_tree_roundtrip(void)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    return 1;
#else
    tifx_bigtiff_node pages[1];
    tifx_bigtiff_node children[1];
    unsigned char root_pixels[32UL * 32UL];
    unsigned char child_pixels[16UL * 16UL];
    unsigned char file_buf[24576];
    unsigned char decode_buf[32UL * 32UL];
    unsigned char workspace[64];
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long subifd_count;
    unsigned long path[1];
    unsigned long root_ifd_offset;
    unsigned long i;
    int rc;

    if (!tifx_bigtiff_available()) {
        return 1;
    }
    for (i = 0UL; i < 32UL * 32UL; ++i) {
        root_pixels[i] = (unsigned char)((i * 5UL + 9UL) & 255UL);
    }
    for (i = 0UL; i < 16UL * 16UL; ++i) {
        child_pixels[i] = (unsigned char)((i * 13UL + 41UL) & 255UL);
    }

    tifx_write_params_init(&children[0].image);
    children[0].image.container_format = TIFX_CONTAINER_BIGTIFF;
    children[0].image.pixel_format = TIFX_PIXEL_GRAY8;
    children[0].image.width = 16UL;
    children[0].image.height = 16UL;
    children[0].image.stride = 16UL;
    children[0].image.tile_width = 16UL;
    children[0].image.tile_length = 16UL;
    children[0].image.new_subfile_type = 1UL;
    children[0].image.pixels = child_pixels;
    children[0].children = 0;
    children[0].child_count = 0UL;

    tifx_write_params_init(&pages[0].image);
    pages[0].image.container_format = TIFX_CONTAINER_BIGTIFF;
    pages[0].image.pixel_format = TIFX_PIXEL_GRAY8;
    pages[0].image.width = 32UL;
    pages[0].image.height = 32UL;
    pages[0].image.stride = 32UL;
    pages[0].image.tile_width = 16UL;
    pages[0].image.tile_length = 16UL;
    pages[0].image.pixels = root_pixels;
    pages[0].children = children;
    pages[0].child_count = 1UL;

    rc = tifx_write_bigtiff_tree_memory(file_buf, sizeof(file_buf), pages, 1UL, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff tiled subifd tree write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_bigtiff_page_count_memory(file_buf, written, &subifd_count);
    if (rc != TIFX_OK || subifd_count != 1UL) {
        printf("FAIL: bigtiff tiled subifd tree page count wrong\n");
        return 0;
    }
    rc = tifx_bigtiff_subifd_count_memory(file_buf, written, 0UL, 0, 0UL, &subifd_count);
    if (rc != TIFX_OK || subifd_count != 1UL) {
        printf("FAIL: bigtiff tiled subifd tree child count wrong\n");
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff tiled subifd tree parse root %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.container_format != TIFX_CONTAINER_BIGTIFF ||
        info.storage_layout != TIFX_LAYOUT_TILES ||
        info.tile_width != 16UL || info.tile_length != 16UL || info.tile_count != 4UL ||
        info.subifd_count != 1UL) {
        printf("FAIL: bigtiff tiled subifd tree root metadata wrong\n");
        return 0;
    }
    root_ifd_offset = info.current_ifd_offset;
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(root_pixels) || stride != 32UL) {
        printf("FAIL: bigtiff tiled subifd tree root buffer size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff tiled subifd tree root decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(root_pixels, decode_buf, sizeof(root_pixels), "bigtiff tiled subifd root")) {
        return 0;
    }

    path[0] = 0UL;
    rc = tifx_parse_bigtiff_node_memory(&info, file_buf, written, 0UL, path, 1UL);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff tiled subifd tree parse child %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.width != 16UL || info.height != 16UL || info.new_subfile_type != 1UL ||
        info.storage_layout != TIFX_LAYOUT_TILES || info.tile_count != 1UL ||
        info.parent_ifd_offset != root_ifd_offset) {
        printf("FAIL: bigtiff tiled subifd tree child metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(child_pixels) || stride != 16UL) {
        printf("FAIL: bigtiff tiled subifd tree child buffer size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff tiled subifd tree child decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(child_pixels, decode_buf, sizeof(child_pixels), "bigtiff tiled subifd child")) {
        return 0;
    }

    path[0] = 1UL;
    rc = tifx_parse_bigtiff_node_memory(&info, file_buf, written, 0UL, path, 1UL);
    if (rc != TIFX_ERR_BAD_ARGUMENT) {
        printf("FAIL: bigtiff tiled subifd tree out-of-range child index\n");
        return 0;
    }

    return 1;
#endif
}



static int test_bigtiff_tiled_deflate_predictor_pyramid_roundtrip(void)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    return 1;
#else
    tifx_bigtiff_node pages[1];
    tifx_bigtiff_node children[1];
    unsigned char root_pixels[64UL * 64UL * 3UL];
    unsigned char child_pixels[32UL * 32UL * 3UL];
    unsigned char file_buf[131072];
    unsigned char decode_buf[64UL * 64UL * 3UL];
    unsigned char workspace[256];
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long path[1];
    unsigned long root_ifd_offset;
    int rc;

    if (!tifx_bigtiff_available()) {
        return 1;
    }

    fill_rgb_pattern(root_pixels, 64UL, 64UL, 64UL * 3UL, 9UL);
    fill_rgb_pattern(child_pixels, 32UL, 32UL, 32UL * 3UL, 41UL);

    tifx_write_params_init(&children[0].image);
    children[0].image.container_format = TIFX_CONTAINER_BIGTIFF;
    children[0].image.pixel_format = TIFX_PIXEL_RGB24;
    children[0].image.compression = 8U;
    children[0].image.predictor = TIFX_PREDICTOR_HORIZONTAL;
    children[0].image.deflate_mode = TIFX_DEFLATE_FIXED;
    children[0].image.width = 32UL;
    children[0].image.height = 32UL;
    children[0].image.stride = 32UL * 3UL;
    children[0].image.tile_width = 16UL;
    children[0].image.tile_length = 16UL;
    children[0].image.new_subfile_type = 1UL;
    children[0].image.pixels = child_pixels;
    children[0].children = 0;
    children[0].child_count = 0UL;

    tifx_write_params_init(&pages[0].image);
    pages[0].image.container_format = TIFX_CONTAINER_BIGTIFF;
    pages[0].image.pixel_format = TIFX_PIXEL_RGB24;
    pages[0].image.compression = 8U;
    pages[0].image.predictor = TIFX_PREDICTOR_HORIZONTAL;
    pages[0].image.deflate_mode = TIFX_DEFLATE_DYNAMIC;
    pages[0].image.width = 64UL;
    pages[0].image.height = 64UL;
    pages[0].image.stride = 64UL * 3UL;
    pages[0].image.tile_width = 16UL;
    pages[0].image.tile_length = 16UL;
    pages[0].image.pixels = root_pixels;
    pages[0].children = children;
    pages[0].child_count = 1UL;

    rc = tifx_write_bigtiff_tree_memory(file_buf, sizeof(file_buf), pages, 1UL, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff tiled deflate pyramid write %s\n", tifx_strerror(rc));
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff tiled deflate pyramid parse root %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.container_format != TIFX_CONTAINER_BIGTIFF ||
        info.storage_layout != TIFX_LAYOUT_TILES ||
        info.compression != 8U ||
        info.predictor != TIFX_PREDICTOR_HORIZONTAL ||
        info.tile_width != 16UL || info.tile_length != 16UL ||
        info.subifd_count != 1UL) {
        printf("FAIL: bigtiff tiled deflate pyramid root metadata wrong\n");
        return 0;
    }
    root_ifd_offset = info.current_ifd_offset;
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(root_pixels) || stride != 64UL * 3UL) {
        printf("FAIL: bigtiff tiled deflate pyramid root size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(root_pixels),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff tiled deflate pyramid root decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(root_pixels, decode_buf, sizeof(root_pixels), "bigtiff tiled deflate pyramid root")) {
        return 0;
    }

    path[0] = 0UL;
    rc = tifx_parse_bigtiff_node_memory(&info, file_buf, written, 0UL, path, 1UL);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff tiled deflate pyramid parse child %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.parent_ifd_offset != root_ifd_offset ||
        info.new_subfile_type != 1UL ||
        info.width != 32UL || info.height != 32UL ||
        info.storage_layout != TIFX_LAYOUT_TILES ||
        info.compression != 8U ||
        info.predictor != TIFX_PREDICTOR_HORIZONTAL) {
        printf("FAIL: bigtiff tiled deflate pyramid child metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(child_pixels) || stride != 32UL * 3UL) {
        printf("FAIL: bigtiff tiled deflate pyramid child size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(child_pixels),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff tiled deflate pyramid child decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(child_pixels, decode_buf, sizeof(child_pixels), "bigtiff tiled deflate pyramid child");
#endif
}

static int test_deflate_auto_heuristics_rgb_roundtrip(void)
{
    unsigned char src_pixels[128UL * 64UL * 3UL];
    unsigned char file_auto[65536];
    unsigned char file_stored[65536];
    unsigned char decode_buf[128UL * 64UL * 3UL];
    unsigned char workspace[1024];
    tifx_write_params auto_params;
    tifx_write_params stored_params;
    tifx_image_info info;
    unsigned long written_auto;
    unsigned long written_stored;
    unsigned long stride;
    int rc;

    fill_rgb_mixed_pattern(src_pixels, 128UL, 64UL, 128UL * 3UL, 17UL);

    tifx_write_params_init(&auto_params);
    auto_params.pixel_format = TIFX_PIXEL_RGB24;
    auto_params.compression = 8U;
    auto_params.predictor = TIFX_PREDICTOR_HORIZONTAL;
    auto_params.deflate_mode = TIFX_DEFLATE_AUTO;
    auto_params.width = 128UL;
    auto_params.height = 64UL;
    auto_params.stride = 128UL * 3UL;
    auto_params.rows_per_strip = 16UL;
    auto_params.pixels = src_pixels;

    stored_params = auto_params;
    stored_params.deflate_mode = TIFX_DEFLATE_STORED;

    rc = tifx_write_memory(file_auto, sizeof(file_auto), &auto_params, &written_auto);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate auto heuristics write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_write_memory(file_stored, sizeof(file_stored), &stored_params, &written_stored);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate stored baseline write %s\n", tifx_strerror(rc));
        return 0;
    }
    if (written_auto >= written_stored) {
        printf("FAIL: deflate auto heuristics did not beat stored (%lu vs %lu)\n",
               written_auto,
               written_stored);
        return 0;
    }

    rc = tifx_parse_memory(&info, file_auto, written_auto);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate auto heuristics parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.compression != 8U || info.predictor != TIFX_PREDICTOR_HORIZONTAL ||
        info.pixel_format != TIFX_PIXEL_RGB24) {
        printf("FAIL: deflate auto heuristics metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(src_pixels) || stride != 128UL * 3UL) {
        printf("FAIL: deflate auto heuristics size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_auto, written_auto, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: deflate auto heuristics decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "deflate auto heuristics");
}

static int test_classic_tiled_packbits_bilevel_roundtrip(void)
{
    unsigned char src_bits[17UL * 3UL];
    unsigned char expected[17UL * 18UL];
    unsigned char file_buf[4096];
    unsigned char decode_buf[17UL * 18UL];
    unsigned char workspace[64];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long x;
    unsigned long y;
    int rc;

    memset(src_bits, 0, sizeof(src_bits));
    for (y = 0UL; y < 17UL; ++y) {
        for (x = 0UL; x < 18UL; ++x) {
            unsigned short bit;

            bit = (unsigned short)((((x * 3UL) + (y * 5UL) + (x ^ y)) & 1UL) != 0UL);
            set_packed_bit_test(src_bits + (y * 3UL), x, bit);
            expected[y * 18UL + x] = bit ? 255U : 0U;
        }
    }

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_BILEVEL;
    params.compression = 32773U;
    params.photometric = 1U;
    params.fill_order = 2U;
    params.width = 18UL;
    params.height = 17UL;
    params.stride = 3UL;
    params.tile_width = 16UL;
    params.tile_length = 16UL;
    params.pixels = src_bits;

    rc = tifx_write_classic_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: classic tiled packbits bilevel write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: classic tiled packbits bilevel parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.storage_layout != TIFX_LAYOUT_TILES || info.compression != 32773U ||
        info.tile_width != 16UL || info.tile_length != 16UL || info.tile_count != 4UL ||
        info.fill_order != 2U) {
        printf("FAIL: classic tiled packbits bilevel metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(decode_buf) || stride != 18UL) {
        printf("FAIL: classic tiled packbits bilevel size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info,
                            file_buf,
                            written,
                            decode_buf,
                            sizeof(decode_buf),
                            stride,
                            workspace,
                            sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: classic tiled packbits bilevel decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(expected, decode_buf, sizeof(expected), "classic tiled packbits bilevel");
}

static int test_classic_adobe_subifd_chain(void)
{
    tifx_tiff_node page;
    tifx_tiff_node children[2];
    unsigned char root_pixels[32UL * 32UL];
    unsigned char child0_pixels[16UL * 16UL];
    unsigned char child1_pixels[8UL * 8UL];
    unsigned char file_buf[32768];
    unsigned char decode_buf[32UL * 32UL];
    unsigned char workspace[64];
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long path[1];
    unsigned long subifd_count;
    unsigned long root_ifd_offset;
    unsigned long tag_count;
    unsigned long tag_value;
    unsigned long next_ifd;
    int rc;

    fill_gray_pattern(root_pixels, 32UL, 32UL, 32UL, 3UL);
    fill_gray_pattern(child0_pixels, 16UL, 16UL, 16UL, 17UL);
    fill_gray_pattern(child1_pixels, 8UL, 8UL, 8UL, 29UL);

    tifx_write_params_init(&children[0].image);
    children[0].image.container_format = TIFX_CONTAINER_CLASSIC;
    children[0].image.pixel_format = TIFX_PIXEL_GRAY8;
    children[0].image.compression = 32773U;
    children[0].image.width = 16UL;
    children[0].image.height = 16UL;
    children[0].image.stride = 16UL;
    children[0].image.tile_width = 16UL;
    children[0].image.tile_length = 16UL;
    children[0].image.new_subfile_type = 1UL;
    children[0].image.pixels = child0_pixels;
    children[0].children = 0;
    children[0].child_count = 0UL;

    tifx_write_params_init(&children[1].image);
    children[1].image.container_format = TIFX_CONTAINER_CLASSIC;
    children[1].image.pixel_format = TIFX_PIXEL_GRAY8;
    children[1].image.compression = 32773U;
    children[1].image.width = 8UL;
    children[1].image.height = 8UL;
    children[1].image.stride = 8UL;
    children[1].image.tile_width = 16UL;
    children[1].image.tile_length = 16UL;
    children[1].image.new_subfile_type = 1UL;
    children[1].image.pixels = child1_pixels;
    children[1].children = 0;
    children[1].child_count = 0UL;

    tifx_write_params_init(&page.image);
    page.image.container_format = TIFX_CONTAINER_CLASSIC;
    page.image.pixel_format = TIFX_PIXEL_GRAY8;
    page.image.compression = 32773U;
    page.image.width = 32UL;
    page.image.height = 32UL;
    page.image.stride = 32UL;
    page.image.tile_width = 16UL;
    page.image.tile_length = 16UL;
    page.image.subifd_style = TIFX_SUBIFD_STYLE_ADOBE_CHAIN;
    page.image.pixels = root_pixels;
    page.children = children;
    page.child_count = 2UL;

    rc = tifx_write_classic_tree_memory(file_buf, sizeof(file_buf), &page, 1UL, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: classic adobe subifd write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: classic adobe subifd parse root %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.subifd_count != 2UL || info.storage_layout != TIFX_LAYOUT_TILES ||
        info.compression != 32773U) {
        printf("FAIL: classic adobe subifd root metadata wrong\n");
        return 0;
    }
    root_ifd_offset = info.current_ifd_offset;
    rc = tifx_classic_subifd_count_memory(file_buf, written, 0UL, 0, 0UL, &subifd_count);
    if (rc != TIFX_OK || subifd_count != 2UL) {
        printf("FAIL: classic adobe subifd count wrong\n");
        return 0;
    }
    if (!find_classic_tag_value(file_buf, written, root_ifd_offset, 330U,
                                &tag_count, &tag_value, &next_ifd) ||
        tag_count != 1UL || tag_value != info.subifd_offsets[0] || next_ifd != 0UL) {
        printf("FAIL: classic adobe subifd root tag wrong\n");
        return 0;
    }
    if (!find_classic_tag_value(file_buf, written, info.subifd_offsets[0], 256U,
                                &tag_count, &tag_value, &next_ifd) ||
        next_ifd != info.subifd_offsets[1]) {
        printf("FAIL: classic adobe subifd first child chain wrong\n");
        return 0;
    }
    if (!find_classic_tag_value(file_buf, written, info.subifd_offsets[1], 256U,
                                &tag_count, &tag_value, &next_ifd) || next_ifd != 0UL) {
        printf("FAIL: classic adobe subifd second child chain wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(root_pixels) || stride != 32UL) {
        printf("FAIL: classic adobe subifd root size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: classic adobe subifd root decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(root_pixels, decode_buf, sizeof(root_pixels), "classic adobe subifd root")) {
        return 0;
    }

    path[0] = 1UL;
    rc = tifx_parse_classic_node_memory(&info, file_buf, written, 0UL, path, 1UL);
    if (rc != TIFX_OK) {
        printf("FAIL: classic adobe subifd parse child1 %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.parent_ifd_offset != root_ifd_offset || info.width != 8UL || info.height != 8UL ||
        info.new_subfile_type != 1UL) {
        printf("FAIL: classic adobe subifd child1 metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(child1_pixels) || stride != 8UL) {
        printf("FAIL: classic adobe subifd child1 size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: classic adobe subifd child1 decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(child1_pixels, decode_buf, sizeof(child1_pixels), "classic adobe subifd child1");
}

static int test_bigtiff_adobe_subifd_chain(void)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    return 1;
#else
    tifx_bigtiff_node page;
    tifx_bigtiff_node children[2];
    unsigned char root_pixels[32UL * 32UL];
    unsigned char child0_pixels[16UL * 16UL];
    unsigned char child1_pixels[8UL * 8UL];
    unsigned char file_buf[65536];
    unsigned char decode_buf[32UL * 32UL];
    unsigned char workspace[64];
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long path[1];
    unsigned long subifd_count;
    unsigned long root_ifd_offset;
    unsigned long tag_count;
    unsigned long tag_value;
    unsigned long next_ifd;
    int rc;

    if (!tifx_bigtiff_available()) {
        return 1;
    }

    fill_gray_pattern(root_pixels, 32UL, 32UL, 32UL, 5UL);
    fill_gray_pattern(child0_pixels, 16UL, 16UL, 16UL, 21UL);
    fill_gray_pattern(child1_pixels, 8UL, 8UL, 8UL, 37UL);

    tifx_write_params_init(&children[0].image);
    children[0].image.container_format = TIFX_CONTAINER_BIGTIFF;
    children[0].image.pixel_format = TIFX_PIXEL_GRAY8;
    children[0].image.compression = 32773U;
    children[0].image.width = 16UL;
    children[0].image.height = 16UL;
    children[0].image.stride = 16UL;
    children[0].image.tile_width = 16UL;
    children[0].image.tile_length = 16UL;
    children[0].image.new_subfile_type = 1UL;
    children[0].image.pixels = child0_pixels;
    children[0].children = 0;
    children[0].child_count = 0UL;

    tifx_write_params_init(&children[1].image);
    children[1].image.container_format = TIFX_CONTAINER_BIGTIFF;
    children[1].image.pixel_format = TIFX_PIXEL_GRAY8;
    children[1].image.compression = 32773U;
    children[1].image.width = 8UL;
    children[1].image.height = 8UL;
    children[1].image.stride = 8UL;
    children[1].image.tile_width = 16UL;
    children[1].image.tile_length = 16UL;
    children[1].image.new_subfile_type = 1UL;
    children[1].image.pixels = child1_pixels;
    children[1].children = 0;
    children[1].child_count = 0UL;

    tifx_write_params_init(&page.image);
    page.image.container_format = TIFX_CONTAINER_BIGTIFF;
    page.image.pixel_format = TIFX_PIXEL_GRAY8;
    page.image.compression = 32773U;
    page.image.width = 32UL;
    page.image.height = 32UL;
    page.image.stride = 32UL;
    page.image.tile_width = 16UL;
    page.image.tile_length = 16UL;
    page.image.subifd_style = TIFX_SUBIFD_STYLE_ADOBE_CHAIN;
    page.image.pixels = root_pixels;
    page.children = children;
    page.child_count = 2UL;

    rc = tifx_write_bigtiff_tree_memory(file_buf, sizeof(file_buf), &page, 1UL, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff adobe subifd write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff adobe subifd parse root %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.subifd_count != 2UL || info.storage_layout != TIFX_LAYOUT_TILES ||
        info.compression != 32773U) {
        printf("FAIL: bigtiff adobe subifd root metadata wrong\n");
        return 0;
    }
    root_ifd_offset = info.current_ifd_offset;
    rc = tifx_bigtiff_subifd_count_memory(file_buf, written, 0UL, 0, 0UL, &subifd_count);
    if (rc != TIFX_OK || subifd_count != 2UL) {
        printf("FAIL: bigtiff adobe subifd count wrong\n");
        return 0;
    }
    if (!find_bigtiff_tag_value(file_buf, written, root_ifd_offset, 330U,
                                &tag_count, &tag_value, &next_ifd) ||
        tag_count != 1UL || tag_value != info.subifd_offsets[0] || next_ifd != 0UL) {
        printf("FAIL: bigtiff adobe subifd root tag wrong\n");
        return 0;
    }
    if (!find_bigtiff_tag_value(file_buf, written, info.subifd_offsets[0], 256U,
                                &tag_count, &tag_value, &next_ifd) ||
        next_ifd != info.subifd_offsets[1]) {
        printf("FAIL: bigtiff adobe subifd first child chain wrong\n");
        return 0;
    }
    if (!find_bigtiff_tag_value(file_buf, written, info.subifd_offsets[1], 256U,
                                &tag_count, &tag_value, &next_ifd) || next_ifd != 0UL) {
        printf("FAIL: bigtiff adobe subifd second child chain wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(root_pixels) || stride != 32UL) {
        printf("FAIL: bigtiff adobe subifd root size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff adobe subifd root decode %s\n", tifx_strerror(rc));
        return 0;
    }
    if (!check_equal(root_pixels, decode_buf, sizeof(root_pixels), "bigtiff adobe subifd root")) {
        return 0;
    }

    path[0] = 1UL;
    rc = tifx_parse_bigtiff_node_memory(&info, file_buf, written, 0UL, path, 1UL);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff adobe subifd parse child1 %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.parent_ifd_offset != root_ifd_offset || info.width != 8UL || info.height != 8UL ||
        info.new_subfile_type != 1UL) {
        printf("FAIL: bigtiff adobe subifd child1 metadata wrong\n");
        return 0;
    }
    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(child1_pixels) || stride != 8UL) {
        printf("FAIL: bigtiff adobe subifd child1 size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff adobe subifd child1 decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(child1_pixels, decode_buf, sizeof(child1_pixels), "bigtiff adobe subifd child1");
#endif
}

static int test_classic_planar_rgb_roundtrip(void)
{
    unsigned char src_pixels[4UL * 3UL * 3UL];
    unsigned char file_buf[4096];
    unsigned char decode_buf[4UL * 3UL * 3UL];
    unsigned char workspace[64];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long tag_count;
    unsigned long tag_value;
    unsigned long next_ifd;
    int rc;

    fill_rgb_pattern(src_pixels, 4UL, 3UL, 12UL, 19UL);

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_RGB24;
    params.planar_config = 2U;
    params.compression = 1U;
    params.width = 4UL;
    params.height = 3UL;
    params.stride = 12UL;
    params.rows_per_strip = 2UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: classic planar rgb write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: classic planar rgb parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.planar_config != 2U || info.pixel_format != TIFX_PIXEL_RGB24 ||
        info.strip_count != 6UL) {
        printf("FAIL: classic planar rgb metadata wrong\n");
        return 0;
    }
    if (!find_classic_tag_value(file_buf, written, info.current_ifd_offset, 284U,
                                &tag_count, &tag_value, &next_ifd) ||
        tag_count != 1UL || tag_value != 2UL) {
        printf("FAIL: classic planar rgb planar tag wrong\n");
        return 0;
    }
    if (!find_classic_tag_value(file_buf, written, info.current_ifd_offset, 273U,
                                &tag_count, &tag_value, &next_ifd) ||
        tag_count != 6UL) {
        printf("FAIL: classic planar rgb strip count tag wrong\n");
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(src_pixels) || stride != 12UL) {
        printf("FAIL: classic planar rgb decode size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: classic planar rgb decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "classic planar rgb");
}

static int test_classic_planar_rgba_packbits_tile_roundtrip(void)
{
    unsigned char src_pixels[16UL * 16UL * 4UL];
    unsigned char file_buf[16384];
    unsigned char decode_buf[16UL * 16UL * 4UL];
    unsigned char workspace[512];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long tag_count;
    unsigned long tag_value;
    unsigned long next_ifd;
    int rc;

    fill_rgba_pattern(src_pixels, 16UL, 16UL, 64UL, 23UL);

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_RGBA32;
    params.alpha_mode = TIFX_ALPHA_UNASSOCIATED;
    params.planar_config = 2U;
    params.compression = 32773U;
    params.width = 16UL;
    params.height = 16UL;
    params.stride = 64UL;
    params.tile_width = 16UL;
    params.tile_length = 16UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: classic planar rgba tile write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: classic planar rgba tile parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.planar_config != 2U || info.pixel_format != TIFX_PIXEL_RGBA32 ||
        info.storage_layout != TIFX_LAYOUT_TILES || info.tile_count != 4UL) {
        printf("FAIL: classic planar rgba tile metadata wrong\n");
        return 0;
    }
    if (!find_classic_tag_value(file_buf, written, info.current_ifd_offset, 284U,
                                &tag_count, &tag_value, &next_ifd) ||
        tag_count != 1UL || tag_value != 2UL) {
        printf("FAIL: classic planar rgba tile planar tag wrong\n");
        return 0;
    }
    if (!find_classic_tag_value(file_buf, written, info.current_ifd_offset, 324U,
                                &tag_count, &tag_value, &next_ifd) ||
        tag_count != 4UL) {
        printf("FAIL: classic planar rgba tile offsets tag wrong\n");
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(src_pixels) || stride != 64UL) {
        printf("FAIL: classic planar rgba tile decode size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: classic planar rgba tile decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "classic planar rgba tile");
}

static int test_bigtiff_planar_rgb_roundtrip(void)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    return 1;
#else
    unsigned char src_pixels[48UL * 24UL * 3UL];
    unsigned char file_buf[32768];
    unsigned char decode_buf[48UL * 24UL * 3UL];
    unsigned char workspace[192];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stride;
    unsigned long tag_count;
    unsigned long tag_value;
    unsigned long next_ifd;
    int rc;

    if (!tifx_bigtiff_available()) {
        return 1;
    }

    fill_rgb_pattern(src_pixels, 5UL, 4UL, 15UL, 31UL);

    tifx_write_params_init(&params);
    params.container_format = TIFX_CONTAINER_BIGTIFF;
    params.pixel_format = TIFX_PIXEL_RGB24;
    params.planar_config = 2U;
    params.compression = 1U;
    params.width = 48UL;
    params.height = 24UL;
    params.stride = 144UL;
    params.rows_per_strip = 2UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff planar rgb write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff planar rgb parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.container_format != TIFX_CONTAINER_BIGTIFF || info.planar_config != 2U ||
        info.pixel_format != TIFX_PIXEL_RGB24 || info.strip_count != 6UL) {
        printf("FAIL: bigtiff planar rgb metadata wrong\n");
        return 0;
    }
    if (!find_bigtiff_tag_value(file_buf, written, info.current_ifd_offset, 284U,
                                &tag_count, &tag_value, &next_ifd) ||
        tag_count != 1UL || tag_value != 2UL) {
        printf("FAIL: bigtiff planar rgb planar tag wrong\n");
        return 0;
    }
    if (!find_bigtiff_tag_value(file_buf, written, info.current_ifd_offset, 273U,
                                &tag_count, &tag_value, &next_ifd) ||
        tag_count != 6UL) {
        printf("FAIL: bigtiff planar rgb strip count tag wrong\n");
        return 0;
    }

    stride = 0UL;
    if (tifx_decode_buffer_size(&info, &stride) != sizeof(src_pixels) || stride != 15UL) {
        printf("FAIL: bigtiff planar rgb decode size wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written, decode_buf, sizeof(decode_buf),
                            stride, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff planar rgb decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "bigtiff planar rgb");
#endif
}


static int test_lzw_gray_roundtrip(void)
{
    unsigned char src_pixels[96UL * 64UL];
    unsigned char file_buf[32768];
    unsigned char decode_buf[96UL * 64UL];
    unsigned char workspace[128];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    int rc;

    fill_gray_pattern(src_pixels, 96UL, 64UL, 96UL, 41UL);

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_GRAY8;
    params.compression = 5U;
    params.width = 96UL;
    params.height = 64UL;
    params.stride = 96UL;
    params.rows_per_strip = 16UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: lzw gray write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: lzw gray parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.compression != 5U || info.pixel_format != TIFX_PIXEL_GRAY8) {
        printf("FAIL: lzw gray metadata wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written,
                            decode_buf, sizeof(decode_buf),
                            96UL, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: lzw gray decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "lzw gray");
}

static int test_deflate_rgb_roundtrip(void)
{
    unsigned char src_pixels[7UL * 5UL * 3UL];
    unsigned char file_buf[8192];
    unsigned char decode_buf[7UL * 5UL * 3UL];
    unsigned char workspace[128];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    int rc;

    fill_rgb_pattern(src_pixels, 7UL, 5UL, 21UL, 53UL);

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_RGB24;
    params.compression = 8U;
    params.width = 7UL;
    params.height = 5UL;
    params.stride = 21UL;
    params.rows_per_strip = 2UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate rgb write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate rgb parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.compression != 8U || info.pixel_format != TIFX_PIXEL_RGB24 || info.strip_count != 3UL) {
        printf("FAIL: deflate rgb metadata wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written,
                            decode_buf, sizeof(decode_buf),
                            21UL, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: deflate rgb decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "deflate rgb");
}

static int test_deflate_gray_empty_final_block_roundtrip(void)
{
    static const unsigned char empty_final_block[5] = {
        0x01U, 0x00U, 0x00U, 0xFFU, 0xFFU
    };
    unsigned char src_pixels[4] = { 1U, 2U, 3U, 4U };
    unsigned char file_buf[512];
    unsigned char decode_buf[4];
    unsigned char workspace[32];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long insert_pos;
    unsigned long old_ifd_offset;
    unsigned long new_ifd_offset;
    unsigned long x_count;
    unsigned long x_offset;
    unsigned long y_count;
    unsigned long y_offset;
    unsigned long next_ifd;
    int rc;

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_GRAY8;
    params.compression = 8U;
    params.width = 4UL;
    params.height = 1UL;
    params.stride = 4UL;
    params.rows_per_strip = 1UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate empty-final write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate empty-final initial parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.strip_count != 1UL || info.strip_offsets[0] < 2UL || info.strip_byte_counts[0] < 6UL) {
        printf("FAIL: deflate empty-final initial metadata wrong\n");
        return 0;
    }

    old_ifd_offset = info.current_ifd_offset;
    if (!find_classic_tag_value(file_buf, written, old_ifd_offset, 282U,
                                &x_count, &x_offset, &next_ifd) ||
        !find_classic_tag_value(file_buf, written, old_ifd_offset, 283U,
                                &y_count, &y_offset, &next_ifd) ||
        x_count != 1UL || y_count != 1UL) {
        printf("FAIL: deflate empty-final locate resolution tags\n");
        return 0;
    }
    insert_pos = info.strip_offsets[0] + info.strip_byte_counts[0] - 4UL;
    if (insert_pos > written || written - insert_pos < 4UL) {
        printf("FAIL: deflate empty-final insert bounds\n");
        return 0;
    }
    if (written + 5UL > sizeof(file_buf)) {
        printf("FAIL: deflate empty-final buffer too small\n");
        return 0;
    }

    memmove(file_buf + insert_pos + 5UL,
            file_buf + insert_pos,
            written - insert_pos);
    memcpy(file_buf + insert_pos, empty_final_block, 5UL);
    file_buf[info.strip_offsets[0] + 2UL] = 0x00U;
    written += 5UL;

    new_ifd_offset = old_ifd_offset + 5UL;
    write_u32le(file_buf + 4UL, new_ifd_offset);
    if (!patch_classic_tag_value(file_buf, written, new_ifd_offset, 279U,
                                 info.strip_byte_counts[0] + 5UL) ||
        !patch_classic_tag_value(file_buf, written, new_ifd_offset, 282U,
                                 x_offset + 5UL) ||
        !patch_classic_tag_value(file_buf, written, new_ifd_offset, 283U,
                                 y_offset + 5UL)) {
        printf("FAIL: deflate empty-final patch tags\n");
        return 0;
    }

    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate empty-final parse %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written,
                            decode_buf, sizeof(decode_buf),
                            4UL, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: deflate empty-final decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels),
                       "deflate empty-final");
}

static int test_legacy_deflate_gray_roundtrip(void)
{
    unsigned char src_pixels[6UL * 4UL];
    unsigned char file_buf[4096];
    unsigned char decode_buf[6UL * 4UL];
    unsigned char workspace[64];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    int rc;

    fill_gray_pattern(src_pixels, 6UL, 4UL, 6UL, 67UL);

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_GRAY8;
    params.compression = 32946U;
    params.width = 6UL;
    params.height = 4UL;
    params.stride = 6UL;
    params.rows_per_strip = 2UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: legacy deflate gray write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: legacy deflate gray parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.compression != 32946U || info.pixel_format != TIFX_PIXEL_GRAY8 || info.strip_count != 2UL) {
        printf("FAIL: legacy deflate gray metadata wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written,
                            decode_buf, sizeof(decode_buf),
                            6UL, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: legacy deflate gray decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "legacy deflate gray");
}

static int test_bigtiff_lzw_rgb_roundtrip(void)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    return 1;
#else
    unsigned char src_pixels[5UL * 4UL * 3UL];
    unsigned char file_buf[8192];
    unsigned char decode_buf[5UL * 4UL * 3UL];
    unsigned char workspace[128];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    int rc;

    if (!tifx_bigtiff_available()) {
        return 1;
    }

    fill_rgb_pattern(src_pixels, 48UL, 24UL, 144UL, 79UL);

    tifx_write_params_init(&params);
    params.container_format = TIFX_CONTAINER_BIGTIFF;
    params.pixel_format = TIFX_PIXEL_RGB24;
    params.compression = 5U;
    params.width = 5UL;
    params.height = 4UL;
    params.stride = 15UL;
    params.rows_per_strip = 2UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff lzw rgb write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff lzw rgb parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.container_format != TIFX_CONTAINER_BIGTIFF ||
        info.compression != 5U || info.pixel_format != TIFX_PIXEL_RGB24 ||
        info.strip_count != 12UL) {
        printf("FAIL: bigtiff lzw rgb metadata wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written,
                            decode_buf, sizeof(decode_buf),
                            144UL, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff lzw rgb decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "bigtiff lzw rgb");
#endif
}

static int test_tiled_lzw_gray_predictor_roundtrip(void)
{
    unsigned char src_pixels[32UL * 32UL];
    unsigned char file_buf[16384];
    unsigned char decode_buf[32UL * 32UL];
    unsigned char workspace[128];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    int rc;

    fill_gray_pattern(src_pixels, 32UL, 32UL, 32UL, 91UL);

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_GRAY8;
    params.compression = 5U;
    params.predictor = TIFX_PREDICTOR_HORIZONTAL;
    params.width = 32UL;
    params.height = 32UL;
    params.stride = 32UL;
    params.tile_width = 16UL;
    params.tile_length = 16UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: tiled lzw predictor write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: tiled lzw predictor parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.compression != 5U || info.predictor != TIFX_PREDICTOR_HORIZONTAL ||
        info.storage_layout != TIFX_LAYOUT_TILES || info.tile_count != 4UL) {
        printf("FAIL: tiled lzw predictor metadata wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written,
                            decode_buf, sizeof(decode_buf),
                            32UL, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: tiled lzw predictor decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "tiled lzw predictor");
}

static int test_tiled_deflate_rgb_predictor_roundtrip(void)
{
    unsigned char src_pixels[32UL * 32UL * 3UL];
    unsigned char file_buf[32768];
    unsigned char decode_buf[32UL * 32UL * 3UL];
    unsigned char workspace[256];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    int rc;

    fill_rgb_pattern(src_pixels, 32UL, 32UL, 96UL, 103UL);

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_RGB24;
    params.compression = 8U;
    params.deflate_mode = TIFX_DEFLATE_DYNAMIC;
    params.predictor = TIFX_PREDICTOR_HORIZONTAL;
    params.width = 32UL;
    params.height = 32UL;
    params.stride = 96UL;
    params.tile_width = 16UL;
    params.tile_length = 16UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: tiled deflate predictor write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: tiled deflate predictor parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.compression != 8U || info.predictor != TIFX_PREDICTOR_HORIZONTAL ||
        info.storage_layout != TIFX_LAYOUT_TILES || info.tile_count != 4UL) {
        printf("FAIL: tiled deflate predictor metadata wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written,
                            decode_buf, sizeof(decode_buf),
                            96UL, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: tiled deflate predictor decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "tiled deflate predictor");
}

static int test_deflate_fixed_rgb_roundtrip(void)
{
    unsigned char src_pixels[64UL * 32UL * 3UL];
    unsigned char file_buf[65536];
    unsigned char decode_buf[64UL * 32UL * 3UL];
    unsigned char workspace[256];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long first_offset;
    unsigned int btype;
    int rc;

    fill_rgb_pattern(src_pixels, 64UL, 32UL, 192UL, 17UL);

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_RGB24;
    params.compression = 8U;
    params.deflate_mode = TIFX_DEFLATE_FIXED;
    params.predictor = TIFX_PREDICTOR_HORIZONTAL;
    params.width = 64UL;
    params.height = 32UL;
    params.stride = 192UL;
    params.rows_per_strip = 16UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate fixed write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate fixed parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.compression != 8U || info.predictor != TIFX_PREDICTOR_HORIZONTAL ||
        info.pixel_format != TIFX_PIXEL_RGB24 || info.strip_count != 2UL) {
        printf("FAIL: deflate fixed metadata wrong\n");
        return 0;
    }
    first_offset = info.strip_offsets[0];
    if (first_offset + 3UL > written) {
        printf("FAIL: deflate fixed strip bounds wrong\n");
        return 0;
    }
    btype = (unsigned int)((file_buf[first_offset + 2UL] >> 1U) & 0x03U);
    if (btype != 1U) {
        printf("FAIL: deflate fixed block type wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written,
                            decode_buf, sizeof(decode_buf),
                            192UL, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: deflate fixed decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "deflate fixed");
}

static int test_deflate_dynamic_rgb_roundtrip(void)
{
    unsigned char src_pixels[64UL * 32UL * 3UL];
    unsigned char file_buf[65536];
    unsigned char stored_file_buf[65536];
    unsigned char decode_buf[64UL * 32UL * 3UL];
    unsigned char workspace[256];
    tifx_write_params params;
    tifx_write_params stored_params;
    tifx_image_info info;
    unsigned long written;
    unsigned long stored_written;
    unsigned long first_offset;
    unsigned int btype;
    int rc;

    {
        unsigned long i;
        memset(src_pixels, 0, sizeof(src_pixels));
        for (i = 0UL; i < 64UL * 32UL; ++i) {
            src_pixels[i * 3UL + 0UL] = (unsigned char)((i & 1UL) ? 0U : 255U);
            src_pixels[i * 3UL + 1UL] = (unsigned char)((i & 3UL) ? 0U : 127U);
            src_pixels[i * 3UL + 2UL] = 0U;
        }
    }

    tifx_write_params_init(&params);
    params.pixel_format = TIFX_PIXEL_RGB24;
    params.compression = 8U;
    params.deflate_mode = TIFX_DEFLATE_DYNAMIC;
    params.predictor = TIFX_PREDICTOR_HORIZONTAL;
    params.width = 64UL;
    params.height = 32UL;
    params.stride = 192UL;
    params.rows_per_strip = 16UL;
    params.pixels = src_pixels;

    stored_params = params;
    stored_params.deflate_mode = TIFX_DEFLATE_STORED;

    rc = tifx_write_memory(stored_file_buf, sizeof(stored_file_buf), &stored_params, &stored_written);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate dynamic stored baseline %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate dynamic write %s\n", tifx_strerror(rc));
        return 0;
    }
    if (written >= stored_written) {
        printf("FAIL: deflate dynamic did not beat stored\n");
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: deflate dynamic parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.compression != 8U || info.predictor != TIFX_PREDICTOR_HORIZONTAL ||
        info.pixel_format != TIFX_PIXEL_RGB24 || info.strip_count != 2UL) {
        printf("FAIL: deflate dynamic metadata wrong\n");
        return 0;
    }
    first_offset = info.strip_offsets[0];
    if (first_offset + 3UL > written) {
        printf("FAIL: deflate dynamic strip bounds wrong\n");
        return 0;
    }
    btype = (unsigned int)((file_buf[first_offset + 2UL] >> 1U) & 0x03U);
    if (btype != 2U) {
        printf("FAIL: deflate dynamic block type wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written,
                            decode_buf, sizeof(decode_buf),
                            192UL, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: deflate dynamic decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "deflate dynamic");
}

static int test_bigtiff_deflate_fixed_predictor_roundtrip(void)
{
#if ULONG_MAX <= 0xFFFFFFFFUL
    return 1;
#else
    unsigned char src_pixels[24UL * 16UL * 3UL];
    unsigned char file_buf[32768];
    unsigned char decode_buf[24UL * 16UL * 3UL];
    unsigned char workspace[256];
    tifx_write_params params;
    tifx_image_info info;
    unsigned long written;
    unsigned long first_offset;
    unsigned int btype;
    int rc;

    if (!tifx_bigtiff_available()) {
        return 1;
    }
    fill_rgb_pattern(src_pixels, 24UL, 16UL, 72UL, 113UL);

    tifx_write_params_init(&params);
    params.container_format = TIFX_CONTAINER_BIGTIFF;
    params.pixel_format = TIFX_PIXEL_RGB24;
    params.compression = 8U;
    params.deflate_mode = TIFX_DEFLATE_FIXED;
    params.predictor = TIFX_PREDICTOR_HORIZONTAL;
    params.width = 24UL;
    params.height = 16UL;
    params.stride = 72UL;
    params.rows_per_strip = 8UL;
    params.pixels = src_pixels;

    rc = tifx_write_memory(file_buf, sizeof(file_buf), &params, &written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff deflate fixed write %s\n", tifx_strerror(rc));
        return 0;
    }
    rc = tifx_parse_memory(&info, file_buf, written);
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff deflate fixed parse %s\n", tifx_strerror(rc));
        return 0;
    }
    if (info.container_format != TIFX_CONTAINER_BIGTIFF || info.compression != 8U ||
        info.predictor != TIFX_PREDICTOR_HORIZONTAL || info.pixel_format != TIFX_PIXEL_RGB24) {
        printf("FAIL: bigtiff deflate fixed metadata wrong\n");
        return 0;
    }
    first_offset = info.strip_offsets[0];
    if (first_offset + 3UL > written) {
        printf("FAIL: bigtiff deflate fixed strip bounds wrong\n");
        return 0;
    }
    btype = (unsigned int)((file_buf[first_offset + 2UL] >> 1U) & 0x03U);
    if (btype != 1U) {
        printf("FAIL: bigtiff deflate fixed block type wrong\n");
        return 0;
    }
    rc = tifx_decode_memory(&info, file_buf, written,
                            decode_buf, sizeof(decode_buf),
                            72UL, workspace, sizeof(workspace));
    if (rc != TIFX_OK) {
        printf("FAIL: bigtiff deflate fixed decode %s\n", tifx_strerror(rc));
        return 0;
    }
    return check_equal(src_pixels, decode_buf, sizeof(src_pixels), "bigtiff deflate fixed");
#endif
}

int main(void)
{
    static const unsigned char expected_row[8] = {
        0U, 255U, 255U, 255U, 255U, 255U, 255U, 0U
    };
    static const unsigned char expected_mixed[16] = {
        255U, 255U, 255U, 255U, 255U, 255U, 255U, 255U,
        0U, 255U, 255U, 255U, 255U, 255U, 255U, 0U
    };

    if (!test_gray_roundtrip()) return 1;
    if (!test_rgb_roundtrip()) return 1;
    if (!test_rgba_roundtrip()) return 1;
    if (!test_packbits_gray()) return 1;
    if (!test_palette4()) return 1;
    if (!test_mh_bilevel(1U, "modified huffman fillorder=1")) return 1;
    if (!test_mh_bilevel(2U, "modified huffman fillorder=2")) return 1;
    if (!test_ccitt_fixture("tests/data/g4_fill1.tif", 4U, 0UL, 0UL, 1U,
                            "t6 group4 fillorder=1")) return 1;
    if (!test_ccitt_fixture("tests/data/g4_fill2.tif", 4U, 0UL, 0UL, 2U,
                            "t6 group4 fillorder=2")) return 1;
    if (!test_ccitt_fixture("tests/data/g3_1d_fill1.tif", 3U, 0UL, 0UL, 1U,
                            "t4 group3 1d")) return 1;
    if (!test_ccitt_fixture("tests/data/g3_1d_fill.tif", 3U, 4UL, 0UL, 1U,
                            "t4 group3 1d fillbits")) return 1;
    if (!test_ccitt_fixture("tests/data/g3_2d_fill.tif", 3U, 5UL, 0UL, 1U,
                            "t4 group3 2d fillbits")) return 1;
    if (!test_small_bilevel_fixture("tests/data/g4_uncompressed.tif", 8UL, 1UL,
                                    4U, 0UL, 2UL, 1U,
                                    expected_row, sizeof(expected_row),
                                    "t6 uncompressed extension")) return 1;
    if (!test_small_bilevel_fixture("tests/data/g3_1d_uncompressed.tif", 8UL, 1UL,
                                    3U, 2UL, 0UL, 1U,
                                    expected_row, sizeof(expected_row),
                                    "t4 1d uncompressed extension")) return 1;
    if (!test_small_bilevel_fixture("tests/data/g3_2d_uncompressed.tif", 8UL, 2UL,
                                    3U, 3UL, 0UL, 1U,
                                    expected_mixed, sizeof(expected_mixed),
                                    "t4 2d uncompressed extension")) return 1;
    if (!test_write_bilevel_ccitt_roundtrip(1U, 0UL, 2UL, 2U,
                                           "write bilevel uncompressed fillorder=2")) return 1;
    if (!test_write_bilevel_ccitt_roundtrip(2U, 0UL, 2UL, 1U,
                                           "write bilevel modified huffman multistrip")) return 1;
    if (!test_write_bilevel_ccitt_roundtrip(2U, 0UL, 2UL, 2U,
                                           "write bilevel modified huffman fillorder=2")) return 1;
    if (!test_write_bilevel_ccitt_roundtrip(3U, 4UL, 2UL, 1U,
                                           "write bilevel t4 1d multistrip")) return 1;
    if (!test_write_bilevel_ccitt_roundtrip(3U, 5UL, 2UL, 2U,
                                           "write bilevel t4 mr fillorder=2")) return 1;
    if (!test_write_bilevel_ccitt_roundtrip(4U, 0UL, 2UL, 1U,
                                           "write bilevel t6 multistrip")) return 1;
    if (!test_write_bilevel_ccitt_roundtrip(4U, 0UL, 2UL, 2U,
                                           "write bilevel t6 fillorder=2")) return 1;
    if (!test_write_bilevel_t6_roundtrip()) return 1;
    if (!test_classic_tiled_packbits_bilevel_roundtrip()) return 1;
    if (!test_classic_adobe_subifd_chain()) return 1;
    if (!test_classic_planar_rgb_roundtrip()) return 1;
    if (!test_classic_planar_rgba_packbits_tile_roundtrip()) return 1;
    if (!test_lzw_gray_roundtrip()) return 1;
    if (!test_deflate_rgb_roundtrip()) return 1;
    if (!test_deflate_gray_empty_final_block_roundtrip()) return 1;
    if (!test_legacy_deflate_gray_roundtrip()) return 1;
    if (!test_tiled_lzw_gray_predictor_roundtrip()) return 1;
    if (!test_tiled_deflate_rgb_predictor_roundtrip()) return 1;
    if (!test_deflate_fixed_rgb_roundtrip()) return 1;
    if (!test_deflate_dynamic_rgb_roundtrip()) return 1;
    if (!test_deflate_auto_heuristics_rgb_roundtrip()) return 1;
    if (!test_bigtiff_gray_roundtrip()) return 1;
    if (!test_bigtiff_planar_rgb_roundtrip()) return 1;
    if (!test_bigtiff_lzw_rgb_roundtrip()) return 1;
    if (!test_bigtiff_deflate_fixed_predictor_roundtrip()) return 1;
    if (!test_bigtiff_adobe_subifd_chain()) return 1;
    if (!test_bigtiff_rgb_roundtrip()) return 1;
    if (!test_bigtiff_rgba_roundtrip()) return 1;
    if (!test_bigtiff_t6_roundtrip()) return 1;
    if (!test_bigtiff_subifd_tree_roundtrip()) return 1;
    if (!test_classic_subifd_tiled_tree_roundtrip()) return 1;
    if (!test_bigtiff_subifd_tiled_tree_roundtrip()) return 1;
    if (!test_bigtiff_tiled_deflate_predictor_pyramid_roundtrip()) return 1;
    if (!test_bigtiff_multipage_roundtrip()) return 1;
    printf("PASS: all tifx tests\n");
    return 0;
}
