#include "deflate_matcher.h"
#include "deflate_core.h"
#include <limits.h>

#define ZRAGF_RFC_HASH_BITS 15
#define ZRAGF_RFC_HASH_SIZE (1 << ZRAGF_RFC_HASH_BITS)
#define ZRAGF_RFC_HASH_MASK (ZRAGF_RFC_HASH_SIZE - 1)
#define ZRAGF_RFC_WINDOW    32768

static unsigned zragf_hash3_bytes(zragf_u8 a, zragf_u8 b, zragf_u8 c)
{
    unsigned h = (unsigned)a;
    h = ((h << 5) ^ (unsigned)b) & ZRAGF_RFC_HASH_MASK;
    h = ((h << 5) ^ (unsigned)c) & ZRAGF_RFC_HASH_MASK;
    return h;
}

static int zragf_match_len_fast(const zragf_u8 *src,
                                zragf_size_t pos,
                                zragf_size_t cand,
                                int max_len)
{
    int len = 0;

    while (len + 4 <= max_len) {
        zragf_size_t base_pos = pos + (zragf_size_t)len;
        zragf_size_t base_cand = cand + (zragf_size_t)len;
        if (src[base_pos] != src[base_cand] ||
            src[base_pos + 1u] != src[base_cand + 1u] ||
            src[base_pos + 2u] != src[base_cand + 2u] ||
            src[base_pos + 3u] != src[base_cand + 3u])
            break;
        len += 4;
    }

    while (len < max_len && src[cand + (zragf_size_t)len] == src[pos + (zragf_size_t)len])
        len++;

    return len;
}

static void zragf_insert_pos(const zragf_u8 *src,
                             zragf_size_t size,
                             zragf_size_t pos,
                             int *head,
                             int *prev)
{
    unsigned h;
    if (!src || !head || !prev)
        return;
    if (pos + 2u >= size)
        return;
    h = zragf_hash3_bytes(src[pos], src[pos + 1u], src[pos + 2u]);
    prev[pos] = head[h];
    head[h] = (int)pos;
}

static void zragf_insert_match_range(const zragf_u8 *src,
                                     zragf_size_t size,
                                     zragf_size_t from,
                                     zragf_size_t to,
                                     int *head,
                                     int *prev,
                                     int structured_fast,
                                     int large_buffer_fast,
                                     int match_len)
{
    zragf_size_t j;

    if (!(structured_fast || large_buffer_fast) || match_len < 24) {
        for (j = from; j < to; ++j)
            zragf_insert_pos(src, size, j, head, prev);
        return;
    }

    {
        zragf_size_t head_keep = from + (structured_fast ? 6u : 6u);
        zragf_size_t tail_slack = structured_fast ? 8u : 8u;
        zragf_size_t tail_keep = (to > tail_slack) ? (to - tail_slack) : from;
        zragf_size_t sparse_step;

        if (match_len >= 192)
            sparse_step = structured_fast ? 4u : 3u;
        else if (match_len >= 96)
            sparse_step = 3u;
        else
            sparse_step = structured_fast ? 2u : 2u;

        if (head_keep > to)
            head_keep = to;
        if (tail_keep < head_keep)
            tail_keep = head_keep;

        for (j = from; j < to; ++j) {
            if (j < head_keep || j >= tail_keep || (((j - head_keep) % sparse_step) == 0u))
                zragf_insert_pos(src, size, j, head, prev);
        }
    }
}

static int zragf_push_literal_with_stats(zragf_token_buffer *tb,
                                        zragf_block_stats *stats,
                                        int lit)
{
    if (tb->size == tb->cap && !zragf_tokens_reserve_extra(tb, 128u))
        return 0;
    if (lit < 0)
        lit = 0;
    if (lit > 255)
        lit = 255;
    zragf_tokens_append_literal_unchecked(tb, lit);
    stats->litlen_freq[lit]++;
    stats->raw_size++;
    return 1;
}

static int zragf_push_match_with_stats(zragf_token_buffer *tb,
                                      zragf_block_stats *stats,
                                      const unsigned short *len_tab,
                                      const unsigned short *dist_tab,
                                      int len,
                                      int dist)
{
    if (tb->size == tb->cap && !zragf_tokens_reserve_extra(tb, 128u))
        return 0;
    if (len < 3)
        len = 3;
    if (len > 258)
        len = 258;
    if (dist < 1)
        dist = 1;
    if (dist > 32768)
        dist = 32768;
    zragf_tokens_append_match_unchecked(tb, len, dist);
    stats->litlen_freq[len_tab[len]]++;
    stats->dist_freq[dist_tab[dist]]++;
    stats->raw_size += len;
    return 1;
}

static void zragf_find_best_match(const zragf_u8 *src,
                                  zragf_size_t size,
                                  zragf_size_t pos,
                                  const int *head,
                                  const int *prev,
                                  int max_chain,
                                  int nice_len,
                                  int *out_len,
                                  int *out_dist)
{
    unsigned h;
    int candidate;
    int best_len = 0;
    int best_dist = 0;
    int limit;
    int remaining;

    *out_len = 0;
    *out_dist = 0;

    if (!src || !head || !prev)
        return;
    if (pos + 2u >= size)
        return;

    h = zragf_hash3_bytes(src[pos], src[pos + 1u], src[pos + 2u]);
    candidate = head[h];
    limit = (pos > ZRAGF_RFC_WINDOW) ? (int)(pos - ZRAGF_RFC_WINDOW) : 0;
    remaining = max_chain;

    while (candidate >= limit && candidate >= 0 && remaining-- > 0) {
        zragf_size_t cand = (zragf_size_t)candidate;
        int len = 0;
        int dist = (int)(pos - cand);

        if (dist <= 0 || dist > ZRAGF_RFC_WINDOW) {
            candidate = prev[cand];
            continue;
        }

        if (best_len >= 4) {
            int probe = best_len - 1;
            if (pos + (zragf_size_t)probe < size && cand + (zragf_size_t)probe < size &&
                src[cand + (zragf_size_t)probe] != src[pos + (zragf_size_t)probe]) {
                candidate = prev[cand];
                continue;
            }
        }

        if (src[cand] == src[pos] &&
            src[cand + 1u] == src[pos + 1u] &&
            src[cand + 2u] == src[pos + 2u]) {
            int max_len = (size - pos > 258u) ? 258 : (int)(size - pos);
            /* Cheap 4th-byte reject to cut dense 3-byte hash collisions on text-heavy inputs. */
            if (max_len > 3 && cand + 3u < size && src[cand + 3u] != src[pos + 3u]) {
                candidate = prev[cand];
                continue;
            }
            if (best_len >= 8) {
                int probe2 = best_len;
                if (probe2 >= max_len)
                    probe2 = max_len - 1;
                if (probe2 >= 3 && cand + (zragf_size_t)probe2 < size && pos + (zragf_size_t)probe2 < size &&
                    src[cand + (zragf_size_t)probe2] != src[pos + (zragf_size_t)probe2]) {
                    candidate = prev[cand];
                    continue;
                }
            }
            len = zragf_match_len_fast(src, pos, cand, max_len);
            if (len > best_len) {
                best_len = len;
                best_dist = dist;
                if (len >= 64 && remaining > 12)
                    remaining = 12;
                else if (len >= 32 && remaining > 24)
                    remaining = 24;
                if (len >= nice_len)
                    break;
            }
        }

        candidate = prev[cand];
    }

    if (best_len >= 3) {
        *out_len = best_len;
        *out_dist = best_dist;
    }
}

static int zragf_tokenize_literals_only(const zragf_u8 *src,
                                        zragf_size_t src_size,
                                        zragf_token_buffer *tb,
                                        zragf_block_stats *stats)
{
    zragf_size_t pos;

    if (!tb || !stats)
        return 0;

    zragf_tokens_reset(tb);
    zragf_blocks_reset_stats(stats);

    for (pos = 0u; pos < src_size; ++pos) {
        if (!zragf_push_literal_with_stats(tb, stats, src[pos]))
            return 0;
    }

    stats->litlen_freq[256]++;
    return 1;
}

static int zragf_tokenize_rle_only(const zragf_u8 *src,
                                   zragf_size_t src_size,
                                   zragf_token_buffer *tb,
                                   zragf_block_stats *stats)
{
    zragf_size_t pos = 0u;
    const unsigned short *len_tab;
    const unsigned short *dist_tab;

    if (!tb || !stats)
        return 0;

    zragf_tokens_reset(tb);
    zragf_blocks_reset_stats(stats);
    len_tab = zragf_blocks_len_sym_table();
    dist_tab = zragf_blocks_dist_sym_table();

    while (pos < src_size) {
        zragf_size_t run = 1u;
        while (pos + run < src_size && src[pos + run] == src[pos])
            run++;

        if (run >= 3u && pos > 0u && src[pos - 1u] == src[pos]) {
            zragf_size_t remaining = run;
            while (remaining > 0u) {
                int chunk = (remaining > 258u) ? 258 : (int)remaining;
                if (chunk < 3)
                    break;
                if (!zragf_push_match_with_stats(tb, stats, len_tab, dist_tab, chunk, 1))
                    return 0;
                remaining -= (zragf_size_t)chunk;
                pos += (zragf_size_t)chunk;
            }
            while (remaining > 0u) {
                if (!zragf_push_literal_with_stats(tb, stats, src[pos]))
                    return 0;
                pos++;
                remaining--;
            }
            continue;
        }

        if (!zragf_push_literal_with_stats(tb, stats, src[pos]))
            return 0;
        pos++;
    }

    stats->litlen_freq[256]++;
    return 1;
}

static int zragf_tokenize_default_prefix(const zragf_u8 *dict,
                                         zragf_size_t dict_size,
                                         const zragf_u8 *src,
                                         zragf_size_t src_size,
                                         const zragf_matcher_config *cfg,
                                         zragf_token_buffer *tb,
                                         zragf_block_stats *stats)
{
    const zragf_u8 *data = src;
    zragf_u8 *work = NULL;
    int *chain_mem = NULL;
    int *head = NULL;
    int *prev = NULL;
    zragf_size_t total_size;
    zragf_size_t start_pos;
    zragf_size_t pos = 0u;
    zragf_deflate_match_policy mp;
    zragf_deflate_match_profile profile;
    int effective_strategy;
    int structured_fast;
    int large_buffer_fast;
    const unsigned short *len_tab;
    const unsigned short *dist_tab;

    if (!tb || !stats)
        return 0;

    zragf_tokens_reset(tb);
    zragf_blocks_reset_stats(stats);
    len_tab = zragf_blocks_len_sym_table();
    dist_tab = zragf_blocks_dist_sym_table();

    if (src_size == 0u) {
        stats->litlen_freq[256] = 1u;
        return 1;
    }

    if (dict_size > (zragf_size_t)ZRAGF_RFC_WINDOW)
        dict_size = (zragf_size_t)ZRAGF_RFC_WINDOW;
    if (src_size > (zragf_size_t)INT_MAX ||
        dict_size > (zragf_size_t)INT_MAX ||
        src_size + dict_size > (zragf_size_t)INT_MAX)
        return 0;

    if (dict_size > 0u) {
        total_size = dict_size + src_size;
        work = (zragf_u8 *)zragf_alloc_default(NULL, total_size, 1u);
        if (!work)
            return 0;
        memcpy(work, dict, dict_size);
        memcpy(work + dict_size, src, src_size);
        data = work;
        start_pos = dict_size;
    } else {
        total_size = src_size;
        start_pos = 0u;
    }

    chain_mem = (int *)zragf_alloc_default(NULL,
                                           (unsigned)(ZRAGF_RFC_HASH_SIZE + total_size),
                                           (unsigned)sizeof(int));
    if (!chain_mem) {
        zragf_free_default(NULL, work);
        return 0;
    }
    head = chain_mem;
    prev = chain_mem + ZRAGF_RFC_HASH_SIZE;
    memset(chain_mem, 0xFF, (size_t)((ZRAGF_RFC_HASH_SIZE + total_size) * sizeof(int)));

    if (dict_size > 0u) {
        for (pos = 0u; pos < dict_size; ++pos)
            zragf_insert_pos(data, total_size, pos, head, prev);
    }

    zragf_deflate_core_profile_input(dict, dict_size, src, src_size, &profile);
    effective_strategy = cfg ? cfg->strategy : ZRAGF_Z_DEFAULT_STRATEGY;
    if (effective_strategy == ZRAGF_Z_DEFAULT_STRATEGY && profile.structured_text_like)
        effective_strategy = ZRAGF_Z_FILTERED;
    zragf_deflate_core_select_policy(cfg ? cfg->level : 6,
                                     effective_strategy,
                                     cfg ? cfg->tune_set : 0,
                                     cfg ? cfg->good_length : 0,
                                     cfg ? cfg->max_lazy : 0,
                                     cfg ? cfg->nice_length : 0,
                                     cfg ? cfg->max_chain : 0,
                                     &profile, &mp);
    structured_fast = (profile.structured_text_like && src_size >= 4096u) ? 1 : 0;
    large_buffer_fast = (src_size >= 131072u && !(cfg && cfg->tune_set) &&
                         !profile.incompressible_like &&
                         !profile.text_like && !profile.structured_text_like &&
                         (profile.rle_like || (cfg ? cfg->level : 6) <= 6)) ? 1 : 0;

    pos = start_pos;
    while (pos < total_size) {
        int best_len = 0;
        int best_dist = 0;
        int next_len = 0;
        int next_dist = 0;
        int inserted_cur = 0;
        int level = cfg ? cfg->level : 6;
        int strategy = effective_strategy;

        if (level > 0) {
            zragf_find_best_match(data, total_size, pos, head, prev,
                                  mp.max_chain, mp.nice_len,
                                  &best_len, &best_dist);
        }

        if (mp.allow_lazy && best_len >= 3 && best_len < 258 &&
            best_len < mp.nice_len && pos + 1u < total_size &&
            !(large_buffer_fast && best_len >= 64)) {
            zragf_insert_pos(data, total_size, pos, head, prev);
            inserted_cur = 1;
            {
                int next_chain = mp.max_chain / 2;
                if (mp.good_len > 0 && best_len >= mp.good_len && next_chain > 8)
                    next_chain /= 2;
                if (next_chain < 4)
                    next_chain = 4;
                zragf_find_best_match(data, total_size, pos + 1u, head, prev,
                                      next_chain, mp.nice_len,
                                      &next_len, &next_dist);
            }
            if (next_len > best_len ||
                (next_len == best_len + 1 && best_len < mp.lazy_probe) ||
                (strategy == ZRAGF_Z_FILTERED && next_len >= best_len && next_dist < best_dist)) {
                best_len = 0;
                best_dist = 0;
            }
        }

        if (best_len >= 3) {
            if (!zragf_push_match_with_stats(tb, stats, len_tab, dist_tab, best_len, best_dist)) {
                zragf_free_default(NULL, chain_mem);
                zragf_free_default(NULL, work);
                return 0;
            }

            zragf_insert_match_range(data, total_size,
                                     inserted_cur ? (pos + 1u) : pos,
                                     pos + (zragf_size_t)best_len,
                                     head, prev,
                                     structured_fast,
                                     large_buffer_fast,
                                     best_len);
            pos += (zragf_size_t)best_len;
        } else {
            if (!zragf_push_literal_with_stats(tb, stats, data[pos])) {
                zragf_free_default(NULL, chain_mem);
                zragf_free_default(NULL, work);
                return 0;
            }
            if (!inserted_cur)
                zragf_insert_pos(data, total_size, pos, head, prev);
            pos++;
        }
    }

    stats->litlen_freq[256]++;
    zragf_free_default(NULL, chain_mem);
    zragf_free_default(NULL, work);
    return 1;
}

int zragf_deflate_build_tokens(const zragf_u8 *dict,
                               zragf_size_t    dict_size,
                               const zragf_u8 *src,
                               zragf_size_t    src_size,
                               const zragf_matcher_config *cfg,
                               zragf_token_buffer *tb,
                               zragf_block_stats  *stats)
{
    int strategy = cfg ? cfg->strategy : ZRAGF_Z_DEFAULT_STRATEGY;
    if (strategy == ZRAGF_Z_HUFFMAN_ONLY)
        return zragf_tokenize_literals_only(src, src_size, tb, stats);
    if (strategy == ZRAGF_Z_RLE)
        return zragf_tokenize_rle_only(src, src_size, tb, stats);
    return zragf_tokenize_default_prefix(dict, dict_size, src, src_size,
                                         cfg, tb, stats);
}
