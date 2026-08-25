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

static int buf_append(buffer *b, const unsigned char *src, size_t n)
{
    unsigned char *p;
    size_t newcap;

    if (n == 0u)
        return 1;
    if (b->size + n < b->size)
        return 0;
    if (b->size + n > b->cap) {
        newcap = (b->cap == 0u) ? 4096u : b->cap;
        while (newcap < b->size + n) {
            if (newcap > ((size_t)-1) / 2u) {
                newcap = b->size + n;
                break;
            }
            newcap *= 2u;
        }
        p = (unsigned char *)zragf_p89_host_resize(b->data, newcap);
        if (!p)
            return 0;
        b->data = p;
        b->cap = newcap;
    }
    memcpy(b->data + b->size, src, n);
    b->size += n;
    return 1;
}

static int inflate_external(const unsigned char *src, size_t src_size,
                            int windowBits,
                            const unsigned char *want, size_t want_size)
{
    z_stream zs;
    unsigned char *out;
    int rc;
    size_t out_cap = want_size + 1024u;
    int ok = 0;

    out = (unsigned char *)zragf_p89_host_take(out_cap);
    if (!out)
        return 0;

    memset(&zs, 0, sizeof(zs));
    rc = inflateInit2(&zs, windowBits);
    if (rc != Z_OK) {
        zragf_p89_host_release(out);
        return 0;
    }

    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_size;
    zs.next_out = out;
    zs.avail_out = (uInt)out_cap;
    rc = inflate(&zs, Z_FINISH);
    if (rc == Z_STREAM_END) {
        size_t got = out_cap - (size_t)zs.avail_out;
        if (got == want_size && memcmp(out, want, want_size) == 0)
            ok = 1;
    }

    inflateEnd(&zs);
    zragf_p89_host_release(out);
    return ok;
}

static int drive_streaming_case(int windowBits)
{
    zragf_stream zs;
    buffer comp;
    unsigned char out_chunk[97];
    unsigned char *src;
    size_t src_size = 180000u;
    size_t pos = 0u;
    size_t pre_finish_out = 0u;
    int rc;
    int finish_seen = 0;
    size_t i;

    memset(&zs, 0, sizeof(zs));
    memset(&comp, 0, sizeof(comp));

    src = (unsigned char *)zragf_p89_host_take(src_size);
    if (!src)
        return 0;

    for (i = 0u; i < src_size; ++i) {
        static const unsigned char pat[] = "streaming-phase5-abcdef-0123456789-";
        src[i] = pat[(i + (i / 97u)) % (sizeof(pat) - 1u)];
    }

    rc = zragf_deflateInit2(&zs, 6, 8, windowBits, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK) {
        zragf_p89_host_release(src);
        return 0;
    }

    while (pos + 12000u < src_size) {
        size_t chunk = 5000u + (pos % 7000u);
        size_t last_total_out;
        size_t produced;
        size_t consumed;

        if (pos + chunk > src_size - 12000u)
            chunk = (src_size - 12000u) - pos;

        zs.next_in = src + pos;
        zs.avail_in = chunk;

        for (;;) {
            last_total_out = zs.total_out;
            zs.next_out = out_chunk;
            zs.avail_out = sizeof(out_chunk);
            {
                size_t before_in = zs.total_in;
                rc = zragf_deflateZ(&zs, ZRAGF_NO_FLUSH);
                produced = zs.total_out - last_total_out;
                consumed = zs.total_in - before_in;
            }
            if (rc < 0) {
                zragf_deflateEndZ(&zs);
                zragf_p89_host_release(src);
                zragf_p89_host_release(comp.data);
                return 0;
            }
            if (!buf_append(&comp, out_chunk, produced)) {
                zragf_deflateEndZ(&zs);
                zragf_p89_host_release(src);
                zragf_p89_host_release(comp.data);
                return 0;
            }
            if (zs.avail_in != 0u) {
                zragf_deflateEndZ(&zs);
                zragf_p89_host_release(src);
                zragf_p89_host_release(comp.data);
                return 0;
            }
            if (produced == 0u && consumed == 0u)
                break;
        }

        pos += chunk;
        pre_finish_out = comp.size;
    }

    if (pre_finish_out == 0u) {
        zragf_deflateEndZ(&zs);
        zragf_p89_host_release(src);
        zragf_p89_host_release(comp.data);
        return 0;
    }

    zs.next_in = src + pos;
    zs.avail_in = src_size - pos;
    while (!finish_seen) {
        size_t last_total_out = zs.total_out;
        size_t produced;
        size_t consumed;
        size_t before_in = zs.total_in;

        zs.next_out = out_chunk;
        zs.avail_out = sizeof(out_chunk);
        rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
        produced = zs.total_out - last_total_out;
        consumed = zs.total_in - before_in;

        if (rc < 0) {
            zragf_deflateEndZ(&zs);
            zragf_p89_host_release(src);
            zragf_p89_host_release(comp.data);
            return 0;
        }
        if (!buf_append(&comp, out_chunk, produced)) {
            zragf_deflateEndZ(&zs);
            zragf_p89_host_release(src);
            zragf_p89_host_release(comp.data);
            return 0;
        }
        if (rc == ZRAGF_STREAM_END) {
            finish_seen = 1;
            break;
        }
        if (produced == 0u && consumed == 0u) {
            zragf_deflateEndZ(&zs);
            zragf_p89_host_release(src);
            zragf_p89_host_release(comp.data);
            return 0;
        }
    }

    zragf_deflateEndZ(&zs);

    if (!inflate_external(comp.data,
                          comp.size,
                          windowBits,
                          src,
                          src_size)) {
        zragf_p89_host_release(src);
        zragf_p89_host_release(comp.data);
        return 0;
    }

    zragf_p89_host_release(src);
    zragf_p89_host_release(comp.data);
    return 1;
}

int main(void)
{
    if (!drive_streaming_case(-15)) {
        fprintf(stderr, "phase5 raw streaming failed\n");
        return 1;
    }
    if (!drive_streaming_case(15)) {
        fprintf(stderr, "phase5 zlib streaming failed\n");
        return 2;
    }
    if (!drive_streaming_case(31)) {
        fprintf(stderr, "phase5 gzip streaming failed\n");
        return 3;
    }
    puts("phase5 streaming ok");
    return 0;
}
