#ifndef SICOL_MESH_H
#define SICOL_MESH_H

/* ============================================================
 * SICOL - Triangle mesh + BVH
 * ------------------------------------------------------------
 * Mesh triangles live in mesh-local space.
 * Supports static builds plus light dynamic/deformable refits
 * without rebuilding BVH topology.
 * ============================================================ */

#include "sicol_math_fx.h"

#define SICOL_MESH_MAX_TRIANGLES 256
#define SICOL_MESH_MAX_BVH_NODES (SICOL_MESH_MAX_TRIANGLES * 2)

typedef struct {
    fx v0[3];
    fx v1[3];
    fx v2[3];
} sicol_triangle_verts_t;

typedef struct {
    fx v[3][3];
    fx normal[3];
    fx centroid[3];
    fx min[3];
    fx max[3];
} sicol_mesh_triangle_t;

typedef struct {
    fx min[3];
    fx max[3];
    int left;
    int right;
    int start;
    int count;
} sicol_mesh_bvh_node_t;

#ifndef SICOL_MESH_FWD_DECL
typedef struct sicol_mesh_t sicol_mesh_t;
#define SICOL_MESH_FWD_DECL 1
#endif

struct sicol_mesh_t {
    int triangle_count;
    sicol_mesh_triangle_t triangles[SICOL_MESH_MAX_TRIANGLES];
    int indices[SICOL_MESH_MAX_TRIANGLES];
    int node_count;
    sicol_mesh_bvh_node_t nodes[SICOL_MESH_MAX_BVH_NODES];
    fx min[3];
    fx max[3];
};

void sicol_mesh_init(sicol_mesh_t* mesh);
int sicol_mesh_build(sicol_mesh_t* mesh, const sicol_triangle_verts_t* tris, int count);
int sicol_mesh_get_triangle(const sicol_mesh_t* mesh, int triangle_index, sicol_triangle_verts_t* out);
int sicol_mesh_set_triangle(sicol_mesh_t* mesh, int triangle_index, const sicol_triangle_verts_t* tri);
int sicol_mesh_refit(sicol_mesh_t* mesh);
int sicol_mesh_query_local_aabb(
    const sicol_mesh_t* mesh,
    const fx query_min[3],
    const fx query_max[3],
    int* out_triangle_indices,
    int max_results
);
int sicol_mesh_raycast_local(
    const sicol_mesh_t* mesh,
    const fx origin[3],
    const fx dir[3],
    fx length,
    int* out_triangle_index,
    fx* out_t,
    fx out_point[3],
    fx out_normal[3]
);

#endif /* SICOL_MESH_H */
