#ifndef BLANK3D_UNIVERSAL_AIM_H
#define BLANK3D_UNIVERSAL_AIM_H

#include "../vendor/gamlib3d/math_helpers/gamlib3d_math.h"
#include "../vendor/soquete3d/soquete3d.h"
#include "blank3d_ballistics.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_UNIVERSAL_AIM_ZERO_DISTANCE G3D_FIX_FROM_INT(32)
#define B3D_UNIVERSAL_AIM_CLEARANCE     G3D_FIX_FROM_INT(2)

typedef struct Blank3DUniversalAimFrameTag {
    Vec3 camera_origin;
    Vec3 camera_forward;
    Vec3 camera_right;
    Vec3 camera_up;
    Vec3 carrier_origin;
    Vec3 target;
    Vec3 launch_direction;
    int target_is_hit;
    int repaired;
} Blank3DUniversalAimFrame;

/*
 * Builds one immutable camera/carrier frame for both weapon presentation and
 * ballistics.  The target always remains in the visible camera hemisphere and
 * in front of the physical carrier/muzzle plane.
 */
int blank3d_universal_aim_build(
    const Vec3 *camera_origin,
    const Vec3 *camera_forward,
    const Vec3 *camera_right,
    const Vec3 *camera_up,
    const Vec3 *carrier_origin,
    const Vec3 *hit_target,
    int hit_valid,
    g3d_fix zero_distance,
    Blank3DUniversalAimFrame *out_frame);

/* Replaces only the carrier basis. Local -Z becomes the shared aim direction. */
int blank3d_universal_aim_apply_pose(
    const soq3d_pose *carrier_pose,
    const Blank3DUniversalAimFrame *frame,
    soq3d_pose *out_pose);

/* Last gate before projectile velocity is committed. */
int blank3d_universal_aim_guard_direction(
    const Vec3 *camera_forward,
    const Vec3 *origin,
    const Vec3 *target,
    Vec3 *in_out_direction);

/*
 * Finalizes one already camera-aligned event. Linear weapons preserve the
 * exact HUD-converged vector; only gravity/Bolt3D receive a second ballistic
 * lift solve. Every result passes the universal forward-hemisphere gate.
 */
int blank3d_universal_aim_finalize_event(
    const GWP89_Event *event,
    const Blank3DWeaponModules *modules,
    GWP89_Vec3 *out_direction,
    gwp89_fx *out_gravity_fx);

#ifdef __cplusplus
}
#endif

#endif
