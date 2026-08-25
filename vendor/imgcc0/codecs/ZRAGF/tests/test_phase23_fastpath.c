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

static size_t zragf_raw_compress(const unsigned char *src,
                                 size_t src_len,
                                 size_t chunk,
                                 unsigned char **out_buf)
{
    zragf_stream zs;
    unsigned char *out;
    size_t cap = src_len * 2u + 1024u;
    size_t off = 0u;
    int rc;

    out = (unsigned char *)zragf_p89_host_take(cap);
    assert(out != NULL);
    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    assert(rc == ZRAGF_OK);
    zs.next_out = out;
    zs.avail_out = cap;

    if (chunk == 0u)
        chunk = src_len;

    while (off < src_len) {
        size_t take = chunk;
        if (take > src_len - off)
            take = src_len - off;
        zs.next_in = (unsigned char *)(src + off);
        zs.avail_in = take;
        rc = zragf_deflateZ(&zs, ZRAGF_NO_FLUSH);
        assert(rc == ZRAGF_OK);
        off += take;
    }

    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    assert(rc == ZRAGF_STREAM_END);
    *out_buf = out;
    cap = zs.total_out;
    zragf_deflateEndZ(&zs);
    return cap;
}

static void assert_zlib_raw_roundtrip(const unsigned char *compressed,
                                      size_t compressed_len,
                                      const unsigned char *original,
                                      size_t original_len)
{
    z_stream zs;
    unsigned char *out;
    int rc;

    out = (unsigned char *)zragf_p89_host_take(original_len);
    assert(out != NULL);
    memset(&zs, 0, sizeof(zs));
    rc = inflateInit2(&zs, -15);
    assert(rc == Z_OK);
    zs.next_in = (Bytef *)compressed;
    zs.avail_in = (uInt)compressed_len;
    zs.next_out = out;
    zs.avail_out = (uInt)original_len;
    rc = inflate(&zs, Z_FINISH);
    assert(rc == Z_STREAM_END);
    assert(zs.total_out == original_len);
    assert(memcmp(out, original, original_len) == 0);
    inflateEnd(&zs);
    zragf_p89_host_release(out);
}

int main(void)
{
    size_t json_len = 0u, log_len = 0u;
    unsigned char *jsonish = make_jsonish(&json_len);
    unsigned char *logish = make_logish(&log_len);
    unsigned char *json_one = NULL, *json_chunk = NULL;
    unsigned char *log_one = NULL, *log_chunk = NULL;
    size_t json_one_len, json_chunk_len, log_one_len, log_chunk_len;

    json_one_len = zragf_raw_compress(jsonish, json_len, 0u, &json_one);
    json_chunk_len = zragf_raw_compress(jsonish, json_len, 4096u, &json_chunk);
    log_one_len = zragf_raw_compress(logish, log_len, 0u, &log_one);
    log_chunk_len = zragf_raw_compress(logish, log_len, 4096u, &log_chunk);

    assert(json_one_len <= 5100u);
    assert(json_chunk_len <= 5100u);
    assert(log_one_len <= 11680u);
    assert(log_chunk_len <= 11680u);

    assert_zlib_raw_roundtrip(json_one, json_one_len, jsonish, json_len);
    assert_zlib_raw_roundtrip(json_chunk, json_chunk_len, jsonish, json_len);
    assert_zlib_raw_roundtrip(log_one, log_one_len, logish, log_len);
    assert_zlib_raw_roundtrip(log_chunk, log_chunk_len, logish, log_len);

    printf("phase23 fastpath ok json=%lu/%lu log=%lu/%lu\n",
           (unsigned long)json_one_len, (unsigned long)json_chunk_len,
           (unsigned long)log_one_len, (unsigned long)log_chunk_len);

    zragf_p89_host_release(jsonish);
    zragf_p89_host_release(logish);
    zragf_p89_host_release(json_one);
    zragf_p89_host_release(json_chunk);
    zragf_p89_host_release(log_one);
    zragf_p89_host_release(log_chunk);
    return 0;
}
