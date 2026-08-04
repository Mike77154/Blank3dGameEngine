#ifndef SICOL_CAST_H
#define SICOL_CAST_H

/* ============================================================
 * SICOL - Pair shape casts / TOI helpers
 * ------------------------------------------------------------
 * Exact or aggressive time-of-impact queries for moving shapes.
 * ============================================================ */

#include "sicol_shape.h"
#include "sicol_gjk.h"

int sicol_shape_cast_pair(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* target,
    sicol_gjk_cache_t* cache,
    sicol_shape_cast_result_t* out
);

#endif /* SICOL_CAST_H */
