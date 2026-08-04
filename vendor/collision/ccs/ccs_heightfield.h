#ifndef CCS_HEIGHTFIELD_H
#define CCS_HEIGHTFIELD_H

#include "ccs_shapes.h"
#include "ccs_narrow.h"
#include "ccs_contact.h"
#include "ccs_raycast.h"

#ifdef __cplusplus
extern "C" {
#endif

void ccs_heightfield_init(
    ccs_shape_heightfield* hf,
    ccs_vec3 origin,
    int width,
    int depth,
    ccs_fixed cell_size_x,
    ccs_fixed cell_size_z,
    const ccs_fixed* heights,
    ccs_u32 flags
);

/* Collision: normal is heightfield -> shape */
int ccs_heightfield_collide_sphere(const ccs_shape_heightfield* hf, const ccs_sphere* s, ccs_contact* out);
int ccs_heightfield_collide_capsule(const ccs_shape_heightfield* hf, const ccs_capsule* c, ccs_contact* out);
int ccs_heightfield_collide_obb(const ccs_shape_heightfield* hf, const ccs_obb* o, ccs_contact* out);

/* Raycast */
int ccs_heightfield_raycast(const ccs_shape_heightfield* hf, const ccs_ray* ray, ccs_raycast_hit* out);

#ifdef __cplusplus
}
#endif

#endif /* CCS_HEIGHTFIELD_H */
