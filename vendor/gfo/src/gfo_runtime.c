/* gfo_runtime.c */
#include "gfo_runtime.h"
#include "gfo_parser.h"

/* internal: add property */
static int add_prop(gfo_ctx* ctx, gfo_u16 key, gfo_value val){
  if(ctx->prop_count >= ctx->lim.max_props) return GFO_ERR_OOM;
  ctx->props[ctx->prop_count].key_sym = key;
  ctx->props[ctx->prop_count].val = val;
  ctx->prop_count++;
  return GFO_OK;
}

/* internal helpers */
static gfo_u32 gfo_align_u32(gfo_u32 p, gfo_u32 align){
  gfo_u32 a = align ? align : 1;
  return (p + (a - 1)) & ~(a - 1);
}
static gfo_u16 gfo_clamp_u16(gfo_u32 v){
  return (v > 0xFFFFU) ? (gfo_u16)0xFFFFU : (gfo_u16)v;
}

gfo_caps gfo_caps_default(gfo_limits lim){
  gfo_caps c;
  /* keep the defaults generous enough for typical DSL usage */
  c.sym_cap = gfo_clamp_u16((gfo_u32)lim.max_types * 8U + (gfo_u32)lim.max_props * 2U + 256U);
  c.ast_cap = gfo_clamp_u16((gfo_u32)lim.max_types * 64U + 256U);
  c.ir_cap  = gfo_clamp_u16((gfo_u32)lim.max_types * 256U + 512U);
  c.bc_cap  = gfo_clamp_u16((gfo_u32)lim.max_types * 256U + 512U);
  return c;
}

gfo_u32 gfo_estimate_arena_bytes(gfo_limits lim, gfo_caps caps){
  gfo_u32 used = 0;
  if(caps.sym_cap == 0 || caps.ast_cap == 0 || caps.ir_cap == 0 || caps.bc_cap == 0) return 0;
  /* mirror gfo_init_ex allocation order + alignment */
  used = gfo_align_u32(used, 4); used += (gfo_u32)caps.sym_cap * (gfo_u32)sizeof(gfo_sym_entry);
  used = gfo_align_u32(used, 4); used += (gfo_u32)caps.ast_cap * (gfo_u32)sizeof(gfo_ast_node);
  used = gfo_align_u32(used, 4); used += (gfo_u32)caps.ir_cap  * (gfo_u32)sizeof(gfo_ir_ins);
  used = gfo_align_u32(used, 4); used += (gfo_u32)caps.bc_cap  * (gfo_u32)sizeof(gfo_bc_ins);
  used = gfo_align_u32(used, 4); used += (gfo_u32)lim.max_types     * (gfo_u32)sizeof(gfo_type);
  used = gfo_align_u32(used, 4); used += (gfo_u32)lim.max_props     * (gfo_u32)sizeof(gfo_prop);
  used = gfo_align_u32(used, 4); used += (gfo_u32)lim.max_instances * (gfo_u32)sizeof(gfo_instance);
  return used;
}

int gfo_init_ex(gfo_ctx* ctx, void* mem, gfo_u32 mem_size, gfo_limits lim, gfo_caps caps, gfo_binds binds){
  gfo_u32 need;
  gfo_u16 i;

  if(!ctx || !mem || mem_size < 1024) return GFO_ERR_RANGE;
  if(caps.sym_cap == 0 || caps.ast_cap == 0 || caps.ir_cap == 0 || caps.bc_cap == 0) return GFO_ERR_RANGE;

  need = gfo_estimate_arena_bytes(lim, caps);
  if(need == 0) return GFO_ERR_RANGE;
  if(mem_size < need) return GFO_ERR_OOM;

  ctx->arena.base = (gfo_u8*)mem;
  ctx->arena.size = mem_size;
  ctx->arena.used = 0;

  ctx->lim = lim;
  ctx->binds = binds;

  /* Allocate fixed arrays from arena (no malloc). */
  if(gfo_sym_init(&ctx->sym, &ctx->arena, caps.sym_cap)!=GFO_OK) return GFO_ERR_OOM;
  if(gfo_ast_init(&ctx->ast, &ctx->arena, caps.ast_cap)!=GFO_OK) return GFO_ERR_OOM;
  if(gfo_ir_init(&ctx->ir, &ctx->arena, caps.ir_cap)!=GFO_OK) return GFO_ERR_OOM;
  if(gfo_bc_init(&ctx->bc, &ctx->arena, caps.bc_cap)!=GFO_OK) return GFO_ERR_OOM;

  ctx->types = (gfo_type*)gfo_arena_alloc(&ctx->arena, (gfo_u32)lim.max_types * (gfo_u32)sizeof(gfo_type), 4);
  if(!ctx->types) return GFO_ERR_OOM;
  ctx->props = (gfo_prop*)gfo_arena_alloc(&ctx->arena, (gfo_u32)lim.max_props * (gfo_u32)sizeof(gfo_prop), 4);
  if(!ctx->props) return GFO_ERR_OOM;
  ctx->inst = (gfo_instance*)gfo_arena_alloc(&ctx->arena, (gfo_u32)lim.max_instances * (gfo_u32)sizeof(gfo_instance), 4);
  if(!ctx->inst) return GFO_ERR_OOM;

  /* Deterministic instance slots even if caller memory isn't zeroed. */
  for(i=0;i<lim.max_instances;i++){
    ctx->inst[i].alive = 0;
    ctx->inst[i].type_index = 0;
    ctx->inst[i].self = 0;
  }

  ctx->type_count = 0;
  ctx->prop_count = 0;
  ctx->inst_count = 0;
  ctx->src = 0;
  ctx->src_len = 0;

  return GFO_OK;
}

int gfo_init(gfo_ctx* ctx, void* mem, gfo_u32 mem_size, gfo_limits lim, gfo_binds binds){
  gfo_caps caps = gfo_caps_default(lim);
  return gfo_init_ex(ctx, mem, mem_size, lim, caps, binds);
}

gfo_u16 gfo_lc_sym_id(gfo_ctx* ctx, const char* name){
  gfo_u16 id = 0;
  gfo_str s; s.ptr=name; s.len=(gfo_u16)0;
  while(name[s.len] != '\0' && s.len < 255) s.len++;
  gfo_sym_intern(&ctx->sym, s, &id);
  return id;
}

/* Build type table by scanning BC once. Records ip of each lifecycle. */
static int index_types(gfo_ctx* ctx){
  gfo_u16 ip=0;
  ctx->type_count = 0;
  ctx->prop_count = 0;

  while(ip < ctx->bc.count){
    gfo_bc_ins ins = ctx->bc.code[ip];
    if(ins.op == GFO_BC_TYPE_BEGIN){
      gfo_type* t;
      gfo_u16 ti;
      if(ctx->type_count >= ctx->lim.max_types) return GFO_ERR_OOM;
      ti = ctx->type_count++;
      t = &ctx->types[ti];
      t->type_sym = ins.a;
      t->inst_sym = ins.b;
      t->prop_start = ctx->prop_count;
      t->prop_count = 0;
      t->lc_ip[0]=t->lc_ip[1]=t->lc_ip[2]=t->lc_ip[3]=0xFFFF;

      ip++;

      /* scan until TYPE_END, collecting props and lifecycle starts */
      while(ip < ctx->bc.count){
        gfo_bc_ins x = ctx->bc.code[ip];
        if(x.op == GFO_BC_PROP_SET){
          /* b==0: section-level default prop. b!=0: lifecycle-level (reserved / not a type default). */
          if(x.b == 0){
            if(add_prop(ctx, x.a, x.val)!=GFO_OK) return GFO_ERR_OOM;
            t->prop_count++;
          }
        } else if(x.op == GFO_BC_LC_BEGIN){
          /* map lifecycle names by symbol spelling */
          gfo_str nm = gfo_sym_name_by_id(&ctx->sym, x.a);
          if(nm.ptr){
            /* compare to create/step/render/destroy */
            if(nm.len==6 && nm.ptr[0]=='c'&&nm.ptr[1]=='r'&&nm.ptr[2]=='e'&&nm.ptr[3]=='a'&&nm.ptr[4]=='t'&&nm.ptr[5]=='e') t->lc_ip[0]=ip;
            else if(nm.len==4 && nm.ptr[0]=='s'&&nm.ptr[1]=='t'&&nm.ptr[2]=='e'&&nm.ptr[3]=='p') t->lc_ip[1]=ip;
            else if(nm.len==6 && nm.ptr[0]=='r'&&nm.ptr[1]=='e'&&nm.ptr[2]=='n'&&nm.ptr[3]=='d'&&nm.ptr[4]=='e'&&nm.ptr[5]=='r') t->lc_ip[2]=ip;
            else if(nm.len==7 && nm.ptr[0]=='d'&&nm.ptr[1]=='e'&&nm.ptr[2]=='s'&&nm.ptr[3]=='t'&&nm.ptr[4]=='r'&&nm.ptr[5]=='o'&&nm.ptr[6]=='y') t->lc_ip[3]=ip;
          }
        } else if(x.op == GFO_BC_TYPE_END){
          ip++;
          break;
        }
        ip++;
      }
      continue;
    }
    ip++;
  }
  return GFO_OK;
}

int gfo_compile(gfo_ctx* ctx, const char* text, gfo_u32 text_len){
  int r;
  gfo_parser p;

  if(!ctx || !text) return GFO_ERR_RANGE;
  ctx->src = text;
  ctx->src_len = text_len;

  /* reset AST/IR/BC counts (reuse allocated buffers) */
  ctx->ast.count = 0;
  ctx->ir.count = 0;
  ctx->bc.count = 0;

  gfo_parse_init(&p, text, text_len, &ctx->sym, &ctx->ast);
  r = gfo_parse_file(&p);
  if(r != GFO_OK) return r;

  r = gfo_ir_from_ast(&ctx->ir, &ctx->ast);
  if(r != GFO_OK) return r;

  r = gfo_bc_from_ir(&ctx->bc, &ctx->ir);
  if(r != GFO_OK) return r;

  r = index_types(ctx);
  return r;
}

static int find_type_index(gfo_ctx* ctx, gfo_u16 type_sym){
  gfo_u16 i;
  for(i=0;i<ctx->type_count;i++){
    if(ctx->types[i].type_sym == type_sym && ctx->types[i].inst_sym == 0xFFFF){
      return (int)i;
    }
  }
  return -1;
}

int gfo_spawn(gfo_ctx* ctx, gfo_str type_name, gfo_self self, gfo_u16* out_inst_index){
  gfo_u16 sym_id;
  int ti;
  gfo_u16 i;

  if(!ctx) return GFO_ERR_RANGE;

  /* NOTE: this interns the name if it doesn't exist yet.
     If you want "lookup-only" semantics, add a gfo_sym_lookup() helper in your fork. */
  if(gfo_sym_intern(&ctx->sym, type_name, &sym_id)!=GFO_OK) return GFO_ERR_OOM;
  ti = find_type_index(ctx, sym_id);
  if(ti < 0) return GFO_ERR_RANGE;

  /* 1) reuse dead slots inside the active window */
  for(i=0;i<ctx->inst_count;i++){
    if(ctx->inst[i].alive == 0){
      ctx->inst[i].alive = 1;
      ctx->inst[i].type_index = (gfo_u16)ti;
      ctx->inst[i].self = self;
      if(out_inst_index) *out_inst_index = i;
      /* call create */
      gfo_tick_one(ctx, i, GFO_LC_CREATE);
      return GFO_OK;
    }
  }

  /* 2) append a fresh slot if we still have capacity */
  if(ctx->inst_count < ctx->lim.max_instances){
    i = ctx->inst_count;
    ctx->inst_count = (gfo_u16)(ctx->inst_count + 1);
    ctx->inst[i].alive = 1;
    ctx->inst[i].type_index = (gfo_u16)ti;
    ctx->inst[i].self = self;
    if(out_inst_index) *out_inst_index = i;
    /* call create */
    gfo_tick_one(ctx, i, GFO_LC_CREATE);
    return GFO_OK;
  }

  return GFO_ERR_OOM;
}

int gfo_kill(gfo_ctx* ctx, gfo_u16 inst_index){
  if(!ctx || inst_index >= ctx->inst_count) return GFO_ERR_RANGE;
  if(!ctx->inst[inst_index].alive) return GFO_OK;
  gfo_tick_one(ctx, inst_index, GFO_LC_DESTROY);
  ctx->inst[inst_index].alive = 0;
  return GFO_OK;
}

static void run_lc(gfo_ctx* ctx, gfo_u16 ip_start, gfo_self self){
  gfo_u16 ip = ip_start;
  /* expect LC_BEGIN at ip_start; scan until LC_END */
  if(ip == 0xFFFF) return;
  if(ip >= ctx->bc.count) return;
  if(ctx->bc.code[ip].op != GFO_BC_LC_BEGIN) return;
  ip++;

  while(ip < ctx->bc.count){
    gfo_bc_ins ins = ctx->bc.code[ip];
    if(ins.op == GFO_BC_LC_END) break;

    if(ins.op == GFO_BC_CALL_HND){
      gfo_u16 hid = ctx->binds.resolve_handler ? ctx->binds.resolve_handler(ctx->binds.user, ins.a) : ins.a;
      if(ctx->binds.call_handler) ctx->binds.call_handler(ctx->binds.user, hid, self);
    } else if(ins.op == GFO_BC_CALL_INV){
      gfo_u8 argc = (gfo_u8)ins.b;
      gfo_u16 inv_id = ctx->binds.resolve_invoker ? ctx->binds.resolve_invoker(ctx->binds.user, ins.a) : ins.a;
      gfo_value argv_local[16];
      gfo_u8 j;
      if(argc > 16) argc = 16; /* hard cap; keep PS1 friendly */
      for(j=0;j<argc;j++){
        if(ip + 1 + j >= ctx->bc.count) break;
        argv_local[j] = ctx->bc.code[ip + 1 + j].val;
      }
      if(ctx->binds.call_invoker) ctx->binds.call_invoker(ctx->binds.user, inv_id, self, argv_local, argc);
      ip = (gfo_u16)(ip + argc); /* skip arg instructions too; plus loop ip++ below */
    } else if(ins.op == GFO_BC_SCRIPT_BLOCK){
      gfo_u16 lang = ctx->binds.resolve_lang ? ctx->binds.resolve_lang(ctx->binds.user, ins.a) : ins.a;
      if(ctx->binds.call_script_block) ctx->binds.call_script_block(ctx->binds.user, lang, self, ins.text);
    }
    ip++;
  }
}

void gfo_tick_one(gfo_ctx* ctx, gfo_u16 inst_index, gfo_lifecycle lc){
  gfo_instance* in;
  gfo_type* t;
  if(!ctx || inst_index >= ctx->inst_count) return;
  in = &ctx->inst[inst_index];
  if(!in->alive) return;
  t = &ctx->types[in->type_index];
  run_lc(ctx, t->lc_ip[(gfo_u16)lc], in->self);
}

void gfo_tick_all(gfo_ctx* ctx, gfo_lifecycle lc){
  gfo_u16 i;
  if(!ctx) return;
  for(i=0;i<ctx->inst_count;i++){
    if(ctx->inst[i].alive) gfo_tick_one(ctx, i, lc);
  }
}
