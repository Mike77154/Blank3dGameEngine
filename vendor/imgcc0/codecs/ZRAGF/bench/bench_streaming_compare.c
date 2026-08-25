#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"
#include "zragflib_internal.h"

static size_t zragf_chunked_raw(const unsigned char *src, size_t src_len,
                                size_t in_chunk, size_t out_chunk)
{
    zragf_stream zs;
    unsigned char *out;
    size_t out_cap = src_len * 2u + 1024u;
    size_t out_size = 0u;
    int rc;
    size_t pos = 0u;

    out = (unsigned char *)zragf_p89_host_take(out_cap);
    if (!out)
        return 0u;

    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK) {
        zragf_p89_host_release(out);
        return 0u;
    }

    while (pos < src_len) {
        size_t take = (src_len - pos > in_chunk) ? in_chunk : (src_len - pos);
        zs.next_in = (unsigned char *)src + pos;
        zs.avail_in = (unsigned int)take;
        do {
            size_t before = zs.total_out;
            zs.next_out = out + out_size;
            zs.avail_out = (unsigned int)out_chunk;
            rc = zragf_deflateZ(&zs, ZRAGF_NO_FLUSH);
            if (rc < 0) {
                zragf_deflateEndZ(&zs);
                zragf_p89_host_release(out);
                return 0u;
            }
            out_size += zs.total_out - before;
        } while (zs.avail_in > 0u || zs.avail_out == 0u);
        pos += take;
    }

    for (;;) {
        size_t before = zs.total_out;
        zs.next_out = out + out_size;
        zs.avail_out = (unsigned int)out_chunk;
        rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
        out_size += zs.total_out - before;
        if (rc == ZRAGF_STREAM_END)
            break;
        if (rc < 0) {
            zragf_deflateEndZ(&zs);
            zragf_p89_host_release(out);
            return 0u;
        }
    }

    zragf_deflateEndZ(&zs);
    zragf_p89_host_release(out);
    return out_size;
}

static size_t zlib_chunked_raw(const unsigned char *src, size_t src_len,
                               size_t in_chunk, size_t out_chunk)
{
    z_stream zs;
    unsigned char *out;
    size_t out_cap = src_len * 2u + 1024u;
    size_t out_size = 0u;
    int rc;
    size_t pos = 0u;

    out = (unsigned char *)zragf_p89_host_take(out_cap);
    if (!out)
        return 0u;

    memset(&zs, 0, sizeof(zs));
    if (deflateInit2(&zs, 6, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
        zragf_p89_host_release(out);
        return 0u;
    }

    while (pos < src_len) {
        size_t take = (src_len - pos > in_chunk) ? in_chunk : (src_len - pos);
        zs.next_in = (Bytef *)src + pos;
        zs.avail_in = (uInt)take;
        do {
            zs.next_out = out + out_size;
            zs.avail_out = (uInt)out_chunk;
            rc = deflate(&zs, Z_NO_FLUSH);
            out_size += out_chunk - (size_t)zs.avail_out;
        } while (zs.avail_in > 0u || zs.avail_out == 0u);
        pos += take;
    }

    do {
        zs.next_out = out + out_size;
        zs.avail_out = (uInt)out_chunk;
        rc = deflate(&zs, Z_FINISH);
        out_size += out_chunk - (size_t)zs.avail_out;
    } while (rc != Z_STREAM_END);

    deflateEnd(&zs);
    zragf_p89_host_release(out);
    return out_size;
}

static size_t zragf_one_shot_raw(const unsigned char *src, size_t src_len)
{
    unsigned char *out;
    size_t out_cap = src_len * 2u + 1024u;
    size_t out_size = out_cap;
    size_t result = 0u;

    out = (unsigned char *)zragf_p89_host_take(out_cap);
    if (!out)
        return 0u;
    if (zragf_deflate_rfc1951_compress(src, src_len,
                                       out, out_cap, &out_size,
                                       6, ZRAGF_Z_DEFAULT_STRATEGY,
                                       0, 0, 0, 0, 0))
        result = out_size;
    zragf_p89_host_release(out);
    return result;
}

int main(void)
{
    size_t n = 1024u * 1024u;
    unsigned char *buf = (unsigned char *)zragf_p89_host_take(n);
    static const unsigned char dict[] =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum vulputate.";
    unsigned char dict_src[512];
    size_t dict_src_len = 0u;
    size_t i;
    zragf_stream zs;
    unsigned char out[4096];
    size_t with_dict = 0u;

    if (!buf)
        return 1;
    for (i = 0u; i < n; ++i) {
        static const unsigned char pat[] =
            "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        buf[i] = pat[i % (sizeof(pat) - 1u)];
    }
    for (i = 0u; i < 6u; ++i) {
        memcpy(dict_src + dict_src_len, dict, sizeof(dict) - 1u);
        dict_src_len += sizeof(dict) - 1u;
    }

    printf("one-shot raw      : %zu\n", zragf_one_shot_raw(buf, n));
    printf("zragf chunked raw : %zu\n", zragf_chunked_raw(buf, n, 4096u, 257u));
    printf("zlib  chunked raw : %zu\n", zlib_chunked_raw(buf, n, 4096u, 257u));

    memset(&zs, 0, sizeof(zs));
    if (zragf_deflateInit2(&zs, 6, 8, 15, 8, ZRAGF_Z_DEFAULT_STRATEGY) == ZRAGF_OK &&
        zragf_deflateSetDictionary(&zs, dict, (unsigned int)(sizeof(dict) - 1u)) == ZRAGF_OK) {
        zs.next_in = dict_src;
        zs.avail_in = (unsigned int)dict_src_len;
        zs.next_out = out;
        zs.avail_out = sizeof(out);
        if (zragf_deflateZ(&zs, ZRAGF_FINISH) == ZRAGF_STREAM_END)
            with_dict = sizeof(out) - zs.avail_out;
        zragf_deflateEndZ(&zs);
    }
    printf("preset dict sample: %zu\n", with_dict);

    zragf_p89_host_release(buf);
    return 0;
}
