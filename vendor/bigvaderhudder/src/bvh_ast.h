#ifndef BVH_AST_H_INCLUDED
#define BVH_AST_H_INCLUDED

#include "bvh_value.h"

int bvh_ast_add_item(BVH_Context *ctx, BVH_AstKind kind, int name_sym, int line, int col, BVH_Error *err);
int bvh_ast_add_prop(BVH_Context *ctx, int key_sym, int raw_sym, int line, int col, BVH_Error *err);
void bvh_ast_append_item(BVH_Context *ctx, int *first, int *last, int item_idx);
void bvh_ast_append_prop(BVH_Context *ctx, int item_idx, int prop_idx);

#endif
