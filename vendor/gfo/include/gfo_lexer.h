/* gfo_lexer.h */
#ifndef GFO_LEXER_H
#define GFO_LEXER_H
#include "gfo_token.h"

typedef struct {
  const char* src;
  gfo_u32     len;
  gfo_u32     pos;
  gfo_u32     line;
  gfo_u32     col;
  int         had_error;
} gfo_lexer;

void gfo_lex_init(gfo_lexer* lx, const char* src, gfo_u32 len);

/* next token (skips spaces/tabs/comments). Keeps NEWLINE tokens. */
gfo_token gfo_lex_next(gfo_lexer* lx);

/* peek without consuming (uses a copy) */
gfo_token gfo_lex_peek(gfo_lexer* lx);

#endif
