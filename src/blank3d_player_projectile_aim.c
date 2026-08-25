#include "blank3d_player_projectile_aim.h"
#include "gplayerprojectileaim89.h"

#include <string.h>

typedef struct B3DPlayerAimProviderTag {
    Blank3DPlayerFireRaycastFn raycast_fn;
    void *raycast_user;
} B3DPlayerAimProvider;

static Vec3 b3d_ppa_from_vendor(gppa89_vec3 value)
{
    return gamlib_vec3((g3d_fix)value.x,
                       (g3d_fix)value.y,
                       (g3d_fix)value.z);
}

static gppa89_vec3 b3d_ppa_to_vendor(Vec3 value)
{
    gppa89_vec3 out;
    out.x = (gppa89_fx)value.x;
    out.y = (gppa89_fx)value.y;
    out.z = (gppa89_fx)value.z;
    return out;
}

static gppa89_vec3 b3d_ppa_gwp_to_vendor(GWP89_Vec3 value)
{
    gppa89_vec3 out;
    out.x = (gppa89_fx)value.x;
    out.y = (gppa89_fx)value.y;
    out.z = (gppa89_fx)value.z;
    return out;
}

static GWP89_Vec3 b3d_ppa_vendor_to_gwp(gppa89_vec3 value)
{
    GWP89_Vec3 out;
    out.x = (gwp89_fx)value.x;
    out.y = (gwp89_fx)value.y;
    out.z = (gwp89_fx)value.z;
    return out;
}

static int b3d_ppa_target_provider(void *user,
                                   const gppa89_query *query,
                                   gppa89_target *target_out)
{
    B3DPlayerAimProvider *provider;
    Blank3DPlayerFireRayInput input;
    Blank3DPlayerFireRayResult result;
    provider = (B3DPlayerAimProvider *)user;
    if (!provider || !query || !target_out || !provider->raycast_fn)
        return 0;
    memset(&input, 0, sizeof(input));
    input.view_mode = query->view_mode;
    input.camera_origin = b3d_ppa_from_vendor(query->camera_origin);
    input.camera_forward = b3d_ppa_from_vendor(query->camera_forward);
    input.camera_right = b3d_ppa_from_vendor(query->camera_right);
    input.camera_up = b3d_ppa_from_vendor(query->camera_up);
    input.muzzle_origin = b3d_ppa_from_vendor(query->muzzle_origin);
    input.range = (g3d_fix)query->range_fx;
    input.spread_degrees = 0;
    input.pellet_index = 0;
    input.pellet_count = 1;
    input.layer_mask = query->layer_mask;
    if (!blank3d_player_fire_ray_resolve(&input,
                                         provider->raycast_fn,
                                         provider->raycast_user,
                                         &result) || !result.valid)
        return 0;
    memset(target_out, 0, sizeof(*target_out));
    target_out->valid = 1;
    target_out->hit = result.hit.hit ? 1 : 0;
    target_out->blocked_from_muzzle = result.blocked_from_muzzle ? 1 : 0;
    target_out->target = b3d_ppa_to_vendor(result.impact_point);
    return 1;
}

int blank3d_player_projectile_aim_prepare(
    const GWP89_Event *event,
    int view_mode,
    Blank3DPlayerFireRaycastFn raycast_fn,
    void *raycast_user,
    unsigned int layer_mask,
    GWP89_Event *out_event)
{
    gppa89_query query;
    gppa89_result result;
    B3DPlayerAimProvider provider;
    if (!event || !out_event || !raycast_fn) return 0;
    memset(&query, 0, sizeof(query));
    query.view_mode = view_mode;
    query.camera_origin = b3d_ppa_gwp_to_vendor(event->camera_origin);
    query.camera_forward = b3d_ppa_gwp_to_vendor(event->camera_forward);
    query.camera_right = b3d_ppa_gwp_to_vendor(event->camera_right);
    query.camera_up = b3d_ppa_gwp_to_vendor(event->camera_up);
    query.muzzle_origin = b3d_ppa_gwp_to_vendor(event->origin);
    query.fallback_direction = b3d_ppa_gwp_to_vendor(event->direction);
    query.range_fx = event->range_fx > 0
                   ? (gppa89_fx)event->range_fx
                   : (gppa89_fx)G3D_FIX_FROM_INT(120);
    query.layer_mask = layer_mask;
    provider.raycast_fn = raycast_fn;
    provider.raycast_user = raycast_user;
    if (!gplayerprojectileaim89_resolve(&query,
                                         b3d_ppa_target_provider,
                                         &provider,
                                         &result))
        return 0;
    *out_event = *event;
    out_event->hit_point = b3d_ppa_vendor_to_gwp(result.target);
    out_event->direction = b3d_ppa_vendor_to_gwp(result.direction);
    if (result.hit) out_event->flags |= GWP89_EVENT_FLAG_HIT_VALID;
    else out_event->flags &= ~GWP89_EVENT_FLAG_HIT_VALID;
    return 1;
}
