/* gfo_parser.h - runtime parser: lexer -> AST */
#ifndef GFO_PARSER_H
#define GFO_PARSER_H
#include "gfo_lexer.h"
#include "gfo_sym.h"
#include "gfo_ast.h"

typedef struct {
  gfo_lexer lx;
  gfo_token cur;
  gfo_symtab* sym;
  gfo_ast* ast;
  int error;
} gfo_parser;

void gfo_parse_init(gfo_parser* p, const char* src, gfo_u32 len, gfo_symtab* sym, gfo_ast* ast);
int  gfo_parse_file(gfo_parser* p);

#endif
