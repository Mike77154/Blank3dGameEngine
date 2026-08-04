#ifndef SICOL_BROADPHASE_H
#define SICOL_BROADPHASE_H

/* ============================================================
 * SICOL - Broadphase
 * ------------------------------------------------------------
 * Sweep & Prune with conservative 3D pruning
 * ============================================================ */

#include "sicol_shape.h"

#define SICOL_BP_MAX_OBJECTS 512
#define SICOL_BP_MAX_PAIRS   4096

typedef struct {
    int a;
    int b;
} sicol_pair_t;

typedef struct {
    int pair_count;
    int truncated;
} sicol_bp_result_t;

int sicol_broadphase_sap(
    const sicol_shape_t* shapes,
    int count,
    sicol_pair_t* out_pairs,
    int max_pairs
);

sicol_bp_result_t sicol_broadphase_sap_ex(
    const sicol_shape_t* shapes,
    int count,
    sicol_pair_t* out_pairs,
    int max_pairs
);

#endif /* SICOL_BROADPHASE_H */
