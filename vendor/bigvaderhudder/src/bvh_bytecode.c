#include "bvh_bytecode.h"

int bvh_bytecode_reset(BVH_Bytecode *bc) {
    if (!bc) return 0;
    bc->len = 0UL;
    return 1;
}

int bvh_bytecode_emit_bytes(BVH_Bytecode *bc, const unsigned char *p, unsigned long n) {
    if (!bc) return 0;
    if (bc->len + n > BVH_MAX_BYTECODE_SIZE) return 0;
    if (n && p) memcpy(bc->data + bc->len, p, (size_t)n);
    bc->len += n;
    return 1;
}

int bvh_bytecode_emit_u8(BVH_Bytecode *bc, int v) {
    unsigned char b;
    b = (unsigned char)(v & 255);
    return bvh_bytecode_emit_bytes(bc, &b, 1UL);
}

int bvh_bytecode_emit_u16(BVH_Bytecode *bc, unsigned long v) {
    unsigned char b[2];
    b[0] = (unsigned char)(v & 255UL);
    b[1] = (unsigned char)((v >> 8) & 255UL);
    return bvh_bytecode_emit_bytes(bc, b, 2UL);
}

int bvh_bytecode_emit_u32(BVH_Bytecode *bc, unsigned long v) {
    unsigned char b[4];
    b[0] = (unsigned char)(v & 255UL);
    b[1] = (unsigned char)((v >> 8) & 255UL);
    b[2] = (unsigned char)((v >> 16) & 255UL);
    b[3] = (unsigned char)((v >> 24) & 255UL);
    return bvh_bytecode_emit_bytes(bc, b, 4UL);
}

int bvh_bytecode_emit_s32(BVH_Bytecode *bc, long v) {
    unsigned long u;
    u = (unsigned long)v & 0xFFFFFFFFUL;
    return bvh_bytecode_emit_u32(bc, u);
}

int bvh_bytecode_emit_symbol(BVH_Context *ctx, BVH_Bytecode *bc, int sym, BVH_Error *err) {
    const char *s;
    unsigned long n;
    unsigned char nul;
    s = bvh_symbol_text(ctx, sym);
    n = bvh_cstr_len(s);
    if (n > 65534UL) {
        bvh_error_set(err, 0, 0, "bytecode string too long");
        return 0;
    }
    nul = 0U;
    if (!bvh_bytecode_emit_u16(bc, n)) return 0;
    if (!bvh_bytecode_emit_bytes(bc, (const unsigned char *)s, n)) return 0;
    if (!bvh_bytecode_emit_bytes(bc, &nul, 1UL)) return 0;
    return 1;
}

static int bvh_emit_header(BVH_Bytecode *bc) {
    unsigned char h[8];
    h[0] = (unsigned char)'B';
    h[1] = (unsigned char)'V';
    h[2] = (unsigned char)'H';
    h[3] = (unsigned char)'B';
    h[4] = (unsigned char)BVH_VERSION_MAJOR;
    h[5] = (unsigned char)BVH_VERSION_MINOR;
    h[6] = 0U;
    h[7] = 0U;
    return bvh_bytecode_emit_bytes(bc, h, 8UL);
}

static int bvh_emit_value(BVH_Bytecode *bc, const BVH_Value *v) {
    if (!bvh_bytecode_emit_u8(bc, (int)v->kind)) return 0;
    switch (v->kind) {
        case BVH_VALUE_INT:
        case BVH_VALUE_FIXED:
            return bvh_bytecode_emit_s32(bc, v->a);
        case BVH_VALUE_VEC2I:
        case BVH_VALUE_RANGE:
            if (!bvh_bytecode_emit_s32(bc, v->a)) return 0;
            return bvh_bytecode_emit_s32(bc, v->b);
        case BVH_VALUE_COLOR:
            return bvh_bytecode_emit_u32(bc, v->u);
        case BVH_VALUE_ANCHOR_POS:
            if (!bvh_bytecode_emit_u8(bc, (int)v->a)) return 0;
            if (!bvh_bytecode_emit_s32(bc, v->b)) return 0;
            return bvh_bytecode_emit_s32(bc, v->c);
        default:
            return 1;
    }
}

int bvh_compile_program(BVH_Context *ctx, BVH_Error *err) {
    int i;
    const BVH_IrOp *op;

    if (err) {
        err->line = 0;
        err->col = 0;
        err->message[0] = '\0';
    }
    if (!ctx) {
        bvh_error_set(err, 0, 0, "null context");
        return 0;
    }

    if (!bvh_build_ir(ctx, err)) return 0;
    bvh_bytecode_reset(&ctx->bytecode);
    if (!bvh_emit_header(&ctx->bytecode)) {
        bvh_error_set(err, 0, 0, "bytecode buffer limit reached");
        return 0;
    }

    for (i = 0; i < ctx->ir.count; i++) {
        op = &ctx->ir.ops[i];
        if (op->code == BVH_IR_BEGIN) {
            if (!bvh_bytecode_emit_u8(&ctx->bytecode, BVH_OP_BEGIN)) goto limit;
            if (!bvh_bytecode_emit_u8(&ctx->bytecode, op->entity_kind)) goto limit;
            if (!bvh_bytecode_emit_symbol(ctx, &ctx->bytecode, op->name_sym, err)) return 0;
        } else if (op->code == BVH_IR_END) {
            if (!bvh_bytecode_emit_u8(&ctx->bytecode, BVH_OP_END)) goto limit;
            if (!bvh_bytecode_emit_u8(&ctx->bytecode, op->entity_kind)) goto limit;
        } else if (op->code == BVH_IR_PROP) {
            if (!bvh_bytecode_emit_u8(&ctx->bytecode, BVH_OP_PROP)) goto limit;
            if (!bvh_bytecode_emit_symbol(ctx, &ctx->bytecode, op->key_sym, err)) return 0;
            if (!bvh_bytecode_emit_symbol(ctx, &ctx->bytecode, op->raw_sym, err)) return 0;
            if (!bvh_emit_value(&ctx->bytecode, &op->value)) goto limit;
        }
    }
    return 1;

limit:
    bvh_error_set(err, 0, 0, "bytecode buffer limit reached");
    return 0;
}
