#ifndef BLANK3D_PROJECTILEVISUAL2D89_BRIDGE_H
#define BLANK3D_PROJECTILEVISUAL2D89_BRIDGE_H

#include "blank3d_projectile_sprite89.h"
#include "blank3d_weapon_modules.h"
#include "gamlib3d_transform.h"
#include "projectilevisual2d89.h"

#ifdef __cplusplus
extern "C" {
#endif

int blank3d_projectilevisual2d89_build(
    Blank3DProjectileSpriteRuntime89 *sprites,
    const Blank3DWeaponModules *modules,
    const Vec3 *position_q12,
    const Vec3 *camera_right_q12,
    const Vec3 *camera_up_q12,
    g3d_fix mesh_scale_q12,
    unsigned int age_ms,
    unsigned int life_ms,
    int projectile_slot,
    PV2D89Sample *out_sample);

void blank3d_projectilevisual2d89_recipe(
    const Blank3DWeaponModules *modules,
    PV2D89Recipe *out_recipe);

#ifdef __cplusplus
}
#endif

#endif
