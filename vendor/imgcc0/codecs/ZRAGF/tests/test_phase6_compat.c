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

static void buf_free(buffer *b)
{
    zragf_p89_host_release(b->data);
    b->data = NULL;
    b->size = 0u;
    b->cap = 0u;
}

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

    newcap = (b->cap == 0u) ? 4096u : b->cap;
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

static int buf_append(buffer *b, const unsigned char *src, size_t n)
{
    if (n == 0u)
        return 1;
    if (!buf_reserve(b, n))
        return 0;
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

static int inflate_external_verify(const unsigned char *comp, size_t comp_size,
                                   int windowBits,
                                   const unsigned char *want, size_t want_size)
{
    z_stream zs;
    unsigned char *out;
    size_t out_cap = want_size + 1024u;
    int rc;
    int ok = 0;

    memset(&zs, 0, sizeof(zs));
    out = (unsigned char *)zragf_p89_host_take(out_cap);
    if (!out)
        return 0;

    rc = inflateInit2(&zs, windowBits);
    if (rc != Z_OK) {
        zragf_p89_host_release(out);
        return 0;
    }

    zs.next_in = (Bytef *)comp;
    zs.avail_in = (uInt)comp_size;
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

static int inflate_with_zragf_existing(zragf_stream *is,
                                       const unsigned char *comp,
                                       size_t comp_size,
                                       buffer *out,
                                       size_t *consumed_out)
{
    size_t pos = 0u;
    size_t step_idx = 0u;
    int rc = ZRAGF_OK;

    while (pos < comp_size) {
        size_t chunk = 5u + (step_idx % 17u);
        if (pos + chunk > comp_size)
            chunk = comp_size - pos;

        is->next_in = (unsigned char *)(comp + pos);
        is->avail_in = chunk;

        for (;;) {
            unsigned char tmp[31];
            zragf_u32 prev_in = is->total_in;
            zragf_u32 prev_out = is->total_out;
            size_t produced;

            is->next_out = tmp;
            is->avail_out = sizeof(tmp);
            rc = zragf_inflateZ(is, ZRAGF_NO_FLUSH);
            produced = (size_t)(is->total_out - prev_out);
            if (rc < 0)
                return 0;
            if (!buf_append(out, tmp, produced))
                return 0;
            if (rc == ZRAGF_STREAM_END)
                goto done;
            if (produced == 0u && is->total_in == prev_in && is->total_out == prev_out)
                break;
        }

        pos += chunk;
        step_idx++;
    }

    for (;;) {
        unsigned char tmp[31];
        zragf_u32 prev_in = is->total_in;
        zragf_u32 prev_out = is->total_out;
        size_t produced;

        is->next_in = NULL;
        is->avail_in = 0u;
        is->next_out = tmp;
        is->avail_out = sizeof(tmp);
        rc = zragf_inflateZ(is, ZRAGF_FINISH);
        produced = (size_t)(is->total_out - prev_out);
        if (rc < 0)
            return 0;
        if (!buf_append(out, tmp, produced))
            return 0;
        if (rc == ZRAGF_STREAM_END)
            break;
        if (produced == 0u && is->total_in == prev_in && is->total_out == prev_out)
            return 0;
    }

done:
    if (consumed_out)
        *consumed_out = (size_t)is->total_in;
    return 1;
}

static int inflate_with_zragf_once(int windowBits,
                                   const unsigned char *comp,
                                   size_t comp_size,
                                   const unsigned char *want,
                                   size_t want_size)
{
    zragf_stream is;
    buffer out;
    int ok;

    memset(&is, 0, sizeof(is));
    memset(&out, 0, sizeof(out));

    if (zragf_inflateInit2(&is, windowBits) != ZRAGF_OK)
        return 0;

    ok = inflate_with_zragf_existing(&is, comp, comp_size, &out, NULL)
      && out.size == want_size
      && memcmp(out.data, want, want_size) == 0;

    zragf_inflateEndZ(&is);
    buf_free(&out);
    return ok;
}

static int deflate_with_zragf_existing(zragf_stream *zs,
                                       const unsigned char *src,
                                       size_t src_size,
                                       buffer *out,
                                       size_t out_chunk_size,
                                       int *pending_seen)
{
    size_t last_total_in;
    int rc;

    if (out_chunk_size == 0u)
        return 0;

    zs->next_in = (unsigned char *)src;
    zs->avail_in = src_size;

    for (;;) {
        unsigned char *chunk;
        size_t produced;
        size_t consumed;
        size_t before_in;
        unsigned pending = 0u;
        int bits = 0;

        if (!buf_reserve(out, out_chunk_size))
            return 0;
        chunk = out->data + out->size;
        before_in = zs->total_in;
        zs->next_out = chunk;
        zs->avail_out = out_chunk_size;
        rc = zragf_deflateZ(zs, ZRAGF_FINISH);
        produced = out_chunk_size - zs->avail_out;
        consumed = zs->total_in - before_in;
        out->size += produced;

        if (zragf_deflatePending(zs, &pending, &bits) != ZRAGF_OK)
            return 0;
        if ((pending > 0u || bits > 0) && pending_seen)
            *pending_seen = 1;

        if (rc < 0)
            return 0;
        if (rc == ZRAGF_STREAM_END)
            return 1;
        if (produced == 0u && consumed == 0u)
            return 0;
        last_total_in = zs->total_in;
        (void)last_total_in;
    }
}

static void fill_pattern(unsigned char *dst, size_t size, const char *pat, size_t pat_len)
{
    size_t i;
    for (i = 0u; i < size; ++i)
        dst[i] = (unsigned char)pat[(i + (i / 29u) + (i / 997u)) % pat_len];
}

static int test_auto_zero_modes(void)
{
    unsigned char *src;
    unsigned char *zlib_stream = NULL;
    unsigned char *gzip_stream = NULL;
    size_t src_size = 150000u;
    size_t zlib_size = 0u;
    size_t gzip_size = 0u;
    int ok;

    src = (unsigned char *)zragf_p89_host_take(src_size);
    if (!src)
        return 0;
    fill_pattern(src, src_size, "phase6-auto-zero-ABCDEFGHIJKLMNOPQRSTUVWXYZ-0123456789-", 52u);

    ok = deflate_external(src, src_size, 15, &zlib_stream, &zlib_size)
      && deflate_external(src, src_size, 31, &gzip_stream, &gzip_size)
      && inflate_with_zragf_once(47, zlib_stream, zlib_size, src, src_size)
      && inflate_with_zragf_once(47, gzip_stream, gzip_size, src, src_size)
      && inflate_with_zragf_once(0, zlib_stream, zlib_size, src, src_size);

    zragf_p89_host_release(src);
    zragf_p89_host_release(zlib_stream);
    zragf_p89_host_release(gzip_stream);
    return ok;
}

static int test_deflate_bound_pending_reset(void)
{
    unsigned char *src_a;
    unsigned char *src_b;
    size_t src_a_size = 96000u;
    size_t src_b_size = 64000u;
    zragf_stream zs;
    buffer out;
    unsigned long bound;
    int pending_seen = 0;
    int ok = 0;

    src_a = (unsigned char *)zragf_p89_host_take(src_a_size);
    src_b = (unsigned char *)zragf_p89_host_take(src_b_size);
    if (!src_a || !src_b) {
        zragf_p89_host_release(src_a);
        zragf_p89_host_release(src_b);
        return 0;
    }

    fill_pattern(src_a, src_a_size, "bound-and-pending-aaaaaaaaaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbbbbbbbb-", 74u);
    fill_pattern(src_b, src_b_size, "reset-second-pass-0123456789-zyxwvutsrqponmlkjihgfedcba-", 58u);

    memset(&zs, 0, sizeof(zs));
    memset(&out, 0, sizeof(out));

    if (zragf_deflateInit2(&zs, 6, 8, 31, 8, ZRAGF_Z_DEFAULT_STRATEGY) != ZRAGF_OK)
        goto done;
    bound = zragf_deflateBound(&zs, (unsigned long)src_a_size);
    if (bound == 0UL)
        goto done;
    if (!buf_reserve(&out, (size_t)bound))
        goto done;
    if (!deflate_with_zragf_existing(&zs, src_a, src_a_size, &out, (size_t)bound, NULL))
        goto done;
    if (out.size > (size_t)bound)
        goto done;
    if (!inflate_external_verify(out.data, out.size, 47, src_a, src_a_size))
        goto done;
    if (zragf_deflateReset(&zs) != ZRAGF_OK)
        goto done;
    out.size = 0u;
    if (!deflate_with_zragf_existing(&zs, src_b, src_b_size, &out, 17u, &pending_seen))
        goto done;
    if (!pending_seen)
        goto done;
    if (!inflate_external_verify(out.data, out.size, 47, src_b, src_b_size))
        goto done;
    ok = 1;

done:
    zragf_deflateEndZ(&zs);
    buf_free(&out);
    zragf_p89_host_release(src_a);
    zragf_p89_host_release(src_b);
    return ok;
}

static int test_inflate_reset_and_reset2(void)
{
    static const unsigned char src1[] = "first-gzip-member-phase6";
    static const unsigned char src2[] = "second-gzip-member-phase6-but-different";
    static const unsigned char src3[] = "third-zlib-member-phase6";
    unsigned char *m1 = NULL;
    unsigned char *m2 = NULL;
    unsigned char *m3 = NULL;
    unsigned char *concat = NULL;
    size_t m1_size = 0u;
    size_t m2_size = 0u;
    size_t m3_size = 0u;
    size_t concat_size;
    size_t consumed1 = 0u;
    zragf_stream is;
    buffer out;
    int ok = 0;

    if (!deflate_external(src1, sizeof(src1) - 1u, 31, &m1, &m1_size)
     || !deflate_external(src2, sizeof(src2) - 1u, 31, &m2, &m2_size)
     || !deflate_external(src3, sizeof(src3) - 1u, 15, &m3, &m3_size))
        goto done;

    concat_size = m1_size + m2_size;
    concat = (unsigned char *)zragf_p89_host_take(concat_size);
    if (!concat)
        goto done;
    memcpy(concat, m1, m1_size);
    memcpy(concat + m1_size, m2, m2_size);

    memset(&is, 0, sizeof(is));
    memset(&out, 0, sizeof(out));

    if (zragf_inflateInit2(&is, 47) != ZRAGF_OK)
        goto done;
    if (!inflate_with_zragf_existing(&is, concat, concat_size, &out, &consumed1))
        goto done;
    if (out.size != sizeof(src1) - 1u || memcmp(out.data, src1, sizeof(src1) - 1u) != 0)
        goto done;
    if (consumed1 == 0u || consumed1 >= concat_size)
        goto done;

    if (zragf_inflateReset(&is) != ZRAGF_OK)
        goto done;
    out.size = 0u;
    if (!inflate_with_zragf_existing(&is, concat + consumed1, concat_size - consumed1, &out, NULL))
        goto done;
    if (out.size != sizeof(src2) - 1u || memcmp(out.data, src2, sizeof(src2) - 1u) != 0)
        goto done;

    if (zragf_inflateReset2(&is, 0) != ZRAGF_OK)
        goto done;
    out.size = 0u;
    if (!inflate_with_zragf_existing(&is, m3, m3_size, &out, NULL))
        goto done;
    if (out.size != sizeof(src3) - 1u || memcmp(out.data, src3, sizeof(src3) - 1u) != 0)
        goto done;

    ok = 1;

done:
    zragf_inflateEndZ(&is);
    buf_free(&out);
    zragf_p89_host_release(m1);
    zragf_p89_host_release(m2);
    zragf_p89_host_release(m3);
    zragf_p89_host_release(concat);
    return ok;
}

int main(void)
{
    if (!test_auto_zero_modes()) {
        fprintf(stderr, "phase6 auto/zero test failed\n");
        return 1;
    }
    if (!test_deflate_bound_pending_reset()) {
        fprintf(stderr, "phase6 bound/pending/reset test failed\n");
        return 1;
    }
    if (!test_inflate_reset_and_reset2()) {
        fprintf(stderr, "phase6 inflate reset/reset2 test failed\n");
        return 1;
    }

    printf("phase6 compat ok\n");
    return 0;
}
