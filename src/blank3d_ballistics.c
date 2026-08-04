#include "blank3d_ballistics.h"
#include "g3dweaponzeroing89.h"

static G3DZ89_Vec3 zvec(GWP89_Vec3 value)
{
    G3DZ89_Vec3 result;
    result.x = value.x;
    result.y = value.y;
    result.z = value.z;
    return result;
}

static GWP89_Vec3 wvec(G3DZ89_Vec3 value)
{
    GWP89_Vec3 result;
    result.x = value.x;
    result.y = value.y;
    result.z = value.z;
    return result;
}

static g3dz89_fx module_gravity(const Blank3DWeaponModules *modules)
{
    g3dz89_fx gravity;
    if (!modules) return 0;
    gravity = (g3dz89_fx)(modules->gravity_q16 / 16L);
    if (modules->physics_backend == B3D_PHYSICS_GRAVITY && gravity == 0)
        gravity = gwp89_fx_from_int(-10);
    return gravity;
}

static int physics_mode(const Blank3DWeaponModules *modules)
{
    if (!modules) return G3DZ89_PHYSICS_LINEAR;
    if (modules->physics_backend == B3D_PHYSICS_GRAVITY)
        return G3DZ89_PHYSICS_GRAVITY;
    if (modules->physics_backend == B3D_PHYSICS_BOLT3D)
        return G3DZ89_PHYSICS_BOLT;
    return G3DZ89_PHYSICS_LINEAR;
}

int blank3d_ballistics_align_event_to_view(
    const GWP89_Event *event,
    const GWP89_Vec3 *camera_origin,
    const GWP89_Vec3 *camera_forward,
    GWP89_Event *out_event)
{
    G3DZ89_Request request;
    G3DZ89_Result result;
    G3DZ89_Config config;

    if (!event || !camera_origin || !camera_forward || !out_event)
        return 0;

    *out_event = *event;
    g3dz89_config_defaults(&config);
    request.camera_origin = zvec(*camera_origin);
    request.camera_forward = zvec(*camera_forward);
    request.muzzle_origin = zvec(event->origin);
    request.target_point = zvec(event->hit_point);
    request.fallback_direction = zvec(event->direction);
    request.range = event->range_fx;
    request.speed = event->speed_fx;
    request.gravity = 0;
    request.physics_mode = G3DZ89_PHYSICS_LINEAR;
    request.view_style =
        (event->view_style == GWP89_VIEW_OVER_SHOULDER ||
         event->view_style == GWP89_VIEW_THIRD_PERSON)
        ? G3DZ89_VIEW_TPS : G3DZ89_VIEW_FPS;
    request.flags = (event->flags & GWP89_EVENT_FLAG_HIT_VALID)
        ? G3DZ89_INPUT_HIT_VALID : 0;

    if (!g3dz89_solve(&config, &request, &result))
        return 0;

    out_event->hit_point = wvec(result.target_point);
    out_event->direction = wvec(result.launch_direction);
    if (result.hit_valid)
        out_event->flags |= GWP89_EVENT_FLAG_HIT_VALID;
    else
        out_event->flags &= ~GWP89_EVENT_FLAG_HIT_VALID;
    return 1;
}

int blank3d_ballistics_resolve_launch(
    const GWP89_Event *event,
    const Blank3DWeaponModules *modules,
    GWP89_Vec3 *out_direction,
    gwp89_fx *out_gravity_fx)
{
    G3DZ89_Request request;
    G3DZ89_Result result;
    G3DZ89_Config config;

    if (!event || !out_direction) return 0;

    g3dz89_config_defaults(&config);
    request.camera_origin = zvec(event->origin);
    request.camera_forward = zvec(event->direction);
    request.muzzle_origin = zvec(event->origin);
    request.target_point = zvec(event->hit_point);
    request.fallback_direction = zvec(event->direction);
    request.range = event->range_fx;
    request.speed = event->speed_fx;
    request.gravity = module_gravity(modules);
    request.physics_mode = physics_mode(modules);
    request.view_style = G3DZ89_VIEW_FPS;
    request.flags = G3DZ89_INPUT_HIT_VALID;

    if (!g3dz89_solve(&config, &request, &result))
        return 0;

    *out_direction = wvec(result.launch_direction);
    if (out_gravity_fx)
        *out_gravity_fx = (gwp89_fx)result.gravity;
    return 1;
}
