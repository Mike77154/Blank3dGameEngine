#include <stdio.h>
#include <string.h>
#include "woff1_decoder.h"
#include "woff1_inflate.h"

static unsigned char g_woff[WOFF1_MAX_WOFF_SIZE];
static unsigned char g_sfnt[WOFF1_MAX_SFNT_SIZE];
static unsigned char g_extra[WOFF1_MAX_SFNT_SIZE];

static int read_file_fixed(const char *path, unsigned char *buf,
                           woff1_u32 cap, woff1_u32 *len)
{
    FILE *f;
    long n;
    size_t got;

    *len = 0;
    f = fopen(path, "rb");
    if (f == 0) {
        fprintf(stderr, "cannot open input: %s\n", path);
        return 0;
    }
    if (fseek(f, 0L, SEEK_END) != 0) {
        fclose(f);
        fprintf(stderr, "cannot seek input\n");
        return 0;
    }
    n = ftell(f);
    if (n < 0) {
        fclose(f);
        fprintf(stderr, "cannot tell input size\n");
        return 0;
    }
    if ((unsigned long)n > (unsigned long)cap) {
        fclose(f);
        fprintf(stderr, "input exceeds static buffer limit (%lu bytes)\n", (unsigned long)cap);
        return 0;
    }
    if (fseek(f, 0L, SEEK_SET) != 0) {
        fclose(f);
        fprintf(stderr, "cannot rewind input\n");
        return 0;
    }
    got = fread(buf, 1u, (size_t)n, f);
    fclose(f);
    if (got != (size_t)n) {
        fprintf(stderr, "short read\n");
        return 0;
    }
    *len = (woff1_u32)n;
    return 1;
}

static int write_file_fixed(const char *path, const unsigned char *buf, woff1_u32 len)
{
    FILE *f;
    size_t wrote;

    f = fopen(path, "wb");
    if (f == 0) {
        fprintf(stderr, "cannot open output: %s\n", path);
        return 0;
    }
    wrote = fwrite(buf, 1u, (size_t)len, f);
    fclose(f);
    if (wrote != (size_t)len) {
        fprintf(stderr, "short write: %s\n", path);
        return 0;
    }
    return 1;
}

static void print_usage(void)
{
    printf("monika-woff1dec C89 static-buffer WOFF1 decoder\n");
    printf("usage:\n");
    printf("  woff1dec input.woff output.ttf [--meta metadata.xml] [--priv private.bin]\n");
    printf("limits:\n");
    printf("  WOFF input <= %lu bytes\n", (unsigned long)WOFF1_MAX_WOFF_SIZE);
    printf("  SFNT output <= %lu bytes\n", (unsigned long)WOFF1_MAX_SFNT_SIZE);
}

int main(int argc, char **argv)
{
    woff1_u32 woff_len;
    woff1_u32 sfnt_len;
    woff1_u32 extra_len;
    woff1_report report;
    int r;
    int i;
    const char *meta_out;
    const char *priv_out;

    meta_out = 0;
    priv_out = 0;
    if (argc < 3) {
        print_usage();
        return 2;
    }
    i = 3;
    while (i < argc) {
        if (strcmp(argv[i], "--meta") == 0 && i + 1 < argc) {
            meta_out = argv[i + 1];
            i += 2;
        } else if (strcmp(argv[i], "--priv") == 0 && i + 1 < argc) {
            priv_out = argv[i + 1];
            i += 2;
        } else {
            print_usage();
            return 2;
        }
    }

    if (!read_file_fixed(argv[1], g_woff, (woff1_u32)WOFF1_MAX_WOFF_SIZE, &woff_len)) {
        return 1;
    }

    r = woff1_decode_to_sfnt(g_woff, woff_len,
                             g_sfnt, (woff1_u32)WOFF1_MAX_SFNT_SIZE,
                             &sfnt_len, &report);
    if (r != WOFF1_OK) {
        fprintf(stderr, "decode failed: %s\n", woff1_error_string(r));
        if (r == WOFF1_ERR_INFLATE) {
            fprintf(stderr, "inflate detail: %s\n", woff1_inflate_error_string(report.last_inflate_error));
        }
        return 1;
    }

    if (!write_file_fixed(argv[2], g_sfnt, sfnt_len)) {
        return 1;
    }

    printf("decoded WOFF1 -> SFNT\n");
    printf("  tables: %u\n", (unsigned int)report.num_tables);
    printf("  WOFF bytes: %lu\n", (unsigned long)woff_len);
    printf("  SFNT bytes: %lu\n", (unsigned long)sfnt_len);
    printf("  checksumAdjustment: 0x%08lx\n", (unsigned long)report.repaired_checksum_adjustment);

    if (meta_out != 0) {
        r = woff1_decode_metadata_xml(g_woff, woff_len,
                                      g_extra, (woff1_u32)WOFF1_MAX_SFNT_SIZE,
                                      &extra_len, &report);
        if (r != WOFF1_OK) {
            fprintf(stderr, "metadata decode failed: %s\n", woff1_error_string(r));
            if (r == WOFF1_ERR_INFLATE) {
                fprintf(stderr, "inflate detail: %s\n", woff1_inflate_error_string(report.last_inflate_error));
            }
            return 1;
        }
        if (extra_len != 0u) {
            if (!write_file_fixed(meta_out, g_extra, extra_len)) {
                return 1;
            }
            printf("  metadata XML bytes: %lu\n", (unsigned long)extra_len);
        } else {
            printf("  metadata XML: none\n");
        }
    }

    if (priv_out != 0) {
        r = woff1_copy_private_data(g_woff, woff_len,
                                    g_extra, (woff1_u32)WOFF1_MAX_SFNT_SIZE,
                                    &extra_len, &report);
        if (r != WOFF1_OK) {
            fprintf(stderr, "private data copy failed: %s\n", woff1_error_string(r));
            return 1;
        }
        if (extra_len != 0u) {
            if (!write_file_fixed(priv_out, g_extra, extra_len)) {
                return 1;
            }
            printf("  private bytes: %lu\n", (unsigned long)extra_len);
        } else {
            printf("  private data: none\n");
        }
    }

    return 0;
}
