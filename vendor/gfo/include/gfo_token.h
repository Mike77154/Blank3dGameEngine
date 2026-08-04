/* gfo_token.h */
#ifndef GFO_TOKEN_H
#define GFO_TOKEN_H
#include "gfo_common.h"

typedef enum {
  GFO_TOK_EOF = 0,
  GFO_TOK_NEWLINE,

  GFO_TOK_IDENT,
  GFO_TOK_NUMBER,
  GFO_TOK_STRING,

  /* symbols */
  GFO_TOK_LBRACK,   /* [ */
  GFO_TOK_RBRACK,   /* ] */
  GFO_TOK_HASH,     /* # */
  GFO_TOK_EQ,       /* = */
  GFO_TOK_STAR,     /* * */
  GFO_TOK_AT,       /* @ */
  GFO_TOK_LPAREN,   /* ( */
  GFO_TOK_RPAREN,   /* ) */
  GFO_TOK_COMMA,    /* , */
  GFO_TOK_COLON     /* : */
} gfo_tok_kind;

typedef struct {
  gfo_tok_kind kind;
  gfo_str      lexeme;    /* slice into source */
  gfo_u32      line;
  gfo_u32      col;
} gfo_token;

#endif
