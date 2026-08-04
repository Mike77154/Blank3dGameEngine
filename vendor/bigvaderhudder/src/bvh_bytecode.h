#ifndef BVH_BYTECODE_H_INCLUDED
#define BVH_BYTECODE_H_INCLUDED

#include "bvh_ir.h"

enum BVH_Opcode {
    BVH_OP_BEGIN = 1,
    BVH_OP_END = 2,
    BVH_OP_PROP = 3
};

int bvh_bytecode_reset(BVH_Bytecode *bc);
int bvh_bytecode_emit_u8(BVH_Bytecode *bc, int v);
int bvh_bytecode_emit_u16(BVH_Bytecode *bc, unsigned long v);
int bvh_bytecode_emit_u32(BVH_Bytecode *bc, unsigned long v);
int bvh_bytecode_emit_s32(BVH_Bytecode *bc, long v);
int bvh_bytecode_emit_bytes(BVH_Bytecode *bc, const unsigned char *p, unsigned long n);
int bvh_bytecode_emit_symbol(BVH_Context *ctx, BVH_Bytecode *bc, int sym, BVH_Error *err);

#endif
