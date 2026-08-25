#include "gveh_camera.h"

void gveh_camera_default(gveh_camera *c)
{
    c->pos = gveh_v3(0,0,0);
    c->look = gveh_v3(0,0,GVEH_FX_ONE);
    c->dist = gveh_fx_from_int(7);
    c->height = gveh_fx_from_int(3);
    c->lag = GVEH_FX_ONE / 4;
    c->fov = gveh_fx_from_int(70);
}

void gveh_camera_update(gveh_camera *c, gveh_vec3 veh_pos, gveh_basis veh_basis, gveh_fx speed, gveh_fx dt)
{
    gveh_vec3 target;
    gveh_vec3 back;
    gveh_fx t;
    gveh_fx speed_boost;

    speed_boost = gveh_fx_abs(speed) / 12;
    back = gveh_v3_scale(veh_basis.fwd, -(c->dist + speed_boost));
    target = gveh_v3_add(veh_pos, back);
    target.y += c->height;
    t = gveh_fx_mul(c->lag, dt) * 8;
    if (t > GVEH_FX_ONE) t = GVEH_FX_ONE;
    c->pos.x = gveh_fx_lerp(c->pos.x, target.x, t);
    c->pos.y = gveh_fx_lerp(c->pos.y, target.y, t);
    c->pos.z = gveh_fx_lerp(c->pos.z, target.z, t);
    c->look = veh_pos;
}
