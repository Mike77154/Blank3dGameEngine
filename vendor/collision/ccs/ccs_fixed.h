#ifndef CCS_FIXED_H
#define CCS_FIXED_H

#include "ccs_types.h"

/* ============================================================
   Fixed-point configuration
   ============================================================ */

/* Q16.16 fixed-point */
typedef ccs_i32 ccs_fixed;

#define CCS_FIXED_SHIFT   16
#define CCS_FIXED_ONE     ((ccs_fixed)(1 << CCS_FIXED_SHIFT))
#define CCS_FIXED_HALF    ((ccs_fixed)(1 << (CCS_FIXED_SHIFT - 1)))

/* ============================================================
   Constructors & conversion (C89-safe)
   ============================================================ */

/* Convert integer <-> fixed without relying on UB (e.g. left-shifting negatives).
   The conversion functions saturate on extreme inputs. */
ccs_fixed ccs_fixed_from_int(ccs_i32 i);

/* Truncate toward zero (deterministic, independent of C89 implementation quirks). */
ccs_i32   ccs_fixed_to_int(ccs_fixed f);

/* Floor/ceil conversions (useful for broadphase grids). */
ccs_i32   ccs_fixed_to_int_floor(ccs_fixed f);
ccs_i32   ccs_fixed_to_int_ceil(ccs_fixed f);

/* ============================================================
   Core arithmetic
   ============================================================ */

ccs_fixed ccs_fixed_mul(ccs_fixed a, ccs_fixed b);
ccs_fixed ccs_fixed_div(ccs_fixed a, ccs_fixed b);

/* ============================================================
   Utilities
   ============================================================ */

ccs_fixed ccs_fixed_abs(ccs_fixed v);
ccs_fixed ccs_fixed_clamp(ccs_fixed v, ccs_fixed min, ccs_fixed max);

/* Deterministic baseline sqrt (Q16.16 -> Q16.16) */
ccs_fixed ccs_fixed_sqrt(ccs_fixed v);

/*
    Hook de sqrt (PC-friendly)
    --------------------------
    Si quieres reemplazar el sqrt por LUT/NR/etc en PC, define:

        #define CCS_FIXED_SQRT(x) my_fast_sqrt((x))

    ANTES de incluir cualquier header de CCS.

    Por defecto, se usa ccs_fixed_sqrt().
*/
#ifndef CCS_FIXED_SQRT
#define CCS_FIXED_SQRT(x) ccs_fixed_sqrt((x))
#endif

#endif /* CCS_FIXED_H */
