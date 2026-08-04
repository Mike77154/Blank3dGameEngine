#ifndef BVH_ARENA_H_INCLUDED
#define BVH_ARENA_H_INCLUDED

#include "bvh_common.h"

void bvh_arena_reset(BVH_Arena *arena);
int bvh_arena_put(BVH_Arena *arena, const char *s, unsigned long n, unsigned long *out_off);
const char *bvh_arena_at(const BVH_Arena *arena, unsigned long off);

#endif
