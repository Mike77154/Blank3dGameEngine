#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>

#include <zlib.h>
#include "zragflib.h"
#include "protocol89_hostmem.h"
#include "protocol89_fixed.h"
#include "protocol89_stdio.h"

#ifdef ZRAGF_HAVE_LIBDEFLATE_BENCH
#include <libdeflate.h>
#endif

#ifdef ZRAGF_HAVE_MINIZ_BENCH
#include "miniz.h"
#endif

#define ZRAGF_P68_MAX_DATASETS 32
#define ZRAGF_P68_MAX_LEVELS 8
#define ZRAGF_P68_MAX_ROWS 256
#define ZRAGF_P68_MB (1024 * 1024)

typedef struct zragf_p68_dataset_s {
    char name[64];
    char family[32];
    char source_kind[16];
    unsigned char *data;
    size_t size;
    int owned;
} zragf_p68_dataset;

typedef struct zragf_p68_row_s {
    char codec[16];
    char wrapper[8];
    int level;
    char dataset[64];
    char family[32];
    char source_kind[16];
    char size_class[16];
    size_t input_bytes;
    size_t compressed_bytes;
    zragf_fx ratio;
    zragf_fx compress_ms;
    zragf_fx decompress_ms;
    zragf_fx compress_mib_s;
    zragf_fx decompress_mib_s;
    long comp_mem_bytes;
    long decomp_mem_bytes;
    int ok;
    char note[64];
} zragf_p68_row;

typedef struct zragf_p68_agg_s {
    char codec[16];
    int level;
    size_t total_in;
    size_t total_out;
    zragf_fx total_compress_ms;
    zragf_fx total_decompress_ms;
    int rows;
} zragf_p68_agg;

static unsigned long zragf_p68_xorshift32(unsigned long *s)
{
    unsigned long x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

static void *zragf_p68_malloc(size_t n)
{
    if (n == 0u) n = 1u;
    return zragf_p89_host_take(n);
}

static char *zragf_p68_strdup(const char *s)
{
    size_t n;
    char *d;
    if (!s) return NULL;
    n = strlen(s) + 1u;
    d = (char *)zragf_p89_host_take(n);
    if (!d) return NULL;
    memcpy(d, s, n);
    return d;
}

static const char *zragf_p68_size_class(size_t n)
{
    if (n < 1024u) return "tiny";
    if (n < 65536u) return "small";
    if (n < 1048576u) return "medium";
    return "huge";
}

static int zragf_p68_add_dataset(zragf_p68_dataset *datasets, int *count,
                                 const char *name, const char *family,
                                 const char *source_kind,
                                 unsigned char *data, size_t size, int owned)
{
    zragf_p68_dataset *ds;
    if (*count >= ZRAGF_P68_MAX_DATASETS) return 0;
    ds = &datasets[*count];
    memset(ds, 0, sizeof(*ds));
    strncpy(ds->name, name, sizeof(ds->name) - 1u);
    strncpy(ds->family, family, sizeof(ds->family) - 1u);
    strncpy(ds->source_kind, source_kind, sizeof(ds->source_kind) - 1u);
    ds->data = data;
    ds->size = size;
    ds->owned = owned;
    *count += 1;
    return 1;
}

static unsigned char *zragf_p68_make_text(size_t n)
{
    static const char *chunk =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. "
        "Sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.\n";
    size_t chunk_len = strlen(chunk);
    size_t i;
    unsigned char *buf = (unsigned char *)zragf_p68_malloc(n);
    if (!buf) return NULL;
    for (i = 0u; i < n; ++i) buf[i] = (unsigned char)chunk[i % chunk_len];
    return buf;
}

static unsigned char *zragf_p68_make_json(size_t n)
{
    size_t i, pos;
    unsigned char *buf = (unsigned char *)zragf_p68_malloc(n);
    if (!buf) return NULL;
    pos = 0u;
    while (pos + 64u < n) {
        int idx = (int)(pos / 64u);
        pos += (size_t)zragf_p89_format((char *)buf + pos, n - pos,
                                "{\"id\":%d,\"name\":\"sprite_%d\",\"x\":%d,\"y\":%d,\"visible\":true}\n",
                                idx, idx % 17, idx % 1024, (idx * 3) % 2048);
    }
    for (i = pos; i < n; ++i) buf[i] = (unsigned char)('a' + (i % 26u));
    return buf;
}

static unsigned char *zragf_p68_make_logs(size_t n)
{
    size_t pos;
    unsigned char *buf = (unsigned char *)zragf_p68_malloc(n);
    if (!buf) return NULL;
    pos = 0u;
    while (pos + 96u < n) {
        int idx = (int)(pos / 96u);
        pos += (size_t)zragf_p89_format((char *)buf + pos, n - pos,
                                "2026-03-15T12:%02d:%02dZ INFO worker=%d path=/api/v1/items/%d status=200 bytes=%d\n",
                                idx % 60, (idx * 7) % 60, idx % 16, idx % 2048, 512 + (idx % 4096));
    }
    while (pos < n) buf[pos++] = '\n';
    return buf;
}

static unsigned char *zragf_p68_make_png_scanlines(size_t rows, size_t cols)
{
    size_t row, col, pos, stride;
    unsigned char *buf;
    stride = 1u + cols * 4u;
    buf = (unsigned char *)zragf_p68_malloc(rows * stride);
    if (!buf) return NULL;
    pos = 0u;
    for (row = 0u; row < rows; ++row) {
        buf[pos++] = (unsigned char)(row % 5u);
        for (col = 0u; col < cols; ++col) {
            buf[pos++] = (unsigned char)((col + row) & 0xFFu);
            buf[pos++] = (unsigned char)((row * 3u) & 0xFFu);
            buf[pos++] = (unsigned char)((col * 5u) & 0xFFu);
            buf[pos++] = (unsigned char)255u;
        }
    }
    return buf;
}

static unsigned char *zragf_p68_make_photoish(size_t n)
{
    size_t i;
    unsigned long st;
    unsigned char *buf = (unsigned char *)zragf_p68_malloc(n);
    if (!buf) return NULL;
    st = 0x12345678ul;
    for (i = 0u; i < n; ++i) {
        unsigned long noise = zragf_p68_xorshift32(&st) & 31ul;
        buf[i] = (unsigned char)(((i * 7u) + (i / 257u) + noise) & 0xFFu);
    }
    return buf;
}

static unsigned char *zragf_p68_make_normalmap(size_t pixels)
{
    size_t i, pos;
    unsigned char *buf = (unsigned char *)zragf_p68_malloc(pixels * 4u);
    if (!buf) return NULL;
    pos = 0u;
    for (i = 0u; i < pixels; ++i) {
        buf[pos++] = (unsigned char)(120u + (i % 16u));
        buf[pos++] = (unsigned char)(120u + ((i / 8u) % 16u));
        buf[pos++] = (unsigned char)(240u + (i % 8u));
        buf[pos++] = 255u;
    }
    return buf;
}

static unsigned char *zragf_p68_make_binary(size_t n)
{
    size_t i;
    unsigned long st;
    unsigned char *buf = (unsigned char *)zragf_p68_malloc(n);
    if (!buf) return NULL;
    st = 0xCAFEBABEul;
    for (i = 0u; i < n; ++i) buf[i] = (unsigned char)(zragf_p68_xorshift32(&st) & 0xFFu);
    return buf;
}

static int zragf_p68_add_builtins(zragf_p68_dataset *datasets, int *count, int quick)
{
    if (!zragf_p68_add_dataset(datasets, count, "text_small", "text", "synthetic", zragf_p68_make_text(32768u), 32768u, 1)) return 0;
    if (!zragf_p68_add_dataset(datasets, count, "json_medium", "json", "synthetic", zragf_p68_make_json(196608u), 196608u, 1)) return 0;
    if (!zragf_p68_add_dataset(datasets, count, "logs_medium", "logs", "synthetic", zragf_p68_make_logs(262144u), 262144u, 1)) return 0;
    if (quick) return 1;
    if (!zragf_p68_add_dataset(datasets, count, "png_scanlines", "png", "synthetic", zragf_p68_make_png_scanlines(512u, 64u), 512u * (1u + 64u * 4u), 1)) return 0;
    if (!zragf_p68_add_dataset(datasets, count, "photoish_medium", "photo", "synthetic", zragf_p68_make_photoish(262144u), 262144u, 1)) return 0;
    if (!zragf_p68_add_dataset(datasets, count, "normalmap_medium", "normalmap", "synthetic", zragf_p68_make_normalmap(65536u), 65536u * 4u, 1)) return 0;
    if (!zragf_p68_add_dataset(datasets, count, "binary_medium", "binary", "synthetic", zragf_p68_make_binary(262144u), 262144u, 1)) return 0;
    return 1;
}

static const char *zragf_p68_file_family(const char *path)
{
    const char *dot = strrchr(path, '.');
    if (!dot) return "file";
    if (strcmp(dot, ".png") == 0) return "png";
    if (strcmp(dot, ".tif") == 0 || strcmp(dot, ".tiff") == 0) return "tiff";
    if (strcmp(dot, ".json") == 0) return "json";
    if (strcmp(dot, ".log") == 0 || strcmp(dot, ".txt") == 0) return "text";
    if (strcmp(dot, ".bin") == 0 || strcmp(dot, ".dat") == 0) return "binary";
    return "file";
}

static unsigned char *zragf_p68_read_file(const char *path, size_t *size_ret)
{
    FILE *f;
    long sz;
    unsigned char *buf;
    size_t got;
    f = fopen(path, "rb");
    if (!f) return NULL;
    if (fseek(f, 0L, SEEK_END) != 0) { fclose(f); return NULL; }
    sz = ftell(f);
    if (sz < 0L) { fclose(f); return NULL; }
    if (fseek(f, 0L, SEEK_SET) != 0) { fclose(f); return NULL; }
    buf = (unsigned char *)zragf_p68_malloc((size_t)sz);
    if (!buf) { fclose(f); return NULL; }
    got = fread(buf, 1u, (size_t)sz, f);
    fclose(f);
    if (got != (size_t)sz) { zragf_p89_host_release(buf); return NULL; }
    *size_ret = (size_t)sz;
    return buf;
}

static const char *zragf_p68_basename(const char *path)
{
    const char *p1 = strrchr(path, '/');
    const char *p2 = strrchr(path, '\\');
    const char *p = p1;
    if (p2 && (!p || p2 > p)) p = p2;
    return p ? p + 1 : path;
}

static int zragf_p68_add_files(zragf_p68_dataset *datasets, int *count, int argc, char **argv, int first)
{
    int i;
    for (i = first; i < argc; ++i) {
        size_t n = 0u;
        unsigned char *buf = zragf_p68_read_file(argv[i], &n);
        if (!buf) return 0;
        if (!zragf_p68_add_dataset(datasets, count, zragf_p68_basename(argv[i]),
                                   zragf_p68_file_family(argv[i]), "file", buf, n, 1)) {
            zragf_p89_host_release(buf);
            return 0;
        }
    }
    return 1;
}

static zragf_fx zragf_p68_ms(clock_t ticks)
{
    return zragf_fx_millis(ticks);
}

static int zragf_p68_split_levels(const char *arg, int *levels, int *count)
{
    char *copy;
    char *tok;
    char *save;
    int n;
    copy = zragf_p68_strdup(arg);
    if (!copy) return 0;
    n = 0;
    save = copy;
    while (save && *save) {
        char *comma = strchr(save, ',');
        if (comma) *comma = '\0';
        tok = save;
        if (*tok) {
            if (n >= ZRAGF_P68_MAX_LEVELS) { zragf_p89_host_release(copy); return 0; }
            levels[n++] = atoi(tok);
        }
        if (!comma) break;
        save = comma + 1;
    }
    zragf_p89_host_release(copy);
    *count = n;
    return n > 0;
}

static int zragf_p68_bench_zragf(const unsigned char *src, size_t n, int level,
                                 int warmup, int repeat, zragf_p68_row *row)
{
    unsigned char *comp;
    unsigned char *decomp;
    unsigned long cap;
    unsigned long comp_len;
    unsigned long out_len;
    int i, rc;
    clock_t t0, t1;
    zragf_fx c_ms, d_ms;

    cap = zragf_compressBound((unsigned long)n);
    comp = (unsigned char *)zragf_p68_malloc((size_t)cap);
    decomp = (unsigned char *)zragf_p68_malloc((n + 1u) ? (n + 1u) : 1u);
    if (!comp || !decomp) {
        zragf_p89_host_release(comp); zragf_p89_host_release(decomp);
        strcpy(row->note, "alloc failed");
        return 0;
    }

    for (i = 0; i < warmup; ++i) {
        comp_len = cap;
        rc = zragf_compress2(comp, &comp_len, src, (unsigned long)n, level);
        if (rc != ZRAGF_OK) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "compress rc=%d", rc); return 0; }
    }

    t0 = clock();
    for (i = 0; i < repeat; ++i) {
        comp_len = cap;
        rc = zragf_compress2(comp, &comp_len, src, (unsigned long)n, level);
        if (rc != ZRAGF_OK) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "compress rc=%d", rc); return 0; }
    }
    t1 = clock();
    c_ms = zragf_fx_avg_nonzero(zragf_p68_ms(t1 - t0), repeat);

    for (i = 0; i < warmup; ++i) {
        out_len = (unsigned long)(n + 1u);
        rc = zragf_uncompress(decomp, &out_len, comp, comp_len);
        if (rc != ZRAGF_OK) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "decompress rc=%d", rc); return 0; }
    }

    t0 = clock();
    for (i = 0; i < repeat; ++i) {
        out_len = (unsigned long)(n + 1u);
        rc = zragf_uncompress(decomp, &out_len, comp, comp_len);
        if (rc != ZRAGF_OK) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "decompress rc=%d", rc); return 0; }
    }
    t1 = clock();
    d_ms = zragf_fx_avg_nonzero(zragf_p68_ms(t1 - t0), repeat);

    if (memcmp(src, decomp, n) != 0) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); strcpy(row->note, "roundtrip mismatch"); return 0; }

    row->compressed_bytes = (size_t)comp_len;
    row->ratio = zragf_fx_ratio_1000((zragf_size_t)comp_len, (zragf_size_t)n);
    row->compress_ms = c_ms;
    row->decompress_ms = d_ms;
    row->compress_mib_s = zragf_fx_mibs_1000((zragf_size_t)n, c_ms);
    row->decompress_mib_s = zragf_fx_mibs_1000((zragf_size_t)n, d_ms);
    row->comp_mem_bytes = (long)zragf_deflate_workspace_bound_z((unsigned long)n, 15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    row->decomp_mem_bytes = (long)zragf_inflate_workspace_bound_z((unsigned long)n, 15);
    row->ok = 1;
    strcpy(row->note, "ok");
    zragf_p89_host_release(comp);
    zragf_p89_host_release(decomp);
    return 1;
}

static int zragf_p68_bench_zlib(const unsigned char *src, size_t n, int level,
                                int warmup, int repeat, zragf_p68_row *row)
{
    unsigned char *comp;
    unsigned char *decomp;
    uLongf cap;
    uLongf comp_len;
    uLongf out_len;
    int i, rc;
    clock_t t0, t1;
    zragf_fx c_ms, d_ms;

    cap = compressBound((uLong)n);
    comp = (unsigned char *)zragf_p68_malloc((size_t)cap);
    decomp = (unsigned char *)zragf_p68_malloc(n ? n : 1u);
    if (!comp || !decomp) {
        zragf_p89_host_release(comp); zragf_p89_host_release(decomp);
        strcpy(row->note, "alloc failed");
        return 0;
    }

    for (i = 0; i < warmup; ++i) {
        comp_len = cap;
        rc = compress2((Bytef *)comp, &comp_len, (const Bytef *)src, (uLong)n, level);
        if (rc != Z_OK) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "compress rc=%d", rc); return 0; }
    }

    t0 = clock();
    for (i = 0; i < repeat; ++i) {
        comp_len = cap;
        rc = compress2((Bytef *)comp, &comp_len, (const Bytef *)src, (uLong)n, level);
        if (rc != Z_OK) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "compress rc=%d", rc); return 0; }
    }
    t1 = clock();
    c_ms = zragf_fx_avg_nonzero(zragf_p68_ms(t1 - t0), repeat);

    for (i = 0; i < warmup; ++i) {
        out_len = (uLongf)n;
        rc = uncompress((Bytef *)decomp, &out_len, (const Bytef *)comp, comp_len);
        if (rc != Z_OK) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "decompress rc=%d", rc); return 0; }
    }

    t0 = clock();
    for (i = 0; i < repeat; ++i) {
        out_len = (uLongf)n;
        rc = uncompress((Bytef *)decomp, &out_len, (const Bytef *)comp, comp_len);
        if (rc != Z_OK) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "decompress rc=%d", rc); return 0; }
    }
    t1 = clock();
    d_ms = zragf_fx_avg_nonzero(zragf_p68_ms(t1 - t0), repeat);

    if (memcmp(src, decomp, n) != 0) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); strcpy(row->note, "roundtrip mismatch"); return 0; }

    row->compressed_bytes = (size_t)comp_len;
    row->ratio = zragf_fx_ratio_1000((zragf_size_t)comp_len, (zragf_size_t)n);
    row->compress_ms = c_ms;
    row->decompress_ms = d_ms;
    row->compress_mib_s = zragf_fx_mibs_1000((zragf_size_t)n, c_ms);
    row->decompress_mib_s = zragf_fx_mibs_1000((zragf_size_t)n, d_ms);
    row->comp_mem_bytes = -1;
    row->decomp_mem_bytes = -1;
    row->ok = 1;
    strcpy(row->note, "ok");
    zragf_p89_host_release(comp);
    zragf_p89_host_release(decomp);
    return 1;
}

#ifdef ZRAGF_HAVE_LIBDEFLATE_BENCH
static int zragf_p68_bench_libdeflate(const unsigned char *src, size_t n, int level,
                                      int warmup, int repeat, zragf_p68_row *row)
{
    struct libdeflate_compressor *comp_state;
    struct libdeflate_decompressor *decomp_state;
    unsigned char *comp;
    unsigned char *decomp;
    size_t cap;
    size_t comp_len;
    size_t out_len;
    int i;
    clock_t t0, t1;
    zragf_fx c_ms, d_ms;
    enum libdeflate_result dr;

    comp_state = libdeflate_alloc_compressor(level);
    decomp_state = libdeflate_alloc_decompressor();
    if (!comp_state || !decomp_state) {
        if (comp_state) libdeflate_free_compressor(comp_state);
        if (decomp_state) libdeflate_free_decompressor(decomp_state);
        strcpy(row->note, "alloc state failed");
        return 0;
    }
    cap = libdeflate_zlib_compress_bound(comp_state, n);
    comp = (unsigned char *)zragf_p68_malloc(cap);
    decomp = (unsigned char *)zragf_p68_malloc(n ? n : 1u);
    if (!comp || !decomp) {
        zragf_p89_host_release(comp); zragf_p89_host_release(decomp);
        libdeflate_free_compressor(comp_state);
        libdeflate_free_decompressor(decomp_state);
        strcpy(row->note, "alloc failed");
        return 0;
    }

    for (i = 0; i < warmup; ++i) {
        comp_len = libdeflate_zlib_compress(comp_state, src, n, comp, cap);
        if (comp_len == 0u) { strcpy(row->note, "compress failed"); goto fail; }
    }

    t0 = clock();
    for (i = 0; i < repeat; ++i) {
        comp_len = libdeflate_zlib_compress(comp_state, src, n, comp, cap);
        if (comp_len == 0u) { strcpy(row->note, "compress failed"); goto fail; }
    }
    t1 = clock();
    c_ms = zragf_fx_avg_nonzero(zragf_p68_ms(t1 - t0), repeat);

    for (i = 0; i < warmup; ++i) {
        out_len = 0u;
        dr = libdeflate_zlib_decompress(decomp_state, comp, comp_len, decomp, n, &out_len);
        if (dr != LIBDEFLATE_SUCCESS || out_len != n) { zragf_p89_format(row->note, sizeof(row->note), "decompress rc=%d", (int)dr); goto fail; }
    }

    t0 = clock();
    for (i = 0; i < repeat; ++i) {
        out_len = 0u;
        dr = libdeflate_zlib_decompress(decomp_state, comp, comp_len, decomp, n, &out_len);
        if (dr != LIBDEFLATE_SUCCESS || out_len != n) { zragf_p89_format(row->note, sizeof(row->note), "decompress rc=%d", (int)dr); goto fail; }
    }
    t1 = clock();
    d_ms = zragf_fx_avg_nonzero(zragf_p68_ms(t1 - t0), repeat);

    if (memcmp(src, decomp, n) != 0) { strcpy(row->note, "roundtrip mismatch"); goto fail; }

    row->compressed_bytes = comp_len;
    row->ratio = zragf_fx_ratio_1000((zragf_size_t)comp_len, (zragf_size_t)n);
    row->compress_ms = c_ms;
    row->decompress_ms = d_ms;
    row->compress_mib_s = zragf_fx_mibs_1000((zragf_size_t)n, c_ms);
    row->decompress_mib_s = zragf_fx_mibs_1000((zragf_size_t)n, d_ms);
    row->comp_mem_bytes = -1;
    row->decomp_mem_bytes = -1;
    row->ok = 1;
    strcpy(row->note, "ok");
    zragf_p89_host_release(comp);
    zragf_p89_host_release(decomp);
    libdeflate_free_compressor(comp_state);
    libdeflate_free_decompressor(decomp_state);
    return 1;
fail:
    zragf_p89_host_release(comp);
    zragf_p89_host_release(decomp);
    libdeflate_free_compressor(comp_state);
    libdeflate_free_decompressor(decomp_state);
    return 0;
}
#endif

#ifdef ZRAGF_HAVE_MINIZ_BENCH
static int zragf_p68_bench_miniz(const unsigned char *src, size_t n, int level,
                                 int warmup, int repeat, zragf_p68_row *row)
{
    unsigned char *comp;
    unsigned char *decomp;
    mz_ulong cap;
    mz_ulong comp_len;
    mz_ulong out_len;
    int i, rc;
    clock_t t0, t1;
    zragf_fx c_ms, d_ms;

    cap = mz_compressBound((mz_ulong)n);
    comp = (unsigned char *)zragf_p68_malloc((size_t)cap);
    decomp = (unsigned char *)zragf_p68_malloc(n ? n : 1u);
    if (!comp || !decomp) {
        zragf_p89_host_release(comp); zragf_p89_host_release(decomp);
        strcpy(row->note, "alloc failed");
        return 0;
    }

    for (i = 0; i < warmup; ++i) {
        comp_len = cap;
        rc = mz_compress2(comp, &comp_len, src, (mz_ulong)n, level);
        if (rc != MZ_OK) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "compress rc=%d", rc); return 0; }
    }

    t0 = clock();
    for (i = 0; i < repeat; ++i) {
        comp_len = cap;
        rc = mz_compress2(comp, &comp_len, src, (mz_ulong)n, level);
        if (rc != MZ_OK) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "compress rc=%d", rc); return 0; }
    }
    t1 = clock();
    c_ms = zragf_fx_avg_nonzero(zragf_p68_ms(t1 - t0), repeat);

    for (i = 0; i < warmup; ++i) {
        out_len = (mz_ulong)n;
        rc = mz_uncompress(decomp, &out_len, comp, comp_len);
        if (rc != MZ_OK || out_len != (mz_ulong)n) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "decompress rc=%d", rc); return 0; }
    }

    t0 = clock();
    for (i = 0; i < repeat; ++i) {
        out_len = (mz_ulong)n;
        rc = mz_uncompress(decomp, &out_len, comp, comp_len);
        if (rc != MZ_OK || out_len != (mz_ulong)n) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); zragf_p89_format(row->note, sizeof(row->note), "decompress rc=%d", rc); return 0; }
    }
    t1 = clock();
    d_ms = zragf_fx_avg_nonzero(zragf_p68_ms(t1 - t0), repeat);

    if (memcmp(src, decomp, n) != 0) { zragf_p89_host_release(comp); zragf_p89_host_release(decomp); strcpy(row->note, "roundtrip mismatch"); return 0; }

    row->compressed_bytes = (size_t)comp_len;
    row->ratio = zragf_fx_ratio_1000((zragf_size_t)comp_len, (zragf_size_t)n);
    row->compress_ms = c_ms;
    row->decompress_ms = d_ms;
    row->compress_mib_s = zragf_fx_mibs_1000((zragf_size_t)n, c_ms);
    row->decompress_mib_s = zragf_fx_mibs_1000((zragf_size_t)n, d_ms);
    row->comp_mem_bytes = -1;
    row->decomp_mem_bytes = -1;
    row->ok = 1;
    strcpy(row->note, "ok");
    zragf_p89_host_release(comp);
    zragf_p89_host_release(decomp);
    return 1;
}
#endif

static void zragf_p68_row_init(zragf_p68_row *row, const char *codec, int level,
                               const zragf_p68_dataset *ds)
{
    memset(row, 0, sizeof(*row));
    strncpy(row->codec, codec, sizeof(row->codec) - 1u);
    strncpy(row->wrapper, "zlib", sizeof(row->wrapper) - 1u);
    row->level = level;
    strncpy(row->dataset, ds->name, sizeof(row->dataset) - 1u);
    strncpy(row->family, ds->family, sizeof(row->family) - 1u);
    strncpy(row->source_kind, ds->source_kind, sizeof(row->source_kind) - 1u);
    strncpy(row->size_class, zragf_p68_size_class(ds->size), sizeof(row->size_class) - 1u);
    row->input_bytes = ds->size;
    row->compressed_bytes = 0u;
    row->ratio = 0;
    row->compress_ms = 0;
    row->decompress_ms = 0;
    row->compress_mib_s = 0;
    row->decompress_mib_s = 0;
    row->comp_mem_bytes = -1;
    row->decomp_mem_bytes = -1;
    row->ok = 0;
    strcpy(row->note, "not-run");
}

static int zragf_p68_write_csv(const char *path, const zragf_p68_row *rows, int row_count)
{
    FILE *f;
    int i;
    f = fopen(path, "wb");
    if (!f) return 0;
    fprintf(f, "codec,wrapper,level,dataset,source_kind,family,size_class,input_bytes,compressed_bytes,ratio,compress_ms,decompress_ms,compress_mib_s,decompress_mib_s,comp_mem_bytes,decomp_mem_bytes,ok,note\n");
    for (i = 0; i < row_count; ++i) {
        const zragf_p68_row *r = &rows[i];
        fprintf(f, "%s,%s,%d,%s,%s,%s,%s,%lu,%lu,%d,%d,%d,%d,%d,%ld,%ld,%d,%s\n",
                r->codec, r->wrapper, r->level, r->dataset, r->source_kind, r->family,
                r->size_class, (unsigned long)r->input_bytes, (unsigned long)r->compressed_bytes,
                r->ratio, r->compress_ms, r->decompress_ms, r->compress_mib_s,
                r->decompress_mib_s, r->comp_mem_bytes, r->decomp_mem_bytes, r->ok, r->note);
    }
    fclose(f);
    return 1;
}

static void zragf_p68_cpu_string(char *buf, size_t n)
{
    FILE *f;
    char line[256];
    if (!buf || n == 0u) return;
    buf[0] = '\0';
    f = fopen("/proc/cpuinfo", "rb");
    if (!f) {
        strncpy(buf, "unknown", n - 1u);
        buf[n - 1u] = '\0';
        return;
    }
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "model name", 10) == 0 || strncmp(line, "Hardware", 8) == 0) {
            char *colon = strchr(line, ':');
            if (colon) {
                char *s = colon + 1;
                while (*s == ' ' || *s == '\t') ++s;
                strncpy(buf, s, n - 1u);
                buf[n - 1u] = '\0';
                s = strchr(buf, '\n');
                if (s) *s = '\0';
                fclose(f);
                return;
            }
        }
    }
    fclose(f);
    strncpy(buf, "unknown", n - 1u);
    buf[n - 1u] = '\0';
}

static void zragf_p68_add_agg(zragf_p68_agg *aggs, int *count, const zragf_p68_row *row)
{
    int i;
    for (i = 0; i < *count; ++i) {
        if (aggs[i].level == row->level && strcmp(aggs[i].codec, row->codec) == 0) {
            aggs[i].total_in += row->input_bytes;
            aggs[i].total_out += row->compressed_bytes;
            aggs[i].total_compress_ms += row->compress_ms;
            aggs[i].total_decompress_ms += row->decompress_ms;
            aggs[i].rows += 1;
            return;
        }
    }
    if (*count >= 16) return;
    memset(&aggs[*count], 0, sizeof(aggs[*count]));
    strncpy(aggs[*count].codec, row->codec, sizeof(aggs[*count].codec) - 1u);
    aggs[*count].level = row->level;
    aggs[*count].total_in = row->input_bytes;
    aggs[*count].total_out = row->compressed_bytes;
    aggs[*count].total_compress_ms = row->compress_ms;
    aggs[*count].total_decompress_ms = row->decompress_ms;
    aggs[*count].rows = 1;
    *count += 1;
}

static int zragf_p68_write_summary(const char *path, const zragf_p68_row *rows, int row_count,
                                   int repeat, int warmup, int builtins, int file_count)
{
    FILE *f;
    int i;
    zragf_p68_agg aggs[16];
    int agg_count;
    char cpu[128];
    f = fopen(path, "wb");
    if (!f) return 0;
    zragf_p68_cpu_string(cpu, sizeof(cpu));
    memset(aggs, 0, sizeof(aggs));
    agg_count = 0;
    for (i = 0; i < row_count; ++i) {
        if (rows[i].ok) zragf_p68_add_agg(aggs, &agg_count, &rows[i]);
    }
    fprintf(f, "# ZRAGFLIB public benchmark suite (phase 68)\n\n");
    fprintf(f, "- zragf_version: `%s`\n", zragf_version());
    fprintf(f, "- zragf_build_config: `%s`\n", zragf_build_config());
    fprintf(f, "- zlib_runtime: `%s`\n", zlibVersion());
#ifdef ZRAGF_HAVE_LIBDEFLATE_BENCH
    fprintf(f, "- libdeflate: available\n");
#else
    fprintf(f, "- libdeflate: unavailable\n");
#endif
#ifdef ZRAGF_HAVE_MINIZ_BENCH
    fprintf(f, "- miniz: available\n");
#else
    fprintf(f, "- miniz: unavailable (set ZRAGF_MINIZ_ROOT or vendor third_party/miniz)\n");
#endif
    fprintf(f, "- compiler: `%s`\n", 
#ifdef __VERSION__
            __VERSION__
#else
            "unknown"
#endif
            );
    fprintf(f, "- cpu: `%s`\n", cpu);
    fprintf(f, "- repeats: `%d`\n", repeat);
    fprintf(f, "- warmups: `%d`\n", warmup);
    fprintf(f, "- builtins_used: `%s`\n", builtins ? "yes" : "no");
    fprintf(f, "- external_files: `%d`\n\n", file_count);
    fprintf(f, "## Aggregate results\n\n");
    fprintf(f, "| codec | level | total input | total output | weighted ratio | compress MiB/s | decompress MiB/s | rows |\n");
    fprintf(f, "|---|---:|---:|---:|---:|---:|---:|---:|\n");
    for (i = 0; i < agg_count; ++i) {
        zragf_fx cr = zragf_fx_ratio_1000((zragf_size_t)aggs[i].total_out, (zragf_size_t)aggs[i].total_in);
        zragf_fx csp = zragf_fx_mibs_1000((zragf_size_t)aggs[i].total_in, aggs[i].total_compress_ms);
        zragf_fx dsp = zragf_fx_mibs_1000((zragf_size_t)aggs[i].total_in, aggs[i].total_decompress_ms);
        fprintf(f, "| %s | %d | %lu | %lu | %d | %d | %d | %d |\n",
                aggs[i].codec, aggs[i].level, (unsigned long)aggs[i].total_in,
                (unsigned long)aggs[i].total_out, cr, csp, dsp, aggs[i].rows);
    }
    fprintf(f, "\n## Notes\n\n");
    fprintf(f, "- This suite compares one-shot **zlib-wrapper** compression because it is the cleanest common surface across zragflib, zlib, libdeflate, and optional miniz.\n");
    fprintf(f, "- zragflib reports wrapper workspace bounds; external libraries are left as `-1` when not known by a stable public formula in this suite.\n");
    fprintf(f, "- Synthetic builtins are deterministic. You can also pass external files on the command line for real corpora.\n");
    fclose(f);
    return 1;
}

static void zragf_p68_free_datasets(zragf_p68_dataset *datasets, int count)
{
    int i;
    for (i = 0; i < count; ++i) {
        if (datasets[i].owned) zragf_p89_host_release(datasets[i].data);
    }
}

int main(int argc, char **argv)
{
    zragf_p68_dataset datasets[ZRAGF_P68_MAX_DATASETS];
    zragf_p68_row rows[ZRAGF_P68_MAX_ROWS];
    int dataset_count;
    int row_count;
    int levels[ZRAGF_P68_MAX_LEVELS];
    int level_count;
    int repeat;
    int warmup;
    int use_builtins;
    int quick;
    const char *csv_path;
    const char *summary_path;
    int file_arg_start;
    int i, j;

    memset(datasets, 0, sizeof(datasets));
    memset(rows, 0, sizeof(rows));
    dataset_count = 0;
    row_count = 0;
    levels[0] = 1; levels[1] = 6; levels[2] = 9; level_count = 3;
    repeat = 3;
    warmup = 1;
    use_builtins = 1;
    quick = 0;
    csv_path = "phase68_public.csv";
    summary_path = "phase68_public.md";
    file_arg_start = argc;

    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--csv") == 0 && i + 1 < argc) {
            csv_path = argv[++i];
        } else if (strcmp(argv[i], "--summary") == 0 && i + 1 < argc) {
            summary_path = argv[++i];
        } else if (strcmp(argv[i], "--repeat") == 0 && i + 1 < argc) {
            repeat = atoi(argv[++i]);
            if (repeat < 1) repeat = 1;
        } else if (strcmp(argv[i], "--warmup") == 0 && i + 1 < argc) {
            warmup = atoi(argv[++i]);
            if (warmup < 0) warmup = 0;
        } else if (strcmp(argv[i], "--quick") == 0) {
            quick = 1;
            repeat = 1;
            warmup = 0;
        } else if (strcmp(argv[i], "--levels") == 0 && i + 1 < argc) {
            if (!zragf_p68_split_levels(argv[++i], levels, &level_count)) {
                fprintf(stderr, "invalid --levels\n");
                return 2;
            }
        } else if (strcmp(argv[i], "--no-builtins") == 0) {
            use_builtins = 0;
        } else {
            file_arg_start = i;
            break;
        }
    }

    if (use_builtins) {
        if (!zragf_p68_add_builtins(datasets, &dataset_count, quick)) {
            fprintf(stderr, "failed to build builtin corpus\n");
            zragf_p68_free_datasets(datasets, dataset_count);
            return 1;
        }
    }
    if (file_arg_start < argc) {
        if (!zragf_p68_add_files(datasets, &dataset_count, argc, argv, file_arg_start)) {
            fprintf(stderr, "failed to read corpus file\n");
            zragf_p68_free_datasets(datasets, dataset_count);
            return 1;
        }
    }
    if (dataset_count == 0) {
        fprintf(stderr, "no datasets\n");
        return 2;
    }

    for (i = 0; i < dataset_count; ++i) {
        for (j = 0; j < level_count; ++j) {
            if (row_count >= ZRAGF_P68_MAX_ROWS) break;
            zragf_p68_row_init(&rows[row_count], "zragf", levels[j], &datasets[i]);
            zragf_p68_bench_zragf(datasets[i].data, datasets[i].size, levels[j], warmup, repeat, &rows[row_count]);
            row_count += 1;
            if (row_count >= ZRAGF_P68_MAX_ROWS) break;
            zragf_p68_row_init(&rows[row_count], "zlib", levels[j], &datasets[i]);
            zragf_p68_bench_zlib(datasets[i].data, datasets[i].size, levels[j], warmup, repeat, &rows[row_count]);
            row_count += 1;
#ifdef ZRAGF_HAVE_LIBDEFLATE_BENCH
            if (row_count >= ZRAGF_P68_MAX_ROWS) break;
            zragf_p68_row_init(&rows[row_count], "libdeflate", levels[j], &datasets[i]);
            zragf_p68_bench_libdeflate(datasets[i].data, datasets[i].size, levels[j], warmup, repeat, &rows[row_count]);
            row_count += 1;
#endif
#ifdef ZRAGF_HAVE_MINIZ_BENCH
            if (row_count >= ZRAGF_P68_MAX_ROWS) break;
            zragf_p68_row_init(&rows[row_count], "miniz", levels[j], &datasets[i]);
            zragf_p68_bench_miniz(datasets[i].data, datasets[i].size, levels[j], warmup, repeat, &rows[row_count]);
            row_count += 1;
#endif
        }
    }

    if (!zragf_p68_write_csv(csv_path, rows, row_count)) {
        fprintf(stderr, "failed to write csv\n");
        zragf_p68_free_datasets(datasets, dataset_count);
        return 1;
    }
    if (!zragf_p68_write_summary(summary_path, rows, row_count, repeat, warmup, use_builtins,
                                 file_arg_start < argc ? argc - file_arg_start : 0)) {
        fprintf(stderr, "failed to write summary\n");
        zragf_p68_free_datasets(datasets, dataset_count);
        return 1;
    }

    printf("phase68 public bench rows=%d csv=%s summary=%s\n", row_count, csv_path, summary_path);
    zragf_p68_free_datasets(datasets, dataset_count);
    return 0;
}
