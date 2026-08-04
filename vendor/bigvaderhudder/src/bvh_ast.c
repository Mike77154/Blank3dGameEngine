#include "bvh_ast.h"

int bvh_ast_add_item(BVH_Context *ctx, BVH_AstKind kind, int name_sym, int line, int col, BVH_Error *err) {
    int idx;
    if (!ctx) return BVH_NIL;
    if (ctx->program.item_count >= BVH_MAX_ITEMS) {
        bvh_error_set(err, line, col, "AST item limit reached");
        return BVH_NIL;
    }
    idx = ctx->program.item_count;
    ctx->items[idx].kind = kind;
    ctx->items[idx].name_sym = name_sym;
    ctx->items[idx].first_prop = BVH_NIL;
    ctx->items[idx].last_prop = BVH_NIL;
    ctx->items[idx].first_child = BVH_NIL;
    ctx->items[idx].last_child = BVH_NIL;
    ctx->items[idx].next = BVH_NIL;
    ctx->items[idx].line = line;
    ctx->items[idx].col = col;
    ctx->program.item_count++;
    return idx;
}

int bvh_ast_add_prop(BVH_Context *ctx, int key_sym, int raw_sym, int line, int col, BVH_Error *err) {
    int idx;
    if (!ctx) return BVH_NIL;
    if (ctx->program.prop_count >= BVH_MAX_PROPS) {
        bvh_error_set(err, line, col, "AST property limit reached");
        return BVH_NIL;
    }
    idx = ctx->program.prop_count;
    ctx->props[idx].key_sym = key_sym;
    ctx->props[idx].raw_sym = raw_sym;
    bvh_value_empty(&ctx->props[idx].value);
    bvh_value_from_raw(ctx, key_sym, raw_sym, &ctx->props[idx].value);
    ctx->props[idx].line = line;
    ctx->props[idx].col = col;
    ctx->props[idx].next = BVH_NIL;
    ctx->program.prop_count++;
    return idx;
}

void bvh_ast_append_item(BVH_Context *ctx, int *first, int *last, int item_idx) {
    if (!ctx || !first || !last || item_idx == BVH_NIL) return;
    ctx->items[item_idx].next = BVH_NIL;
    if (*first == BVH_NIL) {
        *first = item_idx;
        *last = item_idx;
        return;
    }
    ctx->items[*last].next = item_idx;
    *last = item_idx;
}

void bvh_ast_append_prop(BVH_Context *ctx, int item_idx, int prop_idx) {
    BVH_AstItem *item;
    if (!ctx || item_idx == BVH_NIL || prop_idx == BVH_NIL) return;
    item = &ctx->items[item_idx];
    ctx->props[prop_idx].next = BVH_NIL;
    if (item->first_prop == BVH_NIL) {
        item->first_prop = prop_idx;
        item->last_prop = prop_idx;
        return;
    }
    ctx->props[item->last_prop].next = prop_idx;
    item->last_prop = prop_idx;
}

const char *bvh_entity_kind_name(int kind) {
    switch (kind) {
        case BVH_ENTITY_HUD: return "hud";
        case BVH_ENTITY_NODE: return "node";
        case BVH_ENTITY_GROUP: return "group";
        case BVH_ENTITY_ANIMATION: return "animation";
        default: return "entity";
    }
}

static void bvh_dump_item_rec(const BVH_Context *ctx, FILE *out, int idx, int depth) {
    int i;
    int p;
    int ch;
    char val[BVH_VALUE_TEXT_MAX];
    const BVH_AstItem *item;

    if (!ctx || idx == BVH_NIL) return;
    item = &ctx->items[idx];
    for (i = 0; i < depth; i++) fputs("  ", out);
    fprintf(out, "%s %s\n", bvh_entity_kind_name((int)item->kind), bvh_symbol_text(ctx, item->name_sym));

    p = item->first_prop;
    while (p != BVH_NIL) {
        bvh_value_format(ctx, &ctx->props[p].value, val, (unsigned long)sizeof(val));
        for (i = 0; i < depth + 1; i++) fputs("  ", out);
        fprintf(out, "prop %s = %s  <%s:%s>\n", bvh_symbol_text(ctx, ctx->props[p].key_sym), bvh_symbol_text(ctx, ctx->props[p].raw_sym), bvh_value_kind_name(ctx->props[p].value.kind), val);
        p = ctx->props[p].next;
    }

    ch = item->first_child;
    while (ch != BVH_NIL) {
        bvh_dump_item_rec(ctx, out, ch, depth + 1);
        ch = ctx->items[ch].next;
    }
}

void bvh_dump_ast(const BVH_Context *ctx, FILE *out) {
    int idx;
    if (!ctx) return;
    if (!out) out = stdout;
    idx = ctx->program.first_item;
    while (idx != BVH_NIL) {
        bvh_dump_item_rec(ctx, out, idx, 0);
        idx = ctx->items[idx].next;
    }
}
