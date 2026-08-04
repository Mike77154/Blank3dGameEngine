/* gfo_ir.h - intermediate representation (linear) */
#ifndef GFO_IR_H
#define GFO_IR_H
#include "gfo_sym.h"
#include "gfo_ast.h"

typedef enum {
  GFO_IR_NOP=0,
  GFO_IR_BEGIN_TYPE,      /* a=type_sym, b=inst_sym_or_FFFF */
  GFO_IR_SET_PROP,        /* a=key_sym, val=... */
  GFO_IR_BEGIN_LC,        /* a=lc_sym */
  GFO_IR_CALL_INV,        /* a=inv_sym, b=argc, child args are in following IR as GFO_IR_ARG */
  GFO_IR_CALL_HND,        /* a=handler_sym */
  GFO_IR_SCRIPT_PATH,     /* val=PATH slice (in val.v.s) */
  GFO_IR_SCRIPT_BLOCK,    /* a=lang_sym, text=slice */
  GFO_IR_END_LC,
  GFO_IR_END_TYPE,
  GFO_IR_ARG              /* val=... */
} gfo_ir_op;

typedef struct {
  gfo_ir_op op;
  gfo_u16   a;
  gfo_u16   b;
  gfo_value val;
  gfo_str   text;
  gfo_u32   line;
} gfo_ir_ins;

typedef struct {
  gfo_ir_ins* code;
  gfo_u16     cap;
  gfo_u16     count;
} gfo_ir;

int gfo_ir_init(gfo_ir* ir, gfo_arena* a, gfo_u16 capacity);
int gfo_ir_emit(gfo_ir* ir, gfo_ir_ins ins);

/* lower AST -> IR */
int gfo_ir_from_ast(gfo_ir* ir, const gfo_ast* ast);

#endif
