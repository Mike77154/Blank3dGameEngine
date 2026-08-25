#include "gveh_tank_firecontrol.h"

void gveh_tank_firecontrol_default(gveh_tank_firecontrol_cfg *c)
{
    c->range_max = gveh_fx_from_int(2500);
    c->range_noise = gveh_fx_from_int(90);
    c->lead_strength = GVEH_FX_ONE / 2;
    c->stabilizer_bonus = GVEH_FX_ONE / 2;
    c->thermal = 0;
    c->aux_sight = 1;
    c->zoom_steps = 3;
}

void gveh_tank_firecontrol_modern(gveh_tank_firecontrol_cfg *c)
{
    gveh_tank_firecontrol_default(c);
    c->range_max = gveh_fx_from_int(4200);
    c->range_noise = gveh_fx_from_int(18);
    c->lead_strength = GVEH_FX_ONE;
    c->stabilizer_bonus = GVEH_FX_ONE;
    c->thermal = 1;
    c->zoom_steps = 5;
}

void gveh_tank_firecontrol_ww2(gveh_tank_firecontrol_cfg *c)
{
    gveh_tank_firecontrol_default(c);
    c->range_max = gveh_fx_from_int(1600);
    c->range_noise = gveh_fx_from_int(160);
    c->lead_strength = GVEH_FX_ONE / 4;
    c->stabilizer_bonus = 0;
    c->thermal = 0;
    c->zoom_steps = 2;
}

void gveh_tank_firecontrol_clear(gveh_tank_firecontrol_state *s)
{
    s->range = gveh_fx_from_int(800);
    s->lead = 0;
    s->ranged = 0;
    s->locked = 0;
    s->zoom = 1;
    s->warning = 0;
    s->accuracy = GVEH_FX_ONE / 2;
}

void gveh_tank_firecontrol_step(const gveh_tank_firecontrol_cfg *c, gveh_tank_firecontrol_state *s, const gveh_input *in, const gveh_tank_turret_state *turret, const gveh_tank_gun_state *gun, gveh_fx speed_abs, gveh_i32 tick)
{
    gveh_fx noise;
    if (in->tank_range > 0 || in->lock_on > 0) {
        noise = ((tick * 37) & 31) * c->range_noise / 32;
        s->range = gveh_fx_from_int(700) + noise + (speed_abs * 3);
        if (s->range > c->range_max) s->range = c->range_max;
        s->ranged = 1;
        s->locked = 1;
    } else {
        s->ranged = 0;
        if ((tick & 31) == 0) s->locked = 0;
    }
    if (in->tank_station_next > 0) {
        s->zoom++;
        if (s->zoom > c->zoom_steps) s->zoom = 1;
    }
    s->lead = gveh_fx_mul(speed_abs, c->lead_strength) / 8;
    s->accuracy = GVEH_FX_ONE;
    if (turret->stabilized != 0) s->accuracy += c->stabilizer_bonus;
    if (gun->reload_timer > 0) s->accuracy -= GVEH_FX_ONE / 3;
    if (s->accuracy < GVEH_FX_ONE / 4) s->accuracy = GVEH_FX_ONE / 4;
    s->warning = (in->risk > GVEH_FX_ONE / 2) ? 1 : 0;
}
