/* gfo_sym.h - symbol table (interned names -> u16 id), no malloc */
#ifndef GFO_SYM_H
#define GFO_SYM_H
#include "gfo_common.h"

typedef struct {
  gfo_u32 hash;
  gfo_str name;   /* slice into source (caller keeps alive) */
  gfo_u16 id;
  gfo_u8  used;
} gfo_sym_entry;

typedef struct {
  gfo_sym_entry* entries;
  gfo_u16        cap;
  gfo_u16        count;
} gfo_symtab;

/* initialize with pre-allocated entries array in arena */
int gfo_sym_init(gfo_symtab* st, gfo_arena* a, gfo_u16 capacity);

/* get or add symbol, returns id in *out_id */
int gfo_sym_intern(gfo_symtab* st, gfo_str name, gfo_u16* out_id);

/* lookup by id (O(n) in this simple impl). For debug/transpile. */
gfo_str gfo_sym_name_by_id(const gfo_symtab* st, gfo_u16 id);

#endif
