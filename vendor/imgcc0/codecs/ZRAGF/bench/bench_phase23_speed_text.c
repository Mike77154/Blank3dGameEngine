#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <zlib.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"
#include "protocol89_fixed.h"
#include "protocol89_stdio.h"

static unsigned char *make_jsonish(size_t *out_size)
{
    static const char *frag[] = {
        "{\"id\":",
        ",\"user\":\"miguel\",\"type\":\"event\",\"ok\":true,",
        "\"payload\":\"AAAAAAAAAAAAAAAA\",\"tags\":[\"alpha\",\"beta\",\"gamma\"]}\n"
    };
    unsigned char *buf;
    size_t i = 0u;
    *out_size = 180000u;
    buf = (unsigned char *)zragf_p89_host_take(*out_size);
    if (!buf) return NULL;
    while (i < *out_size) {
        char num[32];
        int len = zragf_p89_format(num, sizeof(num), "%lu", (unsigned long)i);
        size_t j, k;
        for (j = 0u; j < strlen(frag[0]) && i < *out_size; ++j) buf[i++] = (unsigned char)frag[0][j];
        for (j = 0u; j < (size_t)len && i < *out_size; ++j) buf[i++] = (unsigned char)num[j];
        for (j = 1u; j < 3u; ++j)
            for (k = 0u; k < strlen(frag[j]) && i < *out_size; ++k)
                buf[i++] = (unsigned char)frag[j][k];
    }
    return buf;
}

static unsigned char *make_logish(size_t *out_size)
{
    unsigned char *buf;
    size_t i = 0u;
    *out_size = 200000u;
    buf = (unsigned char *)zragf_p89_host_take(*out_size);
    if (!buf) return NULL;
    while (i < *out_size) {
        char line[256];
        int len = zragf_p89_format(line, sizeof(line),
                           "2026-03-09T12:%02lu:%02luZ INFO service=auth user=miguel action=login id=%08lu path=/api/v1/session status=200 latency_ms=%lu retry=0\n",
                           (unsigned long)((i / 97u) % 60u),
                           (unsigned long)((i / 53u) % 60u),
                           (unsigned long)i,
                           (unsigned long)((i / 37u) % 17u));
        int j;
        for (j = 0; j < len && i < *out_size; ++j)
            buf[i++] = (unsigned char)line[j];
    }
    return buf;
}

static size_t zragf_size_once(const unsigned char *src, size_t n, size_t chunk)
{
    zragf_stream zs;
    unsigned char *out;
    size_t cap = n * 2u + 1024u;
    size_t off = 0u;
    size_t out_len = 0u;
    int rc;
    out = (unsigned char *)zragf_p89_host_take(cap);
    if (!out) return 0u;
    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK) { zragf_p89_host_release(out); return 0u; }
    zs.next_out = out;
    zs.avail_out = cap;
    if (chunk == 0u) chunk = n;
    while (off < n) {
        size_t take = chunk;
        if (take > n - off) take = n - off;
        zs.next_in = (unsigned char *)(src + off);
        zs.avail_in = take;
        rc = zragf_deflateZ(&zs, ZRAGF_NO_FLUSH);
        if (rc != ZRAGF_OK) { zragf_deflateEndZ(&zs); zragf_p89_host_release(out); return 0u; }
        off += take;
    }
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    if (rc == ZRAGF_STREAM_END) out_len = zs.total_out;
    zragf_deflateEndZ(&zs);
    zragf_p89_host_release(out);
    return out_len;
}

static size_t zlib_size_once(const unsigned char *src, size_t n)
{
    z_stream zs;
    unsigned char *out;
    size_t cap = compressBound((uLong)n) + 64u;
    size_t out_len = 0u;
    int rc;
    out = (unsigned char *)zragf_p89_host_take(cap);
    if (!out) return 0u;
    memset(&zs, 0, sizeof(zs));
    rc = deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    if (rc != Z_OK) { zragf_p89_host_release(out); return 0u; }
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)n;
    zs.next_out = out;
    zs.avail_out = (uInt)cap;
    rc = deflate(&zs, Z_FINISH);
    if (rc == Z_STREAM_END) out_len = zs.total_out;
    deflateEnd(&zs);
    zragf_p89_host_release(out);
    return out_len;
}

static zragf_fx bench_ms(size_t (*fn)(const unsigned char *, size_t, size_t), const unsigned char *src, size_t n, size_t chunk, int iters, size_t *last)
{
    clock_t t0 = clock();
    size_t v = 0u;
    int i;
    for (i = 0; i < iters; ++i)
        v = fn(src, n, chunk);
    if (last) *last = v;
    return 1000 * (zragf_fx)(clock() - t0) / (zragf_fx)CLOCKS_PER_SEC;
}

static size_t zlib_size_bridge(const unsigned char *src, size_t n, size_t chunk)
{
    (void)chunk;
    return zlib_size_once(src, n);
}

int main(void)
{
    size_t json_n = 0u, log_n = 0u;
    size_t json_one = 0u, json_chunk = 0u, json_zlib = 0u;
    size_t log_one = 0u, log_chunk = 0u, log_zlib = 0u;
    zragf_fx json_ms_one, json_ms_chunk, json_ms_zlib;
    zragf_fx log_ms_one, log_ms_chunk, log_ms_zlib;
    unsigned char *jsonish = make_jsonish(&json_n);
    unsigned char *logish = make_logish(&log_n);
    if (!jsonish || !logish) return 1;

    json_ms_one = bench_ms(zragf_size_once, jsonish, json_n, 0u, 20, &json_one);
    json_ms_chunk = bench_ms(zragf_size_once, jsonish, json_n, 4096u, 20, &json_chunk);
    json_ms_zlib = bench_ms(zlib_size_bridge, jsonish, json_n, 0u, 20, &json_zlib);

    log_ms_one = bench_ms(zragf_size_once, logish, log_n, 0u, 20, &log_one);
    log_ms_chunk = bench_ms(zragf_size_once, logish, log_n, 4096u, 20, &log_chunk);
    log_ms_zlib = bench_ms(zlib_size_bridge, logish, log_n, 0u, 20, &log_zlib);

    printf("phase23 json raw:        zragf=%lu zlib=%lu delta=%ld ms_one=%d ms_chunk=%d ms_zlib=%d\n",
           (unsigned long)json_one, (unsigned long)json_zlib,
           (long)json_zlib - (long)json_one,
           json_ms_one, json_ms_chunk, json_ms_zlib);
    printf("phase23 log raw:         zragf=%lu zlib=%lu delta=%ld ms_one=%d ms_chunk=%d ms_zlib=%d\n",
           (unsigned long)log_one, (unsigned long)log_zlib,
           (long)log_zlib - (long)log_one,
           log_ms_one, log_ms_chunk, log_ms_zlib);
    printf("phase23 json chunk size: %lu\n", (unsigned long)json_chunk);
    printf("phase23 log chunk size:  %lu\n", (unsigned long)log_chunk);

    zragf_p89_host_release(jsonish);
    zragf_p89_host_release(logish);
    return 0;
}
