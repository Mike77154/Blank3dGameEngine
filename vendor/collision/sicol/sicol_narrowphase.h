#ifndef SICOL_NARROWPHASE_H
#define SICOL_NARROWPHASE_H

/* ============================================================
 * SICOL - Narrowphase
 * ------------------------------------------------------------
 * Exact collision tests / overlap tests
 * ------------------------------------------------------------
 * Note:
 *   Planes are treated as solid half-spaces:
 *   dot(n, x) + d <= 0
 * ============================================================ */

#include "sicol_shape.h"
#include "sicol_math_fx.h"
#include "sicol_gjk.h"

typedef struct {
    fx normal[3];
    fx point[3];
    fx penetration;
    int feature_a;
    int feature_b;
    int count;
} sicol_contact_t;

int sicol_narrowphase_test(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_contact_t* out
);

int sicol_narrowphase_test_cached(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_contact_t* out,
    sicol_gjk_cache_t* cache
);

#endif /* SICOL_NARROWPHASE_H */
