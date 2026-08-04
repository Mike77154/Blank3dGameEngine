#include "semantics/semantics.h"

#include "store/store.h"

#include <ctype.h>
#include <string.h>

typedef struct ddsl_semantics_ctx {
    ddsl_error *err;
    int nodes;
} ddsl_semantics_ctx;

static int sem_fail(ddsl_semantics_ctx *ctx, const ddsl_token *at, const char *msg) {
    int line;
    int col;
    int offset;

    line = at ? at->line : 0;
    col = at ? at->col : 0;
    offset = at ? at->offset : 0;
    if (ctx && ctx->err) ddsl_error_set(ctx->err, line, col, offset, msg);
    return 0;
}

static int sem_take_node(ddsl_semantics_ctx *ctx, const ddsl_token *at) {
    if (!ctx) return 0;
    ctx->nodes++;
    if (ctx->nodes > DDSL_SEMANTICS_MAX_NODES) {
        return sem_fail(ctx, at, "semantics: AST demasiado grande o con ciclo");
    }
    return 1;
}

static int sv_eq_ci(ddsl_strview sv, const char *text) {
    int i;
    int n;

    if (!text) text = "";
    n = (int)strlen(text);
    if (sv.len != n || !sv.data) return 0;
    for (i = 0; i < n; ++i) {
        int a;
        int b;
        a = tolower((unsigned char)sv.data[i]);
        b = tolower((unsigned char)text[i]);
        if (a != b) return 0;
    }
    return 1;
}

static int sem_identifier_shape(ddsl_strview name) {
    int i;
    int ch;

    if (!name.data || name.len <= 0) return 0;
    ch = (unsigned char)name.data[0];
    if (!(isalpha(ch) || ch == '_')) return 0;
    for (i = 1; i < name.len; ++i) {
        ch = (unsigned char)name.data[i];
        if (!(isalnum(ch) || ch == '_' || ch == '-')) return 0;
    }
    return 1;
}

static int sem_identifier(ddsl_semantics_ctx *ctx, ddsl_strview name,
                          const ddsl_token *at, int is_target) {
    if (!sem_identifier_shape(name)) {
        return sem_fail(ctx, at, "semantics: identificador inválido");
    }
    if (name.len >= DDSL_MAX_KEY_LEN) {
        return sem_fail(ctx, at, "semantics: identificador excede DDSL_MAX_KEY_LEN");
    }
    if (is_target && (sv_eq_ci(name, "true") || sv_eq_ci(name, "false"))) {
        return sem_fail(ctx, at, "semantics: no se puede asignar a true/false");
    }
    return 1;
}

static int sem_binary_op_valid(ddsl_op op) {
    return (op == DDSL_OP_PLUS || op == DDSL_OP_MINUS ||
            op == DDSL_OP_MUL || op == DDSL_OP_DIV ||
            op == DDSL_OP_EQ || op == DDSL_OP_NEQ ||
            op == DDSL_OP_LT || op == DDSL_OP_LTE ||
            op == DDSL_OP_GT || op == DDSL_OP_GTE ||
            op == DDSL_OP_AND || op == DDSL_OP_OR) ? 1 : 0;
}

static int sem_expr(ddsl_semantics_ctx *ctx, const ddsl_expr *expr, int depth) {
    if (!expr) return sem_fail(ctx, NULL, "semantics: expresión nula");
    if (depth > DDSL_SEMANTICS_MAX_DEPTH) {
        return sem_fail(ctx, &expr->at, "semantics: expresión demasiado profunda o cíclica");
    }
    if (!sem_take_node(ctx, &expr->at)) return 0;

    switch (expr->kind) {
        case DDSL_EXPR_NUMBER:
            return 1;

        case DDSL_EXPR_STRING:
            if (expr->as.string.len < 0 ||
                (expr->as.string.len > 0 && !expr->as.string.data)) {
                return sem_fail(ctx, &expr->at, "semantics: string inválido");
            }
            if (expr->as.string.len >= DDSL_MAX_VALUE_LEN) {
                return sem_fail(ctx, &expr->at, "semantics: string excede DDSL_MAX_VALUE_LEN");
            }
            return 1;

        case DDSL_EXPR_IDENT:
            return sem_identifier(ctx, expr->as.ident, &expr->at, 0);

        case DDSL_EXPR_UNARY:
            if (expr->as.unary.op != DDSL_OP_MINUS) {
                return sem_fail(ctx, &expr->at, "semantics: operador unario inválido");
            }
            return sem_expr(ctx, expr->as.unary.rhs, depth + 1);

        case DDSL_EXPR_BINARY:
            if (!sem_binary_op_valid(expr->as.binary.op)) {
                return sem_fail(ctx, &expr->at, "semantics: operador binario inválido");
            }
            if (!sem_expr(ctx, expr->as.binary.lhs, depth + 1)) return 0;
            return sem_expr(ctx, expr->as.binary.rhs, depth + 1);

        default:
            return sem_fail(ctx, &expr->at, "semantics: tipo de expresión desconocido");
    }
}

static int sem_actions(ddsl_semantics_ctx *ctx, const ddsl_action *action) {
    const ddsl_action *cur;

    cur = action;
    while (cur) {
        if (!sem_take_node(ctx, &cur->at)) return 0;
        if (!sem_identifier(ctx, cur->name, &cur->at, 1)) return 0;

        if (cur->kind == DDSL_ACTION_FLAG) {
            if (cur->value) {
                return sem_fail(ctx, &cur->at, "semantics: flag no debe tener valor");
            }
        } else if (cur->kind == DDSL_ACTION_SET) {
            if (!cur->value) {
                return sem_fail(ctx, &cur->at, "semantics: asignación sin valor");
            }
            if (!sem_expr(ctx, cur->value, 1)) return 0;
        } else {
            return sem_fail(ctx, &cur->at, "semantics: tipo de acción desconocido");
        }
        cur = cur->next;
    }
    return 1;
}

static int sem_if(ddsl_semantics_ctx *ctx, const ddsl_if_clause *clause,
                  const ddsl_token *stmt_at) {
    const ddsl_if_clause *cur;
    int index;
    int saw_else;

    if (!clause) return sem_fail(ctx, stmt_at, "semantics: if sin cláusulas");

    cur = clause;
    index = 0;
    saw_else = 0;
    while (cur) {
        if (!sem_take_node(ctx, &cur->at)) return 0;
        if (cur->is_else) {
            if (index == 0) {
                return sem_fail(ctx, &cur->at, "semantics: if no puede comenzar con else");
            }
            if (saw_else || cur->next) {
                return sem_fail(ctx, &cur->at, "semantics: else debe ser único y final");
            }
            if (cur->cond) {
                return sem_fail(ctx, &cur->at, "semantics: else no debe tener condición");
            }
            saw_else = 1;
        } else {
            if (saw_else) {
                return sem_fail(ctx, &cur->at, "semantics: cláusula después de else");
            }
            if (!cur->cond) {
                return sem_fail(ctx, &cur->at, "semantics: if/elif sin condición");
            }
            if (!sem_expr(ctx, cur->cond, 1)) return 0;
        }
        if (!sem_actions(ctx, cur->actions)) return 0;
        cur = cur->next;
        index++;
    }
    return 1;
}

static int sem_stmt(ddsl_semantics_ctx *ctx, const ddsl_stmt *stmt) {
    if (!stmt) return sem_fail(ctx, NULL, "semantics: sentencia nula");
    if (!sem_take_node(ctx, &stmt->at)) return 0;

    if (stmt->kind == DDSL_STMT_ACTIONS) {
        if (!stmt->as.actions.actions) {
            return sem_fail(ctx, &stmt->at, "semantics: sentencia de acciones vacía");
        }
        return sem_actions(ctx, stmt->as.actions.actions);
    }
    if (stmt->kind == DDSL_STMT_IF) {
        return sem_if(ctx, stmt->as.ifs.clauses, &stmt->at);
    }
    if (stmt->kind == DDSL_STMT_EXPR) {
        return sem_expr(ctx, stmt->as.expr.expr, 1);
    }
    return sem_fail(ctx, &stmt->at, "semantics: tipo de sentencia desconocido");
}

int ddsl_semantics_check_program(const ddsl_program *prog, ddsl_error *err) {
    ddsl_semantics_ctx ctx;
    const ddsl_stmt *stmt;
    const ddsl_stmt *last;

    if (err) ddsl_error_clear(err);
    if (!prog) {
        if (err) ddsl_error_set(err, 0, 0, 0, "semantics: programa inválido");
        return 0;
    }

    ctx.err = err;
    ctx.nodes = 0;

    if ((prog->first == NULL) != (prog->last == NULL)) {
        return sem_fail(&ctx, NULL, "semantics: first/last inconsistentes");
    }

    stmt = prog->first;
    last = NULL;
    while (stmt) {
        if (!sem_stmt(&ctx, stmt)) return 0;
        last = stmt;
        stmt = stmt->next;
    }

    if (last != prog->last) {
        return sem_fail(&ctx, NULL, "semantics: last no coincide con la lista");
    }
    if (prog->last && prog->last->next) {
        return sem_fail(&ctx, &prog->last->at, "semantics: last debe cerrar la lista");
    }

    return 1;
}
