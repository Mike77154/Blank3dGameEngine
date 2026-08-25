#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "zragflib.h"

static int inflate_with_zlib(const unsigned char *src, size_t src_len,
                             unsigned char *dst, size_t dst_cap,
                             size_t *dst_len)
{
    z_stream zs;
    int rc;

    memset(&zs, 0, sizeof(zs));
    zs.next_in = (unsigned char *)src;
    zs.avail_in = (uInt)src_len;
    zs.next_out = dst;
    zs.avail_out = (uInt)dst_cap;

    rc = inflateInit(&zs);
    if (rc != Z_OK)
        return rc;

    rc = inflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) {
        inflateEnd(&zs);
        return rc;
    }
    *dst_len = (size_t)zs.total_out;
    inflateEnd(&zs);
    return Z_OK;
}

static int zragf_inflate_all(zragf_stream *zs)
{
    int rc;
    for (;;) {
        rc = zragf_inflateZ(zs, ZRAGF_FINISH);
        if (rc == ZRAGF_STREAM_END || rc < 0)
            return rc;
        if (zs->avail_in == 0u && zs->avail_out > 0u)
            return rc;
    }
}

static int test_deflate_params(void)
{
    unsigned char part1[4096];
    unsigned char part2[3072];
    unsigned char input[7168];
    unsigned char out[16384];
    unsigned char decoded[8192];
    size_t decoded_len = 0u;
    zragf_stream zs;
    int rc;
    size_t out_len;
    size_t i;

    memset(part1, 'A', sizeof(part1));
    for (i = 0; i < sizeof(part2); ++i)
        part2[i] = (unsigned char)((i * 17u) ^ (i >> 3));
    memcpy(input, part1, sizeof(part1));
    memcpy(input + sizeof(part1), part2, sizeof(part2));

    memset(&zs, 0, sizeof(zs));
    rc = zragf_deflateInit2(&zs, 1, 8, 15, 8, ZRAGF_Z_DEFAULT_STRATEGY);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "deflateInit2 rc=%d\n", rc);
        return 1;
    }

    zs.next_out = out;
    zs.avail_out = sizeof(out);

    zs.next_in = part1;
    zs.avail_in = sizeof(part1);
    rc = zragf_deflateZ(&zs, ZRAGF_NO_FLUSH);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "deflate(NO_FLUSH) rc=%d\n", rc);
        zragf_deflateEndZ(&zs);
        return 1;
    }

    rc = zragf_deflateParams(&zs, 9, ZRAGF_Z_FIXED);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "deflateParams rc=%d\n", rc);
        zragf_deflateEndZ(&zs);
        return 1;
    }

    zs.next_in = part2;
    zs.avail_in = sizeof(part2);
    for (;;) {
        rc = zragf_deflateZ(&zs, ZRAGF_FINISH);
        if (rc == ZRAGF_STREAM_END)
            break;
        if (rc != ZRAGF_OK) {
            fprintf(stderr, "deflate(FINISH) rc=%d\n", rc);
            zragf_deflateEndZ(&zs);
            return 1;
        }
        if (zs.avail_out == 0u) {
            fprintf(stderr, "output buffer exhausted in deflate params test\n");
            zragf_deflateEndZ(&zs);
            return 1;
        }
    }
    out_len = sizeof(out) - zs.avail_out;
    zragf_deflateEndZ(&zs);

    if (inflate_with_zlib(out, out_len, decoded, sizeof(decoded), &decoded_len) != Z_OK) {
        fprintf(stderr, "zlib failed to decode params stream\n");
        return 1;
    }
    if (decoded_len != sizeof(input) || memcmp(decoded, input, sizeof(input)) != 0) {
        fprintf(stderr, "decoded params stream mismatch\n");
        return 1;
    }
    return 0;
}

static int test_inflate_validate_toggle(void)
{
    static const unsigned char payload[] =
        "phase9 validate toggle should ignore trailer corruption when asked";
    unsigned char compressed[256];
    uLongf compressed_len = sizeof(compressed);
    unsigned char decoded[256];
    zragf_stream zs;
    int rc;

    if (compress2(compressed, &compressed_len,
                  payload, (uLong)sizeof(payload), Z_BEST_COMPRESSION) != Z_OK) {
        fprintf(stderr, "compress2 failed\n");
        return 1;
    }

    /* Corrupt Adler-32 trailer byte. */
    compressed[compressed_len - 1u] ^= 0x5Au;

    memset(&zs, 0, sizeof(zs));
    rc = zragf_inflateInit(&zs);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "inflateInit rc=%d\n", rc);
        return 1;
    }
    zs.next_in = compressed;
    zs.avail_in = (zragf_size_t)compressed_len;
    zs.next_out = decoded;
    zs.avail_out = sizeof(decoded);
    rc = zragf_inflate_all(&zs);
    zragf_inflateEndZ(&zs);
    if (rc != ZRAGF_DATA_ERROR) {
        fprintf(stderr, "expected DATA_ERROR with validation on, got %d\n", rc);
        return 1;
    }

    memset(decoded, 0, sizeof(decoded));
    memset(&zs, 0, sizeof(zs));
    rc = zragf_inflateInit(&zs);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "inflateInit rc=%d\n", rc);
        return 1;
    }
    rc = zragf_inflateValidate(&zs, 0);
    if (rc != ZRAGF_OK) {
        fprintf(stderr, "inflateValidate rc=%d\n", rc);
        zragf_inflateEndZ(&zs);
        return 1;
    }
    zs.next_in = compressed;
    zs.avail_in = (zragf_size_t)compressed_len;
    zs.next_out = decoded;
    zs.avail_out = sizeof(decoded);
    rc = zragf_inflate_all(&zs);
    zragf_inflateEndZ(&zs);
    if (rc != ZRAGF_STREAM_END) {
        fprintf(stderr, "expected STREAM_END with validation off, got %d\n", rc);
        return 1;
    }
    if (memcmp(decoded, payload, sizeof(payload)) != 0) {
        fprintf(stderr, "decoded payload mismatch with validation off\n");
        return 1;
    }
    return 0;
}

int main(void)
{
    if (test_deflate_params() != 0)
        return 1;
    if (test_inflate_validate_toggle() != 0)
        return 1;
    puts("phase9 params/validate ok");
    return 0;
}
