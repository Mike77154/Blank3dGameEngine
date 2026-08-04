#include "bvh_ir.h"

int bvh_ir_emit(BVH_Context *ctx, BVH_IrCode code, int entity_kind, int name_sym, int key_sym, int raw_sym, const BVH_Value *value, int line, int col, BVH_Error *err) {
    int idx;
    if (!ctx) return 0;
    if (ctx->ir.count >= BVH_MAX_IR_OPS) {
        bvh_error_set(err, line, col, "IR op limit reached");
        return 0;
    }
    idx = ctx->ir.count;
    ctx->ir.ops[idx].code = code;
    ctx->ir.ops[idx].entity_kind = entity_kind;
    ctx->ir.ops[idx].name_sym = name_sym;
    ctx->ir.ops[idx].key_sym = key_sym;
    ctx->ir.ops[idx].raw_sym = raw_sym;
    if (value) ctx->ir.ops[idx].value = *value;
    else bvh_value_empty(&ctx->ir.ops[idx].value);
    ctx->ir.ops[idx].line = line;
    ctx->ir.ops[idx].col = col;
    ctx->ir.count++;
    return 1;
}

static int bvh_ast_kind_to_entity(BVH_AstKind kind) {
    switch (kind) {
        case BVH_AST_HUD: return BVH_ENTITY_HUD;
        case BVH_AST_NODE: return BVH_ENTITY_NODE;
        case BVH_AST_GROUP: return BVH_ENTITY_GROUP;
        case BVH_AST_ANIMATION: return BVH_ENTITY_ANIMATION;
        default: return BVH_ENTITY_NODE;
    }
}

static int bvh_build_ir_item(BVH_Context *ctx, int idx, BVH_Error *err) {
    int prop;
    int ch;
    int entity;
    const BVH_AstItem *item;

    if (!ctx || idx == BVH_NIL) return 1;
    item = &ctx->items[idx];
    entity = bvh_ast_kind_to_entity(item->kind);

    if (!bvh_ir_emit(ctx, BVH_IR_BEGIN, entity, item->name_sym, BVH_NIL, BVH_NIL, NULL, item->line, item->col, err)) return 0;

    prop = item->first_prop;
    while (prop != BVH_NIL) {
        if (!bvh_ir_emit(ctx, BVH_IR_PROP, entity, BVH_NIL, ctx->props[prop].key_sym, ctx->props[prop].raw_sym, &ctx->props[prop].value, ctx->props[prop].line, ctx->props[prop].col, err)) return 0;
        prop = ctx->props[prop].next;
    }

    ch = item->first_child;
    while (ch != BVH_NIL) {
        if (!bvh_build_ir_item(ctx, ch, err)) return 0;
        ch = ctx->items[ch].next;
    }

    if (!bvh_ir_emit(ctx, BVH_IR_END, entity, BVH_NIL, BVH_NIL, BVH_NIL, NULL, item->line, item->col, err)) return 0;
    return 1;
}

int bvh_build_ir(BVH_Context *ctx, BVH_Error *err) {
    int idx;
    if (err) {
        err->line = 0;
        err->col = 0;
        err->message[0] = '\0';
    }
    if (!ctx) {
        bvh_error_set(err, 0, 0, "null context");
        return 0;
    }
    ctx->ir.count = 0;
    idx = ctx->program.first_item;
    while (idx != BVH_NIL) {
        if (!bvh_build_ir_item(ctx, idx, err)) return 0;
        idx = ctx->items[idx].next;
    }
    return 1;
}

void bvh_dump_ir(const BVH_Context *ctx, FILE *out) {
    int i;
    char val[BVH_VALUE_TEXT_MAX];
    const BVH_IrOp *op;

    if (!ctx) return;
    if (!out) out = stdout;
    for (i = 0; i < ctx->ir.count; i++) {
        op = &ctx->ir.ops[i];
        if (op->code == BVH_IR_BEGIN) {
            fprintf(out, "%04d BEGIN %-9s %s\n", i, bvh_entity_kind_name(op->entity_kind), bvh_symbol_text(ctx, op->name_sym));
        } else if (op->code == BVH_IR_END) {
            fprintf(out, "%04d END   %-9s\n", i, bvh_entity_kind_name(op->entity_kind));
        } else if (op->code == BVH_IR_PROP) {
            bvh_value_format(ctx, &op->value, val, (unsigned long)sizeof(val));
            fprintf(out, "%04d PROP  %-12s raw='%s' kind=%s value='%s'\n", i, bvh_symbol_text(ctx, op->key_sym), bvh_symbol_text(ctx, op->raw_sym), bvh_value_kind_name(op->value.kind), val);
        }
    }
}
