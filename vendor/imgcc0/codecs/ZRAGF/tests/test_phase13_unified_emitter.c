#include "zragf_deflate/deflate_emit.h"
#include "zragf_deflate/deflate_matcher.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

static int build_tokens(const zragf_u8 *src,
                        zragf_size_t src_size,
                        zragf_token_buffer *tb,
                        zragf_block_stats *stats)
{
    zragf_matcher_config cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.level = 6;
    cfg.strategy = ZRAGF_Z_DEFAULT_STRATEGY;
    cfg.tune_set = 0;
    return zragf_deflate_build_tokens(NULL, 0u, src, src_size, &cfg, tb, stats);
}

static int raw_inflate_exact(const zragf_u8 *src,
                             zragf_size_t src_size,
                             zragf_u8 *dst,
                             zragf_size_t dst_cap,
                             zragf_size_t *dst_size)
{
    z_stream zs;
    int zr;

    memset(&zs, 0, sizeof(zs));
    zs.next_in = (Bytef *)src;
    zs.avail_in = (uInt)src_size;
    zs.next_out = dst;
    zs.avail_out = (uInt)dst_cap;

    zr = inflateInit2(&zs, -15);
    if (zr != Z_OK)
        return 0;

    zr = inflate(&zs, Z_FINISH);
    if (zr != Z_STREAM_END) {
        inflateEnd(&zs);
        return 0;
    }

    *dst_size = (zragf_size_t)zs.total_out;
    inflateEnd(&zs);
    return 1;
}

int main(void)
{
    const char *part1_str = "phase13-fixed-block phase13-fixed-block phase13-fixed-block "
                            "phase13-fixed-block phase13-fixed-block\n";
    const char *part3_str = "dynamic-tail lorem ipsum lorem ipsum lorem ipsum lorem ipsum "
                            "AAAAABBBBBCCCCCDDDDDEEEEEFFFFFGGGGGHHHHH dynamic-tail\n";
    zragf_u8 part2[513];
    zragf_token_buffer tb1;
    zragf_token_buffer tb3;
    zragf_block_stats st1;
    zragf_block_stats st3;
    zragf_u8 out[8192];
    zragf_u8 decoded[4096];
    zragf_u8 expected[4096];
    zragf_size_t out_pos = 0u;
    zragf_size_t wrote = 0u;
    zragf_size_t decoded_size = 0u;
    zragf_size_t expected_size = 0u;
    unsigned bitbuf = 0u;
    int bitcount = 0;
    int i;

    for (i = 0; i < (int)sizeof(part2); ++i)
        part2[i] = (zragf_u8)((i * 37 + 11) & 0xFF);

    memset(&tb1, 0, sizeof(tb1));
    memset(&tb3, 0, sizeof(tb3));
    memset(&st1, 0, sizeof(st1));
    memset(&st3, 0, sizeof(st3));

    if (!zragf_tokens_init(&tb1, 64u) || !zragf_tokens_init(&tb3, 64u)) {
        fprintf(stderr, "token init failed\n");
        return 1;
    }

    if (!build_tokens((const zragf_u8 *)part1_str, strlen(part1_str), &tb1, &st1)) {
        fprintf(stderr, "build tokens part1 failed\n");
        return 1;
    }
    if (!zragf_deflate_emit_fixed_block(tb1.data, tb1.size,
                                        out + out_pos, sizeof(out) - out_pos, &wrote,
                                        0, &bitbuf, &bitcount, 0)) {
        fprintf(stderr, "fixed emit failed\n");
        return 1;
    }
    out_pos += wrote;

    if (!zragf_deflate_emit_stored_chunk(part2, sizeof(part2),
                                         out + out_pos, sizeof(out) - out_pos, &wrote,
                                         0, &bitbuf, &bitcount)) {
        fprintf(stderr, "stored emit failed\n");
        return 1;
    }
    out_pos += wrote;

    if (!build_tokens((const zragf_u8 *)part3_str, strlen(part3_str), &tb3, &st3)) {
        fprintf(stderr, "build tokens part3 failed\n");
        return 1;
    }
    if (!zragf_deflate_emit_dynamic_block(tb3.data, tb3.size, &st3,
                                          out + out_pos, sizeof(out) - out_pos, &wrote,
                                          1, &bitbuf, &bitcount, 1)) {
        fprintf(stderr, "dynamic emit failed\n");
        return 1;
    }
    out_pos += wrote;

    memcpy(expected + expected_size, part1_str, strlen(part1_str));
    expected_size += strlen(part1_str);
    memcpy(expected + expected_size, part2, sizeof(part2));
    expected_size += sizeof(part2);
    memcpy(expected + expected_size, part3_str, strlen(part3_str));
    expected_size += strlen(part3_str);

    if (!raw_inflate_exact(out, out_pos, decoded, sizeof(decoded), &decoded_size)) {
        fprintf(stderr, "zlib raw inflate failed\n");
        return 1;
    }

    if (decoded_size != expected_size || memcmp(decoded, expected, expected_size) != 0) {
        fprintf(stderr, "decoded mismatch: got=%lu expected=%lu\n",
                (unsigned long)decoded_size,
                (unsigned long)expected_size);
        return 1;
    }

    printf("phase13 unified emitter ok: stream=%lu decoded=%lu\n",
           (unsigned long)out_pos,
           (unsigned long)decoded_size);

    zragf_tokens_free(&tb1);
    zragf_tokens_free(&tb3);
    return 0;
}
