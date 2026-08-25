#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"

static unsigned xorshift32(unsigned *s)
{
    unsigned x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

static void make_mixed(unsigned char *buf, size_t n)
{
    unsigned st = 0x12345678u;
    size_t i;
    assert(n >= 134304u);
    for (i = 0u; i < 32768u; ++i) buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
    for (; i < 65536u; ++i) buf[i] = (unsigned char)("ABCD"[(i - 32768u) & 3u]);
    for (; i < 98304u; ++i) buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
    for (; i < 134304u; ++i) buf[i] = (unsigned char)("hello world "[(i - 98304u) % 12u]);
}

static void make_ramp(unsigned char *buf, size_t n)
{
    size_t i;
    for (i = 0u; i < n; ++i)
        buf[i] = (unsigned char)(i & 0xFFu);
}

static void make_chunked_case(unsigned char *buf, size_t n)
{
    static const unsigned char pat[] =
        "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
    size_t i;
    for (i = 0u; i < n; ++i)
        buf[i] = pat[i % (sizeof(pat) - 1u)];
}

static size_t zragf_raw_size(const unsigned char *src, size_t src_len)
{
    zragf_stream zs;
    unsigned char *out;
    size_t cap = src_len * 2u + 1024u;
    size_t out_len = 0u;
    int rc;

    out = (unsigned char *)zragf_p89_host_take(cap);
    assert(out != NULL);
    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    assert(rc == ZRAGF_OK);
    zs.next_in = (unsigned char *)src;
    zs.avail_in = src_len;
    zs.next_out = out;
    zs.avail_out = cap;
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    assert(rc == ZRAGF_STREAM_END);
    out_len = zs.total_out;
    zragf_deflateEndZ(&zs);
    zragf_p89_host_release(out);
    return out_len;
}

static size_t zlib_raw_size(const unsigned char *src, size_t src_len)
{
    z_stream zs;
    unsigned char *out;
    size_t cap = compressBound((uLong)src_len) + 64u;
    size_t out_len = 0u;
    int rc;

    out = (unsigned char *)zragf_p89_host_take(cap);
    assert(out != NULL);
    memset(&zs, 0, sizeof(zs));
    rc = deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    assert(rc == Z_OK);
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_len;
    zs.next_out = out;
    zs.avail_out = (uInt)cap;
    rc = deflate(&zs, Z_FINISH);
    assert(rc == Z_STREAM_END);
    out_len = zs.total_out;
    deflateEnd(&zs);
    zragf_p89_host_release(out);
    return out_len;
}

static size_t zragf_chunked_raw(const unsigned char *src, size_t src_len,
                                size_t in_chunk, size_t out_chunk)
{
    zragf_stream zs;
    unsigned char *out;
    size_t out_cap = src_len * 2u + 1024u;
    size_t out_size = 0u;
    int rc;
    size_t pos = 0u;

    out = (unsigned char *)zragf_p89_host_take(out_cap);
    assert(out != NULL);

    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    assert(rc == ZRAGF_OK);

    while (pos < src_len) {
        size_t take = (src_len - pos > in_chunk) ? in_chunk : (src_len - pos);
        zs.next_in = (unsigned char *)src + pos;
        zs.avail_in = (unsigned int)take;
        do {
            size_t before = zs.total_out;
            zs.next_out = out + out_size;
            zs.avail_out = (unsigned int)out_chunk;
            rc = zragf_deflateZ(&zs, ZRAGF_NO_FLUSH);
            assert(rc >= 0);
            out_size += zs.total_out - before;
        } while (zs.avail_in > 0u || zs.avail_out == 0u);
        pos += take;
    }

    for (;;) {
        size_t before = zs.total_out;
        zs.next_out = out + out_size;
        zs.avail_out = (unsigned int)out_chunk;
        rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
        out_size += zs.total_out - before;
        if (rc == ZRAGF_STREAM_END)
            break;
        assert(rc >= 0);
    }

    zragf_deflateEndZ(&zs);
    zragf_p89_host_release(out);
    return out_size;
}

static size_t zlib_chunked_raw(const unsigned char *src, size_t src_len,
                               size_t in_chunk, size_t out_chunk)
{
    z_stream zs;
    unsigned char *out;
    size_t out_cap = src_len * 2u + 1024u;
    size_t out_size = 0u;
    int rc;
    size_t pos = 0u;

    out = (unsigned char *)zragf_p89_host_take(out_cap);
    assert(out != NULL);

    memset(&zs, 0, sizeof(zs));
    rc = deflateInit2(&zs, 6, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    assert(rc == Z_OK);

    while (pos < src_len) {
        size_t take = (src_len - pos > in_chunk) ? in_chunk : (src_len - pos);
        zs.next_in = (Bytef *)src + pos;
        zs.avail_in = (uInt)take;
        do {
            zs.next_out = out + out_size;
            zs.avail_out = (uInt)out_chunk;
            rc = deflate(&zs, Z_NO_FLUSH);
            assert(rc == Z_OK);
            out_size += out_chunk - (size_t)zs.avail_out;
        } while (zs.avail_in > 0u || zs.avail_out == 0u);
        pos += take;
    }

    do {
        zs.next_out = out + out_size;
        zs.avail_out = (uInt)out_chunk;
        rc = deflate(&zs, Z_FINISH);
        assert(rc == Z_OK || rc == Z_STREAM_END);
        out_size += out_chunk - (size_t)zs.avail_out;
    } while (rc != Z_STREAM_END);

    deflateEnd(&zs);
    zragf_p89_host_release(out);
    return out_size;
}

int main(void)
{
    unsigned char *mixed;
    unsigned char *ramp;
    unsigned char *chunked;
    size_t mixed_zragf;
    size_t mixed_zlib;
    size_t ramp_zragf;
    size_t ramp_zlib;
    size_t chunked_zragf;
    size_t chunked_zlib;

    mixed = (unsigned char *)zragf_p89_host_take(134304u);
    ramp = (unsigned char *)zragf_p89_host_take(131072u);
    chunked = (unsigned char *)zragf_p89_host_take(1024u * 1024u);
    assert(mixed && ramp && chunked);

    make_mixed(mixed, 134304u);
    make_ramp(ramp, 131072u);
    make_chunked_case(chunked, 1024u * 1024u);

    mixed_zragf = zragf_raw_size(mixed, 134304u);
    mixed_zlib = zlib_raw_size(mixed, 134304u);
    assert(mixed_zragf <= mixed_zlib);

    ramp_zragf = zragf_raw_size(ramp, 131072u);
    ramp_zlib = zlib_raw_size(ramp, 131072u);
    assert(ramp_zragf <= ramp_zlib + 2u);

    chunked_zragf = zragf_chunked_raw(chunked, 1024u * 1024u, 4096u, 257u);
    chunked_zlib = zlib_chunked_raw(chunked, 1024u * 1024u, 4096u, 257u);
    assert(chunked_zragf <= chunked_zlib + 20u);

    printf("phase20 advantage ok mixed=%lu/%lu ramp=%lu/%lu chunked=%lu/%lu\n",
           (unsigned long)mixed_zragf, (unsigned long)mixed_zlib,
           (unsigned long)ramp_zragf, (unsigned long)ramp_zlib,
           (unsigned long)chunked_zragf, (unsigned long)chunked_zlib);

    zragf_p89_host_release(mixed);
    zragf_p89_host_release(ramp);
    zragf_p89_host_release(chunked);
    return 0;
}
