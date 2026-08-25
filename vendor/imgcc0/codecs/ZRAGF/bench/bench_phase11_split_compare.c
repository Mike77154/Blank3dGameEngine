#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
    unsigned st = 0x2468ACE1u;
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

int main(void)
{
    unsigned char *src;
    unsigned char *single;
    unsigned char *split;
    size_t n = 0u;
    size_t cap;
    size_t single_size = 0u;
    size_t split_size = 0u;

    src = make_case(&n);
    if (!src)
        return 1;
    cap = zragf_deflate_rfc1951_stored_bound(n) + 256u;
    single = (unsigned char *)zragf_p89_host_take(cap);
    split = (unsigned char *)zragf_p89_host_take(cap);
    if (!single || !split) {
        zragf_p89_host_release(src);
        zragf_p89_host_release(single);
        zragf_p89_host_release(split);
        return 1;
    }

    if (!zragf_deflate_rfc1951_compress_chunk_single_with_dict(NULL, 0u,
                                                               src, n,
                                                               single, cap, &single_size,
                                                               1, 6, ZRAGF_Z_DEFAULT_STRATEGY,
                                                               0, 0, 0, 0, 0)) {
        fprintf(stderr, "single failed\n");
        zragf_p89_host_release(src); zragf_p89_host_release(single); zragf_p89_host_release(split);
        return 1;
    }
    if (!zragf_deflate_rfc1951_compress_chunk_with_dict(NULL, 0u,
                                                        src, n,
                                                        split, cap, &split_size,
                                                        1, 6, ZRAGF_Z_DEFAULT_STRATEGY,
                                                        0, 0, 0, 0, 0)) {
        fprintf(stderr, "split failed\n");
        zragf_p89_host_release(src); zragf_p89_host_release(single); zragf_p89_host_release(split);
        return 1;
    }

    printf("phase11 split compare input=%lu single=%lu split=%lu delta=%ld\n",
           (unsigned long)n,
           (unsigned long)single_size,
           (unsigned long)split_size,
           (long)(single_size - split_size));

    zragf_p89_host_release(src);
    zragf_p89_host_release(single);
    zragf_p89_host_release(split);
    return 0;
}
