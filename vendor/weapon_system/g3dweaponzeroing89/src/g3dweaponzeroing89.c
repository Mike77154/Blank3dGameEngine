#include "g3dweaponzeroing89.h"
#include "gamlib3d_math.h"

#define ZONE ((g3dz89_fx)G3D_FIX_ONE)

static Vec3 zv(G3DZ89_Vec3 v) { return gamlib_vec3((g3d_fix)v.x,(g3d_fix)v.y,(g3d_fix)v.z); }
static G3DZ89_Vec3 vz(Vec3 v) { G3DZ89_Vec3 r; r.x=v.x;r.y=v.y;r.z=v.z;return r; }
static g3d_fix zdot(Vec3 a, Vec3 b) {
    g3d_fix r=g3d_fix_mul(a.x,b.x);
    r=g3d_fix_add_sat(r,g3d_fix_mul(a.y,b.y));
    return g3d_fix_add_sat(r,g3d_fix_mul(a.z,b.z));
}
static Vec3 znorm(Vec3 v, Vec3 fallback) {
    Vec3 r;
    if (gamlib_vec3_length(&v)<=G3D_FIX_EPSILON) v=fallback;
    if (gamlib_vec3_length(&v)<=G3D_FIX_EPSILON) v=gamlib_vec3(0,0,G3D_FIX_ONE);
    gamlib_vec3_normalize(&r,&v); return r;
}
void g3dz89_config_defaults(G3DZ89_Config *c) {
    if (!c) return;
    c->tps_zero_distance=G3D_FIX_FROM_INT(32);
    c->tps_muzzle_clearance=G3D_FIX_FROM_INT(2);
    c->min_flight_time=G3D_FIX_ONE/64L;
    c->max_flight_time=G3D_FIX_FROM_INT(4);
    c->ballistic_iterations=4;
}
int g3dz89_solve(const G3DZ89_Config *cfg0,const G3DZ89_Request *q,G3DZ89_Result *o) {
    G3DZ89_Config d; const G3DZ89_Config *c=cfg0;
    Vec3 eye,forward,origin,target,ct,cm,mt,ray,scaled,dir,delta,adjusted;
    g3d_fix range,td,md,min_d,rebuild,speed,gabs,time,t2,lift,req,ratio;
    int tps,hit,i;
    if (!q || !o) return 0;
    if (!c) {
        g3dz89_config_defaults(&d);
        c = &d;
    }
    eye=zv(q->camera_origin); forward=znorm(zv(q->camera_forward),zv(q->fallback_direction));
    origin=zv(q->muzzle_origin); target=zv(q->target_point); tps=q->view_style==G3DZ89_VIEW_TPS;
    hit=(q->flags&G3DZ89_INPUT_HIT_VALID)!=0; o->flags=0;o->hit_valid=hit;o->gravity=q->gravity;
    gamlib_vec3_sub(&ct,&target,&eye); gamlib_vec3_sub(&cm,&origin,&eye);
    ray=znorm(ct,forward); if(zdot(ray,forward)<=0) ray=forward;
    range=gamlib_vec3_length(&ct); if(range<=G3D_FIX_EPSILON) range=q->range>0?q->range:G3D_FIX_FROM_INT(64);
    td=zdot(ct,forward); md=zdot(cm,forward); min_d=g3d_fix_add_sat(md,c->tps_muzzle_clearance);
    if(tps){
        rebuild=c->tps_zero_distance; if(q->range>0&&q->range<rebuild) rebuild=q->range; if(rebuild<min_d) rebuild=min_d;
        if(!hit||td<=min_d){ Vec3 rr=hit?forward:ray; gamlib_vec3_scale(&scaled,&rr,rebuild);gamlib_vec3_add(&target,&eye,&scaled);hit=0;o->flags|=G3DZ89_RESULT_TARGET_REBUILT; }
    } else if(td<=0){ gamlib_vec3_scale(&scaled,&forward,range);gamlib_vec3_add(&target,&eye,&scaled);hit=0;o->flags|=G3DZ89_RESULT_TARGET_REBUILT; }
    gamlib_vec3_sub(&mt,&target,&origin); dir=znorm(mt,forward);
    if(zdot(dir,forward)<=0){ Vec3 rr; rebuild=tps?c->tps_zero_distance:range;if(tps&&rebuild<min_d)rebuild=min_d;rr=(tps&&!hit)?ray:forward;if(zdot(rr,forward)<=0)rr=forward;gamlib_vec3_scale(&scaled,&rr,rebuild);gamlib_vec3_add(&target,&eye,&scaled);gamlib_vec3_sub(&mt,&target,&origin);dir=znorm(mt,forward);hit=0;o->flags|=G3DZ89_RESULT_TARGET_REBUILT|G3DZ89_RESULT_HEMISPHERE_REPAIRED; }
    if((q->physics_mode==G3DZ89_PHYSICS_GRAVITY||q->physics_mode==G3DZ89_PHYSICS_BOLT)&&q->gravity!=0&&q->speed>G3D_FIX_EPSILON){
        gamlib_vec3_sub(&delta,&target,&origin); speed=q->speed; range=gamlib_vec3_length(&delta);
        if(range>G3D_FIX_EPSILON){gabs=g3d_fix_abs(q->gravity);time=g3d_fix_clamp(g3d_fix_div(range,speed),c->min_flight_time,c->max_flight_time);adjusted=delta;
            for(i=0;i<c->ballistic_iterations;++i){t2=g3d_fix_mul(time,time);lift=g3d_fix_mul(gabs,t2)/2L;adjusted=delta;adjusted.y=g3d_fix_add_sat(adjusted.y,lift);req=g3d_fix_div(gamlib_vec3_length(&adjusted),time);if(req<=G3D_FIX_EPSILON)break;ratio=g3d_fix_div(req,speed);time=g3d_fix_clamp(g3d_fix_mul(time,ratio),c->min_flight_time,c->max_flight_time);} dir=znorm(adjusted,dir);o->flags|=G3DZ89_RESULT_BALLISTIC_COMPENSATED;}
    }
    o->target_point=vz(target);o->launch_direction=vz(dir);o->hit_valid=hit;return 1;
}
