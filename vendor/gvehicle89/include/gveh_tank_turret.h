#ifndef GVEH_TANK_TURRET_H
#define GVEH_TANK_TURRET_H

#include "gveh_ai.h"

typedef struct gveh_tank_turret_cfg_s {
    gveh_i32 yaw_rate;
    gveh_i32 elev_rate;
    gveh_i32 yaw_limit_left;
    gveh_i32 yaw_limit_right;
    gveh_i32 elev_min;
    gveh_i32 elev_max;
    gveh_fx stabilizer;
    gveh_fx recoil_kick;
    gveh_u16 flags;
} gveh_tank_turret_cfg;

typedef struct gveh_tank_turret_state_s {
    gveh_i32 yaw;
    gveh_i32 elev;
    gveh_i32 yaw_vel;
    gveh_i32 elev_vel;
    gveh_fx recoil;
    gveh_i16 stabilized;
} gveh_tank_turret_state;

void gveh_tank_turret_default(gveh_tank_turret_cfg *c);
void gveh_tank_turret_modern(gveh_tank_turret_cfg *c);
void gveh_tank_turret_ww2(gveh_tank_turret_cfg *c);
void gveh_tank_turret_destroyer(gveh_tank_turret_cfg *c);
void gveh_tank_turret_clear(gveh_tank_turret_state *s);
void gveh_tank_turret_step(const gveh_tank_turret_cfg *c, gveh_tank_turret_state *s, const gveh_input *in, gveh_i32 hull_yaw, gveh_fx speed_abs, gveh_fx dt);
void gveh_tank_turret_fire_recoil(const gveh_tank_turret_cfg *c, gveh_tank_turret_state *s);

#endif
