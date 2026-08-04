#ifndef BLANK3D_SHOTGUN_H
#define BLANK3D_SHOTGUN_H

#include "gweapon89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_SHOTGUN_WEAPON_ID 3
#define B3D_SHOTGUN_PELLET_COUNT 7
#define B3D_SHOTGUN_MIN_LIFE_MS 2800U

/*
 * Rebuild one runtime pellet around the actual center-HUD target.
 * This is intentionally a runner-side finalizer: it guarantees that the
 * physical projectile pool receives seven distinct rays even if an upstream
 * camera/zeroing provider collapses the manager-authored spread.
 */
int blank3d_shotgun_prepare_pellet(
    const GWP89_Event *source,
    const GWP89_Vec3 *camera_origin,
    const GWP89_Vec3 *camera_forward,
    const GWP89_Vec3 *camera_right,
    const GWP89_Vec3 *camera_up,
    const GWP89_Vec3 *center_target,
    GWP89_Event *out_event);

#ifdef __cplusplus
}
#endif

#endif
