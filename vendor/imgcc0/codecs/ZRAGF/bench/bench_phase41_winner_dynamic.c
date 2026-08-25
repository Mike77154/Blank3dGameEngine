#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"
#include "protocol89_fixed.h"
#include "protocol89_stdio.h"
#include "zragf_deflate/deflate_matcher.h"
#include "zragf_deflate/deflate_cost.h"
#include "zragf_deflate/deflate_emit.h"
#include "zragf_deflate/deflate_huffman_shared.h"

static unsigned char *make_jsonish(size_t *out_size)
{
    static const char *frag[] = {
        "{\"id\":", ",\"user\":\"miguel\",\"type\":\"event\",\"ok\":true,",
        "\"payload\":\"AAAAAAAAAAAAAAAA\",\"tags\":[\"alpha\",\"beta\",\"gamma\"]}\n"
    };
    unsigned char *buf;
    size_t i = 0u;
    *out_size = 180000u;
    buf = (unsigned char *)zragf_p89_host_take(*out_size);
    if (!buf) return NULL;
    while (i < *out_size) {
        char num[32];
        int len = zragf_p89_format(num, sizeof(num), "%lu", (unsigned long)i);
        size_t j, k;
        for (j = 0u; j < strlen(frag[0]) && i < *out_size; ++j) buf[i++] = (unsigned char)frag[0][j];
        for (j = 0u; j < (size_t)len && i < *out_size; ++j) buf[i++] = (unsigned char)num[j];
        for (k = 1u; k < 3u; ++k)
            for (j = 0u; j < strlen(frag[k]) && i < *out_size; ++j) buf[i++] = (unsigned char)frag[k][j];
    }
    return buf;
}

static zragf_fx bench_prepared(const unsigned char *src, size_t n, size_t *out_size)
{
    zragf_matcher_config cfg;
    zragf_token_buffer tb;
    zragf_block_stats stats;
    zragf_deflate_dynamic_prepared prep;
    zragf_deflate_cost_result cost;
    unsigned char *out = NULL;
    zragf_fx ms = 0;
    int i;
    clock_t t0;

    memset(&cfg, 0, sizeof(cfg));
    cfg.level = 6;
    cfg.strategy = ZRAGF_Z_DEFAULT_STRATEGY;
    cfg.nice_length = 64;
    cfg.max_chain = 128;

    if (!zragf_tokens_init(&tb, n / 2u + 64u))
        return 0;
    if (!zragf_deflate_build_tokens(NULL, 0u, src, n, &cfg, &tb, &stats)) {
        zragf_tokens_free(&tb);
        return 0;
    }
    if (!zragf_deflate_prepare_dynamic(&stats, &prep)) {
        zragf_tokens_free(&tb);
        return 0;
    }
    out = (unsigned char *)zragf_p89_host_take(n + 2048u);
    if (!out) {
        zragf_tokens_free(&tb);
        return 0;
    }

    t0 = clock();
    for (i = 0; i < 250; ++i) {
        size_t emitted = 0u;
        if (!zragf_deflate_cost_dynamic_prepared(tb.data, tb.size, &prep, 1, 0, 1, &cost) ||
            !zragf_deflate_emit_dynamic_block_prepared(tb.data, tb.size, &prep,
                                                       out, n + 2048u, &emitted,
                                                       1, NULL, NULL, 1)) {
            zragf_p89_host_release(out);
            zragf_tokens_free(&tb);
            return 0;
        }
        if (emitted != cost.size_bytes) {
            zragf_p89_host_release(out);
            zragf_tokens_free(&tb);
            return 0;
        }
        *out_size = emitted;
    }
    ms = 1000 * (zragf_fx)(clock() - t0) / (zragf_fx)CLOCKS_PER_SEC;
    zragf_p89_host_release(out);
    zragf_tokens_free(&tb);
    return ms;
}

int main(void)
{
    unsigned char *buf;
    size_t n = 0u;
    size_t emitted = 0u;
    zragf_fx ms;
    buf = make_jsonish(&n);
    if (!buf) return 1;
    ms = bench_prepared(buf, n, &emitted);
    printf("phase41 prepared dynamic jsonish: ms=%d size=%lu\n", ms, (unsigned long)emitted);
    zragf_p89_host_release(buf);
    return 0;
}
