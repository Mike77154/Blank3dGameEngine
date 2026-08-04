#ifndef SICOL_RAYCAST_H
#define SICOL_RAYCAST_H

/* ============================================================
 * SICOL - Raycast
 * ============================================================ */

#include "sicol_shape.h"
#include "sicol_math_fx.h"
#include "sicol_gjk.h"

typedef struct {
    fx t;
    fx point[3];
    fx normal[3];
    int shape_index;
} sicol_hit_t;

int sicol_raycast(
    const sicol_shape_t* ray,
    const sicol_shape_t* target,
    sicol_hit_t* out
);

int sicol_raycast_gjk(
    const sicol_shape_t* ray,
    const sicol_shape_t* target,
    sicol_gjk_cache_t* cache,
    sicol_hit_t* out
);

#endif /* SICOL_RAYCAST_H */
