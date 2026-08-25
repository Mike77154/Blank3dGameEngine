#include "deflate_plan.h"

typedef struct {
    unsigned unique;
    unsigned maxfreq;
    unsigned repeated;
    unsigned zeroes;
    unsigned asciiish;
    unsigned score;
} zragf_plan_window_score;

typedef struct {
    zragf_size_t offset;
    unsigned score;
} zragf_plan_candidate;

#define ZRAGF_PLAN_WINDOW      2048u
#define ZRAGF_PLAN_ALIGN       1024u
#define ZRAGF_PLAN_MIN_MARGIN  4096u
#define ZRAGF_PLAN_MIN_SPACING 4096u
#define ZRAGF_PLAN_MAX_CANDIDATES 24

static zragf_size_t zragf_align_down(zragf_size_t v, zragf_size_t align)
{
    if (align == 0u)
        return v;
    return v - (v % align);
}

static unsigned zragf_abs_diff_u(unsigned a, unsigned b)
{
    return (a > b) ? (a - b) : (b - a);
}

static zragf_size_t zragf_abs_diff_sz(zragf_size_t a, zragf_size_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

static void zragf_plan_score_window(const zragf_u8 *src,
                                    zragf_size_t len,
                                    zragf_plan_window_score *out)
{
    unsigned hist[256];
    zragf_size_t i;
    unsigned unique = 0u;
    unsigned maxfreq = 0u;
    unsigned repeated = 0u;
    unsigned zeroes = 0u;
    unsigned asciiish = 0u;

    if (!out)
        return;

    memset(hist, 0, sizeof(hist));
    if (!src || len == 0u) {
        memset(out, 0, sizeof(*out));
        return;
    }

    for (i = 0u; i < len; ++i) {
        unsigned idx = (unsigned)src[i];
        hist[idx]++;
        if (hist[idx] == 1u)
            unique++;
        if (hist[idx] > maxfreq)
            maxfreq = hist[idx];
        if (src[i] == 0u)
            zeroes++;
        if ((src[i] >= 32u && src[i] <= 126u) || src[i] == 9u || src[i] == 10u || src[i] == 13u)
            asciiish++;
        if (i > 0u && src[i] == src[i - 1u])
            repeated += 3u;
        if (i > 1u && src[i] == src[i - 2u])
            repeated += 2u;
        if (i > 3u && src[i] == src[i - 4u])
            repeated += 1u;
    }

    out->unique = unique;
    out->maxfreq = maxfreq;
    out->repeated = repeated;
    out->zeroes = zeroes;
    out->asciiish = asciiish;
    out->score = repeated * 10u
               + maxfreq * 8u
               + zeroes * 4u
               + asciiish * 2u
               + (unsigned)((len > unique) ? ((len - unique) * 3u) : 0u);
}

static int zragf_plan_candidate_valid(zragf_size_t offset, zragf_size_t src_size)
{
    return (offset >= ZRAGF_PLAN_MIN_MARGIN &&
            offset + ZRAGF_PLAN_MIN_MARGIN <= src_size) ? 1 : 0;
}

static void zragf_plan_insert_candidate(zragf_plan_candidate *cands,
                                        int *count,
                                        zragf_size_t offset,
                                        unsigned score,
                                        zragf_size_t src_size)
{
    int i;
    int worst;

    if (!cands || !count)
        return;

    offset = zragf_align_down(offset, ZRAGF_PLAN_ALIGN);
    if (!zragf_plan_candidate_valid(offset, src_size))
        return;

    for (i = 0; i < *count; ++i) {
        if (cands[i].offset == offset) {
            if (score > cands[i].score)
                cands[i].score = score;
            return;
        }
    }

    if (*count < ZRAGF_PLAN_MAX_CANDIDATES) {
        cands[*count].offset = offset;
        cands[*count].score = score;
        (*count)++;
        return;
    }

    worst = 0;
    for (i = 1; i < *count; ++i) {
        if (cands[i].score < cands[worst].score)
            worst = i;
    }
    if (score > cands[worst].score) {
        cands[worst].offset = offset;
        cands[worst].score = score;
    }
}

static void zragf_plan_select(zragf_deflate_split_plan *plan,
                              const zragf_plan_candidate *cands,
                              int cand_count)
{
    int used[ZRAGF_PLAN_MAX_CANDIDATES];
    int i;

    if (!plan)
        return;

    memset(used, 0, sizeof(used));
    while (plan->count < ZRAGF_DEFLATE_PLAN_MAX_SPLITS) {
        int best = -1;
        for (i = 0; i < cand_count; ++i) {
            int ok = 1;
            int j;
            if (used[i])
                continue;
            for (j = 0; j < plan->count; ++j) {
                if (zragf_abs_diff_sz(plan->offsets[j], cands[i].offset) < ZRAGF_PLAN_MIN_SPACING) {
                    ok = 0;
                    break;
                }
            }
            if (!ok)
                continue;
            if (best < 0 || cands[i].score > cands[best].score)
                best = i;
        }
        if (best < 0)
            break;
        plan->offsets[plan->count++] = cands[best].offset;
        used[best] = 1;
    }
}

static void zragf_plan_sort(zragf_deflate_split_plan *plan)
{
    int i;
    int j;
    if (!plan)
        return;
    for (i = 0; i < plan->count; ++i) {
        for (j = i + 1; j < plan->count; ++j) {
            if (plan->offsets[j] < plan->offsets[i]) {
                zragf_size_t t = plan->offsets[i];
                plan->offsets[i] = plan->offsets[j];
                plan->offsets[j] = t;
            }
        }
    }
}

void zragf_deflate_plan_reset(zragf_deflate_split_plan *plan)
{
    if (!plan)
        return;
    memset(plan, 0, sizeof(*plan));
}

int zragf_deflate_plan_build(const zragf_u8 *src,
                             zragf_size_t src_size,
                             zragf_deflate_split_plan *plan)
{
    zragf_plan_candidate cands[ZRAGF_PLAN_MAX_CANDIDATES];
    zragf_plan_window_score prev;
    int cand_count = 0;
    int have_prev = 0;
    zragf_size_t pos;

    if (!plan)
        return 0;

    zragf_deflate_plan_reset(plan);
    if (!src || src_size < 16384u)
        return 1;

    memset(cands, 0, sizeof(cands));

    for (pos = 0u; pos < src_size; pos += ZRAGF_PLAN_WINDOW) {
        zragf_plan_window_score cur;
        zragf_size_t len = src_size - pos;
        if (len > ZRAGF_PLAN_WINDOW)
            len = ZRAGF_PLAN_WINDOW;
        zragf_plan_score_window(src + pos, len, &cur);

        if (have_prev) {
            unsigned dscore = zragf_abs_diff_u(prev.score, cur.score);
            unsigned dunique = zragf_abs_diff_u(prev.unique, cur.unique);
            unsigned dmaxfreq = zragf_abs_diff_u(prev.maxfreq, cur.maxfreq);
            unsigned dzero = zragf_abs_diff_u(prev.zeroes, cur.zeroes);
            unsigned dascii = zragf_abs_diff_u(prev.asciiish, cur.asciiish);
            unsigned transition = dscore * 2u
                                + dunique * 24u
                                + dmaxfreq * 10u
                                + dzero * 8u
                                + dascii * 6u;
            zragf_size_t boundary = zragf_align_down(pos, ZRAGF_PLAN_ALIGN);
            if (transition >= 1200u)
                zragf_plan_insert_candidate(cands, &cand_count, boundary, transition, src_size);
        }

        prev = cur;
        have_prev = 1;
    }

    if (cand_count == 0 && src_size >= 32768u) {
        zragf_size_t mid = zragf_align_down(src_size / 2u, ZRAGF_PLAN_ALIGN);
        zragf_plan_insert_candidate(cands, &cand_count, mid, 1u, src_size);
    }

    zragf_plan_select(plan, cands, cand_count);
    zragf_plan_sort(plan);
    return 1;
}
