#ifndef GVEH_TANK_GUN_H
#define GVEH_TANK_GUN_H

#include "gveh_ai.h"
#include "gveh_fx.h"

#define GVEH_TANK_AMMO_AP       0
#define GVEH_TANK_AMMO_APCR     1
#define GVEH_TANK_AMMO_APFSDS   2
#define GVEH_TANK_AMMO_HE       3
#define GVEH_TANK_AMMO_HEAT     4
#define GVEH_TANK_AMMO_HESH     5
#define GVEH_TANK_AMMO_SMOKE    6
#define GVEH_TANK_AMMO_COUNT    7

typedef struct gveh_tank_ammo_s {
    gveh_i16 kind;
    gveh_fx muzzle_vel;
    gveh_fx pen_close;
    gveh_fx pen_far;
    gveh_fx blast;
    gveh_fx spall;
    gveh_fx drop;
} gveh_tank_ammo;

typedef struct gveh_tank_gun_cfg_s {
    gveh_fx caliber;
    gveh_fx reload_ticks;
    gveh_fx ready_reload_ticks;
    gveh_fx recoil_force;
    gveh_i16 ammo_count[GVEH_TANK_AMMO_COUNT];
    gveh_tank_ammo ammo[GVEH_TANK_AMMO_COUNT];
    gveh_u16 flags;
} gveh_tank_gun_cfg;

typedef struct gveh_tank_gun_state_s {
    gveh_fx reload_timer;
    gveh_i16 ammo_left[GVEH_TANK_AMMO_COUNT];
    gveh_i16 selected;
    gveh_i16 fired;
    gveh_i16 smoke_ready;
    gveh_fx last_pen;
    gveh_fx last_range;
} gveh_tank_gun_state;

void gveh_tank_gun_default(gveh_tank_gun_cfg *c);
void gveh_tank_gun_modern120(gveh_tank_gun_cfg *c);
void gveh_tank_gun_soviet125(gveh_tank_gun_cfg *c);
void gveh_tank_gun_ww2_88(gveh_tank_gun_cfg *c);
void gveh_tank_gun_ww2_75(gveh_tank_gun_cfg *c);
void gveh_tank_gun_clear(gveh_tank_gun_state *s, const gveh_tank_gun_cfg *c);
const gveh_tank_ammo *gveh_tank_gun_selected_ammo(const gveh_tank_gun_cfg *c, const gveh_tank_gun_state *s);
gveh_i16 gveh_tank_gun_step(const gveh_tank_gun_cfg *c, gveh_tank_gun_state *s, const gveh_input *in, gveh_fx range_hint, gveh_fx dt, gveh_fx_queue *fxq, gveh_vec3 pos);

#endif
