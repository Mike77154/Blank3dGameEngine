#ifndef CCS_CONVEX_H
#define CCS_CONVEX_H

#include "ccs_shapes.h"
#include "ccs_narrow.h"
#include "ccs_contact.h"
#include "ccs_raycast.h"

#ifdef __cplusplus
extern "C" {
#endif

void ccs_convex_init(
    ccs_shape_convex* cv,
    const ccs_vec3* vertices,
    int vertex_count,
    const ccs_vec3* plane_normals,
    const ccs_fixed* plane_ds,
    int plane_count,
    ccs_vec3 center,
    ccs_u32 flags
);

/* Collision: normal is convex -> shape */
int ccs_convex_collide_sphere(const ccs_shape_convex* cv, const ccs_sphere* s, ccs_contact* out);
int ccs_convex_collide_capsule(const ccs_shape_convex* cv, const ccs_capsule* c, ccs_contact* out);
int ccs_convex_collide_obb(const ccs_shape_convex* cv, const ccs_obb* o, ccs_contact* out);

/* Raycast */
int ccs_convex_raycast(const ccs_shape_convex* cv, const ccs_ray* ray, ccs_raycast_hit* out);

#ifdef __cplusplus
}
#endif

#endif /* CCS_CONVEX_H */
