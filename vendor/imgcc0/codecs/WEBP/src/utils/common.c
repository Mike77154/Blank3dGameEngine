#include "common.h"

GWPBool GWPAddU32(GWPu32 a, GWPu32 b, GWPu32* out) {
  GWPu32 r;
  if (out == 0) return GWP_FALSE;
  r = a + b;
  if (r < a) return GWP_FALSE;
  *out = r;
  return GWP_TRUE;
}

GWPBool GWPMulU32(GWPu32 a, GWPu32 b, GWPu32* out) {
  if (out == 0) return GWP_FALSE;
  if (a == 0u || b == 0u) {
    *out = 0u;
    return GWP_TRUE;
  }
  if (a > 0xffffffffu / b) return GWP_FALSE;
  *out = a * b;
  return GWP_TRUE;
}

void GWPZero(void* dst, GWPu32 size) {
  GWPu8* p;
  GWPu32 i;
  if (dst == 0) return;
  p = (GWPu8*)dst;
  for (i = 0; i < size; ++i) p[i] = 0u;
}

void GWPCopy(void* dst, const void* src, GWPu32 size) {
  GWPu8* d;
  const GWPu8* s;
  GWPu32 i;
  if (dst == 0 || src == 0) return;
  d = (GWPu8*)dst;
  s = (const GWPu8*)src;
  for (i = 0; i < size; ++i) d[i] = s[i];
}
