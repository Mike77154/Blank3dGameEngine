#ifndef BLANK3D_VPHYSICS_H
#define BLANK3D_VPHYSICS_H

#include "blank3d_collision.h"
#include "vp_api.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_VPHYS_MAX_OBJECTS 144
#define B3D_VPHYS_WORLD_ARENA_BYTES 524288UL
#define B3D_VPHYS_TOTAL_ARENA_BYTES 49152UL

#define B3D_VPHYS_SHAPE_SPHERE 1
#define B3D_VPHYS_SHAPE_BOX    2

#define B3D_VPHYS_EXTERNAL_DEMO_FIRST 70001UL
#define B3D_VPHYS_EXTERNAL_DEMO_BEACON 70005UL

typedef struct Blank3DVPhysicsObjectTag {
    int used;
    int dynamic_body;
    int collidable;
    int shape_type;
    vp_u32 external_id;
    vpBodyId body_id;
    vpTransformNodeId node_id;
    vpTransform world;
    vpVec3 half_extents;
    vp_fx collision_radius;
    vp_fx friction;
    vp_fx restitution;
    int pair_collidable;
    int ccd_enabled;
    int debug_draw;
} Blank3DVPhysicsObject;

typedef struct Blank3DVPhysicsStatsTag {
    vp_u32 steps;
    vp_u32 transform_reads;
    vp_u32 transform_writes;
    vp_u32 contacts_last_step;
    vp_u32 contacts_total;
    vp_u32 sweep_queries;
    vp_u32 sweep_hits;
} Blank3DVPhysicsStats;

typedef struct Blank3DVPhysicsTag {
    int initialized;
    int enabled;
    Blank3DCollision *collision;
    vpWorld *world;
    vpTotalSolver *total;
    vpTransformProvider transform_provider;
    vpCollisionProvider collision_provider;
    Blank3DVPhysicsObject objects[B3D_VPHYS_MAX_OBJECTS];
    int object_count;
    Blank3DVPhysicsStats stats;
    vp_u8 world_arena[B3D_VPHYS_WORLD_ARENA_BYTES];
    vp_u8 total_arena[B3D_VPHYS_TOTAL_ARENA_BYTES];
    char status[192];
} Blank3DVPhysics;

int blank3d_vphysics_init(Blank3DVPhysics *physics,
                          Blank3DCollision *collision);
void blank3d_vphysics_set_enabled(Blank3DVPhysics *physics, int enabled);
int blank3d_vphysics_is_enabled(const Blank3DVPhysics *physics);

/* Explicit provider getters: callers may inspect or route the same ports used
 * by vpTotalSolver without reaching into Blank3DVPhysics internals. */
const vpTransformProvider *blank3d_vphysics_get_transform_provider(
    const Blank3DVPhysics *physics);
const vpCollisionProvider *blank3d_vphysics_get_collision_provider(
    const Blank3DVPhysics *physics);
vpTotalSolver *blank3d_vphysics_get_total_solver(Blank3DVPhysics *physics);
vpWorld *blank3d_vphysics_get_world(Blank3DVPhysics *physics);

int blank3d_vphysics_create_dynamic_box_q12(
    Blank3DVPhysics *physics, vp_u32 external_id,
    long x_q12, long y_q12, long z_q12,
    long half_x_q12, long half_y_q12, long half_z_q12,
    long mass_q12);

int blank3d_vphysics_create_dynamic_sphere_q12(
    Blank3DVPhysics *physics, vp_u32 external_id,
    long x_q12, long y_q12, long z_q12,
    long radius_q12, long mass_q12);

int blank3d_vphysics_create_kinematic_box_q12(
    Blank3DVPhysics *physics, vp_u32 external_id,
    long x_q12, long y_q12, long z_q12,
    long half_x_q12, long half_y_q12, long half_z_q12,
    int collidable);

int blank3d_vphysics_destroy_object(Blank3DVPhysics *physics,
                                    vp_u32 external_id);

int blank3d_vphysics_get_transform(const Blank3DVPhysics *physics,
                                   vp_u32 external_id,
                                   vpTransform *out_transform);
int blank3d_vphysics_set_transform(Blank3DVPhysics *physics,
                                   vp_u32 external_id,
                                   const vpTransform *transform);
int blank3d_vphysics_move_q12(Blank3DVPhysics *physics,
                              vp_u32 external_id,
                              long dx_q12, long dy_q12, long dz_q12);
int blank3d_vphysics_rotate_y_q12(Blank3DVPhysics *physics,
                                  vp_u32 external_id,
                                  long yaw_degrees_q12);
int blank3d_vphysics_scale_q12(Blank3DVPhysics *physics,
                               vp_u32 external_id,
                               long sx_q12, long sy_q12, long sz_q12);
int blank3d_vphysics_set_position_q12(Blank3DVPhysics *physics,
                                      vp_u32 external_id,
                                      long x_q12, long y_q12, long z_q12);
int blank3d_vphysics_set_velocity_q12(Blank3DVPhysics *physics,
                                      vp_u32 external_id,
                                      long vx_q12, long vy_q12, long vz_q12,
                                      long wx_q12, long wy_q12, long wz_q12);
int blank3d_vphysics_add_impulse_q12(Blank3DVPhysics *physics,
                                     vp_u32 external_id,
                                     long ix_q12, long iy_q12, long iz_q12);

/* Per-object tuning used by lightweight props such as spent casings. Values
 * are Q20.12 at the engine boundary and converted once to VPhysics Q16.16. */
int blank3d_vphysics_configure_object_q12(
    Blank3DVPhysics *physics, vp_u32 external_id,
    long friction_q12, long restitution_q12,
    long linear_damping_q12, long angular_damping_q12,
    int pair_collidable, int debug_draw);
int blank3d_vphysics_set_ccd_enabled(
    Blank3DVPhysics *physics, vp_u32 external_id, int enabled);
int blank3d_vphysics_body_is_asleep(
    const Blank3DVPhysics *physics, vp_u32 external_id);
int blank3d_vphysics_get_velocity_q12(
    const Blank3DVPhysics *physics, vp_u32 external_id,
    long *vx_q12, long *vy_q12, long *vz_q12,
    long *wx_q12, long *wy_q12, long *wz_q12);
int blank3d_vphysics_get_draw_matrix_scaled_q12(
    const Blank3DVPhysics *physics, vp_u32 external_id,
    long sx_q12, long sy_q12, long sz_q12,
    signed int out_matrix[4][4]);

void blank3d_vphysics_step_q12(Blank3DVPhysics *physics, long dt_q12);
void blank3d_vphysics_spawn_demo(Blank3DVPhysics *physics);
void blank3d_vphysics_update_demo(Blank3DVPhysics *physics,
                                  long time_q12);

int blank3d_vphysics_object_count(const Blank3DVPhysics *physics);
const Blank3DVPhysicsObject *blank3d_vphysics_object_at(
    const Blank3DVPhysics *physics, int index);
int blank3d_vphysics_get_draw_matrix_q16(
    const Blank3DVPhysics *physics, vp_u32 external_id,
    signed int out_matrix[4][4]);

const Blank3DVPhysicsStats *blank3d_vphysics_stats(
    const Blank3DVPhysics *physics);
const char *blank3d_vphysics_status(const Blank3DVPhysics *physics);

#ifdef __cplusplus
}
#endif

#endif
