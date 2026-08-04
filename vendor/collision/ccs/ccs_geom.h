#ifndef CCS_GEOM_H
#define CCS_GEOM_H

#include "ccs_math.h"

/* ============================================================
   Canonical geometric primitives (narrow-phase)
   ============================================================ */

typedef struct {
    ccs_vec3 center;
    ccs_vec3 axis[3]; /* orthonormal basis (unit vectors) */
    ccs_vec3 half;    /* half-extents */
} ccs_obb;

typedef struct {
    ccs_vec3  center;
    ccs_vec3  axis;        /* unit direction */
    ccs_fixed half_height; /* half length of inner segment */
    ccs_fixed radius;
} ccs_capsule;

/* ============================================================
   Helpers
   ============================================================ */

void ccs_obb_set_identity(ccs_obb* o);

/* Transform points/vectors to/from OBB local space */
ccs_vec3 ccs_obb_to_local_point(const ccs_obb* o, ccs_vec3 p);
ccs_vec3 ccs_obb_to_local_dir(const ccs_obb* o, ccs_vec3 v);
ccs_vec3 ccs_obb_to_world_point(const ccs_obb* o, ccs_vec3 p_local);
ccs_vec3 ccs_obb_to_world_dir(const ccs_obb* o, ccs_vec3 v_local);

/* Support point (for potential GJK usage / debug) */
ccs_vec3 ccs_obb_support(const ccs_obb* o, ccs_vec3 dir_world);

/* Capsule endpoints in world space */
void ccs_capsule_endpoints(const ccs_capsule* c, ccs_vec3* out_a, ccs_vec3* out_b);

#endif /* CCS_GEOM_H */
