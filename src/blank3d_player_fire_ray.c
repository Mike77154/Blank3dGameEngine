#include "blank3d_player_fire_ray.h"
#include <string.h>

typedef struct B3DPFRBridgeTag { Blank3DPlayerFireRaycastFn fn; void *user; } B3DPFRBridge;
static GWP89_Vec3 b3d_pfr_gwp(Vec3 v){GWP89_Vec3 r;r.x=v.x;r.y=v.y;r.z=v.z;return r;}
static Vec3 b3d_pfr_vec(GWP89_Vec3 v){return gamlib_vec3(v.x,v.y,v.z);}
static int b3d_pfr_bridge(void *u,const GWP89_Vec3 *o,const GWP89_Vec3 *d,gwp89_fx range,unsigned int layers,GPlayerFireRayHit89 *out){
    B3DPFRBridge *b=(B3DPFRBridge*)u; Blank3DCollisionHit h;
    if (!b || !b->fn || !out) return 0;
    memset(&h, 0, sizeof(h));
    h.enemy_index = -1;
    if(!b->fn(b->user,o,d,range,layers,&h)) return 0;
    memset(out,0,sizeof(*out)); out->hit=h.hit; out->target_index=h.enemy_index; out->material_id=h.material_id; out->fraction_fx=h.fraction_fx; out->distance_fx=h.distance_fx; out->point=h.point; out->normal=h.normal; return 1;
}
int blank3d_player_fire_ray_resolve(const Blank3DPlayerFireRayInput *in,Blank3DPlayerFireRaycastFn fn,void *user,Blank3DPlayerFireRayResult *out){
    GPlayerFireRayInput89 q; GPlayerFireRayResult89 r; B3DPFRBridge b;
    if (!in || !fn || !out) return 0;
    memset(&q, 0, sizeof(q));
    q.view_mode=in->view_mode; q.camera_origin=b3d_pfr_gwp(in->camera_origin); q.camera_forward=b3d_pfr_gwp(in->camera_forward); q.camera_right=b3d_pfr_gwp(in->camera_right); q.camera_up=b3d_pfr_gwp(in->camera_up); q.muzzle_origin=b3d_pfr_gwp(in->muzzle_origin); q.range=in->range; q.spread_degrees=in->spread_degrees; q.pellet_index=in->pellet_index; q.pellet_count=in->pellet_count; q.layer_mask=in->layer_mask;
    b.fn=fn; b.user=user; if(!gplayerfireray89_resolve(&q,b3d_pfr_bridge,&b,&r)) return 0;
    memset(out,0,sizeof(*out)); out->valid=r.valid; out->camera_hit=r.camera_hit; out->muzzle_hit=r.muzzle_hit; out->blocked_from_muzzle=r.blocked_from_muzzle; out->camera_ray_origin=b3d_pfr_vec(r.camera_ray_origin); out->camera_ray_direction=b3d_pfr_vec(r.camera_ray_direction); out->aim_point=b3d_pfr_vec(r.aim_point); out->tracer_origin=b3d_pfr_vec(r.tracer_origin); out->tracer_direction=b3d_pfr_vec(r.tracer_direction); out->impact_point=b3d_pfr_vec(r.impact_point);
    out->hit.hit=r.hit.hit; out->hit.enemy_index=r.hit.target_index; out->hit.material_id=r.hit.material_id; out->hit.fraction_fx=r.hit.fraction_fx; out->hit.distance_fx=r.hit.distance_fx; out->hit.point=r.hit.point; out->hit.normal=r.hit.normal; return 1;
}
