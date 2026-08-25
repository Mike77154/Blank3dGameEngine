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

static unsigned char *make_case(const char *name, size_t *out_size)
{
    unsigned char *buf;
    size_t i;
    if (strcmp(name, "mixed") == 0) {
        unsigned st = 0x12345678u;
        *out_size = 134304u;
        buf = (unsigned char *)zragf_p89_host_take(*out_size);
        if (!buf) return NULL;
        for (i = 0u; i < 32768u; ++i) buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
        for (; i < 65536u; ++i) buf[i] = (unsigned char)("ABCD"[(i - 32768u) & 3u]);
        for (; i < 98304u; ++i) buf[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
        for (; i < *out_size; ++i) buf[i] = (unsigned char)("hello world "[(i - 98304u) % 12u]);
        return buf;
    }
    if (strcmp(name, "jsonish") == 0) {
        static const char *frag[] = {
            "{\"id\":", ",\"user\":\"miguel\",\"type\":\"event\",\"ok\":true,",
            "\"payload\":\"AAAAAAAAAAAAAAAA\",\"tags\":[\"alpha\",\"beta\",\"gamma\"]}\n"
        };
        *out_size = 180000u;
        buf = (unsigned char *)zragf_p89_host_take(*out_size);
        if (!buf) return NULL;
        i = 0u;
        while (i < *out_size) {
            char num[32];
            int len = zragf_p89_format(num, sizeof(num), "%lu", (unsigned long)i);
            size_t j;
            for (j = 0u; j < strlen(frag[0]) && i < *out_size; ++j) buf[i++] = (unsigned char)frag[0][j];
            for (j = 0u; j < (size_t)len && i < *out_size; ++j) buf[i++] = (unsigned char)num[j];
            for (j = 1u; j < 3u; ++j) {
                size_t k;
                for (k = 0u; k < strlen(frag[j]) && i < *out_size; ++k) buf[i++] = (unsigned char)frag[j][k];
            }
        }
        return buf;
    }
    return NULL;
}

static size_t zragf_raw_compress(const unsigned char *src, size_t n, unsigned char **outp)
{
    zragf_stream zs;
    unsigned char *out;
    size_t cap = n * 2u + 1024u;
    int rc;
    out = (unsigned char *)zragf_p89_host_take(cap);
    if (!out) return 0u;
    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK) {
        zragf_p89_host_release(out);
        return 0u;
    }
    zs.next_in = (unsigned char *)src;
    zs.avail_in = n;
    zs.next_out = out;
    zs.avail_out = cap;
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    if (rc != ZRAGF_STREAM_END) {
        zragf_deflateEndZ(&zs);
        zragf_p89_host_release(out);
        return 0u;
    }
    *outp = out;
    cap = zs.total_out;
    zragf_deflateEndZ(&zs);
    return cap;
}

static size_t zlib_raw_compress(const unsigned char *src, size_t n, unsigned char **outp)
{
    z_stream zs;
    unsigned char *out;
    size_t cap = compressBound((uLong)n) + 64u;
    int rc;
    out = (unsigned char *)zragf_p89_host_take(cap);
    if (!out) return 0u;
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
    if (rc != Z_STREAM_END) {
        deflateEnd(&zs);
        zragf_p89_host_release(out);
        return 0u;
    }
    *outp = out;
    cap = zs.total_out;
    deflateEnd(&zs);
    return cap;
}

static int zlib_raw_decompress(const unsigned char *src, size_t src_n,
                               unsigned char *dst, size_t dst_n)
{
    z_stream zs;
    int rc;
    memset(&zs, 0, sizeof(zs));
    rc = inflateInit2(&zs, -15);
    if (rc != Z_OK) return 0;
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_n;
    zs.next_out = dst;
    zs.avail_out = (uInt)dst_n;
    rc = inflate(&zs, Z_FINISH);
    inflateEnd(&zs);
    return (rc == Z_STREAM_END && zs.total_out == dst_n) ? 1 : 0;
}

int main(void)
{
    size_t n_mixed = 0u, n_json = 0u;
    unsigned char *mixed = make_case("mixed", &n_mixed);
    unsigned char *json = make_case("jsonish", &n_json);
    unsigned char *zragf_out = NULL;
    unsigned char *zlib_out = NULL;
    unsigned char *tmp = NULL;
    size_t zragf_mixed, zlib_mixed, zragf_json, zlib_json;
    int ok = 1;
    if (!mixed || !json) return 1;

    zragf_mixed = zragf_raw_compress(mixed, n_mixed, &zragf_out);
    zlib_mixed = zlib_raw_compress(mixed, n_mixed, &zlib_out);
    if (zragf_mixed == 0u || zlib_mixed == 0u || zragf_mixed > zlib_mixed) ok = 0;
    tmp = (unsigned char *)zragf_p89_host_take(n_mixed);
    if (!tmp || !zlib_raw_decompress(zragf_out, zragf_mixed, tmp, n_mixed) || memcmp(tmp, mixed, n_mixed) != 0) ok = 0;
    zragf_p89_host_release(tmp); tmp = NULL; zragf_p89_host_release(zragf_out); zragf_p89_host_release(zlib_out); zragf_out = zlib_out = NULL;

    zragf_json = zragf_raw_compress(json, n_json, &zragf_out);
    zlib_json = zlib_raw_compress(json, n_json, &zlib_out);
    if (zragf_json == 0u || zlib_json == 0u || zragf_json > zlib_json) ok = 0;
    tmp = (unsigned char *)zragf_p89_host_take(n_json);
    if (!tmp || !zlib_raw_decompress(zragf_out, zragf_json, tmp, n_json) || memcmp(tmp, json, n_json) != 0) ok = 0;

    if (!ok) {
        fprintf(stderr, "phase29 step29 failed mixed=%lu/%lu json=%lu/%lu\n",
                (unsigned long)zragf_mixed, (unsigned long)zlib_mixed,
                (unsigned long)zragf_json, (unsigned long)zlib_json);
        zragf_p89_host_release(tmp);
        zragf_p89_host_release(zragf_out);
        zragf_p89_host_release(zlib_out);
        zragf_p89_host_release(mixed);
        zragf_p89_host_release(json);
        return 1;
    }

    printf("phase29 step29 ok mixed=%lu/%lu json=%lu/%lu\n",
           (unsigned long)zragf_mixed, (unsigned long)zlib_mixed,
           (unsigned long)zragf_json, (unsigned long)zlib_json);
    zragf_p89_host_release(tmp);
    zragf_p89_host_release(zragf_out);
    zragf_p89_host_release(zlib_out);
    zragf_p89_host_release(mixed);
    zragf_p89_host_release(json);
    return 0;
}
