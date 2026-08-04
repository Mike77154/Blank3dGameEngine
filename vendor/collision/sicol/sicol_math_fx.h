#ifndef SICOL_MATH_FX_H
#define SICOL_MATH_FX_H

/* ============================================================
 * SICOL - Fixed Point Math
 * ------------------------------------------------------------
 * - C89
 * - No floats
 * - Deterministic
 * ============================================================ */

#include "sicol_types.h"

typedef int32_t fx;

#define FX_SHIFT       12
#define FX_ONE         ((fx)(1 << FX_SHIFT))
#define FX_HALF        ((fx)(1 << (FX_SHIFT - 1)))
#define FX_EPSILON     ((fx)1)
#define FX_SQRT2       ((fx)5793) /* ~1.4142 in Q20.12 */

#define FX_FROM_INT(x)   ((fx)((x) << FX_SHIFT))
#define FX_TO_INT(x)     ((int)((x) >> FX_SHIFT))
#define FX_FROM_RATIO(n, d) ((fx)(((int64_t)(n) << FX_SHIFT) / (d)))

#define FX_MUL(a,b) \
    ((fx)(((int64_t)(a) * (int64_t)(b)) >> FX_SHIFT))

#define FX_DIV(a,b) \
    ((fx)(((int64_t)(a) << FX_SHIFT) / (b)))

#define FX_ABS(x) \
    ((x) < 0 ? -(x) : (x))

#define FX_MIN(a,b) \
    ((a) < (b) ? (a) : (b))

#define FX_MAX(a,b) \
    ((a) > (b) ? (a) : (b))

#define FX_SIGN(x) \
    ((x) < 0 ? -FX_ONE : FX_ONE)

fx fx_clamp(fx v, fx min_value, fx max_value);
fx fx_sqrt(fx v);

void fx_zero3(fx out[3]);
void fx_set3(fx out[3], fx x, fx y, fx z);
void fx_copy3(fx out[3], const fx v[3]);

fx fx_dot3(const fx a[3], const fx b[3]);
void fx_cross3(fx out[3], const fx a[3], const fx b[3]);

void fx_add3(fx out[3], const fx a[3], const fx b[3]);
void fx_sub3(fx out[3], const fx a[3], const fx b[3]);
void fx_neg3(fx out[3], const fx v[3]);
void fx_scale3(fx out[3], const fx v[3], fx s);
void fx_madd3(fx out[3], const fx a[3], const fx b[3], fx s);

fx fx_len_sq3(const fx v[3]);
fx fx_len3(const fx v[3]);
fx fx_distance_sq3(const fx a[3], const fx b[3]);
fx fx_distance3(const fx a[3], const fx b[3]);
int fx_normalize3(fx out[3], const fx v[3]);

void fx_identity_mat3(fx out[3][3]);
void fx_transpose_mat3(fx out[3][3], const fx in_m[3][3]);
void fx_mul_mat3_vec3(fx out[3], const fx m[3][3], const fx v[3]);

void fx_basis_to_local3(fx out[3], const fx basis[3][3], const fx v[3]);
void fx_basis_to_world3(fx out[3], const fx basis[3][3], const fx v[3]);

fx fx_closest_point_segment3(
    fx out[3],
    const fx a[3],
    const fx b[3],
    const fx p[3]
);

void fx_closest_points_segment_segment3(
    fx out_a[3],
    fx out_b[3],
    const fx p1[3],
    const fx q1[3],
    const fx p2[3],
    const fx q2[3],
    fx* s_out,
    fx* t_out
);

#endif /* SICOL_MATH_FX_H */
