#ifndef GVEH_TANK_FIRECONTROL_H
#define GVEH_TANK_FIRECONTROL_H

#include "gveh_tank_turret.h"
#include "gveh_tank_gun.h"

typedef struct gveh_tank_firecontrol_cfg_s {
    gveh_fx range_max;
    gveh_fx range_noise;
    gveh_fx lead_strength;
    gveh_fx stabilizer_bonus;
    gveh_i16 thermal;
    gveh_i16 aux_sight;
    gveh_i16 zoom_steps;
} gveh_tank_firecontrol_cfg;

typedef struct gveh_tank_firecontrol_state_s {
    gveh_fx range;
    gveh_fx lead;
    gveh_i16 ranged;
    gveh_i16 locked;
    gveh_i16 zoom;
    gveh_i16 warning;
    gveh_fx accuracy;
} gveh_tank_firecontrol_state;

void gveh_tank_firecontrol_default(gveh_tank_firecontrol_cfg *c);
void gveh_tank_firecontrol_modern(gveh_tank_firecontrol_cfg *c);
void gveh_tank_firecontrol_ww2(gveh_tank_firecontrol_cfg *c);
void gveh_tank_firecontrol_clear(gveh_tank_firecontrol_state *s);
void gveh_tank_firecontrol_step(const gveh_tank_firecontrol_cfg *c, gveh_tank_firecontrol_state *s, const gveh_input *in, const gveh_tank_turret_state *turret, const gveh_tank_gun_state *gun, gveh_fx speed_abs, gveh_i32 tick);

#endif
