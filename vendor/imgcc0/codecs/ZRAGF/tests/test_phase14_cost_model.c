#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "zragflib_internal.h"
#include "zragf_deflate/deflate_plan.h"
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
    unsigned st = 0xCAFEBABEu;
    unsigned char *buf;
    size_t i;
    const size_t a = 20u * 1024u;
    const size_t b = 28u * 1024u;
    const size_t c = 36u * 1024u;
    const size_t d = 60u * 1024u;
    const char pat1[] = "ABCDABCDABCDABCD";
    const char pat2[] = "lorem ipsum dolor sit amet, consectetur adipiscing elit. ";

    *out_size = a + b + c + d;
    buf = (unsigned char *)zragf_p89_host_take(*out_size);
    if (!buf)
        return NULL;

    for (i = 0u; i < a; ++i)
        buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
    for (; i < a + b; ++i)
        buf[i] = (unsigned char)pat1[(i - a) % (sizeof(pat1) - 1u)];
    for (; i < a + b + c; ++i)
        buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
    for (; i < *out_size; ++i)
        buf[i] = (unsigned char)pat2[(i - (a + b + c)) % (sizeof(pat2) - 1u)];
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

static int near_boundary(zragf_size_t got, zragf_size_t want)
{
    zragf_size_t diff = (got > want) ? (got - want) : (want - got);
    return (diff <= 3072u) ? 1 : 0;
}

int main(void)
{
    size_t n = 0u;
    unsigned char *src = make_case(&n);
    zragf_deflate_split_plan plan;
    const zragf_size_t boundaries[3] = { 20u * 1024u, 48u * 1024u, 84u * 1024u };
    int hits = 0;
    size_t cap;
    unsigned char *single_out;
    unsigned char *split_out;
    size_t single_size = 0u;
    size_t split_size = 0u;
    int i;
    int b;

    if (!src)
        return 1;

    if (!zragf_deflate_plan_build(src, n, &plan)) {
        zragf_p89_host_release(src);
        return 1;
    }

    for (i = 0; i < plan.count; ++i) {
        for (b = 0; b < 3; ++b) {
            if (near_boundary(plan.offsets[i], boundaries[b])) {
                hits++;
                break;
            }
        }
    }
    if (hits < 2) {
        fprintf(stderr, "planner did not find enough strong transitions (hits=%d count=%d)\n", hits, plan.count);
        zragf_p89_host_release(src);
        return 1;
    }

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
        fprintf(stderr, "expected phase14 split cost model to improve output: single=%lu split=%lu\n",
                (unsigned long)single_size, (unsigned long)split_size);
        zragf_p89_host_release(src); zragf_p89_host_release(single_out); zragf_p89_host_release(split_out);
        return 1;
    }

    printf("phase14 cost-model hits=%d single=%lu split=%lu\n",
           hits, (unsigned long)single_size, (unsigned long)split_size);

    zragf_p89_host_release(src);
    zragf_p89_host_release(single_out);
    zragf_p89_host_release(split_out);
    return 0;
}
