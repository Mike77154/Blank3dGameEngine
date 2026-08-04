#include "bvh_polysym.h"

void bvh_build_reset(BVH_Context *ctx) {
    if (!ctx) return;
    bvh_polysym_reset(ctx);
    ctx->program.first_item = BVH_NIL;
    ctx->program.last_item = BVH_NIL;
    ctx->program.item_count = 0;
    ctx->program.prop_count = 0;
    ctx->ir.count = 0;
    ctx->bytecode.len = 0UL;
    memset(ctx->items, 0, sizeof(ctx->items));
    memset(ctx->props, 0, sizeof(ctx->props));
}

void bvh_context_init(BVH_Context *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->source[0] = '\0';
    ctx->source_len = 0UL;
    bvh_build_reset(ctx);
}
