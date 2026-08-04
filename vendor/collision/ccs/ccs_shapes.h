#ifndef CCS_SHAPES_H
#define CCS_SHAPES_H

#include "ccs_config.h"
#include "ccs_types.h"
#include "ccs_math.h"

/* ============================================================
   Shape types
   ============================================================ */

typedef enum {
    CCS_SHAPE_NONE = 0,

    /* Primitives */
    CCS_SHAPE_SPHERE,
    CCS_SHAPE_BOX,      /* AABB box */
    CCS_SHAPE_CAPSULE,
    CCS_SHAPE_OBB,

    /* Infinite-ish */
    CCS_SHAPE_PLANE,      /* two-sided plane */
    CCS_SHAPE_HALFSPACE,  /* one-sided plane (solid in -normal side) */

    /* World geometry */
    CCS_SHAPE_TRIMESH,    /* static triangle mesh */
    CCS_SHAPE_HEIGHTFIELD,

    /* Generic */
    CCS_SHAPE_CONVEX,     /* convex polyhedron (planes + vertices) */
    CCS_SHAPE_COMPOUND,   /* list of child shapes */

    CCS_SHAPE_COUNT
} ccs_shape_type;

/* ============================================================
   Shape flags (DATA ONLY)
   ============================================================ */

enum {
    CCS_SHAPE_FLAG_NONE    = 0,
    CCS_SHAPE_FLAG_STATIC  = 1u << 0, /* optimization hint */
    CCS_SHAPE_FLAG_TRIGGER = 1u << 1  /* overlap only */
};

/* ============================================================
   Base shape header
   ============================================================ */

typedef struct {
    ccs_shape_type type;
    ccs_u32        flags;
} ccs_shape_header;

/* ============================================================
   Generic shape handle (BASE TYPE)
   ============================================================ */

typedef void ccs_shape;

/* ============================================================
   AABB
   ============================================================ */

typedef struct {
    ccs_vec3 min;
    ccs_vec3 max;
} ccs_aabb;

/* ============================================================
   Shape definitions (DATA ONLY)
   ============================================================ */

typedef struct {
    ccs_shape_header header;
    ccs_vec3  center;
    ccs_fixed radius;
} ccs_shape_sphere;

typedef struct {
    ccs_shape_header header;
    ccs_vec3 center;
    ccs_vec3 half_extents;
} ccs_shape_box;

typedef struct {
    ccs_shape_header header;
    ccs_vec3  center;
    ccs_vec3  axis;        /* unit direction */
    ccs_fixed half_height; /* half length of inner segment */
    ccs_fixed radius;
} ccs_shape_capsule;

typedef struct {
    ccs_shape_header header;
    ccs_vec3 center;
    ccs_vec3 half_extents;
    ccs_vec3 axis[3]; /* orthonormal basis */
} ccs_shape_obb;

/* ============================================================
   Plane / Half-space
   ============================================================ */

typedef struct {
    ccs_shape_header header;
    ccs_vec3 point;   /* point on plane */
    ccs_vec3 normal;  /* unit normal */
    ccs_fixed dist;   /* dot(normal, point) */
} ccs_shape_plane;

typedef struct {
    ccs_shape_header header;
    ccs_vec3 point;   /* point on plane */
    ccs_vec3 normal;  /* unit normal (points OUT of solid) */
    ccs_fixed dist;   /* dot(normal, point) */
} ccs_shape_halfspace;

/* ============================================================
   Static triangle mesh
   ============================================================

   - vertices/indices son EXTERNOS (ownership del usuario).
   - Se asume que indices apunta a tripletas (tri_count*3) de índices a vertices.
   - El mesh puede llevar una traslación (center).
   - Aceleración opcional:
       * tri_aabbs: array externo de AABBs por triángulo (en espacio LOCAL)
       * tree: BVH/AABBTree construido sobre esos AABBs.

   Nota: CCS no genera hulls ni cocina meshes; sólo consume data.
*/

typedef struct CCS_AABBTree CCS_AABBTree; /* forward (ver ccs_aabbtree.h) */

typedef struct {
    ccs_shape_header header;

    const ccs_vec3* vertices; /* local-space */
    int vertex_count;

    const ccs_u32* indices;   /* 3*tri_count */
    int tri_count;

    ccs_vec3 center;          /* translation */

    ccs_aabb local_aabb;      /* vertices bounds in local space */

    /* optional accel */
    const ccs_aabb* tri_aabbs;     /* local-space AABB per triangle (tri_count) */
    const CCS_AABBTree* tree;      /* may be NULL */
} ccs_shape_trimesh;

/* ============================================================
   Heightfield
   ============================================================

   - heights: EXTERNO (ownership del usuario), tamaño width*depth.
   - origin: world-space origin del grid (x,z en origen; y base).
   - cell_size_x/z: tamaño de celda en fixed.

   Convención:
     P(i,j) = (origin.x + i*cell_x, origin.y + heights[i + j*width], origin.z + j*cell_z)

   Triángulos por celda:
     (i,j) base:
       T0: p00, p10, p01
       T1: p10, p11, p01
*/

typedef struct {
    ccs_shape_header header;

    ccs_vec3 origin;
    int width;
    int depth;

    ccs_fixed cell_size_x;
    ccs_fixed cell_size_z;

    const ccs_fixed* heights; /* length = width*depth */

    ccs_fixed min_h;
    ccs_fixed max_h;
} ccs_shape_heightfield;

/* ============================================================
   Convex polyhedron (planes + vertices)
   ============================================================

   Representación híbrida:
   - vertices: para AABB y tests por ejes (SAT)
   - planes: para raycast y tests rápidos (face normals)

   Todo es EXTERNO (ownership del usuario).

   Plane equation (LOCAL): dot(n, x) <= d
   World: x_world = x_local + center
          dot(n, x_world) <= d + dot(n, center)
*/

typedef struct {
    ccs_shape_header header;

    ccs_vec3 center;

    const ccs_vec3* vertices;
    int vertex_count;

    const ccs_vec3* plane_normals; /* outward */
    const ccs_fixed* plane_ds;     /* local-space */
    int plane_count;

    ccs_aabb local_aabb;
} ccs_shape_convex;

/* ============================================================
   Compound shape
   ============================================================ */

typedef struct {
    const ccs_shape* shape; /* child shape (local center relative to 0) */
    ccs_vec3 offset;        /* translation from compound center */
} ccs_compound_child;

typedef struct {
    ccs_shape_header header;
    ccs_vec3 center;

    int child_count;
    ccs_compound_child children[CCS_COMPOUND_MAX_CHILDREN];
} ccs_shape_compound;

/* ============================================================
   Shape utility API
   ============================================================ */

ccs_aabb ccs_shape_compute_aabb(const void* shape);

/* ============================================================
   Shape helpers
   ============================================================ */

/* Validation */
int ccs_shape_is_valid(const void* shape);

/* Center access (genérico) */
ccs_vec3 ccs_shape_get_center(const void* shape);

/* Center write (genérico) */
void ccs_shape_set_center(void* shape, ccs_vec3 center);

/* AABB helpers */
ccs_vec3 ccs_aabb_center(ccs_aabb aabb);
ccs_vec3 ccs_aabb_extents(ccs_aabb aabb);
int      ccs_aabb_contains_point(ccs_aabb aabb, ccs_vec3 p);

/* Utility */
int ccs_shape_is_static(const void* shape);
int ccs_shape_is_trigger(const void* shape);

#endif /* CCS_SHAPES_H */
