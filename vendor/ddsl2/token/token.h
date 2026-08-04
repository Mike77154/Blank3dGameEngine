#ifndef DDSL_TOKEN_H
#define DDSL_TOKEN_H

#include "common/strview.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ddsl_tok_kind {
    DDSL_TOK_EOF = 0,
    DDSL_TOK_NEWLINE,

    DDSL_TOK_IDENT,
    DDSL_TOK_NUMBER,
    DDSL_TOK_STRING,

    DDSL_TOK_IF,
    DDSL_TOK_ELIF,
    DDSL_TOK_ELSE,
    DDSL_TOK_THEN,
    DDSL_TOK_AND,
    DDSL_TOK_OR,

    /* operadores */
    DDSL_TOK_EQ,      /* '=' */
    DDSL_TOK_EQEQ,    /* '==' */
    DDSL_TOK_NEQ,     /* '!=' */
    DDSL_TOK_LT,      /* '<' */
    DDSL_TOK_LTE,     /* '<=' */
    DDSL_TOK_GT,      /* '>' */
    DDSL_TOK_GTE,     /* '>=' */

    DDSL_TOK_PLUS,
    DDSL_TOK_MINUS,
    DDSL_TOK_STAR,
    DDSL_TOK_SLASH,

    DDSL_TOK_LPAREN,
    DDSL_TOK_RPAREN,
    DDSL_TOK_COMMA,
    DDSL_TOK_QMARK
} ddsl_tok_kind;

typedef struct ddsl_token {
    ddsl_tok_kind kind;
    ddsl_strview lexeme; /* slice dentro del source */
    int line;            /* 1-based */
    int col;             /* 1-based */
    int offset;          /* 0-based */
} ddsl_token;

const char *ddsl_tok_kind_name(ddsl_tok_kind k);

/* Vector fijo de tokens (sin asignador dinámico).
 * Ajusta DDSL_MAX_TOKENS según tu caso.
 */
#ifndef DDSL_MAX_TOKENS
#define DDSL_MAX_TOKENS 4096
#endif

typedef struct ddsl_token_vec {
    ddsl_token items[DDSL_MAX_TOKENS];
    int count;
} ddsl_token_vec;

void ddsl_tokens_init(ddsl_token_vec *v);
void ddsl_tokens_reset(ddsl_token_vec *v);
int ddsl_tokens_push(ddsl_token_vec *v, ddsl_token t);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_TOKEN_H */
