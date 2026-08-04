#ifndef BVH_VALUE_H_INCLUDED
#define BVH_VALUE_H_INCLUDED

#include "bvh_polysym.h"

void bvh_value_empty(BVH_Value *v);
int bvh_value_from_raw(BVH_Context *ctx, int key_sym, int raw_sym, BVH_Value *out);
int bvh_parse_fixed_text(const char *s, BVH_Fixed *out);
int bvh_parse_long_text(const char *s, long *out);

#endif
