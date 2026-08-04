#include "runtime/runtime.h"
#include "lexer/lexer.h"
#include "parser/parser.h"
#include "semantics/semantics.h"

#include <string.h>
#include <ctype.h>

static int sv_eq_ci(ddsl_strview sv, const char *cstr) {
    int i;
    int n;

    if (!cstr) cstr = "";
    n = (int)strlen(cstr);
    if (sv.len != n) return 0;

    for (i = 0; i < n; ++i) {
        char a = (char)tolower((unsigned char)sv.data[i]);
        char b = (char)tolower((unsigned char)cstr[i]);
        if (a != b) return 0;
    }
    return 1;
}

static int sv_to_cstr_trunc(ddsl_strview sv, char *dst, int dst_cap) {
    int n;
    if (!dst || dst_cap <= 0) return 0;
    n = sv.len;
    if (n < 0) n = 0;
    if (n > dst_cap - 1) n = dst_cap - 1;
    if (n > 0 && sv.data) memcpy(dst, sv.data, (size_t)n);
    dst[n] = '\0';
    return n;
}

static int store_get_value(ddsl_runtime *rt, ddsl_strview ident, ddsl_value *out) {
    char keybuf[DDSL_MAX_KEY_LEN];
    const char *cur;

    if (!rt || !rt->store || !out) return 0;

    /* bool literals */
    if (sv_eq_ci(ident, "true")) {
        *out = ddsl_v_bool(1);
        return 1;
    }
    if (sv_eq_ci(ident, "false")) {
        *out = ddsl_v_bool(0);
        return 1;
    }

    /* lookup store */
    sv_to_cstr_trunc(ident, keybuf, (int)sizeof(keybuf));
    cur = ddsl_store_get(rt->store, keybuf);

    if (cur) {
        ddsl_fixed d;
        if (ddsl_fixed_parse_cstr(cur, &d)) {
            *out = ddsl_v_num(d);
        } else {
            *out = ddsl_v_str(ddsl_sv_from_cstr(cur));
        }
        return 1;
    }

    return 0;
}

static ddsl_value eval_value(ddsl_runtime *rt, ddsl_expr *e);
static int eval_bool(ddsl_runtime *rt, ddsl_expr *e);

static ddsl_value eval_value(ddsl_runtime *rt, ddsl_expr *e) {
    ddsl_value v;

    if (!rt || !e) return ddsl_v_null();

    switch (e->kind) {
        case DDSL_EXPR_NUMBER:
            return ddsl_v_num(e->as.number);

        case DDSL_EXPR_STRING:
            return ddsl_v_str(e->as.string);

        case DDSL_EXPR_IDENT:
            if (store_get_value(rt, e->as.ident, &v)) return v;
            /* si no existe: treat como string constante con el mismo nombre */
            return ddsl_v_str(e->as.ident);

        case DDSL_EXPR_UNARY: {
            if (e->as.unary.op == DDSL_OP_MINUS) {
                ddsl_value rhs;
                int ok;
                ddsl_fixed n;

                rhs = eval_value(rt, e->as.unary.rhs);
                n = ddsl_value_to_num(rhs, &ok);
                if (!ok) n = DDSL_FIXED_ZERO;
                return ddsl_v_num(ddsl_fixed_neg(n));
            }
            return ddsl_v_null();
        }

        case DDSL_EXPR_BINARY: {
            ddsl_op op;
            op = e->as.binary.op;

            /* boolean ops: devolvemos bool */
            if (op == DDSL_OP_AND) {
                return ddsl_v_bool(eval_bool(rt, e));
            }
            if (op == DDSL_OP_OR) {
                return ddsl_v_bool(eval_bool(rt, e));
            }

            /* comparaciones: devolvemos bool */
            if (op == DDSL_OP_EQ || op == DDSL_OP_NEQ || op == DDSL_OP_LT || op == DDSL_OP_LTE ||
                op == DDSL_OP_GT || op == DDSL_OP_GTE) {
                return ddsl_v_bool(eval_bool(rt, e));
            }

            /* aritmética */
            {
                ddsl_value a;
                ddsl_value b;
                int ok1;
                int ok2;
                ddsl_fixed n1;
                ddsl_fixed n2;

                a = eval_value(rt, e->as.binary.lhs);
                b = eval_value(rt, e->as.binary.rhs);
                n1 = ddsl_value_to_num(a, &ok1);
                n2 = ddsl_value_to_num(b, &ok2);
                if (!ok1) n1 = DDSL_FIXED_ZERO;
                if (!ok2) n2 = DDSL_FIXED_ZERO;

                if (op == DDSL_OP_PLUS) return ddsl_v_num(ddsl_fixed_add(n1, n2));
                if (op == DDSL_OP_MINUS) return ddsl_v_num(ddsl_fixed_sub(n1, n2));
                if (op == DDSL_OP_MUL) return ddsl_v_num(ddsl_fixed_mul(n1, n2));
                if (op == DDSL_OP_DIV) {
                    if (n2 == DDSL_FIXED_ZERO) return ddsl_v_num(DDSL_FIXED_ZERO);
                    return ddsl_v_num(ddsl_fixed_div(n1, n2));
                }
            }

            return ddsl_v_null();
        }

        default:
            break;
    }

    return ddsl_v_null();
}

static int ci_cmp_sv(ddsl_strview a, ddsl_strview b) {
    int i;
    int n;
    int na;
    int nb;

    na = a.len;
    nb = b.len;
    if (na < 0) na = 0;
    if (nb < 0) nb = 0;

    n = (na < nb) ? na : nb;
    for (i = 0; i < n; ++i) {
        char ca = (char)tolower((unsigned char)a.data[i]);
        char cb = (char)tolower((unsigned char)b.data[i]);
        if (ca < cb) return -1;
        if (ca > cb) return 1;
    }

    if (na < nb) return -1;
    if (na > nb) return 1;
    return 0;
}

static int eval_cmp(ddsl_runtime *rt, ddsl_op op, ddsl_expr *lhs, ddsl_expr *rhs) {
    ddsl_value a;
    ddsl_value b;
    int ok1;
    int ok2;
    ddsl_fixed n1;
    ddsl_fixed n2;

    a = eval_value(rt, lhs);
    b = eval_value(rt, rhs);

    /* igualdad: num si se puede, si no string */
    if (op == DDSL_OP_EQ || op == DDSL_OP_NEQ) {
        n1 = ddsl_value_to_num(a, &ok1);
        n2 = ddsl_value_to_num(b, &ok2);
        if (ok1 && ok2) {
            int eq = (n1 == n2) ? 1 : 0;
            return (op == DDSL_OP_EQ) ? eq : (!eq);
        }

        /* string compare (case-insensitive) */
        {
            ddsl_strview sa;
            ddsl_strview sb;
            char ba[64];
            char bb[64];

            if (a.kind == DDSL_VAL_STR) sa = a.str;
            else {
                ddsl_value_to_cstr(a, ba, (int)sizeof(ba));
                sa = ddsl_sv_from_cstr(ba);
            }

            if (b.kind == DDSL_VAL_STR) sb = b.str;
            else {
                ddsl_value_to_cstr(b, bb, (int)sizeof(bb));
                sb = ddsl_sv_from_cstr(bb);
            }

            {
                int eq2 = (ci_cmp_sv(sa, sb) == 0) ? 1 : 0;
                return (op == DDSL_OP_EQ) ? eq2 : (!eq2);
            }
        }
    }

    /* orden: solo num */
    n1 = ddsl_value_to_num(a, &ok1);
    n2 = ddsl_value_to_num(b, &ok2);
    if (!(ok1 && ok2)) return 0;

    if (op == DDSL_OP_LT) return (n1 < n2) ? 1 : 0;
    if (op == DDSL_OP_LTE) return (n1 <= n2) ? 1 : 0;
    if (op == DDSL_OP_GT) return (n1 > n2) ? 1 : 0;
    if (op == DDSL_OP_GTE) return (n1 >= n2) ? 1 : 0;

    return 0;
}

static int eval_bool(ddsl_runtime *rt, ddsl_expr *e) {
    if (!rt || !e) return 0;

    /* Regla clave (para que el lenguaje no castigue):
     * Si la condición es IDENT sola, se interpreta como "flag exists & truthy".
     * Si no existe en store => false.
     */
    if (e->kind == DDSL_EXPR_IDENT) {
        ddsl_value v;
        if (!store_get_value(rt, e->as.ident, &v)) return 0;
        return ddsl_value_truthy(v);
    }

    if (e->kind == DDSL_EXPR_BINARY) {
        ddsl_op op;
        op = e->as.binary.op;

        if (op == DDSL_OP_AND) {
            if (!eval_bool(rt, e->as.binary.lhs)) return 0;
            return eval_bool(rt, e->as.binary.rhs);
        }
        if (op == DDSL_OP_OR) {
            if (eval_bool(rt, e->as.binary.lhs)) return 1;
            return eval_bool(rt, e->as.binary.rhs);
        }

        if (op == DDSL_OP_EQ || op == DDSL_OP_NEQ || op == DDSL_OP_LT || op == DDSL_OP_LTE ||
            op == DDSL_OP_GT || op == DDSL_OP_GTE) {
            return eval_cmp(rt, op, e->as.binary.lhs, e->as.binary.rhs);
        }
    }

    /* fallback: eval value y truthy */
    return ddsl_value_truthy(eval_value(rt, e));
}

static int do_emit(ddsl_runtime *rt, const char *key_norm, ddsl_value v) {
    if (!rt || !rt->emit) return 1;
    return rt->emit(rt->emit_user, key_norm, v);
}

static int exec_action(ddsl_runtime *rt, ddsl_action *a, ddsl_error *err) {
    char keybuf[128];
    char key_norm[DDSL_MAX_KEY_LEN];

    if (err) ddsl_error_clear(err);
    if (!rt || !rt->store || !a) return 1;

    /* nombre (IDENT) -> C-string -> normaliza */
    sv_to_cstr_trunc(a->name, keybuf, (int)sizeof(keybuf));
    ddsl_norm_key(keybuf, key_norm, sizeof(key_norm));

    if (a->kind == DDSL_ACTION_FLAG) {
        if (!ddsl_store_set(rt->store, key_norm, "true")) {
            if (err) ddsl_error_set(err, a->at.line, a->at.col, a->at.offset,
                                    "runtime: store lleno o escritura inválida");
            return 0;
        }
        return do_emit(rt, key_norm, ddsl_v_bool(1));
    }

    if (a->kind == DDSL_ACTION_SET) {
        ddsl_value v;
        char valbuf[DDSL_MAX_VALUE_LEN];

        v = eval_value(rt, a->value);
        ddsl_value_to_cstr(v, valbuf, (int)sizeof(valbuf));
        if (!ddsl_store_set(rt->store, key_norm, valbuf)) {
            if (err) ddsl_error_set(err, a->at.line, a->at.col, a->at.offset,
                                    "runtime: store lleno o escritura inválida");
            return 0;
        }
        return do_emit(rt, key_norm, v);
    }

    (void)err;
    return 1;
}

static int exec_actions(ddsl_runtime *rt, ddsl_action *a, ddsl_error *err) {
    while (a) {
        int ok = exec_action(rt, a, err);
        if (!ok) return 0;
        a = a->next;
    }
    return 1;
}

static int exec_stmt(ddsl_runtime *rt, ddsl_stmt *s, ddsl_error *err) {
    if (!rt || !s) return 1;

    if (s->kind == DDSL_STMT_ACTIONS) {
        return exec_actions(rt, s->as.actions.actions, err);
    }

    if (s->kind == DDSL_STMT_EXPR) {
        (void)eval_bool(rt, s->as.expr.expr);
        return 1;
    }

    if (s->kind == DDSL_STMT_IF) {
        ddsl_if_clause *c;
        int matched;

        matched = 0;
        for (c = s->as.ifs.clauses; c; c = c->next) {
            if (c->is_else) {
                if (!matched) {
                    if (!exec_actions(rt, c->actions, err)) return 0;
                    matched = 1;
                }
                break;
            } else {
                if (eval_bool(rt, c->cond)) {
                    if (!exec_actions(rt, c->actions, err)) return 0;
                    matched = 1;
                    break;
                }
            }
        }
        return 1;
    }

    return 1;
}

void ddsl_runtime_init(ddsl_runtime *rt, ddsl_store *store) {
    if (!rt) return;
    rt->store = store;
    rt->emit = NULL;
    rt->emit_user = NULL;
    rt->arena = NULL;
}

void ddsl_runtime_set_emit(ddsl_runtime *rt, ddsl_emit_fn fn, void *user) {
    if (!rt) return;
    rt->emit = fn;
    rt->emit_user = user;
}

void ddsl_runtime_set_arena(ddsl_runtime *rt, ddsl_arena *arena) {
    if (!rt) return;
    rt->arena = arena;
}

int ddsl_runtime_run(ddsl_runtime *rt, const ddsl_program *prog, ddsl_error *err) {
    ddsl_stmt *s;

    if (err) ddsl_error_clear(err);
    if (!rt || !rt->store || !prog) {
        if (err) ddsl_error_set(err, 0, 0, 0, "runtime: argumentos inválidos");
        return 0;
    }

    if (!ddsl_semantics_check_program(prog, err)) return 0;

    for (s = prog->first; s; s = s->next) {
        if (!exec_stmt(rt, s, err)) {
            if (err && err->message[0] == '\0') {
                ddsl_error_set(err, s->at.line, s->at.col, s->at.offset, "runtime: abortado por callback");
            }
            return 0;
        }
    }

    return 1;
}

int ddsl_exec_source(ddsl_runtime *rt, const char *source, ddsl_error *err) {
    ddsl_token_vec toks;
    ddsl_program *prog;
    int ok;
    size_t mark;

    if (err) ddsl_error_clear(err);
    if (!rt || !source) {
        if (err) ddsl_error_set(err, 0, 0, 0, "exec: argumentos inválidos");
        return 0;
    }

    if (!rt->arena) {
        if (err) ddsl_error_set(err, 0, 0, 0, "exec: necesitas configurar ddsl_runtime_set_arena() (sin asignador dinámico)" );
        return 0;
    }

    mark = ddsl_arena_mark(rt->arena);

    if (!ddsl_lex_all(source, &toks, err)) {
        return 0;
    }

    prog = NULL;
    ok = ddsl_parse_program(&toks, rt->arena, &prog, err);
    if (!ok) {
        ddsl_tokens_reset(&toks);
        return 0;
    }

    ok = ddsl_runtime_run(rt, prog, err);
    ddsl_tokens_reset(&toks);
    ddsl_arena_rewind(rt->arena, mark);
    return ok;
}
