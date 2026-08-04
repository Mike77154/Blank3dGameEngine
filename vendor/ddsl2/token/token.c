#include "token/token.h"

/* Vector fijo (ver DDSL_MAX_TOKENS). */

const char *ddsl_tok_kind_name(ddsl_tok_kind k) {
    switch (k) {
        case DDSL_TOK_EOF: return "EOF";
        case DDSL_TOK_NEWLINE: return "NEWLINE";
        case DDSL_TOK_IDENT: return "IDENT";
        case DDSL_TOK_NUMBER: return "NUMBER";
        case DDSL_TOK_STRING: return "STRING";
        case DDSL_TOK_IF: return "IF";
        case DDSL_TOK_ELIF: return "ELIF";
        case DDSL_TOK_ELSE: return "ELSE";
        case DDSL_TOK_THEN: return "THEN";
        case DDSL_TOK_AND: return "AND";
        case DDSL_TOK_OR: return "OR";
        case DDSL_TOK_EQ: return "=";
        case DDSL_TOK_EQEQ: return "==";
        case DDSL_TOK_NEQ: return "!=";
        case DDSL_TOK_LT: return "<";
        case DDSL_TOK_LTE: return "<=";
        case DDSL_TOK_GT: return ">";
        case DDSL_TOK_GTE: return ">=";
        case DDSL_TOK_PLUS: return "+";
        case DDSL_TOK_MINUS: return "-";
        case DDSL_TOK_STAR: return "*";
        case DDSL_TOK_SLASH: return "/";
        case DDSL_TOK_LPAREN: return "(";
        case DDSL_TOK_RPAREN: return ")";
        case DDSL_TOK_COMMA: return ",";
        case DDSL_TOK_QMARK: return "?";
        default: return "<unknown>";
    }
}

void ddsl_tokens_init(ddsl_token_vec *v) {
    if (!v) return;
    v->count = 0;
}

void ddsl_tokens_reset(ddsl_token_vec *v) {
    /* No-op: no hay memoria dinámica. */
    if (!v) return;
    v->count = 0;
}

int ddsl_tokens_push(ddsl_token_vec *v, ddsl_token t) {
    if (!v) return 0;

    if (v->count >= DDSL_MAX_TOKENS) return 0;
    v->items[v->count++] = t;
    return 1;
}
