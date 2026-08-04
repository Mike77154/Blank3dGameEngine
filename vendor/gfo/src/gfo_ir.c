/* gfo_ir.c */
#include "gfo_ir.h"

int gfo_ir_init(gfo_ir* ir, gfo_arena* a, gfo_u16 capacity){
  if(!ir || !a) return GFO_ERR_RANGE;
  ir->code = (gfo_ir_ins*)gfo_arena_alloc(a, (gfo_u32)capacity * (gfo_u32)sizeof(gfo_ir_ins), 4);
  if(!ir->code) return GFO_ERR_OOM;
  ir->cap = capacity;
  ir->count = 0;
  return GFO_OK;
}

int gfo_ir_emit(gfo_ir* ir, gfo_ir_ins ins){
  if(!ir) return GFO_ERR_RANGE;
  if(ir->count >= ir->cap) return GFO_ERR_OOM;
  ir->code[ir->count++] = ins;
  return GFO_OK;
}

static int emit_args(gfo_ir* ir, const gfo_ast* ast, gfo_u16 inv_node){
  /* args are AST children of inv_node as ASSIGN nodes with val */
  gfo_u16 a = ast->nodes[inv_node].child;
  while(a != 0xFFFF){
    gfo_ir_ins ins; ins.op=GFO_IR_ARG; ins.a=0; ins.b=0; ins.val=ast->nodes[a].val; ins.text.ptr=GFO_NULL; ins.text.len=0; ins.line=ast->nodes[a].line;
    if(gfo_ir_emit(ir, ins)!=GFO_OK) return GFO_ERR_OOM;
    a = ast->nodes[a].next;
  }
  return GFO_OK;
}

int gfo_ir_from_ast(gfo_ir* ir, const gfo_ast* ast){
  gfo_u16 sec = ast->nodes[ast->root].child;
  while(sec != 0xFFFF){
    gfo_ir_ins begin; begin.op=GFO_IR_BEGIN_TYPE; begin.a=ast->nodes[sec].a; begin.b=ast->nodes[sec].b; begin.val.t=GFO_VAL_NONE; begin.text.ptr=GFO_NULL; begin.text.len=0; begin.line=ast->nodes[sec].line;
    if(gfo_ir_emit(ir, begin)!=GFO_OK) return GFO_ERR_OOM;

    /* walk children: assignments and lifecycles */
    {
      gfo_u16 st = ast->nodes[sec].child;
      while(st != 0xFFFF){
        gfo_ast_node n = ast->nodes[st];
        if(n.kind == GFO_AST_ASSIGN){
          gfo_ir_ins s; s.op=GFO_IR_SET_PROP; s.a=n.a; s.b=0; s.val=n.val; s.text.ptr=GFO_NULL; s.text.len=0; s.line=n.line;
          if(gfo_ir_emit(ir, s)!=GFO_OK) return GFO_ERR_OOM;
        } else if(n.kind == GFO_AST_LIFECYCLE){
          gfo_ir_ins bl; bl.op=GFO_IR_BEGIN_LC; bl.a=n.a; bl.b=0; bl.val.t=GFO_VAL_NONE; bl.text.ptr=GFO_NULL; bl.text.len=0; bl.line=n.line;
          if(gfo_ir_emit(ir, bl)!=GFO_OK) return GFO_ERR_OOM;

          /* actions inside lifecycle */
          {
            gfo_u16 act = n.child;
            while(act != 0xFFFF){
              gfo_ast_node a = ast->nodes[act];
              if(a.kind == GFO_AST_HANDLER){
                gfo_ir_ins h; h.op=GFO_IR_CALL_HND; h.a=a.a; h.b=0; h.val.t=GFO_VAL_NONE; h.text.ptr=GFO_NULL; h.text.len=0; h.line=a.line;
                if(gfo_ir_emit(ir, h)!=GFO_OK) return GFO_ERR_OOM;
              } else if(a.kind == GFO_AST_INVOKER){
                gfo_ir_ins c; c.op=GFO_IR_CALL_INV; c.a=a.a; c.b=a.b; c.val.t=GFO_VAL_NONE; c.text.ptr=GFO_NULL; c.text.len=0; c.line=a.line;
                if(gfo_ir_emit(ir, c)!=GFO_OK) return GFO_ERR_OOM;
                if(emit_args(ir, ast, act)!=GFO_OK) return GFO_ERR_OOM;
              } else if(a.kind == GFO_AST_ASSIGN){
                /* allow script = path inside lifecycle or other props; treat same as SET_PROP for now */
                gfo_ir_ins s2; s2.op=GFO_IR_SET_PROP; s2.a=a.a; s2.b=1; s2.val=a.val; s2.text.ptr=GFO_NULL; s2.text.len=0; s2.line=a.line;
                if(gfo_ir_emit(ir, s2)!=GFO_OK) return GFO_ERR_OOM;
              } else if(a.kind == GFO_AST_SCRIPT_BLOCK){
                gfo_ir_ins sb; sb.op=GFO_IR_SCRIPT_BLOCK; sb.a=a.a; sb.b=0; sb.val.t=GFO_VAL_NONE; sb.text=a.text; sb.line=a.line;
                if(gfo_ir_emit(ir, sb)!=GFO_OK) return GFO_ERR_OOM;
              }
              act = a.next;
            }
          }

          {
            gfo_ir_ins el; el.op=GFO_IR_END_LC; el.a=0; el.b=0; el.val.t=GFO_VAL_NONE; el.text.ptr=GFO_NULL; el.text.len=0; el.line=n.line;
            if(gfo_ir_emit(ir, el)!=GFO_OK) return GFO_ERR_OOM;
          }
        }

        st = n.next;
      }
    }

    {
      gfo_ir_ins end; end.op=GFO_IR_END_TYPE; end.a=0; end.b=0; end.val.t=GFO_VAL_NONE; end.text.ptr=GFO_NULL; end.text.len=0; end.line=ast->nodes[sec].line;
      if(gfo_ir_emit(ir, end)!=GFO_OK) return GFO_ERR_OOM;
    }

    sec = ast->nodes[sec].next;
  }
  return GFO_OK;
}
