#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>
#include <libdeflate.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"

static unsigned int xs(unsigned int *s)
{
    unsigned int x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

static void make_case(unsigned char *buf, size_t n, int kind, unsigned int *seed)
{
    size_t i;
    switch (kind) {
        case 0: memset(buf, 'A', n); break;
        case 1: for (i = 0; i < n; ++i) buf[i] = (unsigned char)(i & 0xFFu); break;
        case 2: for (i = 0; i < n; ++i) buf[i] = (unsigned char)(xs(seed) & 0xFFu); break;
        default: for (i = 0; i < n; ++i) buf[i] = (unsigned char)('a' + (i % 5u)); break;
    }
}

static size_t zragf_size(const unsigned char *src, size_t src_len, int wb)
{
    unsigned char *out;
    size_t cap = src_len * 2u + 1024u;
    size_t out_len = 0u;
    zragf_stream s;
    int rc;

    out = (unsigned char *)zragf_p89_host_take(cap);
    if (!out)
        return 0u;
    memset(&s, 0, sizeof(s));
    if (zragf_deflateInit2(&s, 6, 8, wb, 8, ZRAGF_Z_DEFAULT_STRATEGY) != ZRAGF_OK) {
        zragf_p89_host_release(out);
        return 0u;
    }
    s.next_in = (zragf_u8 *)src;
    s.avail_in = src_len;
    s.next_out = out;
    s.avail_out = cap;
    rc = zragf_deflateZ(&s, ZRAGF_FINISH);
    if (rc == ZRAGF_STREAM_END)
        out_len = (size_t)s.total_out;
    zragf_deflateEndZ(&s);
    zragf_p89_host_release(out);
    return out_len;
}

static size_t zlib_size(const unsigned char *src, size_t src_len, int wb)
{
    unsigned char *out;
    size_t cap = compressBound((uLong)src_len) + 64u;
    size_t out_len = 0u;
    z_stream zs;
    int rc;

    out = (unsigned char *)zragf_p89_host_take(cap);
    if (!out)
        return 0u;
    memset(&zs, 0, sizeof(zs));
    if (deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, wb, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        zragf_p89_host_release(out);
        return 0u;
    }
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_len;
    zs.next_out = out;
    zs.avail_out = (uInt)cap;
    rc = deflate(&zs, Z_FINISH);
    if (rc == Z_STREAM_END)
        out_len = (size_t)zs.total_out;
    deflateEnd(&zs);
    zragf_p89_host_release(out);
    return out_len;
}

static size_t libdeflate_size(const unsigned char *src, size_t src_len, int fmt)
{
    struct libdeflate_compressor *c;
    unsigned char *out;
    size_t cap;
    size_t n = 0u;

    c = libdeflate_alloc_compressor(6);
    if (!c)
        return 0u;
    cap = (fmt == 0) ? libdeflate_deflate_compress_bound(c, src_len)
          : (fmt == 1) ? libdeflate_zlib_compress_bound(c, src_len)
                       : libdeflate_gzip_compress_bound(c, src_len);
    out = (unsigned char *)zragf_p89_host_take(cap);
    if (!out) {
        libdeflate_free_compressor(c);
        return 0u;
    }
    if (fmt == 0)
        n = libdeflate_deflate_compress(c, src, src_len, out, cap);
    else if (fmt == 1)
        n = libdeflate_zlib_compress(c, src, src_len, out, cap);
    else
        n = libdeflate_gzip_compress(c, src, src_len, out, cap);
    zragf_p89_host_release(out);
    libdeflate_free_compressor(c);
    return n;
}

int main(void)
{
    static const char *names[] = { "repetitive", "ramp", "random", "smallalpha" };
    unsigned char *buf;
    unsigned int seed = 0x19u;
    int i;
    size_t n = 131072u;

    buf = (unsigned char *)zragf_p89_host_take(n);
    if (!buf)
        return 1;

    for (i = 0; i < 4; ++i) {
        make_case(buf, n, i, &seed);
        printf("phase19 %s raw: zragf=%zu zlib=%zu libdeflate=%zu\n",
               names[i],
               zragf_size(buf, n, -15),
               zlib_size(buf, n, -15),
               libdeflate_size(buf, n, 0));
    }

    zragf_p89_host_release(buf);
    return 0;
}
