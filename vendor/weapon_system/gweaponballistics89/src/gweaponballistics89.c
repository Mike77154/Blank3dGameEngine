#include "gweaponballistics89.h"
#include "gweaponlaunch89.h"

static gwp89_fx module_gravity(const GWeaponModules89 *modules)
{
    gwp89_fx gravity;
    if (!modules) return 0;
    gravity = (gwp89_fx)(modules->gravity_q16 / 16L);
    if (modules->physics_backend == GWM89_PHYSICS_GRAVITY && gravity == 0)
        gravity = gwp89_fx_from_int(-10);
    return gravity;
}

static int physics_mode(const GWeaponModules89 *modules)
{
    if (!modules) return G3DZ89_PHYSICS_LINEAR;
    if (modules->physics_backend == GWM89_PHYSICS_GRAVITY)
        return G3DZ89_PHYSICS_GRAVITY;
    if (modules->physics_backend == GWM89_PHYSICS_BOLT3D)
        return G3DZ89_PHYSICS_BOLT;
    return G3DZ89_PHYSICS_LINEAR;
}

int gweaponballistics89_align_event_to_view(
    const GWP89_Event *event,
    const GWP89_Vec3 *camera_origin,
    const GWP89_Vec3 *camera_forward,
    GWP89_Event *out_event)
{
    return gwl89_align_event_to_view(event, camera_origin,
                                     camera_forward, out_event);
}

int gweaponballistics89_resolve_launch(
    const GWP89_Event *event,
    const GWeaponModules89 *modules,
    GWP89_Vec3 *out_direction,
    gwp89_fx *out_gravity_fx)
{
    GWL89_PhysicsConfig physics;
    gwl89_physics_config_defaults(&physics);
    physics.physics_mode = physics_mode(modules);
    physics.gravity_fx = module_gravity(modules);
    return gwl89_finalize_event(event, &physics, out_direction,
                                out_gravity_fx, 0);
}
