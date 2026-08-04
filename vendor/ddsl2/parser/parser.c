#include "parser/parser.h"

#include "arena/arena.h"

#include <string.h>
#include <ctype.h>

typedef struct ddsl_parser {
    const ddsl_token *toks;
    int count;
    int pos;
    ddsl_error *err;
    ddsl_arena *arena;
} ddsl_parser;

static const ddsl_token *p_peek(ddsl_parser *p) {
    if (!p) return NULL;
    if (p->pos < 0) return NULL;
    if (p->pos >= p->count) return &p->toks[p->count - 1];
    return &p->toks[p->pos];
}

static const ddsl_token *p_prev(ddsl_parser *p) {
    if (!p) return NULL;
    if (p->pos <= 0) return &p->toks[0];
    return &p->toks[p->pos - 1];
}

static int p_is_end(ddsl_parser *p) {
    const ddsl_token *t = p_peek(p);
    return (t && t->kind == DDSL_TOK_EOF) ? 1 : 0;
}

static const ddsl_token *p_advance(ddsl_parser *p) {
    if (!p_is_end(p)) p->pos++;
    return p_prev(p);
}

static int p_check(ddsl_parser *p, ddsl_tok_kind k) {
    const ddsl_token *t = p_peek(p);
    if (!t) return 0;
    return (t->kind == k) ? 1 : 0;
}

static int p_match(ddsl_parser *p, ddsl_tok_kind k) {
    if (p_check(p, k)) {
        p_advance(p);
        return 1;
    }
    return 0;
}

static void p_skip_newlines(ddsl_parser *p) {
    while (p_check(p, DDSL_TOK_NEWLINE)) p_advance(p);
}

static void p_error_at(ddsl_parser *p, const ddsl_token *t, const char *msg) {
    if (!p || !p->err) return;
    if (!t) t = p_peek(p);
    ddsl_error_set(p->err,
                   t ? t->line : 0,
                   t ? t->col : 0,
                   t ? t->offset : 0,
                   msg ? msg : "parse error");
}

static int p_expect(ddsl_parser *p, ddsl_tok_kind k, const char *msg) {
    if (p_check(p, k)) { p_advance(p); return 1; }
    p_error_at(p, p_peek(p), msg);
    return 0;
}

/* ======================== AST alloc helpers ======================== */

static ddsl_program *new_program(ddsl_parser *ps) {
    ddsl_program *p;
    if (!ps || !ps->arena) return NULL;
    p = (ddsl_program *)ddsl_arena_alloc_zero(ps->arena, sizeof(ddsl_program), sizeof(void *));
    return p;
}

static ddsl_stmt *new_stmt(ddsl_parser *ps, ddsl_stmt_kind kind, ddsl_token at) {
    ddsl_stmt *s;
    if (!ps || !ps->arena) return NULL;
    s = (ddsl_stmt *)ddsl_arena_alloc_zero(ps->arena, sizeof(ddsl_stmt), sizeof(void *));
    if (!s) return NULL;
    s->kind = kind;
    s->at = at;
    s->next = NULL;
    return s;
}

static ddsl_action *new_action(ddsl_parser *ps, ddsl_action_kind kind, ddsl_token at) {
    ddsl_action *a;
    if (!ps || !ps->arena) return NULL;
    a = (ddsl_action *)ddsl_arena_alloc_zero(ps->arena, sizeof(ddsl_action), sizeof(void *));
    if (!a) return NULL;
    a->kind = kind;
    a->at = at;
    a->next = NULL;
    return a;
}

static ddsl_if_clause *new_clause(ddsl_parser *ps, ddsl_token at) {
    ddsl_if_clause *c;
    if (!ps || !ps->arena) return NULL;
    c = (ddsl_if_clause *)ddsl_arena_alloc_zero(ps->arena, sizeof(ddsl_if_clause), sizeof(void *));
    if (!c) return NULL;
    c->at = at;
    c->next = NULL;
    return c;
}

static ddsl_expr *new_expr(ddsl_parser *ps, ddsl_expr_kind kind, ddsl_token at) {
    ddsl_expr *e;
    if (!ps || !ps->arena) return NULL;
    e = (ddsl_expr *)ddsl_arena_alloc_zero(ps->arena, sizeof(ddsl_expr), sizeof(void *));
    if (!e) return NULL;
    e->kind = kind;
    e->at = at;
    return e;
}

static void prog_push(ddsl_program *p, ddsl_stmt *s) {
    if (!p || !s) return;
    if (!p->first) {
        p->first = s;
        p->last = s;
        return;
    }
    p->last->next = s;
    p->last = s;
}

/* ======================== Expression parsing (Pratt) ======================== */

static int is_terminator(ddsl_tok_kind k) {
    return (k == DDSL_TOK_NEWLINE || k == DDSL_TOK_EOF || k == DDSL_TOK_THEN ||
            k == DDSL_TOK_COMMA || k == DDSL_TOK_ELIF || k == DDSL_TOK_ELSE) ? 1 : 0;
}

static int infix_prec(ddsl_tok_kind k, ddsl_op *out_op) {
    if (out_op) *out_op = DDSL_OP_PLUS;

    if (k == DDSL_TOK_OR) { if (out_op) *out_op = DDSL_OP_OR; return 1; }
    if (k == DDSL_TOK_AND) { if (out_op) *out_op = DDSL_OP_AND; return 2; }

    if (k == DDSL_TOK_EQ || k == DDSL_TOK_EQEQ) { if (out_op) *out_op = DDSL_OP_EQ; return 3; }
    if (k == DDSL_TOK_NEQ) { if (out_op) *out_op = DDSL_OP_NEQ; return 3; }
    if (k == DDSL_TOK_LT) { if (out_op) *out_op = DDSL_OP_LT; return 3; }
    if (k == DDSL_TOK_LTE) { if (out_op) *out_op = DDSL_OP_LTE; return 3; }
    if (k == DDSL_TOK_GT) { if (out_op) *out_op = DDSL_OP_GT; return 3; }
    if (k == DDSL_TOK_GTE) { if (out_op) *out_op = DDSL_OP_GTE; return 3; }

    if (k == DDSL_TOK_PLUS) { if (out_op) *out_op = DDSL_OP_PLUS; return 4; }
    if (k == DDSL_TOK_MINUS) { if (out_op) *out_op = DDSL_OP_MINUS; return 4; }

    if (k == DDSL_TOK_STAR) { if (out_op) *out_op = DDSL_OP_MUL; return 5; }
    if (k == DDSL_TOK_SLASH) { if (out_op) *out_op = DDSL_OP_DIV; return 5; }

    return 0;
}

static ddsl_expr *parse_expr_bp(ddsl_parser *p, int min_prec);

static ddsl_expr *parse_primary(ddsl_parser *p) {
    const ddsl_token *t;
    ddsl_expr *e;

    if (!p) return NULL;
    t = p_peek(p);
    if (!t) return NULL;

    if (t->kind == DDSL_TOK_NUMBER) {
        ddsl_fixed v;
        if (!ddsl_fixed_parse_sv(t->lexeme, &v)) {
            p_error_at(p, t, "número inválido o fuera de rango");
            return NULL;
        }
        p_advance(p);
        e = new_expr(p, DDSL_EXPR_NUMBER, *t);
        if (!e) return NULL;
        e->as.number = v;
        return e;
    }

    if (t->kind == DDSL_TOK_STRING) {
        p_advance(p);
        e = new_expr(p, DDSL_EXPR_STRING, *t);
        if (!e) return NULL;
        e->as.string = t->lexeme;
        return e;
    }

    if (t->kind == DDSL_TOK_IDENT) {
        p_advance(p);

        /* true/false como literales booleanos "ligeros":
         * los representamos como IDENT y el runtime los interpreta.
         */
        e = new_expr(p, DDSL_EXPR_IDENT, *t);
        if (!e) return NULL;
        e->as.ident = t->lexeme;
        return e;
    }

    if (t->kind == DDSL_TOK_LPAREN) {
        p_advance(p);
        e = parse_expr_bp(p, 0);
        if (!e) return NULL;
        if (!p_expect(p, DDSL_TOK_RPAREN, "se esperaba ')'")) {
            ddsl_ast_discard_expr(e);
            return NULL;
        }
        return e;
    }

    if (t->kind == DDSL_TOK_MINUS) {
        ddsl_expr *rhs;
        p_advance(p);
        rhs = parse_expr_bp(p, 6);
        if (!rhs) {
            p_error_at(p, t, "se esperaba expresión después de '-'" );
            return NULL;
        }
        e = new_expr(p, DDSL_EXPR_UNARY, *t);
        if (!e) {
            ddsl_ast_discard_expr(rhs);
            return NULL;
        }
        e->as.unary.op = DDSL_OP_MINUS;
        e->as.unary.rhs = rhs;
        return e;
    }

    if (is_terminator(t->kind)) {
        p_error_at(p, t, "expresión vacía");
        return NULL;
    }

    p_error_at(p, t, "token inesperado en expresión");
    return NULL;
}

static ddsl_expr *parse_expr_bp(ddsl_parser *p, int min_prec) {
    ddsl_expr *lhs;

    lhs = parse_primary(p);
    if (!lhs) return NULL;

    while (!p_is_end(p)) {
        const ddsl_token *op_tok;
        ddsl_op op;
        int prec;
        ddsl_expr *rhs;
        ddsl_expr *bin;

        op_tok = p_peek(p);
        if (!op_tok) break;
        if (is_terminator(op_tok->kind) || op_tok->kind == DDSL_TOK_RPAREN) break;

        prec = infix_prec(op_tok->kind, &op);
        if (prec == 0 || prec < min_prec) break;

        /* left-associative => RHS parses with prec+1 */
        p_advance(p);
        rhs = parse_expr_bp(p, prec + 1);
        if (!rhs) {
            ddsl_ast_discard_expr(lhs);
            return NULL;
        }

        bin = new_expr(p, DDSL_EXPR_BINARY, *op_tok);
        if (!bin) {
            ddsl_ast_discard_expr(lhs);
            ddsl_ast_discard_expr(rhs);
            return NULL;
        }
        bin->as.binary.op = op;
        bin->as.binary.lhs = lhs;
        bin->as.binary.rhs = rhs;
        lhs = bin;
    }

    return lhs;
}

static ddsl_expr *parse_expr(ddsl_parser *p) {
    return parse_expr_bp(p, 0);
}

static int p_require_line_end(ddsl_parser *p, int allow_clause_tokens) {
    ddsl_tok_kind k;
    const ddsl_token *t;

    t = p_peek(p);
    if (!t) return 1;
    k = t->kind;
    if (k == DDSL_TOK_NEWLINE || k == DDSL_TOK_EOF) return 1;
    if (allow_clause_tokens && (k == DDSL_TOK_ELIF || k == DDSL_TOK_ELSE)) return 1;

    p_error_at(p, t, "se esperaba ',' o fin de línea después de la acción");
    return 0;
}


/* ======================== Actions parsing ======================== */

static ddsl_action *parse_action_list(ddsl_parser *p) {
    ddsl_action *first;
    ddsl_action *last;

    first = NULL;
    last = NULL;

    while (1) {
        const ddsl_token *name_tok;
        ddsl_action *a;

        if (p_check(p, DDSL_TOK_NEWLINE) || p_check(p, DDSL_TOK_EOF) ||
            p_check(p, DDSL_TOK_ELIF) || p_check(p, DDSL_TOK_ELSE)) {
            break;
        }

        name_tok = p_peek(p);
        if (!name_tok || name_tok->kind != DDSL_TOK_IDENT) {
            p_error_at(p, name_tok, "se esperaba IDENT en acción");
            ddsl_ast_discard_actions(first);
            return NULL;
        }
        p_advance(p);

        /* flag o asignación */
        if (p_match(p, DDSL_TOK_EQ)) {
            ddsl_expr *value;
            a = new_action(p, DDSL_ACTION_SET, *name_tok);
            if (!a) { ddsl_ast_discard_actions(first); return NULL; }
            a->name = name_tok->lexeme;

            /* valor hasta COMMA/NEWLINE/ELIF/ELSE/EOF */
            value = parse_expr(p);
            if (!value) { ddsl_ast_discard_actions(first); return NULL; }
            a->value = value;
        } else {
            a = new_action(p, DDSL_ACTION_FLAG, *name_tok);
            if (!a) { ddsl_ast_discard_actions(first); return NULL; }
            a->name = name_tok->lexeme;
            a->value = NULL;
        }

        if (!first) first = a;
        else last->next = a;
        last = a;

        if (p_match(p, DDSL_TOK_COMMA)) {
            continue;
        }

        /* si no hay coma, termina la lista */
        break;
    }

    return first;
}

/* ======================== If parsing ======================== */

static ddsl_if_clause *parse_if_clause(ddsl_parser *p, int is_else, ddsl_token at) {
    ddsl_if_clause *c;

    c = new_clause(p, at);
    if (!c) return NULL;

    c->is_else = is_else;
    c->cond = NULL;
    c->actions = NULL;

    if (!is_else) {
        ddsl_expr *cond;
        cond = parse_expr(p);
        if (!cond) { return NULL; }
        c->cond = cond;
    }

    if (p_match(p, DDSL_TOK_THEN)) {
        /* acciones opcionales */
        c->actions = parse_action_list(p);
        if (p->err && p->err->message[0] != '\0') {
            ddsl_ast_discard_clauses(c);
            return NULL;
        }
        if (!p_require_line_end(p, 1)) {
            ddsl_ast_discard_clauses(c);
            return NULL;
        }
    } else {
        /* sin THEN: cláusula sin acciones (solo evalúa condición). */
        c->actions = NULL;
    }

    return c;
}

static ddsl_stmt *parse_if_stmt(ddsl_parser *p) {
    ddsl_stmt *s;
    ddsl_if_clause *first;
    ddsl_if_clause *last;
    ddsl_token start_tok;

    first = NULL;
    last = NULL;

    start_tok = *p_peek(p);

    if (p_match(p, DDSL_TOK_QMARK)) {
        /* shorthand: ? cond then ... */
        ddsl_if_clause *c0;
        c0 = parse_if_clause(p, 0, start_tok);
        if (!c0) return NULL;
        first = c0;
        last = c0;
    } else {
        /* IF */
        if (!p_expect(p, DDSL_TOK_IF, "se esperaba 'if'") ) return NULL;
        {
            ddsl_if_clause *c0;
            c0 = parse_if_clause(p, 0, start_tok);
            if (!c0) return NULL;
            first = c0;
            last = c0;
        }
    }

    /* Consumimos NEWLINEs opcionales y luego ELIF/ELSE (inline o multi-línea). */
    while (1) {
        p_skip_newlines(p);

        if (p_check(p, DDSL_TOK_ELIF)) {
            ddsl_token t = *p_peek(p);
            ddsl_if_clause *c;
            p_advance(p);
            c = parse_if_clause(p, 0, t);
            if (!c) { ddsl_ast_discard_clauses(first); return NULL; }
            last->next = c;
            last = c;
            continue;
        }

        if (p_check(p, DDSL_TOK_ELSE)) {
            ddsl_token t = *p_peek(p);
            ddsl_if_clause *c;
            p_advance(p);

            /* else [then] acciones */
            c = new_clause(p, t);
            if (!c) { ddsl_ast_discard_clauses(first); return NULL; }
            c->is_else = 1;
            c->cond = NULL;
            c->actions = NULL;

            if (p_match(p, DDSL_TOK_THEN)) {
                c->actions = parse_action_list(p);
                if (p->err && p->err->message[0] != '\0') {
                    ddsl_ast_discard_clauses(first);
                    ddsl_ast_discard_clauses(c);
                    return NULL;
                }
                if (!p_require_line_end(p, 0)) {
                    ddsl_ast_discard_clauses(first);
                    ddsl_ast_discard_clauses(c);
                    return NULL;
                }
            } else {
                c->actions = parse_action_list(p);
                if (p->err && p->err->message[0] != '\0') {
                    ddsl_ast_discard_clauses(first);
                    ddsl_ast_discard_clauses(c);
                    return NULL;
                }
                if (!p_require_line_end(p, 0)) {
                    ddsl_ast_discard_clauses(first);
                    ddsl_ast_discard_clauses(c);
                    return NULL;
                }
            }

            last->next = c;
            last = c;
            break;
        }

        break;
    }

    s = new_stmt(p, DDSL_STMT_IF, start_tok);
    if (!s) {
        ddsl_ast_discard_clauses(first);
        return NULL;
    }
    s->as.ifs.clauses = first;

    /* consume trailing newlines */
    p_skip_newlines(p);
    return s;
}

/* ======================== Statement parsing ======================== */

static int line_has_assign(ddsl_parser *p) {
    int i;
    if (!p) return 0;
    i = p->pos;
    while (i < p->count) {
        ddsl_tok_kind k = p->toks[i].kind;
        if (k == DDSL_TOK_NEWLINE || k == DDSL_TOK_EOF) break;
        if (k == DDSL_TOK_EQ) return 1;
        i++;
    }
    return 0;
}

static ddsl_stmt *parse_actions_stmt(ddsl_parser *p) {
    ddsl_stmt *s;
    ddsl_action *acts;
    ddsl_token at;

    at = *p_peek(p);
    acts = parse_action_list(p);
    if (!acts && p->err && p->err->message[0] != '\0') return NULL;
    if (!p_require_line_end(p, 0)) {
        ddsl_ast_discard_actions(acts);
        return NULL;
    }

    s = new_stmt(p, DDSL_STMT_ACTIONS, at);
    if (!s) {
        ddsl_ast_discard_actions(acts);
        return NULL;
    }
    s->as.actions.actions = acts;

    /* consume rest of line */
    if (p_check(p, DDSL_TOK_NEWLINE)) p_advance(p);
    p_skip_newlines(p);
    return s;
}

static ddsl_stmt *parse_expr_stmt(ddsl_parser *p) {
    ddsl_stmt *s;
    ddsl_expr *e;
    ddsl_token at;

    at = *p_peek(p);
    e = parse_expr(p);
    if (!e) return NULL;
    if (!p_check(p, DDSL_TOK_NEWLINE) && !p_check(p, DDSL_TOK_EOF)) {
        p_error_at(p, p_peek(p), "token inesperado después de la expresión");
        ddsl_ast_discard_expr(e);
        return NULL;
    }

    s = new_stmt(p, DDSL_STMT_EXPR, at);
    if (!s) {
        ddsl_ast_discard_expr(e);
        return NULL;
    }
    s->as.expr.expr = e;

    if (p_check(p, DDSL_TOK_NEWLINE)) p_advance(p);
    p_skip_newlines(p);
    return s;
}

static ddsl_stmt *parse_stmt(ddsl_parser *p) {
    if (!p) return NULL;

    if (p_check(p, DDSL_TOK_ELIF) || p_check(p, DDSL_TOK_ELSE)) {
        p_error_at(p, p_peek(p), "'elif/else' sin un 'if' previo");
        return NULL;
    }

    if (p_check(p, DDSL_TOK_IF) || p_check(p, DDSL_TOK_QMARK)) {
        return parse_if_stmt(p);
    }

    /* línea normal: si tiene '=', es acciones; si no, es expresión (condición) */
    if (line_has_assign(p)) {
        return parse_actions_stmt(p);
    }

    return parse_expr_stmt(p);
}

int ddsl_parse_program(const ddsl_token_vec *tokens, ddsl_arena *arena, ddsl_program **out_prog, ddsl_error *err) {
    ddsl_parser p;
    ddsl_program *prog;
    size_t mark;

    if (err) ddsl_error_clear(err);
    if (out_prog) *out_prog = NULL;

    if (!tokens || tokens->count <= 0 || !out_prog || !arena) {
        if (err) ddsl_error_set(err, 0, 0, 0, "parser: argumentos inválidos");
        return 0;
    }

    p.toks = tokens->items;
    p.count = tokens->count;
    p.pos = 0;
    p.err = err;
    p.arena = arena;

    mark = ddsl_arena_mark(arena);

    prog = new_program(&p);
    if (!prog) {
        ddsl_arena_rewind(arena, mark);
        if (err) ddsl_error_set(err, 0, 0, 0, "sin memoria para programa (arena)" );
        return 0;
    }

    p_skip_newlines(&p);

    while (!p_is_end(&p)) {
        ddsl_stmt *s;
        s = parse_stmt(&p);
        if (!s) {
            ddsl_arena_rewind(arena, mark);
            return 0;
        }
        prog_push(prog, s);
        p_skip_newlines(&p);
    }

    *out_prog = prog;
    return 1;
}
