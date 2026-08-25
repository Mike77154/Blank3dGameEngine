#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"
#include "protocol89_stdio.h"

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

static unsigned char *make_jsonish(size_t *out_size)
{
    static const char *frag[] = {
        "{\"id\":", ",\"user\":\"miguel\",\"type\":\"event\",\"ok\":true,",
        "\"payload\":\"AAAAAAAAAAAAAAAA\",\"tags\":[\"alpha\",\"beta\",\"gamma\"]}\n"
    };
    unsigned char *buf;
    size_t i = 0u;
    *out_size = 180000u;
    buf = (unsigned char *)zragf_p89_host_take(*out_size);
    if (!buf) return NULL;
    while (i < *out_size) {
        char num[32];
        int len = zragf_p89_format(num, sizeof(num), "%lu", (unsigned long)i);
        size_t j, k;
        for (j = 0u; j < strlen(frag[0]) && i < *out_size; ++j) buf[i++] = (unsigned char)frag[0][j];
        for (j = 0u; j < (size_t)len && i < *out_size; ++j) buf[i++] = (unsigned char)num[j];
        for (k = 1u; k < 3u; ++k)
            for (j = 0u; j < strlen(frag[k]) && i < *out_size; ++j) buf[i++] = (unsigned char)frag[k][j];
    }
    return buf;
}

static int roundtrip_with_zlib(const unsigned char *src, size_t n, size_t max_expect)
{
    zragf_stream zs;
    unsigned char *out = NULL;
    unsigned char *dec = NULL;
    size_t cap = n * 2u + 1024u;
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
    size_t n1 = 0u, n2 = 0u;
    unsigned char *mixed = make_mixed(&n1);
    unsigned char *jsonish = make_jsonish(&n2);
    if (!mixed || !jsonish) {
        zragf_p89_host_release(mixed);
        zragf_p89_host_release(jsonish);
        return 1;
    }

    if (!roundtrip_with_zlib(mixed, n1, 66150u)) {
        fprintf(stderr, "phase30 mixed whole-buffer regression\n");
        zragf_p89_host_release(mixed);
        zragf_p89_host_release(jsonish);
        return 2;
    }
    if (!roundtrip_with_zlib(jsonish, n2, 5900u)) {
        fprintf(stderr, "phase30 json whole-buffer regression\n");
        zragf_p89_host_release(mixed);
        zragf_p89_host_release(jsonish);
        return 3;
    }

    printf("phase30 whole-buffer ok\n");
    zragf_p89_host_release(mixed);
    zragf_p89_host_release(jsonish);
    return 0;
}
