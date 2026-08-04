/* gfo_sym.c */
#include "gfo_sym.h"

int gfo_sym_init(gfo_symtab* st, gfo_arena* a, gfo_u16 capacity){
  gfo_u16 i;
  if(!st || !a) return GFO_ERR_RANGE;
  st->entries = (gfo_sym_entry*)gfo_arena_alloc(a, (gfo_u32)capacity * (gfo_u32)sizeof(gfo_sym_entry), 4);
  if(!st->entries) return GFO_ERR_OOM;
  st->cap = capacity;
  st->count = 0;
  for(i=0;i<capacity;i++){
    st->entries[i].used = 0;
    st->entries[i].hash = 0;
    st->entries[i].id = 0;
    st->entries[i].name.ptr = GFO_NULL;
    st->entries[i].name.len = 0;
  }
  return GFO_OK;
}

static gfo_u16 probe_index(gfo_u32 h, gfo_u16 cap){
  return (gfo_u16)(h % (gfo_u32)cap);
}

int gfo_sym_intern(gfo_symtab* st, gfo_str name, gfo_u16* out_id){
  gfo_u32 h;
  gfo_u16 idx, start, i;
  if(!st || !out_id) return GFO_ERR_RANGE;
  if(st->count + 1 >= st->cap) return GFO_ERR_OOM; /* keep it simple */
  h = gfo_hash_fnv1a(name);
  idx = probe_index(h, st->cap);
  start = idx;

  for(i=0;i<st->cap;i++){
    gfo_sym_entry* e = &st->entries[idx];
    if(!e->used){
      /* insert */
      e->used = 1;
      e->hash = h;
      e->name = name;
      e->id = st->count; /* 0.. */
      *out_id = e->id;
      st->count++;
      return GFO_OK;
    }
    if(e->hash == h && gfo_str_eq(e->name, name)){
      *out_id = e->id;
      return GFO_OK;
    }
    idx = (gfo_u16)((idx + 1) % st->cap);
    if(idx == start) break;
  }
  return GFO_ERR_OOM;
}

gfo_str gfo_sym_name_by_id(const gfo_symtab* st, gfo_u16 id){
  gfo_u16 i;
  gfo_str z; z.ptr=GFO_NULL; z.len=0;
  if(!st) return z;
  for(i=0;i<st->cap;i++){
    if(st->entries[i].used && st->entries[i].id == id){
      return st->entries[i].name;
    }
  }
  return z;
}
