#ifndef GWEAPONAIM89_H
#define GWEAPONAIM89_H

#include "gamlib3d_math.h"
#include "soquete3d.h"
#include "gweaponballistics89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GWA89_ZERO_DISTANCE G3D_FIX_FROM_INT(32)
#define GWA89_CLEARANCE     G3D_FIX_FROM_INT(2)

typedef struct GWeaponAimFrame89Tag {
    Vec3 camera_origin;
    Vec3 camera_forward;
    Vec3 camera_right;
    Vec3 camera_up;
    Vec3 carrier_origin;
    Vec3 target;
    Vec3 launch_direction;
    int target_is_hit;
    int repaired;
} GWeaponAimFrame89;

/*
 * Builds one immutable camera/carrier frame for both weapon presentation and
 * ballistics.  The target always remains in the visible camera hemisphere and
 * in front of the physical carrier/muzzle plane.
 */
int gweaponaim89_build(
    const Vec3 *camera_origin,
    const Vec3 *camera_forward,
    const Vec3 *camera_right,
    const Vec3 *camera_up,
    const Vec3 *carrier_origin,
    const Vec3 *hit_target,
    int hit_valid,
    g3d_fix zero_distance,
    GWeaponAimFrame89 *out_frame);

/* Replaces only the carrier basis. Local -Z becomes the shared aim direction. */
int gweaponaim89_apply_pose(
    const soq3d_pose *carrier_pose,
    const GWeaponAimFrame89 *frame,
    soq3d_pose *out_pose);

/* Last gate before projectile velocity is committed. */
int gweaponaim89_guard_direction(
    const Vec3 *camera_forward,
    const Vec3 *origin,
    const Vec3 *target,
    Vec3 *in_out_direction);

/*
 * Finalizes one already camera-aligned event. Linear weapons preserve the
 * exact HUD-converged vector; only gravity/Bolt3D receive a second ballistic
 * lift solve. Every result passes the universal forward-hemisphere gate.
 */
int gweaponaim89_finalize_event(
    const GWP89_Event *event,
    const GWeaponModules89 *modules,
    GWP89_Vec3 *out_direction,
    gwp89_fx *out_gravity_fx);

#ifdef __cplusplus
}
#endif

#endif
