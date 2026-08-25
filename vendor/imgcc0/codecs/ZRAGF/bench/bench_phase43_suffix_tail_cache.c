#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <zlib.h>
#include "zragflib_internal.h"
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

static unsigned char *make_case(size_t *out_size)
{
    unsigned st = 0x12345678u;
    unsigned char *buf;
    size_t i;
    *out_size = 134304u;
    buf = (unsigned char *)zragf_p89_host_take(*out_size);
    if (!buf)
        return NULL;
    for (i = 0u; i < 32768u; ++i)
        buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
    for (; i < 65536u; ++i)
        buf[i] = (unsigned char)("ABCD"[(i - 32768u) & 3u]);
    for (; i < 98304u; ++i)
        buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
    for (; i < *out_size; ++i)
        buf[i] = (unsigned char)("hello world "[(i - 98304u) % 12u]);
    return buf;
}

static size_t zragf_split_size(const unsigned char *src, size_t n)
{
    size_t cap = zragf_deflate_rfc1951_stored_bound(n) + 512u;
    unsigned char *out = (unsigned char *)zragf_p89_host_take(cap);
    size_t out_len = 0u;
    if (!out)
        return 0u;
    if (!zragf_deflate_rfc1951_compress_chunk_with_dict(NULL, 0u,
                                                        src, n,
                                                        out, cap,
                                                        &out_len,
                                                        1, 6,
                                                        ZRAGF_Z_DEFAULT_STRATEGY,
                                                        0, 0, 0, 0, 0))
        out_len = 0u;
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
    if (rc == Z_STREAM_END) out_len = (size_t)zs.total_out;
    deflateEnd(&zs);
    zragf_p89_host_release(out);
    return out_len;
}

static zragf_fx bench_ms(size_t (*fn)(const unsigned char *, size_t), const unsigned char *src, size_t n, int iters, size_t *last)
{
    clock_t t0 = clock();
    size_t v = 0u;
    int i;
    for (i = 0; i < iters; ++i)
        v = fn(src, n);
    if (last)
        *last = v;
    return 1000 * (zragf_fx)(clock() - t0) / (zragf_fx)CLOCKS_PER_SEC;
}

int main(void)
{
    size_t n = 0u;
    size_t out_split = 0u, out_zlib = 0u;
    zragf_fx ms_split, ms_zlib;
    unsigned char *buf = make_case(&n);
    if (!buf)
        return 1;
    ms_split = bench_ms(zragf_split_size, buf, n, 8, &out_split);
    ms_zlib = bench_ms(zlib_size_once, buf, n, 8, &out_zlib);
    printf("phase43 mixed raw: split=%lu zlib=%lu delta_zlib=%ld ms_split=%d ms_zlib=%d\n",
           (unsigned long)out_split,
           (unsigned long)out_zlib,
           (long)out_zlib - (long)out_split,
           ms_split, ms_zlib);
    zragf_p89_host_release(buf);
    return 0;
}
