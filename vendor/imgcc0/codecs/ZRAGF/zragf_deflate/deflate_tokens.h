/* deflate_tokens.h */

#ifndef ZRAGF_DEFLATE_TOKENS_H_INCLUDED
#define ZRAGF_DEFLATE_TOKENS_H_INCLUDED

#include "zragflib_internal.h"

/* Tipo de token: literal o match LZ */
typedef enum {
    ZRAGF_TOK_LITERAL = 0,
    ZRAGF_TOK_MATCH   = 1
} zragf_token_type;

typedef struct {
    zragf_token_type type;
    int              lit;   /* válido si type == LITERAL (0–255) */
    int              len;   /* válido si type == MATCH  (3–258)  */
    int              dist;  /* válido si type == MATCH  (1–32768)*/
} zragf_token;

typedef struct {
    zragf_token *data;
    zragf_size_t size;
    zragf_size_t cap;
} zragf_token_buffer;

/* Inicializa buffer (cap inicial opcional, 0=auto) */
int zragf_tokens_init(zragf_token_buffer *tb, zragf_size_t initial_cap);

/* Libera memoria */
void zragf_tokens_free(zragf_token_buffer *tb);

/* Resetea a vacío (no libera memoria) */
void zragf_tokens_reset(zragf_token_buffer *tb);

/* Reserva espacio extra para append rápido. */
int zragf_tokens_reserve_extra(zragf_token_buffer *tb, zragf_size_t extra);

/* Empuja un literal */
int zragf_tokens_push_literal(zragf_token_buffer *tb, int lit);

/* Empuja un match */
int zragf_tokens_push_match(zragf_token_buffer *tb, int len, int dist);


static ZRAGF_MAYBE_UNUSED ZRAGF_INLINE void zragf_tokens_append_literal_unchecked(zragf_token_buffer *tb, int lit)
{
    zragf_token *t = &tb->data[tb->size++];
    t->type = ZRAGF_TOK_LITERAL;
    t->lit = lit;
    t->len = 0;
    t->dist = 0;
}

static ZRAGF_MAYBE_UNUSED ZRAGF_INLINE void zragf_tokens_append_match_unchecked(zragf_token_buffer *tb, int len, int dist)
{
    zragf_token *t = &tb->data[tb->size++];
    t->type = ZRAGF_TOK_MATCH;
    t->lit = -1;
    t->len = len;
    t->dist = dist;
}

#endif /* ZRAGF_DEFLATE_TOKENS_H_INCLUDED */
