#include <stdio.h>
#include <string.h>

#include "../bmp/bmp.h"

#ifndef BMP_TOOL_FILE_CAPACITY
#define BMP_TOOL_FILE_CAPACITY (16U * 1024U * 1024U)
#endif
#ifndef BMP_TOOL_RGBA_CAPACITY
#define BMP_TOOL_RGBA_CAPACITY (64U * 1024U * 1024U)
#endif
#define BMP_TOOL_IO_CHUNK 65536U

static bmp_u8 g_file_data[BMP_TOOL_FILE_CAPACITY];
static bmp_u8 g_rgba_data[BMP_TOOL_RGBA_CAPACITY];

static int read_file_all(const char *path, const bmp_u8 **out_data, bmp_u32 *out_size)
{
    FILE *f;
    bmp_u32 used = 0U;
    bmp_u32 want;
    bmp_u32 got;
    int extra;

    if (!path || !out_data || !out_size) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;

    while (used < BMP_TOOL_FILE_CAPACITY) {
        want = BMP_TOOL_FILE_CAPACITY - used;
        if (want > BMP_TOOL_IO_CHUNK) want = BMP_TOOL_IO_CHUNK;
        got = (bmp_u32)fread(g_file_data + used, 1U, want, f);
        used += got;
        if (got != want) {
            if (ferror(f)) {
                fclose(f);
                return 0;
            }
            break;
        }
    }

    if (used == BMP_TOOL_FILE_CAPACITY) {
        extra = fgetc(f);
        if (extra != EOF) {
            fclose(f);
            return 0;
        }
    }
    fclose(f);
    *out_data = g_file_data;
    *out_size = used;
    return 1;
}

static int decode_static(const bmp_image *img, bmp_u32 *out_stride)
{
    bmp_u32 total = 0U;
    bmp_u32 width;
    int rc;

    if (!img || !out_stride) return BMP_ERR_ARGUMENT;
    if (img->meta.width <= 0) return BMP_ERR_FORMAT;
    width = (bmp_u32)img->meta.width;
    if (width > 0x3FFFFFFFU) return BMP_ERR_LIMITS;
    *out_stride = width * 4U;
    rc = bmp_calc_rgba32_buffer_size(img, &total);
    if (rc != BMP_OK) return rc;
    if (total > BMP_TOOL_RGBA_CAPACITY) return BMP_ERR_BUFFER_TOO_SMALL;
    return bmp_decode_to_rgba32(img, g_rgba_data, *out_stride);
}

static void usage(const char *argv0)
{
    fprintf(stderr, "usage: %s [--json] <file.bmp>\n", argv0 ? argv0 : "bmp_compat_cli");
    fprintf(stderr, "static limits: file=%u bytes rgba=%u bytes\n",
            (unsigned)BMP_TOOL_FILE_CAPACITY, (unsigned)BMP_TOOL_RGBA_CAPACITY);
}

int main(int argc, char **argv)
{
    const char *path = 0;
    const bmp_u8 *data = 0;
    int json = 0;
    int i;
    bmp_u32 size = 0U;
    bmp_image img;
    bmp_diagnostics diag;
    bmp_u32 stride = 0U;
    int parse_rc;
    int decode_rc = BMP_ERR_ARGUMENT;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--json") == 0) json = 1;
        else if (argv[i][0] == '-') { usage(argv[0]); return 2; }
        else path = argv[i];
    }
    if (!path) { usage(argv[0]); return 2; }

    if (!read_file_all(path, &data, &size)) {
        fprintf(stderr, "failed to read %s (missing, I/O error, or static file bank too small)\n", path);
        return 1;
    }

    parse_rc = bmp_parse_memory(data, size, &img);
    bmp_diagnostics_default(&diag);
    if (parse_rc == BMP_OK) {
        decode_rc = bmp_collect_diagnostics(&img, &diag);
        if (decode_rc == BMP_OK) decode_rc = decode_static(&img, &stride);
    }

    if (json) {
        printf("{\n");
        printf("  \"path\": \"%s\",\n", path);
        printf("  \"size\": %u,\n", (unsigned)size);
        printf("  \"parse_ok\": %s,\n", (parse_rc == BMP_OK) ? "true" : "false");
        printf("  \"parse_status\": \"%s\",\n", bmp_error_string(parse_rc));
        printf("  \"decode_ok\": %s,\n", (decode_rc == BMP_OK) ? "true" : "false");
        printf("  \"decode_status\": \"%s\",\n", bmp_error_string(decode_rc));
        if (parse_rc == BMP_OK) {
            printf("  \"dib_type\": \"%s\",\n", bmp_dib_type_string(img.meta.dib_type));
            printf("  \"compression\": \"%s\",\n", bmp_compression_string(img.meta.compression));
            printf("  \"width\": %d,\n", (int)img.meta.width);
            printf("  \"height\": %d,\n", (int)img.meta.height);
            printf("  \"bpp\": %u,\n", (unsigned)img.meta.bpp);
            printf("  \"warning_mask\": %u\n", (unsigned)diag.warning_mask);
        } else {
            printf("  \"dib_type\": null,\n");
            printf("  \"compression\": null,\n");
            printf("  \"width\": null,\n");
            printf("  \"height\": null,\n");
            printf("  \"bpp\": null,\n");
            printf("  \"warning_mask\": 0\n");
        }
        printf("}\n");
    } else {
        printf("parse=%s decode=%s\n", bmp_error_string(parse_rc), bmp_error_string(decode_rc));
    }

    return 0;
}
