#include <stdio.h>
#include "w2f.h"

static W2F_U8 g_woff2[W2F_MAX_WOFF2_BYTES];
static W2F_U8 g_scratch[W2F_MAX_DECOMPRESSED_TABLE_BYTES];
static W2F_U8 g_sfnt[W2F_MAX_SFNT_BYTES];

static int read_all(const char *path, W2F_U8 *buf, W2F_U32 cap, W2F_U32 *len)
{
    FILE *f;
    size_t n;
    if (path == 0 || buf == 0 || len == 0) return 0;
    *len = 0u;
    f = fopen(path, "rb");
    if (f == 0) return 0;
    n = fread(buf, 1u, (size_t)cap, f);
    if (ferror(f)) { fclose(f); return 0; }
    if (!feof(f)) { fclose(f); return 0; }
    fclose(f);
    *len = (W2F_U32)n;
    return 1;
}

static int write_all(const char *path, const W2F_U8 *buf, W2F_U32 len)
{
    FILE *f;
    size_t n;
    if (path == 0 || buf == 0) return 0;
    f = fopen(path, "wb");
    if (f == 0) return 0;
    n = fwrite(buf, 1u, (size_t)len, f);
    if (n != (size_t)len) { fclose(f); return 0; }
    if (fclose(f) != 0) return 0;
    return 1;
}

int main(int argc, char **argv)
{
    W2F_U32 in_len;
    W2F_U32 out_len;
    W2F_DecoderInfo info;
    int rc;
    W2F_BrotliDecodeFn brotli_fn;

    if (argc != 3) {
        printf("usage: w2f_cli_static input.woff2 output.ttf_or_otf\n");
        printf("note: compile with -DW2F_USE_GOOGLE_BROTLI and link -lbrotlidec -lbrotlicommon,\n");
        printf("      or provide your own W2F_BrotliDecodeFn implementation.\n");
        return 2;
    }

    if (!read_all(argv[1], g_woff2, (W2F_U32)W2F_MAX_WOFF2_BYTES, &in_len)) {
        printf("read failed or file too large\n");
        return 3;
    }

#ifdef W2F_USE_GOOGLE_BROTLI
    brotli_fn = w2f_brotli_google_static_decode;
#else
    brotli_fn = w2f_brotli_stub_decode;
#endif

    rc = w2f_decode_woff2_to_sfnt(g_woff2, in_len,
                                  g_sfnt, (W2F_U32)W2F_MAX_SFNT_BYTES, &out_len,
                                  g_scratch, (W2F_U32)W2F_MAX_DECOMPRESSED_TABLE_BYTES,
                                  brotli_fn,
                                  &info);
    if (rc != W2F_OK) {
        printf("decode failed: %s (%d)\n", w2f_error_name(rc), rc);
        if (rc == W2F_ERR_UNSUPPORTED_TRANSFORM) {
            printf("this build supports only null-transformed WOFF2 tables.\n");
        }
        return 4;
    }

    if (!write_all(argv[2], g_sfnt, out_len)) {
        printf("write failed\n");
        return 5;
    }

    printf("ok: %lu bytes -> %lu bytes, tables=%u\n",
           (unsigned long)in_len, (unsigned long)out_len, (unsigned)info.num_tables);
    return 0;
}
