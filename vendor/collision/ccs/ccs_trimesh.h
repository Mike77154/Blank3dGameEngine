#ifndef CCS_TRIMESH_H
#define CCS_TRIMESH_H

#include "ccs_shapes.h"
#include "ccs_aabbtree.h"
#include "ccs_narrow.h"    /* primitives */
#include "ccs_raycast.h"   /* ray */
#include "ccs_contact.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Init helpers (no malloc). vertices/indices are external. */
void ccs_trimesh_init(
    ccs_shape_trimesh* m,
    const ccs_vec3* vertices,
    int vertex_count,
    const ccs_u32* indices,
    int tri_count,
    ccs_vec3 center,
    ccs_u32 flags
);

/* Optional accel: build aabbtree from triangles.
   tri_aabbs and tri_ids are user-provided buffers length >= tri_count.
   Returns 1 on success.
*/
int ccs_trimesh_build_aabbtree(
    ccs_shape_trimesh* m,
    CCS_AABBTree* tree,
    ccs_aabb* tri_aabbs,
    int* tri_ids,
    int tri_count
);

/* Collision: normal is mesh -> shape */
int ccs_trimesh_collide_sphere(const ccs_shape_trimesh* m, const ccs_sphere* s, ccs_contact* out);
int ccs_trimesh_collide_capsule(const ccs_shape_trimesh* m, const ccs_capsule* c, ccs_contact* out);
int ccs_trimesh_collide_obb(const ccs_shape_trimesh* m, const ccs_obb* o, ccs_contact* out);

/* Raycast */
int ccs_trimesh_raycast(const ccs_shape_trimesh* m, const ccs_ray* ray, ccs_raycast_hit* out);

#ifdef __cplusplus
}
#endif

#endif /* CCS_TRIMESH_H */
