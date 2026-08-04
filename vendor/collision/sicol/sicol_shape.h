#ifndef SICOL_SHAPE_H
#define SICOL_SHAPE_H

/* ============================================================
 * SICOL - Shape definitions and helpers
 * ============================================================ */

#include "sicol_math_fx.h"

#define SICOL_CONVEX_MAX_VERTS 32

typedef struct sicol_mesh_t sicol_mesh_t;
#define SICOL_MESH_FWD_DECL 1

typedef enum {
    SICOL_SHAPE_NONE = 0,
    SICOL_SHAPE_AABB,
    SICOL_SHAPE_OBB,
    SICOL_SHAPE_PLANE,
    SICOL_SHAPE_RAY,
    SICOL_SHAPE_SPHERE,
    SICOL_SHAPE_CAPSULE,
    SICOL_SHAPE_SEGMENT,
    SICOL_SHAPE_CONVEX,
    SICOL_SHAPE_TRIANGLE,
    SICOL_SHAPE_MESH
} sicol_shape_type_t;

typedef struct {
    sicol_shape_type_t type;
    fx pos[3];

    union {
        struct {
            fx half[3];
        } aabb;

        struct {
            fx half[3];
            fx axis[3][3];
        } obb;

        struct {
            fx normal[3];
            fx d;
        } plane;

        struct {
            fx dir[3];
            fx length;
        } ray;

        struct {
            fx radius;
        } sphere;

        struct {
            fx axis[3];
            fx half_segment;
            fx radius;
        } capsule;

        struct {
            fx to[3];
        } segment;

        struct {
            int count;
            fx verts[SICOL_CONVEX_MAX_VERTS][3];
            fx axis[3][3];
        } convex;

        struct {
            fx verts[3][3];
        } triangle;

        struct {
            const sicol_mesh_t* mesh;
            fx axis[3][3];
        } mesh;
    } u;
} sicol_shape_t;

void sicol_shape_make_none(sicol_shape_t* s);
void sicol_shape_make_aabb(sicol_shape_t* s, const fx pos[3], const fx half[3]);
void sicol_shape_make_obb(sicol_shape_t* s, const fx pos[3], const fx half[3], fx axis[3][3]);
void sicol_shape_make_obb_identity(sicol_shape_t* s, const fx pos[3], const fx half[3]);
void sicol_shape_make_plane(sicol_shape_t* s, const fx normal[3], fx d);
void sicol_shape_make_ray(sicol_shape_t* s, const fx pos[3], const fx dir[3], fx length);
void sicol_shape_make_sphere(sicol_shape_t* s, const fx pos[3], fx radius);
void sicol_shape_make_capsule(sicol_shape_t* s, const fx pos[3], const fx axis[3], fx half_segment, fx radius);
void sicol_shape_make_segment(sicol_shape_t* s, const fx from[3], const fx to[3]);
void sicol_shape_make_convex(sicol_shape_t* s, const fx pos[3], fx verts[][3], int count, fx axis[3][3]);
void sicol_shape_make_convex_identity(sicol_shape_t* s, const fx pos[3], fx verts[][3], int count);
void sicol_shape_make_triangle(sicol_shape_t* s, const fx a[3], const fx b[3], const fx c[3]);
void sicol_shape_make_mesh(sicol_shape_t* s, const fx pos[3], const sicol_mesh_t* mesh);
void sicol_shape_make_mesh_oriented(sicol_shape_t* s, const fx pos[3], const sicol_mesh_t* mesh, fx axis[3][3]);

int sicol_shape_validate(const sicol_shape_t* s);
void sicol_shape_translate(sicol_shape_t* s, const fx delta[3]);
void sicol_shape_compute_aabb(const sicol_shape_t* s, fx out_min[3], fx out_max[3]);

void sicol_shape_get_basis(const sicol_shape_t* s, fx out_basis[3][3]);
void sicol_shape_apply_angular_velocity(sicol_shape_t* s, const fx omega[3], fx dt);

void sicol_shape_get_capsule_segment(const sicol_shape_t* s, fx out_a[3], fx out_b[3]);
void sicol_shape_get_segment_points(const sicol_shape_t* s, fx out_a[3], fx out_b[3]);
void sicol_shape_get_triangle_points(const sicol_shape_t* s, fx out_a[3], fx out_b[3], fx out_c[3]);
int sicol_shape_get_mesh_triangle_points(const sicol_shape_t* s, int triangle_index, fx out_a[3], fx out_b[3], fx out_c[3]);
void sicol_shape_mesh_local_to_world(const sicol_shape_t* s, const fx local[3], fx out_world[3]);
void sicol_shape_mesh_world_to_local(const sicol_shape_t* s, const fx world[3], fx out_local[3]);
void sicol_shape_mesh_world_aabb_to_local_aabb(const sicol_shape_t* s, const fx world_min[3], const fx world_max[3], fx out_local_min[3], fx out_local_max[3]);

#endif /* SICOL_SHAPE_H */
