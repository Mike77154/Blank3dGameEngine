#ifndef CCS_COMPOUND_H
#define CCS_COMPOUND_H

#include "ccs_shapes.h"
#include "ccs_contact.h"
#include "ccs_raycast.h"

#ifdef __cplusplus
extern "C" {
#endif

void ccs_compound_init(ccs_shape_compound* cp, ccs_vec3 center, ccs_u32 flags);

/* Adds a child (shape pointer + offset). Returns 1 on success. */
int ccs_compound_add_child(ccs_shape_compound* cp, const ccs_shape* child_shape, ccs_vec3 offset);

/* Collision: normal is compound -> other (when compound is A) */
int ccs_compound_collide(const ccs_shape_compound* cp, const void* other_shape, ccs_contact* out);

/* Raycast */
int ccs_compound_raycast(const ccs_shape_compound* cp, const ccs_ray* ray, ccs_raycast_hit* out);

#ifdef __cplusplus
}
#endif

#endif /* CCS_COMPOUND_H */
