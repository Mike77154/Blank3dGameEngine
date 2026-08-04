#include "ast/ast.h"

/* Memoria externa fija: el AST vive en una arena. */

void ddsl_ast_discard_expr(ddsl_expr *e) {
    (void)e;
}

void ddsl_ast_discard_actions(ddsl_action *a) {
    (void)a;
}

void ddsl_ast_discard_clauses(ddsl_if_clause *c) {
    (void)c;
}

void ddsl_ast_discard_program(ddsl_program *p) {
    (void)p;
}
