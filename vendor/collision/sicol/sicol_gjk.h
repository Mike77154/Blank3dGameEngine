#ifndef SICOL_GJK_H
#define SICOL_GJK_H

/* ============================================================
 * SICOL - GJK / EPA / Convex shape cast
 * ------------------------------------------------------------
 * Support-mapped convex pairs only.
 * ============================================================ */

#include "sicol_convex.h"

#define SICOL_GJK_MAX_ITERS 32
#define SICOL_EPA_MAX_ITERS 48

typedef struct {
    int valid;
    int simplex_count;
    fx last_dir[3];
    fx last_closest[3];
    fx p[4][3];
    fx a[4][3];
    fx b[4][3];
} sicol_gjk_cache_t;

typedef struct {
    int intersect;
    fx distance;
    fx closest[3];
    fx normal[3];
    int iterations;
} sicol_gjk_result_t;

typedef struct {
    int hit;
    fx depth;
    fx normal[3];
    fx point[3];
    int iterations;
} sicol_epa_result_t;

typedef struct {
    int hit;
    fx fraction;
    fx safe_fraction;
    fx normal[3];
    fx point[3];
    int iterations;
} sicol_shape_cast_result_t;

typedef struct {
    int hit;
    fx fraction;
    fx point[3];
    fx normal[3];
    int iterations;
} sicol_gjk_raycast_result_t;

void sicol_gjk_cache_reset(sicol_gjk_cache_t* cache);

int sicol_gjk_distance_query(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_gjk_result_t* out
);

int sicol_gjk_distance_cached_query(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_gjk_cache_t* cache,
    sicol_gjk_result_t* out
);

int sicol_gjk_penetration_query(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_epa_result_t* out
);

int sicol_gjk_penetration_cached_query(
    const sicol_shape_t* a,
    const sicol_shape_t* b,
    sicol_gjk_cache_t* cache,
    sicol_epa_result_t* out
);

int sicol_shape_cast_exact(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* target,
    sicol_shape_cast_result_t* out
);

int sicol_shape_cast_exact_cached(
    const sicol_shape_t* moving,
    const fx delta[3],
    const sicol_shape_t* target,
    sicol_gjk_cache_t* cache,
    sicol_shape_cast_result_t* out
);

int sicol_gjk_raycast_query(
    const sicol_shape_t* ray,
    const sicol_shape_t* target,
    sicol_gjk_cache_t* cache,
    sicol_gjk_raycast_result_t* out
);

#endif /* SICOL_GJK_H */
