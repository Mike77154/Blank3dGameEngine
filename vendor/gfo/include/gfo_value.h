/* gfo_value.h */
#ifndef GFO_VALUE_H
#define GFO_VALUE_H
#include "gfo_common.h"

typedef enum {
  GFO_VAL_NONE=0,
  GFO_VAL_I32,
  GFO_VAL_BOOL,
  GFO_VAL_STR,
  GFO_VAL_PATH,
  GFO_VAL_SYM,
  GFO_VAL_UNKNOWN /* for '?' */
} gfo_valtype;

typedef struct {
  gfo_valtype t;
  union { gfo_i32 i; gfo_u32 b; gfo_str s; gfo_u16 sym; } v;
} gfo_value;

#endif
