#ifndef BLANK3D_CASING_PHYSICS_H
#define BLANK3D_CASING_PHYSICS_H

#include "blank3d_vphysics.h"
#include "../vendor/gamlib3d/math_helpers/gamlib3d_math.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_CASING_PHYSICS_CAPACITY 128
#define B3D_CASING_PHYSICS_ID_BASE 80000UL

typedef struct Blank3DCasingPhysicsTag {
    Blank3DVPhysics *physics;
    unsigned long spawned;
    unsigned long rejected;
} Blank3DCasingPhysics;

void blank3d_casing_physics_init(Blank3DCasingPhysics *casings,
                                 Blank3DVPhysics *physics);
int blank3d_casing_physics_spawn(Blank3DCasingPhysics *casings,
                                 int slot,
                                 int weapon_id,
                                 int casing_mesh_id,
                                 const Vec3 *origin,
                                 const Vec3 *right,
                                 const Vec3 *up,
                                 const Vec3 *forward);
void blank3d_casing_physics_release(Blank3DCasingPhysics *casings,
                                    int slot);
int blank3d_casing_physics_get_draw_matrix_q16(
    const Blank3DCasingPhysics *casings,
    int slot,
    g3d_fix visual_scale,
    signed int out_matrix[4][4]);
int blank3d_casing_physics_is_sleeping(
    const Blank3DCasingPhysics *casings,
    int slot);
int blank3d_casing_physics_get_velocity(
    const Blank3DCasingPhysics *casings,
    int slot,
    Vec3 *linear,
    Vec3 *angular);
unsigned long blank3d_casing_physics_spawned(
    const Blank3DCasingPhysics *casings);
unsigned long blank3d_casing_physics_rejected(
    const Blank3DCasingPhysics *casings);

#ifdef __cplusplus
}
#endif

#endif
