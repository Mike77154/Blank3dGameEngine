#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include "zragflib_internal.h"
#include "zragf_deflate/deflate_cost.h"
#include "zragf_deflate/deflate_emit.h"
#include "zragf_deflate/deflate_matcher.h"
#include "zragf_deflate/deflate_stream.h"
#include "protocol89_hostmem.h"

static int verify_raw_zlib(const unsigned char *compressed,
                           size_t compressed_size,
                           const unsigned char *expected,
                           size_t expected_size)
{
    z_stream zs;
    unsigned char *out;
    int rc;

    memset(&zs, 0, sizeof(zs));
    out = (unsigned char *)zragf_p89_host_take(expected_size + 64u);
    if (!out)
        return 0;

    rc = inflateInit2(&zs, -15);
    if (rc != Z_OK) {
        zragf_p89_host_release(out);
        return 0;
    }

    zs.next_in = (Bytef *)compressed;
    zs.avail_in = (uInt)compressed_size;
    zs.next_out = out;
    zs.avail_out = (uInt)(expected_size + 64u);

    rc = inflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END) {
        inflateEnd(&zs);
        zragf_p89_host_release(out);
        return 0;
    }
    if ((size_t)zs.total_out != expected_size || memcmp(out, expected, expected_size) != 0) {
        inflateEnd(&zs);
        zragf_p89_host_release(out);
        return 0;
    }
    inflateEnd(&zs);
    zragf_p89_host_release(out);
    return 1;
}

static void make_mixed(unsigned char *dst, size_t n)
{
    static const char text[] = "lorem ipsum dolor sit amet, consectetur adipiscing elit. ";
    size_t i;
    for (i = 0u; i < n; ++i) {
        if (i < n / 3u)
            dst[i] = (unsigned char)text[i % (sizeof(text) - 1u)];
        else if (i < (2u * n) / 3u)
            dst[i] = (unsigned char)((i / 7u) & 0xFFu);
        else
            dst[i] = (unsigned char)((i % 23u) ? 'A' : 0u);
    }
}

int main(void)
{
    enum { N = 32768 };
    unsigned char *src = (unsigned char *)zragf_p89_host_take((size_t)N);
    zragf_token_buffer tb;
    zragf_block_stats stats;
    zragf_matcher_config cfg;
    zragf_deflate_cost_result cost_fixed;
    zragf_deflate_cost_result cost_dynamic;
    zragf_deflate_cost_result cost_stored;
    unsigned char *buf_fixed = NULL;
    unsigned char *buf_dynamic = NULL;
    unsigned char *buf_stored = NULL;
    size_t cap;
    size_t got_fixed = 0u;
    size_t got_dynamic = 0u;
    size_t got_stored = 0u;
    zragf_deflate_params params;
    zragf_deflate_stream ds;
    unsigned char *stream_out = NULL;

    if (!src)
        return 1;
    make_mixed(src, (size_t)N);

    if (!zragf_tokens_init(&tb, (size_t)N / 2u + 32u)) {
        zragf_p89_host_release(src);
        return 1;
    }
    memset(&cfg, 0, sizeof(cfg));
    cfg.level = 6;
    cfg.strategy = ZRAGF_Z_DEFAULT_STRATEGY;
    if (!zragf_deflate_build_tokens(NULL, 0u, src, (size_t)N, &cfg, &tb, &stats)) {
        fprintf(stderr, "tokenize failed\n");
        zragf_tokens_free(&tb);
        zragf_p89_host_release(src);
        return 1;
    }

    if (!zragf_deflate_cost_fixed(tb.data, tb.size, 1, 0, 1, &cost_fixed) ||
        !zragf_deflate_cost_dynamic(tb.data, tb.size, &stats, 1, 0, 1, &cost_dynamic) ||
        !zragf_deflate_cost_stored(src, (size_t)N, 1, 0, 1, &cost_stored)) {
        fprintf(stderr, "cost estimator failed\n");
        zragf_tokens_free(&tb);
        zragf_p89_host_release(src);
        return 1;
    }

    cap = zragf_deflate_rfc1951_stored_bound((size_t)N) + 256u;
    buf_fixed = (unsigned char *)zragf_p89_host_take(cap);
    buf_dynamic = (unsigned char *)zragf_p89_host_take(cap);
    buf_stored = (unsigned char *)zragf_p89_host_take(cap);
    stream_out = (unsigned char *)zragf_p89_host_take(cap);
    if (!buf_fixed || !buf_dynamic || !buf_stored || !stream_out) {
        fprintf(stderr, "alloc failed\n");
        zragf_tokens_free(&tb);
        zragf_p89_host_release(src); zragf_p89_host_release(buf_fixed); zragf_p89_host_release(buf_dynamic); zragf_p89_host_release(buf_stored); zragf_p89_host_release(stream_out);
        return 1;
    }

    {
        unsigned bitbuf = 0u;
        int bitcount = 0;
        if (!zragf_deflate_emit_fixed_block(tb.data, tb.size,
                                            buf_fixed, cap, &got_fixed,
                                            1, &bitbuf, &bitcount, 1)) {
            fprintf(stderr, "fixed emit failed\n");
            goto fail;
        }
    }
    {
        unsigned bitbuf = 0u;
        int bitcount = 0;
        if (!zragf_deflate_emit_dynamic_block(tb.data, tb.size, &stats,
                                              buf_dynamic, cap, &got_dynamic,
                                              1, &bitbuf, &bitcount, 1)) {
            fprintf(stderr, "dynamic emit failed\n");
            goto fail;
        }
    }
    {
        unsigned bitbuf = 0u;
        int bitcount = 0;
        if (!zragf_deflate_emit_stored_chunk(src, (size_t)N,
                                             buf_stored, cap, &got_stored,
                                             1, &bitbuf, &bitcount)) {
            fprintf(stderr, "stored emit failed\n");
            goto fail;
        }
    }

    if (cost_fixed.size_bytes != got_fixed ||
        cost_dynamic.size_bytes != got_dynamic ||
        cost_stored.size_bytes != got_stored) {
        fprintf(stderr, "cost mismatch fixed=%lu/%lu dynamic=%lu/%lu stored=%lu/%lu\n",
                (unsigned long)cost_fixed.size_bytes, (unsigned long)got_fixed,
                (unsigned long)cost_dynamic.size_bytes, (unsigned long)got_dynamic,
                (unsigned long)cost_stored.size_bytes, (unsigned long)got_stored);
        goto fail;
    }

    memset(&params, 0, sizeof(params));
    params.level = 6;
    params.strategy = ZRAGF_Z_DEFAULT_STRATEGY;
    params.window_bits = 15;
    params.use_big_blocks = 1;
    params.window_size = 32768;
    if (!zragf_deflate_stream_init(&ds, &params, stream_out, cap)) {
        fprintf(stderr, "legacy stream init failed\n");
        goto fail;
    }
    if (!zragf_deflate_stream_write_block(&ds, src, (size_t)N / 2u, 0) ||
        !zragf_deflate_stream_write_block(&ds, src + ((size_t)N / 2u), (size_t)N / 2u, 1) ||
        !zragf_deflate_stream_flush(&ds)) {
        fprintf(stderr, "legacy stream path failed\n");
        goto fail;
    }
    if (!verify_raw_zlib(stream_out, zragf_deflate_stream_size(&ds), src, (size_t)N)) {
        fprintf(stderr, "legacy stream verification failed\n");
        goto fail;
    }

    printf("phase15 cost-model exact fixed=%lu dynamic=%lu stored=%lu stream=%lu\n",
           (unsigned long)got_fixed,
           (unsigned long)got_dynamic,
           (unsigned long)got_stored,
           (unsigned long)zragf_deflate_stream_size(&ds));

    zragf_tokens_free(&tb);
    zragf_p89_host_release(src); zragf_p89_host_release(buf_fixed); zragf_p89_host_release(buf_dynamic); zragf_p89_host_release(buf_stored); zragf_p89_host_release(stream_out);
    return 0;

fail:
    zragf_tokens_free(&tb);
    zragf_p89_host_release(src); zragf_p89_host_release(buf_fixed); zragf_p89_host_release(buf_dynamic); zragf_p89_host_release(buf_stored); zragf_p89_host_release(stream_out);
    return 1;
}
