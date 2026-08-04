#include "bvh_polysym.h"

unsigned long bvh_symbol_hash(const char *s, unsigned long n) {
    unsigned long h;
    unsigned long i;
    h = 2166136261UL;
    for (i = 0UL; i < n; i++) {
        h ^= (unsigned long)(unsigned char)s[i];
        h *= 16777619UL;
    }
    return h;
}

void bvh_polysym_reset(BVH_Context *ctx) {
    int empty;
    if (!ctx) return;
    ctx->symbol_count = 0;
    bvh_arena_reset(&ctx->arena);
    empty = bvh_symbol_intern_n(ctx, "", 0UL, NULL);
    BVH_UNUSED(empty);
}

int bvh_symbol_intern_n(BVH_Context *ctx, const char *s, unsigned long n, BVH_Error *err) {
    unsigned long h;
    int i;
    unsigned long off;
    const char *old;

    if (!ctx) return BVH_NIL;
    if (!s) {
        s = "";
        n = 0UL;
    }

    h = bvh_symbol_hash(s, n);
    for (i = 0; i < ctx->symbol_count; i++) {
        if (ctx->symbols[i].hash == h && ctx->symbols[i].len == n) {
            old = bvh_arena_at(&ctx->arena, ctx->symbols[i].off);
            if (n == 0UL || memcmp(old, s, (size_t)n) == 0) return i;
        }
    }

    if (ctx->symbol_count >= BVH_MAX_SYMBOLS) {
        bvh_error_set(err, 0, 0, "symbol table limit reached");
        return BVH_NIL;
    }
    if (!bvh_arena_put(&ctx->arena, s, n, &off)) {
        bvh_error_set(err, 0, 0, "string arena limit reached");
        return BVH_NIL;
    }

    i = ctx->symbol_count;
    ctx->symbols[i].hash = h;
    ctx->symbols[i].off = off;
    ctx->symbols[i].len = n;
    ctx->symbol_count++;
    return i;
}

int bvh_symbol_intern(BVH_Context *ctx, const char *s, BVH_Error *err) {
    return bvh_symbol_intern_n(ctx, s ? s : "", bvh_cstr_len(s), err);
}

const char *bvh_symbol_text(const BVH_Context *ctx, int sym) {
    if (!ctx) return "";
    if (sym < 0 || sym >= ctx->symbol_count) return "";
    return bvh_arena_at(&ctx->arena, ctx->symbols[sym].off);
}

void bvh_dump_symbols(const BVH_Context *ctx, FILE *out) {
    int i;
    if (!ctx) return;
    if (!out) out = stdout;
    for (i = 0; i < ctx->symbol_count; i++) {
        fprintf(out, "%04d  %lu  %s\n", i, ctx->symbols[i].len, bvh_symbol_text(ctx, i));
    }
}
