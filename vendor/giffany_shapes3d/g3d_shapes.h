#ifndef G3D_SHAPES_H
#define G3D_SHAPES_H

/*
 * Giffany Shapes3D
 * C89, allocation-free, renderer-agnostic primitive mesh and collision library.
 * Fixed point format: signed Q16.16 stored in signed long.
 */


#ifdef __cplusplus
extern "C" {
#endif

#define G3D_VERSION_MAJOR 1
#define G3D_VERSION_MINOR 0
#define G3D_VERSION_PATCH 0

#define G3D_FX_ONE       65536L
#define G3D_FX_HALF      32768L
#define G3D_FX_EPSILON   8L
#define G3D_FX_MAX       2147483647L
#define G3D_FX_MIN       (-2147483647L - 1L)

#define G3D_TURN_0       0U
#define G3D_TURN_45      8192U
#define G3D_TURN_90      16384U
#define G3D_TURN_180     32768U
#define G3D_TURN_270     49152U

#define G3D_OK                    0
#define G3D_ERR_NULL             -1
#define G3D_ERR_CAPACITY         -2
#define G3D_ERR_BAD_ARGUMENT     -3
#define G3D_ERR_RANGE            -4
#define G3D_ERR_DEGENERATE       -5

#define G3D_PRIMITIVE_TRIANGLES  1
#define G3D_PRIMITIVE_LINES      2

#define G3D_TRUE  1
#define G3D_FALSE 0

typedef signed long g3d_fx;
typedef unsigned short g3d_index;
typedef unsigned short g3d_angle;

typedef struct g3d_vec2_s {
    g3d_fx x;
    g3d_fx y;
} g3d_vec2;

typedef struct g3d_vec3_s {
    g3d_fx x;
    g3d_fx y;
    g3d_fx z;
} g3d_vec3;

typedef struct g3d_color_s {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} g3d_color;

typedef struct g3d_vertex_s {
    g3d_vec3 position;
    g3d_vec3 normal;
    g3d_vec2 uv;
    g3d_color color;
} g3d_vertex;

typedef struct g3d_mesh_s {
    g3d_vertex *vertices;
    g3d_index *indices;
    unsigned short vertex_capacity;
    unsigned short index_capacity;
    unsigned short vertex_count;
    unsigned short index_count;
    unsigned char primitive;
    int error;
} g3d_mesh;

typedef struct g3d_mesh_requirements_s {
    unsigned long vertices;
    unsigned long indices;
} g3d_mesh_requirements;

typedef struct g3d_transform_s {
    g3d_vec3 position;
    g3d_vec3 scale;
    g3d_angle rotation_x;
    g3d_angle rotation_y;
    g3d_angle rotation_z;
} g3d_transform;

typedef struct g3d_aabb_s {
    g3d_vec3 min;
    g3d_vec3 max;
} g3d_aabb;

typedef struct g3d_sphere_s {
    g3d_vec3 center;
    g3d_fx radius;
} g3d_sphere;

typedef struct g3d_capsule_s {
    g3d_vec3 a;
    g3d_vec3 b;
    g3d_fx radius;
} g3d_capsule;

typedef struct g3d_plane_s {
    g3d_vec3 normal;
    g3d_fx d;
} g3d_plane;

typedef struct g3d_ray_s {
    g3d_vec3 origin;
    g3d_vec3 direction;
} g3d_ray;

typedef struct g3d_hit_s {
    int hit;
    g3d_fx t;
    g3d_vec3 point;
    g3d_vec3 normal;
    g3d_fx u;
    g3d_fx v;
} g3d_hit;

/* Fixed point */
g3d_fx g3d_fx_from_int(long value);
g3d_fx g3d_fx_from_ratio(long numerator, long denominator);
long g3d_fx_to_int(g3d_fx value);
g3d_fx g3d_fx_floor(g3d_fx value);
g3d_fx g3d_fx_abs(g3d_fx value);
g3d_fx g3d_fx_add(g3d_fx a, g3d_fx b);
g3d_fx g3d_fx_sub(g3d_fx a, g3d_fx b);
g3d_fx g3d_fx_mul(g3d_fx a, g3d_fx b);
g3d_fx g3d_fx_div(g3d_fx a, g3d_fx b);
g3d_fx g3d_fx_sqrt(g3d_fx value);
g3d_fx g3d_fx_clamp(g3d_fx value, g3d_fx low, g3d_fx high);
g3d_fx g3d_fx_lerp(g3d_fx a, g3d_fx b, g3d_fx t);

/* Angles use one unsigned 16-bit turn: 0=0 degrees, 16384=90 degrees. */
void g3d_sincos(g3d_angle angle, g3d_fx *out_sine, g3d_fx *out_cosine);
g3d_fx g3d_sin(g3d_angle angle);
g3d_fx g3d_cos(g3d_angle angle);

/* Vector math */
g3d_vec2 g3d_vec2_make(g3d_fx x, g3d_fx y);
g3d_vec3 g3d_vec3_make(g3d_fx x, g3d_fx y, g3d_fx z);
g3d_vec3 g3d_vec3_add(g3d_vec3 a, g3d_vec3 b);
g3d_vec3 g3d_vec3_sub(g3d_vec3 a, g3d_vec3 b);
g3d_vec3 g3d_vec3_scale(g3d_vec3 v, g3d_fx scalar);
g3d_fx g3d_vec3_dot(g3d_vec3 a, g3d_vec3 b);
g3d_vec3 g3d_vec3_cross(g3d_vec3 a, g3d_vec3 b);
g3d_fx g3d_vec3_length_sq(g3d_vec3 v);
g3d_fx g3d_vec3_length(g3d_vec3 v);
g3d_vec3 g3d_vec3_normalize(g3d_vec3 v);
g3d_vec3 g3d_vec3_lerp(g3d_vec3 a, g3d_vec3 b, g3d_fx t);

/* Color helpers */
g3d_color g3d_color_rgba(unsigned char r, unsigned char g,
                          unsigned char b, unsigned char a);
g3d_color g3d_color_lerp(g3d_color a, g3d_color b, g3d_fx t);

/* Mesh setup. The caller owns and supplies both fixed-size arrays. */
int g3d_mesh_init(g3d_mesh *mesh,
                  g3d_vertex *vertices, unsigned short vertex_capacity,
                  g3d_index *indices, unsigned short index_capacity);
void g3d_mesh_clear(g3d_mesh *mesh);
int g3d_mesh_add_vertex(g3d_mesh *mesh, const g3d_vertex *vertex,
                        g3d_index *out_index);
int g3d_mesh_add_triangle(g3d_mesh *mesh, g3d_index a,
                          g3d_index b, g3d_index c);
int g3d_mesh_add_line(g3d_mesh *mesh, g3d_index a, g3d_index b);
int g3d_mesh_compute_aabb(const g3d_mesh *mesh, g3d_aabb *out_aabb);
int g3d_mesh_has_capacity(const g3d_mesh *mesh,
                          g3d_mesh_requirements requirements);

/* Exact static-buffer requirements for each generator. */
g3d_mesh_requirements g3d_require_triangle(void);
g3d_mesh_requirements g3d_require_quad(void);
g3d_mesh_requirements g3d_require_grid(unsigned short segments_x,
                                       unsigned short segments_z);
g3d_mesh_requirements g3d_require_disc(unsigned short segments);
g3d_mesh_requirements g3d_require_box(void);
g3d_mesh_requirements g3d_require_prism(unsigned short sides);
g3d_mesh_requirements g3d_require_pyramid(unsigned short sides);
g3d_mesh_requirements g3d_require_bipyramid(unsigned short sides);
g3d_mesh_requirements g3d_require_frustum(g3d_fx bottom_radius,
                                          g3d_fx top_radius,
                                          unsigned short segments,
                                          int cap_bottom, int cap_top);
g3d_mesh_requirements g3d_require_uv_sphere(unsigned short slices,
                                            unsigned short stacks);
g3d_mesh_requirements g3d_require_hemisphere(unsigned short slices,
                                             unsigned short stacks, int cap);
g3d_mesh_requirements g3d_require_capsule(unsigned short slices,
                                          unsigned short hemisphere_stacks);
void g3d_mesh_set_color(g3d_mesh *mesh, g3d_color color);
void g3d_mesh_gradient_y(g3d_mesh *mesh, g3d_fx min_y, g3d_fx max_y,
                         g3d_color bottom, g3d_color top);
void g3d_mesh_checker_uv(g3d_mesh *mesh, unsigned short tiles_u,
                         unsigned short tiles_v, g3d_color a, g3d_color b);
void g3d_mesh_uv_transform(g3d_mesh *mesh, g3d_fx scale_u,
                           g3d_fx scale_v, g3d_fx offset_u,
                           g3d_fx offset_v);

/* Transform */
void g3d_transform_identity(g3d_transform *transform);
g3d_vec3 g3d_transform_point(const g3d_transform *transform, g3d_vec3 point);
g3d_vec3 g3d_transform_normal(const g3d_transform *transform, g3d_vec3 normal);
void g3d_mesh_apply_transform(g3d_mesh *mesh, const g3d_transform *transform);

/* Primitive generators. All shapes are centered at the origin and use Y-up. */
int g3d_make_triangle(g3d_mesh *mesh, g3d_fx width, g3d_fx height,
                      g3d_color color);
int g3d_make_quad(g3d_mesh *mesh, g3d_fx width, g3d_fx depth,
                  g3d_color color);
int g3d_make_grid(g3d_mesh *mesh, g3d_fx width, g3d_fx depth,
                  unsigned short segments_x, unsigned short segments_z,
                  g3d_color color);
int g3d_make_disc(g3d_mesh *mesh, g3d_fx radius,
                  unsigned short segments, g3d_color color);
int g3d_make_box(g3d_mesh *mesh, g3d_fx width, g3d_fx height,
                 g3d_fx depth, g3d_color color);
int g3d_make_prism(g3d_mesh *mesh, g3d_fx radius, g3d_fx height,
                   unsigned short sides, g3d_color color);
int g3d_make_pyramid(g3d_mesh *mesh, g3d_fx radius, g3d_fx height,
                     unsigned short sides, g3d_color color);
int g3d_make_bipyramid(g3d_mesh *mesh, g3d_fx radius, g3d_fx height,
                       unsigned short sides, g3d_color color);
int g3d_make_frustum(g3d_mesh *mesh, g3d_fx bottom_radius,
                     g3d_fx top_radius, g3d_fx height,
                     unsigned short segments, int cap_bottom,
                     int cap_top, g3d_color color);
int g3d_make_cylinder(g3d_mesh *mesh, g3d_fx radius, g3d_fx height,
                      unsigned short segments, g3d_color color);
int g3d_make_cone(g3d_mesh *mesh, g3d_fx radius, g3d_fx height,
                  unsigned short segments, g3d_color color);
int g3d_make_uv_sphere(g3d_mesh *mesh, g3d_fx radius,
                       unsigned short slices, unsigned short stacks,
                       g3d_color color);
int g3d_make_hemisphere(g3d_mesh *mesh, g3d_fx radius,
                        unsigned short slices, unsigned short stacks,
                        int upper, int cap, g3d_color color);
int g3d_make_capsule(g3d_mesh *mesh, g3d_fx radius,
                     g3d_fx body_height, unsigned short slices,
                     unsigned short hemisphere_stacks, g3d_color color);

/* Convenience aliases. */
#define g3d_make_square_plane(m, size, color) \
    g3d_make_quad((m), (size), (size), (color))
#define g3d_make_cube(m, size, color) \
    g3d_make_box((m), (size), (size), (size), (color))
#define g3d_make_triangular_prism(m, radius, height, color) \
    g3d_make_prism((m), (radius), (height), 3U, (color))
#define g3d_make_triangular_pyramid(m, radius, height, color) \
    g3d_make_pyramid((m), (radius), (height), 3U, (color))
#define g3d_make_square_pyramid(m, radius, height, color) \
    g3d_make_pyramid((m), (radius), (height), 4U, (color))
#define g3d_make_pentagonal_pyramid(m, radius, height, color) \
    g3d_make_pyramid((m), (radius), (height), 5U, (color))
#define g3d_make_hexagonal_pyramid(m, radius, height, color) \
    g3d_make_pyramid((m), (radius), (height), 6U, (color))
#define g3d_make_octahedron(m, radius, height, color) \
    g3d_make_bipyramid((m), (radius), (height), 4U, (color))

/* Collision and queries */
g3d_aabb g3d_aabb_from_center_half(g3d_vec3 center, g3d_vec3 half_size);
int g3d_point_in_aabb(g3d_vec3 point, const g3d_aabb *box);
int g3d_aabb_overlap(const g3d_aabb *a, const g3d_aabb *b);
g3d_vec3 g3d_closest_point_aabb(g3d_vec3 point, const g3d_aabb *box);
int g3d_sphere_overlap(const g3d_sphere *a, const g3d_sphere *b);
int g3d_sphere_aabb_overlap(const g3d_sphere *sphere, const g3d_aabb *box);
g3d_vec3 g3d_closest_point_segment(g3d_vec3 point,
                                   g3d_vec3 a, g3d_vec3 b,
                                   g3d_fx *out_t);
int g3d_point_in_capsule(g3d_vec3 point, const g3d_capsule *capsule);
int g3d_capsule_sphere_overlap(const g3d_capsule *capsule,
                               const g3d_sphere *sphere);
int g3d_capsule_overlap(const g3d_capsule *a, const g3d_capsule *b);
g3d_fx g3d_plane_distance(const g3d_plane *plane, g3d_vec3 point);
int g3d_sphere_plane_overlap(const g3d_sphere *sphere,
                             const g3d_plane *plane);
int g3d_ray_plane(const g3d_ray *ray, const g3d_plane *plane,
                  g3d_hit *out_hit);
int g3d_ray_sphere(const g3d_ray *ray, const g3d_sphere *sphere,
                   g3d_hit *out_hit);
int g3d_ray_aabb(const g3d_ray *ray, const g3d_aabb *box,
                 g3d_hit *out_hit);
int g3d_ray_triangle(const g3d_ray *ray,
                     g3d_vec3 a, g3d_vec3 b, g3d_vec3 c,
                     g3d_hit *out_hit);
int g3d_ray_mesh(const g3d_ray *ray, const g3d_mesh *mesh,
                 g3d_hit *out_hit, unsigned short *out_triangle);

#ifdef __cplusplus
}
#endif

#endif
