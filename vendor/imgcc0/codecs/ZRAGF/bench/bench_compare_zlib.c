#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>
#include "zragflib.h"
#include "protocol89_hostmem.h"
#include "protocol89_fixed.h"

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
    if (strcmp(name, "repetitive") == 0) {
        *out_size = 120000u;
        buf = (unsigned char *)zragf_p89_host_take(*out_size);
        if (!buf) return NULL;
        for (i = 0u; i < *out_size; ++i) buf[i] = (unsigned char)("abcde "[i % 6u]);
        return buf;
    }
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
    if (strcmp(name, "random") == 0) {
        unsigned st = 0xCAFEBABEu;
        *out_size = 131072u;
        buf = (unsigned char *)zragf_p89_host_take(*out_size);
        if (!buf) return NULL;
        for (i = 0u; i < *out_size; ++i) buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
        return buf;
    }
    if (strcmp(name, "rle") == 0) {
        *out_size = 65536u;
        buf = (unsigned char *)zragf_p89_host_take(*out_size);
        if (!buf) return NULL;
        memset(buf, 'A', *out_size);
        return buf;
    }
    return NULL;
}

static size_t bench_zragf_raw(const unsigned char *src, size_t n, zragf_fx *ms)
{
    zragf_stream zs;
    unsigned char *out;
    size_t cap = n * 2u + 1024u;
    clock_t t0, t1;
    int rc;

    memset(&zs, 0, sizeof(zs));
    out = (unsigned char *)zragf_p89_host_take(cap);
    if (!out) return 0u;

    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK) {
        zragf_p89_host_release(out);
        return 0u;
    }

    zs.next_in = (unsigned char *)src;
    zs.avail_in = n;
    zs.next_out = out;
    zs.avail_out = cap;

    t0 = clock();
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    t1 = clock();
    if (ms) *ms = 1000 * (zragf_fx)(t1 - t0) / (zragf_fx)CLOCKS_PER_SEC;
    if (rc != ZRAGF_STREAM_END) {
        zragf_deflateEndZ(&zs);
        zragf_p89_host_release(out);
        return 0u;
    }

    n = cap - zs.avail_out;
    zragf_deflateEndZ(&zs);
    zragf_p89_host_release(out);
    return n;
}

static size_t bench_zlib_raw(const unsigned char *src, size_t n, zragf_fx *ms)
{
    z_stream zs;
    unsigned char *out;
    size_t cap = compressBound((uLong)n) + 32u;
    clock_t t0, t1;
    int rc;

    memset(&zs, 0, sizeof(zs));
    out = (unsigned char *)zragf_p89_host_take(cap);
    if (!out) return 0u;

    rc = deflateInit2(&zs, 6, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    if (rc != Z_OK) {
        zragf_p89_host_release(out);
        return 0u;
    }

    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)n;
    zs.next_out = out;
    zs.avail_out = (uInt)cap;

    t0 = clock();
    rc = deflate(&zs, Z_FINISH);
    t1 = clock();
    if (ms) *ms = 1000 * (zragf_fx)(t1 - t0) / (zragf_fx)CLOCKS_PER_SEC;
    if (rc != Z_STREAM_END) {
        deflateEnd(&zs);
        zragf_p89_host_release(out);
        return 0u;
    }

    n = cap - zs.avail_out;
    deflateEnd(&zs);
    zragf_p89_host_release(out);
    return n;
}

int main(void)
{
    static const char *cases[] = {"repetitive", "mixed", "random", "rle"};
    size_t i;
    for (i = 0u; i < sizeof(cases) / sizeof(cases[0]); ++i) {
        size_t n = 0u;
        zragf_fx ms_zragf = 0, ms_zlib = 0;
        size_t out_zragf, out_zlib;
        unsigned char *buf = make_case(cases[i], &n);
        if (!buf) return 1;
        out_zragf = bench_zragf_raw(buf, n, &ms_zragf);
        out_zlib = bench_zlib_raw(buf, n, &ms_zlib);
        printf("%s input=%lu zragf=%lu zlib=%lu ratio_zragf=%d ratio_zlib=%d ms_zragf=%d ms_zlib=%d\n",
               cases[i], (unsigned long)n, (unsigned long)out_zragf, (unsigned long)out_zlib,
               n ? (zragf_fx)out_zragf / (zragf_fx)n : 0,
               n ? (zragf_fx)out_zlib / (zragf_fx)n : 0,
               ms_zragf, ms_zlib);
        zragf_p89_host_release(buf);
    }
    return 0;
}
