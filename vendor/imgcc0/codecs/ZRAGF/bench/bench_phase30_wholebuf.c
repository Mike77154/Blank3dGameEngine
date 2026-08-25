#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>
#include "zragflib.h"
#include "protocol89_hostmem.h"
#include "protocol89_fixed.h"
#include "protocol89_stdio.h"

static unsigned xorshift32(unsigned *s)
{
    unsigned x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

static unsigned char *make_case(const char *name, size_t *out_size)
{
    unsigned char *buf;
    size_t i;
    if (strcmp(name, "mixed") == 0) {
        unsigned st = 0x12345678u;
        *out_size = 134304u;
        buf = (unsigned char *)zragf_p89_host_take(*out_size);
        if (!buf) return NULL;
        for (i = 0u; i < 32768u; ++i) buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
        for (; i < 65536u; ++i) buf[i] = (unsigned char)("ABCD"[(i - 32768u) & 3u]);
        for (; i < 98304u; ++i) buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
        for (; i < *out_size; ++i) buf[i] = (unsigned char)("hello world "[(i - 98304u) % 12u]);
        return buf;
    }
    if (strcmp(name, "jsonish") == 0) {
        static const char *frag[] = {
            "{\"id\":", ",\"user\":\"miguel\",\"type\":\"event\",\"ok\":true,",
            "\"payload\":\"AAAAAAAAAAAAAAAA\",\"tags\":[\"alpha\",\"beta\",\"gamma\"]}\n"
        };
        *out_size = 180000u;
        buf = (unsigned char *)zragf_p89_host_take(*out_size);
        if (!buf) return NULL;
        i = 0u;
        while (i < *out_size) {
            char num[32];
            int len = zragf_p89_format(num, sizeof(num), "%lu", (unsigned long)i);
            size_t j;
            for (j = 0u; j < strlen(frag[0]) && i < *out_size; ++j) buf[i++] = (unsigned char)frag[0][j];
            for (j = 0u; j < (size_t)len && i < *out_size; ++j) buf[i++] = (unsigned char)num[j];
            for (j = 1u; j < 3u; ++j) {
                size_t k;
                for (k = 0u; k < strlen(frag[j]) && i < *out_size; ++k) buf[i++] = (unsigned char)frag[j][k];
            }
        }
        return buf;
    }
    if (strcmp(name, "logish") == 0) {
        *out_size = 200000u;
        buf = (unsigned char *)zragf_p89_host_take(*out_size);
        if (!buf) return NULL;
        i = 0u;
        while (i < *out_size) {
            char line[256];
            int len = zragf_p89_format(line, sizeof(line),
                               "2026-03-09T12:%02lu:%02luZ INFO service=auth user=miguel action=login id=%08lu path=/api/v1/session status=200 latency_ms=%lu retry=0\n",
                               (unsigned long)((i / 97u) % 60u),
                               (unsigned long)((i / 53u) % 60u),
                               (unsigned long)i,
                               (unsigned long)((i / 37u) % 17u));
            int j;
            for (j = 0; j < len && i < *out_size; ++j) buf[i++] = (unsigned char)line[j];
        }
        return buf;
    }
    return NULL;
}

static size_t zragf_size_once(const unsigned char *src, size_t n)
{
    zragf_stream zs;
    unsigned char *out;
    size_t cap = n * 2u + 1024u;
    size_t out_len = 0u;
    int rc;
    out = (unsigned char *)zragf_p89_host_take(cap);
    if (!out) return 0u;
    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK) { zragf_p89_host_release(out); return 0u; }
    zs.next_in = (unsigned char *)src;
    zs.avail_in = n;
    zs.next_out = out;
    zs.avail_out = cap;
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

static zragf_fx bench_ms(size_t (*fn)(const unsigned char *, size_t), const unsigned char *src, size_t n, int iters, size_t *last)
{
    clock_t t0 = clock();
    size_t v = 0u;
    int i;
    for (i = 0; i < iters; ++i) v = fn(src, n);
    if (last) *last = v;
    return 1000 * (zragf_fx)(clock() - t0) / (zragf_fx)CLOCKS_PER_SEC;
}

int main(void)
{
    const char *cases[] = {"mixed", "jsonish", "logish"};
    size_t i;
    for (i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        size_t n = 0u, out_zragf = 0u, out_zlib = 0u;
        zragf_fx ms_zragf, ms_zlib;
        unsigned char *buf = make_case(cases[i], &n);
        if (!buf) return 1;
        ms_zragf = bench_ms(zragf_size_once, buf, n, 20, &out_zragf);
        ms_zlib = bench_ms(zlib_size_once, buf, n, 20, &out_zlib);
        printf("phase30 %s raw: zragf=%lu zlib=%lu delta=%ld ms_zragf=%d ms_zlib=%d\n",
               cases[i],
               (unsigned long)out_zragf,
               (unsigned long)out_zlib,
               (long)out_zlib - (long)out_zragf,
               ms_zragf,
               ms_zlib);
        zragf_p89_host_release(buf);
    }
    return 0;
}
