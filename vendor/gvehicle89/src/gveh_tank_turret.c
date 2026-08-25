#include "gveh_tank_turret.h"

static gveh_i32 clamp_i32(gveh_i32 v, gveh_i32 lo, gveh_i32 hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

void gveh_tank_turret_default(gveh_tank_turret_cfg *c)
{
    c->yaw_rate = 2;
    c->elev_rate = 1;
    c->yaw_limit_left = -128;
    c->yaw_limit_right = 128;
    c->elev_min = -16;
    c->elev_max = 32;
    c->stabilizer = GVEH_FX_ONE / 2;
    c->recoil_kick = gveh_fx_from_int(4);
    c->flags = 0;
}

void gveh_tank_turret_modern(gveh_tank_turret_cfg *c)
{
    gveh_tank_turret_default(c);
    c->yaw_rate = 4;
    c->elev_rate = 2;
    c->stabilizer = GVEH_FX_ONE;
    c->recoil_kick = gveh_fx_from_int(3);
}

void gveh_tank_turret_ww2(gveh_tank_turret_cfg *c)
{
    gveh_tank_turret_default(c);
    c->yaw_rate = 1;
    c->elev_rate = 1;
    c->stabilizer = 0;
    c->recoil_kick = gveh_fx_from_int(6);
}

void gveh_tank_turret_destroyer(gveh_tank_turret_cfg *c)
{
    gveh_tank_turret_default(c);
    c->yaw_rate = 1;
    c->yaw_limit_left = -10;
    c->yaw_limit_right = 10;
    c->stabilizer = 0;
}

void gveh_tank_turret_clear(gveh_tank_turret_state *s)
{
    s->yaw = 0;
    s->elev = 0;
    s->yaw_vel = 0;
    s->elev_vel = 0;
    s->recoil = 0;
    s->stabilized = 0;
}

void gveh_tank_turret_step(const gveh_tank_turret_cfg *c, gveh_tank_turret_state *s, const gveh_input *in, gveh_i32 hull_yaw, gveh_fx speed_abs, gveh_fx dt)
{
    gveh_i32 y;
    gveh_i32 e;
    gveh_i32 damp;
    (void)hull_yaw;
    (void)dt;
    y = gveh_fx_to_int(in->tank_turret) * c->yaw_rate;
    e = gveh_fx_to_int(in->tank_gun) * c->elev_rate;
    if (y == 0 && in->air_yaw != 0) y = gveh_fx_to_int(in->air_yaw) * c->yaw_rate;
    if (e == 0 && in->air_pitch != 0) e = gveh_fx_to_int(in->air_pitch) * c->elev_rate;
    s->yaw_vel += y;
    s->elev_vel += e;
    s->yaw_vel = (s->yaw_vel * 220) / 256;
    s->elev_vel = (s->elev_vel * 220) / 256;
    s->yaw += s->yaw_vel;
    s->elev += s->elev_vel;
    s->yaw = clamp_i32(s->yaw, c->yaw_limit_left, c->yaw_limit_right);
    s->elev = clamp_i32(s->elev, c->elev_min, c->elev_max);
    if (c->stabilizer > 0 && speed_abs > 0) {
        damp = gveh_fx_to_int(gveh_fx_mul(c->stabilizer, speed_abs / 16));
        if (s->elev > damp) s->elev -= damp;
        else if (s->elev < -damp) s->elev += damp;
        s->stabilized = 1;
    } else s->stabilized = 0;
    if (s->recoil > 0) s->recoil = (s->recoil * 210) / 256;
}

void gveh_tank_turret_fire_recoil(const gveh_tank_turret_cfg *c, gveh_tank_turret_state *s)
{
    s->recoil += c->recoil_kick;
    s->elev_vel -= gveh_fx_to_int(c->recoil_kick);
}
