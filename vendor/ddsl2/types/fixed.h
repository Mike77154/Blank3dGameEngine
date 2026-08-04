#ifndef DDSL_FIXED_H
#define DDSL_FIXED_H

#include <stddef.h>
#include "common/strview.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef DDSL_FIXED_SCALE
#define DDSL_FIXED_SCALE 1000L
#endif

#ifndef DDSL_FIXED_FRAC_DIGITS
#define DDSL_FIXED_FRAC_DIGITS 3
#endif

typedef long ddsl_fixed;

#define DDSL_FIXED_ZERO ((ddsl_fixed)0L)
#define DDSL_FIXED_ONE  ((ddsl_fixed)DDSL_FIXED_SCALE)

ddsl_fixed ddsl_fixed_from_int(long v);
long ddsl_fixed_to_int(ddsl_fixed v);

ddsl_fixed ddsl_fixed_neg(ddsl_fixed a);
ddsl_fixed ddsl_fixed_add(ddsl_fixed a, ddsl_fixed b);
ddsl_fixed ddsl_fixed_sub(ddsl_fixed a, ddsl_fixed b);
ddsl_fixed ddsl_fixed_mul(ddsl_fixed a, ddsl_fixed b);
ddsl_fixed ddsl_fixed_div(ddsl_fixed a, ddsl_fixed b);

int ddsl_fixed_parse_cstr(const char *s, ddsl_fixed *out);
int ddsl_fixed_parse_sv(ddsl_strview sv, ddsl_fixed *out);
int ddsl_fixed_to_cstr(ddsl_fixed v, char *dst, int dst_cap);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_FIXED_H */
