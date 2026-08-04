/* gfo_bc.h - packed bytecode for runtime dispatch */
#ifndef GFO_BC_H
#define GFO_BC_H
#include "gfo_ir.h"

/* Bytecode opcodes (compact, fixed-size instruction for PS1 style) */
typedef enum {
  GFO_BC_NOP=0,
  GFO_BC_TYPE_BEGIN,   /* a=type_sym, b=inst_sym(FFFF=archetype) */
  GFO_BC_TYPE_END,
  GFO_BC_PROP_SET,     /* a=key_sym, val=... */
  GFO_BC_LC_BEGIN,     /* a=lc_sym */
  GFO_BC_LC_END,
  GFO_BC_CALL_INV,     /* a=inv_sym, b=argc, followed by argc ARG instructions */
  GFO_BC_CALL_HND,     /* a=handler_sym */
  GFO_BC_ARG,          /* val=... */
  GFO_BC_SCRIPT_BLOCK  /* a=lang_sym, text=slice */
} gfo_bc_op;

typedef struct {
  gfo_bc_op op;
  gfo_u16   a;
  gfo_u16   b;
  gfo_value val;
  gfo_str   text;
} gfo_bc_ins;

typedef struct {
  gfo_bc_ins* code;
  gfo_u16     cap;
  gfo_u16     count;
} gfo_bc;

int gfo_bc_init(gfo_bc* bc, gfo_arena* a, gfo_u16 capacity);
int gfo_bc_emit(gfo_bc* bc, gfo_bc_ins ins);

/* assemble IR -> BC (1:1 mapping here, but separated for future optimizations) */
int gfo_bc_from_ir(gfo_bc* bc, const gfo_ir* ir);

#endif
