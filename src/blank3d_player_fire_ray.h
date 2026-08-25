#ifndef BLANK3D_PLAYER_FIRE_RAY_H
#define BLANK3D_PLAYER_FIRE_RAY_H
#include "gamlib3d_math.h"
#include "blank3d_collision.h"
#include "gplayerfireray89.h"
#ifdef __cplusplus
extern "C" {
#endif
#define B3D_PLAYER_FIRE_VIEW_TPS GPFR89_VIEW_TPS
#define B3D_PLAYER_FIRE_VIEW_FPS GPFR89_VIEW_FPS
#define B3D_PLAYER_FIRE_VIEW_OTS GPFR89_VIEW_OTS
typedef int (*Blank3DPlayerFireRaycastFn)(void *user,const GWP89_Vec3 *origin,const GWP89_Vec3 *direction,gwp89_fx range_fx,unsigned int layer_mask,Blank3DCollisionHit *out_hit);
typedef struct Blank3DPlayerFireRayInputTag {
    int view_mode; Vec3 camera_origin; Vec3 camera_forward; Vec3 camera_right; Vec3 camera_up; Vec3 muzzle_origin;
    g3d_fix range; g3d_fix spread_degrees; int pellet_index; int pellet_count; unsigned int layer_mask;
} Blank3DPlayerFireRayInput;
typedef struct Blank3DPlayerFireRayResultTag {
    int valid; int camera_hit; int muzzle_hit; int blocked_from_muzzle;
    Vec3 camera_ray_origin; Vec3 camera_ray_direction; Vec3 aim_point; Vec3 tracer_origin; Vec3 tracer_direction; Vec3 impact_point;
    Blank3DCollisionHit hit;
} Blank3DPlayerFireRayResult;
int blank3d_player_fire_ray_resolve(const Blank3DPlayerFireRayInput *input,Blank3DPlayerFireRaycastFn raycast_fn,void *raycast_user,Blank3DPlayerFireRayResult *out_result);
#ifdef __cplusplus
}
#endif
#endif
