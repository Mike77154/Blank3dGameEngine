/* flags_fx.h - fixed-point helpers (Q16.16) - C89 */
#ifndef FLAGS_FX_H
#define FLAGS_FX_H

#include "flags_config.h"

/* Signed fixed-point Q16.16 stored in a 'long'. */
typedef long flags_fx_t;

#define FLAGS_FX_FRAC_BITS 16L
#define FLAGS_FX_ONE       (1L << FLAGS_FX_FRAC_BITS)

#define FLAGS_FX_FROM_INT(x) ((flags_fx_t)((long)(x) << FLAGS_FX_FRAC_BITS))
#define FLAGS_FX_TO_INT(x)   ((long)((flags_fx_t)(x) >> FLAGS_FX_FRAC_BITS))

/* Convert from an integer scaled by 10^scale to fixed-point.
 * Example: value=12345, scale=3 represents 12.345
 */
flags_fx_t flags_fx_from_scaled(long value, int scale);

/* Parse ASCII number into fixed-point.
 * Supports: [+|-]?[0-9]+(.[0-9]+)?
 * Returns 1 on success, 0 on failure.
 */
int flags_fx_parse(const char *s, int len, flags_fx_t *out_fx);

/* Compare helpers */
int flags_fx_lt(flags_fx_t a, flags_fx_t b);
int flags_fx_gt(flags_fx_t a, flags_fx_t b);

/* Clamp helper */
flags_fx_t flags_fx_clamp(flags_fx_t x, int has_lo, flags_fx_t lo, int has_hi, flags_fx_t hi);

#endif /* FLAGS_FX_H */
