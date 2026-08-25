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
    unsigned st = 0x13579BDFu;
    unsigned char *buf;
    size_t i;
    *out_size = 131072u;
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
        buf[i] = (unsigned char)("hello world hello world "[(i - 98304u) % 24u]);
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

int main(void)
{
    size_t n = 0u;
    unsigned char *src = make_case(&n);
    size_t cap;
    unsigned char *single_out;
    unsigned char *split_out;
    size_t single_size = 0u;
    size_t split_size = 0u;

    if (!src)
        return 1;

    cap = zragf_deflate_rfc1951_stored_bound(n) + 256u;
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
        zragf_p89_host_release(src); zragf_p89_host_release(single_out); zragf_p89_host_release(split_out);
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
        fprintf(stderr, "split encode failed\n");
        zragf_p89_host_release(src); zragf_p89_host_release(single_out); zragf_p89_host_release(split_out);
        return 1;
    }

    if (!verify_raw_zlib(single_out, single_size, src, n) ||
        !verify_raw_zlib(split_out, split_size, src, n)) {
        fprintf(stderr, "external zlib verification failed\n");
        zragf_p89_host_release(src); zragf_p89_host_release(single_out); zragf_p89_host_release(split_out);
        return 1;
    }

    if (!(split_size < single_size)) {
        fprintf(stderr, "expected splitter to improve output: single=%lu split=%lu\n",
                (unsigned long)single_size, (unsigned long)split_size);
        zragf_p89_host_release(src); zragf_p89_host_release(single_out); zragf_p89_host_release(split_out);
        return 1;
    }

    printf("phase11 splitter single=%lu split=%lu\n",
           (unsigned long)single_size, (unsigned long)split_size);

    zragf_p89_host_release(src);
    zragf_p89_host_release(single_out);
    zragf_p89_host_release(split_out);
    return 0;
}
