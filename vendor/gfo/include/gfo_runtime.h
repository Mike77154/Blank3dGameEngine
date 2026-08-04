/* gfo_runtime.h - runtime program + instance system */
#ifndef GFO_RUNTIME_H
#define GFO_RUNTIME_H
#include "gfo_bc.h"

typedef gfo_u32 gfo_self;

/* Engine bind callbacks */
typedef struct {
  void* user;

  /* Optional: resolve symbol name -> binding id (engine-specific) */
  gfo_u16 (*resolve_invoker)(void* user, gfo_u16 sym_id);
  gfo_u16 (*resolve_handler)(void* user, gfo_u16 sym_id);
  gfo_u16 (*resolve_lang)(void* user, gfo_u16 sym_id);

  /* Callbacks */
  void (*call_invoker)(void* user, gfo_u16 inv_id, gfo_self self, const gfo_value* argv, gfo_u8 argc);
  void (*call_handler)(void* user, gfo_u16 hnd_id, gfo_self self);
  void (*call_script_block)(void* user, gfo_u16 lang_id, gfo_self self, gfo_str code);
} gfo_binds;

/* Lifecycle enum: resolved from symbol names in your engine.
   Default mapping supported by helper: create/step/render/destroy */
typedef enum { GFO_LC_CREATE=0, GFO_LC_STEP, GFO_LC_RENDER, GFO_LC_DESTROY } gfo_lifecycle;

/* Limits */
typedef struct {
  gfo_u16 max_types;
  gfo_u16 max_instances;
  gfo_u16 max_props;
} gfo_limits;

/* Internal buffer caps (advanced).
   These control how much arena memory is reserved for compiler/runtime internals.
   Use gfo_caps_default(lim) to match the built-in heuristics, or override to shrink/expand.
*/
typedef struct {
  gfo_u16 sym_cap;  /* symbol table entries (hash slots) */
  gfo_u16 ast_cap;  /* AST nodes */
  gfo_u16 ir_cap;   /* IR instructions */
  gfo_u16 bc_cap;   /* bytecode instructions */
} gfo_caps;

/* Compute default caps from limits (matches library heuristics). */
gfo_caps gfo_caps_default(gfo_limits lim);

/* Exact arena bytes needed for a given configuration (includes alignment). */
gfo_u32 gfo_estimate_arena_bytes(gfo_limits lim, gfo_caps caps);

/* Property stored on types or instances */
typedef struct {
  gfo_u16 key_sym;
  gfo_value val;
} gfo_prop;

/* Compiled type info */
typedef struct {
  gfo_u16 type_sym;
  gfo_u16 inst_sym;      /* FFFF means archetype, else instance-id override block */
  gfo_u16 prop_start;
  gfo_u16 prop_count;
  gfo_u16 lc_ip[4];      /* instruction pointer into bc for each lifecycle (start of LC block) or FFFF */
} gfo_type;

/* Runtime instance */
typedef struct {
  gfo_u8  alive;
  gfo_u16 type_index;   /* points into types[] */
  gfo_self self;
} gfo_instance;

typedef struct {
  gfo_arena arena;

  gfo_limits lim;
  gfo_symtab sym;
  gfo_ast ast;
  gfo_ir  ir;
  gfo_bc  bc;

  gfo_type* types;
  gfo_u16   type_count;

  gfo_prop* props;
  gfo_u16   prop_count;

  gfo_instance* inst;
  gfo_u16       inst_count;

  gfo_binds binds;

  const char* src;
  gfo_u32     src_len;
} gfo_ctx;

/* Initialize with user memory */
/* Initialize with explicit caps (advanced). */
int gfo_init_ex(gfo_ctx* ctx, void* mem, gfo_u32 mem_size, gfo_limits lim, gfo_caps caps, gfo_binds binds);

/* Initialize with default caps derived from lim (simple). */
int gfo_init(gfo_ctx* ctx, void* mem, gfo_u32 mem_size, gfo_limits lim, gfo_binds binds);

/* Compile from DSL text (text must remain alive if you want slices for scripts) */
int gfo_compile(gfo_ctx* ctx, const char* text, gfo_u32 text_len);

/* Spawn: by type symbol name (slice) */
int gfo_spawn(gfo_ctx* ctx, gfo_str type_name, gfo_self self, gfo_u16* out_inst_index);

/* Kill: dispatch destroy then mark free */
int gfo_kill(gfo_ctx* ctx, gfo_u16 inst_index);

/* Tick */
void gfo_tick_one(gfo_ctx* ctx, gfo_u16 inst_index, gfo_lifecycle lc);
void gfo_tick_all(gfo_ctx* ctx, gfo_lifecycle lc);

/* Helpers: map common lifecycle names */
gfo_u16 gfo_lc_sym_id(gfo_ctx* ctx, const char* name); /* interns name and returns sym id */

#endif
