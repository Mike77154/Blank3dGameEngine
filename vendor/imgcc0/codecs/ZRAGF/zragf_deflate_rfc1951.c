#include "zragflib_internal.h"
#include "zragf_deflate/deflate_blocks.h"
#include "zragf_deflate/deflate_emit.h"
#include "zragf_deflate/deflate_cost.h"
#include "zragf_deflate/deflate_plan.h"
#include "zragf_deflate/deflate_matcher.h"
#include <limits.h>
#include <stdlib.h>

#define ZRAGF_RFC_WINDOW    32768

zragf_size_t zragf_deflate_rfc1951_stored_bound(zragf_size_t src_size)
{
    zragf_size_t blocks;
    if (src_size == 0u)
        return 5u;
    blocks = (src_size + 65534u) / 65535u;
    return src_size + (blocks * 5u);
}

int zragf_deflate_rfc1951_store_chunk_stream(const zragf_u8 *src,
                                             zragf_size_t    src_size,
                                             zragf_u8       *dst,
                                             zragf_size_t    dst_cap,
                                             zragf_size_t   *dst_size,
                                             int             final_block,
                                             unsigned       *bitbuf_io,
                                             int            *bitcount_io)
{
    return zragf_deflate_emit_stored_chunk(src, src_size,
                                     dst, dst_cap, dst_size,
                                     final_block,
                                     bitbuf_io, bitcount_io);
}

int zragf_deflate_rfc1951_store_chunk(const zragf_u8 *src,
                                      zragf_size_t    src_size,
                                      zragf_u8       *dst,
                                      zragf_size_t    dst_cap,
                                      zragf_size_t   *dst_size,
                                      int             final_block)
{
    unsigned bitbuf = 0u;
    int bitcount = 0;
    return zragf_deflate_emit_stored_chunk(src, src_size,
                                     dst, dst_cap, dst_size,
                                     final_block,
                                     &bitbuf, &bitcount);
}

static int zragf_rfc_emit_candidate(zragf_block_type type,
                                    const zragf_u8 *src,
                                    zragf_size_t src_size,
                                    const zragf_token_buffer *tb,
                                    const zragf_block_stats *stats,
                                    const zragf_deflate_dynamic_prepared *dynprep,
                                    zragf_u8 *dst,
                                    zragf_size_t dst_cap,
                                    zragf_size_t *dst_size,
                                    int final_block,
                                    unsigned *bitbuf_io,
                                    int *bitcount_io,
                                    int flush_final_bits)
{
    if (!dst || !dst_size)
        return 0;

    switch (type) {
    case ZRAGF_BLOCK_STORED:
        return zragf_deflate_emit_stored_chunk(src, src_size,
                                               dst, dst_cap, dst_size,
                                               final_block,
                                               bitbuf_io, bitcount_io);
    case ZRAGF_BLOCK_FIXED:
        return zragf_deflate_emit_fixed_block(tb ? tb->data : NULL,
                                              tb ? tb->size : 0u,
                                              dst, dst_cap, dst_size,
                                              final_block,
                                              bitbuf_io, bitcount_io,
                                              flush_final_bits);
    case ZRAGF_BLOCK_DYNAMIC:
        if (dynprep && dynprep->valid) {
            return zragf_deflate_emit_dynamic_block_prepared(tb ? tb->data : NULL,
                                                             tb ? tb->size : 0u,
                                                             dynprep,
                                                             dst, dst_cap, dst_size,
                                                             final_block,
                                                             bitbuf_io, bitcount_io,
                                                             flush_final_bits);
        }
        return zragf_deflate_emit_dynamic_block(tb ? tb->data : NULL,
                                                tb ? tb->size : 0u,
                                                stats,
                                                dst, dst_cap, dst_size,
                                                final_block,
                                                bitbuf_io, bitcount_io,
                                                flush_final_bits);
    default:
        return 0;
    }
}

typedef struct {
    zragf_block_type type;
    zragf_size_t     size;
    int              end_bitcount;
    int              ok;
    int              tie_bias;
} zragf_rfc_candidate;

static int zragf_rfc_candidate_cmp(const void *a, const void *b)
{
    const zragf_rfc_candidate *ca = (const zragf_rfc_candidate *)a;
    const zragf_rfc_candidate *cb = (const zragf_rfc_candidate *)b;
    if (ca->ok != cb->ok)
        return cb->ok - ca->ok;
    if (!ca->ok)
        return 0;
    if (ca->size < cb->size)
        return -1;
    if (ca->size > cb->size)
        return 1;
    return cb->tie_bias - ca->tie_bias;
}

static int zragf_rfc_estimate_candidates(const zragf_u8 *src,
                                         zragf_size_t src_size,
                                         const zragf_token_buffer *tb,
                                         const zragf_block_stats *stats,
                                         const zragf_deflate_dynamic_prepared *dynprep,
                                         int final_block,
                                         int strategy,
                                         int start_bitcount,
                                         int flush_final_bits,
                                         zragf_rfc_candidate out[3],
                                         zragf_block_type *best_type)
{
    zragf_deflate_cost_result cost;
    zragf_block_type preferred;

    if (!out || !stats || !tb)
        return 0;

    preferred = zragf_blocks_choose_type(stats, NULL);
    memset(out, 0, sizeof(zragf_rfc_candidate) * 3u);

    out[0].type = ZRAGF_BLOCK_STORED;
    out[0].tie_bias = (preferred == ZRAGF_BLOCK_STORED) ? 3 : 0;
    if (zragf_deflate_cost_stored(src, src_size, final_block,
                                  start_bitcount, flush_final_bits, &cost)) {
        out[0].size = cost.size_bytes;
        out[0].end_bitcount = cost.end_bitcount;
        out[0].ok = 1;
    }

    out[1].type = ZRAGF_BLOCK_FIXED;
    out[1].tie_bias = (preferred == ZRAGF_BLOCK_FIXED) ? 2 : 0;
    if (zragf_deflate_cost_fixed(tb->data, tb->size,
                                 final_block,
                                 start_bitcount, flush_final_bits, &cost)) {
        out[1].size = cost.size_bytes;
        out[1].end_bitcount = cost.end_bitcount;
        out[1].ok = 1;
    }

    out[2].type = ZRAGF_BLOCK_DYNAMIC;
    out[2].tie_bias = (preferred == ZRAGF_BLOCK_DYNAMIC) ? 1 : 0;
    if (strategy != ZRAGF_Z_FIXED &&
        dynprep && dynprep->valid &&
        zragf_deflate_cost_dynamic_prepared(tb->data, tb->size, dynprep,
                                            final_block,
                                            start_bitcount, flush_final_bits, &cost)) {
        out[2].size = cost.size_bytes;
        out[2].end_bitcount = cost.end_bitcount;
        out[2].ok = 1;
    }

    qsort(out, 3u, sizeof(out[0]), zragf_rfc_candidate_cmp);
    if (best_type)
        *best_type = out[0].ok ? out[0].type : ZRAGF_BLOCK_STORED;
    return 1;
}

typedef struct {
    zragf_token_buffer tb;
    zragf_block_stats stats;
    zragf_deflate_dynamic_prepared dynprep;
    int valid;
} zragf_rfc_prepared_segment;

static void zragf_rfc_prepared_segment_init(zragf_rfc_prepared_segment *seg)
{
    if (!seg)
        return;
    memset(seg, 0, sizeof(*seg));
}

static void zragf_rfc_prepared_segment_free(zragf_rfc_prepared_segment *seg)
{
    if (!seg)
        return;
    if (seg->tb.data)
        zragf_tokens_free(&seg->tb);
    memset(seg, 0, sizeof(*seg));
}

static void zragf_rfc_prepared_segment_move(zragf_rfc_prepared_segment *dst,
                                            zragf_rfc_prepared_segment *src)
{
    if (!dst || !src || dst == src)
        return;
    zragf_rfc_prepared_segment_free(dst);
    *dst = *src;
    memset(src, 0, sizeof(*src));
}


static int zragf_rfc_prepared_segment_clone(zragf_rfc_prepared_segment *dst,
                                            const zragf_rfc_prepared_segment *src)
{
    if (!dst)
        return 0;
    zragf_rfc_prepared_segment_init(dst);
    if (!src || !src->valid || !src->tb.data)
        return 0;
    if (!zragf_tokens_init(&dst->tb, src->tb.size))
        return 0;
    if (src->tb.size > 0u)
        memcpy(dst->tb.data, src->tb.data, (size_t)(src->tb.size * sizeof(zragf_token)));
    dst->tb.size = src->tb.size;
    dst->stats = src->stats;
    dst->dynprep = src->dynprep;
    dst->valid = 1;
    return 1;
}

#define ZRAGF_RFC_ESTIMATE_CACHE_CAP 48
#define ZRAGF_RFC_PREPARED_SEGMENT_MAX 98304u
#define ZRAGF_RFC_PREPARED_TOKEN_MAX   49152u

static zragf_size_t zragf_rfc_initial_token_cap(zragf_size_t src_size);
static int zragf_rfc_build_tokens_and_stats(const zragf_u8 *dict,
                                            zragf_size_t dict_size,
                                            const zragf_u8 *src,
                                            zragf_size_t src_size,
                                            int level,
                                            int strategy,
                                            int tune_set,
                                            int good_length,
                                            int max_lazy,
                                            int nice_length,
                                            int max_chain,
                                            zragf_token_buffer *tb,
                                            zragf_block_stats *stats);

static int zragf_rfc_emit_single_known_type_impl(const zragf_u8 *dict,
                                                 zragf_size_t dict_size,
                                                 const zragf_u8 *src,
                                                 zragf_size_t src_size,
                                                 zragf_block_type type,
                                                 zragf_u8 *dst,
                                                 zragf_size_t dst_cap,
                                                 zragf_size_t *dst_size,
                                                 int final_block,
                                                 int level,
                                                 int strategy,
                                                 int tune_set,
                                                 int good_length,
                                                 int max_lazy,
                                                 int nice_length,
                                                 int max_chain,
                                                 unsigned *bitbuf_io,
                                                 int *bitcount_io,
                                                 int flush_final_bits)
{
    zragf_token_buffer tb;
    zragf_block_stats stats;
    zragf_deflate_dynamic_prepared dynprep;

    if (!dst || !dst_size)
        return 0;
    if (src_size > 0u && !src)
        return 0;

    if (type == ZRAGF_BLOCK_STORED || src_size == 0u) {
        return zragf_deflate_emit_stored_chunk(src, src_size,
                                               dst, dst_cap, dst_size,
                                               final_block,
                                               bitbuf_io, bitcount_io);
    }

    if (!zragf_tokens_init(&tb, zragf_rfc_initial_token_cap(src_size)))
        return 0;
    if (!zragf_rfc_build_tokens_and_stats(dict, dict_size,
                                          src, src_size,
                                          level, strategy,
                                          tune_set,
                                          good_length,
                                          max_lazy,
                                          nice_length,
                                          max_chain,
                                          &tb, &stats)) {
        zragf_tokens_free(&tb);
        return 0;
    }

    memset(&dynprep, 0, sizeof(dynprep));
    if (type == ZRAGF_BLOCK_DYNAMIC &&
        !zragf_deflate_prepare_dynamic(&stats, &dynprep)) {
        zragf_tokens_free(&tb);
        return 0;
    }

    switch (type) {
    case ZRAGF_BLOCK_FIXED:
        if (!zragf_deflate_emit_fixed_block(tb.data, tb.size,
                                            dst, dst_cap, dst_size,
                                            final_block,
                                            bitbuf_io, bitcount_io,
                                            flush_final_bits)) {
            zragf_tokens_free(&tb);
            return 0;
        }
        break;
    case ZRAGF_BLOCK_DYNAMIC:
        if (!dynprep.valid ||
            !zragf_deflate_emit_dynamic_block_prepared(tb.data, tb.size,
                                                       &dynprep,
                                                       dst, dst_cap, dst_size,
                                                       final_block,
                                                       bitbuf_io, bitcount_io,
                                                       flush_final_bits)) {
            zragf_tokens_free(&tb);
            return 0;
        }
        break;
    default:
        zragf_tokens_free(&tb);
        return 0;
    }

    zragf_tokens_free(&tb);
    return 1;
}

static zragf_size_t zragf_rfc_initial_token_cap(zragf_size_t src_size)
{
    if (src_size <= 4096u)
        return src_size / 2u + 32u;
    if (src_size <= 65536u)
        return src_size * 3u / 4u + 64u;
    if (src_size <= 262144u)
        return src_size * 7u / 8u + 128u;
    return src_size + 128u;
}

static int zragf_rfc_build_tokens_and_stats(const zragf_u8 *dict,
                                            zragf_size_t dict_size,
                                            const zragf_u8 *src,
                                            zragf_size_t src_size,
                                            int level,
                                            int strategy,
                                            int tune_set,
                                            int good_length,
                                            int max_lazy,
                                            int nice_length,
                                            int max_chain,
                                            zragf_token_buffer *tb,
                                            zragf_block_stats *stats)
{
    zragf_matcher_config matcher_cfg;

    if (!tb || !stats)
        return 0;

    matcher_cfg.level = level;
    matcher_cfg.strategy = strategy;
    matcher_cfg.tune_set = tune_set;
    matcher_cfg.good_length = good_length;
    matcher_cfg.max_lazy = max_lazy;
    matcher_cfg.nice_length = nice_length;
    matcher_cfg.max_chain = max_chain;

    return zragf_deflate_build_tokens(dict, dict_size,
                                      src, src_size,
                                      &matcher_cfg, tb, stats);
}

static int zragf_rfc_estimate_single_impl(const zragf_u8 *dict,
                                          zragf_size_t dict_size,
                                          const zragf_u8 *src,
                                          zragf_size_t src_size,
                                          int final_block,
                                          int level,
                                          int strategy,
                                          int tune_set,
                                          int good_length,
                                          int max_lazy,
                                          int nice_length,
                                          int max_chain,
                                          int start_bitcount,
                                          int flush_final_bits,
                                          zragf_size_t *out_size,
                                          int *out_end_bitcount,
                                          zragf_block_type *out_best_type,
                                          zragf_rfc_prepared_segment *out_prepared)
{
    zragf_token_buffer tb;
    zragf_block_stats stats;
    zragf_deflate_dynamic_prepared dynprep;
    zragf_rfc_candidate cands[3];
    zragf_rfc_prepared_segment local_prepared;

    if (out_prepared)
        zragf_rfc_prepared_segment_init(out_prepared);
    zragf_rfc_prepared_segment_init(&local_prepared);
    if (!out_size)
        return 0;
    if (src_size > 0u && !src)
        return 0;

    if (src_size == 0u) {
        zragf_deflate_cost_result cost;
        if (!zragf_deflate_cost_stored(src, src_size, final_block,
                                       start_bitcount, flush_final_bits, &cost))
            return 0;
        *out_size = cost.size_bytes;
        if (out_end_bitcount)
            *out_end_bitcount = cost.end_bitcount;
        if (out_best_type)
            *out_best_type = ZRAGF_BLOCK_STORED;
        return 1;
    }

    if (!zragf_tokens_init(&tb, zragf_rfc_initial_token_cap(src_size)))
        return 0;
    if (!zragf_rfc_build_tokens_and_stats(dict, dict_size,
                                          src, src_size,
                                          level, strategy,
                                          tune_set,
                                          good_length,
                                          max_lazy,
                                          nice_length,
                                          max_chain,
                                          &tb, &stats)) {
        zragf_tokens_free(&tb);
        return 0;
    }
    memset(&dynprep, 0, sizeof(dynprep));
    if (strategy != ZRAGF_Z_FIXED && !zragf_deflate_prepare_dynamic(&stats, &dynprep)) {
        zragf_tokens_free(&tb);
        return 0;
    }
    if (!zragf_rfc_estimate_candidates(src, src_size, &tb, &stats, &dynprep,
                                       final_block, strategy,
                                       start_bitcount, flush_final_bits,
                                       cands,
                                       out_best_type)) {
        zragf_tokens_free(&tb);
        return 0;
    }
    if (!cands[0].ok) {
        zragf_tokens_free(&tb);
        return 0;
    }
    *out_size = cands[0].size;
    if (out_end_bitcount)
        *out_end_bitcount = cands[0].end_bitcount;
    if (out_prepared && cands[0].type != ZRAGF_BLOCK_STORED &&
        src_size <= ZRAGF_RFC_PREPARED_SEGMENT_MAX &&
        tb.size <= ZRAGF_RFC_PREPARED_TOKEN_MAX) {
        local_prepared.tb = tb;
        memset(&tb, 0, sizeof(tb));
        local_prepared.stats = stats;
        local_prepared.dynprep = dynprep;
        local_prepared.valid = 1;
        zragf_rfc_prepared_segment_move(out_prepared, &local_prepared);
    }
    zragf_tokens_free(&tb);
    return 1;
}

static int zragf_deflate_rfc1951_compress_single_impl(const zragf_u8 *dict,
                                               zragf_size_t dict_size,
                                               const zragf_u8 *src,
                                               zragf_size_t src_size,
                                               zragf_u8 *dst,
                                               zragf_size_t dst_cap,
                                               zragf_size_t *dst_size,
                                               int final_block,
                                               int level,
                                               int strategy,
                                               int tune_set,
                                               int good_length,
                                               int max_lazy,
                                               int nice_length,
                                               int max_chain,
                                               unsigned *bitbuf_io,
                                               int *bitcount_io,
                                               int flush_final_bits)
{
    zragf_token_buffer tb;
    zragf_block_stats stats;
    zragf_deflate_dynamic_prepared dynprep;
    zragf_rfc_candidate cands[3];
    unsigned start_bitbuf = bitbuf_io ? *bitbuf_io : 0u;
    int start_bitcount = bitcount_io ? *bitcount_io : 0;
    int i;

    if (!dst || !dst_size)
        return 0;
    if (src_size > 0u && !src)
        return 0;

    if (src_size == 0u)
        return zragf_deflate_emit_stored_chunk(src, src_size,
                                         dst, dst_cap, dst_size,
                                         final_block ? 1 : 0,
                                         bitbuf_io, bitcount_io);

    if (!zragf_tokens_init(&tb, zragf_rfc_initial_token_cap(src_size)))
        return 0;

    if (!zragf_rfc_build_tokens_and_stats(dict, dict_size,
                                          src, src_size,
                                          level, strategy,
                                          tune_set,
                                          good_length,
                                          max_lazy,
                                          nice_length,
                                          max_chain,
                                          &tb, &stats)) {
        zragf_tokens_free(&tb);
        return 0;
    }

    memset(&dynprep, 0, sizeof(dynprep));
    if (strategy != ZRAGF_Z_FIXED && !zragf_deflate_prepare_dynamic(&stats, &dynprep)) {
        zragf_tokens_free(&tb);
        return 0;
    }

    if (!zragf_rfc_estimate_candidates(src, src_size, &tb, &stats, &dynprep,
                                       final_block, strategy,
                                       start_bitcount, flush_final_bits,
                                       cands,
                                       NULL)) {
        zragf_tokens_free(&tb);
        return 0;
    }

    for (i = 0; i < 3; ++i) {
        unsigned local_bitbuf = start_bitbuf;
        int local_bitcount = start_bitcount;
        if (!cands[i].ok)
            continue;
        if (cands[i].size > dst_cap)
            continue;
        if (zragf_rfc_emit_candidate(cands[i].type,
                                     src, src_size,
                                     &tb, &stats,
                                     &dynprep,
                                     dst, dst_cap, dst_size,
                                     final_block,
                                     &local_bitbuf,
                                     &local_bitcount,
                                     flush_final_bits)) {
            if (bitbuf_io)
                *bitbuf_io = local_bitbuf;
            if (bitcount_io)
                *bitcount_io = local_bitcount;
            zragf_tokens_free(&tb);
            return 1;
        }
    }

    zragf_tokens_free(&tb);
    return 0;
}


#define ZRAGF_RFC_SPLIT_DEPTH 2
#define ZRAGF_RFC_SPLIT_EXTRA_BOUND 64u
#define ZRAGF_RFC_MAX_PROBED_SPLITS 3
#define ZRAGF_RFC_MAX_PARTITION_SPLITS 4
#define ZRAGF_RFC_PARTITION_TOP_PROBES 4
#define ZRAGF_RFC_PARTITION_MAX_SEARCH_SPLITS 4
#define ZRAGF_RFC_PARTITION_SLACK 512u
#define ZRAGF_RFC_PARTITION_RECURSE_SLACK 512u

static int zragf_deflate_rfc1951_compress_single_impl(const zragf_u8 *dict,
                                               zragf_size_t dict_size,
                                               const zragf_u8 *src,
                                               zragf_size_t src_size,
                                               zragf_u8 *dst,
                                               zragf_size_t dst_cap,
                                               zragf_size_t *dst_size,
                                               int final_block,
                                               int level,
                                               int strategy,
                                               int tune_set,
                                               int good_length,
                                               int max_lazy,
                                               int nice_length,
                                               int max_chain,
                                               unsigned *bitbuf_io,
                                               int *bitcount_io,
                                               int flush_final_bits);

typedef struct {
    zragf_size_t offsets[ZRAGF_RFC_MAX_PARTITION_SPLITS];
    int count;
    zragf_size_t total_size;
    int end_bitcount;
    int valid;
} zragf_rfc_partition;

typedef struct {
    zragf_size_t start;
    zragf_size_t end;
    int final_block;
    int start_bitcount;
    int flush_final_bits;
    zragf_size_t size;
    int end_bitcount;
    zragf_block_type best_type;
    zragf_rfc_prepared_segment prepared;
    int valid;
} zragf_rfc_estimate_cache_entry;

#define ZRAGF_RFC_SUFFIX_CACHE_CAP 128
#define ZRAGF_RFC_SUFFIX_LB_CACHE_CAP 160
#define ZRAGF_RFC_SUFFIX_EXACT_CACHE_CAP 96
#define ZRAGF_RFC_SUFFIX_EXACT_QUERY_CACHE_CAP 128
#define ZRAGF_RFC_SUFFIX_LB_QUERY_CACHE_CAP 256
#define ZRAGF_RFC_SUFFIX_QUERY_CACHE_CAP 256
#define ZRAGF_RFC_ESTIMATE_SLOT_HINT_CAP 64
#define ZRAGF_RFC_ESTIMATE_PREPARED_SLOT_HINT_CAP 64
#define ZRAGF_RFC_ESTIMATE_PREPARED_QUERY_CACHE_CAP 128
#define ZRAGF_RFC_ESTIMATE_PREPARED_MISS_HINT_CAP 64
#define ZRAGF_RFC_SUFFIX_SLOT_HINT_CAP 128
#define ZRAGF_RFC_SUFFIX_LB_SLOT_HINT_CAP 128
#define ZRAGF_RFC_SUFFIX_EXACT_SLOT_HINT_CAP 64
#define ZRAGF_RFC_SUFFIX_MISS_HINT_CAP 128
#define ZRAGF_RFC_SUFFIX_LB_MISS_HINT_CAP 128
#define ZRAGF_RFC_SUFFIX_EXACT_MISS_HINT_CAP 64

typedef struct {
    zragf_size_t start;
    int next_index;
    int remaining_splits;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    zragf_rfc_partition part;
    int valid;
} zragf_rfc_suffix_cache_entry;

typedef struct {
    zragf_size_t start;
    int next_index;
    int remaining_splits;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    zragf_rfc_partition part;
    int valid;
} zragf_rfc_suffix_query_cache_entry;

typedef struct {
    zragf_size_t start;
    int next_index;
    int remaining_splits;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    zragf_size_t total_size;
    int valid;
} zragf_rfc_suffix_lb_cache_entry;

typedef struct {
    zragf_size_t start;
    int next_index;
    int remaining_splits;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    zragf_size_t total_size;
    int valid;
} zragf_rfc_suffix_lb_query_cache_entry;

typedef struct {
    zragf_size_t start;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    zragf_size_t total_size;
    int end_bitcount;
    int valid;
} zragf_rfc_suffix_exact_cache_entry;

typedef struct {
    zragf_size_t start;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    zragf_size_t total_size;
    int end_bitcount;
    int valid;
} zragf_rfc_suffix_exact_query_cache_entry;

typedef struct {
    zragf_size_t start;
    zragf_size_t end;
    int final_block;
    int start_bitcount;
    int flush_final_bits;
    unsigned slot;
    int valid;
} zragf_rfc_estimate_slot_hint_entry;

typedef struct {
    zragf_size_t start;
    zragf_size_t end;
    unsigned slot;
    int valid;
} zragf_rfc_estimate_prepared_slot_hint_entry;

typedef struct {
    zragf_size_t start;
    zragf_size_t end;
    unsigned slot;
    unsigned generation;
    int valid;
} zragf_rfc_estimate_prepared_query_cache_entry;

typedef struct {
    zragf_size_t start;
    zragf_size_t end;
    unsigned generation;
    int valid;
} zragf_rfc_estimate_prepared_miss_hint_entry;

typedef struct {
    zragf_size_t start;
    int next_index;
    int remaining_splits;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    unsigned slot;
    int valid;
} zragf_rfc_suffix_slot_hint_entry;

typedef struct {
    zragf_size_t start;
    int next_index;
    int remaining_splits;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    unsigned slot;
    int valid;
} zragf_rfc_suffix_lb_slot_hint_entry;

typedef struct {
    zragf_size_t start;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    unsigned slot;
    int valid;
} zragf_rfc_suffix_exact_slot_hint_entry;

typedef struct {
    zragf_size_t start;
    int next_index;
    int remaining_splits;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    unsigned generation;
    int valid;
} zragf_rfc_suffix_miss_hint_entry;

typedef struct {
    zragf_size_t start;
    int next_index;
    int remaining_splits;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    unsigned generation;
    int valid;
} zragf_rfc_suffix_lb_miss_hint_entry;

typedef struct {
    zragf_size_t start;
    int start_bitcount;
    int final_block;
    int flush_final_bits;
    unsigned generation;
    int valid;
} zragf_rfc_suffix_exact_miss_hint_entry;

typedef struct {
    zragf_rfc_estimate_cache_entry entries[ZRAGF_RFC_ESTIMATE_CACHE_CAP];
    zragf_rfc_suffix_cache_entry suffix_entries[ZRAGF_RFC_SUFFIX_CACHE_CAP];
    zragf_rfc_suffix_query_cache_entry suffix_query_entries[ZRAGF_RFC_SUFFIX_QUERY_CACHE_CAP];
    zragf_rfc_suffix_lb_cache_entry suffix_lb_entries[ZRAGF_RFC_SUFFIX_LB_CACHE_CAP];
    zragf_rfc_suffix_lb_query_cache_entry suffix_lb_query_entries[ZRAGF_RFC_SUFFIX_LB_QUERY_CACHE_CAP];
    zragf_rfc_suffix_exact_cache_entry suffix_exact_entries[ZRAGF_RFC_SUFFIX_EXACT_CACHE_CAP];
    zragf_rfc_suffix_exact_query_cache_entry suffix_exact_query_entries[ZRAGF_RFC_SUFFIX_EXACT_QUERY_CACHE_CAP];
    zragf_rfc_estimate_slot_hint_entry estimate_slot_hints[ZRAGF_RFC_ESTIMATE_SLOT_HINT_CAP];
    zragf_rfc_estimate_prepared_slot_hint_entry estimate_prepared_slot_hints[ZRAGF_RFC_ESTIMATE_PREPARED_SLOT_HINT_CAP];
    zragf_rfc_estimate_prepared_query_cache_entry estimate_prepared_query_entries[ZRAGF_RFC_ESTIMATE_PREPARED_QUERY_CACHE_CAP];
    zragf_rfc_estimate_prepared_miss_hint_entry estimate_prepared_miss_hints[ZRAGF_RFC_ESTIMATE_PREPARED_MISS_HINT_CAP];
    zragf_rfc_suffix_slot_hint_entry suffix_slot_hints[ZRAGF_RFC_SUFFIX_SLOT_HINT_CAP];
    zragf_rfc_suffix_lb_slot_hint_entry suffix_lb_slot_hints[ZRAGF_RFC_SUFFIX_LB_SLOT_HINT_CAP];
    zragf_rfc_suffix_exact_slot_hint_entry suffix_exact_slot_hints[ZRAGF_RFC_SUFFIX_EXACT_SLOT_HINT_CAP];
    zragf_rfc_suffix_miss_hint_entry suffix_miss_hints[ZRAGF_RFC_SUFFIX_MISS_HINT_CAP];
    zragf_rfc_suffix_lb_miss_hint_entry suffix_lb_miss_hints[ZRAGF_RFC_SUFFIX_LB_MISS_HINT_CAP];
    zragf_rfc_suffix_exact_miss_hint_entry suffix_exact_miss_hints[ZRAGF_RFC_SUFFIX_EXACT_MISS_HINT_CAP];
    unsigned next_slot;
    unsigned estimate_prepared_generation;
    unsigned suffix_generation;
    unsigned suffix_lb_generation;
    unsigned suffix_exact_generation;
    unsigned suffix_next_slot;
    unsigned suffix_lb_next_slot;
    unsigned suffix_exact_next_slot;
    unsigned hits;
    unsigned misses;
    unsigned suffix_query_hits;
    unsigned suffix_hits;
    unsigned suffix_relaxed_hits;
    unsigned suffix_lower_bound_hits;
    unsigned suffix_lower_bound_query_hits;
    unsigned suffix_lower_bound_relaxed_hits;
    unsigned suffix_exact_hits;
    unsigned suffix_exact_query_hits;
    unsigned suffix_misses;
} zragf_rfc_estimate_cache;

static unsigned zragf_rfc_estimate_slot_hint_hash(zragf_size_t start,
                                                     zragf_size_t end,
                                                     int final_block,
                                                     int start_bitcount,
                                                     int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)(end ^ (end >> 9));
    h ^= (unsigned)((unsigned)final_block * 131u);
    h ^= (unsigned)((unsigned)start_bitcount * 977u);
    h ^= (unsigned)((unsigned)flush_final_bits * 8191u);
    return h % ZRAGF_RFC_ESTIMATE_SLOT_HINT_CAP;
}

static unsigned zragf_rfc_estimate_slot_hint_hash_alt(zragf_size_t start,
                                                         zragf_size_t end,
                                                         int final_block,
                                                         int start_bitcount,
                                                         int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 11));
    h ^= (unsigned)(end ^ (end >> 13));
    h ^= (unsigned)((unsigned)final_block * 257u);
    h ^= (unsigned)((unsigned)start_bitcount * 65537u);
    h ^= (unsigned)((unsigned)flush_final_bits * 131071u);
    h ^= 0x9e3779b9u;
    return h % ZRAGF_RFC_ESTIMATE_SLOT_HINT_CAP;
}

static int zragf_rfc_estimate_slot_hint_matches(const zragf_rfc_estimate_slot_hint_entry *entry,
                                                zragf_size_t start,
                                                zragf_size_t end,
                                                int final_block,
                                                int start_bitcount,
                                                int flush_final_bits)
{
    return entry && entry->valid &&
           entry->start == start &&
           entry->end == end &&
           entry->final_block == final_block &&
           entry->start_bitcount == start_bitcount &&
           entry->flush_final_bits == flush_final_bits;
}

static zragf_rfc_estimate_slot_hint_entry *zragf_rfc_estimate_slot_hint_find(zragf_rfc_estimate_cache *cache,
                                                                              zragf_size_t start,
                                                                              zragf_size_t end,
                                                                              int final_block,
                                                                              int start_bitcount,
                                                                              int flush_final_bits)
{
    zragf_rfc_estimate_slot_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return NULL;
    idx = zragf_rfc_estimate_slot_hint_hash(start,
                                            end,
                                            final_block,
                                            start_bitcount,
                                            flush_final_bits);
    entry = &cache->estimate_slot_hints[idx];
    if (zragf_rfc_estimate_slot_hint_matches(entry,
                                             start,
                                             end,
                                             final_block,
                                             start_bitcount,
                                             flush_final_bits))
        return entry;
    alt_idx = zragf_rfc_estimate_slot_hint_hash_alt(start,
                                                    end,
                                                    final_block,
                                                    start_bitcount,
                                                    flush_final_bits);
    if (alt_idx == idx)
        return NULL;
    entry = &cache->estimate_slot_hints[alt_idx];
    if (zragf_rfc_estimate_slot_hint_matches(entry,
                                             start,
                                             end,
                                             final_block,
                                             start_bitcount,
                                             flush_final_bits))
        return entry;
    return NULL;
}

static void zragf_rfc_estimate_slot_hint_store(zragf_rfc_estimate_cache *cache,
                                               zragf_size_t start,
                                               zragf_size_t end,
                                               int final_block,
                                               int start_bitcount,
                                               int flush_final_bits,
                                               unsigned slot)
{
    zragf_rfc_estimate_slot_hint_entry *entry1;
    zragf_rfc_estimate_slot_hint_entry *entry2 = NULL;
    zragf_rfc_estimate_slot_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return;
    idx = zragf_rfc_estimate_slot_hint_hash(start,
                                            end,
                                            final_block,
                                            start_bitcount,
                                            flush_final_bits);
    entry1 = &cache->estimate_slot_hints[idx];
    alt_idx = zragf_rfc_estimate_slot_hint_hash_alt(start,
                                                    end,
                                                    final_block,
                                                    start_bitcount,
                                                    flush_final_bits);
    if (alt_idx != idx)
        entry2 = &cache->estimate_slot_hints[alt_idx];
    if (zragf_rfc_estimate_slot_hint_matches(entry1,
                                             start,
                                             end,
                                             final_block,
                                             start_bitcount,
                                             flush_final_bits))
        entry = entry1;
    else if (entry2 && zragf_rfc_estimate_slot_hint_matches(entry2,
                                                            start,
                                                            end,
                                                            final_block,
                                                            start_bitcount,
                                                            flush_final_bits))
        entry = entry2;
    else if (!entry1->valid)
        entry = entry1;
    else if (entry2 && !entry2->valid)
        entry = entry2;
    else if (entry2 && (((slot + idx + alt_idx) & 1u) != 0u))
        entry = entry2;
    else
        entry = entry1;
    entry->start = start;
    entry->end = end;
    entry->final_block = final_block;
    entry->start_bitcount = start_bitcount;
    entry->flush_final_bits = flush_final_bits;
    entry->slot = slot;
    entry->valid = 1;
}

static unsigned zragf_rfc_estimate_prepared_slot_hint_hash(zragf_size_t start,
                                                            zragf_size_t end)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)(end ^ (end >> 9));
    h ^= 0x9e3779b9u;
    return h % ZRAGF_RFC_ESTIMATE_PREPARED_SLOT_HINT_CAP;
}

static unsigned zragf_rfc_estimate_prepared_slot_hint_hash_alt(zragf_size_t start,
                                                                zragf_size_t end)
{
    unsigned h = (unsigned)(start ^ (start >> 11));
    h ^= (unsigned)(end ^ (end >> 13));
    h ^= 0x85ebca6bu;
    return h % ZRAGF_RFC_ESTIMATE_PREPARED_SLOT_HINT_CAP;
}

static int zragf_rfc_estimate_prepared_slot_hint_matches(const zragf_rfc_estimate_prepared_slot_hint_entry *entry,
                                                         zragf_size_t start,
                                                         zragf_size_t end)
{
    return entry && entry->valid && entry->start == start && entry->end == end;
}

static zragf_rfc_estimate_prepared_slot_hint_entry *zragf_rfc_estimate_prepared_slot_hint_find(zragf_rfc_estimate_cache *cache,
                                                                                                zragf_size_t start,
                                                                                                zragf_size_t end)
{
    zragf_rfc_estimate_prepared_slot_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return NULL;
    idx = zragf_rfc_estimate_prepared_slot_hint_hash(start, end);
    entry = &cache->estimate_prepared_slot_hints[idx];
    if (zragf_rfc_estimate_prepared_slot_hint_matches(entry, start, end))
        return entry;
    alt_idx = zragf_rfc_estimate_prepared_slot_hint_hash_alt(start, end);
    if (alt_idx == idx)
        return NULL;
    entry = &cache->estimate_prepared_slot_hints[alt_idx];
    if (zragf_rfc_estimate_prepared_slot_hint_matches(entry, start, end))
        return entry;
    return NULL;
}

static void zragf_rfc_estimate_prepared_slot_hint_store(zragf_rfc_estimate_cache *cache,
                                                         zragf_size_t start,
                                                         zragf_size_t end,
                                                         unsigned slot)
{
    zragf_rfc_estimate_prepared_slot_hint_entry *entry1;
    zragf_rfc_estimate_prepared_slot_hint_entry *entry2 = NULL;
    zragf_rfc_estimate_prepared_slot_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return;
    idx = zragf_rfc_estimate_prepared_slot_hint_hash(start, end);
    entry1 = &cache->estimate_prepared_slot_hints[idx];
    alt_idx = zragf_rfc_estimate_prepared_slot_hint_hash_alt(start, end);
    if (alt_idx != idx)
        entry2 = &cache->estimate_prepared_slot_hints[alt_idx];
    if (zragf_rfc_estimate_prepared_slot_hint_matches(entry1, start, end))
        entry = entry1;
    else if (entry2 && zragf_rfc_estimate_prepared_slot_hint_matches(entry2, start, end))
        entry = entry2;
    else if (!entry1->valid)
        entry = entry1;
    else if (entry2 && !entry2->valid)
        entry = entry2;
    else if (entry2 && (((slot + idx + alt_idx) & 1u) != 0u))
        entry = entry2;
    else
        entry = entry1;
    entry->start = start;
    entry->end = end;
    entry->slot = slot;
    entry->valid = 1;
}

static unsigned zragf_rfc_estimate_prepared_query_hash(zragf_size_t start,
                                                       zragf_size_t end)
{
    unsigned h = (unsigned)(start ^ (start >> 5));
    h ^= (unsigned)(end ^ (end >> 9));
    h ^= 0x27d4eb2du;
    return h % ZRAGF_RFC_ESTIMATE_PREPARED_QUERY_CACHE_CAP;
}

static unsigned zragf_rfc_estimate_prepared_query_hash_alt(zragf_size_t start,
                                                           zragf_size_t end)
{
    unsigned h = (unsigned)(start ^ (start >> 11));
    h ^= (unsigned)(end ^ (end >> 15));
    h ^= 0x165667b1u;
    return h % ZRAGF_RFC_ESTIMATE_PREPARED_QUERY_CACHE_CAP;
}

static int zragf_rfc_estimate_prepared_query_matches(const zragf_rfc_estimate_prepared_query_cache_entry *entry,
                                                     zragf_rfc_estimate_cache *cache,
                                                     zragf_size_t start,
                                                     zragf_size_t end)
{
    return entry && entry->valid &&
           entry->generation == cache->estimate_prepared_generation &&
           entry->start == start && entry->end == end;
}

static zragf_rfc_estimate_prepared_query_cache_entry *zragf_rfc_estimate_prepared_query_find(zragf_rfc_estimate_cache *cache,
                                                                                              zragf_size_t start,
                                                                                              zragf_size_t end)
{
    zragf_rfc_estimate_prepared_query_cache_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return NULL;
    idx = zragf_rfc_estimate_prepared_query_hash(start, end);
    entry = &cache->estimate_prepared_query_entries[idx];
    if (zragf_rfc_estimate_prepared_query_matches(entry, cache, start, end))
        return entry;
    alt_idx = zragf_rfc_estimate_prepared_query_hash_alt(start, end);
    if (alt_idx == idx)
        return NULL;
    entry = &cache->estimate_prepared_query_entries[alt_idx];
    if (zragf_rfc_estimate_prepared_query_matches(entry, cache, start, end))
        return entry;
    return NULL;
}

static void zragf_rfc_estimate_prepared_query_store(zragf_rfc_estimate_cache *cache,
                                                    zragf_size_t start,
                                                    zragf_size_t end,
                                                    unsigned slot)
{
    zragf_rfc_estimate_prepared_query_cache_entry *entry1;
    zragf_rfc_estimate_prepared_query_cache_entry *entry2 = NULL;
    zragf_rfc_estimate_prepared_query_cache_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return;
    idx = zragf_rfc_estimate_prepared_query_hash(start, end);
    entry1 = &cache->estimate_prepared_query_entries[idx];
    alt_idx = zragf_rfc_estimate_prepared_query_hash_alt(start, end);
    if (alt_idx != idx)
        entry2 = &cache->estimate_prepared_query_entries[alt_idx];
    if (zragf_rfc_estimate_prepared_query_matches(entry1, cache, start, end))
        entry = entry1;
    else if (entry2 && zragf_rfc_estimate_prepared_query_matches(entry2, cache, start, end))
        entry = entry2;
    else if (!entry1->valid || entry1->generation != cache->estimate_prepared_generation)
        entry = entry1;
    else if (entry2 && (!entry2->valid || entry2->generation != cache->estimate_prepared_generation))
        entry = entry2;
    else if (entry2 && (((slot + idx + alt_idx) & 1u) != 0u))
        entry = entry2;
    else
        entry = entry1;
    entry->start = start;
    entry->end = end;
    entry->slot = slot;
    entry->generation = cache->estimate_prepared_generation;
    entry->valid = 1;
}

static unsigned zragf_rfc_estimate_prepared_miss_hint_hash(zragf_size_t start,
                                                           zragf_size_t end)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)(end ^ (end >> 9));
    h ^= 0x85ebca6bu;
    return h % ZRAGF_RFC_ESTIMATE_PREPARED_MISS_HINT_CAP;
}

static unsigned zragf_rfc_estimate_prepared_miss_hint_hash_alt(zragf_size_t start,
                                                               zragf_size_t end)
{
    unsigned h = (unsigned)(start ^ (start >> 11));
    h ^= (unsigned)(end ^ (end >> 13));
    h ^= 0xc2b2ae35u;
    return h % ZRAGF_RFC_ESTIMATE_PREPARED_MISS_HINT_CAP;
}

static int zragf_rfc_estimate_prepared_miss_hint_matches(const zragf_rfc_estimate_prepared_miss_hint_entry *entry,
                                                         zragf_rfc_estimate_cache *cache,
                                                         zragf_size_t start,
                                                         zragf_size_t end)
{
    return entry && entry->valid &&
           entry->generation == cache->estimate_prepared_generation &&
           entry->start == start && entry->end == end;
}

static int zragf_rfc_estimate_prepared_miss_hint_hit(zragf_rfc_estimate_cache *cache,
                                                     zragf_size_t start,
                                                     zragf_size_t end)
{
    zragf_rfc_estimate_prepared_miss_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return 0;
    idx = zragf_rfc_estimate_prepared_miss_hint_hash(start, end);
    entry = &cache->estimate_prepared_miss_hints[idx];
    if (zragf_rfc_estimate_prepared_miss_hint_matches(entry, cache, start, end))
        return 1;
    alt_idx = zragf_rfc_estimate_prepared_miss_hint_hash_alt(start, end);
    if (alt_idx == idx)
        return 0;
    entry = &cache->estimate_prepared_miss_hints[alt_idx];
    return zragf_rfc_estimate_prepared_miss_hint_matches(entry, cache, start, end);
}

static void zragf_rfc_estimate_prepared_miss_hint_store(zragf_rfc_estimate_cache *cache,
                                                        zragf_size_t start,
                                                        zragf_size_t end)
{
    zragf_rfc_estimate_prepared_miss_hint_entry *entry1;
    zragf_rfc_estimate_prepared_miss_hint_entry *entry2 = NULL;
    zragf_rfc_estimate_prepared_miss_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return;
    idx = zragf_rfc_estimate_prepared_miss_hint_hash(start, end);
    entry1 = &cache->estimate_prepared_miss_hints[idx];
    alt_idx = zragf_rfc_estimate_prepared_miss_hint_hash_alt(start, end);
    if (alt_idx != idx)
        entry2 = &cache->estimate_prepared_miss_hints[alt_idx];
    if (zragf_rfc_estimate_prepared_miss_hint_matches(entry1, cache, start, end))
        entry = entry1;
    else if (entry2 && zragf_rfc_estimate_prepared_miss_hint_matches(entry2, cache, start, end))
        entry = entry2;
    else if (!entry1->valid || entry1->generation != cache->estimate_prepared_generation)
        entry = entry1;
    else if (entry2 && (!entry2->valid || entry2->generation != cache->estimate_prepared_generation))
        entry = entry2;
    else if (entry2 && (((unsigned)(start ^ end) & 1u) != 0u))
        entry = entry2;
    else
        entry = entry1;
    entry->start = start;
    entry->end = end;
    entry->generation = cache->estimate_prepared_generation;
    entry->valid = 1;
}

static zragf_rfc_estimate_cache_entry *zragf_rfc_estimate_cache_find(zragf_rfc_estimate_cache *cache,
                                                                     zragf_size_t start,
                                                                     zragf_size_t end,
                                                                     int final_block,
                                                                     int start_bitcount,
                                                                     int flush_final_bits)
{
    unsigned i;
    zragf_rfc_estimate_slot_hint_entry *hint;
    if (!cache)
        return NULL;
    hint = zragf_rfc_estimate_slot_hint_find(cache,
                                             start,
                                             end,
                                             final_block,
                                             start_bitcount,
                                             flush_final_bits);
    if (hint && hint->slot < ZRAGF_RFC_ESTIMATE_CACHE_CAP) {
        zragf_rfc_estimate_cache_entry *entry = &cache->entries[hint->slot];
        if (entry->valid &&
            entry->start == start &&
            entry->end == end &&
            entry->final_block == final_block &&
            entry->start_bitcount == start_bitcount &&
            entry->flush_final_bits == flush_final_bits)
            return entry;
    }
    for (i = 0u; i < ZRAGF_RFC_ESTIMATE_CACHE_CAP; ++i) {
        zragf_rfc_estimate_cache_entry *entry = &cache->entries[i];
        if (!entry->valid)
            continue;
        if (entry->start == start &&
            entry->end == end &&
            entry->final_block == final_block &&
            entry->start_bitcount == start_bitcount &&
            entry->flush_final_bits == flush_final_bits) {
            zragf_rfc_estimate_slot_hint_store(cache,
                                               start,
                                               end,
                                               final_block,
                                               start_bitcount,
                                               flush_final_bits,
                                               i);
            return entry;
        }
    }
    return NULL;
}

static zragf_rfc_estimate_cache_entry *zragf_rfc_estimate_cache_find_prepared(zragf_rfc_estimate_cache *cache,
                                                                               zragf_size_t start,
                                                                               zragf_size_t end)
{
    unsigned i;
    zragf_rfc_estimate_prepared_slot_hint_entry *hint;
    zragf_rfc_estimate_prepared_query_cache_entry *query;
    if (!cache)
        return NULL;
    if (end < start || (end - start) > (zragf_size_t)ZRAGF_RFC_PREPARED_SEGMENT_MAX)
        return NULL;
    query = zragf_rfc_estimate_prepared_query_find(cache, start, end);
    if (query && query->slot < ZRAGF_RFC_ESTIMATE_CACHE_CAP) {
        zragf_rfc_estimate_cache_entry *entry = &cache->entries[query->slot];
        if (entry->valid && entry->prepared.valid && entry->prepared.tb.data &&
            entry->start == start && entry->end == end)
            return entry;
    }
    hint = zragf_rfc_estimate_prepared_slot_hint_find(cache, start, end);
    if (hint && hint->slot < ZRAGF_RFC_ESTIMATE_CACHE_CAP) {
        zragf_rfc_estimate_cache_entry *entry = &cache->entries[hint->slot];
        if (entry->valid && entry->prepared.valid && entry->prepared.tb.data &&
            entry->start == start && entry->end == end) {
            zragf_rfc_estimate_prepared_query_store(cache, start, end, hint->slot);
            return entry;
        }
    }
    if (zragf_rfc_estimate_prepared_miss_hint_hit(cache, start, end))
        return NULL;
    for (i = 0u; i < ZRAGF_RFC_ESTIMATE_CACHE_CAP; ++i) {
        zragf_rfc_estimate_cache_entry *entry = &cache->entries[i];
        if (!entry->valid || !entry->prepared.valid || !entry->prepared.tb.data)
            continue;
        if (entry->start == start && entry->end == end) {
            zragf_rfc_estimate_prepared_slot_hint_store(cache, start, end, i);
            zragf_rfc_estimate_prepared_query_store(cache, start, end, i);
            return entry;
        }
    }
    zragf_rfc_estimate_prepared_miss_hint_store(cache, start, end);
    return NULL;
}

static void zragf_rfc_estimate_cache_store(zragf_rfc_estimate_cache *cache,
                                           zragf_size_t start,
                                           zragf_size_t end,
                                           int final_block,
                                           int start_bitcount,
                                           int flush_final_bits,
                                           zragf_size_t size,
                                           int end_bitcount,
                                           zragf_block_type best_type,
                                           zragf_rfc_prepared_segment *prepared)
{
    zragf_rfc_estimate_cache_entry *entry;
    unsigned slot;
    int old_prepared_valid;
    int new_prepared_valid;
    if (!cache)
        return;
    slot = cache->next_slot % ZRAGF_RFC_ESTIMATE_CACHE_CAP;
    entry = &cache->entries[slot];
    old_prepared_valid = entry->valid && entry->prepared.valid && entry->prepared.tb.data;
    new_prepared_valid = (prepared && prepared->valid && prepared->tb.data) ? 1 : 0;
    if (old_prepared_valid || new_prepared_valid)
        cache->estimate_prepared_generation++;
    zragf_rfc_prepared_segment_free(&entry->prepared);
    entry->start = start;
    entry->end = end;
    entry->final_block = final_block;
    entry->start_bitcount = start_bitcount;
    entry->flush_final_bits = flush_final_bits;
    entry->size = size;
    entry->end_bitcount = end_bitcount;
    entry->best_type = best_type;
    if (prepared)
        zragf_rfc_prepared_segment_move(&entry->prepared, prepared);
    entry->valid = 1;
    zragf_rfc_estimate_slot_hint_store(cache,
                                       start,
                                       end,
                                       final_block,
                                       start_bitcount,
                                       flush_final_bits,
                                       slot);
    if (entry->prepared.valid && entry->prepared.tb.data) {
        zragf_rfc_estimate_prepared_slot_hint_store(cache, start, end, slot);
        zragf_rfc_estimate_prepared_query_store(cache, start, end, slot);
    }
    cache->next_slot = (cache->next_slot + 1u) % ZRAGF_RFC_ESTIMATE_CACHE_CAP;
}

static void zragf_rfc_suffix_cache_store(zragf_rfc_estimate_cache *cache,
                                         zragf_size_t start,
                                         int next_index,
                                         int remaining_splits,
                                         int start_bitcount,
                                         int final_block,
                                         int flush_final_bits,
                                         const zragf_rfc_partition *part);

static void zragf_rfc_suffix_lb_cache_store(zragf_rfc_estimate_cache *cache,
                                            zragf_size_t start,
                                            int next_index,
                                            int remaining_splits,
                                            int start_bitcount,
                                            int final_block,
                                            int flush_final_bits,
                                            zragf_size_t total_size);

static unsigned zragf_rfc_suffix_query_hash(zragf_size_t start,
                                            int next_index,
                                            int remaining_splits,
                                            int start_bitcount,
                                            int final_block,
                                            int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)((unsigned)(next_index + 257) * 131u);
    h ^= (unsigned)((unsigned)(remaining_splits + 257) * 911u);
    h ^= (unsigned)((unsigned)start_bitcount * 977u);
    h ^= (unsigned)((unsigned)final_block * 8191u);
    h ^= (unsigned)((unsigned)flush_final_bits * 65537u);
    return h % ZRAGF_RFC_SUFFIX_QUERY_CACHE_CAP;
}

static unsigned zragf_rfc_suffix_query_hash_alt(zragf_size_t start,
                                                int next_index,
                                                int remaining_splits,
                                                int start_bitcount,
                                                int final_block,
                                                int flush_final_bits)
{
    unsigned h = (unsigned)((start >> 3) ^ (start >> 11));
    h ^= (unsigned)((unsigned)(next_index + 509) * 193u);
    h ^= (unsigned)((unsigned)(remaining_splits + 503) * 769u);
    h ^= (unsigned)((unsigned)start_bitcount * 3907u);
    h ^= (unsigned)((unsigned)final_block * 12289u);
    h ^= (unsigned)((unsigned)flush_final_bits * 131071u);
    return h % ZRAGF_RFC_SUFFIX_QUERY_CACHE_CAP;
}

static int zragf_rfc_suffix_query_entry_matches(const zragf_rfc_suffix_query_cache_entry *entry,
                                                zragf_size_t start,
                                                int next_index,
                                                int remaining_splits,
                                                int start_bitcount,
                                                int final_block,
                                                int flush_final_bits)
{
    return entry && entry->valid &&
           entry->start == start &&
           entry->next_index == next_index &&
           entry->remaining_splits == remaining_splits &&
           entry->start_bitcount == start_bitcount &&
           entry->final_block == final_block &&
           entry->flush_final_bits == flush_final_bits;
}

static zragf_rfc_suffix_query_cache_entry *zragf_rfc_suffix_query_cache_find(zragf_rfc_estimate_cache *cache,
                                                                              zragf_size_t start,
                                                                              int next_index,
                                                                              int remaining_splits,
                                                                              int start_bitcount,
                                                                              int final_block,
                                                                              int flush_final_bits)
{
    zragf_rfc_suffix_query_cache_entry *entry;
    unsigned idx;
    unsigned alt_idx;
    if (!cache)
        return NULL;
    idx = zragf_rfc_suffix_query_hash(start,
                                      next_index,
                                      remaining_splits,
                                      start_bitcount,
                                      final_block,
                                      flush_final_bits);
    entry = &cache->suffix_query_entries[idx];
    if (zragf_rfc_suffix_query_entry_matches(entry,
                                             start,
                                             next_index,
                                             remaining_splits,
                                             start_bitcount,
                                             final_block,
                                             flush_final_bits))
        return entry;
    alt_idx = zragf_rfc_suffix_query_hash_alt(start,
                                              next_index,
                                              remaining_splits,
                                              start_bitcount,
                                              final_block,
                                              flush_final_bits);
    if (alt_idx == idx)
        return NULL;
    entry = &cache->suffix_query_entries[alt_idx];
    if (zragf_rfc_suffix_query_entry_matches(entry,
                                             start,
                                             next_index,
                                             remaining_splits,
                                             start_bitcount,
                                             final_block,
                                             flush_final_bits))
        return entry;
    return NULL;
}

static void zragf_rfc_suffix_query_cache_store(zragf_rfc_estimate_cache *cache,
                                               zragf_size_t start,
                                               int next_index,
                                               int remaining_splits,
                                               int start_bitcount,
                                               int final_block,
                                               int flush_final_bits,
                                               const zragf_rfc_partition *part)
{
    zragf_rfc_suffix_query_cache_entry *entry1;
    zragf_rfc_suffix_query_cache_entry *entry2 = NULL;
    unsigned idx;
    unsigned alt_idx;
    if (!cache || !part)
        return;
    idx = zragf_rfc_suffix_query_hash(start,
                                      next_index,
                                      remaining_splits,
                                      start_bitcount,
                                      final_block,
                                      flush_final_bits);
    entry1 = &cache->suffix_query_entries[idx];
    alt_idx = zragf_rfc_suffix_query_hash_alt(start,
                                              next_index,
                                              remaining_splits,
                                              start_bitcount,
                                              final_block,
                                              flush_final_bits);
    if (alt_idx != idx)
        entry2 = &cache->suffix_query_entries[alt_idx];
    if (zragf_rfc_suffix_query_entry_matches(entry1,
                                             start,
                                             next_index,
                                             remaining_splits,
                                             start_bitcount,
                                             final_block,
                                             flush_final_bits)) {
        entry2 = entry1;
    } else if (entry2 && zragf_rfc_suffix_query_entry_matches(entry2,
                                                              start,
                                                              next_index,
                                                              remaining_splits,
                                                              start_bitcount,
                                                              final_block,
                                                              flush_final_bits)) {
        /* keep entry2 */
    } else if (!entry1->valid) {
        entry2 = entry1;
    } else if (entry2 && !entry2->valid) {
        /* keep entry2 */
    } else {
        entry2 = entry1;
    }
    entry2->start = start;
    entry2->next_index = next_index;
    entry2->remaining_splits = remaining_splits;
    entry2->start_bitcount = start_bitcount;
    entry2->final_block = final_block;
    entry2->flush_final_bits = flush_final_bits;
    entry2->part = *part;
    entry2->valid = 1;
}


static unsigned zragf_rfc_suffix_lb_query_hash(zragf_size_t start,
                                                int next_index,
                                                int remaining_splits,
                                                int start_bitcount,
                                                int final_block,
                                                int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)((unsigned)(next_index + 257) * 131u);
    h ^= (unsigned)((unsigned)(remaining_splits + 257) * 911u);
    h ^= (unsigned)((unsigned)start_bitcount * 977u);
    h ^= (unsigned)((unsigned)final_block * 8191u);
    h ^= (unsigned)((unsigned)flush_final_bits * 65537u);
    return h % ZRAGF_RFC_SUFFIX_LB_QUERY_CACHE_CAP;
}

static unsigned zragf_rfc_suffix_lb_query_hash_alt(zragf_size_t start,
                                                   int next_index,
                                                   int remaining_splits,
                                                   int start_bitcount,
                                                   int final_block,
                                                   int flush_final_bits)
{
    unsigned h = (unsigned)((start >> 5) ^ (start >> 13));
    h ^= (unsigned)((unsigned)(next_index + 509) * 193u);
    h ^= (unsigned)((unsigned)(remaining_splits + 503) * 769u);
    h ^= (unsigned)((unsigned)start_bitcount * 3907u);
    h ^= (unsigned)((unsigned)final_block * 12289u);
    h ^= (unsigned)((unsigned)flush_final_bits * 131071u);
    return h % ZRAGF_RFC_SUFFIX_LB_QUERY_CACHE_CAP;
}

static int zragf_rfc_suffix_lb_query_entry_matches(const zragf_rfc_suffix_lb_query_cache_entry *entry,
                                                   zragf_size_t start,
                                                   int next_index,
                                                   int remaining_splits,
                                                   int start_bitcount,
                                                   int final_block,
                                                   int flush_final_bits)
{
    return entry && entry->valid &&
           entry->start == start &&
           entry->next_index == next_index &&
           entry->remaining_splits == remaining_splits &&
           entry->start_bitcount == start_bitcount &&
           entry->final_block == final_block &&
           entry->flush_final_bits == flush_final_bits;
}

static zragf_rfc_suffix_lb_query_cache_entry *zragf_rfc_suffix_lb_query_cache_find(zragf_rfc_estimate_cache *cache,
                                                                                    zragf_size_t start,
                                                                                    int next_index,
                                                                                    int remaining_splits,
                                                                                    int start_bitcount,
                                                                                    int final_block,
                                                                                    int flush_final_bits)
{
    zragf_rfc_suffix_lb_query_cache_entry *entry;
    unsigned idx;
    unsigned alt_idx;
    if (!cache)
        return NULL;
    idx = zragf_rfc_suffix_lb_query_hash(start,
                                         next_index,
                                         remaining_splits,
                                         start_bitcount,
                                         final_block,
                                         flush_final_bits);
    entry = &cache->suffix_lb_query_entries[idx];
    if (zragf_rfc_suffix_lb_query_entry_matches(entry,
                                                start,
                                                next_index,
                                                remaining_splits,
                                                start_bitcount,
                                                final_block,
                                                flush_final_bits))
        return entry;
    alt_idx = zragf_rfc_suffix_lb_query_hash_alt(start,
                                                 next_index,
                                                 remaining_splits,
                                                 start_bitcount,
                                                 final_block,
                                                 flush_final_bits);
    if (alt_idx == idx)
        return NULL;
    entry = &cache->suffix_lb_query_entries[alt_idx];
    if (zragf_rfc_suffix_lb_query_entry_matches(entry,
                                                start,
                                                next_index,
                                                remaining_splits,
                                                start_bitcount,
                                                final_block,
                                                flush_final_bits))
        return entry;
    return NULL;
}

static void zragf_rfc_suffix_lb_query_cache_store(zragf_rfc_estimate_cache *cache,
                                                  zragf_size_t start,
                                                  int next_index,
                                                  int remaining_splits,
                                                  int start_bitcount,
                                                  int final_block,
                                                  int flush_final_bits,
                                                  zragf_size_t total_size)
{
    zragf_rfc_suffix_lb_query_cache_entry *entry1;
    zragf_rfc_suffix_lb_query_cache_entry *entry2 = NULL;
    unsigned idx;
    unsigned alt_idx;
    if (!cache)
        return;
    idx = zragf_rfc_suffix_lb_query_hash(start,
                                         next_index,
                                         remaining_splits,
                                         start_bitcount,
                                         final_block,
                                         flush_final_bits);
    entry1 = &cache->suffix_lb_query_entries[idx];
    alt_idx = zragf_rfc_suffix_lb_query_hash_alt(start,
                                                 next_index,
                                                 remaining_splits,
                                                 start_bitcount,
                                                 final_block,
                                                 flush_final_bits);
    if (alt_idx != idx)
        entry2 = &cache->suffix_lb_query_entries[alt_idx];
    if (zragf_rfc_suffix_lb_query_entry_matches(entry1,
                                                start,
                                                next_index,
                                                remaining_splits,
                                                start_bitcount,
                                                final_block,
                                                flush_final_bits)) {
        entry2 = entry1;
    } else if (entry2 && zragf_rfc_suffix_lb_query_entry_matches(entry2,
                                                                 start,
                                                                 next_index,
                                                                 remaining_splits,
                                                                 start_bitcount,
                                                                 final_block,
                                                                 flush_final_bits)) {
        /* keep entry2 */
    } else if (!entry1->valid) {
        entry2 = entry1;
    } else if (entry2 && !entry2->valid) {
        /* keep entry2 */
    } else {
        entry2 = entry1;
    }
    entry2->start = start;
    entry2->next_index = next_index;
    entry2->remaining_splits = remaining_splits;
    entry2->start_bitcount = start_bitcount;
    entry2->final_block = final_block;
    entry2->flush_final_bits = flush_final_bits;
    entry2->total_size = total_size;
    entry2->valid = 1;
}

static unsigned zragf_rfc_suffix_exact_query_hash(zragf_size_t start,
                                                   int start_bitcount,
                                                   int final_block,
                                                   int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)((unsigned)start_bitcount * 131u);
    h ^= (unsigned)((unsigned)final_block * 977u);
    h ^= (unsigned)((unsigned)flush_final_bits * 8191u);
    return h % ZRAGF_RFC_SUFFIX_EXACT_QUERY_CACHE_CAP;
}

static unsigned zragf_rfc_suffix_exact_query_hash_alt(zragf_size_t start,
                                                      int start_bitcount,
                                                      int final_block,
                                                      int flush_final_bits)
{
    unsigned h = (unsigned)((start >> 5) ^ (start >> 13));
    h ^= (unsigned)((unsigned)start_bitcount * 193u);
    h ^= (unsigned)((unsigned)final_block * 3907u);
    h ^= (unsigned)((unsigned)flush_final_bits * 12289u);
    return h % ZRAGF_RFC_SUFFIX_EXACT_QUERY_CACHE_CAP;
}

static int zragf_rfc_suffix_exact_query_entry_matches(const zragf_rfc_suffix_exact_query_cache_entry *entry,
                                                      zragf_size_t start,
                                                      int start_bitcount,
                                                      int final_block,
                                                      int flush_final_bits)
{
    return entry && entry->valid &&
           entry->start == start &&
           entry->start_bitcount == start_bitcount &&
           entry->final_block == final_block &&
           entry->flush_final_bits == flush_final_bits;
}

static zragf_rfc_suffix_exact_query_cache_entry *zragf_rfc_suffix_exact_query_cache_find(zragf_rfc_estimate_cache *cache,
                                                                                          zragf_size_t start,
                                                                                          int start_bitcount,
                                                                                          int final_block,
                                                                                          int flush_final_bits)
{
    zragf_rfc_suffix_exact_query_cache_entry *entry;
    unsigned idx;
    unsigned alt_idx;
    if (!cache)
        return NULL;
    idx = zragf_rfc_suffix_exact_query_hash(start,
                                            start_bitcount,
                                            final_block,
                                            flush_final_bits);
    entry = &cache->suffix_exact_query_entries[idx];
    if (zragf_rfc_suffix_exact_query_entry_matches(entry,
                                                   start,
                                                   start_bitcount,
                                                   final_block,
                                                   flush_final_bits))
        return entry;
    alt_idx = zragf_rfc_suffix_exact_query_hash_alt(start,
                                                    start_bitcount,
                                                    final_block,
                                                    flush_final_bits);
    if (alt_idx == idx)
        return NULL;
    entry = &cache->suffix_exact_query_entries[alt_idx];
    if (zragf_rfc_suffix_exact_query_entry_matches(entry,
                                                   start,
                                                   start_bitcount,
                                                   final_block,
                                                   flush_final_bits))
        return entry;
    return NULL;
}

static void zragf_rfc_suffix_exact_query_cache_store(zragf_rfc_estimate_cache *cache,
                                                     zragf_size_t start,
                                                     int start_bitcount,
                                                     int final_block,
                                                     int flush_final_bits,
                                                     zragf_size_t total_size,
                                                     int end_bitcount)
{
    zragf_rfc_suffix_exact_query_cache_entry *entry1;
    zragf_rfc_suffix_exact_query_cache_entry *entry2 = NULL;
    unsigned idx;
    unsigned alt_idx;
    if (!cache)
        return;
    idx = zragf_rfc_suffix_exact_query_hash(start,
                                            start_bitcount,
                                            final_block,
                                            flush_final_bits);
    entry1 = &cache->suffix_exact_query_entries[idx];
    alt_idx = zragf_rfc_suffix_exact_query_hash_alt(start,
                                                    start_bitcount,
                                                    final_block,
                                                    flush_final_bits);
    if (alt_idx != idx)
        entry2 = &cache->suffix_exact_query_entries[alt_idx];
    if (zragf_rfc_suffix_exact_query_entry_matches(entry1,
                                                   start,
                                                   start_bitcount,
                                                   final_block,
                                                   flush_final_bits)) {
        entry2 = entry1;
    } else if (entry2 && zragf_rfc_suffix_exact_query_entry_matches(entry2,
                                                                    start,
                                                                    start_bitcount,
                                                                    final_block,
                                                                    flush_final_bits)) {
        /* keep entry2 */
    } else if (!entry1->valid) {
        entry2 = entry1;
    } else if (entry2 && !entry2->valid) {
        /* keep entry2 */
    } else {
        entry2 = entry1;
    }
    entry2->start = start;
    entry2->start_bitcount = start_bitcount;
    entry2->final_block = final_block;
    entry2->flush_final_bits = flush_final_bits;
    entry2->total_size = total_size;
    entry2->end_bitcount = end_bitcount;
    entry2->valid = 1;
}

static unsigned zragf_rfc_suffix_miss_hint_hash(zragf_size_t start,
                                                int next_index,
                                                int remaining_splits,
                                                int start_bitcount,
                                                int final_block,
                                                int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)((unsigned)(next_index + 257) * 131u);
    h ^= (unsigned)((unsigned)(remaining_splits + 257) * 911u);
    h ^= (unsigned)((unsigned)start_bitcount * 977u);
    h ^= (unsigned)((unsigned)final_block * 8191u);
    h ^= (unsigned)((unsigned)flush_final_bits * 65537u);
    return h % ZRAGF_RFC_SUFFIX_MISS_HINT_CAP;
}

static int zragf_rfc_suffix_miss_hint_hit(zragf_rfc_estimate_cache *cache,
                                          zragf_size_t start,
                                          int next_index,
                                          int remaining_splits,
                                          int start_bitcount,
                                          int final_block,
                                          int flush_final_bits)
{
    zragf_rfc_suffix_miss_hint_entry *entry;
    unsigned idx;
    if (!cache)
        return 0;
    idx = zragf_rfc_suffix_miss_hint_hash(start,
                                          next_index,
                                          remaining_splits,
                                          start_bitcount,
                                          final_block,
                                          flush_final_bits);
    entry = &cache->suffix_miss_hints[idx];
    if (!entry->valid)
        return 0;
    if (entry->generation != cache->suffix_generation)
        return 0;
    return entry->start == start &&
           entry->next_index == next_index &&
           entry->remaining_splits == remaining_splits &&
           entry->start_bitcount == start_bitcount &&
           entry->final_block == final_block &&
           entry->flush_final_bits == flush_final_bits;
}

static void zragf_rfc_suffix_miss_hint_store(zragf_rfc_estimate_cache *cache,
                                             zragf_size_t start,
                                             int next_index,
                                             int remaining_splits,
                                             int start_bitcount,
                                             int final_block,
                                             int flush_final_bits)
{
    zragf_rfc_suffix_miss_hint_entry *entry;
    unsigned idx;
    if (!cache)
        return;
    idx = zragf_rfc_suffix_miss_hint_hash(start,
                                          next_index,
                                          remaining_splits,
                                          start_bitcount,
                                          final_block,
                                          flush_final_bits);
    entry = &cache->suffix_miss_hints[idx];
    entry->start = start;
    entry->next_index = next_index;
    entry->remaining_splits = remaining_splits;
    entry->start_bitcount = start_bitcount;
    entry->final_block = final_block;
    entry->flush_final_bits = flush_final_bits;
    entry->generation = cache->suffix_generation;
    entry->valid = 1;
}

static unsigned zragf_rfc_suffix_lb_miss_hint_hash(zragf_size_t start,
                                                   int next_index,
                                                   int remaining_splits,
                                                   int start_bitcount,
                                                   int final_block,
                                                   int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)((unsigned)(next_index + 257) * 131u);
    h ^= (unsigned)((unsigned)(remaining_splits + 257) * 911u);
    h ^= (unsigned)((unsigned)start_bitcount * 977u);
    h ^= (unsigned)((unsigned)final_block * 8191u);
    h ^= (unsigned)((unsigned)flush_final_bits * 65537u);
    return h % ZRAGF_RFC_SUFFIX_LB_MISS_HINT_CAP;
}

static int zragf_rfc_suffix_lb_miss_hint_hit(zragf_rfc_estimate_cache *cache,
                                             zragf_size_t start,
                                             int next_index,
                                             int remaining_splits,
                                             int start_bitcount,
                                             int final_block,
                                             int flush_final_bits)
{
    zragf_rfc_suffix_lb_miss_hint_entry *entry;
    unsigned idx;
    if (!cache)
        return 0;
    idx = zragf_rfc_suffix_lb_miss_hint_hash(start,
                                             next_index,
                                             remaining_splits,
                                             start_bitcount,
                                             final_block,
                                             flush_final_bits);
    entry = &cache->suffix_lb_miss_hints[idx];
    if (!entry->valid)
        return 0;
    if (entry->generation != cache->suffix_lb_generation)
        return 0;
    return entry->start == start &&
           entry->next_index == next_index &&
           entry->remaining_splits == remaining_splits &&
           entry->start_bitcount == start_bitcount &&
           entry->final_block == final_block &&
           entry->flush_final_bits == flush_final_bits;
}

static void zragf_rfc_suffix_lb_miss_hint_store(zragf_rfc_estimate_cache *cache,
                                                zragf_size_t start,
                                                int next_index,
                                                int remaining_splits,
                                                int start_bitcount,
                                                int final_block,
                                                int flush_final_bits)
{
    zragf_rfc_suffix_lb_miss_hint_entry *entry;
    unsigned idx;
    if (!cache)
        return;
    idx = zragf_rfc_suffix_lb_miss_hint_hash(start,
                                             next_index,
                                             remaining_splits,
                                             start_bitcount,
                                             final_block,
                                             flush_final_bits);
    entry = &cache->suffix_lb_miss_hints[idx];
    entry->start = start;
    entry->next_index = next_index;
    entry->remaining_splits = remaining_splits;
    entry->start_bitcount = start_bitcount;
    entry->final_block = final_block;
    entry->flush_final_bits = flush_final_bits;
    entry->generation = cache->suffix_lb_generation;
    entry->valid = 1;
}

static unsigned zragf_rfc_suffix_exact_miss_hint_hash(zragf_size_t start,
                                                      int start_bitcount,
                                                      int final_block,
                                                      int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)((unsigned)start_bitcount * 131u);
    h ^= (unsigned)((unsigned)final_block * 977u);
    h ^= (unsigned)((unsigned)flush_final_bits * 8191u);
    return h % ZRAGF_RFC_SUFFIX_EXACT_MISS_HINT_CAP;
}

static int zragf_rfc_suffix_exact_miss_hint_hit(zragf_rfc_estimate_cache *cache,
                                                zragf_size_t start,
                                                int start_bitcount,
                                                int final_block,
                                                int flush_final_bits)
{
    zragf_rfc_suffix_exact_miss_hint_entry *entry;
    unsigned idx;
    if (!cache)
        return 0;
    idx = zragf_rfc_suffix_exact_miss_hint_hash(start,
                                                start_bitcount,
                                                final_block,
                                                flush_final_bits);
    entry = &cache->suffix_exact_miss_hints[idx];
    if (!entry->valid)
        return 0;
    if (entry->generation != cache->suffix_exact_generation)
        return 0;
    return entry->start == start &&
           entry->start_bitcount == start_bitcount &&
           entry->final_block == final_block &&
           entry->flush_final_bits == flush_final_bits;
}

static void zragf_rfc_suffix_exact_miss_hint_store(zragf_rfc_estimate_cache *cache,
                                                   zragf_size_t start,
                                                   int start_bitcount,
                                                   int final_block,
                                                   int flush_final_bits)
{
    zragf_rfc_suffix_exact_miss_hint_entry *entry;
    unsigned idx;
    if (!cache)
        return;
    idx = zragf_rfc_suffix_exact_miss_hint_hash(start,
                                                start_bitcount,
                                                final_block,
                                                flush_final_bits);
    entry = &cache->suffix_exact_miss_hints[idx];
    entry->start = start;
    entry->start_bitcount = start_bitcount;
    entry->final_block = final_block;
    entry->flush_final_bits = flush_final_bits;
    entry->generation = cache->suffix_exact_generation;
    entry->valid = 1;
}

static unsigned zragf_rfc_suffix_exact_slot_hint_hash(zragf_size_t start,
                                                         int start_bitcount,
                                                         int final_block,
                                                         int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)((unsigned)start_bitcount * 131u);
    h ^= (unsigned)((unsigned)final_block * 977u);
    h ^= (unsigned)((unsigned)flush_final_bits * 8191u);
    return h % ZRAGF_RFC_SUFFIX_EXACT_SLOT_HINT_CAP;
}

static unsigned zragf_rfc_suffix_exact_slot_hint_hash_alt(zragf_size_t start,
                                                             int start_bitcount,
                                                             int final_block,
                                                             int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 11));
    h ^= (unsigned)((unsigned)start_bitcount * 257u);
    h ^= (unsigned)((unsigned)final_block * 65537u);
    h ^= (unsigned)((unsigned)flush_final_bits * 131071u);
    h ^= 0x85ebca6bu;
    return h % ZRAGF_RFC_SUFFIX_EXACT_SLOT_HINT_CAP;
}

static int zragf_rfc_suffix_exact_slot_hint_matches(const zragf_rfc_suffix_exact_slot_hint_entry *entry,
                                                    zragf_size_t start,
                                                    int start_bitcount,
                                                    int final_block,
                                                    int flush_final_bits)
{
    return entry && entry->valid &&
           entry->start == start &&
           entry->start_bitcount == start_bitcount &&
           entry->final_block == final_block &&
           entry->flush_final_bits == flush_final_bits;
}

static zragf_rfc_suffix_exact_slot_hint_entry *zragf_rfc_suffix_exact_slot_hint_find(zragf_rfc_estimate_cache *cache,
                                                                                      zragf_size_t start,
                                                                                      int start_bitcount,
                                                                                      int final_block,
                                                                                      int flush_final_bits)
{
    zragf_rfc_suffix_exact_slot_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return NULL;
    idx = zragf_rfc_suffix_exact_slot_hint_hash(start,
                                                start_bitcount,
                                                final_block,
                                                flush_final_bits);
    entry = &cache->suffix_exact_slot_hints[idx];
    if (zragf_rfc_suffix_exact_slot_hint_matches(entry,
                                                 start,
                                                 start_bitcount,
                                                 final_block,
                                                 flush_final_bits))
        return entry;
    alt_idx = zragf_rfc_suffix_exact_slot_hint_hash_alt(start,
                                                        start_bitcount,
                                                        final_block,
                                                        flush_final_bits);
    if (alt_idx == idx)
        return NULL;
    entry = &cache->suffix_exact_slot_hints[alt_idx];
    if (zragf_rfc_suffix_exact_slot_hint_matches(entry,
                                                 start,
                                                 start_bitcount,
                                                 final_block,
                                                 flush_final_bits))
        return entry;
    return NULL;
}

static void zragf_rfc_suffix_exact_slot_hint_store(zragf_rfc_estimate_cache *cache,
                                                   zragf_size_t start,
                                                   int start_bitcount,
                                                   int final_block,
                                                   int flush_final_bits,
                                                   unsigned slot)
{
    zragf_rfc_suffix_exact_slot_hint_entry *entry1;
    zragf_rfc_suffix_exact_slot_hint_entry *entry2 = NULL;
    zragf_rfc_suffix_exact_slot_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return;
    idx = zragf_rfc_suffix_exact_slot_hint_hash(start,
                                                start_bitcount,
                                                final_block,
                                                flush_final_bits);
    entry1 = &cache->suffix_exact_slot_hints[idx];
    alt_idx = zragf_rfc_suffix_exact_slot_hint_hash_alt(start,
                                                        start_bitcount,
                                                        final_block,
                                                        flush_final_bits);
    if (alt_idx != idx)
        entry2 = &cache->suffix_exact_slot_hints[alt_idx];
    if (zragf_rfc_suffix_exact_slot_hint_matches(entry1,
                                                 start,
                                                 start_bitcount,
                                                 final_block,
                                                 flush_final_bits))
        entry = entry1;
    else if (entry2 && zragf_rfc_suffix_exact_slot_hint_matches(entry2,
                                                                start,
                                                                start_bitcount,
                                                                final_block,
                                                                flush_final_bits))
        entry = entry2;
    else if (!entry1->valid)
        entry = entry1;
    else if (entry2 && !entry2->valid)
        entry = entry2;
    else if (entry2 && (((slot + idx + alt_idx) & 1u) != 0u))
        entry = entry2;
    else
        entry = entry1;
    entry->start = start;
    entry->start_bitcount = start_bitcount;
    entry->final_block = final_block;
    entry->flush_final_bits = flush_final_bits;
    entry->slot = slot;
    entry->valid = 1;
}

static zragf_rfc_suffix_exact_cache_entry *zragf_rfc_suffix_exact_cache_find(zragf_rfc_estimate_cache *cache,
                                                                              zragf_size_t start,
                                                                              int start_bitcount,
                                                                              int final_block,
                                                                              int flush_final_bits)
{
    unsigned i;
    zragf_rfc_suffix_exact_slot_hint_entry *hint;
    if (!cache)
        return NULL;
    hint = zragf_rfc_suffix_exact_slot_hint_find(cache,
                                                 start,
                                                 start_bitcount,
                                                 final_block,
                                                 flush_final_bits);
    if (hint && hint->slot < ZRAGF_RFC_SUFFIX_EXACT_CACHE_CAP) {
        zragf_rfc_suffix_exact_cache_entry *entry = &cache->suffix_exact_entries[hint->slot];
        if (entry->valid &&
            entry->start == start &&
            entry->start_bitcount == start_bitcount &&
            entry->final_block == final_block &&
            entry->flush_final_bits == flush_final_bits)
            return entry;
    }
    if (zragf_rfc_suffix_exact_miss_hint_hit(cache,
                                             start,
                                             start_bitcount,
                                             final_block,
                                             flush_final_bits))
        return NULL;
    for (i = 0u; i < ZRAGF_RFC_SUFFIX_EXACT_CACHE_CAP; ++i) {
        zragf_rfc_suffix_exact_cache_entry *entry = &cache->suffix_exact_entries[i];
        if (!entry->valid)
            continue;
        if (entry->start == start &&
            entry->start_bitcount == start_bitcount &&
            entry->final_block == final_block &&
            entry->flush_final_bits == flush_final_bits) {
            zragf_rfc_suffix_exact_slot_hint_store(cache,
                                                   start,
                                                   start_bitcount,
                                                   final_block,
                                                   flush_final_bits,
                                                   i);
            return entry;
        }
    }
    zragf_rfc_suffix_exact_miss_hint_store(cache,
                                           start,
                                           start_bitcount,
                                           final_block,
                                           flush_final_bits);
    return NULL;
}

static void zragf_rfc_suffix_exact_cache_store(zragf_rfc_estimate_cache *cache,
                                               zragf_size_t start,
                                               int start_bitcount,
                                               int final_block,
                                               int flush_final_bits,
                                               zragf_size_t total_size,
                                               int end_bitcount)
{
    zragf_rfc_suffix_exact_cache_entry *entry;
    if (!cache)
        return;
    entry = zragf_rfc_suffix_exact_cache_find(cache,
                                              start,
                                              start_bitcount,
                                              final_block,
                                              flush_final_bits);
    if (!entry) {
        entry = &cache->suffix_exact_entries[cache->suffix_exact_next_slot % ZRAGF_RFC_SUFFIX_EXACT_CACHE_CAP];
        cache->suffix_exact_next_slot = (cache->suffix_exact_next_slot + 1u) % ZRAGF_RFC_SUFFIX_EXACT_CACHE_CAP;
    }
    cache->suffix_exact_generation++;
    entry->start = start;
    entry->start_bitcount = start_bitcount;
    entry->final_block = final_block;
    entry->flush_final_bits = flush_final_bits;
    entry->total_size = total_size;
    entry->end_bitcount = end_bitcount;
    entry->valid = 1;
    zragf_rfc_suffix_exact_slot_hint_store(cache,
                                           start,
                                           start_bitcount,
                                           final_block,
                                           flush_final_bits,
                                           (unsigned)(entry - cache->suffix_exact_entries));
    zragf_rfc_suffix_exact_query_cache_store(cache,
                                             start,
                                             start_bitcount,
                                             final_block,
                                             flush_final_bits,
                                             total_size,
                                             end_bitcount);
}

static int zragf_rfc_suffix_exact_cache_lookup(zragf_rfc_estimate_cache *cache,
                                               zragf_size_t start,
                                               int start_bitcount,
                                               int final_block,
                                               int flush_final_bits,
                                               zragf_size_t *out_total_size,
                                               int *out_end_bitcount)
{
    zragf_rfc_suffix_exact_cache_entry *entry;
    zragf_rfc_suffix_exact_query_cache_entry *query_entry;
    if (!cache || !out_total_size)
        return 0;
    query_entry = zragf_rfc_suffix_exact_query_cache_find(cache,
                                                          start,
                                                          start_bitcount,
                                                          final_block,
                                                          flush_final_bits);
    if (query_entry) {
        *out_total_size = query_entry->total_size;
        if (out_end_bitcount)
            *out_end_bitcount = query_entry->end_bitcount;
        cache->suffix_exact_query_hits++;
        return 1;
    }
    entry = zragf_rfc_suffix_exact_cache_find(cache,
                                              start,
                                              start_bitcount,
                                              final_block,
                                              flush_final_bits);
    if (!entry)
        return 0;
    *out_total_size = entry->total_size;
    if (out_end_bitcount)
        *out_end_bitcount = entry->end_bitcount;
    zragf_rfc_suffix_exact_query_cache_store(cache,
                                             start,
                                             start_bitcount,
                                             final_block,
                                             flush_final_bits,
                                             entry->total_size,
                                             entry->end_bitcount);
    cache->suffix_exact_hits++;
    return 1;
}

static unsigned zragf_rfc_suffix_slot_hint_hash(zragf_size_t start,
                                                     int next_index,
                                                     int remaining_splits,
                                                     int start_bitcount,
                                                     int final_block,
                                                     int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)((unsigned)(next_index + 257) * 131u);
    h ^= (unsigned)((unsigned)(remaining_splits + 257) * 911u);
    h ^= (unsigned)((unsigned)start_bitcount * 977u);
    h ^= (unsigned)((unsigned)final_block * 8191u);
    h ^= (unsigned)((unsigned)flush_final_bits * 65537u);
    return h % ZRAGF_RFC_SUFFIX_SLOT_HINT_CAP;
}

static unsigned zragf_rfc_suffix_slot_hint_hash_alt(zragf_size_t start,
                                                         int next_index,
                                                         int remaining_splits,
                                                         int start_bitcount,
                                                         int final_block,
                                                         int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 11));
    h ^= (unsigned)((unsigned)(next_index + 257) * 257u);
    h ^= (unsigned)((unsigned)(remaining_splits + 257) * 65537u);
    h ^= (unsigned)((unsigned)start_bitcount * 131071u);
    h ^= (unsigned)((unsigned)final_block * 4099u);
    h ^= (unsigned)((unsigned)flush_final_bits * 104729u);
    h ^= 0x9e3779b9u;
    return h % ZRAGF_RFC_SUFFIX_SLOT_HINT_CAP;
}

static int zragf_rfc_suffix_slot_hint_matches(const zragf_rfc_suffix_slot_hint_entry *entry,
                                              zragf_size_t start,
                                              int next_index,
                                              int remaining_splits,
                                              int start_bitcount,
                                              int final_block,
                                              int flush_final_bits)
{
    return entry && entry->valid &&
           entry->start == start &&
           entry->next_index == next_index &&
           entry->remaining_splits == remaining_splits &&
           entry->start_bitcount == start_bitcount &&
           entry->final_block == final_block &&
           entry->flush_final_bits == flush_final_bits;
}

static zragf_rfc_suffix_slot_hint_entry *zragf_rfc_suffix_slot_hint_find(zragf_rfc_estimate_cache *cache,
                                                                         zragf_size_t start,
                                                                         int next_index,
                                                                         int remaining_splits,
                                                                         int start_bitcount,
                                                                         int final_block,
                                                                         int flush_final_bits)
{
    zragf_rfc_suffix_slot_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return NULL;
    idx = zragf_rfc_suffix_slot_hint_hash(start,
                                          next_index,
                                          remaining_splits,
                                          start_bitcount,
                                          final_block,
                                          flush_final_bits);
    entry = &cache->suffix_slot_hints[idx];
    if (zragf_rfc_suffix_slot_hint_matches(entry,
                                           start,
                                           next_index,
                                           remaining_splits,
                                           start_bitcount,
                                           final_block,
                                           flush_final_bits))
        return entry;
    alt_idx = zragf_rfc_suffix_slot_hint_hash_alt(start,
                                                  next_index,
                                                  remaining_splits,
                                                  start_bitcount,
                                                  final_block,
                                                  flush_final_bits);
    if (alt_idx == idx)
        return NULL;
    entry = &cache->suffix_slot_hints[alt_idx];
    if (zragf_rfc_suffix_slot_hint_matches(entry,
                                           start,
                                           next_index,
                                           remaining_splits,
                                           start_bitcount,
                                           final_block,
                                           flush_final_bits))
        return entry;
    return NULL;
}

static void zragf_rfc_suffix_slot_hint_store(zragf_rfc_estimate_cache *cache,
                                             zragf_size_t start,
                                             int next_index,
                                             int remaining_splits,
                                             int start_bitcount,
                                             int final_block,
                                             int flush_final_bits,
                                             unsigned slot)
{
    zragf_rfc_suffix_slot_hint_entry *entry1;
    zragf_rfc_suffix_slot_hint_entry *entry2 = NULL;
    zragf_rfc_suffix_slot_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return;
    idx = zragf_rfc_suffix_slot_hint_hash(start,
                                          next_index,
                                          remaining_splits,
                                          start_bitcount,
                                          final_block,
                                          flush_final_bits);
    entry1 = &cache->suffix_slot_hints[idx];
    alt_idx = zragf_rfc_suffix_slot_hint_hash_alt(start,
                                                  next_index,
                                                  remaining_splits,
                                                  start_bitcount,
                                                  final_block,
                                                  flush_final_bits);
    if (alt_idx != idx)
        entry2 = &cache->suffix_slot_hints[alt_idx];
    if (zragf_rfc_suffix_slot_hint_matches(entry1,
                                           start,
                                           next_index,
                                           remaining_splits,
                                           start_bitcount,
                                           final_block,
                                           flush_final_bits))
        entry = entry1;
    else if (entry2 && zragf_rfc_suffix_slot_hint_matches(entry2,
                                                          start,
                                                          next_index,
                                                          remaining_splits,
                                                          start_bitcount,
                                                          final_block,
                                                          flush_final_bits))
        entry = entry2;
    else if (!entry1->valid)
        entry = entry1;
    else if (entry2 && !entry2->valid)
        entry = entry2;
    else if (entry2 && (((slot + idx + alt_idx) & 1u) != 0u))
        entry = entry2;
    else
        entry = entry1;
    entry->start = start;
    entry->next_index = next_index;
    entry->remaining_splits = remaining_splits;
    entry->start_bitcount = start_bitcount;
    entry->final_block = final_block;
    entry->flush_final_bits = flush_final_bits;
    entry->slot = slot;
    entry->valid = 1;
}

static zragf_rfc_suffix_cache_entry *zragf_rfc_suffix_cache_find(zragf_rfc_estimate_cache *cache,
                                                                 zragf_size_t start,
                                                                 int next_index,
                                                                 int remaining_splits,
                                                                 int start_bitcount,
                                                                 int final_block,
                                                                 int flush_final_bits)
{
    unsigned i;
    zragf_rfc_suffix_slot_hint_entry *hint;
    if (!cache)
        return NULL;
    hint = zragf_rfc_suffix_slot_hint_find(cache,
                                           start,
                                           next_index,
                                           remaining_splits,
                                           start_bitcount,
                                           final_block,
                                           flush_final_bits);
    if (hint && hint->slot < ZRAGF_RFC_SUFFIX_CACHE_CAP) {
        zragf_rfc_suffix_cache_entry *entry = &cache->suffix_entries[hint->slot];
        if (entry->valid &&
            entry->start == start &&
            entry->next_index == next_index &&
            entry->remaining_splits == remaining_splits &&
            entry->start_bitcount == start_bitcount &&
            entry->final_block == final_block &&
            entry->flush_final_bits == flush_final_bits)
            return entry;
    }
    if (zragf_rfc_suffix_miss_hint_hit(cache,
                                       start,
                                       next_index,
                                       remaining_splits,
                                       start_bitcount,
                                       final_block,
                                       flush_final_bits))
        return NULL;
    for (i = 0u; i < ZRAGF_RFC_SUFFIX_CACHE_CAP; ++i) {
        zragf_rfc_suffix_cache_entry *entry = &cache->suffix_entries[i];
        if (!entry->valid)
            continue;
        if (entry->start == start &&
            entry->next_index == next_index &&
            entry->remaining_splits == remaining_splits &&
            entry->start_bitcount == start_bitcount &&
            entry->final_block == final_block &&
            entry->flush_final_bits == flush_final_bits) {
            zragf_rfc_suffix_slot_hint_store(cache,
                                             start,
                                             next_index,
                                             remaining_splits,
                                             start_bitcount,
                                             final_block,
                                             flush_final_bits,
                                             i);
            return entry;
        }
    }
    zragf_rfc_suffix_miss_hint_store(cache,
                                     start,
                                     next_index,
                                     remaining_splits,
                                     start_bitcount,
                                     final_block,
                                     flush_final_bits);
    return NULL;
}

static int zragf_rfc_partition_respects_next_index(const zragf_deflate_split_plan *plan,
                                                  const zragf_rfc_partition *part,
                                                  int next_index)
{
    if (!part || !part->valid)
        return 0;
    if (part->count <= 0)
        return 1;
    if (!plan)
        return 0;
    if (next_index <= 0)
        return 1;
    if (next_index >= plan->count)
        return 0;
    return part->offsets[0] >= plan->offsets[next_index];
}

static zragf_rfc_suffix_cache_entry *zragf_rfc_suffix_cache_find_relaxed(zragf_rfc_estimate_cache *cache,
                                                                         const zragf_deflate_split_plan *plan,
                                                                         zragf_size_t start,
                                                                         int next_index,
                                                                         int remaining_splits,
                                                                         int start_bitcount,
                                                                         int final_block,
                                                                         int flush_final_bits)
{
    unsigned i;
    zragf_rfc_suffix_cache_entry *best = NULL;
    if (!cache)
        return NULL;
    for (i = 0u; i < ZRAGF_RFC_SUFFIX_CACHE_CAP; ++i) {
        zragf_rfc_suffix_cache_entry *entry = &cache->suffix_entries[i];
        if (!entry->valid || !entry->part.valid)
            continue;
        if (entry->start != start ||
            entry->start_bitcount != start_bitcount ||
            entry->final_block != final_block ||
            entry->flush_final_bits != flush_final_bits)
            continue;
        if (entry->remaining_splits < remaining_splits)
            continue;
        if (entry->part.count > remaining_splits)
            continue;
        if (entry->next_index > next_index)
            continue;
        if (!zragf_rfc_partition_respects_next_index(plan, &entry->part, next_index))
            continue;
        if (!best || entry->part.total_size < best->part.total_size ||
            (entry->part.total_size == best->part.total_size && entry->part.count < best->part.count) ||
            (entry->part.total_size == best->part.total_size && entry->part.count == best->part.count &&
             entry->next_index > best->next_index))
            best = entry;
    }
    return best;
}

static int zragf_rfc_suffix_cache_lookup(zragf_rfc_estimate_cache *cache,
                                         const zragf_deflate_split_plan *plan,
                                         zragf_size_t start,
                                         int next_index,
                                         int remaining_splits,
                                         int start_bitcount,
                                         int final_block,
                                         int flush_final_bits,
                                         int count_stats,
                                         zragf_rfc_partition *out)
{
    zragf_rfc_suffix_cache_entry *cached;
    zragf_rfc_suffix_query_cache_entry *query_cached;

    if (!out)
        return 0;

    query_cached = zragf_rfc_suffix_query_cache_find(cache,
                                                     start,
                                                     next_index,
                                                     remaining_splits,
                                                     start_bitcount,
                                                     final_block,
                                                     flush_final_bits);
    if (query_cached) {
        *out = query_cached->part;
        if (cache) {
            zragf_rfc_suffix_lb_cache_store(cache,
                                            start,
                                            next_index,
                                            remaining_splits,
                                            start_bitcount,
                                            final_block,
                                            flush_final_bits,
                                            query_cached->part.total_size);
            if (query_cached->part.count == 0)
                zragf_rfc_suffix_exact_cache_store(cache,
                                                   start,
                                                   start_bitcount,
                                                   final_block,
                                                   flush_final_bits,
                                                   query_cached->part.total_size,
                                                   query_cached->part.end_bitcount);
        }
        if (count_stats && cache)
            cache->suffix_query_hits++;
        return query_cached->part.valid;
    }

    cached = zragf_rfc_suffix_cache_find(cache,
                                         start,
                                         next_index,
                                         remaining_splits,
                                         start_bitcount,
                                         final_block,
                                         flush_final_bits);
    if (cached) {
        *out = cached->part;
        if (cache) {
            zragf_rfc_suffix_query_cache_store(cache,
                                               start,
                                               next_index,
                                               remaining_splits,
                                               start_bitcount,
                                               final_block,
                                               flush_final_bits,
                                               &cached->part);
            zragf_rfc_suffix_lb_cache_store(cache,
                                            start,
                                            next_index,
                                            remaining_splits,
                                            start_bitcount,
                                            final_block,
                                            flush_final_bits,
                                            cached->part.total_size);
            if (cached->part.count == 0)
                zragf_rfc_suffix_exact_cache_store(cache,
                                                   start,
                                                   start_bitcount,
                                                   final_block,
                                                   flush_final_bits,
                                                   cached->part.total_size,
                                                   cached->part.end_bitcount);
        }
        if (count_stats && cache)
            cache->suffix_hits++;
        return cached->part.valid;
    }

    cached = zragf_rfc_suffix_cache_find_relaxed(cache,
                                                 plan,
                                                 start,
                                                 next_index,
                                                 remaining_splits,
                                                 start_bitcount,
                                                 final_block,
                                                 flush_final_bits);
    if (cached) {
        *out = cached->part;
        if (cache) {
            zragf_rfc_suffix_cache_store(cache,
                                         start,
                                         next_index,
                                         remaining_splits,
                                         start_bitcount,
                                         final_block,
                                         flush_final_bits,
                                         out);
            zragf_rfc_suffix_lb_cache_store(cache,
                                            start,
                                            next_index,
                                            remaining_splits,
                                            start_bitcount,
                                            final_block,
                                            flush_final_bits,
                                            out->total_size);
            if (out->count == 0)
                zragf_rfc_suffix_exact_cache_store(cache,
                                                   start,
                                                   start_bitcount,
                                                   final_block,
                                                   flush_final_bits,
                                                   out->total_size,
                                                   out->end_bitcount);
        }
        if (count_stats && cache)
            cache->suffix_relaxed_hits++;
        return cached->part.valid;
    }

    if (count_stats && cache)
        cache->suffix_misses++;
    return 0;
}

static unsigned zragf_rfc_suffix_lb_slot_hint_hash(zragf_size_t start,
                                                          int next_index,
                                                          int remaining_splits,
                                                          int start_bitcount,
                                                          int final_block,
                                                          int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 7));
    h ^= (unsigned)((unsigned)(next_index + 257) * 131u);
    h ^= (unsigned)((unsigned)(remaining_splits + 257) * 911u);
    h ^= (unsigned)((unsigned)start_bitcount * 977u);
    h ^= (unsigned)((unsigned)final_block * 8191u);
    h ^= (unsigned)((unsigned)flush_final_bits * 65537u);
    return h % ZRAGF_RFC_SUFFIX_LB_SLOT_HINT_CAP;
}

static unsigned zragf_rfc_suffix_lb_slot_hint_hash_alt(zragf_size_t start,
                                                            int next_index,
                                                            int remaining_splits,
                                                            int start_bitcount,
                                                            int final_block,
                                                            int flush_final_bits)
{
    unsigned h = (unsigned)(start ^ (start >> 11));
    h ^= (unsigned)((unsigned)(next_index + 257) * 257u);
    h ^= (unsigned)((unsigned)(remaining_splits + 257) * 65537u);
    h ^= (unsigned)((unsigned)start_bitcount * 131071u);
    h ^= (unsigned)((unsigned)final_block * 4099u);
    h ^= (unsigned)((unsigned)flush_final_bits * 104729u);
    h ^= 0x85ebca6bu;
    return h % ZRAGF_RFC_SUFFIX_LB_SLOT_HINT_CAP;
}

static int zragf_rfc_suffix_lb_slot_hint_matches(const zragf_rfc_suffix_lb_slot_hint_entry *entry,
                                                 zragf_size_t start,
                                                 int next_index,
                                                 int remaining_splits,
                                                 int start_bitcount,
                                                 int final_block,
                                                 int flush_final_bits)
{
    return entry && entry->valid &&
           entry->start == start &&
           entry->next_index == next_index &&
           entry->remaining_splits == remaining_splits &&
           entry->start_bitcount == start_bitcount &&
           entry->final_block == final_block &&
           entry->flush_final_bits == flush_final_bits;
}

static zragf_rfc_suffix_lb_slot_hint_entry *zragf_rfc_suffix_lb_slot_hint_find(zragf_rfc_estimate_cache *cache,
                                                                                zragf_size_t start,
                                                                                int next_index,
                                                                                int remaining_splits,
                                                                                int start_bitcount,
                                                                                int final_block,
                                                                                int flush_final_bits)
{
    zragf_rfc_suffix_lb_slot_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return NULL;
    idx = zragf_rfc_suffix_lb_slot_hint_hash(start,
                                             next_index,
                                             remaining_splits,
                                             start_bitcount,
                                             final_block,
                                             flush_final_bits);
    entry = &cache->suffix_lb_slot_hints[idx];
    if (zragf_rfc_suffix_lb_slot_hint_matches(entry,
                                              start,
                                              next_index,
                                              remaining_splits,
                                              start_bitcount,
                                              final_block,
                                              flush_final_bits))
        return entry;
    alt_idx = zragf_rfc_suffix_lb_slot_hint_hash_alt(start,
                                                     next_index,
                                                     remaining_splits,
                                                     start_bitcount,
                                                     final_block,
                                                     flush_final_bits);
    if (alt_idx == idx)
        return NULL;
    entry = &cache->suffix_lb_slot_hints[alt_idx];
    if (zragf_rfc_suffix_lb_slot_hint_matches(entry,
                                              start,
                                              next_index,
                                              remaining_splits,
                                              start_bitcount,
                                              final_block,
                                              flush_final_bits))
        return entry;
    return NULL;
}

static void zragf_rfc_suffix_lb_slot_hint_store(zragf_rfc_estimate_cache *cache,
                                                zragf_size_t start,
                                                int next_index,
                                                int remaining_splits,
                                                int start_bitcount,
                                                int final_block,
                                                int flush_final_bits,
                                                unsigned slot)
{
    zragf_rfc_suffix_lb_slot_hint_entry *entry1;
    zragf_rfc_suffix_lb_slot_hint_entry *entry2 = NULL;
    zragf_rfc_suffix_lb_slot_hint_entry *entry;
    unsigned idx, alt_idx;
    if (!cache)
        return;
    idx = zragf_rfc_suffix_lb_slot_hint_hash(start,
                                             next_index,
                                             remaining_splits,
                                             start_bitcount,
                                             final_block,
                                             flush_final_bits);
    entry1 = &cache->suffix_lb_slot_hints[idx];
    alt_idx = zragf_rfc_suffix_lb_slot_hint_hash_alt(start,
                                                     next_index,
                                                     remaining_splits,
                                                     start_bitcount,
                                                     final_block,
                                                     flush_final_bits);
    if (alt_idx != idx)
        entry2 = &cache->suffix_lb_slot_hints[alt_idx];
    if (zragf_rfc_suffix_lb_slot_hint_matches(entry1,
                                              start,
                                              next_index,
                                              remaining_splits,
                                              start_bitcount,
                                              final_block,
                                              flush_final_bits))
        entry = entry1;
    else if (entry2 && zragf_rfc_suffix_lb_slot_hint_matches(entry2,
                                                             start,
                                                             next_index,
                                                             remaining_splits,
                                                             start_bitcount,
                                                             final_block,
                                                             flush_final_bits))
        entry = entry2;
    else if (!entry1->valid)
        entry = entry1;
    else if (entry2 && !entry2->valid)
        entry = entry2;
    else if (entry2 && (((slot + idx + alt_idx) & 1u) != 0u))
        entry = entry2;
    else
        entry = entry1;
    entry->start = start;
    entry->next_index = next_index;
    entry->remaining_splits = remaining_splits;
    entry->start_bitcount = start_bitcount;
    entry->final_block = final_block;
    entry->flush_final_bits = flush_final_bits;
    entry->slot = slot;
    entry->valid = 1;
}

static zragf_rfc_suffix_lb_cache_entry *zragf_rfc_suffix_lb_cache_find(zragf_rfc_estimate_cache *cache,
                                                                           zragf_size_t start,
                                                                           int next_index,
                                                                           int remaining_splits,
                                                                           int start_bitcount,
                                                                           int final_block,
                                                                           int flush_final_bits)
{
    unsigned i;
    zragf_rfc_suffix_lb_slot_hint_entry *hint;
    if (!cache)
        return NULL;
    hint = zragf_rfc_suffix_lb_slot_hint_find(cache,
                                              start,
                                              next_index,
                                              remaining_splits,
                                              start_bitcount,
                                              final_block,
                                              flush_final_bits);
    if (hint && hint->slot < ZRAGF_RFC_SUFFIX_LB_CACHE_CAP) {
        zragf_rfc_suffix_lb_cache_entry *entry = &cache->suffix_lb_entries[hint->slot];
        if (entry->valid &&
            entry->start == start &&
            entry->next_index == next_index &&
            entry->remaining_splits == remaining_splits &&
            entry->start_bitcount == start_bitcount &&
            entry->final_block == final_block &&
            entry->flush_final_bits == flush_final_bits)
            return entry;
    }
    if (zragf_rfc_suffix_lb_miss_hint_hit(cache,
                                          start,
                                          next_index,
                                          remaining_splits,
                                          start_bitcount,
                                          final_block,
                                          flush_final_bits))
        return NULL;
    for (i = 0u; i < ZRAGF_RFC_SUFFIX_LB_CACHE_CAP; ++i) {
        zragf_rfc_suffix_lb_cache_entry *entry = &cache->suffix_lb_entries[i];
        if (!entry->valid)
            continue;
        if (entry->start == start &&
            entry->next_index == next_index &&
            entry->remaining_splits == remaining_splits &&
            entry->start_bitcount == start_bitcount &&
            entry->final_block == final_block &&
            entry->flush_final_bits == flush_final_bits) {
            zragf_rfc_suffix_lb_slot_hint_store(cache,
                                                start,
                                                next_index,
                                                remaining_splits,
                                                start_bitcount,
                                                final_block,
                                                flush_final_bits,
                                                i);
            return entry;
        }
    }
    zragf_rfc_suffix_lb_miss_hint_store(cache,
                                        start,
                                        next_index,
                                        remaining_splits,
                                        start_bitcount,
                                        final_block,
                                        flush_final_bits);
    return NULL;
}

static zragf_rfc_suffix_lb_cache_entry *zragf_rfc_suffix_lb_cache_find_relaxed(zragf_rfc_estimate_cache *cache,
                                                                                zragf_size_t start,
                                                                                int next_index,
                                                                                int remaining_splits,
                                                                                int start_bitcount,
                                                                                int final_block,
                                                                                int flush_final_bits)
{
    unsigned i;
    zragf_rfc_suffix_lb_cache_entry *best = NULL;
    if (!cache)
        return NULL;
    for (i = 0u; i < ZRAGF_RFC_SUFFIX_LB_CACHE_CAP; ++i) {
        zragf_rfc_suffix_lb_cache_entry *entry = &cache->suffix_lb_entries[i];
        if (!entry->valid)
            continue;
        if (entry->start != start ||
            entry->start_bitcount != start_bitcount ||
            entry->final_block != final_block ||
            entry->flush_final_bits != flush_final_bits)
            continue;
        if (entry->next_index > next_index)
            continue;
        if (entry->remaining_splits < remaining_splits)
            continue;
        if (!best || entry->total_size < best->total_size ||
            (entry->total_size == best->total_size && entry->next_index > best->next_index) ||
            (entry->total_size == best->total_size && entry->next_index == best->next_index &&
             entry->remaining_splits > best->remaining_splits))
            best = entry;
    }
    return best;
}

static void zragf_rfc_suffix_lb_cache_store(zragf_rfc_estimate_cache *cache,
                                            zragf_size_t start,
                                            int next_index,
                                            int remaining_splits,
                                            int start_bitcount,
                                            int final_block,
                                            int flush_final_bits,
                                            zragf_size_t total_size)
{
    zragf_rfc_suffix_lb_cache_entry *entry;
    if (!cache)
        return;
    entry = zragf_rfc_suffix_lb_cache_find(cache,
                                           start,
                                           next_index,
                                           remaining_splits,
                                           start_bitcount,
                                           final_block,
                                           flush_final_bits);
    if (!entry) {
        entry = &cache->suffix_lb_entries[cache->suffix_lb_next_slot % ZRAGF_RFC_SUFFIX_LB_CACHE_CAP];
        cache->suffix_lb_next_slot = (cache->suffix_lb_next_slot + 1u) % ZRAGF_RFC_SUFFIX_LB_CACHE_CAP;
    }
    cache->suffix_lb_generation++;
    entry->start = start;
    entry->next_index = next_index;
    entry->remaining_splits = remaining_splits;
    entry->start_bitcount = start_bitcount;
    entry->final_block = final_block;
    entry->flush_final_bits = flush_final_bits;
    entry->total_size = total_size;
    entry->valid = 1;
    zragf_rfc_suffix_lb_slot_hint_store(cache,
                                        start,
                                        next_index,
                                        remaining_splits,
                                        start_bitcount,
                                        final_block,
                                        flush_final_bits,
                                        (unsigned)(entry - cache->suffix_lb_entries));
    zragf_rfc_suffix_lb_query_cache_store(cache,
                                          start,
                                          next_index,
                                          remaining_splits,
                                          start_bitcount,
                                          final_block,
                                          flush_final_bits,
                                          total_size);
}

static int zragf_rfc_suffix_cache_lookup_lower_bound(zragf_rfc_estimate_cache *cache,
                                                     zragf_size_t start,
                                                     int next_index,
                                                     int remaining_splits,
                                                     int start_bitcount,
                                                     int final_block,
                                                     int flush_final_bits,
                                                     zragf_size_t *out_total_size)
{
    unsigned i;
    int found = 0;
    zragf_size_t best_total = 0u;

    zragf_rfc_suffix_lb_cache_entry *cached;
    zragf_rfc_suffix_lb_query_cache_entry *query_cached;

    if (!cache || !out_total_size)
        return 0;

    query_cached = zragf_rfc_suffix_lb_query_cache_find(cache,
                                                        start,
                                                        next_index,
                                                        remaining_splits,
                                                        start_bitcount,
                                                        final_block,
                                                        flush_final_bits);
    if (query_cached) {
        *out_total_size = query_cached->total_size;
        cache->suffix_lower_bound_query_hits++;
        return 1;
    }

    cached = zragf_rfc_suffix_lb_cache_find(cache,
                                            start,
                                            next_index,
                                            remaining_splits,
                                            start_bitcount,
                                            final_block,
                                            flush_final_bits);
    if (cached) {
        *out_total_size = cached->total_size;
        zragf_rfc_suffix_lb_query_cache_store(cache,
                                              start,
                                              next_index,
                                              remaining_splits,
                                              start_bitcount,
                                              final_block,
                                              flush_final_bits,
                                              cached->total_size);
        cache->suffix_lower_bound_query_hits++;
        return 1;
    }

    cached = zragf_rfc_suffix_lb_cache_find_relaxed(cache,
                                                    start,
                                                    next_index,
                                                    remaining_splits,
                                                    start_bitcount,
                                                    final_block,
                                                    flush_final_bits);
    if (cached) {
        *out_total_size = cached->total_size;
        zragf_rfc_suffix_lb_cache_store(cache,
                                        start,
                                        next_index,
                                        remaining_splits,
                                        start_bitcount,
                                        final_block,
                                        flush_final_bits,
                                        cached->total_size);
        cache->suffix_lower_bound_relaxed_hits++;
        return 1;
    }

    for (i = 0u; i < ZRAGF_RFC_SUFFIX_CACHE_CAP; ++i) {
        zragf_rfc_suffix_cache_entry *entry = &cache->suffix_entries[i];
        if (!entry->valid || !entry->part.valid)
            continue;
        if (entry->start != start ||
            entry->start_bitcount != start_bitcount ||
            entry->final_block != final_block ||
            entry->flush_final_bits != flush_final_bits)
            continue;
        if (entry->next_index > next_index)
            continue;
        if (entry->remaining_splits < remaining_splits)
            continue;
        if (!found || entry->part.total_size < best_total) {
            best_total = entry->part.total_size;
            found = 1;
        }
    }

    if (!found)
        return 0;

    *out_total_size = best_total;
    zragf_rfc_suffix_lb_cache_store(cache,
                                    start,
                                    next_index,
                                    remaining_splits,
                                    start_bitcount,
                                    final_block,
                                    flush_final_bits,
                                    best_total);
    cache->suffix_lower_bound_hits++;
    return 1;
}


static void zragf_rfc_suffix_cache_store(zragf_rfc_estimate_cache *cache,
                                         zragf_size_t start,
                                         int next_index,
                                         int remaining_splits,
                                         int start_bitcount,
                                         int final_block,
                                         int flush_final_bits,
                                         const zragf_rfc_partition *part)
{
    zragf_rfc_suffix_cache_entry *entry;
    if (!cache || !part)
        return;
    entry = zragf_rfc_suffix_cache_find(cache,
                                        start,
                                        next_index,
                                        remaining_splits,
                                        start_bitcount,
                                        final_block,
                                        flush_final_bits);
    if (!entry) {
        entry = &cache->suffix_entries[cache->suffix_next_slot % ZRAGF_RFC_SUFFIX_CACHE_CAP];
        cache->suffix_next_slot = (cache->suffix_next_slot + 1u) % ZRAGF_RFC_SUFFIX_CACHE_CAP;
    }
    cache->suffix_generation++;
    memset(entry, 0, sizeof(*entry));
    entry->start = start;
    entry->next_index = next_index;
    entry->remaining_splits = remaining_splits;
    entry->start_bitcount = start_bitcount;
    entry->final_block = final_block;
    entry->flush_final_bits = flush_final_bits;
    entry->part = *part;
    entry->valid = 1;
    zragf_rfc_suffix_slot_hint_store(cache,
                                     start,
                                     next_index,
                                     remaining_splits,
                                     start_bitcount,
                                     final_block,
                                     flush_final_bits,
                                     (unsigned)(entry - cache->suffix_entries));
    zragf_rfc_suffix_query_cache_store(cache,
                                       start,
                                       next_index,
                                       remaining_splits,
                                       start_bitcount,
                                       final_block,
                                       flush_final_bits,
                                       part);
    zragf_rfc_suffix_lb_cache_store(cache,
                                    start,
                                    next_index,
                                    remaining_splits,
                                    start_bitcount,
                                    final_block,
                                    flush_final_bits,
                                    part->total_size);
    if (part->count == 0)
        zragf_rfc_suffix_exact_cache_store(cache,
                                           start,
                                           start_bitcount,
                                           final_block,
                                           flush_final_bits,
                                           part->total_size,
                                           part->end_bitcount);
}

static void zragf_rfc_estimate_cache_cleanup(zragf_rfc_estimate_cache *cache)
{
    unsigned i;
    if (!cache)
        return;
    for (i = 0u; i < ZRAGF_RFC_ESTIMATE_CACHE_CAP; ++i)
        zragf_rfc_prepared_segment_free(&cache->entries[i].prepared);
}

static zragf_size_t zragf_rfc_history_tail_copy(const zragf_u8 *dict,
                                                zragf_size_t dict_size,
                                                const zragf_u8 *prefix,
                                                zragf_size_t prefix_len,
                                                zragf_u8 *dst);

static int zragf_rfc_estimate_range_cached(zragf_rfc_estimate_cache *cache,
                                           const zragf_u8 *dict,
                                           zragf_size_t dict_size,
                                           const zragf_u8 *src,
                                           zragf_size_t src_size,
                                           zragf_size_t seg_start,
                                           zragf_size_t seg_end,
                                           int final_block,
                                           int level,
                                           int strategy,
                                           int tune_set,
                                           int good_length,
                                           int max_lazy,
                                           int nice_length,
                                           int max_chain,
                                           int start_bitcount,
                                           int flush_final_bits,
                                           zragf_size_t *out_size,
                                           int *out_end_bitcount,
                                           zragf_block_type *out_best_type)
{
    zragf_u8 hist[ZRAGF_RFC_WINDOW];
    const zragf_u8 *seg_dict = dict;
    zragf_size_t seg_dict_size = dict_size;
    zragf_size_t seg_size;
    zragf_block_type best_type = ZRAGF_BLOCK_STORED;
    zragf_rfc_estimate_cache_entry *donor = NULL;

    if (!out_size)
        return 0;
    if (seg_end < seg_start || seg_end > src_size)
        return 0;

    seg_size = seg_end - seg_start;

    if (cache) {
        zragf_rfc_estimate_cache_entry *entry = zragf_rfc_estimate_cache_find(cache,
                                                                               seg_start, seg_end,
                                                                               final_block,
                                                                               start_bitcount,
                                                                               flush_final_bits);
        if (entry) {
            *out_size = entry->size;
            if (out_end_bitcount)
                *out_end_bitcount = entry->end_bitcount;
            if (out_best_type)
                *out_best_type = entry->best_type;
            cache->hits++;
            return 1;
        }
        cache->misses++;
        donor = zragf_rfc_estimate_cache_find_prepared(cache, seg_start, seg_end);
    }

    if (donor) {
        zragf_rfc_candidate cands[3];
        zragf_rfc_prepared_segment prepared_copy;
        zragf_rfc_prepared_segment *prepared_ptr = NULL;
        zragf_rfc_prepared_segment_init(&prepared_copy);
        if (!zragf_rfc_estimate_candidates(src + seg_start, seg_size,
                                           &donor->prepared.tb,
                                           &donor->prepared.stats,
                                           &donor->prepared.dynprep,
                                           final_block, strategy,
                                           start_bitcount, flush_final_bits,
                                           cands, &best_type))
            return 0;
        if (!cands[0].ok)
            return 0;
        *out_size = cands[0].size;
        if (out_end_bitcount)
            *out_end_bitcount = cands[0].end_bitcount;
        if (out_best_type)
            *out_best_type = cands[0].type;
        if (cands[0].type != ZRAGF_BLOCK_STORED &&
            donor->prepared.valid && donor->prepared.tb.data &&
            donor->prepared.tb.size <= ZRAGF_RFC_PREPARED_TOKEN_MAX &&
            zragf_rfc_prepared_segment_clone(&prepared_copy, &donor->prepared))
            prepared_ptr = &prepared_copy;
        if (cache) {
            zragf_rfc_estimate_cache_store(cache,
                                           seg_start, seg_end,
                                           final_block,
                                           start_bitcount,
                                           flush_final_bits,
                                           *out_size,
                                           out_end_bitcount ? *out_end_bitcount : 0,
                                           cands[0].type,
                                           prepared_ptr);
        }
        zragf_rfc_prepared_segment_free(&prepared_copy);
        return 1;
    }

    if (seg_start > 0u) {
        seg_dict_size = zragf_rfc_history_tail_copy(dict, dict_size, src, seg_start, hist);
        seg_dict = hist;
    }

    {
        zragf_rfc_prepared_segment prepared;
        zragf_rfc_prepared_segment_init(&prepared);
        if (!zragf_rfc_estimate_single_impl(seg_dict, seg_dict_size,
                                            src + seg_start, seg_size,
                                            final_block,
                                            level, strategy,
                                            tune_set,
                                            good_length,
                                            max_lazy,
                                            nice_length,
                                            max_chain,
                                            start_bitcount,
                                            flush_final_bits,
                                            out_size,
                                            out_end_bitcount,
                                            &best_type,
                                            &prepared)) {
            zragf_rfc_prepared_segment_free(&prepared);
            return 0;
        }

        if (out_best_type)
            *out_best_type = best_type;

        if (cache) {
            zragf_rfc_estimate_cache_store(cache,
                                           seg_start, seg_end,
                                           final_block,
                                           start_bitcount,
                                           flush_final_bits,
                                           *out_size,
                                           out_end_bitcount ? *out_end_bitcount : 0,
                                           best_type,
                                           &prepared);
        }
        zragf_rfc_prepared_segment_free(&prepared);
        return 1;
    }
}


static ZRAGF_MAYBE_UNUSED zragf_size_t zragf_rfc_split_bound(zragf_size_t src_size)
{
    return zragf_deflate_rfc1951_stored_bound(src_size) + ZRAGF_RFC_SPLIT_EXTRA_BOUND;
}

static zragf_size_t zragf_rfc_history_tail_len(zragf_size_t dict_size,
                                               zragf_size_t prefix_len)
{
    zragf_size_t total = dict_size + prefix_len;
    return (total > (zragf_size_t)ZRAGF_RFC_WINDOW) ? (zragf_size_t)ZRAGF_RFC_WINDOW : total;
}

static zragf_size_t zragf_rfc_history_tail_copy(const zragf_u8 *dict,
                                                zragf_size_t dict_size,
                                                const zragf_u8 *prefix,
                                                zragf_size_t prefix_len,
                                                zragf_u8 *dst)
{
    zragf_size_t keep;
    zragf_size_t drop;

    if (!dst)
        return 0u;

    keep = zragf_rfc_history_tail_len(dict_size, prefix_len);
    drop = dict_size + prefix_len - keep;

    if (drop >= dict_size) {
        drop -= dict_size;
        dict = NULL;
        dict_size = 0u;
        if (drop > prefix_len)
            drop = prefix_len;
        prefix += drop;
        prefix_len -= drop;
    } else {
        dict += drop;
        dict_size -= drop;
    }

    if (dict && dict_size > 0u) {
        memcpy(dst, dict, dict_size);
        dst += dict_size;
    }
    if (prefix && prefix_len > 0u)
        memcpy(dst, prefix, prefix_len);
    return keep;
}

static int zragf_rfc_should_consider_split(const zragf_u8 *dict,
                                           zragf_size_t dict_size,
                                           const zragf_u8 *src,
                                           zragf_size_t src_size,
                                           int strategy,
                                           int level,
                                           int depth)
{
    zragf_deflate_match_profile profile;
    if (depth <= 0)
        return 0;
    if (src_size < 16384u)
        return 0;
    if (strategy == ZRAGF_Z_FIXED || strategy == ZRAGF_Z_HUFFMAN_ONLY || strategy == ZRAGF_Z_RLE)
        return 0;
    if (level <= 1)
        return 0;

    zragf_deflate_core_profile_input(dict, dict_size, src, src_size, &profile);
    /* Structured text already benefits from the matcher; exact repartitioning costs too much here. */
    if (profile.structured_text_like && src_size >= 65536u)
        return 0;

    return 1;
}

static int zragf_deflate_rfc1951_compress_recursive(const zragf_u8 *dict,
                                                    zragf_size_t dict_size,
                                                    const zragf_u8 *src,
                                                    zragf_size_t src_size,
                                                    zragf_u8 *dst,
                                                    zragf_size_t dst_cap,
                                                    zragf_size_t *dst_size,
                                                    int final_block,
                                                    int level,
                                                    int strategy,
                                                    int tune_set,
                                                    int good_length,
                                                    int max_lazy,
                                                    int nice_length,
                                                    int max_chain,
                                                    unsigned *bitbuf_io,
                                                    int *bitcount_io,
                                                    int flush_final_bits,
                                                    int split_depth);

typedef struct {
    zragf_size_t offset;
    zragf_size_t estimated_total;
    int valid;
} zragf_rfc_split_probe;

static ZRAGF_MAYBE_UNUSED int zragf_rfc_split_probe_cmp(const void *a, const void *b)
{
    const zragf_rfc_split_probe *pa = (const zragf_rfc_split_probe *)a;
    const zragf_rfc_split_probe *pb = (const zragf_rfc_split_probe *)b;
    if (pa->valid != pb->valid)
        return pb->valid - pa->valid;
    if (pa->estimated_total < pb->estimated_total)
        return -1;
    if (pa->estimated_total > pb->estimated_total)
        return 1;
    if (pa->offset < pb->offset)
        return -1;
    if (pa->offset > pb->offset)
        return 1;
    return 0;
}

static ZRAGF_MAYBE_UNUSED void zragf_rfc_plan_from_probes(zragf_deflate_split_plan *dst,
                                       zragf_rfc_split_probe *probes,
                                       int probe_count)
{
    zragf_size_t best_total = 0u;
    int i;

    if (!dst)
        return;
    zragf_deflate_plan_reset(dst);
    if (!probes || probe_count <= 0)
        return;

    qsort(probes, (size_t)probe_count, sizeof(probes[0]), zragf_rfc_split_probe_cmp);
    for (i = 0; i < probe_count; ++i) {
        int j;
        int duplicate = 0;
        if (!probes[i].valid)
            continue;
        if (best_total == 0u)
            best_total = probes[i].estimated_total;
        if (dst->count >= ZRAGF_RFC_PARTITION_TOP_PROBES)
            break;
        if (dst->count >= 2 && probes[i].estimated_total > best_total + ZRAGF_RFC_PARTITION_SLACK)
            continue;
        for (j = 0; j < dst->count; ++j) {
            if (dst->offsets[j] == probes[i].offset) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate)
            dst->offsets[dst->count++] = probes[i].offset;
    }
    for (i = 0; i < probe_count && dst->count < 2; ++i) {
        int j;
        int duplicate = 0;
        if (!probes[i].valid)
            continue;
        for (j = 0; j < dst->count; ++j) {
            if (dst->offsets[j] == probes[i].offset) {
                duplicate = 1;
                break;
            }
        }
        if (!duplicate)
            dst->offsets[dst->count++] = probes[i].offset;
    }
    for (i = 0; i < dst->count; ++i) {
        int j;
        for (j = i + 1; j < dst->count; ++j) {
            if (dst->offsets[j] < dst->offsets[i]) {
                zragf_size_t t = dst->offsets[i];
                dst->offsets[i] = dst->offsets[j];
                dst->offsets[j] = t;
            }
        }
    }
}

static int zragf_rfc_probe_split_candidate(zragf_rfc_estimate_cache *cache,
                                           const zragf_u8 *dict,
                                           zragf_size_t dict_size,
                                           const zragf_u8 *src,
                                           zragf_size_t src_size,
                                           zragf_size_t split,
                                           int final_block,
                                           int level,
                                           int strategy,
                                           int tune_set,
                                           int good_length,
                                           int max_lazy,
                                           int nice_length,
                                           int max_chain,
                                           unsigned *bitbuf_io,
                                           int *bitcount_io,
                                           int flush_final_bits,
                                           zragf_rfc_split_probe *probe)
{
    zragf_size_t left_size = 0u;
    zragf_size_t right_size = 0u;
    int left_bitcount;
    int right_bitcount;

    (void)bitbuf_io;

    if (probe) {
        probe->offset = split;
        probe->estimated_total = 0u;
        probe->valid = 0;
    }

    if (split < 4096u || split + 4096u > src_size)
        return 0;

    left_bitcount = bitcount_io ? *bitcount_io : 0;
    if (!zragf_rfc_estimate_range_cached(cache,
                                         dict, dict_size,
                                         src, src_size,
                                         0u, split,
                                         0,
                                         level, strategy,
                                         tune_set,
                                         good_length,
                                         max_lazy,
                                         nice_length,
                                         max_chain,
                                         left_bitcount,
                                         0,
                                         &left_size,
                                         &left_bitcount,
                                         NULL))
        return 0;

    right_bitcount = left_bitcount;
    if (!zragf_rfc_estimate_range_cached(cache,
                                         dict, dict_size,
                                         src, src_size,
                                         split, src_size,
                                         final_block,
                                         level, strategy,
                                         tune_set,
                                         good_length,
                                         max_lazy,
                                         nice_length,
                                         max_chain,
                                         right_bitcount,
                                         flush_final_bits,
                                         &right_size,
                                         &right_bitcount,
                                         NULL))
        return 0;

    if (probe) {
        probe->offset = split;
        probe->estimated_total = left_size + right_size;
        probe->valid = 1;
    }
    return 1;
}
static ZRAGF_MAYBE_UNUSED int zragf_rfc_estimate_partition(zragf_rfc_estimate_cache *cache,
                                        const zragf_u8 *dict,
                                        zragf_size_t dict_size,
                                        const zragf_u8 *src,
                                        zragf_size_t src_size,
                                        const zragf_size_t *offsets,
                                        int offset_count,
                                        int final_block,
                                        int level,
                                        int strategy,
                                        int tune_set,
                                        int good_length,
                                        int max_lazy,
                                        int nice_length,
                                        int max_chain,
                                        int start_bitcount,
                                        int flush_final_bits,
                                        zragf_size_t *out_total_size,
                                        int *out_end_bitcount,
                                        zragf_size_t stop_at)
{
    zragf_size_t seg_start = 0u;
    zragf_size_t total = 0u;
    int bitcount = start_bitcount;
    int i;

    if (!out_total_size)
        return 0;
    if (offset_count < 0 || offset_count > ZRAGF_RFC_MAX_PARTITION_SPLITS)
        return 0;

    for (i = 0; i <= offset_count; ++i) {
        zragf_size_t seg_end = (i < offset_count) ? offsets[i] : src_size;
        zragf_size_t seg_bytes = 0u;
        int is_last;

        if (seg_end < seg_start || seg_end > src_size)
            return 0;
        is_last = (i == offset_count) ? 1 : 0;

        if (!zragf_rfc_estimate_range_cached(cache,
                                             dict, dict_size,
                                             src, src_size,
                                             seg_start, seg_end,
                                             is_last ? final_block : 0,
                                             level, strategy,
                                             tune_set,
                                             good_length,
                                             max_lazy,
                                             nice_length,
                                             max_chain,
                                             bitcount,
                                             is_last ? flush_final_bits : 0,
                                             &seg_bytes,
                                             &bitcount,
                                             NULL))
            return 0;
        total += seg_bytes;
        if (stop_at > 0u && total >= stop_at)
            return 0;
        seg_start = seg_end;
    }

    *out_total_size = total;
    if (out_end_bitcount)
        *out_end_bitcount = bitcount;
    return 1;
}
static int zragf_rfc_emit_range_cached(zragf_rfc_estimate_cache *cache,
                                       const zragf_u8 *dict,
                                       zragf_size_t dict_size,
                                       const zragf_u8 *src,
                                       zragf_size_t src_size,
                                       zragf_size_t seg_start,
                                       zragf_size_t seg_end,
                                       zragf_u8 *dst,
                                       zragf_size_t dst_cap,
                                       zragf_size_t *dst_size,
                                       int final_block,
                                       int level,
                                       int strategy,
                                       int tune_set,
                                       int good_length,
                                       int max_lazy,
                                       int nice_length,
                                       int max_chain,
                                       unsigned *bitbuf_io,
                                       int *bitcount_io,
                                       int flush_final_bits)
{
    zragf_rfc_estimate_cache_entry *entry = NULL;
    zragf_block_type best_type = ZRAGF_BLOCK_STORED;
    zragf_size_t seg_size;

    if (!dst || !dst_size)
        return 0;
    if (seg_end < seg_start || seg_end > src_size)
        return 0;
    seg_size = seg_end - seg_start;

    if (cache) {
        entry = zragf_rfc_estimate_cache_find(cache,
                                              seg_start, seg_end,
                                              final_block,
                                              bitcount_io ? *bitcount_io : 0,
                                              flush_final_bits);
    }
    if (entry)
        best_type = entry->best_type;

    if (best_type != ZRAGF_BLOCK_STORED) {
        const zragf_rfc_prepared_segment *prepared = NULL;
        if (entry && entry->prepared.valid && entry->prepared.tb.data)
            prepared = &entry->prepared;
        else if (cache) {
            zragf_rfc_estimate_cache_entry *donor = zragf_rfc_estimate_cache_find_prepared(cache,
                                                                                            seg_start,
                                                                                            seg_end);
            if (donor && donor->prepared.valid && donor->prepared.tb.data)
                prepared = &donor->prepared;
        }
        if (prepared) {
            return zragf_rfc_emit_candidate(best_type,
                                            src + seg_start, seg_size,
                                            &prepared->tb,
                                            &prepared->stats,
                                            &prepared->dynprep,
                                            dst, dst_cap, dst_size,
                                            final_block,
                                            bitbuf_io, bitcount_io,
                                            flush_final_bits);
        }
    }

    return zragf_rfc_emit_single_known_type_impl(dict, dict_size,
                                                 src + seg_start, seg_size,
                                                 best_type,
                                                 dst, dst_cap, dst_size,
                                                 final_block,
                                                 level, strategy,
                                                 tune_set,
                                                 good_length,
                                                 max_lazy,
                                                 nice_length,
                                                 max_chain,
                                                 bitbuf_io,
                                                 bitcount_io,
                                                 flush_final_bits);
}

static int zragf_rfc_emit_partition(zragf_rfc_estimate_cache *cache,
                                    const zragf_u8 *dict,
                                    zragf_size_t dict_size,
                                    const zragf_u8 *src,
                                    zragf_size_t src_size,
                                    const zragf_size_t *offsets,
                                    int offset_count,
                                    zragf_u8 *dst,
                                    zragf_size_t dst_cap,
                                    zragf_size_t *dst_size,
                                    int final_block,
                                    int level,
                                    int strategy,
                                    int tune_set,
                                    int good_length,
                                    int max_lazy,
                                    int nice_length,
                                    int max_chain,
                                    unsigned *bitbuf_io,
                                    int *bitcount_io,
                                    int flush_final_bits)
{
    zragf_u8 hist[ZRAGF_RFC_WINDOW];
    zragf_size_t seg_start = 0u;
    zragf_size_t out_pos = 0u;
    unsigned bitbuf = bitbuf_io ? *bitbuf_io : 0u;
    int bitcount = bitcount_io ? *bitcount_io : 0;
    int i;

    if (!dst || !dst_size)
        return 0;
    if (offset_count < 0 || offset_count > ZRAGF_RFC_MAX_PARTITION_SPLITS)
        return 0;

    for (i = 0; i <= offset_count; ++i) {
        zragf_size_t seg_end = (i < offset_count) ? offsets[i] : src_size;
        const zragf_u8 *seg_dict = dict;
        zragf_size_t seg_dict_size = dict_size;
        zragf_size_t seg_size;
        zragf_size_t wrote = 0u;
        zragf_size_t cached_size = 0u;
        zragf_block_type best_type = ZRAGF_BLOCK_STORED;
        int is_last;

        if (seg_end < seg_start || seg_end > src_size)
            return 0;
        seg_size = seg_end - seg_start;
        is_last = (i == offset_count) ? 1 : 0;

        if (seg_start > 0u) {
            seg_dict_size = zragf_rfc_history_tail_copy(dict, dict_size, src, seg_start, hist);
            seg_dict = hist;
        }

        if (!zragf_rfc_estimate_range_cached(cache,
                                             dict, dict_size,
                                             src, src_size,
                                             seg_start, seg_end,
                                             is_last ? final_block : 0,
                                             level, strategy,
                                             tune_set,
                                             good_length,
                                             max_lazy,
                                             nice_length,
                                             max_chain,
                                             bitcount,
                                             is_last ? flush_final_bits : 0,
                                             &cached_size,
                                             NULL,
                                             &best_type)) {
            best_type = ZRAGF_BLOCK_STORED;
        }

        if (!zragf_rfc_emit_range_cached(cache,
                                         seg_dict, seg_dict_size,
                                         src, src_size,
                                         seg_start, seg_end,
                                         dst + out_pos, dst_cap - out_pos, &wrote,
                                         is_last ? final_block : 0,
                                         level, strategy,
                                         tune_set,
                                         good_length,
                                         max_lazy,
                                         nice_length,
                                         max_chain,
                                         &bitbuf, &bitcount,
                                         is_last ? flush_final_bits : 0)) {
            if (!zragf_deflate_rfc1951_compress_single_impl(seg_dict, seg_dict_size,
                                                            src + seg_start, seg_size,
                                                            dst + out_pos, dst_cap - out_pos, &wrote,
                                                            is_last ? final_block : 0,
                                                            level, strategy,
                                                            tune_set,
                                                            good_length,
                                                            max_lazy,
                                                            nice_length,
                                                            max_chain,
                                                            &bitbuf, &bitcount,
                                                            is_last ? flush_final_bits : 0))
                return 0;
        }
        out_pos += wrote;
        if (out_pos > dst_cap)
            return 0;
        seg_start = seg_end;
    }

    *dst_size = out_pos;
    if (bitbuf_io)
        *bitbuf_io = bitbuf;
    if (bitcount_io)
        *bitcount_io = bitcount;
    return 1;
}

static ZRAGF_MAYBE_UNUSED void zragf_rfc_partition_try_update(zragf_rfc_partition *best,
                                           const zragf_size_t *offsets,
                                           int offset_count,
                                           zragf_size_t total_size,
                                           int end_bitcount)
{
    int i;
    int copy_count = offset_count;
    if (!best)
        return;
    if (copy_count < 0)
        copy_count = 0;
    if (copy_count > ZRAGF_RFC_MAX_PARTITION_SPLITS)
        copy_count = ZRAGF_RFC_MAX_PARTITION_SPLITS;
    if (!best->valid || total_size < best->total_size) {
        best->count = copy_count;
        for (i = 0; i < copy_count; ++i)
            best->offsets[i] = offsets[i];
        best->total_size = total_size;
        best->end_bitcount = end_bitcount;
        best->valid = 1;
    }
}

typedef struct {
    zragf_size_t split;
    zragf_size_t seg_bytes;
    zragf_size_t cheap_total;
    zragf_size_t exact_total;
    int seg_end_bitcount;
    int exact_end_bitcount;
    int plan_index;
    int has_tail_part;
    int has_tail_lower_bound;
    int has_exact_tail;
    int valid;
    zragf_rfc_partition tail_part;
} zragf_rfc_suffix_candidate;

static int zragf_rfc_suffix_candidate_cmp(const void *a, const void *b)
{
    const zragf_rfc_suffix_candidate *ca = (const zragf_rfc_suffix_candidate *)a;
    const zragf_rfc_suffix_candidate *cb = (const zragf_rfc_suffix_candidate *)b;

    if (ca->valid != cb->valid)
        return cb->valid - ca->valid;
    if (!ca->valid)
        return 0;
    if (ca->cheap_total < cb->cheap_total)
        return -1;
    if (ca->cheap_total > cb->cheap_total)
        return 1;
    if (ca->has_exact_tail != cb->has_exact_tail)
        return cb->has_exact_tail - ca->has_exact_tail;
    if (ca->has_exact_tail && cb->has_exact_tail) {
        if (ca->exact_total < cb->exact_total)
            return -1;
        if (ca->exact_total > cb->exact_total)
            return 1;
    }
    if (ca->has_tail_part != cb->has_tail_part)
        return cb->has_tail_part - ca->has_tail_part;
    if (ca->seg_bytes < cb->seg_bytes)
        return -1;
    if (ca->seg_bytes > cb->seg_bytes)
        return 1;
    if (ca->split < cb->split)
        return -1;
    if (ca->split > cb->split)
        return 1;
    return 0;
}

static int zragf_rfc_find_best_suffix_limited(zragf_rfc_estimate_cache *cache,
                                              const zragf_u8 *dict,
                                              zragf_size_t dict_size,
                                              const zragf_u8 *src,
                                              zragf_size_t src_size,
                                              const zragf_deflate_split_plan *plan,
                                              zragf_size_t partial_start,
                                              int next_index,
                                              int remaining_splits,
                                              int final_block,
                                              int level,
                                              int strategy,
                                              int tune_set,
                                              int good_length,
                                              int max_lazy,
                                              int nice_length,
                                              int max_chain,
                                              int start_bitcount,
                                              int flush_final_bits,
                                              int has_upper_bound,
                                              zragf_size_t upper_bound,
                                              zragf_rfc_partition *best)
{
    zragf_size_t tail_size = 0u;
    int tail_end_bitcount = start_bitcount;
    zragf_size_t prune_limit = (zragf_size_t)(~(zragf_size_t)0);
    zragf_rfc_suffix_candidate candidates[ZRAGF_DEFLATE_PLAN_MAX_SPLITS];
    int candidate_count = 0;
    int i;

    if (!best || !plan)
        return 0;

    if (cache && zragf_rfc_suffix_cache_lookup(cache,
                                               plan,
                                               partial_start,
                                               next_index,
                                               remaining_splits,
                                               start_bitcount,
                                               final_block,
                                               flush_final_bits,
                                               1,
                                               best))
        return best->valid;

    memset(best, 0, sizeof(*best));
    memset(candidates, 0, sizeof(candidates));
    if (!zragf_rfc_suffix_exact_cache_lookup(cache,
                                            partial_start,
                                            start_bitcount,
                                            final_block,
                                            flush_final_bits,
                                            &tail_size,
                                            &tail_end_bitcount)) {
        if (!zragf_rfc_estimate_range_cached(cache,
                                             dict, dict_size,
                                             src, src_size,
                                             partial_start, src_size,
                                             final_block,
                                             level, strategy,
                                             tune_set,
                                             good_length,
                                             max_lazy,
                                             nice_length,
                                             max_chain,
                                             start_bitcount,
                                             flush_final_bits,
                                             &tail_size,
                                             &tail_end_bitcount,
                                             NULL)) {
            return 0;
        }
        zragf_rfc_suffix_exact_cache_store(cache,
                                           partial_start,
                                           start_bitcount,
                                           final_block,
                                           flush_final_bits,
                                           tail_size,
                                           tail_end_bitcount);
    }
    best->valid = 1;
    best->count = 0;
    best->total_size = tail_size;
    best->end_bitcount = tail_end_bitcount;

    prune_limit = best->total_size;
    if (has_upper_bound && upper_bound < prune_limit)
        prune_limit = upper_bound;

    if (remaining_splits <= 0)
        goto done;

    if (has_upper_bound && cache) {
        zragf_size_t tail_lower_bound = 0u;
        if (zragf_rfc_suffix_cache_lookup_lower_bound(cache,
                                                      partial_start,
                                                      next_index,
                                                      remaining_splits,
                                                      start_bitcount,
                                                      final_block,
                                                      flush_final_bits,
                                                      &tail_lower_bound) &&
            tail_lower_bound >= prune_limit + ZRAGF_RFC_PARTITION_RECURSE_SLACK)
            goto done;
    }

    for (i = next_index; i < plan->count && candidate_count < ZRAGF_DEFLATE_PLAN_MAX_SPLITS; ++i) {
        zragf_size_t split = plan->offsets[i];
        zragf_size_t seg_bytes = 0u;
        int seg_end_bitcount = start_bitcount;
        zragf_rfc_suffix_candidate *cand = &candidates[candidate_count];

        if (split <= partial_start || split >= src_size)
            continue;

        if (!zragf_rfc_estimate_range_cached(cache,
                                             dict, dict_size,
                                             src, src_size,
                                             partial_start, split,
                                             0,
                                             level, strategy,
                                             tune_set,
                                             good_length,
                                             max_lazy,
                                             nice_length,
                                             max_chain,
                                             start_bitcount,
                                             0,
                                             &seg_bytes,
                                             &seg_end_bitcount,
                                             NULL)) {
            continue;
        }

        if (best->valid && seg_bytes >= prune_limit + ZRAGF_RFC_PARTITION_RECURSE_SLACK)
            continue;

        memset(cand, 0, sizeof(*cand));
        cand->split = split;
        cand->seg_bytes = seg_bytes;
        cand->seg_end_bitcount = seg_end_bitcount;
        cand->plan_index = i;
        cand->cheap_total = seg_bytes;
        cand->exact_total = (zragf_size_t)(~(zragf_size_t)0);
        cand->valid = 1;

        if (zragf_rfc_suffix_cache_lookup(cache,
                                          plan,
                                          split,
                                          i + 1,
                                          remaining_splits - 1,
                                          seg_end_bitcount,
                                          final_block,
                                          flush_final_bits,
                                          0,
                                          &cand->tail_part)) {
            cand->has_tail_part = 1;
            cand->cheap_total = seg_bytes + cand->tail_part.total_size;
        } else {
            zragf_size_t tail_lower_bound = 0u;
            if (zragf_rfc_suffix_cache_lookup_lower_bound(cache,
                                                          split,
                                                          i + 1,
                                                          remaining_splits - 1,
                                                          seg_end_bitcount,
                                                          final_block,
                                                          flush_final_bits,
                                                          &tail_lower_bound)) {
                cand->has_tail_lower_bound = 1;
                if (seg_bytes + tail_lower_bound < cand->cheap_total)
                    cand->cheap_total = seg_bytes + tail_lower_bound;
            }
        }

        if (best->valid && (cand->has_tail_part || cand->has_tail_lower_bound) &&
            cand->cheap_total >= prune_limit + ZRAGF_RFC_PARTITION_RECURSE_SLACK)
            continue;

        if (!cand->has_tail_part) {
            zragf_size_t exact_tail_size = 0u;
            int exact_tail_end_bitcount = seg_end_bitcount;
            if (zragf_rfc_suffix_exact_cache_lookup(cache,
                                                    split,
                                                    seg_end_bitcount,
                                                    final_block,
                                                    flush_final_bits,
                                                    &exact_tail_size,
                                                    &exact_tail_end_bitcount) ||
                zragf_rfc_estimate_range_cached(cache,
                                                dict, dict_size,
                                                src, src_size,
                                                split, src_size,
                                                final_block,
                                                level, strategy,
                                                tune_set,
                                                good_length,
                                                max_lazy,
                                                nice_length,
                                                max_chain,
                                                seg_end_bitcount,
                                                flush_final_bits,
                                                &exact_tail_size,
                                                &exact_tail_end_bitcount,
                                                NULL)) {
                zragf_rfc_suffix_exact_cache_store(cache,
                                                   split,
                                                   seg_end_bitcount,
                                                   final_block,
                                                   flush_final_bits,
                                                   exact_tail_size,
                                                   exact_tail_end_bitcount);
                cand->has_exact_tail = 1;
                cand->exact_total = seg_bytes + exact_tail_size;
                cand->exact_end_bitcount = exact_tail_end_bitcount;
                if (cand->exact_total < cand->cheap_total)
                    cand->cheap_total = cand->exact_total;
                if (!best->valid || cand->exact_total < best->total_size) {
                    zragf_size_t one_split[1];
                    one_split[0] = split;
                    zragf_rfc_partition_try_update(best,
                                                   one_split,
                                                   1,
                                                   cand->exact_total,
                                                   exact_tail_end_bitcount);
                    prune_limit = best->total_size;
                    if (has_upper_bound && upper_bound < prune_limit)
                        prune_limit = upper_bound;
                }
            }
        }
        candidate_count++;
    }

    if (candidate_count > 1)
        qsort(candidates, (size_t)candidate_count, sizeof(candidates[0]), zragf_rfc_suffix_candidate_cmp);

    for (i = 0; i < candidate_count; ++i) {
        const zragf_rfc_suffix_candidate *cand = &candidates[i];
        zragf_rfc_partition tail_part;
        zragf_size_t total;
        int j;

        if (!cand->valid)
            continue;

        if (best->valid && cand->seg_bytes >= prune_limit + ZRAGF_RFC_PARTITION_RECURSE_SLACK)
            continue;

        if (best->valid && (cand->has_tail_part || cand->has_tail_lower_bound) &&
            cand->cheap_total >= prune_limit + ZRAGF_RFC_PARTITION_RECURSE_SLACK)
            continue;

        if (cand->has_tail_part) {
            tail_part = cand->tail_part;
            if (best->valid && cand->seg_bytes + tail_part.total_size >= prune_limit + ZRAGF_RFC_PARTITION_RECURSE_SLACK)
                continue;
        } else if (!zragf_rfc_find_best_suffix_limited(cache,
                                                       dict, dict_size,
                                                       src, src_size,
                                                       plan,
                                                       cand->split,
                                                       cand->plan_index + 1,
                                                       remaining_splits - 1,
                                                       final_block,
                                                       level, strategy,
                                                       tune_set,
                                                       good_length,
                                                       max_lazy,
                                                       nice_length,
                                                       max_chain,
                                                       cand->seg_end_bitcount,
                                                       flush_final_bits,
                                                       best->valid && prune_limit > cand->seg_bytes,
                                                       (best->valid && prune_limit > cand->seg_bytes)
                                                           ? (prune_limit - cand->seg_bytes)
                                                           : 0u,
                                                       &tail_part) || !tail_part.valid) {
            if (cand->has_exact_tail) {
                memset(&tail_part, 0, sizeof(tail_part));
                tail_part.valid = 1;
                tail_part.count = 0;
                tail_part.total_size = cand->exact_total - cand->seg_bytes;
                tail_part.end_bitcount = cand->exact_end_bitcount;
            } else {
                continue;
            }
        }

        total = cand->seg_bytes + tail_part.total_size;
        if (!best->valid || total < best->total_size) {
            best->valid = 1;
            best->count = tail_part.count + 1;
            if (best->count > ZRAGF_RFC_MAX_PARTITION_SPLITS)
                best->count = ZRAGF_RFC_MAX_PARTITION_SPLITS;
            best->offsets[0] = cand->split;
            for (j = 0; j < tail_part.count && j + 1 < ZRAGF_RFC_MAX_PARTITION_SPLITS; ++j)
                best->offsets[j + 1] = tail_part.offsets[j];
            best->total_size = total;
            best->end_bitcount = tail_part.end_bitcount;
            prune_limit = best->total_size;
            if (has_upper_bound && upper_bound < prune_limit)
                prune_limit = upper_bound;
        }
    }

done:
    if (cache)
        zragf_rfc_suffix_cache_store(cache,
                                     partial_start,
                                     next_index,
                                     remaining_splits,
                                     start_bitcount,
                                     final_block,
                                     flush_final_bits,
                                     best);
    return best->valid;
}

static int zragf_rfc_find_best_suffix(zragf_rfc_estimate_cache *cache,
                                      const zragf_u8 *dict,
                                      zragf_size_t dict_size,
                                      const zragf_u8 *src,
                                      zragf_size_t src_size,
                                      const zragf_deflate_split_plan *plan,
                                      zragf_size_t partial_start,
                                      int next_index,
                                      int remaining_splits,
                                      int final_block,
                                      int level,
                                      int strategy,
                                      int tune_set,
                                      int good_length,
                                      int max_lazy,
                                      int nice_length,
                                      int max_chain,
                                      int start_bitcount,
                                      int flush_final_bits,
                                      zragf_rfc_partition *best)
{
    return zragf_rfc_find_best_suffix_limited(cache,
                                              dict, dict_size,
                                              src, src_size,
                                              plan,
                                              partial_start,
                                              next_index,
                                              remaining_splits,
                                              final_block,
                                              level, strategy,
                                              tune_set,
                                              good_length,
                                              max_lazy,
                                              nice_length,
                                              max_chain,
                                              start_bitcount,
                                              flush_final_bits,
                                              0,
                                              0u,
                                              best);
}

static ZRAGF_MAYBE_UNUSED int zragf_rfc_find_best_partition(zragf_rfc_estimate_cache *cache,
                                         const zragf_u8 *dict,
                                         zragf_size_t dict_size,
                                         const zragf_u8 *src,
                                         zragf_size_t src_size,
                                         const zragf_deflate_split_plan *plan,
                                         int final_block,
                                         int level,
                                         int strategy,
                                         int tune_set,
                                         int good_length,
                                         int max_lazy,
                                         int nice_length,
                                         int max_chain,
                                         int start_bitcount,
                                         int flush_final_bits,
                                         zragf_rfc_partition *best)
{
    int max_splits;

    if (!best)
        return 0;
    memset(best, 0, sizeof(*best));
    if (!plan || plan->count <= 0)
        return 0;

    max_splits = plan->count;
    if (max_splits > ZRAGF_RFC_PARTITION_MAX_SEARCH_SPLITS)
        max_splits = ZRAGF_RFC_PARTITION_MAX_SEARCH_SPLITS;

    if (!zragf_rfc_find_best_suffix(cache,
                                    dict, dict_size,
                                    src, src_size,
                                    plan,
                                    0u,
                                    0,
                                    max_splits,
                                    final_block,
                                    level, strategy,
                                    tune_set,
                                    good_length,
                                    max_lazy,
                                    nice_length,
                                    max_chain,
                                    start_bitcount,
                                    flush_final_bits,
                                    best))
        return 0;
    return best->valid && best->count > 0;
}
static ZRAGF_MAYBE_UNUSED int zragf_rfc_try_split_candidate(const zragf_u8 *dict,
                                         zragf_size_t dict_size,
                                         const zragf_u8 *src,
                                         zragf_size_t src_size,
                                         zragf_size_t split,
                                         zragf_u8 *dst,
                                         zragf_size_t dst_cap,
                                         zragf_size_t *dst_size,
                                         int final_block,
                                         int level,
                                         int strategy,
                                         int tune_set,
                                         int good_length,
                                         int max_lazy,
                                         int nice_length,
                                         int max_chain,
                                         unsigned *bitbuf_io,
                                         int *bitcount_io,
                                         int flush_final_bits,
                                         int split_depth,
                                         zragf_size_t current_best)
{
    zragf_u8 *tmp_left = NULL;
    zragf_u8 *tmp_right = NULL;
    zragf_u8 *right_hist = NULL;
    zragf_size_t left_cap;
    zragf_size_t right_cap;
    zragf_size_t left_size = 0u;
    zragf_size_t right_size = 0u;
    zragf_size_t total_size;
    zragf_size_t right_hist_len;
    unsigned left_bitbuf;
    unsigned right_bitbuf;
    int left_bitcount;
    int right_bitcount;
    int ok = 0;

    if (split < 4096u || split + 4096u > src_size)
        return 0;

    left_cap = zragf_rfc_split_bound(split);
    right_cap = zragf_rfc_split_bound(src_size - split);

    tmp_left = (zragf_u8 *)zragf_alloc_default(NULL, left_cap, 1u);
    tmp_right = (zragf_u8 *)zragf_alloc_default(NULL, right_cap, 1u);
    right_hist_len = zragf_rfc_history_tail_len(dict_size, split);
    if (right_hist_len > 0u)
        right_hist = (zragf_u8 *)zragf_alloc_default(NULL, right_hist_len, 1u);

    if (!tmp_left || !tmp_right || (right_hist_len > 0u && !right_hist))
        goto done;

    left_bitbuf = bitbuf_io ? *bitbuf_io : 0u;
    left_bitcount = bitcount_io ? *bitcount_io : 0;
    if (!zragf_deflate_rfc1951_compress_recursive(dict, dict_size,
                                                  src, split,
                                                  tmp_left, left_cap, &left_size,
                                                  0,
                                                  level, strategy,
                                                  tune_set,
                                                  good_length,
                                                  max_lazy,
                                                  nice_length,
                                                  max_chain,
                                                  &left_bitbuf, &left_bitcount,
                                                  0,
                                                  split_depth - 1))
        goto done;

    right_hist_len = zragf_rfc_history_tail_copy(dict, dict_size, src, split, right_hist);
    right_bitbuf = left_bitbuf;
    right_bitcount = left_bitcount;
    if (!zragf_deflate_rfc1951_compress_recursive(right_hist, right_hist_len,
                                                  src + split, src_size - split,
                                                  tmp_right, right_cap, &right_size,
                                                  final_block,
                                                  level, strategy,
                                                  tune_set,
                                                  good_length,
                                                  max_lazy,
                                                  nice_length,
                                                  max_chain,
                                                  &right_bitbuf, &right_bitcount,
                                                  flush_final_bits,
                                                  split_depth - 1))
        goto done;

    total_size = left_size + right_size;
    if (total_size + 12u >= current_best || total_size > dst_cap)
        goto done;

    memcpy(dst, tmp_left, left_size);
    memcpy(dst + left_size, tmp_right, right_size);
    *dst_size = total_size;
    if (bitbuf_io)
        *bitbuf_io = right_bitbuf;
    if (bitcount_io)
        *bitcount_io = right_bitcount;
    ok = 1;

done:
    zragf_free_default(NULL, tmp_left);
    zragf_free_default(NULL, tmp_right);
    zragf_free_default(NULL, right_hist);
    return ok;
}

static int zragf_deflate_rfc1951_compress_impl(const zragf_u8 *dict,
                                               zragf_size_t dict_size,
                                               const zragf_u8 *src,
                                               zragf_size_t src_size,
                                               zragf_u8 *dst,
                                               zragf_size_t dst_cap,
                                               zragf_size_t *dst_size,
                                               int final_block,
                                               int level,
                                               int strategy,
                                               int tune_set,
                                               int good_length,
                                               int max_lazy,
                                               int nice_length,
                                               int max_chain,
                                               unsigned *bitbuf_io,
                                               int *bitcount_io,
                                               int flush_final_bits)
{
    return zragf_deflate_rfc1951_compress_recursive(dict, dict_size,
                                                    src, src_size,
                                                    dst, dst_cap, dst_size,
                                                    final_block,
                                                    level, strategy,
                                                    tune_set,
                                                    good_length,
                                                    max_lazy,
                                                    nice_length,
                                                    max_chain,
                                                    bitbuf_io,
                                                    bitcount_io,
                                                    flush_final_bits,
                                                    ZRAGF_RFC_SPLIT_DEPTH);
}

static int zragf_deflate_rfc1951_compress_recursive(const zragf_u8 *dict,
                                                    zragf_size_t dict_size,
                                                    const zragf_u8 *src,
                                                    zragf_size_t src_size,
                                                    zragf_u8 *dst,
                                                    zragf_size_t dst_cap,
                                                    zragf_size_t *dst_size,
                                                    int final_block,
                                                    int level,
                                                    int strategy,
                                                    int tune_set,
                                                    int good_length,
                                                    int max_lazy,
                                                    int nice_length,
                                                    int max_chain,
                                                    unsigned *bitbuf_io,
                                                    int *bitcount_io,
                                                    int flush_final_bits,
                                                    int split_depth)
{
    zragf_deflate_split_plan plan;
    zragf_rfc_estimate_cache est_cache;
    zragf_size_t single_size = 0u;
    zragf_size_t stored_bound = 0u;
    zragf_block_type single_type = ZRAGF_BLOCK_STORED;
    int i;
    int split_allowed;
    int start_bitcount;

    if (!dst || !dst_size)
        return 0;

    start_bitcount = bitcount_io ? *bitcount_io : 0;
    memset(&est_cache, 0, sizeof(est_cache));

    split_allowed = zragf_rfc_should_consider_split(dict, dict_size,
                                                  src, src_size,
                                                  strategy, level,
                                                  split_depth);
    if (!split_allowed) {
        int ok = zragf_deflate_rfc1951_compress_single_impl(dict, dict_size,
                                                            src, src_size,
                                                            dst, dst_cap, dst_size,
                                                            final_block,
                                                            level, strategy,
                                                            tune_set,
                                                            good_length,
                                                            max_lazy,
                                                            nice_length,
                                                            max_chain,
                                                            bitbuf_io,
                                                            bitcount_io,
                                                            flush_final_bits);
        zragf_rfc_estimate_cache_cleanup(&est_cache);
        return ok;
    }

    if (!zragf_rfc_estimate_range_cached(&est_cache,
                                         dict, dict_size,
                                         src, src_size,
                                         0u, src_size,
                                         final_block,
                                         level, strategy,
                                         tune_set,
                                         good_length,
                                         max_lazy,
                                         nice_length,
                                         max_chain,
                                         start_bitcount,
                                         flush_final_bits,
                                         &single_size,
                                         NULL,
                                         &single_type)) {
        int ok = zragf_deflate_rfc1951_compress_single_impl(dict, dict_size,
                                                            src, src_size,
                                                            dst, dst_cap, dst_size,
                                                            final_block,
                                                            level, strategy,
                                                            tune_set,
                                                            good_length,
                                                            max_lazy,
                                                            nice_length,
                                                            max_chain,
                                                            bitbuf_io,
                                                            bitcount_io,
                                                            flush_final_bits);
        zragf_rfc_estimate_cache_cleanup(&est_cache);
        return ok;
    }

    stored_bound = zragf_deflate_rfc1951_stored_bound(src_size);
    if (split_allowed && (single_size + 8u >= stored_bound ||
                          (src_size > 0u && single_size * 20u < src_size)))
        split_allowed = 0;

    if (split_allowed && zragf_deflate_plan_build(src, src_size, &plan)) {
        zragf_rfc_partition best_part;
        zragf_rfc_split_probe best_probe;

        memset(&best_part, 0, sizeof(best_part));
        memset(&best_probe, 0, sizeof(best_probe));

        for (i = 0; i < plan.count; ++i) {
            zragf_rfc_split_probe probe;
            memset(&probe, 0, sizeof(probe));
            if (!zragf_rfc_probe_split_candidate(&est_cache,
                                                 dict, dict_size,
                                                 src, src_size,
                                                 plan.offsets[i],
                                                 final_block,
                                                 level, strategy,
                                                 tune_set,
                                                 good_length,
                                                 max_lazy,
                                                 nice_length,
                                                 max_chain,
                                                 bitbuf_io,
                                                 bitcount_io,
                                                 flush_final_bits,
                                                 &probe)) {
                continue;
            }
            if (probe.valid && (!best_probe.valid || probe.estimated_total < best_probe.estimated_total))
                best_probe = probe;
        }

        if (best_probe.valid && plan.count > 1 && best_probe.estimated_total + 32u < single_size) {
            best_part.valid = 1;
            best_part.count = 1;
            best_part.offsets[0] = best_probe.offset;
            best_part.total_size = best_probe.estimated_total;
            best_part.end_bitcount = 0;
            if (zragf_rfc_find_best_partition(&est_cache,
                                              dict, dict_size,
                                              src, src_size,
                                              &plan,
                                              final_block,
                                              level, strategy,
                                              tune_set,
                                              good_length,
                                              max_lazy,
                                              nice_length,
                                              max_chain,
                                              start_bitcount,
                                              flush_final_bits,
                                              &best_part) &&
                best_part.valid && best_part.count > 1 && best_part.total_size + 4u < single_size) {
                if (zragf_rfc_emit_partition(&est_cache,
                                             dict, dict_size,
                                             src, src_size,
                                             best_part.offsets,
                                             best_part.count,
                                             dst, dst_cap, dst_size,
                                             final_block,
                                             level, strategy,
                                             tune_set,
                                             good_length,
                                             max_lazy,
                                             nice_length,
                                             max_chain,
                                             bitbuf_io,
                                             bitcount_io,
                                             flush_final_bits)) {
                    zragf_rfc_estimate_cache_cleanup(&est_cache);
                    return 1;
                }
            }
        }

        if (best_probe.valid && best_probe.estimated_total + 4u < single_size) {
            zragf_size_t off = best_probe.offset;
            if (zragf_rfc_emit_partition(&est_cache,
                                         dict, dict_size,
                                         src, src_size,
                                         &off,
                                         1,
                                         dst, dst_cap, dst_size,
                                         final_block,
                                         level, strategy,
                                         tune_set,
                                         good_length,
                                         max_lazy,
                                         nice_length,
                                         max_chain,
                                         bitbuf_io,
                                         bitcount_io,
                                         flush_final_bits)) {
                zragf_rfc_estimate_cache_cleanup(&est_cache);
                return 1;
            }
        }
    }

    if (!zragf_rfc_emit_range_cached(&est_cache,
                                     dict, dict_size,
                                     src, src_size,
                                     0u, src_size,
                                     dst, dst_cap, dst_size,
                                     final_block,
                                     level, strategy,
                                     tune_set,
                                     good_length,
                                     max_lazy,
                                     nice_length,
                                     max_chain,
                                     bitbuf_io,
                                     bitcount_io,
                                     flush_final_bits)) {
        int ok = zragf_deflate_rfc1951_compress_single_impl(dict, dict_size,
                                                            src, src_size,
                                                            dst, dst_cap, dst_size,
                                                            final_block,
                                                            level, strategy,
                                                            tune_set,
                                                            good_length,
                                                            max_lazy,
                                                            nice_length,
                                                            max_chain,
                                                            bitbuf_io,
                                                            bitcount_io,
                                                            flush_final_bits);
        zragf_rfc_estimate_cache_cleanup(&est_cache);
        return ok;
    }
    zragf_rfc_estimate_cache_cleanup(&est_cache);
    return 1;
}


int zragf_deflate_rfc1951_compress_chunk_single_stream(const zragf_u8 *dict,
                                                       zragf_size_t    dict_size,
                                                       const zragf_u8 *src,
                                                       zragf_size_t    src_size,
                                                       zragf_u8       *dst,
                                                       zragf_size_t    dst_cap,
                                                       zragf_size_t   *dst_size,
                                                       int             final_block,
                                                       int             level,
                                                       int             strategy,
                                                       int             tune_set,
                                                       int             good_length,
                                                       int             max_lazy,
                                                       int             nice_length,
                                                       int             max_chain,
                                                       unsigned       *bitbuf_io,
                                                       int            *bitcount_io,
                                                       int             flush_final_bits)
{
    return zragf_deflate_rfc1951_compress_single_impl(dict, dict_size,
                                                      src, src_size,
                                                      dst, dst_cap, dst_size,
                                                      final_block,
                                                      level, strategy,
                                                      tune_set,
                                                      good_length,
                                                      max_lazy,
                                                      nice_length,
                                                      max_chain,
                                                      bitbuf_io, bitcount_io,
                                                      flush_final_bits);
}

int zragf_deflate_rfc1951_compress_chunk_single_with_dict(const zragf_u8 *dict,
                                                          zragf_size_t    dict_size,
                                                          const zragf_u8 *src,
                                                          zragf_size_t    src_size,
                                                          zragf_u8       *dst,
                                                          zragf_size_t    dst_cap,
                                                          zragf_size_t   *dst_size,
                                                          int             final_block,
                                                          int             level,
                                                          int             strategy,
                                                          int             tune_set,
                                                          int             good_length,
                                                          int             max_lazy,
                                                          int             nice_length,
                                                          int             max_chain)
{
    unsigned bitbuf = 0u;
    int bitcount = 0;
    return zragf_deflate_rfc1951_compress_single_impl(dict, dict_size,
                                                      src, src_size,
                                                      dst, dst_cap, dst_size,
                                                      final_block,
                                                      level, strategy,
                                                      tune_set,
                                                      good_length,
                                                      max_lazy,
                                                      nice_length,
                                                      max_chain,
                                                      &bitbuf, &bitcount,
                                                      1);
}

int zragf_deflate_rfc1951_compress_chunk_stream(const zragf_u8 *dict,
                                                    zragf_size_t    dict_size,
                                                    const zragf_u8 *src,
                                                    zragf_size_t    src_size,
                                                    zragf_u8       *dst,
                                                    zragf_size_t    dst_cap,
                                                    zragf_size_t   *dst_size,
                                                    int             final_block,
                                                    int             level,
                                                    int             strategy,
                                                    int             tune_set,
                                                    int             good_length,
                                                    int             max_lazy,
                                                    int             nice_length,
                                                    int             max_chain,
                                                    unsigned       *bitbuf_io,
                                                    int            *bitcount_io,
                                                    int             flush_final_bits)
{
    return zragf_deflate_rfc1951_compress_impl(dict, dict_size,
                                               src, src_size,
                                               dst, dst_cap, dst_size,
                                               final_block,
                                               level, strategy,
                                               tune_set,
                                               good_length,
                                               max_lazy,
                                               nice_length,
                                               max_chain,
                                               bitbuf_io, bitcount_io,
                                               flush_final_bits);
}

int zragf_deflate_rfc1951_compress_chunk_with_dict(const zragf_u8 *dict,
                                                   zragf_size_t    dict_size,
                                                   const zragf_u8 *src,
                                                   zragf_size_t    src_size,
                                                   zragf_u8       *dst,
                                                   zragf_size_t    dst_cap,
                                                   zragf_size_t   *dst_size,
                                                   int             final_block,
                                                   int             level,
                                                   int             strategy,
                                                   int             tune_set,
                                                   int             good_length,
                                                   int             max_lazy,
                                                   int             nice_length,
                                                   int             max_chain)
{
    unsigned bitbuf = 0u;
    int bitcount = 0;
    return zragf_deflate_rfc1951_compress_impl(dict, dict_size,
                                               src, src_size,
                                               dst, dst_cap, dst_size,
                                               final_block,
                                               level, strategy,
                                               tune_set,
                                               good_length,
                                               max_lazy,
                                               nice_length,
                                               max_chain,
                                               &bitbuf, &bitcount,
                                               1);
}

int zragf_deflate_rfc1951_compress_chunk(const zragf_u8 *src,
                                         zragf_size_t    src_size,
                                         zragf_u8       *dst,
                                         zragf_size_t    dst_cap,
                                         zragf_size_t   *dst_size,
                                         int             final_block,
                                         int             level,
                                         int             strategy,
                                         int             tune_set,
                                         int             good_length,
                                         int             max_lazy,
                                         int             nice_length,
                                         int             max_chain)
{
    {
        unsigned bitbuf = 0u;
        int bitcount = 0;
        return zragf_deflate_rfc1951_compress_impl(NULL, 0u,
                                                   src, src_size,
                                                   dst, dst_cap, dst_size,
                                                   final_block,
                                                   level, strategy,
                                                   tune_set,
                                                   good_length,
                                                   max_lazy,
                                                   nice_length,
                                                   max_chain,
                                                   &bitbuf, &bitcount,
                                                   1);
    }
}

int zragf_deflate_rfc1951_compress(const zragf_u8 *src,
                                   zragf_size_t    src_size,
                                   zragf_u8       *dst,
                                   zragf_size_t    dst_cap,
                                   zragf_size_t   *dst_size,
                                   int             level,
                                   int             strategy,
                                   int             tune_set,
                                   int             good_length,
                                   int             max_lazy,
                                   int             nice_length,
                                   int             max_chain)
{
    {
        unsigned bitbuf = 0u;
        int bitcount = 0;
        return zragf_deflate_rfc1951_compress_impl(NULL, 0u,
                                                   src, src_size,
                                                   dst, dst_cap, dst_size,
                                                   1,
                                                   level, strategy,
                                                   tune_set,
                                                   good_length,
                                                   max_lazy,
                                                   nice_length,
                                                   max_chain,
                                                   &bitbuf, &bitcount,
                                                   1);
    }
}
