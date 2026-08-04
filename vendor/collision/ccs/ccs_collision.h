#ifndef CCS_COLLISION_H
#define CCS_COLLISION_H

#include "ccs_shapes.h"
#include "ccs_dispatch.h"

/* ============================================================
   Collision result (single contact)
   ============================================================ */

typedef struct {
    int hit;
    ccs_contact contact;
} ccs_collision_result;

/* ============================================================
   Expanded collision results (manifold)
   ============================================================ */

typedef struct {
    int hit;
    ccs_manifold manifold;
} ccs_collision_manifold_result;

/* ============================================================
   Collision flags
   ============================================================ */

enum {
    CCS_COLLIDE_DEFAULT         = 0,
    CCS_COLLIDE_SKIP_BROADPHASE = 1u << 0,
    CCS_COLLIDE_PREFER_MANIFOLD = 1u << 1
};

/* ============================================================
   Main collision API
   ============================================================ */

ccs_collision_result ccs_collide(
    const void* shape_a,
    const void* shape_b,
    ccs_u32 flags
);

/* Boolean-only query */
int ccs_overlap(
    const void* shape_a,
    const void* shape_b,
    ccs_u32 flags
);

/* Manifold collision */
ccs_collision_manifold_result ccs_collide_manifold(
    const void* shape_a,
    const void* shape_b,
    ccs_u32 flags
);

/* Manifold + warm start (copia impulsos desde prev usando feature_id) */
ccs_collision_manifold_result ccs_collide_manifold_warmstart(
    const void* shape_a,
    const void* shape_b,
    const ccs_manifold* prev,
    ccs_u32 flags
);

/* Shape capability query (passthrough) */
unsigned int ccs_shape_caps(
    const void* shape
);

#endif /* CCS_COLLISION_H */
