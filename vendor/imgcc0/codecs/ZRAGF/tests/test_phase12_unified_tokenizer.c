#include "zragflib.h"
#include "zragflib_internal.h"
#include "zragf_deflate/deflate_matcher.h"
#include <stdio.h>
#include <string.h>
#include <zlib.h>

static int has_match(const zragf_token_buffer *tb)
{
    zragf_size_t i;
    for (i = 0; i < tb->size; ++i) {
        if (tb->data[i].type == ZRAGF_TOK_MATCH)
            return 1;
    }
    return 0;
}

int main(void)
{
    static const zragf_u8 dict[] = "HELLO-HELLO-HELLO-HELLO-";
    static const zragf_u8 src[]  = "HELLO-HELLO-HELLO-HELLO-world-world-world";
    zragf_matcher_config cfg;
    zragf_token_buffer with_dict;
    zragf_token_buffer without_dict;
    zragf_block_stats stats_with;
    zragf_block_stats stats_without;
    zragf_u8 comp[1024];
    zragf_size_t comp_size = 0u;
    z_stream zs;
    unsigned char out[1024];
    int rc;

    memset(&cfg, 0, sizeof(cfg));
    cfg.level = 6;
    cfg.strategy = ZRAGF_Z_DEFAULT_STRATEGY;

    if (!zragf_tokens_init(&with_dict, 0u) || !zragf_tokens_init(&without_dict, 0u)) {
        fprintf(stderr, "token init failed\n");
        return 1;
    }

    if (!zragf_deflate_build_tokens(NULL, 0u, src, sizeof(src) - 1u,
                                    &cfg, &without_dict, &stats_without) ||
        !zragf_deflate_build_tokens(dict, sizeof(dict) - 1u, src, sizeof(src) - 1u,
                                    &cfg, &with_dict, &stats_with)) {
        fprintf(stderr, "build tokens failed\n");
        zragf_tokens_free(&with_dict);
        zragf_tokens_free(&without_dict);
        return 1;
    }

    if (!(stats_with.raw_size == (int)(sizeof(src) - 1u) &&
          stats_without.raw_size == (int)(sizeof(src) - 1u) &&
          without_dict.size > 0u &&
          with_dict.size > 0u &&
          without_dict.data[0].type == ZRAGF_TOK_LITERAL &&
          with_dict.data[0].type == ZRAGF_TOK_MATCH &&
          has_match(&with_dict))) {
        fprintf(stderr, "token properties mismatch\n");
        zragf_tokens_free(&with_dict);
        zragf_tokens_free(&without_dict);
        return 1;
    }

    if (!zragf_deflate_rfc1951_compress_chunk_with_dict(dict, sizeof(dict) - 1u,
                                                        src, sizeof(src) - 1u,
                                                        comp, sizeof(comp), &comp_size,
                                                        1, 6, ZRAGF_Z_DEFAULT_STRATEGY,
                                                        0, 0, 0, 0, 0)) {
        fprintf(stderr, "compress with dict failed\n");
        zragf_tokens_free(&with_dict);
        zragf_tokens_free(&without_dict);
        return 1;
    }

    memset(&zs, 0, sizeof(zs));
    zs.next_in = comp;
    zs.avail_in = (uInt)comp_size;
    zs.next_out = out;
    zs.avail_out = (uInt)sizeof(out);

    rc = inflateInit2(&zs, -15);
    if (rc != Z_OK) {
        fprintf(stderr, "inflateInit2 failed: %d\n", rc);
        zragf_tokens_free(&with_dict);
        zragf_tokens_free(&without_dict);
        return 1;
    }
    rc = inflateSetDictionary(&zs, dict, (uInt)(sizeof(dict) - 1u));
    if (rc != Z_OK) {
        fprintf(stderr, "inflateSetDictionary failed: %d\n", rc);
        inflateEnd(&zs);
        zragf_tokens_free(&with_dict);
        zragf_tokens_free(&without_dict);
        return 1;
    }
    rc = inflate(&zs, Z_FINISH);
    if (rc != Z_STREAM_END || zs.total_out != sizeof(src) - 1u || memcmp(out, src, sizeof(src) - 1u) != 0) {
        fprintf(stderr, "inflate mismatch rc=%d total_out=%lu\n", rc, (unsigned long)zs.total_out);
        inflateEnd(&zs);
        zragf_tokens_free(&with_dict);
        zragf_tokens_free(&without_dict);
        return 1;
    }
    inflateEnd(&zs);

    zragf_tokens_free(&with_dict);
    zragf_tokens_free(&without_dict);
    printf("phase12 unified tokenizer ok\n");
    return 0;
}
