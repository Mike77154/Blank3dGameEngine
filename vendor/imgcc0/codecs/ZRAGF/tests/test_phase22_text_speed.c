#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"
#include "protocol89_stdio.h"

static unsigned char *make_jsonish(size_t *out_size)
{
    static const char *frag[] = {
        "{\"id\":",
        ",\"user\":\"miguel\",\"type\":\"event\",\"ok\":true,",
        "\"payload\":\"AAAAAAAAAAAAAAAA\",\"tags\":[\"alpha\",\"beta\",\"gamma\"]}\n"
    };
    unsigned char *buf;
    size_t i = 0u;

    *out_size = 180000u;
    buf = (unsigned char *)zragf_p89_host_take(*out_size);
    assert(buf != NULL);

    while (i < *out_size) {
        char num[32];
        int len = zragf_p89_format(num, sizeof(num), "%lu", (unsigned long)i);
        size_t j;
        for (j = 0u; j < strlen(frag[0]) && i < *out_size; ++j)
            buf[i++] = (unsigned char)frag[0][j];
        for (j = 0u; j < (size_t)len && i < *out_size; ++j)
            buf[i++] = (unsigned char)num[j];
        for (j = 1u; j < 3u; ++j) {
            size_t k;
            for (k = 0u; k < strlen(frag[j]) && i < *out_size; ++k)
                buf[i++] = (unsigned char)frag[j][k];
        }
    }
    return buf;
}

static unsigned char *make_logish(size_t *out_size)
{
    unsigned char *buf;
    size_t i = 0u;

    *out_size = 200000u;
    buf = (unsigned char *)zragf_p89_host_take(*out_size);
    assert(buf != NULL);

    while (i < *out_size) {
        char line[256];
        int len = zragf_p89_format(line, sizeof(line),
                           "2026-03-09T12:%02lu:%02luZ INFO service=auth user=miguel action=login id=%08lu path=/api/v1/session status=200 latency_ms=%lu retry=0\n",
                           (unsigned long)((i / 97u) % 60u),
                           (unsigned long)((i / 53u) % 60u),
                           (unsigned long)i,
                           (unsigned long)((i / 37u) % 17u));
        int j;
        for (j = 0; j < len && i < *out_size; ++j)
            buf[i++] = (unsigned char)line[j];
    }
    return buf;
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

int main(void)
{
    size_t json_len, log_len;
    unsigned char *jsonish = make_jsonish(&json_len);
    unsigned char *logish = make_logish(&log_len);
    size_t json_zragf = zragf_raw_size(jsonish, json_len);
    size_t json_zlib = zlib_raw_size(jsonish, json_len);
    size_t log_zragf = zragf_raw_size(logish, log_len);
    size_t log_zlib = zlib_raw_size(logish, log_len);

    assert(json_zragf <= json_zlib);
    assert(log_zragf <= log_zlib);
    assert(json_zragf <= 5200u);
    assert(log_zragf <= 11600u);

    printf("phase22 text speed guard ok json=%lu/%lu log=%lu/%lu\n",
           (unsigned long)json_zragf, (unsigned long)json_zlib,
           (unsigned long)log_zragf, (unsigned long)log_zlib);

    zragf_p89_host_release(jsonish);
    zragf_p89_host_release(logish);
    return 0;
}
