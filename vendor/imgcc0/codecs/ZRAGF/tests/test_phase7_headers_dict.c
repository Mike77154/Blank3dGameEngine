#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "zragflib.h"
#include "protocol89_hostmem.h"

typedef struct {
    unsigned char *data;
    size_t size;
    size_t cap;
} buffer;

static int buf_reserve(buffer *b, size_t extra)
{
    unsigned char *p;
    size_t need;
    size_t newcap;

    if (extra == 0u)
        return 1;
    if (b->size > ((size_t)-1) - extra)
        return 0;
    need = b->size + extra;
    if (need <= b->cap)
        return 1;
    newcap = b->cap ? b->cap : 4096u;
    while (newcap < need) {
        if (newcap > ((size_t)-1) / 2u) {
            newcap = need;
            break;
        }
        newcap *= 2u;
    }
    p = (unsigned char *)zragf_p89_host_resize(b->data, newcap);
    if (!p)
        return 0;
    b->data = p;
    b->cap = newcap;
    return 1;
}

static int __attribute__((unused)) zragf_unused_buf_append(buffer *b, const unsigned char *src, size_t n)
{
    if (!buf_reserve(b, n))
        return 0;
    if (n > 0u) {
        memcpy(b->data + b->size, src, n);
        b->size += n;
    }
    return 1;
}

static void buf_free(buffer *b)
{
    zragf_p89_host_release(b->data);
    b->data = NULL;
    b->size = 0u;
    b->cap = 0u;
}

static int zragf_compress_all(int windowBits,
                              const unsigned char *src,
                              size_t src_size,
                              const unsigned char *dict,
                              unsigned int dict_len,
                              const zragf_gz_header *hdr,
                              buffer *out)
{
    zragf_stream zs;
    size_t bound;
    int rc;

    memset(&zs, 0, sizeof(zs));
    if (zragf_deflateInit2(&zs, ZRAGF_LEVEL_DEFAULT, 8, windowBits, 8, ZRAGF_Z_DEFAULT_STRATEGY) != ZRAGF_OK)
        return 0;
    if (dict_len > 0u) {
        rc = zragf_deflateSetDictionary(&zs, dict, dict_len);
        if (rc != ZRAGF_OK) {
            zragf_deflateEndZ(&zs);
            return 0;
        }
    }
    if (hdr) {
        rc = zragf_deflateSetHeader(&zs, (zragf_gz_headerp)hdr);
        if (rc != ZRAGF_OK) {
            zragf_deflateEndZ(&zs);
            return 0;
        }
    }

    bound = zragf_deflateBound(&zs, (unsigned long)src_size) + 256u;
    out->data = (unsigned char *)zragf_p89_host_take(bound);
    if (!out->data) {
        zragf_deflateEndZ(&zs);
        return 0;
    }
    out->cap = bound;

    zs.next_in = (unsigned char *)src;
    zs.avail_in = src_size;
    zs.next_out = out->data;
    zs.avail_out = out->cap;

    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    if (rc != ZRAGF_STREAM_END) {
        zragf_deflateEndZ(&zs);
        buf_free(out);
        return 0;
    }
    out->size = out->cap - zs.avail_out;
    zragf_deflateEndZ(&zs);
    return 1;
}

static int zlib_compress_with_dict(const unsigned char *src, size_t src_size,
                                   const unsigned char *dict, unsigned int dict_len,
                                   buffer *out)
{
    z_stream zs;
    int rc;
    uLong bound;

    memset(&zs, 0, sizeof(zs));
    bound = compressBound((uLong)src_size) + 256u;
    out->data = (unsigned char *)zragf_p89_host_take((size_t)bound);
    if (!out->data)
        return 0;
    out->cap = (size_t)bound;

    rc = deflateInit(&zs, Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK) {
        buf_free(out);
        return 0;
    }
    rc = deflateSetDictionary(&zs, dict, dict_len);
    if (rc != Z_OK) {
        deflateEnd(&zs);
        buf_free(out);
        return 0;
    }

    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_size;
    zs.next_out = out->data;
    zs.avail_out = (uInt)out->cap;
    rc = deflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) {
        deflateEnd(&zs);
        buf_free(out);
        return 0;
    }
    out->size = out->cap - (size_t)zs.avail_out;
    deflateEnd(&zs);
    return 1;
}

static int zlib_gzip_with_header(const unsigned char *src, size_t src_size,
                                 const gz_header *hdr,
                                 buffer *out)
{
    z_stream zs;
    int rc;
    uLong bound;

    memset(&zs, 0, sizeof(zs));
    bound = compressBound((uLong)src_size) + 512u;
    out->data = (unsigned char *)zragf_p89_host_take((size_t)bound);
    if (!out->data)
        return 0;
    out->cap = (size_t)bound;

    rc = deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 31, 8, Z_DEFAULT_STRATEGY);
    if (rc != Z_OK) {
        buf_free(out);
        return 0;
    }
    rc = deflateSetHeader(&zs, (gz_headerp)hdr);
    if (rc != Z_OK) {
        deflateEnd(&zs);
        buf_free(out);
        return 0;
    }
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_size;
    zs.next_out = out->data;
    zs.avail_out = (uInt)out->cap;
    rc = deflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) {
        deflateEnd(&zs);
        buf_free(out);
        return 0;
    }
    out->size = out->cap - (size_t)zs.avail_out;
    deflateEnd(&zs);
    return 1;
}

static int test_external_dict_to_zragf(void)
{
    static const unsigned char dict[] = "phase7-dictionary: lorem ipsum dolor sit amet ";
    static const unsigned char src[] = "lorem ipsum dolor sit amet -- lorem ipsum dolor sit amet -- repeated repeated repeated";
    buffer comp = {0};
    zragf_stream is;
    unsigned char out[256];
    int rc;
    size_t got;

    if (!zlib_compress_with_dict(src, sizeof(src) - 1u, dict, (unsigned int)(sizeof(dict) - 1u), &comp))
        return 0;

    memset(&is, 0, sizeof(is));
    if (zragf_inflateInit2(&is, 15) != ZRAGF_OK) {
        buf_free(&comp);
        return 0;
    }

    is.next_in = comp.data;
    is.avail_in = comp.size;
    is.next_out = out;
    is.avail_out = sizeof(out);
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    if (rc != ZRAGF_NEED_DICT || is.adler != adler32(1L, dict, (uInt)(sizeof(dict) - 1u))) {
        zragf_inflateEndZ(&is);
        buf_free(&comp);
        return 0;
    }

    rc = zragf_inflateSetDictionary(&is, dict, (unsigned int)(sizeof(dict) - 1u));
    if (rc != ZRAGF_OK) {
        zragf_inflateEndZ(&is);
        buf_free(&comp);
        return 0;
    }

    is.next_in = NULL;
    is.avail_in = 0u;
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    got = sizeof(out) - is.avail_out;
    zragf_inflateEndZ(&is);
    buf_free(&comp);
    return rc == ZRAGF_STREAM_END && got == sizeof(src) - 1u && memcmp(out, src, got) == 0;
}

static int test_zragf_dict_to_external(void)
{
    static const unsigned char dict[] = "phase7-dictionary: lorem ipsum dolor sit amet ";
    static const unsigned char src[] = "lorem ipsum dolor sit amet -- lorem ipsum dolor sit amet -- repeated repeated repeated";
    buffer comp = {0};
    z_stream zs;
    unsigned char out[256];
    int rc;
    size_t got;

    if (!zragf_compress_all(15, src, sizeof(src) - 1u, dict, (unsigned int)(sizeof(dict) - 1u), NULL, &comp))
        return 0;

    memset(&zs, 0, sizeof(zs));
    if (inflateInit(&zs) != Z_OK) {
        buf_free(&comp);
        return 0;
    }
    zs.next_in = comp.data;
    zs.avail_in = (uInt)comp.size;
    zs.next_out = out;
    zs.avail_out = sizeof(out);
    rc = inflate(&zs, Z_FINISH);
    if (rc != Z_NEED_DICT || zs.adler != adler32(1L, dict, (uInt)(sizeof(dict) - 1u))) {
        inflateEnd(&zs);
        buf_free(&comp);
        return 0;
    }
    rc = inflateSetDictionary(&zs, dict, (uInt)(sizeof(dict) - 1u));
    if (rc != Z_OK) {
        inflateEnd(&zs);
        buf_free(&comp);
        return 0;
    }
    rc = inflate(&zs, Z_FINISH);
    got = sizeof(out) - zs.avail_out;
    inflateEnd(&zs);
    buf_free(&comp);
    return rc == Z_STREAM_END && got == sizeof(src) - 1u && memcmp(out, src, got) == 0;
}

static int test_zragf_gzip_header_to_external(void)
{
    static const unsigned char src[] = "phase7 gzip metadata from zragf";
    static const unsigned char extra[] = {0x41u, 0x42u, 0x43u, 0x44u};
    static const unsigned char name[] = "phase7.txt";
    static const unsigned char comment[] = "hola-metadata";
    zragf_gz_header hdr;
    buffer comp = {0};
    z_stream zs;
    gz_header gh;
    unsigned char extra_buf[16];
    unsigned char name_buf[64];
    unsigned char comment_buf[64];
    unsigned char out[256];
    int rc;
    size_t got;

    memset(&hdr, 0, sizeof(hdr));
    hdr.text = 1;
    hdr.time = 123456789u;
    hdr.os = 3;
    hdr.hcrc = 1;
    hdr.extra = (unsigned char *)extra;
    hdr.extra_len = (unsigned int)sizeof(extra);
    hdr.name = (unsigned char *)name;
    hdr.comment = (unsigned char *)comment;

    if (!zragf_compress_all(31, src, sizeof(src) - 1u, NULL, 0u, &hdr, &comp))
        return 0;

    memset(&zs, 0, sizeof(zs));
    memset(&gh, 0, sizeof(gh));
    gh.extra = extra_buf; gh.extra_max = sizeof(extra_buf);
    gh.name = name_buf; gh.name_max = sizeof(name_buf);
    gh.comment = comment_buf; gh.comm_max = sizeof(comment_buf);

    if (inflateInit2(&zs, 31) != Z_OK) {
        buf_free(&comp);
        return 0;
    }
    if (inflateGetHeader(&zs, &gh) != Z_OK) {
        inflateEnd(&zs);
        buf_free(&comp);
        return 0;
    }
    zs.next_in = comp.data;
    zs.avail_in = (uInt)comp.size;
    zs.next_out = out;
    zs.avail_out = sizeof(out);
    rc = inflate(&zs, Z_FINISH);
    got = sizeof(out) - zs.avail_out;
    inflateEnd(&zs);
    buf_free(&comp);

    return rc == Z_STREAM_END && gh.done == 1 && gh.text == 1 && gh.time == 123456789u &&
           gh.os == 3 && gh.hcrc == 1 && gh.extra_len == sizeof(extra) &&
           memcmp(extra_buf, extra, sizeof(extra)) == 0 && strcmp((char *)name_buf, (const char *)name) == 0 &&
           strcmp((char *)comment_buf, (const char *)comment) == 0 &&
           got == sizeof(src) - 1u && memcmp(out, src, got) == 0;
}

static int test_external_gzip_header_to_zragf(void)
{
    static const unsigned char src[] = "phase7 gzip metadata from zlib";
    static const unsigned char extra[] = {0x11u, 0x22u, 0x33u};
    static const unsigned char name[] = "external.txt";
    static const unsigned char comment[] = "from-zlib";
    gz_header hdr;
    buffer comp = {0};
    zragf_stream is;
    zragf_gz_header gh;
    unsigned char extra_buf[16];
    unsigned char name_buf[64];
    unsigned char comment_buf[64];
    unsigned char out[256];
    int rc;
    size_t got;

    memset(&hdr, 0, sizeof(hdr));
    hdr.text = 1;
    hdr.time = 987654321u;
    hdr.os = 13;
    hdr.hcrc = 1;
    hdr.extra = (Bytef *)extra;
    hdr.extra_len = sizeof(extra);
    hdr.name = (Bytef *)name;
    hdr.comment = (Bytef *)comment;

    if (!zlib_gzip_with_header(src, sizeof(src) - 1u, &hdr, &comp))
        return 0;

    memset(&is, 0, sizeof(is));
    memset(&gh, 0, sizeof(gh));
    gh.extra = extra_buf; gh.extra_max = sizeof(extra_buf);
    gh.name = name_buf; gh.name_max = sizeof(name_buf);
    gh.comment = comment_buf; gh.comm_max = sizeof(comment_buf);

    if (zragf_inflateInit2(&is, 47) != ZRAGF_OK) {
        buf_free(&comp);
        return 0;
    }
    if (zragf_inflateGetHeader(&is, &gh) != ZRAGF_OK) {
        zragf_inflateEndZ(&is);
        buf_free(&comp);
        return 0;
    }
    is.next_in = comp.data;
    is.avail_in = comp.size;
    is.next_out = out;
    is.avail_out = sizeof(out);
    rc = zragf_inflateZ(&is, ZRAGF_FINISH);
    got = sizeof(out) - is.avail_out;
    zragf_inflateEndZ(&is);
    buf_free(&comp);

    return rc == ZRAGF_STREAM_END && gh.done == 1 && gh.text == 1 && gh.time == 987654321u &&
           gh.os == 13 && gh.hcrc == 1 && gh.extra_len == sizeof(extra) &&
           memcmp(extra_buf, extra, sizeof(extra)) == 0 && strcmp((char *)name_buf, (const char *)name) == 0 &&
           strcmp((char *)comment_buf, (const char *)comment) == 0 &&
           got == sizeof(src) - 1u && memcmp(out, src, got) == 0;
}

int main(void)
{
    if (!test_external_dict_to_zragf()) {
        fprintf(stderr, "phase7: external dict -> zragf failed\n");
        return 1;
    }
    if (!test_zragf_dict_to_external()) {
        fprintf(stderr, "phase7: zragf dict -> external failed\n");
        return 1;
    }
    if (!test_zragf_gzip_header_to_external()) {
        fprintf(stderr, "phase7: zragf gzip header -> external failed\n");
        return 1;
    }
    if (!test_external_gzip_header_to_zragf()) {
        fprintf(stderr, "phase7: external gzip header -> zragf failed\n");
        return 1;
    }
    puts("phase7 headers+dict ok");
    return 0;
}
