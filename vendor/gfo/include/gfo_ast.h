/* gfo_ast.h - compact AST stored in arrays, no malloc */
#ifndef GFO_AST_H
#define GFO_AST_H
#include "gfo_value.h"

/* Node kinds */
typedef enum {
  GFO_AST_NONE=0,
  GFO_AST_FILE,
  GFO_AST_SECTION,
  GFO_AST_ASSIGN,
  GFO_AST_LIFECYCLE,
  GFO_AST_HANDLER,
  GFO_AST_INVOKER,
  GFO_AST_SCRIPT_PATH,
  GFO_AST_SCRIPT_BLOCK
} gfo_ast_kind;

typedef struct {
  gfo_ast_kind kind;
  gfo_u16 next;   /* linked list within a parent */
  gfo_u16 child;  /* first child */
  gfo_u16 a;      /* generic field (symbol ids etc) */
  gfo_u16 b;      /* generic field */
  gfo_value val;  /* for assignments, args, etc (optional) */
  gfo_str text;   /* for script blocks (slice) or raw lexeme */
  gfo_u32 line;
} gfo_ast_node;

typedef struct {
  gfo_ast_node* nodes;
  gfo_u16       cap;
  gfo_u16       count;
  gfo_u16       root; /* node index */
} gfo_ast;

/* init with arena */
int gfo_ast_init(gfo_ast* ast, gfo_arena* a, gfo_u16 capacity);

/* allocate node, returns index */
int gfo_ast_new(gfo_ast* ast, gfo_ast_kind kind, gfo_u16* out_idx);

/* append child */
void gfo_ast_add_child(gfo_ast* ast, gfo_u16 parent, gfo_u16 child);

#endif
