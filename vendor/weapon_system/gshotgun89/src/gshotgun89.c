#include "gshotgun89.h"

#include "gamlib3d_math.h"

static Vec3 b3d_shotgun_vec(GWP89_Vec3 value)
{
    return gamlib_vec3((g3d_fix)value.x,
                       (g3d_fix)value.y,
                       (g3d_fix)value.z);
}

static GWP89_Vec3 b3d_shotgun_gwp(Vec3 value)
{
    GWP89_Vec3 result;
    result.x = (gwp89_fx)value.x;
    result.y = (gwp89_fx)value.y;
    result.z = (gwp89_fx)value.z;
    return result;
}

static Vec3 b3d_shotgun_normalized_or(Vec3 value, Vec3 fallback)
{
    Vec3 result;
    if (gamlib_vec3_length(&value) <= G3D_FIX_EPSILON) {
        if (gamlib_vec3_length(&fallback) <= G3D_FIX_EPSILON)
            return gamlib_vec3(0, 0, G3D_FIX_ONE);
        gamlib_vec3_normalize(&result, &fallback);
        return result;
    }
    gamlib_vec3_normalize(&result, &value);
    return result;
}

static g3d_fix b3d_shotgun_dot(Vec3 a, Vec3 b)
{
    g3d_fix result;
    result = g3d_fix_mul(a.x, b.x);
    result = g3d_fix_add_sat(result, g3d_fix_mul(a.y, b.y));
    result = g3d_fix_add_sat(result, g3d_fix_mul(a.z, b.z));
    return result;
}

int gshotgun89_prepare_pellet(
    const GWP89_Event *source,
    const GWP89_Vec3 *camera_origin,
    const GWP89_Vec3 *camera_forward,
    const GWP89_Vec3 *camera_right,
    const GWP89_Vec3 *camera_up,
    const GWP89_Vec3 *center_target,
    GWP89_Event *out_event)
{
    static const int kx[GSHOT89_PELLET_COUNT] = {
        0, -1000, 1000, -650, 650, -300, 300
    };
    static const int ky[GSHOT89_PELLET_COUNT] = {
        0, 0, 0, 700, 700, -850, -850
    };
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 center;
    Vec3 center_delta;
    Vec3 center_ray;
    Vec3 pellet_ray;
    Vec3 pellet_target;
    Vec3 muzzle;
    Vec3 launch_delta;
    Vec3 launch_direction;
    Vec3 term;
    g3d_fix target_distance;
    g3d_fix spread_tangent;
    g3d_fix ox;
    g3d_fix oy;
    int index;

    if (!source || !camera_origin || !camera_forward ||
        !camera_right || !camera_up || !center_target || !out_event)
        return 0;
    if (source->weapon_id != GSHOT89_WEAPON_ID) return 0;

    *out_event = *source;
    index = source->pellet_index;
    if (index < 0) index = 0;
    index %= GSHOT89_PELLET_COUNT;

    eye = b3d_shotgun_vec(*camera_origin);
    forward = b3d_shotgun_normalized_or(
        b3d_shotgun_vec(*camera_forward), gamlib_vec3(0, 0, G3D_FIX_ONE));
    right = b3d_shotgun_normalized_or(
        b3d_shotgun_vec(*camera_right), gamlib_vec3(G3D_FIX_ONE, 0, 0));
    up = b3d_shotgun_normalized_or(
        b3d_shotgun_vec(*camera_up), gamlib_vec3(0, G3D_FIX_ONE, 0));
    center = b3d_shotgun_vec(*center_target);
    gamlib_vec3_sub(&center_delta, &center, &eye);
    target_distance = gamlib_vec3_length(&center_delta);
    if (target_distance <= G3D_FIX_EPSILON)
        target_distance = source->range_fx > 0
            ? (g3d_fix)source->range_fx
            : G3D_FIX_FROM_INT(28);
    center_ray = b3d_shotgun_normalized_or(center_delta, forward);
    if (b3d_shotgun_dot(center_ray, forward) <= 0)
        center_ray = forward;

    /* spread_fx is authored in degrees. degrees / 57 approximates tan(theta)
       for this gameplay-sized cone without float/double. */
    spread_tangent = (g3d_fix)(source->spread_fx / 57L);
    if (spread_tangent <= 0)
        spread_tangent = (g3d_fix)(gwp89_fx_from_text("7.0") / 57L);
    if (spread_tangent > G3D_FIX_HALF)
        spread_tangent = G3D_FIX_HALF;

    ox = (spread_tangent * (g3d_fix)kx[index]) / 1000L;
    oy = (spread_tangent * (g3d_fix)ky[index]) / 1000L;
    pellet_ray = center_ray;
    gamlib_vec3_scale(&term, &right, ox);
    gamlib_vec3_add(&pellet_ray, &pellet_ray, &term);
    gamlib_vec3_scale(&term, &up, oy);
    gamlib_vec3_add(&pellet_ray, &pellet_ray, &term);
    pellet_ray = b3d_shotgun_normalized_or(pellet_ray, center_ray);
    if (b3d_shotgun_dot(pellet_ray, forward) <= 0)
        pellet_ray = center_ray;

    gamlib_vec3_scale(&term, &pellet_ray, target_distance);
    gamlib_vec3_add(&pellet_target, &eye, &term);
    muzzle = b3d_shotgun_vec(source->origin);
    gamlib_vec3_sub(&launch_delta, &pellet_target, &muzzle);
    launch_direction = b3d_shotgun_normalized_or(launch_delta, pellet_ray);
    if (b3d_shotgun_dot(launch_direction, forward) <= 0)
        launch_direction = pellet_ray;

    out_event->pellet_index = index;
    out_event->pellet_count = GSHOT89_PELLET_COUNT;
    out_event->projectile_id = 3;
    out_event->projectile_mesh_id = 3;
    out_event->direction = b3d_shotgun_gwp(launch_direction);
    out_event->hit_point = b3d_shotgun_gwp(pellet_target);
    if (index != 0)
        out_event->flags &= ~GWP89_EVENT_FLAG_HIT_VALID;
    if (out_event->life_ms < GSHOT89_MIN_LIFE_MS)
        out_event->life_ms = GSHOT89_MIN_LIFE_MS;
    if (out_event->projectile_mesh_scale_fx < gwp89_fx_from_text("0.36"))
        out_event->projectile_mesh_scale_fx = gwp89_fx_from_text("0.36");
    return 1;
}
