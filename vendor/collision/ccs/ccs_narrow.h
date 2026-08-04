#ifndef CCS_NARROW_H
#define CCS_NARROW_H

#include "ccs_contact.h"
#include "ccs_manifold.h"
#include "ccs_geom.h"

/* ============================================================
   Shape primitives (narrow-phase)
   ============================================================ */

typedef struct {
    ccs_vec3 center;
    ccs_vec3 half_extents; /* AABB */
} ccs_box;

typedef struct {
    ccs_vec3  center;
    ccs_fixed radius;
} ccs_sphere;

/* ============================================================
   Narrow-phase tests (single contact)
   ============================================================ */

int ccs_narrow_sphere_sphere(
    const ccs_sphere* a,
    const ccs_sphere* b,
    ccs_contact* out
);

int ccs_narrow_box_box(
    const ccs_box* a,
    const ccs_box* b,
    ccs_contact* out
);

/* Sphere (A) vs AABB (B): normal A->B */
int ccs_narrow_sphere_box(
    const ccs_sphere* s,
    const ccs_box* b,
    ccs_contact* out
);

/* Sphere (A) vs OBB (B): normal A->B */
int ccs_narrow_sphere_obb(
    const ccs_sphere* s,
    const ccs_obb* b,
    ccs_contact* out
);

/* OBB vs OBB (single contact normal + penetration) */
int ccs_narrow_obb_obb(
    const ccs_obb* a,
    const ccs_obb* b,
    ccs_contact* out
);

/* Capsule (A) vs Sphere (B): normal A->B */
int ccs_narrow_capsule_sphere(
    const ccs_capsule* a,
    const ccs_sphere* b,
    ccs_contact* out
);

/* Capsule (A) vs Capsule (B): normal A->B */
int ccs_narrow_capsule_capsule(
    const ccs_capsule* a,
    const ccs_capsule* b,
    ccs_contact* out
);

/* Capsule (A) vs OBB (B): normal A->B */
int ccs_narrow_capsule_obb(
    const ccs_capsule* a,
    const ccs_obb* b,
    ccs_contact* out
);

/* ============================================================
   Manifold generation
   ============================================================ */

int ccs_narrow_box_box_manifold(
    const ccs_box* a,
    const ccs_box* b,
    ccs_manifold* out
);

int ccs_narrow_obb_obb_manifold(
    const ccs_obb* a,
    const ccs_obb* b,
    ccs_manifold* out
);

/* Manifold for capsule vs others (1 point) */
int ccs_narrow_capsule_sphere_manifold(
    const ccs_capsule* a,
    const ccs_sphere* b,
    ccs_manifold* out
);

int ccs_narrow_capsule_capsule_manifold(
    const ccs_capsule* a,
    const ccs_capsule* b,
    ccs_manifold* out
);

int ccs_narrow_capsule_obb_manifold(
    const ccs_capsule* a,
    const ccs_obb* b,
    ccs_manifold* out
);

/* Sphere vs OBB manifold (1 point) */
int ccs_narrow_sphere_obb_manifold(
    const ccs_sphere* a,
    const ccs_obb* b,
    ccs_manifold* out
);

#endif /* CCS_NARROW_H */
