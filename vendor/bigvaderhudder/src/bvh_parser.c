#include "bvh_parser.h"

static int bvh_tok_is_ident_lit(BVH_Token t, const char *lit) {
    if (t.type != BVH_TOK_IDENT) return 0;
    return bvh_slice_eq_lit(t.start, t.len, lit);
}

static int bvh_is_decl(BVH_Token t) {
    if (bvh_tok_is_ident_lit(t, "hud")) return 1;
    if (bvh_tok_is_ident_lit(t, "node")) return 1;
    if (bvh_tok_is_ident_lit(t, "group")) return 1;
    if (bvh_tok_is_ident_lit(t, "animation")) return 1;
    return 0;
}

static BVH_AstKind bvh_kind_from_token(BVH_Token t) {
    if (bvh_tok_is_ident_lit(t, "hud")) return BVH_AST_HUD;
    if (bvh_tok_is_ident_lit(t, "group")) return BVH_AST_GROUP;
    if (bvh_tok_is_ident_lit(t, "animation")) return BVH_AST_ANIMATION;
    return BVH_AST_NODE;
}

static int bvh_decl_allowed(BVH_AstKind owner, BVH_Token decl) {
    if (owner == BVH_AST_HUD) {
        if (bvh_tok_is_ident_lit(decl, "node")) return 1;
        if (bvh_tok_is_ident_lit(decl, "group")) return 1;
        if (bvh_tok_is_ident_lit(decl, "animation")) return 1;
        return 0;
    }
    if (owner == BVH_AST_GROUP) {
        if (bvh_tok_is_ident_lit(decl, "node")) return 1;
        if (bvh_tok_is_ident_lit(decl, "group")) return 1;
        return 0;
    }
    return 0;
}

static void bvh_skip_eol(BVH_Parser *ps) {
    BVH_Token t;
    for (;;) {
        t = bvh_lexer_peek(&ps->lx);
        if (t.type != BVH_TOK_EOL) break;
        (void)bvh_lexer_next(&ps->lx);
    }
}

static int bvh_expect_ident(BVH_Parser *ps, BVH_Token *out, const char *what) {
    BVH_Token t;
    t = bvh_lexer_next(&ps->lx);
    if (t.type != BVH_TOK_IDENT) {
        char msg[128];
        sprintf(msg, "expected %s, got %s", what ? what : "identifier", bvh_token_name(t.type));
        bvh_error_set(ps->err, t.line, t.col, msg);
        return 0;
    }
    if (out) *out = t;
    return 1;
}

static int bvh_maybe_open(BVH_Parser *ps) {
    BVH_Token t;
    bvh_skip_eol(ps);
    t = bvh_lexer_peek(&ps->lx);
    if (t.type == BVH_TOK_BLOCK_OPEN) {
        (void)bvh_lexer_next(&ps->lx);
        return 1;
    }
    return 0;
}

static int bvh_parse_item(BVH_Parser *ps, BVH_Token kw, int *out_idx);

static int bvh_parse_block_body(BVH_Parser *ps, int owner_idx) {
    BVH_AstItem *owner;
    BVH_Token t;
    owner = &ps->ctx->items[owner_idx];

    for (;;) {
        bvh_skip_eol(ps);
        t = bvh_lexer_peek(&ps->lx);

        if (t.type == BVH_TOK_EOF) return 1;

        if (t.type == BVH_TOK_BLOCK_CLOSE) {
            (void)bvh_lexer_next(&ps->lx);
            return 1;
        }

        if (bvh_is_decl(t) && !bvh_decl_allowed(owner->kind, t)) return 1;

        if (bvh_is_decl(t) && bvh_decl_allowed(owner->kind, t)) {
            int child_idx;
            t = bvh_lexer_next(&ps->lx);
            child_idx = BVH_NIL;
            if (!bvh_parse_item(ps, t, &child_idx)) return 0;
            bvh_ast_append_item(ps->ctx, &owner->first_child, &owner->last_child, child_idx);
            continue;
        }

        if (t.type == BVH_TOK_IDENT) {
            const char *raw_start;
            unsigned long raw_len;
            int key_sym;
            int raw_sym;
            int prop_idx;
            BVH_Token key;

            key = bvh_lexer_next(&ps->lx);
            bvh_lexer_capture_line_rest(&ps->lx, &raw_start, &raw_len);
            key_sym = bvh_symbol_intern_n(ps->ctx, key.start, key.len, ps->err);
            if (key_sym == BVH_NIL) return 0;
            raw_sym = bvh_symbol_intern_n(ps->ctx, raw_start, raw_len, ps->err);
            if (raw_sym == BVH_NIL) return 0;
            prop_idx = bvh_ast_add_prop(ps->ctx, key_sym, raw_sym, key.line, key.col, ps->err);
            if (prop_idx == BVH_NIL) return 0;
            bvh_ast_append_prop(ps->ctx, owner_idx, prop_idx);
            bvh_lexer_skip_line(&ps->lx);
            continue;
        }

        (void)bvh_lexer_next(&ps->lx);
        bvh_lexer_skip_line(&ps->lx);
    }
}

static int bvh_parse_item(BVH_Parser *ps, BVH_Token kw, int *out_idx) {
    BVH_Token name;
    int name_sym;
    int idx;
    int opened;

    if (!bvh_expect_ident(ps, &name, "item name")) return 0;
    name_sym = bvh_symbol_intern_n(ps->ctx, name.start, name.len, ps->err);
    if (name_sym == BVH_NIL) return 0;

    idx = bvh_ast_add_item(ps->ctx, bvh_kind_from_token(kw), name_sym, kw.line, kw.col, ps->err);
    if (idx == BVH_NIL) return 0;

    opened = bvh_maybe_open(ps);
    if (!opened) bvh_lexer_skip_line(&ps->lx);
    if (opened) {
        if (!bvh_parse_block_body(ps, idx)) return 0;
    }

    if (out_idx) *out_idx = idx;
    return 1;
}

int bvh_parse_loaded(BVH_Context *ctx, BVH_Error *err) {
    BVH_Parser ps;
    BVH_Token t;
    int item_idx;

    if (err) {
        err->line = 0;
        err->col = 0;
        err->message[0] = '\0';
    }
    if (!ctx) {
        bvh_error_set(err, 0, 0, "null context");
        return 0;
    }

    bvh_build_reset(ctx);

    ps.ctx = ctx;
    ps.err = err;
    bvh_lexer_init(&ps.lx, ctx->source);

    for (;;) {
        bvh_skip_eol(&ps);
        t = bvh_lexer_peek(&ps.lx);
        if (t.type == BVH_TOK_EOF) break;

        if (bvh_is_decl(t)) {
            t = bvh_lexer_next(&ps.lx);
            item_idx = BVH_NIL;
            if (!bvh_parse_item(&ps, t, &item_idx)) return 0;
            bvh_ast_append_item(ctx, &ctx->program.first_item, &ctx->program.last_item, item_idx);
            continue;
        }

        (void)bvh_lexer_next(&ps.lx);
        bvh_lexer_skip_line(&ps.lx);
    }

    return 1;
}
