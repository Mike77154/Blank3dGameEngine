#ifndef BLANK3D_BALLISTICS_H
#define BLANK3D_BALLISTICS_H

#include "gweapon89.h"
#include "blank3d_weapon_modules.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Converts the crosshair-converged pose emitted by gweapon89 into the launch
 * direction used by the physical projectile backend. Linear projectiles keep
 * the exact muzzle-to-crosshair direction. Gravity/Bolt3D projectiles receive
 * a fixed-point low-arc zeroing correction based on the same gravity value
 * used by their flight provider.
 */
/*
 * Reconciles a projectile event with the camera ray used to render the HUD.
 * If a provider/convention mismatch places the target or launch direction in
 * the hemisphere behind the current view, the target is rebuilt in front of
 * the camera at the same range and the muzzle direction is recomputed.
 */
int blank3d_ballistics_align_event_to_view(
    const GWP89_Event *event,
    const GWP89_Vec3 *camera_origin,
    const GWP89_Vec3 *camera_forward,
    GWP89_Event *out_event);

int blank3d_ballistics_resolve_launch(
    const GWP89_Event *event,
    const Blank3DWeaponModules *modules,
    GWP89_Vec3 *out_direction,
    gwp89_fx *out_gravity_fx);

#ifdef __cplusplus
}
#endif

#endif
