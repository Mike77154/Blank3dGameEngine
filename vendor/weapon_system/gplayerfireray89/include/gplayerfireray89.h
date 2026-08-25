#ifndef GPLAYERFIRERAY89_H
#define GPLAYERFIRERAY89_H

#include "gweapon89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GPFR89_VIEW_TPS 0
#define GPFR89_VIEW_FPS 1
#define GPFR89_VIEW_OTS 2

typedef struct GPlayerFireRayHit89Tag {
    int hit;
    int target_index;
    int material_id;
    gwp89_fx fraction_fx;
    gwp89_fx distance_fx;
    GWP89_Vec3 point;
    GWP89_Vec3 normal;
} GPlayerFireRayHit89;

typedef int (*GPlayerFireRay89RaycastFn)(
    void *user,
    const GWP89_Vec3 *origin,
    const GWP89_Vec3 *direction,
    gwp89_fx range_fx,
    unsigned int layer_mask,
    GPlayerFireRayHit89 *out_hit);

typedef struct GPlayerFireRayInput89Tag {
    int view_mode;
    GWP89_Vec3 camera_origin;
    GWP89_Vec3 camera_forward;
    GWP89_Vec3 camera_right;
    GWP89_Vec3 camera_up;
    GWP89_Vec3 muzzle_origin;
    gwp89_fx range;
    gwp89_fx spread_degrees;
    int pellet_index;
    int pellet_count;
    unsigned int layer_mask;
} GPlayerFireRayInput89;

typedef struct GPlayerFireRayResult89Tag {
    int valid;
    int camera_hit;
    int muzzle_hit;
    int blocked_from_muzzle;
    GWP89_Vec3 camera_ray_origin;
    GWP89_Vec3 camera_ray_direction;
    GWP89_Vec3 aim_point;
    GWP89_Vec3 tracer_origin;
    GWP89_Vec3 tracer_direction;
    GWP89_Vec3 impact_point;
    GPlayerFireRayHit89 hit;
} GPlayerFireRayResult89;

int gplayerfireray89_resolve(
    const GPlayerFireRayInput89 *input,
    GPlayerFireRay89RaycastFn raycast_fn,
    void *raycast_user,
    GPlayerFireRayResult89 *out_result);

#ifdef __cplusplus
}
#endif

#endif
