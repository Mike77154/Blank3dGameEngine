/* gfo_ast.c */
#include "gfo_ast.h"

int gfo_ast_init(gfo_ast* ast, gfo_arena* a, gfo_u16 capacity){
  gfo_u16 i;
  if(!ast || !a) return GFO_ERR_RANGE;
  ast->nodes = (gfo_ast_node*)gfo_arena_alloc(a, (gfo_u32)capacity * (gfo_u32)sizeof(gfo_ast_node), 4);
  if(!ast->nodes) return GFO_ERR_OOM;
  ast->cap = capacity;
  ast->count = 0;
  ast->root = 0;
  for(i=0;i<capacity;i++){
    ast->nodes[i].kind = GFO_AST_NONE;
    ast->nodes[i].next = 0xFFFF;
    ast->nodes[i].child = 0xFFFF;
    ast->nodes[i].a = 0;
    ast->nodes[i].b = 0;
    ast->nodes[i].val.t = GFO_VAL_NONE;
    ast->nodes[i].text.ptr = GFO_NULL;
    ast->nodes[i].text.len = 0;
    ast->nodes[i].line = 0;
  }
  return GFO_OK;
}

int gfo_ast_new(gfo_ast* ast, gfo_ast_kind kind, gfo_u16* out_idx){
  gfo_u16 idx;
  if(!ast || !out_idx) return GFO_ERR_RANGE;
  if(ast->count >= ast->cap) return GFO_ERR_OOM;
  idx = ast->count++;
  ast->nodes[idx].kind = kind;
  ast->nodes[idx].next = 0xFFFF;
  ast->nodes[idx].child = 0xFFFF;
  ast->nodes[idx].a = 0;
  ast->nodes[idx].b = 0;
  ast->nodes[idx].val.t = GFO_VAL_NONE;
  ast->nodes[idx].text.ptr = GFO_NULL;
  ast->nodes[idx].text.len = 0;
  ast->nodes[idx].line = 0;
  *out_idx = idx;
  return GFO_OK;
}

void gfo_ast_add_child(gfo_ast* ast, gfo_u16 parent, gfo_u16 child){
  gfo_u16* p;
  if(!ast) return;
  p = &ast->nodes[parent].child;
  if(*p == 0xFFFF){
    *p = child;
  } else {
    gfo_u16 n = *p;
    while(ast->nodes[n].next != 0xFFFF) n = ast->nodes[n].next;
    ast->nodes[n].next = child;
  }
}
