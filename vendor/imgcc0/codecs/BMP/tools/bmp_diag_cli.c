#include <stdio.h>
#include <string.h>

#include "../bmp/bmp.h"

#ifndef BMP_TOOL_FILE_CAPACITY
#define BMP_TOOL_FILE_CAPACITY (16U * 1024U * 1024U)
#endif
#define BMP_TOOL_IO_CHUNK 65536U

static bmp_u8 g_file_data[BMP_TOOL_FILE_CAPACITY];

static void usage(const char *argv0)
{
    fprintf(stderr, "usage: %s [--json|--text] <file.bmp>\n",
            argv0 ? argv0 : "bmp_diag_cli");
    fprintf(stderr, "static file limit: %u bytes\n", (unsigned)BMP_TOOL_FILE_CAPACITY);
}

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

int main(int argc, char **argv)
{
    const char *path = 0;
    const bmp_u8 *data = 0;
    int json = 0;
    int i;
    int rc;
    bmp_u32 size = 0U;
    bmp_image img;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--json") == 0) json = 1;
        else if (strcmp(argv[i], "--text") == 0) json = 0;
        else if (argv[i][0] == '-') { usage(argv[0]); return 2; }
        else path = argv[i];
    }
    if (!path) { usage(argv[0]); return 2; }

    if (!read_file_all(path, &data, &size)) {
        fprintf(stderr, "failed to read %s (missing, I/O error, or static file bank too small)\n", path);
        return 1;
    }

    rc = bmp_parse_memory(data, size, &img);
    if (rc != BMP_OK) {
        fprintf(stderr, "bmp_parse_memory failed: %s\n", bmp_error_string(rc));
        return 1;
    }

    rc = json ? bmp_write_diagnostics_json(stdout, &img)
              : bmp_write_diagnostics_text(stdout, &img);
    if (rc != BMP_OK) {
        fprintf(stderr, "report output failed: %s\n", bmp_error_string(rc));
        return 1;
    }
    return 0;
}
