#include "gplayerfireray89.h"
#include "gamlib3d_math.h"

#include <string.h>

static Vec3 gpfr89_vec(GWP89_Vec3 value)
{
    return gamlib_vec3((g3d_fix)value.x, (g3d_fix)value.y, (g3d_fix)value.z);
}

static GWP89_Vec3 gpfr89_gwp(Vec3 value)
{
    GWP89_Vec3 out;
    out.x = (gwp89_fx)value.x; out.y = (gwp89_fx)value.y; out.z = (gwp89_fx)value.z;
    return out;
}

static Vec3 gpfr89_normalized(Vec3 value, Vec3 fallback)
{
    Vec3 out;
    if (gamlib_vec3_length(&value) <= G3D_FIX_EPSILON) value = fallback;
    if (gamlib_vec3_length(&value) <= G3D_FIX_EPSILON)
        value = gamlib_vec3(0, 0, G3D_FIX_ONE);
    gamlib_vec3_normalize(&out, &value);
    return out;
}

static Vec3 gpfr89_spread_direction(const GPlayerFireRayInput89 *input,
                                    Vec3 forward, Vec3 right, Vec3 up)
{
    static const int kx[12] = {0,-1000,1000,-500,500,0,-850,850,-250,250,-650,650};
    static const int ky[12] = {0,0,0,750,750,-850,-450,-450,350,350,150,150};
    Vec3 term;
    Vec3 direction;
    g3d_fix cone_radius;
    g3d_fix ox;
    g3d_fix oy;
    int pattern;
    if (!input || input->pellet_count <= 1 || input->spread_degrees == 0)
        return forward;
    pattern = input->pellet_index % 12;
    if (pattern < 0) pattern += 12;
    cone_radius = g3d_fix_div((g3d_fix)input->spread_degrees,
                              G3D_FIX_FROM_INT(57));
    cone_radius = g3d_fix_clamp(cone_radius,
                                g3d_fix_neg_sat(G3D_FIX_HALF),
                                G3D_FIX_HALF);
    ox = (g3d_fix)(((long)cone_radius * (long)kx[pattern]) / 1000L);
    oy = (g3d_fix)(((long)cone_radius * (long)ky[pattern]) / 1000L);
    direction = forward;
    gamlib_vec3_scale(&term, &right, ox);
    gamlib_vec3_add(&direction, &direction, &term);
    gamlib_vec3_scale(&term, &up, oy);
    gamlib_vec3_add(&direction, &direction, &term);
    return gpfr89_normalized(direction, forward);
}

static int gpfr89_cast(GPlayerFireRay89RaycastFn raycast_fn,
                       void *user,
                       Vec3 origin,
                       Vec3 direction,
                       g3d_fix range,
                       unsigned int layer_mask,
                       GPlayerFireRayHit89 *out_hit)
{
    GWP89_Vec3 o;
    GWP89_Vec3 d;
    if (!raycast_fn || !out_hit || range <= 0) return 0;
    o = gpfr89_gwp(origin); d = gpfr89_gwp(direction);
    memset(out_hit, 0, sizeof(*out_hit));
    out_hit->target_index = -1;
    return raycast_fn(user, &o, &d, (gwp89_fx)range,
                      layer_mask, out_hit) ? 1 : 0;
}

int gplayerfireray89_resolve(const GPlayerFireRayInput89 *input,
                             GPlayerFireRay89RaycastFn raycast_fn,
                             void *raycast_user,
                             GPlayerFireRayResult89 *out_result)
{
    Vec3 camera_origin;
    Vec3 muzzle_origin;
    Vec3 forward;
    Vec3 right;
    Vec3 up;
    Vec3 aim_point;
    Vec3 impact_point;
    Vec3 tracer_origin;
    Vec3 tracer_direction;
    Vec3 aim_offset;
    Vec3 muzzle_delta;
    Vec3 muzzle_direction;
    Vec3 tracer_delta;
    g3d_fix muzzle_range;
    GPlayerFireRayHit89 camera_hit;
    GPlayerFireRayHit89 muzzle_hit;
    int camera_did_hit;
    int muzzle_did_hit;
    if (!input || !out_result || !raycast_fn) return 0;
    memset(out_result, 0, sizeof(*out_result));
    out_result->hit.target_index = -1;
    camera_origin = gpfr89_vec(input->camera_origin);
    muzzle_origin = gpfr89_vec(input->muzzle_origin);
    forward = gpfr89_normalized(gpfr89_vec(input->camera_forward),
                                gamlib_vec3(0,0,G3D_FIX_ONE));
    right = gpfr89_normalized(gpfr89_vec(input->camera_right),
                              gamlib_vec3(G3D_FIX_ONE,0,0));
    up = gpfr89_normalized(gpfr89_vec(input->camera_up),
                           gamlib_vec3(0,G3D_FIX_ONE,0));
    forward = gpfr89_spread_direction(input, forward, right, up);
    out_result->camera_ray_origin = input->camera_origin;
    out_result->camera_ray_direction = gpfr89_gwp(forward);
    camera_did_hit = gpfr89_cast(raycast_fn, raycast_user,
                                 camera_origin, forward,
                                 (g3d_fix)input->range,
                                 input->layer_mask, &camera_hit);
    if (camera_did_hit) aim_point = gpfr89_vec(camera_hit.point);
    else {
        gamlib_vec3_scale(&aim_offset, &forward, (g3d_fix)input->range);
        gamlib_vec3_add(&aim_point, &camera_origin, &aim_offset);
    }
    out_result->aim_point = gpfr89_gwp(aim_point);
    out_result->camera_hit = camera_did_hit;
    if (input->view_mode == GPFR89_VIEW_FPS) {
        tracer_origin = camera_origin;
        impact_point = aim_point;
        if (camera_did_hit) out_result->hit = camera_hit;
    } else {
        tracer_origin = muzzle_origin;
        gamlib_vec3_sub(&muzzle_delta, &aim_point, &muzzle_origin);
        muzzle_range = gamlib_vec3_length(&muzzle_delta);
        muzzle_direction = gpfr89_normalized(muzzle_delta, forward);
        muzzle_did_hit = 0;
        if (muzzle_range > G3D_FIX_EPSILON)
            muzzle_did_hit = gpfr89_cast(raycast_fn, raycast_user,
                                         muzzle_origin, muzzle_direction,
                                         muzzle_range, input->layer_mask,
                                         &muzzle_hit);
        out_result->muzzle_hit = muzzle_did_hit;
        if (muzzle_did_hit) {
            impact_point = gpfr89_vec(muzzle_hit.point);
            out_result->hit = muzzle_hit;
            if (!camera_did_hit ||
                muzzle_hit.distance_fx + G3D_FIX_EPSILON < muzzle_range)
                out_result->blocked_from_muzzle = 1;
        } else {
            impact_point = aim_point;
            if (camera_did_hit) out_result->hit = camera_hit;
        }
    }
    gamlib_vec3_sub(&tracer_delta, &impact_point, &tracer_origin);
    tracer_direction = gpfr89_normalized(tracer_delta, forward);
    out_result->tracer_origin = gpfr89_gwp(tracer_origin);
    out_result->impact_point = gpfr89_gwp(impact_point);
    out_result->tracer_direction = gpfr89_gwp(tracer_direction);
    out_result->valid = 1;
    return 1;
}
