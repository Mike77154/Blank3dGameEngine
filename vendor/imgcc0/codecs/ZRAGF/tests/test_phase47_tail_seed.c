#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "zragflib_internal.h"
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

static int verify_raw_zlib(const unsigned char *compressed,
                           size_t compressed_size,
                           const unsigned char *expected,
                           size_t expected_size)
{
    z_stream zs;
    unsigned char *out;
    int rc;

    memset(&zs, 0, sizeof(zs));
    out = (unsigned char *)zragf_p89_host_take(expected_size + 32u);
    if (!out)
        return 0;

    rc = inflateInit2(&zs, -15);
    if (rc != Z_OK) {
        zragf_p89_host_release(out);
        return 0;
    }

    zs.next_in = (Bytef *)compressed;
    zs.avail_in = (uInt)compressed_size;
    zs.next_out = out;
    zs.avail_out = (uInt)(expected_size + 32u);

    rc = inflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) {
        inflateEnd(&zs);
        zragf_p89_host_release(out);
        return 0;
    }
    if ((size_t)zs.total_out != expected_size || memcmp(out, expected, expected_size) != 0) {
        inflateEnd(&zs);
        zragf_p89_host_release(out);
        return 0;
    }
    inflateEnd(&zs);
    zragf_p89_host_release(out);
    return 1;
}

static size_t zlib_raw_size(const unsigned char *src, size_t n)
{
    z_stream zs;
    unsigned char *out;
    size_t cap = compressBound((uLong)n) + 64u;
    size_t out_len = 0u;
    int rc;

    out = (unsigned char *)zragf_p89_host_take(cap);
    if (!out)
        return 0u;
    memset(&zs, 0, sizeof(zs));
    rc = deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    if (rc != Z_OK) {
        zragf_p89_host_release(out);
        return 0u;
    }
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)n;
    zs.next_out = out;
    zs.avail_out = (uInt)cap;
    rc = deflate(&zs, Z_FINISH);
    if (rc == Z_STREAM_END)
        out_len = (size_t)zs.total_out;
    deflateEnd(&zs);
    zragf_p89_host_release(out);
    return out_len;
}

int main(void)
{
    size_t n = 0u;
    unsigned char *src = make_case(&n);
    size_t cap;
    unsigned char *single_out;
    unsigned char *split_out;
    size_t single_size = 0u;
    size_t split_size = 0u;
    size_t zlib_size;

    if (!src)
        return 1;

    cap = zragf_deflate_rfc1951_stored_bound(n) + 512u;
    single_out = (unsigned char *)zragf_p89_host_take(cap);
    split_out = (unsigned char *)zragf_p89_host_take(cap);
    if (!single_out || !split_out) {
        zragf_p89_host_release(src);
        zragf_p89_host_release(single_out);
        zragf_p89_host_release(split_out);
        return 1;
    }

    if (!zragf_deflate_rfc1951_compress_chunk_single_with_dict(NULL, 0u,
                                                               src, n,
                                                               single_out, cap,
                                                               &single_size,
                                                               1,
                                                               6,
                                                               ZRAGF_Z_DEFAULT_STRATEGY,
                                                               0, 0, 0, 0, 0)) {
        fprintf(stderr, "single encode failed\n");
        return 1;
    }

    if (!zragf_deflate_rfc1951_compress_chunk_with_dict(NULL, 0u,
                                                        src, n,
                                                        split_out, cap,
                                                        &split_size,
                                                        1,
                                                        6,
                                                        ZRAGF_Z_DEFAULT_STRATEGY,
                                                        0, 0, 0, 0, 0)) {
        fprintf(stderr, "phase47 split encode failed\n");
        return 1;
    }

    if (!verify_raw_zlib(single_out, single_size, src, n) ||
        !verify_raw_zlib(split_out, split_size, src, n)) {
        fprintf(stderr, "external zlib verification failed\n");
        return 1;
    }

    zlib_size = zlib_raw_size(src, n);
    if (zlib_size == 0u) {
        fprintf(stderr, "zlib encode failed\n");
        return 1;
    }

    if (!(split_size + 100u < single_size)) {
        fprintf(stderr, "expected split path to stay clearly better than single: single=%lu split=%lu\n",
                (unsigned long)single_size, (unsigned long)split_size);
        return 1;
    }
    if (!(split_size < zlib_size)) {
        fprintf(stderr, "expected split path to beat zlib on mixed corpus: split=%lu zlib=%lu\n",
                (unsigned long)split_size, (unsigned long)zlib_size);
        return 1;
    }
    if (!(split_size <= 65698u)) {
        fprintf(stderr, "expected phase47 to preserve phase46 ratio target: split=%lu\n",
                (unsigned long)split_size);
        return 1;
    }

    printf("phase47 tail-seed single=%lu split=%lu zlib=%lu delta_single=%ld delta_zlib=%ld\n",
           (unsigned long)single_size,
           (unsigned long)split_size,
           (unsigned long)zlib_size,
           (long)single_size - (long)split_size,
           (long)zlib_size - (long)split_size);
    zragf_p89_host_release(src);
    zragf_p89_host_release(single_out);
    zragf_p89_host_release(split_out);
    return 0;
}
