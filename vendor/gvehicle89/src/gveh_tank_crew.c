#include "gveh_tank_crew.h"

void gveh_tank_crew_default(gveh_tank_crew_cfg *c)
{
    gveh_i32 i;
    i = 0;
    while (i < GVEH_TANK_CREW_COUNT) { c->skill[i] = GVEH_FX_ONE; i++; }
    c->morale = GVEH_FX_ONE;
    c->autoloader = 0;
    c->crew_count = GVEH_TANK_CREW_COUNT;
}

void gveh_tank_crew_autoloader(gveh_tank_crew_cfg *c)
{
    gveh_tank_crew_default(c);
    c->autoloader = 1;
    c->crew_count = 3;
    c->skill[GVEH_TANK_CREW_LOADER] = GVEH_FX_ONE / 2;
}

void gveh_tank_crew_clear(gveh_tank_crew_state *s, const gveh_tank_crew_cfg *c)
{
    gveh_i32 i;
    i = 0;
    while (i < GVEH_TANK_CREW_COUNT) { s->health[i] = gveh_fx_from_int(100); s->wounded[i] = 0; i++; }
    s->shock = 0;
    s->alive_count = c->crew_count;
    s->bail = 0;
}

void gveh_tank_crew_apply_hit(const gveh_tank_crew_cfg *c, gveh_tank_crew_state *s, const gveh_tank_hit *hit, gveh_i32 seed)
{
    gveh_i32 idx;
    gveh_fx dmg;
    if (hit->shock > 0) s->shock += hit->shock / 4;
    if (hit->penetrated == 0) return;
    idx = seed % GVEH_TANK_CREW_COUNT;
    if (idx < 0) idx = -idx;
    if (c->autoloader != 0 && idx == GVEH_TANK_CREW_LOADER) idx = GVEH_TANK_CREW_GUNNER;
    dmg = hit->spall / 2;
    if (dmg < gveh_fx_from_int(20)) dmg = gveh_fx_from_int(20);
    s->health[idx] -= dmg;
    if (s->health[idx] <= 0 && s->wounded[idx] == 0) {
        s->wounded[idx] = 1;
        s->alive_count--;
        if (s->alive_count < 0) s->alive_count = 0;
    }
    if (s->alive_count <= 1 || s->shock > gveh_fx_from_int(240)) s->bail = 1;
}

gveh_fx gveh_tank_crew_reload_mul(const gveh_tank_crew_cfg *c, const gveh_tank_crew_state *s)
{
    if (c->autoloader != 0) return GVEH_FX_ONE;
    if (s->wounded[GVEH_TANK_CREW_LOADER] != 0) return gveh_fx_from_int(2);
    return GVEH_FX_ONE;
}

gveh_fx gveh_tank_crew_aim_mul(const gveh_tank_crew_cfg *c, const gveh_tank_crew_state *s)
{
    gveh_fx m;
    m = GVEH_FX_ONE;
    if (s->wounded[GVEH_TANK_CREW_GUNNER] != 0) m += GVEH_FX_ONE;
    if (s->wounded[GVEH_TANK_CREW_COMMANDER] != 0) m += GVEH_FX_ONE / 2;
    if (s->shock > gveh_fx_from_int(80)) m += GVEH_FX_ONE / 2;
    (void)c;
    return m;
}
