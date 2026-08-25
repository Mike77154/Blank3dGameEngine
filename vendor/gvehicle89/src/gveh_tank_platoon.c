#include "gveh_tank_platoon.h"

void gveh_tank_platoon_default(gveh_tank_platoon_cfg *c)
{
    c->wing_tanks = 3;
    c->formation_spacing = gveh_fx_from_int(18);
    c->order_cooldown = gveh_fx_from_int(2);
    c->support_strength = GVEH_FX_ONE;
}

void gveh_tank_platoon_clear(gveh_tank_platoon_state *s, const gveh_tank_platoon_cfg *c)
{
    s->order = GVEH_TANK_ORDER_HOLD;
    s->wing_alive = c->wing_tanks;
    s->cooldown = 0;
    s->hull_down = 0;
    s->smoke_screen = 0;
    s->platoon_score = 0;
}

void gveh_tank_platoon_step(const gveh_tank_platoon_cfg *c, gveh_tank_platoon_state *s, const gveh_input *in, gveh_i16 fired, gveh_i16 penetrated, gveh_fx dt)
{
    if (s->cooldown > 0) {
        s->cooldown -= dt;
        if (s->cooldown < 0) s->cooldown = 0;
    }
    if (in->tank_order > 0 && s->cooldown <= 0) {
        s->order++;
        if (s->order > GVEH_TANK_ORDER_OVERWATCH) s->order = GVEH_TANK_ORDER_HOLD;
        s->cooldown = c->order_cooldown;
    }
    s->hull_down = (s->order == GVEH_TANK_ORDER_HULLDOWN) ? 1 : 0;
    if (s->order == GVEH_TANK_ORDER_SMOKE) s->smoke_screen = 45;
    else if (s->smoke_screen > 0) s->smoke_screen--;
    if (fired != 0) s->platoon_score += 1;
    if (penetrated != 0) s->platoon_score += 10 + s->wing_alive;
    (void)c;
}
