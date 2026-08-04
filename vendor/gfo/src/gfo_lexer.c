/* gfo_lexer.c */
#include "gfo_lexer.h"

static int is_alpha(char c){
  return (c>='a'&&c<='z') || (c>='A'&&c<='Z') || c=='_' || c=='-';
}
static int is_digit(char c){
  return (c>='0'&&c<='9');
}
static int is_alnum(char c){
  return is_alpha(c) || is_digit(c);
}

static char cur(gfo_lexer* lx){
  if(lx->pos >= lx->len) return '\0';
  return lx->src[lx->pos];
}
static char adv(gfo_lexer* lx){
  char c = cur(lx);
  if(c=='\0') return c;
  lx->pos++;
  if(c=='\n'){ lx->line++; lx->col=1; }
  else { lx->col++; }
  return c;
}
static gfo_token tok(gfo_tok_kind k, const char* start, gfo_u32 n, gfo_u32 line, gfo_u32 col){
  gfo_token t; t.kind=k; t.lexeme.ptr=start; t.lexeme.len=(gfo_u16)n; t.line=line; t.col=col; return t;
}
static void skip_ws(gfo_lexer* lx){
  for(;;){
    char c = cur(lx);
    if(c==' ' || c=='\t' || c=='\r'){ adv(lx); continue; }
    /* comment ; ... or // ... */
    if(c==';'){
      while(cur(lx)!='\0' && cur(lx)!='\n') adv(lx);
      continue;
    }
    if(c=='/' && lx->pos+1<lx->len && lx->src[lx->pos+1]=='/'){
      while(cur(lx)!='\0' && cur(lx)!='\n') adv(lx);
      continue;
    }
    break;
  }
}

void gfo_lex_init(gfo_lexer* lx, const char* src, gfo_u32 len){
  lx->src = src; lx->len=len; lx->pos=0; lx->line=1; lx->col=1; lx->had_error=0;
}

gfo_token gfo_lex_next(gfo_lexer* lx){
  const char* start;
  gfo_u32 line, col;
  char c;

  if(lx->pos >= lx->len) return tok(GFO_TOK_EOF, lx->src+lx->len, 0, lx->line, lx->col);

  skip_ws(lx);

  line = lx->line; col = lx->col;
  c = cur(lx);

  if(c=='\0') return tok(GFO_TOK_EOF, lx->src+lx->len, 0, line, col);

  /* keep newline token */
  if(c=='\n'){ adv(lx); return tok(GFO_TOK_NEWLINE, lx->src+lx->pos-1, 1, line, col); }

  /* single-char tokens */
  if(c=='['){ adv(lx); return tok(GFO_TOK_LBRACK, lx->src+lx->pos-1, 1, line, col); }
  if(c==']'){ adv(lx); return tok(GFO_TOK_RBRACK, lx->src+lx->pos-1, 1, line, col); }
  if(c=='#'){ adv(lx); return tok(GFO_TOK_HASH, lx->src+lx->pos-1, 1, line, col); }
  if(c=='='){ adv(lx); return tok(GFO_TOK_EQ, lx->src+lx->pos-1, 1, line, col); }
  if(c=='*'){ adv(lx); return tok(GFO_TOK_STAR, lx->src+lx->pos-1, 1, line, col); }
  if(c=='@'){ adv(lx); return tok(GFO_TOK_AT, lx->src+lx->pos-1, 1, line, col); }
  if(c=='('){ adv(lx); return tok(GFO_TOK_LPAREN, lx->src+lx->pos-1, 1, line, col); }
  if(c==')'){ adv(lx); return tok(GFO_TOK_RPAREN, lx->src+lx->pos-1, 1, line, col); }
  if(c==','){ adv(lx); return tok(GFO_TOK_COMMA, lx->src+lx->pos-1, 1, line, col); }
  if(c==':'){ adv(lx); return tok(GFO_TOK_COLON, lx->src+lx->pos-1, 1, line, col); }

  /* string: "..." or '...' */
  if(c=='"' || c=='\''){
    char q = c;
    start = lx->src + lx->pos;
    adv(lx); /* consume quote */
    while(cur(lx)!='\0' && cur(lx)!=q && cur(lx)!='\n') adv(lx);
    if(cur(lx)!=q){ lx->had_error=1; return tok(GFO_TOK_STRING, start, (gfo_u32)(lx->src+lx->pos-start), line, col); }
    /* include inner content only (no quotes) */
    {
      const char* inner = start+1;
      gfo_u32 n = (gfo_u32)((lx->src + lx->pos) - inner);
      adv(lx); /* consume closing quote */
      return tok(GFO_TOK_STRING, inner, n, line, col);
    }
  }

  /* number: [+-]?[0-9]+ */
  if(c=='+' || c=='-' || is_digit(c)){
    const char* p = lx->src + lx->pos;
    if(c=='+' || c=='-'){
      /* only treat as number if next is digit */
      if(lx->pos+1<lx->len && is_digit(lx->src[lx->pos+1])){
        adv(lx);
      } else {
        /* treat as ident char? here return IDENT of single char */
        adv(lx);
        return tok(GFO_TOK_IDENT, p, 1, line, col);
      }
    }
    if(is_digit(cur(lx))){
      while(is_digit(cur(lx))) adv(lx);
      return tok(GFO_TOK_NUMBER, p, (gfo_u32)((lx->src+lx->pos)-p), line, col);
    }
  }

  /* identifier */
  if(is_alpha(c) || c=='.' || c=='/'){
    start = lx->src + lx->pos;
    adv(lx);
    while(is_alnum(cur(lx)) || cur(lx)=='.' || cur(lx)=='/' ) adv(lx);
    return tok(GFO_TOK_IDENT, start, (gfo_u32)((lx->src+lx->pos)-start), line, col);
  }

  /* unknown */
  lx->had_error=1;
  adv(lx);
  return tok(GFO_TOK_IDENT, lx->src+lx->pos-1, 1, line, col);
}

gfo_token gfo_lex_peek(gfo_lexer* lx){
  gfo_lexer c = *lx;
  return gfo_lex_next(&c);
}
