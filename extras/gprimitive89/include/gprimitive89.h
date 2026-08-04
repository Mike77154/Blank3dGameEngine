#ifndef GPRIMITIVE89_H
#define GPRIMITIVE89_H

/* gprimitive89 - fixed-point, allocation-free 3D primitive mesh generation. */
/* C89 public domain / CC0. */

#ifdef __cplusplus
extern "C" {
#endif

#define GP89_VERSION_MAJOR 1
#define GP89_VERSION_MINOR 0
#define GP89_VERSION_PATCH 1

#define GP89_FX_SHIFT 16
#define GP89_FX_ONE   65536
#define GP89_FX_HALF  32768
#define GP89_TURN     65536U
#define GP89_QUARTER_TURN 16384U
#define GP89_HALF_TURN 32768U

#define GP89_OK                  0
#define GP89_ERR_ARGUMENT       -1
#define GP89_ERR_VERTEX_CAPACITY -2
#define GP89_ERR_TRI_CAPACITY   -3
#define GP89_ERR_INDEX_RANGE    -4
#define GP89_ERR_DEGENERATE     -5

#define GP89_AXIS_X 0
#define GP89_AXIS_Y 1
#define GP89_AXIS_Z 2

#define GP89_CAP_NONE   0
#define GP89_CAP_BOTTOM 1
#define GP89_CAP_TOP    2
#define GP89_CAP_BOTH   3

#define GP89_POLY_TETRAHEDRON          1
#define GP89_POLY_OCTAHEDRON           2
#define GP89_POLY_DODECAHEDRON         3
#define GP89_POLY_ICOSAHEDRON          4
#define GP89_POLY_CUBOCTAHEDRON        5
#define GP89_POLY_RHOMBIC_DODECAHEDRON 6
#define GP89_POLY_TRUNCATED_OCTAHEDRON 7

typedef int gp89_fx;
typedef unsigned short gp89_index;
typedef unsigned short gp89_angle;

typedef struct gp89_vec2 {
    gp89_fx x;
    gp89_fx y;
} gp89_vec2;

typedef struct gp89_vec3 {
    gp89_fx x;
    gp89_fx y;
    gp89_fx z;
} gp89_vec3;

typedef struct gp89_rgba {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} gp89_rgba;

typedef struct gp89_vertex {
    gp89_fx x;
    gp89_fx y;
    gp89_fx z;
    gp89_fx nx;
    gp89_fx ny;
    gp89_fx nz;
    gp89_fx tx;
    gp89_fx ty;
    gp89_fx tz;
    gp89_fx tw;
    gp89_fx u;
    gp89_fx v;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} gp89_vertex;

typedef struct gp89_triangle {
    gp89_index a;
    gp89_index b;
    gp89_index c;
    unsigned short material;
} gp89_triangle;

typedef struct gp89_face3 {
    gp89_index a;
    gp89_index b;
    gp89_index c;
} gp89_face3;

typedef struct gp89_mesh {
    gp89_vertex *vertices;
    gp89_triangle *triangles;
    unsigned int vertex_capacity;
    unsigned int triangle_capacity;
    unsigned int vertex_count;
    unsigned int triangle_count;
    int status;
} gp89_mesh;

typedef struct gp89_transform {
    gp89_fx tx;
    gp89_fx ty;
    gp89_fx tz;
    gp89_angle rx;
    gp89_angle ry;
    gp89_angle rz;
    gp89_fx sx;
    gp89_fx sy;
    gp89_fx sz;
} gp89_transform;

typedef void (*gp89_deform_fn)(gp89_vertex *vertex,
                               unsigned int vertex_index,
                               void *user_data);

/* Fixed-point helpers. */
gp89_fx gp89_fx_from_int(int value);
int gp89_fx_to_int(gp89_fx value);
gp89_fx gp89_fx_mul(gp89_fx a, gp89_fx b);
gp89_fx gp89_fx_div(gp89_fx a, gp89_fx b);
gp89_fx gp89_fx_sqrt(gp89_fx value);
gp89_fx gp89_fx_lerp(gp89_fx a, gp89_fx b, gp89_fx t);
void gp89_sincos(gp89_angle angle, gp89_fx *out_sin, gp89_fx *out_cos);
gp89_vec3 gp89_vec3_normalized(gp89_vec3 value);

/* Buffer ownership stays with the caller. */
void gp89_mesh_init(gp89_mesh *mesh,
                    gp89_vertex *vertex_buffer,
                    unsigned int vertex_capacity,
                    gp89_triangle *triangle_buffer,
                    unsigned int triangle_capacity);
void gp89_mesh_reset(gp89_mesh *mesh);
int gp89_mesh_valid(const gp89_mesh *mesh);

/* Surface primitives. */
int gp89_make_triangle(gp89_mesh *mesh, gp89_fx width, gp89_fx height);
int gp89_make_quad(gp89_mesh *mesh, gp89_fx width, gp89_fx height);
int gp89_make_plane(gp89_mesh *mesh,
                    gp89_fx width,
                    gp89_fx depth,
                    unsigned int x_segments,
                    unsigned int z_segments);
int gp89_make_disc(gp89_mesh *mesh, gp89_fx radius, unsigned int segments);
int gp89_make_annulus(gp89_mesh *mesh,
                      gp89_fx inner_radius,
                      gp89_fx outer_radius,
                      unsigned int segments);

/* Rounded and rotational solids. */
int gp89_make_uv_sphere(gp89_mesh *mesh,
                        gp89_fx radius,
                        unsigned int slices,
                        unsigned int stacks);
int gp89_make_capsule(gp89_mesh *mesh,
                      gp89_fx radius,
                      gp89_fx cylinder_height,
                      unsigned int slices,
                      unsigned int hemisphere_rings);
/* Naming aliases for the same native capsule/spherocylinder topology. */
int gp89_make_spherocylinder(gp89_mesh *mesh,
                             gp89_fx radius,
                             gp89_fx cylinder_height,
                             unsigned int slices,
                             unsigned int hemisphere_rings);
int gp89_make_rounded_cylinder(gp89_mesh *mesh,
                               gp89_fx radius,
                               gp89_fx cylinder_height,
                               unsigned int slices,
                               unsigned int hemisphere_rings);
int gp89_make_cylinder(gp89_mesh *mesh,
                       gp89_fx radius,
                       gp89_fx height,
                       unsigned int slices,
                       int caps);
int gp89_make_cone(gp89_mesh *mesh,
                   gp89_fx radius,
                   gp89_fx height,
                   unsigned int slices,
                   int cap);
int gp89_make_frustum(gp89_mesh *mesh,
                      gp89_fx bottom_radius,
                      gp89_fx top_radius,
                      gp89_fx height,
                      unsigned int slices,
                      int caps);
int gp89_make_torus(gp89_mesh *mesh,
                    gp89_fx major_radius,
                    gp89_fx minor_radius,
                    unsigned int major_segments,
                    unsigned int minor_segments);

/* Faceted solids and families. */
int gp89_make_box(gp89_mesh *mesh,
                  gp89_fx width,
                  gp89_fx height,
                  gp89_fx depth);
int gp89_make_cube(gp89_mesh *mesh, gp89_fx size);
int gp89_make_prism(gp89_mesh *mesh,
                    unsigned int sides,
                    gp89_fx radius,
                    gp89_fx height,
                    int caps);
int gp89_make_pyramid(gp89_mesh *mesh,
                      unsigned int sides,
                      gp89_fx radius,
                      gp89_fx height,
                      int cap);
int gp89_make_antiprism(gp89_mesh *mesh,
                        unsigned int sides,
                        gp89_fx radius,
                        gp89_fx height,
                        int caps);
int gp89_make_bipyramid(gp89_mesh *mesh,
                        unsigned int sides,
                        gp89_fx radius,
                        gp89_fx top_height,
                        gp89_fx bottom_height);
int gp89_make_wedge(gp89_mesh *mesh,
                    gp89_fx width,
                    gp89_fx height,
                    gp89_fx depth);
int gp89_make_regular_polyhedron(gp89_mesh *mesh,
                                 int polyhedron_kind,
                                 gp89_fx radius);
int gp89_make_custom_flat(gp89_mesh *mesh,
                          const gp89_vec3 *positions,
                          unsigned int position_count,
                          const gp89_face3 *faces,
                          unsigned int face_count,
                          gp89_fx uniform_scale);

/* Appearance. */
void gp89_color_all(gp89_mesh *mesh, gp89_rgba color);
void gp89_color_axis_gradient(gp89_mesh *mesh,
                              int axis,
                              gp89_rgba low,
                              gp89_rgba high);
void gp89_material_all(gp89_mesh *mesh, unsigned short material);
void gp89_material_cycle(gp89_mesh *mesh, unsigned short material_count);
void gp89_uv_transform(gp89_mesh *mesh,
                       gp89_fx scale_u,
                       gp89_fx scale_v,
                       gp89_fx offset_u,
                       gp89_fx offset_v);
void gp89_uv_planar(gp89_mesh *mesh, int axis_u, int axis_v);

/* Geometry processing. */
void gp89_translate(gp89_mesh *mesh, gp89_fx x, gp89_fx y, gp89_fx z);
void gp89_scale(gp89_mesh *mesh, gp89_fx x, gp89_fx y, gp89_fx z);
void gp89_rotate_xyz(gp89_mesh *mesh,
                     gp89_angle x,
                     gp89_angle y,
                     gp89_angle z);
void gp89_apply_transform(gp89_mesh *mesh, const gp89_transform *transform);
void gp89_reverse_winding(gp89_mesh *mesh);
void gp89_recalculate_smooth_normals(gp89_mesh *mesh);
void gp89_recalculate_tangents(gp89_mesh *mesh);

/* Built-in deformations and arbitrary caller callbacks. */
void gp89_deform_taper_y(gp89_mesh *mesh,
                         gp89_fx bottom_scale,
                         gp89_fx top_scale);
void gp89_deform_twist_y(gp89_mesh *mesh, gp89_angle total_turn);
void gp89_deform_shear(gp89_mesh *mesh,
                       gp89_fx x_by_y,
                       gp89_fx z_by_y);
void gp89_deform_spherize(gp89_mesh *mesh,
                          gp89_fx radius,
                          gp89_fx amount);
void gp89_deform_wave_y(gp89_mesh *mesh,
                        gp89_fx amplitude,
                        gp89_fx x_turns_per_unit,
                        gp89_fx z_turns_per_unit);
void gp89_deform_custom(gp89_mesh *mesh,
                        gp89_deform_fn deform,
                        void *user_data,
                        int rebuild_normals_and_tangents);


#ifdef __cplusplus
}
#endif

#endif
