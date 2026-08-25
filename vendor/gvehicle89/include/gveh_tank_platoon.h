#ifndef GVEH_TANK_PLATOON_H
#define GVEH_TANK_PLATOON_H

#include "gveh_ai.h"

#define GVEH_TANK_ORDER_HOLD       0
#define GVEH_TANK_ORDER_ADVANCE    1
#define GVEH_TANK_ORDER_HULLDOWN   2
#define GVEH_TANK_ORDER_FIRE       3
#define GVEH_TANK_ORDER_SMOKE      4
#define GVEH_TANK_ORDER_OVERWATCH  5

typedef struct gveh_tank_platoon_cfg_s {
    gveh_i16 wing_tanks;
    gveh_fx formation_spacing;
    gveh_fx order_cooldown;
    gveh_fx support_strength;
} gveh_tank_platoon_cfg;

typedef struct gveh_tank_platoon_state_s {
    gveh_i16 order;
    gveh_i16 wing_alive;
    gveh_fx cooldown;
    gveh_i16 hull_down;
    gveh_i16 smoke_screen;
    gveh_i32 platoon_score;
} gveh_tank_platoon_state;

void gveh_tank_platoon_default(gveh_tank_platoon_cfg *c);
void gveh_tank_platoon_clear(gveh_tank_platoon_state *s, const gveh_tank_platoon_cfg *c);
void gveh_tank_platoon_step(const gveh_tank_platoon_cfg *c, gveh_tank_platoon_state *s, const gveh_input *in, gveh_i16 fired, gveh_i16 penetrated, gveh_fx dt);

#endif
