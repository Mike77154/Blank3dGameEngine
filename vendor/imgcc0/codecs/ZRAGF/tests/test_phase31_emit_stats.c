#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

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
        size_t j;
        for (j = 0u; j < strlen(frag[0]) && i < *out_size; ++j) buf[i++] = (unsigned char)frag[0][j];
        for (j = 0u; j < (size_t)len && i < *out_size; ++j) buf[i++] = (unsigned char)num[j];
        for (j = 1u; j < 3u; ++j) {
            size_t k;
            for (k = 0u; k < strlen(frag[j]) && i < *out_size; ++k) buf[i++] = (unsigned char)frag[j][k];
        }
    }
    return buf;
}

static int inflate_raw_with_zlib(const unsigned char *comp,
                                 size_t comp_size,
                                 const unsigned char *expect,
                                 size_t expect_size)
{
    z_stream z;
    unsigned char *out;
    int rc;
    int ok = 0;

    out = (unsigned char *)zragf_p89_host_take(expect_size + 16u);
    if (!out)
        return 0;

    memset(&z, 0, sizeof(z));
    if (inflateInit2(&z, -15) != Z_OK) {
        zragf_p89_host_release(out);
        return 0;
    }
    z.next_in = (Bytef *)comp;
    z.avail_in = (uInt)comp_size;
    z.next_out = out;
    z.avail_out = (uInt)(expect_size + 16u);
    rc = inflate(&z, Z_FINISH);
    if (rc == Z_STREAM_END && z.total_out == expect_size && memcmp(out, expect, expect_size) == 0)
        ok = 1;
    inflateEnd(&z);
    zragf_p89_host_release(out);
    return ok;
}

int main(void)
{
    unsigned char *src = NULL;
    unsigned char *dst = NULL;
    size_t src_size = 0u;
    size_t dst_cap;
    size_t emitted = 0u;
    zragf_matcher_config cfg;
    zragf_token_buffer tb;
    zragf_block_stats stats;
    zragf_deflate_dynamic_prepared prep;
    zragf_deflate_cost_result cost;

    src = make_jsonish(&src_size);
    if (!src)
        return 1;

    memset(&cfg, 0, sizeof(cfg));
    cfg.level = 6;
    cfg.strategy = ZRAGF_Z_DEFAULT_STRATEGY;
    cfg.nice_length = 64;
    cfg.max_chain = 128;

    if (!zragf_tokens_init(&tb, src_size / 2u + 64u)) {
        zragf_p89_host_release(src);
        return 2;
    }

    if (!zragf_deflate_build_tokens(NULL, 0u, src, src_size, &cfg, &tb, &stats)) {
        zragf_tokens_free(&tb);
        zragf_p89_host_release(src);
        return 3;
    }

    if (!zragf_deflate_prepare_dynamic(&stats, &prep) || !prep.valid) {
        zragf_tokens_free(&tb);
        zragf_p89_host_release(src);
        return 4;
    }

    if (!zragf_deflate_cost_dynamic_prepared(tb.data, tb.size, &prep, 1, 0, 1, &cost)) {
        zragf_tokens_free(&tb);
        zragf_p89_host_release(src);
        return 5;
    }

    dst_cap = cost.size_bytes + 32u;
    dst = (unsigned char *)zragf_p89_host_take(dst_cap);
    if (!dst) {
        zragf_tokens_free(&tb);
        zragf_p89_host_release(src);
        return 6;
    }

    if (!zragf_deflate_emit_dynamic_block_prepared(tb.data, tb.size, &prep,
                                                   dst, dst_cap, &emitted,
                                                   1, NULL, NULL, 1)) {
        zragf_p89_host_release(dst);
        zragf_tokens_free(&tb);
        zragf_p89_host_release(src);
        return 7;
    }

    if (emitted != cost.size_bytes) {
        fprintf(stderr, "phase31 prepared cost mismatch: cost=%lu emitted=%lu\n",
                (unsigned long)cost.size_bytes, (unsigned long)emitted);
        zragf_p89_host_release(dst);
        zragf_tokens_free(&tb);
        zragf_p89_host_release(src);
        return 8;
    }

    if (!inflate_raw_with_zlib(dst, emitted, src, src_size)) {
        fprintf(stderr, "phase31 prepared emit rejected by zlib\n");
        zragf_p89_host_release(dst);
        zragf_tokens_free(&tb);
        zragf_p89_host_release(src);
        return 9;
    }

    printf("phase31 emit/stats reuse ok size=%lu tokens=%lu\n",
           (unsigned long)emitted,
           (unsigned long)tb.size);

    zragf_p89_host_release(dst);
    zragf_tokens_free(&tb);
    zragf_p89_host_release(src);
    return 0;
}
