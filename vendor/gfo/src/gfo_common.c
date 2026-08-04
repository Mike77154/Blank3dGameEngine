/* gfo_common.c */
#include "gfo_common.h"

int gfo_str_eq(gfo_str a, gfo_str b){
  gfo_u16 i;
  if(a.len != b.len) return 0;
  for(i=0;i<a.len;i++){
    if(a.ptr[i] != b.ptr[i]) return 0;
  }
  return 1;
}

gfo_u32 gfo_hash_fnv1a(gfo_str s){
  /* 32-bit FNV-1a */
  gfo_u32 h = 2166136261UL;
  gfo_u16 i;
  for(i=0;i<s.len;i++){
    h ^= (gfo_u8)s.ptr[i];
    h *= 16777619UL;
  }
  return h;
}

void* gfo_arena_alloc(gfo_arena* a, gfo_u32 bytes, gfo_u32 align){
  gfo_u32 p, aligned;
  if(!a || !a->base) return GFO_NULL;
  if(align == 0) align = 1;
  p = a->used;
  aligned = (p + (align - 1)) & ~(align - 1);
  if(aligned + bytes > a->size) return GFO_NULL;
  a->used = aligned + bytes;
  return (void*)(a->base + aligned);
}
