#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "zragflib.h"
#include "protocol89_hostmem.h"
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
        for (k = 1u; k < 3u; ++k) {
            for (j = 0u; j < strlen(frag[k]) && i < *out_size; ++j) buf[i++] = (unsigned char)frag[k][j];
        }
    }
    return buf;
}

int main(void)
{
    zragf_matcher_config cfg;
    zragf_token_buffer tb;
    zragf_block_stats stats;
    zragf_deflate_dynamic_prepared prep;
    zragf_deflate_cost_result cost;
    unsigned char *src = NULL;
    unsigned char *out = NULL;
    size_t n = 0u;
    size_t emitted = 0u;
    int i;

    memset(&cfg, 0, sizeof(cfg));
    cfg.level = 6;
    cfg.strategy = ZRAGF_Z_DEFAULT_STRATEGY;
    cfg.nice_length = 64;
    cfg.max_chain = 128;

    src = make_jsonish(&n);
    if (!src) return 1;
    if (!zragf_tokens_init(&tb, n / 2u + 64u)) { zragf_p89_host_release(src); return 1; }
    if (!zragf_deflate_build_tokens(NULL, 0u, src, n, &cfg, &tb, &stats)) {
        zragf_p89_host_release(src); zragf_tokens_free(&tb); return 1;
    }
    if (!zragf_deflate_prepare_dynamic(&stats, &prep)) {
        zragf_p89_host_release(src); zragf_tokens_free(&tb); return 1;
    }
    if (!prep.valid || prep.header_bits == 0u) {
        zragf_p89_host_release(src); zragf_tokens_free(&tb); return 1;
    }
    out = (unsigned char *)zragf_p89_host_take(n + 2048u);
    if (!out) { zragf_p89_host_release(src); zragf_tokens_free(&tb); return 1; }

    for (i = 0; i < 25; ++i) {
        if (!zragf_deflate_cost_dynamic_prepared(tb.data, tb.size, &prep, 1, 0, 1, &cost)) {
            zragf_p89_host_release(out); zragf_p89_host_release(src); zragf_tokens_free(&tb); return 1;
        }
        emitted = 0u;
        if (!zragf_deflate_emit_dynamic_block_prepared(tb.data, tb.size, &prep,
                                                       out, n + 2048u, &emitted,
                                                       1, NULL, NULL, 1)) {
            zragf_p89_host_release(out); zragf_p89_host_release(src); zragf_tokens_free(&tb); return 1;
        }
        if (emitted != cost.size_bytes) {
            zragf_p89_host_release(out); zragf_p89_host_release(src); zragf_tokens_free(&tb); return 1;
        }
    }

    printf("phase41 winner dynamic ok size=%lu header_bits=%u tokens=%lu\n",
           (unsigned long)emitted,
           prep.header_bits,
           (unsigned long)tb.size);
    zragf_p89_host_release(out);
    zragf_p89_host_release(src);
    zragf_tokens_free(&tb);
    return 0;
}
