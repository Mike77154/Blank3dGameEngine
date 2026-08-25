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

static int deflate_external(const unsigned char *src, size_t src_size,
                            int windowBits,
                            unsigned char **out_data,
                            size_t *out_size)
{
    z_stream zs;
    unsigned char *out;
    size_t out_cap = compressBound((uLong)src_size) + 256u;
    int rc;

    memset(&zs, 0, sizeof(zs));
    out = (unsigned char *)zragf_p89_host_take(out_cap);
    if (!out)
        return 0;

    rc = deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED,
                      windowBits, 8, Z_DEFAULT_STRATEGY);
    if (rc != Z_OK) {
        zragf_p89_host_release(out);
        return 0;
    }

    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_size;
    zs.next_out = out;
    zs.avail_out = (uInt)out_cap;

    rc = deflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) {
        deflateEnd(&zs);
        zragf_p89_host_release(out);
        return 0;
    }

    *out_size = out_cap - (size_t)zs.avail_out;
    *out_data = out;
    deflateEnd(&zs);
    return 1;
}

static int drive_incremental_case(int windowBits)
{
    unsigned char *src;
    unsigned char *comp = NULL;
    size_t src_size = 160000u;
    size_t comp_size = 0u;
    size_t pos;
    size_t step_idx;
    buffer out;
    zragf_stream is;
    int saw_pre_finish_output = 0;
    int rc;

    memset(&out, 0, sizeof(out));
    memset(&is, 0, sizeof(is));

    src = (unsigned char *)zragf_p89_host_take(src_size);
    if (!src)
        return 0;

    for (pos = 0u; pos < src_size; ++pos) {
        static const unsigned char pat[] =
            "phase5-incremental-inflate-abcdefghijklmnopqrstuvwxyz-0123456789-";
        src[pos] = pat[(pos + (pos / 97u) + (pos / 791u)) % (sizeof(pat) - 1u)];
        if ((pos % 257u) == 0u)
            src[pos] = (unsigned char)(pos & 0xFFu);
    }

    if (!deflate_external(src, src_size, windowBits, &comp, &comp_size)) {
        zragf_p89_host_release(src);
        return 0;
    }

    rc = zragf_inflateInit2(&is, windowBits);
    if (rc != ZRAGF_OK) {
        zragf_p89_host_release(comp);
        zragf_p89_host_release(src);
        return 0;
    }

    pos = 0u;
    step_idx = 0u;
    while (pos < comp_size) {
        size_t chunk = 7u + (step_idx % 19u);
        if (pos + chunk > comp_size)
            chunk = comp_size - pos;

        is.next_in = comp + pos;
        is.avail_in = chunk;

        for (;;) {
            unsigned char tmp[29];
            zragf_u32 prev_in = is.total_in;
            zragf_u32 prev_out = is.total_out;
            size_t produced;

            is.next_out = tmp;
            is.avail_out = sizeof(tmp);
            rc = zragf_inflateZ(&is, ZRAGF_NO_FLUSH);
            produced = (size_t)(is.total_out - prev_out);
            if (rc < 0) {
                zragf_inflateEndZ(&is);
                zragf_p89_host_release(comp);
                zragf_p89_host_release(src);
                zragf_p89_host_release(out.data);
                return 0;
            }
            if (!buf_append(&out, tmp, produced)) {
                zragf_inflateEndZ(&is);
                zragf_p89_host_release(comp);
                zragf_p89_host_release(src);
                zragf_p89_host_release(out.data);
                return 0;
            }
            if (produced > 0u)
                saw_pre_finish_output = 1;
            if (rc == ZRAGF_STREAM_END)
                break;
            if (produced == 0u && is.total_in == prev_in && is.total_out == prev_out)
                break;
        }

        pos += chunk;
        step_idx++;
    }

    for (;;) {
        unsigned char tmp[23];
        zragf_u32 prev_in = is.total_in;
        zragf_u32 prev_out = is.total_out;
        size_t produced;

        is.next_in = NULL;
        is.avail_in = 0u;
        is.next_out = tmp;
        is.avail_out = sizeof(tmp);
        rc = zragf_inflateZ(&is, ZRAGF_FINISH);
        produced = (size_t)(is.total_out - prev_out);
        if (rc < 0) {
            zragf_inflateEndZ(&is);
            zragf_p89_host_release(comp);
            zragf_p89_host_release(src);
            zragf_p89_host_release(out.data);
            return 0;
        }
        if (!buf_append(&out, tmp, produced)) {
            zragf_inflateEndZ(&is);
            zragf_p89_host_release(comp);
            zragf_p89_host_release(src);
            zragf_p89_host_release(out.data);
            return 0;
        }
        if (rc == ZRAGF_STREAM_END)
            break;
        if (produced == 0u && is.total_in == prev_in && is.total_out == prev_out) {
            zragf_inflateEndZ(&is);
            zragf_p89_host_release(comp);
            zragf_p89_host_release(src);
            zragf_p89_host_release(out.data);
            return 0;
        }
    }

    zragf_inflateEndZ(&is);

    if (!saw_pre_finish_output) {
        zragf_p89_host_release(comp);
        zragf_p89_host_release(src);
        zragf_p89_host_release(out.data);
        return 0;
    }
    if (out.size != src_size || memcmp(out.data, src, src_size) != 0) {
        zragf_p89_host_release(comp);
        zragf_p89_host_release(src);
        zragf_p89_host_release(out.data);
        return 0;
    }

    zragf_p89_host_release(comp);
    zragf_p89_host_release(src);
    zragf_p89_host_release(out.data);
    return 1;
}

int main(void)
{
    if (!drive_incremental_case(-15)) {
        fprintf(stderr, "incremental raw inflate failed\n");
        return 1;
    }
    if (!drive_incremental_case(15)) {
        fprintf(stderr, "incremental zlib inflate failed\n");
        return 2;
    }
    if (!drive_incremental_case(31)) {
        fprintf(stderr, "incremental gzip inflate failed\n");
        return 3;
    }
    puts("phase5 incremental inflate ok");
    return 0;
}
