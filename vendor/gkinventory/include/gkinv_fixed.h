#ifndef GKINV_FIXED_H
#define GKINV_FIXED_H

#include "gkinv_types.h"

/* Signed Q16.16 fixed point by default. Integer-only helpers. */
typedef gkinv_i32 gkinv_fx;

#define GKINV_FX_ONE ((gkinv_fx)(1L << GKINV_FX_SHIFT))
#define GKINV_FX_ZERO ((gkinv_fx)0L)
#define GKINV_FX_MAX ((gkinv_fx)2147483647L)
#define GKINV_FX_MIN ((gkinv_fx)(-2147483647L - 1L))

#define GKINV_FX_FROM_INT(x) ((gkinv_fx)((gkinv_i32)(x) << GKINV_FX_SHIFT))
#define GKINV_FX_TO_INT_FLOOR(x) ((gkinv_i32)((x) >> GKINV_FX_SHIFT))

gkinv_fx gkinv_fx_from_int(gkinv_i32 value);
gkinv_i32 gkinv_fx_to_int_floor(gkinv_fx value);
gkinv_fx gkinv_fx_from_ratio(gkinv_i32 numerator, gkinv_i32 denominator);
gkinv_fx gkinv_fx_add_sat(gkinv_fx a, gkinv_fx b);
gkinv_fx gkinv_fx_sub_sat(gkinv_fx a, gkinv_fx b);
gkinv_fx gkinv_fx_mul_int_sat(gkinv_fx value, gkinv_u16 count);

#endif
