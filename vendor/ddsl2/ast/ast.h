#ifndef DDSL_AST_H
#define DDSL_AST_H

#include "token/token.h"
#include "types/fixed.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================ AST ============================
 * El AST se aloca desde una arena (ver arena/arena.h).
 * ddsl_ast_discard_program() es un no-op (en modo arena).
 */

typedef struct ddsl_expr ddsl_expr;
typedef struct ddsl_action ddsl_action;
typedef struct ddsl_stmt ddsl_stmt;
typedef struct ddsl_program ddsl_program;

typedef enum ddsl_expr_kind {
    DDSL_EXPR_NUMBER = 1,
    DDSL_EXPR_STRING,
    DDSL_EXPR_IDENT,
    DDSL_EXPR_UNARY,
    DDSL_EXPR_BINARY
} ddsl_expr_kind;

typedef enum ddsl_op {
    DDSL_OP_PLUS,
    DDSL_OP_MINUS,
    DDSL_OP_MUL,
    DDSL_OP_DIV,

    DDSL_OP_EQ,
    DDSL_OP_NEQ,
    DDSL_OP_LT,
    DDSL_OP_LTE,
    DDSL_OP_GT,
    DDSL_OP_GTE,

    DDSL_OP_AND,
    DDSL_OP_OR
} ddsl_op;

struct ddsl_expr {
    ddsl_expr_kind kind;
    ddsl_token at; /* token de referencia (para debug/errores) */
    union {
        ddsl_fixed number;
        ddsl_strview string;
        ddsl_strview ident;
        struct {
            ddsl_op op;
            ddsl_expr *rhs;
        } unary;
        struct {
            ddsl_op op;
            ddsl_expr *lhs;
            ddsl_expr *rhs;
        } binary;
    } as;
};

typedef enum ddsl_action_kind {
    DDSL_ACTION_FLAG = 1,   /* foo  => foo=true */
    DDSL_ACTION_SET         /* foo = expr */
} ddsl_action_kind;

struct ddsl_action {
    ddsl_action_kind kind;
    ddsl_token at;
    ddsl_strview name;      /* IDENT */
    ddsl_expr *value;       /* NULL si FLAG */
    ddsl_action *next;
};

typedef struct ddsl_if_clause {
    int is_else;            /* 0 => if/elif con cond; 1 => else */
    ddsl_token at;
    ddsl_expr *cond;        /* NULL si else */
    ddsl_action *actions;   /* puede ser NULL */
    struct ddsl_if_clause *next;
} ddsl_if_clause;

typedef enum ddsl_stmt_kind {
    DDSL_STMT_ACTIONS = 1,
    DDSL_STMT_IF,
    DDSL_STMT_EXPR /* evalúa expr y descarta (útil para "solo condición") */
} ddsl_stmt_kind;

struct ddsl_stmt {
    ddsl_stmt_kind kind;
    ddsl_token at;
    union {
        struct { ddsl_action *actions; } actions;
        struct { ddsl_if_clause *clauses; } ifs;
        struct { ddsl_expr *expr; } expr;
    } as;
    ddsl_stmt *next;
};

struct ddsl_program {
    ddsl_stmt *first;
    ddsl_stmt *last;
};

/* Free helpers */
void ddsl_ast_discard_expr(ddsl_expr *e);
void ddsl_ast_discard_actions(ddsl_action *a);
void ddsl_ast_discard_clauses(ddsl_if_clause *c);
void ddsl_ast_discard_program(ddsl_program *p);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_AST_H */
