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

static unsigned char *make_mixed(size_t *out_size)
{
    unsigned st = 0x12345678u;
    unsigned char *buf;
    size_t i;
    *out_size = 134304u;
    buf = (unsigned char *)zragf_p89_host_take(*out_size);
    if (!buf) return NULL;
    for (i = 0u; i < 32768u; ++i) buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
    for (; i < 65536u; ++i) buf[i] = (unsigned char)("ABCD"[(i - 32768u) & 3u]);
    for (; i < 98304u; ++i) buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
    for (; i < *out_size; ++i) buf[i] = (unsigned char)("hello world "[(i - 98304u) % 12u]);
    return buf;
}

static unsigned char *make_random(size_t *out_size)
{
    unsigned st = 0xCAFEBABEu;
    unsigned char *buf;
    size_t i;
    *out_size = 131072u;
    buf = (unsigned char *)zragf_p89_host_take(*out_size);
    if (!buf) return NULL;
    for (i = 0u; i < *out_size; ++i) buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
    return buf;
}

static int roundtrip_raw(const unsigned char *src, size_t n, size_t max_expect)
{
    zragf_stream zs;
    unsigned char *out = NULL;
    unsigned char *dec = NULL;
    size_t cap = n * 2u + 2048u;
    int rc;
    int ok = 0;

    out = (unsigned char *)zragf_p89_host_take(cap);
    dec = (unsigned char *)zragf_p89_host_take(n + 16u);
    if (!out || !dec)
        goto done;

    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK)
        goto done;
    zs.next_in = (unsigned char *)src;
    zs.avail_in = n;
    zs.next_out = out;
    zs.avail_out = cap;
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    if (rc != ZRAGF_STREAM_END) {
        zragf_deflateEndZ(&zs);
        goto done;
    }
    if (zs.total_out == 0u || zs.total_out >= max_expect) {
        zragf_deflateEndZ(&zs);
        goto done;
    }
    zragf_deflateEndZ(&zs);

    {
        z_stream z;
        memset(&z, 0, sizeof(z));
        if (inflateInit2(&z, -15) != Z_OK)
            goto done;
        z.next_in = out;
        z.avail_in = (uInt)zs.total_out;
        z.next_out = dec;
        z.avail_out = (uInt)(n + 16u);
        rc = inflate(&z, Z_FINISH);
        inflateEnd(&z);
        if (rc != Z_STREAM_END || z.total_out != n)
            goto done;
    }

    if (memcmp(src, dec, n) != 0)
        goto done;
    ok = 1;
done:
    zragf_p89_host_release(out);
    zragf_p89_host_release(dec);
    return ok;
}

int main(void)
{
    size_t nm = 0u, nr = 0u;
    unsigned char *mixed = make_mixed(&nm);
    unsigned char *random_buf = make_random(&nr);
    if (!mixed || !random_buf) {
        zragf_p89_host_release(mixed);
        zragf_p89_host_release(random_buf);
        return 1;
    }
    if (!roundtrip_raw(mixed, nm, 66100u)) {
        fprintf(stderr, "phase32 mixed matcher/token regression\n");
        zragf_p89_host_release(mixed);
        zragf_p89_host_release(random_buf);
        return 2;
    }
    if (!roundtrip_raw(random_buf, nr, 131120u)) {
        fprintf(stderr, "phase32 random matcher/token regression\n");
        zragf_p89_host_release(mixed);
        zragf_p89_host_release(random_buf);
        return 3;
    }
    printf("phase32 matcher/token path ok\n");
    zragf_p89_host_release(mixed);
    zragf_p89_host_release(random_buf);
    return 0;
}
