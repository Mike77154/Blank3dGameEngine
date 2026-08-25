#ifndef BLANK3D_PLAYER_PROJECTILE_AIM_H
#define BLANK3D_PLAYER_PROJECTILE_AIM_H

#include "blank3d_player_fire_ray.h"
#include "blank3d_weapon_modules.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Player-only physical projectile target selection.
 *
 * The camera ray selects the world target. The physical projectile keeps its
 * real origin, then launches from that origin toward the camera-selected
 * point. Gravity and Bolt3D compensation remain a later backend concern.
 *
 * NPCs must not call this adapter; their manager/muzzle path is unchanged.
 */
int blank3d_player_projectile_aim_prepare(
    const GWP89_Event *event,
    int view_mode,
    Blank3DPlayerFireRaycastFn raycast_fn,
    void *raycast_user,
    unsigned int layer_mask,
    GWP89_Event *out_event);

#ifdef __cplusplus
}
#endif

#endif
