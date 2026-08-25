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
    newcap = b->cap ? b->cap : 256u;
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

static int zlib_inflate_all(const unsigned char *src, size_t src_size, int windowBits,
                            unsigned char *dst, size_t dst_cap, size_t *dst_size)
{
    z_stream zs;
    int rc;

    memset(&zs, 0, sizeof(zs));
    rc = inflateInit2(&zs, windowBits);
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
    *dst_size = dst_cap - (size_t)zs.avail_out;
    inflateEnd(&zs);
    return 1;
}

static int zlib_raw_deflate(const unsigned char *src, size_t src_size, buffer *out)
{
    z_stream zs;
    int rc;
    unsigned char tmp[256];

    memset(&zs, 0, sizeof(zs));
    rc = deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    if (rc != Z_OK)
        return 0;
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_size;
    do {
        zs.next_out = tmp;
        zs.avail_out = (uInt)sizeof(tmp);
        rc = deflate(&zs, Z_FINISH);
        if (rc != Z_OK && rc != Z_STREAM_END) {
            deflateEnd(&zs);
            return 0;
        }
        if (!buf_append(out, tmp, sizeof(tmp) - (size_t)zs.avail_out)) {
            deflateEnd(&zs);
            return 0;
        }
    } while (rc != Z_STREAM_END);
    deflateEnd(&zs);
    return 1;
}

static int __attribute__((unused)) zlib_zlib_with_full_flush(const unsigned char *a, size_t a_len,
                                     const unsigned char *b, size_t b_len,
                                     buffer *out)
{
    z_stream zs;
    int rc;
    unsigned char tmp[256];
    int stage = 0;

    memset(&zs, 0, sizeof(zs));
    rc = deflateInit(&zs, Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK)
        return 0;

    zs.next_in = (Bytef *)a;
    zs.avail_in = (uInt)a_len;
    do {
        zs.next_out = tmp;
        zs.avail_out = (uInt)sizeof(tmp);
        rc = deflate(&zs, Z_FULL_FLUSH);
        if (rc != Z_OK) {
            deflateEnd(&zs);
            return 0;
        }
        if (!buf_append(out, tmp, sizeof(tmp) - (size_t)zs.avail_out)) {
            deflateEnd(&zs);
            return 0;
        }
    } while (zs.avail_in != 0u || zs.avail_out == 0u);

    zs.next_in = (Bytef *)b;
    zs.avail_in = (uInt)b_len;
    do {
        zs.next_out = tmp;
        zs.avail_out = (uInt)sizeof(tmp);
        rc = deflate(&zs, Z_FINISH);
        if (rc != Z_OK && rc != Z_STREAM_END) {
            deflateEnd(&zs);
            return 0;
        }
        if (!buf_append(out, tmp, sizeof(tmp) - (size_t)zs.avail_out)) {
            deflateEnd(&zs);
            return 0;
        }
        stage = rc;
    } while (stage != Z_STREAM_END);

    deflateEnd(&zs);
    return 1;
}

static int finish_deflater(zragf_stream *zs,
                           const unsigned char *suffix,
                           size_t suffix_len,
                           buffer *out)
{
    int rc;
    int first = 1;
    unsigned char tmp[173];
    size_t produced;

    for (;;) {
        zs->next_out = tmp;
        zs->avail_out = sizeof(tmp);
        if (first) {
            zs->next_in = (unsigned char *)suffix;
            zs->avail_in = suffix_len;
            first = 0;
        } else {
            zs->next_in = NULL;
            zs->avail_in = 0u;
        }
        rc = zragf_deflateZ(zs, ZRAGF_FINISH);
        produced = sizeof(tmp) - zs->avail_out;
        if (!buf_append(out, tmp, produced))
            return 0;
        if (rc == ZRAGF_STREAM_END)
            return 1;
        if (rc != ZRAGF_OK && rc != ZRAGF_BUF_ERROR)
            return 0;
        if (produced == 0u && rc == ZRAGF_BUF_ERROR)
            return 0;
    }
}

static int drive_inflater(zragf_stream *zs, buffer *out, int flush, int *retcode)
{
    int rc;
    unsigned char tmp[127];
    size_t produced;

    for (;;) {
        zs->next_out = tmp;
        zs->avail_out = sizeof(tmp);
        rc = zragf_inflateZ(zs, flush);
        produced = sizeof(tmp) - zs->avail_out;
        if (!buf_append(out, tmp, produced))
            return 0;
        if (rc == ZRAGF_STREAM_END || rc == ZRAGF_DATA_ERROR || rc == ZRAGF_NEED_DICT) {
            *retcode = rc;
            return 1;
        }
        if (rc != ZRAGF_OK && rc != ZRAGF_BUF_ERROR)
            return 0;
        if (produced == 0u && rc == ZRAGF_BUF_ERROR)
            return 0;
        if (produced == 0u && rc == ZRAGF_OK && zs->avail_in == 0u)
            return 0;
    }
}

static void build_prime_payload(const unsigned char *src, size_t src_size,
                                int prime_bits, unsigned *prime_value,
                                buffer *shifted)
{
    size_t total_bits = src_size * 8u;
    size_t remain_bits = total_bits - (size_t)prime_bits;
    size_t i;

    *prime_value = 0u;
    for (i = 0u; i < (size_t)prime_bits; ++i) {
        unsigned bit = (unsigned)((src[i / 8u] >> (i % 8u)) & 1u);
        *prime_value |= bit << i;
    }

    shifted->size = 0u;
    shifted->cap = (remain_bits + 7u) / 8u;
    shifted->data = (unsigned char *)zragf_p89_host_take_zero(shifted->cap ? shifted->cap : 1u, 1u);
    if (!shifted->data)
        return;
    shifted->size = shifted->cap;
    for (i = 0u; i < remain_bits; ++i) {
        unsigned bit = (unsigned)((src[(i + (size_t)prime_bits) / 8u] >> ((i + (size_t)prime_bits) % 8u)) & 1u);
        shifted->data[i / 8u] |= (unsigned char)(bit << (i % 8u));
    }
}

static int __attribute__((unused)) find_marker(const buffer *b, size_t *pos)
{
    size_t i;
    for (i = 0u; i + 3u < b->size; ++i) {
        if (b->data[i] == 0x00u && b->data[i + 1u] == 0x00u &&
            b->data[i + 2u] == 0xFFu && b->data[i + 3u] == 0xFFu) {
            *pos = i;
            return 1;
        }
    }
    return 0;
}

static int test_deflate_copy_and_tune(void)
{
    zragf_stream a, b;
    buffer out_a = {0}, out_b = {0};
    unsigned char check[8192];
    size_t check_size = 0u;
    static const unsigned char prefix[] =
        "phase8 prefix: The quick brown fox jumps over the lazy dog. "
        "The quick brown fox jumps over the lazy dog. "
        "12345123451234512345 abcdefghijklmnopqrstuvwxyz ";
    static const unsigned char suffix[] =
        "phase8 suffix: lorem ipsum lorem ipsum lorem ipsum --- "
        "with enough repeated material to exercise lazy matching and chain tuning. "
        "lorem ipsum lorem ipsum lorem ipsum --- END.";
    unsigned char full[sizeof(prefix) + sizeof(suffix)];
    unsigned char tmp[211];
    int rc;

    memcpy(full, prefix, sizeof(prefix) - 1u);
    memcpy(full + sizeof(prefix) - 1u, suffix, sizeof(suffix) - 1u);

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    if (zragf_deflateInit2(&a, ZRAGF_LEVEL_DEFAULT, 8, -15, 8, ZRAGF_Z_DEFAULT_STRATEGY) != ZRAGF_OK)
        return 0;
    if (zragf_deflateTune(&a, 16, 8, 192, 1024) != ZRAGF_OK) {
        zragf_deflateEndZ(&a);
        return 0;
    }

    a.next_in = (unsigned char *)prefix;
    a.avail_in = sizeof(prefix) - 1u;
    a.next_out = tmp;
    a.avail_out = sizeof(tmp);
    rc = zragf_deflateZ(&a, ZRAGF_NO_FLUSH);
    if (rc != ZRAGF_OK) {
        zragf_deflateEndZ(&a);
        return 0;
    }
    if (!buf_append(&out_a, tmp, sizeof(tmp) - a.avail_out)) {
        zragf_deflateEndZ(&a);
        return 0;
    }

    if (zragf_deflateCopy(&b, &a) != ZRAGF_OK) {
        zragf_deflateEndZ(&a);
        return 0;
    }
    if (!buf_append(&out_b, out_a.data, out_a.size)) {
        zragf_deflateEndZ(&a);
        zragf_deflateEndZ(&b);
        return 0;
    }

    if (!finish_deflater(&a, suffix, sizeof(suffix) - 1u, &out_a) ||
        !finish_deflater(&b, suffix, sizeof(suffix) - 1u, &out_b)) {
        zragf_deflateEndZ(&a);
        zragf_deflateEndZ(&b);
        buf_free(&out_a);
        buf_free(&out_b);
        return 0;
    }
    zragf_deflateEndZ(&a);
    zragf_deflateEndZ(&b);

    if (out_a.size != out_b.size || memcmp(out_a.data, out_b.data, out_a.size) != 0) {
        buf_free(&out_a);
        buf_free(&out_b);
        return 0;
    }
    if (!zlib_inflate_all(out_a.data, out_a.size, -15, check, sizeof(check), &check_size)) {
        buf_free(&out_a);
        buf_free(&out_b);
        return 0;
    }
    if (check_size != (sizeof(full) - 2u) || memcmp(check, full, check_size) != 0) {
        buf_free(&out_a);
        buf_free(&out_b);
        return 0;
    }

    buf_free(&out_a);
    buf_free(&out_b);
    return 1;
}

static int test_inflate_copy(void)
{
    static const unsigned char src[] =
        "inflate copy test: abcabcabcabcabcabcabcabcabcabc :: "
        "longer tail with repeated words repeated words repeated words. "
        "0123456789 0123456789 0123456789 0123456789";
    buffer comp = {0};
    z_stream zc;
    zragf_stream a, b;
    buffer out_a = {0}, out_b = {0};
    int rc;

    memset(&zc, 0, sizeof(zc));
    if (deflateInit(&zc, Z_DEFAULT_COMPRESSION) != Z_OK)
        return 0;
    {
        unsigned char tmp[256];
        zc.next_in = (Bytef *)src;
        zc.avail_in = (uInt)(sizeof(src) - 1u);
        do {
            zc.next_out = tmp;
            zc.avail_out = (uInt)sizeof(tmp);
            rc = deflate(&zc, Z_FINISH);
            if (rc != Z_OK && rc != Z_STREAM_END) {
                deflateEnd(&zc);
                return 0;
            }
            if (!buf_append(&comp, tmp, sizeof(tmp) - (size_t)zc.avail_out)) {
                deflateEnd(&zc);
                return 0;
            }
        } while (rc != Z_STREAM_END);
        deflateEnd(&zc);
    }

    memset(&a, 0, sizeof(a));
    memset(&b, 0, sizeof(b));
    if (zragf_inflateInit2(&a, 15) != ZRAGF_OK) {
        buf_free(&comp);
        return 0;
    }
    a.next_in = comp.data;
    a.avail_in = comp.size;
    for (;;) {
        unsigned char tmp[41];
        a.next_out = tmp;
        a.avail_out = sizeof(tmp);
        rc = zragf_inflateZ(&a, ZRAGF_NO_FLUSH);
        if (!buf_append(&out_a, tmp, sizeof(tmp) - a.avail_out)) {
            zragf_inflateEndZ(&a);
            buf_free(&comp);
            return 0;
        }
        if (out_a.size >= 70u)
            break;
        if (rc != ZRAGF_OK)
            break;
    }
    if (rc != ZRAGF_OK || zragf_inflateCopy(&b, &a) != ZRAGF_OK) {
        zragf_inflateEndZ(&a);
        buf_free(&comp);
        buf_free(&out_a);
        return 0;
    }
    if (!buf_append(&out_b, out_a.data, out_a.size)) {
        zragf_inflateEndZ(&a);
        zragf_inflateEndZ(&b);
        buf_free(&comp);
        buf_free(&out_a);
        return 0;
    }

    for (;;) {
        unsigned char tmp[89];
        a.next_out = tmp;
        a.avail_out = sizeof(tmp);
        a.next_in = NULL;
        a.avail_in = 0u;
        rc = zragf_inflateZ(&a, ZRAGF_NO_FLUSH);
        if (!buf_append(&out_a, tmp, sizeof(tmp) - a.avail_out)) {
            zragf_inflateEndZ(&a);
            zragf_inflateEndZ(&b);
            buf_free(&comp);
            buf_free(&out_a);
            buf_free(&out_b);
            return 0;
        }
        if (rc == ZRAGF_STREAM_END)
            break;
        if (rc != ZRAGF_OK)
            return 0;
    }

    for (;;) {
        unsigned char tmp[89];
        b.next_out = tmp;
        b.avail_out = sizeof(tmp);
        b.next_in = NULL;
        b.avail_in = 0u;
        rc = zragf_inflateZ(&b, ZRAGF_NO_FLUSH);
        if (!buf_append(&out_b, tmp, sizeof(tmp) - b.avail_out)) {
            zragf_inflateEndZ(&a);
            zragf_inflateEndZ(&b);
            buf_free(&comp);
            buf_free(&out_a);
            buf_free(&out_b);
            return 0;
        }
        if (rc == ZRAGF_STREAM_END)
            break;
        if (rc != ZRAGF_OK)
            return 0;
    }

    zragf_inflateEndZ(&a);
    zragf_inflateEndZ(&b);
    buf_free(&comp);

    if (out_a.size != sizeof(src) - 1u || out_b.size != out_a.size ||
        memcmp(out_a.data, src, out_a.size) != 0 || memcmp(out_b.data, src, out_b.size) != 0) {
        buf_free(&out_a);
        buf_free(&out_b);
        return 0;
    }

    buf_free(&out_a);
    buf_free(&out_b);
    return 1;
}

static int test_inflate_prime(void)
{
    static const unsigned char src[] =
        "inflatePrime/raw test with a bit-shifted payload. "
        "This sentence repeats. This sentence repeats. This sentence repeats.";
    buffer raw = {0};
    buffer shifted = {0};
    zragf_stream zs;
    unsigned prime_value = 0u;
    unsigned char out[512];
    int rc;

    if (!zlib_raw_deflate(src, sizeof(src) - 1u, &raw))
        return 0;
    build_prime_payload(raw.data, raw.size, 5, &prime_value, &shifted);
    if (!shifted.data) {
        buf_free(&raw);
        return 0;
    }

    memset(&zs, 0, sizeof(zs));
    if (zragf_inflateInit2(&zs, -15) != ZRAGF_OK) {
        buf_free(&raw);
        buf_free(&shifted);
        return 0;
    }
    if (zragf_inflatePrime(&zs, 5, (int)prime_value) != ZRAGF_OK) {
        zragf_inflateEndZ(&zs);
        buf_free(&raw);
        buf_free(&shifted);
        return 0;
    }
    zs.next_in = shifted.data;
    zs.avail_in = shifted.size;
    zs.next_out = out;
    zs.avail_out = sizeof(out);
    rc = zragf_inflateZ(&zs, ZRAGF_FINISH);
    zragf_inflateEndZ(&zs);
    buf_free(&raw);
    buf_free(&shifted);

    if (rc != ZRAGF_STREAM_END)
        return 0;
    if (sizeof(out) - zs.avail_out != sizeof(src) - 1u)
        return 0;
    return memcmp(out, src, sizeof(src) - 1u) == 0;
}

static int test_inflate_sync(void)
{
    static const unsigned char junk_prefix[] = { 0x06u, 0xAAu, 0xBBu, 0xCCu };
    static const unsigned char marker[] = { 0x00u, 0x00u, 0xFFu, 0xFFu };
    static const unsigned char part2[] =
        "RECOVER-ME-TAIL RECOVER-ME-TAIL RECOVER-ME-TAIL RECOVER-ME-TAIL";
    buffer comp = {0};
    buffer stream = {0};
    buffer junk = {0};
    buffer recovered = {0};
    zragf_stream zs;
    int rc;

    if (!zlib_raw_deflate(part2, sizeof(part2) - 1u, &comp))
        return 0;
    if (!buf_append(&stream, junk_prefix, sizeof(junk_prefix)) ||
        !buf_append(&stream, marker, sizeof(marker)) ||
        !buf_append(&stream, comp.data, comp.size)) {
        buf_free(&comp);
        buf_free(&stream);
        return 0;
    }

    memset(&zs, 0, sizeof(zs));
    if (zragf_inflateInit2(&zs, -15) != ZRAGF_OK) {
        buf_free(&comp);
        buf_free(&stream);
        return 0;
    }
    zs.next_in = stream.data;
    zs.avail_in = stream.size;
    if (!drive_inflater(&zs, &junk, ZRAGF_NO_FLUSH, &rc) || rc != ZRAGF_DATA_ERROR) {
        zragf_inflateEndZ(&zs);
        buf_free(&comp);
        buf_free(&stream);
        buf_free(&junk);
        return 0;
    }
    if (zragf_inflateSync(&zs) != ZRAGF_OK) {
        zragf_inflateEndZ(&zs);
        buf_free(&comp);
        buf_free(&stream);
        buf_free(&junk);
        return 0;
    }
    zs.next_in = NULL;
    zs.avail_in = 0u;
    if (!drive_inflater(&zs, &recovered, ZRAGF_NO_FLUSH, &rc) || rc != ZRAGF_STREAM_END) {
        zragf_inflateEndZ(&zs);
        buf_free(&comp);
        buf_free(&stream);
        buf_free(&junk);
        buf_free(&recovered);
        return 0;
    }
    zragf_inflateEndZ(&zs);
    buf_free(&comp);
    buf_free(&stream);
    buf_free(&junk);

    if (recovered.size != sizeof(part2) - 1u || memcmp(recovered.data, part2, sizeof(part2) - 1u) != 0) {
        buf_free(&recovered);
        return 0;
    }
    buf_free(&recovered);
    return 1;
}

int main(void)
{
    if (!test_deflate_copy_and_tune()) {
        fprintf(stderr, "phase8 deflateCopy/deflateTune failed\n");
        return 1;
    }
    if (!test_inflate_copy()) {
        fprintf(stderr, "phase8 inflateCopy failed\n");
        return 1;
    }
    if (!test_inflate_prime()) {
        fprintf(stderr, "phase8 inflatePrime failed\n");
        return 1;
    }
    if (!test_inflate_sync()) {
        fprintf(stderr, "phase8 inflateSync failed\n");
        return 1;
    }
    puts("phase8 copy/prime/sync ok");
    return 0;
}
