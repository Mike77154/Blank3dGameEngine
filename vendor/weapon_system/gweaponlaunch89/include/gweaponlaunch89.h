#ifndef GWEAPONLAUNCH89_H
#define GWEAPONLAUNCH89_H

#include "gweapon89.h"
#include "g3dweaponzeroing89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct GWL89_PhysicsConfigTag {
    int physics_mode;
    gwp89_fx gravity_fx;
    gwp89_fx tps_zero_distance;
    gwp89_fx tps_muzzle_clearance;
    gwp89_fx min_flight_time;
    gwp89_fx max_flight_time;
    int ballistic_iterations;
} GWL89_PhysicsConfig;

void gwl89_physics_config_defaults(GWL89_PhysicsConfig *config);

/*
 * Reconciles an already-created weapon event with the rendered camera ray.
 * This is the portable launch-boundary invariant that prevents provider-axis
 * mismatches, stale target planes or camera/muzzle separation from producing
 * a launch vector behind the visible camera.
 */
int gwl89_align_event_to_view(
    const GWP89_Event *event,
    const GWP89_Vec3 *camera_origin,
    const GWP89_Vec3 *camera_forward,
    GWP89_Event *out_event);

/*
 * Final launch solve. Linear projectiles retain the camera/muzzle-converged
 * direction. Gravity/Bolt modes receive fixed-point ballistic compensation.
 * Every result still passes the forward-hemisphere invariant.
 */

/* Low-level final gate for callers that already own target selection. */
int gwl89_guard_direction(
    const GWP89_Vec3 *camera_forward,
    const GWP89_Vec3 *origin,
    const GWP89_Vec3 *target,
    GWP89_Vec3 *in_out_direction);

int gwl89_finalize_event(
    const GWP89_Event *event,
    const GWL89_PhysicsConfig *physics,
    GWP89_Vec3 *out_direction,
    gwp89_fx *out_gravity_fx,
    unsigned long *out_result_flags);

#ifdef __cplusplus
}
#endif

#endif
