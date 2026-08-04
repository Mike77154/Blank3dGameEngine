/* gfo_bc.c */
#include "gfo_bc.h"

int gfo_bc_init(gfo_bc* bc, gfo_arena* a, gfo_u16 capacity){
  if(!bc || !a) return GFO_ERR_RANGE;
  bc->code = (gfo_bc_ins*)gfo_arena_alloc(a, (gfo_u32)capacity * (gfo_u32)sizeof(gfo_bc_ins), 4);
  if(!bc->code) return GFO_ERR_OOM;
  bc->cap = capacity;
  bc->count = 0;
  return GFO_OK;
}

int gfo_bc_emit(gfo_bc* bc, gfo_bc_ins ins){
  if(!bc) return GFO_ERR_RANGE;
  if(bc->count >= bc->cap) return GFO_ERR_OOM;
  bc->code[bc->count++] = ins;
  return GFO_OK;
}

int gfo_bc_from_ir(gfo_bc* bc, const gfo_ir* ir){
  gfo_u16 i;
  for(i=0;i<ir->count;i++){
    gfo_ir_ins in = ir->code[i];
    gfo_bc_ins out;
    out.a=in.a; out.b=in.b; out.val=in.val; out.text=in.text;
    switch(in.op){
      case GFO_IR_BEGIN_TYPE: out.op = GFO_BC_TYPE_BEGIN; break;
      case GFO_IR_END_TYPE: out.op = GFO_BC_TYPE_END; break;
      case GFO_IR_SET_PROP: out.op = GFO_BC_PROP_SET; break;
      case GFO_IR_BEGIN_LC: out.op = GFO_BC_LC_BEGIN; break;
      case GFO_IR_END_LC: out.op = GFO_BC_LC_END; break;
      case GFO_IR_CALL_INV: out.op = GFO_BC_CALL_INV; break;
      case GFO_IR_CALL_HND: out.op = GFO_BC_CALL_HND; break;
      case GFO_IR_ARG: out.op = GFO_BC_ARG; break;
      case GFO_IR_SCRIPT_BLOCK: out.op = GFO_BC_SCRIPT_BLOCK; break;
      default: out.op = GFO_BC_NOP; break;
    }
    if(gfo_bc_emit(bc, out)!=GFO_OK) return GFO_ERR_OOM;
  }
  return GFO_OK;
}
