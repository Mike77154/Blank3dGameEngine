#ifndef BVH_IR_H_INCLUDED
#define BVH_IR_H_INCLUDED

#include "bvh_ast.h"

int bvh_ir_emit(BVH_Context *ctx, BVH_IrCode code, int entity_kind, int name_sym, int key_sym, int raw_sym, const BVH_Value *value, int line, int col, BVH_Error *err);

#endif
