#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"

static int inflate_external_raw(const unsigned char *src, size_t src_size,
                                unsigned char *dst, size_t dst_cap,
                                size_t *dst_size)
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
    zs.avail_out = (uInt)dst_cap;
    rc = inflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) {
        inflateEnd(&zs);
        return 0;
    }
    *dst_size = (size_t)zs.total_out;
    inflateEnd(&zs);
    return 1;
}

static int inflate_external_zlib_dict(const unsigned char *src, size_t src_size,
                                      const unsigned char *dict, size_t dict_size,
                                      unsigned char *dst, size_t dst_cap,
                                      size_t *dst_size)
{
    z_stream zs;
    int rc;

    memset(&zs, 0, sizeof(zs));
    rc = inflateInit(&zs);
    if (rc != Z_OK)
        return 0;
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_size;
    zs.next_out = dst;
    zs.avail_out = (uInt)dst_cap;
    rc = inflate(&zs, Z_FINISH);
    if (rc == Z_NEED_DICT)
        rc = inflateSetDictionary(&zs, dict, (uInt)dict_size);
    if (rc == Z_OK)
        rc = inflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) {
        inflateEnd(&zs);
        return 0;
    }
    *dst_size = (size_t)zs.total_out;
    inflateEnd(&zs);
    return 1;
}

static int test_streaming_backend_ratio(void)
{
    zragf_stream zs;
    unsigned char out_chunk[257];
    unsigned char *input;
    unsigned char *comp;
    unsigned char *decoded;
    size_t input_size = 1024u * 1024u;
    size_t comp_cap = input_size * 2u + 1024u;
    size_t comp_size = 0u;
    size_t decoded_size = 0u;
    size_t pos = 0u;
    int rc;
    size_t i;

    input = (unsigned char *)zragf_p89_host_take(input_size);
    comp = (unsigned char *)zragf_p89_host_take(comp_cap);
    decoded = (unsigned char *)zragf_p89_host_take(input_size + 64u);
    if (!input || !comp || !decoded) {
        zragf_p89_host_release(input);
        zragf_p89_host_release(comp);
        zragf_p89_host_release(decoded);
        return 0;
    }

    for (i = 0u; i < input_size; ++i) {
        static const unsigned char pat[] =
            "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        input[i] = pat[i % (sizeof(pat) - 1u)];
    }

    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, -15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK) {
        zragf_p89_host_release(input);
        zragf_p89_host_release(comp);
        zragf_p89_host_release(decoded);
        return 0;
    }

    while (pos < input_size) {
        size_t take = (input_size - pos > 4096u) ? 4096u : (input_size - pos);
        zs.next_in = input + pos;
        zs.avail_in = (unsigned int)take;
        do {
            size_t before = zs.total_out;
            zs.next_out = out_chunk;
            zs.avail_out = sizeof(out_chunk);
            rc = zragf_deflateZ(&zs, ZRAGF_NO_FLUSH);
            if (rc < 0) {
                zragf_deflateEndZ(&zs);
                zragf_p89_host_release(input);
                zragf_p89_host_release(comp);
                zragf_p89_host_release(decoded);
                return 0;
            }
            if (comp_size + (zs.total_out - before) > comp_cap) {
                zragf_deflateEndZ(&zs);
                zragf_p89_host_release(input);
                zragf_p89_host_release(comp);
                zragf_p89_host_release(decoded);
                return 0;
            }
            memcpy(comp + comp_size, out_chunk, zs.total_out - before);
            comp_size += zs.total_out - before;
        } while (zs.avail_in > 0u || zs.avail_out == 0u);
        pos += take;
    }

    for (;;) {
        size_t before = zs.total_out;
        zs.next_out = out_chunk;
        zs.avail_out = sizeof(out_chunk);
        rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
        if (rc < 0) {
            zragf_deflateEndZ(&zs);
            zragf_p89_host_release(input);
            zragf_p89_host_release(comp);
            zragf_p89_host_release(decoded);
            return 0;
        }
        if (comp_size + (zs.total_out - before) > comp_cap) {
            zragf_deflateEndZ(&zs);
            zragf_p89_host_release(input);
            zragf_p89_host_release(comp);
            zragf_p89_host_release(decoded);
            return 0;
        }
        memcpy(comp + comp_size, out_chunk, zs.total_out - before);
        comp_size += zs.total_out - before;
        if (rc == ZRAGF_STREAM_END)
            break;
    }
    zragf_deflateEndZ(&zs);

    if (comp_size >= 6000u) {
        fprintf(stderr, "streaming ratio regressed: got %zu bytes\n", comp_size);
        zragf_p89_host_release(input);
        zragf_p89_host_release(comp);
        zragf_p89_host_release(decoded);
        return 0;
    }

    if (!inflate_external_raw(comp, comp_size, decoded, input_size + 64u, &decoded_size) ||
        decoded_size != input_size || memcmp(decoded, input, input_size) != 0) {
        fprintf(stderr, "streaming raw output not externally decodable\n");
        zragf_p89_host_release(input);
        zragf_p89_host_release(comp);
        zragf_p89_host_release(decoded);
        return 0;
    }

    zragf_p89_host_release(input);
    zragf_p89_host_release(comp);
    zragf_p89_host_release(decoded);
    return 1;
}

static int test_dictionary_backend_effective(void)
{
    static const unsigned char dict[] =
        "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Vestibulum vulputate.";
    unsigned char input[512];
    unsigned char comp_nodict[4096];
    unsigned char comp_dict[4096];
    unsigned char decoded[1024];
    size_t input_size = 0u;
    size_t decoded_size = 0u;
    zragf_stream zs;
    int rc;
    size_t nodict_size;
    size_t dict_size;
    int i;

    for (i = 0; i < 6; ++i) {
        memcpy(input + input_size, dict, sizeof(dict) - 1u);
        input_size += sizeof(dict) - 1u;
    }

    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, 15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK)
        return 0;
    zs.next_in = input;
    zs.avail_in = (unsigned int)input_size;
    zs.next_out = comp_nodict;
    zs.avail_out = sizeof(comp_nodict);
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    nodict_size = sizeof(comp_nodict) - zs.avail_out;
    zragf_deflateEndZ(&zs);
    if (rc != ZRAGF_STREAM_END)
        return 0;

    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 6, 8, 15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK)
        return 0;
    rc = zragf_deflateSetDictionary(&zs, dict, (unsigned int)(sizeof(dict) - 1u));
    if (rc != ZRAGF_OK) {
        zragf_deflateEndZ(&zs);
        return 0;
    }
    zs.next_in = input;
    zs.avail_in = (unsigned int)input_size;
    zs.next_out = comp_dict;
    zs.avail_out = sizeof(comp_dict);
    rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
    dict_size = sizeof(comp_dict) - zs.avail_out;
    zragf_deflateEndZ(&zs);
    if (rc != ZRAGF_STREAM_END)
        return 0;

    if (!(dict_size < nodict_size)) {
        fprintf(stderr, "preset dictionary not reducing output: no=%zu dict=%zu\n",
                nodict_size, dict_size);
        return 0;
    }

    if (!inflate_external_zlib_dict(comp_dict, dict_size,
                                    dict, sizeof(dict) - 1u,
                                    decoded, sizeof(decoded), &decoded_size) ||
        decoded_size != input_size || memcmp(decoded, input, input_size) != 0) {
        fprintf(stderr, "externally decoding preset dictionary stream failed\n");
        return 0;
    }
    return 1;
}

int main(void)
{
    if (!test_streaming_backend_ratio())
        return 1;
    if (!test_dictionary_backend_effective())
        return 1;
    puts("phase10 backend running ok");
    return 0;
}
