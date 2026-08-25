#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>
#include <libdeflate.h>

#include "zragflib.h"

#define PH19_MAX_INPUT   40000u
#define PH19_MAX_OUTPUT  160000u
#define PH19_DICT_LEN      512u

static unsigned int ph19_xorshift32(unsigned int *state)
{
    unsigned int x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static void ph19_make_case(unsigned char *buf, size_t n, int kind, unsigned int *seed)
{
    size_t i;
    switch (kind) {
        case 0: /* random */
            for (i = 0; i < n; ++i)
                buf[i] = (unsigned char)(ph19_xorshift32(seed) & 0xFFu);
            break;
        case 1: /* alternating */
            for (i = 0; i < n; ++i)
                buf[i] = (unsigned char)((i & 1u) ? 0x00u : 0xFFu);
            break;
        case 2: /* small alphabet */
            for (i = 0; i < n; ++i)
                buf[i] = (unsigned char)('A' + (i % 3u));
            break;
        case 3: /* ramp */
            for (i = 0; i < n; ++i)
                buf[i] = (unsigned char)(i & 0xFFu);
            break;
        case 4: /* long run */
            memset(buf, 'A', n);
            break;
        case 5: /* sparse mostly-zero */
            for (i = 0; i < n; ++i)
                buf[i] = (unsigned char)((i % 257u) ? 0u : 1u);
            break;
        case 6: /* copyback-friendly mixed text */
            for (i = 0; i < n; ++i)
                buf[i] = (unsigned char)('a' + (ph19_xorshift32(seed) % 26u));
            if (n > 128u)
                memcpy(buf + (n / 2u), buf, n / 4u);
            break;
        default:
            memset(buf, 0, n);
            break;
    }
}

static size_t ph19_chunk_in(const unsigned char *buf, size_t pos, size_t len, unsigned step)
{
    size_t n;
    if (pos >= len)
        return 0u;
    n = 1u + (((unsigned)buf[pos] + step * 17u) & 0x3Fu);
    if (n > len - pos)
        n = len - pos;
    return n;
}

static size_t ph19_chunk_out(size_t total_out, size_t cap)
{
    size_t n = 1u + ((total_out * 7u + 11u) & 0x7Fu);
    if (n > cap)
        n = cap;
    if (n == 0u)
        n = 1u;
    return n;
}

static int ph19_zragf_deflate_stream(const unsigned char *src,
                                     size_t src_len,
                                     int window_bits,
                                     int level,
                                     int strategy,
                                     const unsigned char *dict,
                                     unsigned dict_len,
                                     unsigned char *dst,
                                     size_t dst_cap,
                                     size_t *dst_len)
{
    zragf_stream s;
    size_t in_pos = 0u;
    int rc;
    unsigned step = 0u;

    memset(&s, 0, sizeof(s));
    if (zragf_deflateInit2(&s, level, 8, window_bits, 8, strategy) != ZRAGF_OK)
        return 0;

    if (dict_len > 0u && window_bits != 31) {
        rc = zragf_deflateSetDictionary(&s, dict, dict_len);
        if (rc != ZRAGF_OK) {
            zragf_deflateEndZ(&s);
            return 0;
        }
    }

    for (;;) {
        size_t in_chunk = ph19_chunk_in(src, in_pos, src_len, step++);
        int flush = ZRAGF_NO_FLUSH;

        s.next_in = (zragf_u8 *)(src + in_pos);
        s.avail_in = in_chunk;
        if (in_pos + in_chunk >= src_len)
            flush = ZRAGF_FINISH;
        else if (src_len > 1024u && ((in_pos + in_chunk) % 257u) == 0u)
            flush = ZRAGF_SYNC_FLUSH;

        do {
            size_t out_space = dst_cap - (size_t)s.total_out;
            size_t out_chunk;
            if (out_space == 0u) {
                zragf_deflateEndZ(&s);
                return 0;
            }
            out_chunk = ph19_chunk_out((size_t)s.total_out, out_space);
            s.next_out = dst + (size_t)s.total_out;
            s.avail_out = out_chunk;
            rc = zragf_deflateZ(&s, flush);
            if (rc == ZRAGF_STREAM_END) {
                *dst_len = (size_t)s.total_out;
                zragf_deflateEndZ(&s);
                return 1;
            }
            if (rc < 0 && rc != ZRAGF_BUF_ERROR) {
                zragf_deflateEndZ(&s);
                return 0;
            }
        } while (s.avail_in > 0u || s.avail_out == 0u);

        in_pos += in_chunk;
    }
}

static int ph19_zragf_inflate_stream(const unsigned char *src,
                                     size_t src_len,
                                     int window_bits,
                                     const unsigned char *dict,
                                     unsigned dict_len,
                                     unsigned char *dst,
                                     size_t dst_cap,
                                     size_t *dst_len)
{
    zragf_stream s;
    size_t in_pos = 0u;
    int rc;
    unsigned step = 0u;

    memset(&s, 0, sizeof(s));
    if (zragf_inflateInit2(&s, window_bits) != ZRAGF_OK)
        return 0;
    if (dict_len > 0u && window_bits < 0) {
        rc = zragf_inflateSetDictionary(&s, dict, dict_len);
        if (rc != ZRAGF_OK) {
            zragf_inflateEndZ(&s);
            return 0;
        }
    }

    for (;;) {
        size_t in_chunk = ph19_chunk_in(src, in_pos, src_len, step++);
        int flush = (in_pos + in_chunk >= src_len) ? ZRAGF_FINISH : ZRAGF_NO_FLUSH;

        s.next_in = (zragf_u8 *)(src + in_pos);
        s.avail_in = in_chunk;

        do {
            size_t out_space = dst_cap - (size_t)s.total_out;
            size_t out_chunk;
            if (out_space == 0u) {
                zragf_inflateEndZ(&s);
                return 0;
            }
            out_chunk = ph19_chunk_out((size_t)s.total_out + 3u, out_space);
            s.next_out = dst + (size_t)s.total_out;
            s.avail_out = out_chunk;
            rc = zragf_inflateZ(&s, flush);
            if (rc == ZRAGF_NEED_DICT) {
                if (dict_len == 0u) {
                    zragf_inflateEndZ(&s);
                    return 0;
                }
                rc = zragf_inflateSetDictionary(&s, dict, dict_len);
                if (rc != ZRAGF_OK) {
                    zragf_inflateEndZ(&s);
                    return 0;
                }
                continue;
            }
            if (rc == ZRAGF_STREAM_END) {
                *dst_len = (size_t)s.total_out;
                zragf_inflateEndZ(&s);
                return 1;
            }
            if (rc < 0 && rc != ZRAGF_BUF_ERROR) {
                zragf_inflateEndZ(&s);
                return 0;
            }
        } while (s.avail_in > 0u || s.avail_out == 0u);

        in_pos += in_chunk;
        if (in_pos >= src_len && src_len == 0u) {
            /* allow FINISH-only zero-length drive */
            src = (const unsigned char *)"";
            src_len = 0u;
        }
    }
}

static int ph19_zlib_inflate_stream(const unsigned char *src,
                                    size_t src_len,
                                    int window_bits,
                                    const unsigned char *dict,
                                    unsigned dict_len,
                                    unsigned char *dst,
                                    size_t dst_cap,
                                    size_t *dst_len)
{
    z_stream zs;
    size_t in_pos = 0u;
    int rc;
    unsigned step = 0u;

    memset(&zs, 0, sizeof(zs));
    if (inflateInit2(&zs, window_bits) != Z_OK)
        return 0;
    if (dict_len > 0u && window_bits < 0) {
        if (inflateSetDictionary(&zs, dict, dict_len) != Z_OK) {
            inflateEnd(&zs);
            return 0;
        }
    }

    for (;;) {
        size_t in_chunk = ph19_chunk_in(src, in_pos, src_len, step++);
        int flush = (in_pos + in_chunk >= src_len) ? Z_FINISH : Z_NO_FLUSH;

        zs.next_in = (Bytef *)(src + in_pos);
        zs.avail_in = (uInt)in_chunk;
        do {
            size_t out_space = dst_cap - (size_t)zs.total_out;
            size_t out_chunk;
            if (out_space == 0u) {
                inflateEnd(&zs);
                return 0;
            }
            out_chunk = ph19_chunk_out((size_t)zs.total_out + 5u, out_space);
            zs.next_out = dst + (size_t)zs.total_out;
            zs.avail_out = (uInt)out_chunk;
            rc = inflate(&zs, flush);
            if (rc == Z_NEED_DICT) {
                if (dict_len == 0u || inflateSetDictionary(&zs, dict, dict_len) != Z_OK) {
                    inflateEnd(&zs);
                    return 0;
                }
                continue;
            }
            if (rc == Z_STREAM_END) {
                *dst_len = (size_t)zs.total_out;
                inflateEnd(&zs);
                return 1;
            }
            if (rc != Z_OK && rc != Z_BUF_ERROR) {
                inflateEnd(&zs);
                return 0;
            }
        } while (zs.avail_in > 0u || zs.avail_out == 0u);
        in_pos += in_chunk;
        if (src_len == 0u && in_pos == 0u)
            in_pos = src_len;
    }
}

static int ph19_zlib_deflate_one_shot(const unsigned char *src,
                                      size_t src_len,
                                      int window_bits,
                                      int level,
                                      int strategy,
                                      const unsigned char *dict,
                                      unsigned dict_len,
                                      unsigned char *dst,
                                      size_t dst_cap,
                                      size_t *dst_len)
{
    z_stream zs;
    int rc;

    memset(&zs, 0, sizeof(zs));
    if (deflateInit2(&zs, level, Z_DEFLATED, window_bits, 8, strategy) != Z_OK)
        return 0;
    if (dict_len > 0u && window_bits != 31) {
        rc = deflateSetDictionary(&zs, dict, dict_len);
        if (rc != Z_OK) {
            deflateEnd(&zs);
            return 0;
        }
    }
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_len;
    zs.next_out = dst;
    zs.avail_out = (uInt)dst_cap;
    rc = deflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) {
        deflateEnd(&zs);
        return 0;
    }
    *dst_len = (size_t)zs.total_out;
    rc = deflateEnd(&zs);
    return rc == Z_OK;
}

static int ph19_libdeflate_decompress(const unsigned char *src,
                                      size_t src_len,
                                      int format,
                                      unsigned char *dst,
                                      size_t dst_cap,
                                      size_t *dst_len)
{
    struct libdeflate_decompressor *d;
    enum libdeflate_result res;
    size_t actual_in = 0u;
    size_t actual_out = 0u;

    d = libdeflate_alloc_decompressor();
    if (!d)
        return 0;

    if (format == 0)
        res = libdeflate_deflate_decompress_ex(d, src, src_len, dst, dst_cap, &actual_in, &actual_out);
    else if (format == 1)
        res = libdeflate_zlib_decompress_ex(d, src, src_len, dst, dst_cap, &actual_in, &actual_out);
    else
        res = libdeflate_gzip_decompress_ex(d, src, src_len, dst, dst_cap, &actual_in, &actual_out);

    libdeflate_free_decompressor(d);
    if (res != LIBDEFLATE_SUCCESS)
        return 0;
    *dst_len = actual_out;
    return 1;
}

static int ph19_libdeflate_compress(const unsigned char *src,
                                    size_t src_len,
                                    int format,
                                    int level,
                                    unsigned char *dst,
                                    size_t dst_cap,
                                    size_t *dst_len)
{
    struct libdeflate_compressor *c;
    size_t out_len = 0u;

    c = libdeflate_alloc_compressor(level);
    if (!c)
        return 0;

    if (format == 0)
        out_len = libdeflate_deflate_compress(c, src, src_len, dst, dst_cap);
    else if (format == 1)
        out_len = libdeflate_zlib_compress(c, src, src_len, dst, dst_cap);
    else
        out_len = libdeflate_gzip_compress(c, src, src_len, dst, dst_cap);

    libdeflate_free_compressor(c);
    if (out_len == 0u)
        return 0;
    *dst_len = out_len;
    return 1;
}

static int ph19_check_one_matrix_case(const unsigned char *input,
                                      size_t input_len,
                                      int window_bits,
                                      int level,
                                      int strategy,
                                      const unsigned char *dict,
                                      unsigned dict_len)
{
    unsigned char comp[PH19_MAX_OUTPUT];
    unsigned char out_a[PH19_MAX_INPUT + PH19_DICT_LEN + 256u];
    unsigned char out_b[PH19_MAX_INPUT + PH19_DICT_LEN + 256u];
    unsigned char out_c[PH19_MAX_INPUT + PH19_DICT_LEN + 256u];
    size_t comp_len = 0u;
    size_t out_a_len = 0u;
    size_t out_b_len = 0u;
    size_t out_c_len = 0u;
    int format = (window_bits < 0) ? 0 : (window_bits == 31 ? 2 : 1);

    if (!ph19_zragf_deflate_stream(input, input_len, window_bits, level, strategy,
                                   dict, dict_len, comp, sizeof(comp), &comp_len))
        return 0;

    if (!ph19_zragf_inflate_stream(comp, comp_len, window_bits,
                                   dict, dict_len, out_a, sizeof(out_a), &out_a_len))
        return 0;
    if (out_a_len != input_len || memcmp(out_a, input, input_len) != 0)
        return 0;

    if (!ph19_zlib_inflate_stream(comp, comp_len, window_bits,
                                  dict, dict_len, out_b, sizeof(out_b), &out_b_len))
        return 0;
    if (out_b_len != input_len || memcmp(out_b, input, input_len) != 0)
        return 0;

    if (dict_len == 0u) {
        if (!ph19_libdeflate_decompress(comp, comp_len, format,
                                        out_c, sizeof(out_c), &out_c_len))
            return 0;
        if (out_c_len != input_len || memcmp(out_c, input, input_len) != 0)
            return 0;
    }

    return 1;
}

static int test_phase19_fixed_regression(void)
{
    static const unsigned char raw_fixed[] = { 0xabu, 0x5au, 0x0fu, 0x00u };
    static const unsigned char expected[] = { 0x7au, 0xafu };
    unsigned char out[16];
    size_t out_len = 0u;

    if (!ph19_zragf_inflate_stream(raw_fixed, sizeof(raw_fixed), -15,
                                   NULL, 0u, out, sizeof(out), &out_len))
        return 0;
    return out_len == sizeof(expected) && memcmp(out, expected, sizeof(expected)) == 0;
}

static int test_phase19_cross_matrix(void)
{
    static const size_t sizes[] = { 0u, 1u, 2u, 15u, 257u, 4099u, 32781u };
    static const int wrappers[] = { -15, 15, 31 };
    static const int strategies[] = {
        ZRAGF_Z_DEFAULT_STRATEGY,
        ZRAGF_Z_FIXED,
        ZRAGF_Z_HUFFMAN_ONLY,
        ZRAGF_Z_RLE
    };
    unsigned char input[PH19_MAX_INPUT];
    unsigned char dict[PH19_DICT_LEN];
    unsigned int seed = 0xC0FFEE11u;
    size_t i, j, k, m;

    for (i = 0; i < 7u; ++i) {
        for (j = 0; j < sizeof(sizes) / sizeof(sizes[0]); ++j) {
            size_t n = sizes[j];
            ph19_make_case(input, n, (int)i, &seed);
            ph19_make_case(dict, sizeof(dict), (int)((i + 3u) % 7u), &seed);

            for (k = 0; k < sizeof(wrappers) / sizeof(wrappers[0]); ++k) {
                int wb = wrappers[k];
                for (m = 0; m < sizeof(strategies) / sizeof(strategies[0]); ++m) {
                    if (!ph19_check_one_matrix_case(input, n, wb, 6, strategies[m], NULL, 0u)) {
                        fprintf(stderr, "phase19 matrix fail: kind=%u size=%zu wb=%d strat=%d\n",
                                (unsigned)i, n, wb, strategies[m]);
                        return 0;
                    }
                }
                if (wb != 31 && n >= 2u) {
                    if (!ph19_check_one_matrix_case(input, n, wb, 6, ZRAGF_Z_DEFAULT_STRATEGY,
                                                    dict, (unsigned)sizeof(dict))) {
                        fprintf(stderr, "phase19 dict fail: kind=%u size=%zu wb=%d\n",
                                (unsigned)i, n, wb);
                        return 0;
                    }
                }
            }
        }
    }

    return 1;
}

static int test_phase19_external_to_zragf(void)
{
    static const size_t sizes[] = { 0u, 2u, 31u, 1024u, 16397u };
    static const int wrappers[] = { -15, 15, 31 };
    unsigned char input[PH19_MAX_INPUT];
    unsigned char dict[PH19_DICT_LEN];
    unsigned char comp[PH19_MAX_OUTPUT];
    unsigned char out[PH19_MAX_INPUT + PH19_DICT_LEN + 256u];
    unsigned int seed = 0x51A7BEEFu;
    size_t i, j, k;

    for (i = 0; i < 5u; ++i) {
        for (j = 0; j < sizeof(sizes) / sizeof(sizes[0]); ++j) {
            size_t n = sizes[j];
            size_t comp_len = 0u;
            size_t out_len = 0u;
            ph19_make_case(input, n, (int)i, &seed);
            ph19_make_case(dict, sizeof(dict), (int)((i + 1u) % 7u), &seed);

            for (k = 0; k < sizeof(wrappers) / sizeof(wrappers[0]); ++k) {
                int wb = wrappers[k];
                if (!ph19_zlib_deflate_one_shot(input, n, wb, Z_BEST_SPEED,
                                                Z_DEFAULT_STRATEGY, NULL, 0u,
                                                comp, sizeof(comp), &comp_len)) {
                    fprintf(stderr, "phase19 zlib emit fail: kind=%u size=%zu wb=%d\n",
                            (unsigned)i, n, wb);
                    return 0;
                }
                if (!ph19_zragf_inflate_stream(comp, comp_len, wb, NULL, 0u,
                                               out, sizeof(out), &out_len) ||
                    out_len != n || memcmp(out, input, n) != 0) {
                    fprintf(stderr, "phase19 zlib->zragf fail: kind=%u size=%zu wb=%d\n",
                            (unsigned)i, n, wb);
                    return 0;
                }

                if (!ph19_libdeflate_compress(input, n, (wb < 0) ? 0 : (wb == 31 ? 2 : 1), 6,
                                              comp, sizeof(comp), &comp_len)) {
                    fprintf(stderr, "phase19 libdef emit fail: kind=%u size=%zu wb=%d\n",
                            (unsigned)i, n, wb);
                    return 0;
                }
                if (!ph19_zragf_inflate_stream(comp, comp_len, wb, NULL, 0u,
                                               out, sizeof(out), &out_len) ||
                    out_len != n || memcmp(out, input, n) != 0) {
                    fprintf(stderr, "phase19 libdef->zragf fail: kind=%u size=%zu wb=%d\n",
                            (unsigned)i, n, wb);
                    return 0;
                }
            }

            if (!ph19_zlib_deflate_one_shot(input, n, 15, Z_BEST_SPEED,
                                            Z_DEFAULT_STRATEGY, dict, (unsigned)sizeof(dict),
                                            comp, sizeof(comp), &comp_len)) {
                fprintf(stderr, "phase19 zlib dict emit fail: kind=%u size=%zu\n",
                        (unsigned)i, n);
                return 0;
            }
            if (!ph19_zragf_inflate_stream(comp, comp_len, 15, dict, (unsigned)sizeof(dict),
                                           out, sizeof(out), &out_len) ||
                out_len != n || memcmp(out, input, n) != 0) {
                fprintf(stderr, "phase19 zlib dict -> zragf fail: kind=%u size=%zu\n",
                        (unsigned)i, n);
                return 0;
            }
        }
    }
    return 1;
}

int main(void)
{
    if (!test_phase19_fixed_regression()) {
        fprintf(stderr, "phase19 fixed regression failed\n");
        return 1;
    }
    if (!test_phase19_cross_matrix()) {
        fprintf(stderr, "phase19 cross matrix failed\n");
        return 1;
    }
    if (!test_phase19_external_to_zragf()) {
        fprintf(stderr, "phase19 external->zragf failed\n");
        return 1;
    }

    printf("phase19 differential ok\n");
    return 0;
}
