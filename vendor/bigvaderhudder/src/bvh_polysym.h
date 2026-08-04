#ifndef BVH_POLYSYM_H_INCLUDED
#define BVH_POLYSYM_H_INCLUDED

#include "bvh_arena.h"

void bvh_polysym_reset(BVH_Context *ctx);
int bvh_symbol_intern_n(BVH_Context *ctx, const char *s, unsigned long n, BVH_Error *err);
int bvh_symbol_intern(BVH_Context *ctx, const char *s, BVH_Error *err);
unsigned long bvh_symbol_hash(const char *s, unsigned long n);

#endif
