#ifndef GPROJ2D89_H
#define GPROJ2D89_H

/*
 * GProj2D89
 * Renderer-neutral 2D vector ammunition silhouettes for strict C89 runtimes.
 * Numeric format: signed Q16.16 in signed long.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define GP2D_VERSION_MAJOR 1
#define GP2D_VERSION_MINOR 0
#define GP2D_VERSION_PATCH 1

#define GP2D_FX_ONE       65536L
#define GP2D_FX_HALF      32768L
#define GP2D_FX_QUARTER   16384L
#define GP2D_FX_EIGHTH     8192L
#define GP2D_FX_MAX  2147483647L
#define GP2D_FX_MIN (-2147483647L - 1L)

#define GP2D_OK                 0
#define GP2D_ERR_NULL          -1
#define GP2D_ERR_CAPACITY      -2
#define GP2D_ERR_ARGUMENT      -3
#define GP2D_ERR_UNSUPPORTED   -4
#define GP2D_ERR_CORRUPT       -5

#define GP2D_FALSE 0
#define GP2D_TRUE  1

#define GP2D_AXIS_VERTICAL   0
#define GP2D_AXIS_HORIZONTAL 1

#define GP2D_PART_SHELL      1
#define GP2D_PART_PROJECTILE 2
#define GP2D_PART_COMPLETE   3

#define GP2D_AMMO_PISTOL            1
#define GP2D_AMMO_SHOTGUN_BUCKSHOT  2
#define GP2D_AMMO_SHOTGUN_SLUG      3
#define GP2D_AMMO_MACHINE_GUN       4
#define GP2D_AMMO_MAGNUM            5
#define GP2D_AMMO_SNIPER            6
#define GP2D_AMMO_MISSILE           7
#define GP2D_AMMO_HAND_GRENADE      8
#define GP2D_AMMO_GRENADE_LAUNCHER  9
#define GP2D_AMMO_COUNT             9

#define GP2D_ROLE_GENERIC       0
#define GP2D_ROLE_SHELL_BODY    1
#define GP2D_ROLE_SHELL_RIM     2
#define GP2D_ROLE_PRIMER        3
#define GP2D_ROLE_PROJECTILE    4
#define GP2D_ROLE_JACKET        5
#define GP2D_ROLE_CORE          6
#define GP2D_ROLE_HULL          7
#define GP2D_ROLE_PELLET        8
#define GP2D_ROLE_BAND          9
#define GP2D_ROLE_FUZE         10
#define GP2D_ROLE_LEVER        11
#define GP2D_ROLE_PIN          12
#define GP2D_ROLE_FIN          13
#define GP2D_ROLE_NOZZLE       14
#define GP2D_ROLE_DETAIL       15

#define GP2D_SHAPE_CIRCLE           1
#define GP2D_SHAPE_ELLIPSE          2
#define GP2D_SHAPE_CAPSULE          3
#define GP2D_SHAPE_DROPLET          4
#define GP2D_SHAPE_ROUND_NOSE       5
#define GP2D_SHAPE_SPITZER          6
#define GP2D_SHAPE_SPITZER_BOATTAIL 7
#define GP2D_SHAPE_FLAT_POINT       8
#define GP2D_SHAPE_CASE_STRAIGHT    9
#define GP2D_SHAPE_CASE_BOTTLENECK 10
#define GP2D_SHAPE_SHOT_SHELL      11
#define GP2D_SHAPE_ROCKET          12
#define GP2D_SHAPE_HAND_GRENADE    13
#define GP2D_SHAPE_40MM_GRENADE    14

#define GP2D_PROFILE_FLAG_HAS_SHELL      1U
#define GP2D_PROFILE_FLAG_HAS_PROJECTILE 2U
#define GP2D_PROFILE_FLAG_MULTI_PATH     4U

#define GP2D_OVERRIDE_FILL    1U
#define GP2D_OVERRIDE_OUTLINE 2U

#define GP2D_ROTATION_STEPS 64U

typedef signed long gp2d_fx;

typedef struct gp2d_vec2_s {
    gp2d_fx x;
    gp2d_fx y;
} gp2d_vec2;

typedef struct gp2d_color_s {
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
} gp2d_color;

typedef struct gp2d_style_s {
    gp2d_color fill;
    gp2d_color outline;
    gp2d_fx outline_width;
    unsigned char fill_enabled;
    unsigned char outline_enabled;
} gp2d_style;

typedef struct gp2d_transform_s {
    gp2d_vec2 position;
    gp2d_vec2 scale;
    unsigned char rotation_step;
} gp2d_transform;

typedef struct gp2d_path_s {
    unsigned short first_point;
    unsigned short point_count;
    gp2d_style style;
    unsigned char closed;
    unsigned char role;
} gp2d_path;

typedef struct gp2d_scene_s {
    gp2d_path *paths;
    gp2d_vec2 *points;
    unsigned short path_capacity;
    unsigned short point_capacity;
    unsigned short path_count;
    unsigned short point_count;
    int error;
} gp2d_scene;

typedef struct gp2d_draw_options_s {
    gp2d_color fill_override;
    gp2d_color outline_override;
    gp2d_fx outline_width;
    unsigned char override_mask;
    unsigned char outline_enabled;
    unsigned char detail_enabled;
} gp2d_draw_options;

typedef struct gp2d_profile_s {
    int ammo_id;
    const char *name;
    const char *label;
    int shell_shape;
    int projectile_shape;
    unsigned char flags;
} gp2d_profile;

/* Fixed-point helpers. */
gp2d_fx gp2d_fx_from_int(long value);
gp2d_fx gp2d_fx_from_ratio(long numerator, long denominator);
long gp2d_fx_to_int(gp2d_fx value);
gp2d_fx gp2d_fx_mul(gp2d_fx a, gp2d_fx b);
gp2d_fx gp2d_fx_abs(gp2d_fx value);

/* Small value constructors. */
gp2d_vec2 gp2d_vec2_make(gp2d_fx x, gp2d_fx y);
gp2d_color gp2d_color_rgba(unsigned char r, unsigned char g,
                            unsigned char b, unsigned char a);
gp2d_transform gp2d_transform_identity(void);
gp2d_style gp2d_style_make(gp2d_color fill, gp2d_color outline,
                            gp2d_fx outline_width,
                            int fill_enabled, int outline_enabled);
gp2d_draw_options gp2d_draw_options_default(void);

/* Caller-owned scene storage. */
void gp2d_scene_init(gp2d_scene *scene,
                     gp2d_path *paths, unsigned short path_capacity,
                     gp2d_vec2 *points, unsigned short point_capacity);
void gp2d_scene_reset(gp2d_scene *scene);
int gp2d_scene_validate(const gp2d_scene *scene);

/* Low-level vector primitives. */
int gp2d_add_polygon(gp2d_scene *scene,
                     const gp2d_vec2 *local_points,
                     unsigned short point_count,
                     const gp2d_transform *transform,
                     const gp2d_style *style,
                     int closed, unsigned char role);
int gp2d_add_circle(gp2d_scene *scene, gp2d_fx radius,
                    const gp2d_transform *transform,
                    const gp2d_style *style, unsigned char role);
int gp2d_add_ellipse(gp2d_scene *scene, gp2d_fx radius_x, gp2d_fx radius_y,
                     const gp2d_transform *transform,
                     const gp2d_style *style, unsigned char role);
int gp2d_add_capsule(gp2d_scene *scene, gp2d_fx width, gp2d_fx height,
                     int axis,
                     const gp2d_transform *transform,
                     const gp2d_style *style, unsigned char role);
int gp2d_add_droplet(gp2d_scene *scene, gp2d_fx width, gp2d_fx height,
                     const gp2d_transform *transform,
                     const gp2d_style *style, unsigned char role);
int gp2d_add_round_nose(gp2d_scene *scene, gp2d_fx width, gp2d_fx height,
                        const gp2d_transform *transform,
                        const gp2d_style *style, unsigned char role);
int gp2d_add_spitzer(gp2d_scene *scene, gp2d_fx width, gp2d_fx height,
                     int boat_tail,
                     const gp2d_transform *transform,
                     const gp2d_style *style, unsigned char role);
int gp2d_add_flat_point(gp2d_scene *scene, gp2d_fx width, gp2d_fx height,
                        const gp2d_transform *transform,
                        const gp2d_style *style, unsigned char role);

/* Ammunition catalog. Geometry and ammunition identity stay separate. */
unsigned short gp2d_profile_count(void);
const gp2d_profile *gp2d_profile_at(unsigned short index);
const gp2d_profile *gp2d_profile_get(int ammo_id);
int gp2d_profile_has_part(int ammo_id, int part);
int gp2d_build_part(gp2d_scene *scene, int ammo_id, int part,
                    const gp2d_transform *transform,
                    const gp2d_draw_options *options);

/* Post-build styling. */
void gp2d_scene_recolor_all(gp2d_scene *scene, gp2d_color color);
void gp2d_scene_recolor_role(gp2d_scene *scene, unsigned char role,
                             gp2d_color color);
void gp2d_scene_set_outline(gp2d_scene *scene, int enabled,
                            gp2d_color color, gp2d_fx width);

#ifdef __cplusplus
}
#endif

#endif
