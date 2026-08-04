#ifndef RPYL_FIXED_H
#define RPYL_FIXED_H

#include "rpyl_config.h"
#include "rpyl_types.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Signed 16.16 value with an explicitly 32-bit representation. */
typedef rpyl_i32 rpyl_fx;

rpyl_fx rpyl_fx_from_int(long v);
rpyl_fx rpyl_fx_from_decimal_text(const char* text, int* ok);
int rpyl_fx_parse(const char* text, rpyl_fx* out_value);
long rpyl_fx_to_int(rpyl_fx v);
rpyl_fx rpyl_fx_add(rpyl_fx a, rpyl_fx b);
rpyl_fx rpyl_fx_sub(rpyl_fx a, rpyl_fx b);
rpyl_fx rpyl_fx_mul(rpyl_fx a, rpyl_fx b);
rpyl_fx rpyl_fx_div(rpyl_fx a, rpyl_fx b, int* ok);

#ifdef __cplusplus
}
#endif

#endif
