#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "zragflib.h"
#include "protocol89_hostmem.h"

static int compress_raw(const unsigned char *src, size_t src_size, int strategy,
                        unsigned char *dst, size_t *dst_size)
{
    zragf_stream zs;
    int rc;
    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, strategy);
    if (rc != ZRAGF_OK)
        return 0;
    zs.next_in = (unsigned char *)src;
    zs.avail_in = src_size;
    zs.next_out = dst;
    zs.avail_out = *dst_size;
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    *dst_size -= zs.avail_out;
    zragf_deflateEndZ(&zs);
    return rc == ZRAGF_STREAM_END;
}

static int inflate_raw_zlib(const unsigned char *src, size_t src_size,
                            unsigned char *dst, size_t *dst_size)
{
    z_stream zs;
    int rc;
    memset(&zs, 0, sizeof(zs));
    rc = inflateInit2(&zs, -15);
    if (rc != Z_OK)
        return 0;
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_size;
    zs.next_out = dst;
    zs.avail_out = (uInt)*dst_size;
    rc = inflate(&zs, Z_FINISH);
    *dst_size -= (size_t)zs.avail_out;
    inflateEnd(&zs);
    return rc == Z_STREAM_END;
}

int main(void)
{
    size_t n = 120000u;
    unsigned char *src = (unsigned char *)zragf_p89_host_take(n);
    unsigned char *out = (unsigned char *)zragf_p89_host_take(n * 2u + 1024u);
    unsigned char *dec = (unsigned char *)zragf_p89_host_take(n + 16u);
    size_t out_size = n * 2u + 1024u;
    size_t dec_size = n + 16u;
    size_t fixed_size = n * 2u + 1024u;
    size_t i;

    if (!src || !out || !dec)
        return 2;

    for (i = 0u; i < n; ++i)
        src[i] = (unsigned char)("abcde "[i % 6u]);

    if (!compress_raw(src, n, ZRAGF_Z_DEFAULT_STRATEGY, out, &out_size))
        return 3;
    if (((out[0] >> 1) & 0x3u) != 2u)
        return 4;
    if (out_size > 220u)
        return 5;
    if (!inflate_raw_zlib(out, out_size, dec, &dec_size))
        return 6;
    if (dec_size != n || memcmp(src, dec, n) != 0)
        return 7;

    if (!compress_raw(src, n, ZRAGF_Z_FIXED, out, &fixed_size))
        return 8;
    if (((out[0] >> 1) & 0x3u) != 1u)
        return 9;

    zragf_p89_host_release(src);
    zragf_p89_host_release(out);
    zragf_p89_host_release(dec);
    printf("phase4 tuning ok\n");
    return 0;
}
