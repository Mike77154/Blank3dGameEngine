#include "gweaponlaunch89.h"
#include "gamlib3d_math.h"

static G3DZ89_Vec3 gwl89_to_zero_vec(GWP89_Vec3 value)
{
    G3DZ89_Vec3 result;
    result.x = (g3dz89_fx)value.x;
    result.y = (g3dz89_fx)value.y;
    result.z = (g3dz89_fx)value.z;
    return result;
}

static GWP89_Vec3 gwl89_from_zero_vec(G3DZ89_Vec3 value)
{
    GWP89_Vec3 result;
    result.x = (gwp89_fx)value.x;
    result.y = (gwp89_fx)value.y;
    result.z = (gwp89_fx)value.z;
    return result;
}

static int gwl89_view_style(const GWP89_Event *event)
{
    if (event == 0) return G3DZ89_VIEW_FPS;
    if (event->view_style == GWP89_VIEW_OVER_SHOULDER ||
        event->view_style == GWP89_VIEW_THIRD_PERSON)
        return G3DZ89_VIEW_TPS;
    return G3DZ89_VIEW_FPS;
}

void gwl89_physics_config_defaults(GWL89_PhysicsConfig *config)
{
    G3DZ89_Config zeroing;
    if (config == 0) return;
    g3dz89_config_defaults(&zeroing);
    config->physics_mode = G3DZ89_PHYSICS_LINEAR;
    config->gravity_fx = 0;
    config->tps_zero_distance = (gwp89_fx)zeroing.tps_zero_distance;
    config->tps_muzzle_clearance = (gwp89_fx)zeroing.tps_muzzle_clearance;
    config->min_flight_time = (gwp89_fx)zeroing.min_flight_time;
    config->max_flight_time = (gwp89_fx)zeroing.max_flight_time;
    config->ballistic_iterations = zeroing.ballistic_iterations;
}

static void gwl89_zeroing_config(const GWL89_PhysicsConfig *physics,
                                 G3DZ89_Config *out_config)
{
    GWL89_PhysicsConfig defaults;
    const GWL89_PhysicsConfig *use;
    if (out_config == 0) return;
    gwl89_physics_config_defaults(&defaults);
    use = physics != 0 ? physics : &defaults;
    out_config->tps_zero_distance = (g3dz89_fx)use->tps_zero_distance;
    out_config->tps_muzzle_clearance =
        (g3dz89_fx)use->tps_muzzle_clearance;
    out_config->min_flight_time = (g3dz89_fx)use->min_flight_time;
    out_config->max_flight_time = (g3dz89_fx)use->max_flight_time;
    out_config->ballistic_iterations = use->ballistic_iterations;
}

int gwl89_align_event_to_view(
    const GWP89_Event *event,
    const GWP89_Vec3 *camera_origin,
    const GWP89_Vec3 *camera_forward,
    GWP89_Event *out_event)
{
    G3DZ89_Request request;
    G3DZ89_Result result;
    G3DZ89_Config config;

    if (event == 0 || camera_origin == 0 || camera_forward == 0 ||
        out_event == 0)
        return 0;

    *out_event = *event;
    g3dz89_config_defaults(&config);
    request.camera_origin = gwl89_to_zero_vec(*camera_origin);
    request.camera_forward = gwl89_to_zero_vec(*camera_forward);
    request.muzzle_origin = gwl89_to_zero_vec(event->origin);
    request.target_point = gwl89_to_zero_vec(event->hit_point);
    request.fallback_direction = gwl89_to_zero_vec(event->direction);
    request.range = (g3dz89_fx)event->range_fx;
    request.speed = (g3dz89_fx)event->speed_fx;
    request.gravity = 0;
    request.physics_mode = G3DZ89_PHYSICS_LINEAR;
    request.view_style = gwl89_view_style(event);
    request.flags = (event->flags & GWP89_EVENT_FLAG_HIT_VALID)
        ? G3DZ89_INPUT_HIT_VALID : 0UL;

    if (!g3dz89_solve(&config, &request, &result)) return 0;

    out_event->hit_point = gwl89_from_zero_vec(result.target_point);
    out_event->direction = gwl89_from_zero_vec(result.launch_direction);
    if (result.hit_valid)
        out_event->flags |= GWP89_EVENT_FLAG_HIT_VALID;
    else
        out_event->flags &= ~GWP89_EVENT_FLAG_HIT_VALID;
    return 1;
}


static Vec3 gwl89_vec3(GWP89_Vec3 value)
{
    return gamlib_vec3((g3d_fix)value.x,
                       (g3d_fix)value.y,
                       (g3d_fix)value.z);
}

static GWP89_Vec3 gwl89_gwp(Vec3 value)
{
    GWP89_Vec3 result;
    result.x = (gwp89_fx)value.x;
    result.y = (gwp89_fx)value.y;
    result.z = (gwp89_fx)value.z;
    return result;
}

static Vec3 gwl89_normalized(Vec3 value, Vec3 fallback)
{
    Vec3 result;
    if (gamlib_vec3_length(&value) <= G3D_FIX_EPSILON)
        value = fallback;
    if (gamlib_vec3_length(&value) <= G3D_FIX_EPSILON)
        value = gamlib_vec3(0, 0, G3D_FIX_ONE);
    gamlib_vec3_normalize(&result, &value);
    return result;
}

int gwl89_guard_direction(
    const GWP89_Vec3 *camera_forward,
    const GWP89_Vec3 *origin,
    const GWP89_Vec3 *target,
    GWP89_Vec3 *in_out_direction)
{
    Vec3 forward;
    Vec3 direction;
    Vec3 delta;
    int repaired;
    if (camera_forward == 0 || in_out_direction == 0) return 0;
    forward = gwl89_normalized(gwl89_vec3(*camera_forward),
                               gamlib_vec3(0, 0, G3D_FIX_ONE));
    direction = gwl89_normalized(gwl89_vec3(*in_out_direction), forward);
    repaired = 0;
    if (gamlib_vec3_dot(&direction, &forward) <= 0) {
        if (origin != 0 && target != 0) {
            { Vec3 tv = gwl89_vec3(*target); Vec3 ov = gwl89_vec3(*origin); gamlib_vec3_sub(&delta, &tv, &ov); }
            direction = gwl89_normalized(delta, forward);
        }
        if (gamlib_vec3_dot(&direction, &forward) <= 0)
            direction = forward;
        repaired = 1;
    }
    *in_out_direction = gwl89_gwp(direction);
    return repaired ? 2 : 1;
}

int gwl89_finalize_event(
    const GWP89_Event *event,
    const GWL89_PhysicsConfig *physics,
    GWP89_Vec3 *out_direction,
    gwp89_fx *out_gravity_fx,
    unsigned long *out_result_flags)
{
    GWL89_PhysicsConfig defaults;
    const GWL89_PhysicsConfig *use;
    G3DZ89_Request request;
    G3DZ89_Result result;
    G3DZ89_Config config;

    if (event == 0 || out_direction == 0) return 0;

    gwl89_physics_config_defaults(&defaults);
    use = physics != 0 ? physics : &defaults;
    gwl89_zeroing_config(use, &config);

    request.camera_origin = gwl89_to_zero_vec(event->camera_origin);
    request.camera_forward = gwl89_to_zero_vec(event->camera_forward);
    request.muzzle_origin = gwl89_to_zero_vec(event->origin);
    request.target_point = gwl89_to_zero_vec(event->hit_point);
    request.fallback_direction = gwl89_to_zero_vec(event->direction);
    request.range = (g3dz89_fx)event->range_fx;
    request.speed = (g3dz89_fx)event->speed_fx;
    request.gravity = (g3dz89_fx)use->gravity_fx;
    request.physics_mode = use->physics_mode;
    request.view_style = gwl89_view_style(event);
    request.flags = (event->flags & GWP89_EVENT_FLAG_HIT_VALID)
        ? G3DZ89_INPUT_HIT_VALID : 0UL;

    if (!g3dz89_solve(&config, &request, &result)) return 0;

    *out_direction = gwl89_from_zero_vec(result.launch_direction);
    if (out_gravity_fx != 0)
        *out_gravity_fx = (gwp89_fx)result.gravity;
    if (out_result_flags != 0)
        *out_result_flags = result.flags;
    return 1;
}
