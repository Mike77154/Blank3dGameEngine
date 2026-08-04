#ifndef CCS_PLANE_H
#define CCS_PLANE_H

#include "ccs_shapes.h"
#include "ccs_narrow.h"
#include "ccs_contact.h"
#include "ccs_raycast.h"

#ifdef __cplusplus
extern "C" {
#endif

void ccs_plane_init(ccs_shape_plane* p, ccs_vec3 point, ccs_vec3 normal, ccs_u32 flags);
void ccs_halfspace_init(ccs_shape_halfspace* p, ccs_vec3 point, ccs_vec3 normal, ccs_u32 flags);

/* Collision: normal is plane/halfspace -> shape */
int ccs_plane_collide_sphere(const ccs_shape_plane* p, const ccs_sphere* s, ccs_contact* out);
int ccs_plane_collide_capsule(const ccs_shape_plane* p, const ccs_capsule* c, ccs_contact* out);
int ccs_plane_collide_obb(const ccs_shape_plane* p, const ccs_obb* o, ccs_contact* out);

int ccs_halfspace_collide_sphere(const ccs_shape_halfspace* p, const ccs_sphere* s, ccs_contact* out);
int ccs_halfspace_collide_capsule(const ccs_shape_halfspace* p, const ccs_capsule* c, ccs_contact* out);
int ccs_halfspace_collide_obb(const ccs_shape_halfspace* p, const ccs_obb* o, ccs_contact* out);

/* Raycast */
int ccs_plane_raycast(const ccs_shape_plane* p, const ccs_ray* ray, ccs_raycast_hit* out);
int ccs_halfspace_raycast(const ccs_shape_halfspace* p, const ccs_ray* ray, ccs_raycast_hit* out);

#ifdef __cplusplus
}
#endif

#endif /* CCS_PLANE_H */
