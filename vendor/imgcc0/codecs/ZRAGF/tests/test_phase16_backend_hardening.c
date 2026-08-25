#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "zragflib_internal.h"
#include "zragf_deflate/deflate_core.h"
#include "zragf_deflate/deflate_matcher.h"
#include "zragf_deflate/deflate_cost.h"
#include "zragf_deflate/deflate_emit.h"
#include "zragf_deflate/deflate_huffman.h"
#include "zragf_deflate/deflate_huffman_shared.h"
#include "protocol89_hostmem.h"

static unsigned xorshift32(unsigned *s)
{
    unsigned x = *s;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *s = x;
    return x;
}

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

static void fill_noise(unsigned char *dst, size_t n)
{
    unsigned st = 0x12345678u;
    size_t i;
    for (i = 0u; i < n; ++i)
        dst[i] = (unsigned char)(xorshift32(&st) & 0xFFu);
}

static void fill_alternating(unsigned char *dst, size_t n)
{
    size_t i;
    for (i = 0u; i < n; ++i)
        dst[i] = (unsigned char)((i & 1u) ? 'B' : 'A');
}

static void fill_runs(unsigned char *dst, size_t n)
{
    size_t i = 0u;
    while (i < n) {
        size_t run = ((i / 257u) & 1u) ? 192u : 96u;
        unsigned char b = ((i / 1024u) & 1u) ? 0u : (unsigned char)('a' + ((i / 257u) % 13u));
        if (i + run > n)
            run = n - i;
        memset(dst + i, b, run);
        i += run;
    }
}

static void fill_hybrid(unsigned char *dst, size_t n)
{
    size_t a = n / 3u;
    size_t b = (n * 2u) / 3u;
    fill_noise(dst, a);
    fill_alternating(dst + a, b - a);
    fill_runs(dst + b, n - b);
}

int main(void)
{
    enum { N = 65536 };
    unsigned char *noise = (unsigned char *)zragf_p89_host_take((size_t)N);
    unsigned char *alt = (unsigned char *)zragf_p89_host_take((size_t)N);
    unsigned char *runs = (unsigned char *)zragf_p89_host_take((size_t)N);
    unsigned char *hybrid = (unsigned char *)zragf_p89_host_take((size_t)N);
    unsigned char *out = NULL;
    zragf_deflate_match_profile prof_noise;
    zragf_deflate_match_profile prof_alt;
    zragf_deflate_match_profile prof_runs;
    zragf_deflate_match_policy pol_noise;
    zragf_deflate_match_policy pol_alt;
    zragf_deflate_match_policy pol_runs;
    zragf_token_buffer tb;
    zragf_block_stats stats;
    zragf_matcher_config cfg;
    zragf_deflate_cost_result dyn_cost;
    zragf_deflate_cost_result fix_cost;
    size_t out_cap;
    size_t out_size = 0u;
    size_t dyn_size = 0u;
    zragf_huff_tables ht;
    int ll_count;
    int d_count;
    int hclen_count;

    if (!noise || !alt || !runs || !hybrid) {
        zragf_p89_host_release(noise); zragf_p89_host_release(alt); zragf_p89_host_release(runs); zragf_p89_host_release(hybrid);
        return 1;
    }

    fill_noise(noise, (size_t)N);
    fill_alternating(alt, (size_t)N);
    fill_runs(runs, (size_t)N);
    fill_hybrid(hybrid, (size_t)N);

    zragf_deflate_core_profile_input(NULL, 0u, noise, (size_t)N, &prof_noise);
    zragf_deflate_core_profile_input(NULL, 0u, alt, (size_t)N, &prof_alt);
    zragf_deflate_core_profile_input(NULL, 0u, runs, (size_t)N, &prof_runs);

    zragf_deflate_core_select_policy(9, ZRAGF_Z_DEFAULT_STRATEGY, 0, 0, 0, 0, 0,
                                     &prof_noise, &pol_noise);
    zragf_deflate_core_select_policy(9, ZRAGF_Z_DEFAULT_STRATEGY, 0, 0, 0, 0, 0,
                                     &prof_alt, &pol_alt);
    zragf_deflate_core_select_policy(6, ZRAGF_Z_DEFAULT_STRATEGY, 0, 0, 0, 0, 0,
                                     &prof_runs, &pol_runs);

    if (!prof_noise.incompressible_like || pol_noise.max_chain > 64 || pol_noise.lazy_probe > 4) {
        fprintf(stderr, "noise hardening profile failed inc=%d chain=%d lazy=%d\n",
                prof_noise.incompressible_like, pol_noise.max_chain, pol_noise.lazy_probe);
        goto fail;
    }
    if (!prof_alt.alternating_like || pol_alt.max_chain > 96 || pol_alt.nice_len > 64) {
        fprintf(stderr, "alternating hardening profile failed alt=%d chain=%d nice=%d\n",
                prof_alt.alternating_like, pol_alt.max_chain, pol_alt.nice_len);
        goto fail;
    }
    if (!prof_runs.rle_like || pol_runs.max_chain < 128 || pol_runs.nice_len < 128) {
        fprintf(stderr, "run-friendly profile failed rle=%d chain=%d nice=%d\n",
                prof_runs.rle_like, pol_runs.max_chain, pol_runs.nice_len);
        goto fail;
    }

    if (!zragf_tokens_init(&tb, (size_t)N / 2u + 64u)) {
        fprintf(stderr, "token buffer init failed\n");
        goto fail;
    }
    memset(&cfg, 0, sizeof(cfg));
    cfg.level = 9;
    cfg.strategy = ZRAGF_Z_DEFAULT_STRATEGY;
    if (!zragf_deflate_build_tokens(NULL, 0u, hybrid, (size_t)N, &cfg, &tb, &stats)) {
        fprintf(stderr, "tokenization failed\n");
        zragf_tokens_free(&tb);
        goto fail;
    }

    if (!zragf_deflate_cost_dynamic(tb.data, tb.size, &stats, 1, 0, 1, &dyn_cost) ||
        !zragf_deflate_cost_fixed(tb.data, tb.size, 1, 0, 1, &fix_cost)) {
        fprintf(stderr, "cost computation failed\n");
        zragf_tokens_free(&tb);
        goto fail;
    }

    out_cap = zragf_deflate_rfc1951_stored_bound((size_t)N) + 512u;
    out = (unsigned char *)zragf_p89_host_take(out_cap);
    if (!out) {
        zragf_tokens_free(&tb);
        goto fail;
    }

    {
        unsigned bitbuf = 0u;
        int bitcount = 0;
        if (!zragf_deflate_emit_dynamic_block(tb.data, tb.size, &stats,
                                              out, out_cap, &dyn_size,
                                              1, &bitbuf, &bitcount, 1)) {
            fprintf(stderr, "dynamic emit failed\n");
            zragf_tokens_free(&tb);
            goto fail;
        }
    }
    if (dyn_cost.size_bytes != dyn_size) {
        fprintf(stderr, "dynamic cost mismatch est=%lu got=%lu\n",
                (unsigned long)dyn_cost.size_bytes, (unsigned long)dyn_size);
        zragf_tokens_free(&tb);
        goto fail;
    }

    memset(&ht, 0, sizeof(ht));
    zragf_huff_build_lengths(&ht, &stats);
    if (ht.ll_len[256] == 0)
        ht.ll_len[256] = 1;
    zragf_huff_build_cl(&ht);
    zragf_huff_compute_header_sizes(&ht);
    zragf_deflate_huff_trim_counts(ht.ll_len, ht.d_len, &ll_count, &d_count, &hclen_count);
    if (ht.hlit != ll_count - 257 || ht.hdist != d_count - 1 || ht.hclen != hclen_count - 4) {
        fprintf(stderr, "shared huffman trim mismatch hlit=%d/%d hdist=%d/%d hclen=%d/%d\n",
                ht.hlit, ll_count - 257, ht.hdist, d_count - 1, ht.hclen, hclen_count - 4);
        zragf_tokens_free(&tb);
        goto fail;
    }

    if (!zragf_deflate_rfc1951_compress_chunk_with_dict(NULL, 0u,
                                                        hybrid, (size_t)N,
                                                        out, out_cap, &out_size,
                                                        1,
                                                        9,
                                                        ZRAGF_Z_DEFAULT_STRATEGY,
                                                        0, 0, 0, 0, 0)) {
        fprintf(stderr, "phase16 encode failed\n");
        zragf_tokens_free(&tb);
        goto fail;
    }
    if (!verify_raw_zlib(out, out_size, hybrid, (size_t)N)) {
        fprintf(stderr, "phase16 external verification failed\n");
        zragf_tokens_free(&tb);
        goto fail;
    }

    printf("phase16 hardening noise_chain=%d alt_chain=%d run_chain=%d dynamic=%lu fixed=%lu final=%lu\n",
           pol_noise.max_chain,
           pol_alt.max_chain,
           pol_runs.max_chain,
           (unsigned long)dyn_size,
           (unsigned long)fix_cost.size_bytes,
           (unsigned long)out_size);

    zragf_tokens_free(&tb);
    zragf_p89_host_release(noise); zragf_p89_host_release(alt); zragf_p89_host_release(runs); zragf_p89_host_release(hybrid); zragf_p89_host_release(out);
    return 0;

fail:
    zragf_p89_host_release(noise); zragf_p89_host_release(alt); zragf_p89_host_release(runs); zragf_p89_host_release(hybrid); zragf_p89_host_release(out);
    return 1;
}
