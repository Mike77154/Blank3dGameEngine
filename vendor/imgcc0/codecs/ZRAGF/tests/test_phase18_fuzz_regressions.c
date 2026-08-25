#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <zlib.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"

#define PHASE18_GZIP_META_LIMIT 1048576u

static int build_stream(const unsigned char *payload,
                        size_t payload_len,
                        int window_bits,
                        unsigned char *dst,
                        size_t dst_cap,
                        size_t *dst_len)
{
    z_stream zs;
    int rc;

    memset(&zs, 0, sizeof(zs));
    if (deflateInit2(&zs,
                     Z_BEST_SPEED,
                     Z_DEFLATED,
                     window_bits,
                     8,
                     Z_DEFAULT_STRATEGY) != Z_OK)
        return 0;

    zs.next_in = (Bytef *)payload;
    zs.avail_in = (uInt)payload_len;
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

static int run_wrapper_case(const unsigned char *src,
                            size_t src_len,
                            int window_bits,
                            int want_header,
                            int validate_checks,
                            unsigned char *out,
                            size_t out_cap,
                            size_t *out_len,
                            int *out_rc)
{
    zragf_stream is;
    zragf_gz_header hdr;
    size_t in_pos = 0u;
    int rc = ZRAGF_OK;
    unsigned steps = 0u;

    memset(&is, 0, sizeof(is));
    memset(&hdr, 0, sizeof(hdr));
    *out_len = 0u;
    if (out_rc)
        *out_rc = ZRAGF_STREAM_ERROR;

    if (zragf_inflateInit2(&is, window_bits) != ZRAGF_OK)
        return 0;

    if (want_header)
        (void)zragf_inflateGetHeader(&is, &hdr);
    (void)zragf_inflateValidate(&is, validate_checks ? 1 : 0);

    while (steps++ < 4096u) {
        size_t in_chunk = 0u;
        size_t out_chunk;

        if (in_pos < src_len) {
            in_chunk = 1u + ((src[(in_pos + steps) % src_len] ^ (unsigned char)steps) & 0x0Fu);
            if (in_chunk > src_len - in_pos)
                in_chunk = src_len - in_pos;
            is.next_in = (zragf_u8 *)(src + in_pos);
            is.avail_in = (unsigned int)in_chunk;
        } else {
            is.next_in = NULL;
            is.avail_in = 0u;
        }

        out_chunk = 1u + ((steps * 7u) & 0x3Fu);
        if (out_chunk > out_cap - *out_len)
            out_chunk = out_cap - *out_len;
        if (out_chunk == 0u)
            out_chunk = 1u;
        is.next_out = out + *out_len;
        is.avail_out = (unsigned int)out_chunk;

        rc = zragf_inflateZ(&is, (in_pos >= src_len) ? ZRAGF_FINISH : ZRAGF_NO_FLUSH);
        in_pos += in_chunk;
        *out_len = (size_t)(is.total_out);

        if (rc == ZRAGF_STREAM_END || rc == ZRAGF_DATA_ERROR ||
            rc == ZRAGF_STREAM_ERROR || rc == ZRAGF_MEM_ERROR)
            break;

        if (rc == ZRAGF_NEED_DICT) {
            unsigned char dict_buf[32];
            unsigned dict_len = (unsigned)((src_len > sizeof(dict_buf)) ? sizeof(dict_buf) : src_len);
            if (dict_len > 0u)
                memcpy(dict_buf, src, dict_len);
            (void)zragf_inflateSetDictionary(&is, dict_buf, dict_len);
        } else if (rc == ZRAGF_BUF_ERROR && in_pos >= src_len) {
            (void)zragf_inflateSync(&is);
        }
    }

    zragf_inflateEndZ(&is);
    if (out_rc)
        *out_rc = rc;
    return 1;
}

static int test_valid_wrappers(void)
{
    static const unsigned char payload[] =
        "phase18 fuzz regression payload: hello hello hello";
    unsigned char comp[512];
    unsigned char out[256];
    size_t comp_len;
    size_t out_len;
    int rc;

    if (!build_stream(payload, sizeof(payload) - 1u, -15, comp, sizeof(comp), &comp_len))
        return 0;
    if (!run_wrapper_case(comp, comp_len, -15, 0, 1, out, sizeof(out), &out_len, &rc))
        return 0;
    if (rc != ZRAGF_STREAM_END || out_len != sizeof(payload) - 1u ||
        memcmp(out, payload, sizeof(payload) - 1u) != 0)
        return 0;

    if (!build_stream(payload, sizeof(payload) - 1u, 15, comp, sizeof(comp), &comp_len))
        return 0;
    if (!run_wrapper_case(comp, comp_len, 47, 0, 1, out, sizeof(out), &out_len, &rc))
        return 0;
    if (rc != ZRAGF_STREAM_END || out_len != sizeof(payload) - 1u ||
        memcmp(out, payload, sizeof(payload) - 1u) != 0)
        return 0;

    if (!build_stream(payload, sizeof(payload) - 1u, 31, comp, sizeof(comp), &comp_len))
        return 0;
    if (!run_wrapper_case(comp, comp_len, 31, 1, 1, out, sizeof(out), &out_len, &rc))
        return 0;
    if (rc != ZRAGF_STREAM_END || out_len != sizeof(payload) - 1u ||
        memcmp(out, payload, sizeof(payload) - 1u) != 0)
        return 0;

    return 1;
}

static int test_bad_gzip_reserved_flag(void)
{
    static const unsigned char payload[] = "gzip-reserved-flags";
    unsigned char comp[256];
    unsigned char out[64];
    size_t comp_len;
    size_t out_len;
    int rc;

    if (!build_stream(payload, sizeof(payload) - 1u, 31, comp, sizeof(comp), &comp_len))
        return 0;
    if (comp_len < 4u)
        return 0;
    comp[3] |= 0xE0u;

    if (!run_wrapper_case(comp, comp_len, 31, 1, 1, out, sizeof(out), &out_len, &rc))
        return 0;
    return rc == ZRAGF_DATA_ERROR;
}

static int test_bad_gzip_huge_comment_cap(void)
{
    unsigned char *buf;
    size_t len = 10u + PHASE18_GZIP_META_LIMIT + 32u;
    zragf_stream is;
    unsigned char out[8];
    int rc;

    buf = (unsigned char *)zragf_p89_host_take(len);
    if (!buf)
        return 0;

    memset(buf, 'A', len);
    buf[0] = 0x1fu;
    buf[1] = 0x8bu;
    buf[2] = 8u;
    buf[3] = 0x10u; /* FCOMMENT */
    buf[4] = 0u;
    buf[5] = 0u;
    buf[6] = 0u;
    buf[7] = 0u;
    buf[8] = 0u;
    buf[9] = 255u;
    /* no NUL terminator inside the cap */

    memset(&is, 0, sizeof(is));
    if (zragf_inflateInit2(&is, 31) != ZRAGF_OK) {
        zragf_p89_host_release(buf);
        return 0;
    }
    is.next_in = buf;
    is.avail_in = (unsigned int)len;
    is.next_out = out;
    is.avail_out = (unsigned int)sizeof(out);
    rc = zragf_inflateZ(&is, ZRAGF_NO_FLUSH);
    zragf_inflateEndZ(&is);
    zragf_p89_host_release(buf);

    return rc == ZRAGF_DATA_ERROR;
}

static int test_invalid_distance_too_far(void)
{
    static const unsigned char raw[] = { 0x03u, 0x02u, 0x00u };
    unsigned char out[32];
    size_t out_len;
    int rc;

    if (!run_wrapper_case(raw, sizeof(raw), -15, 0, 1, out, sizeof(out), &out_len, &rc))
        return 0;
    return rc == ZRAGF_DATA_ERROR;
}

static unsigned phase18_xorshift32(unsigned *state)
{
    unsigned x = *state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static int allowed_rc(int rc)
{
    switch (rc) {
        case ZRAGF_OK:
        case ZRAGF_STREAM_END:
        case ZRAGF_NEED_DICT:
        case ZRAGF_BUF_ERROR:
        case ZRAGF_DATA_ERROR:
        case ZRAGF_STREAM_ERROR:
        case ZRAGF_MEM_ERROR:
            return 1;
        default:
            return 0;
    }
}

static int test_deterministic_mutation_smoke(void)
{
    static const unsigned char seed0[] = { 0x03u, 0x00u };
    static const unsigned char seed1[] = { 0x03u, 0x02u, 0x00u };
    static const unsigned char seed2[] = { 0x1fu, 0x8bu, 0x08u, 0x00u, 0u, 0u, 0u, 0u, 0u, 255u };
    const unsigned char *seeds[] = { seed0, seed1, seed2 };
    const size_t seed_lens[] = { sizeof(seed0), sizeof(seed1), sizeof(seed2) };
    unsigned char buf[96];
    unsigned char out[256];
    size_t out_len;
    int rc;
    size_t i;
    unsigned state = 0xC0FFEEu;

    for (i = 0u; i < sizeof(seeds) / sizeof(seeds[0]); ++i) {
        unsigned iter;
        for (iter = 0u; iter < 256u; ++iter) {
            size_t j;
            memset(buf, 0, sizeof(buf));
            memcpy(buf, seeds[i], seed_lens[i]);
            for (j = seed_lens[i]; j < sizeof(buf); ++j)
                buf[j] = (unsigned char)phase18_xorshift32(&state);
            buf[phase18_xorshift32(&state) % sizeof(buf)] ^= (unsigned char)(1u << (phase18_xorshift32(&state) & 7u));
            if (!run_wrapper_case(buf, sizeof(buf), 47, 1, 1, out, sizeof(out), &out_len, &rc))
                return 0;
            if (!allowed_rc(rc))
                return 0;
        }
    }

    return 1;
}

int main(void)
{
    if (!test_valid_wrappers()) {
        fprintf(stderr, "phase18 valid wrapper replay failed\n");
        return 1;
    }
    if (!test_bad_gzip_reserved_flag()) {
        fprintf(stderr, "phase18 reserved gzip flag hardening failed\n");
        return 1;
    }
    if (!test_bad_gzip_huge_comment_cap()) {
        fprintf(stderr, "phase18 gzip metadata cap hardening failed\n");
        return 1;
    }
    if (!test_invalid_distance_too_far()) {
        fprintf(stderr, "phase18 invalid back-reference regression failed\n");
        return 1;
    }
    if (!test_deterministic_mutation_smoke()) {
        fprintf(stderr, "phase18 deterministic mutation smoke failed\n");
        return 1;
    }

    printf("phase18 fuzz regressions ok\n");
    return 0;
}
