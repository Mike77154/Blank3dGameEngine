#include "gveh_tank_station.h"

void gveh_tank_station_default(gveh_tank_station_cfg *c)
{
    c->default_station = GVEH_TANK_STATION_GUNNER;
    c->hatch_open_allowed = 1;
    c->driver_limit = GVEH_FX_ONE / 2;
    c->gunner_zoom_bonus = GVEH_FX_ONE / 2;
    c->commander_spot_bonus = GVEH_FX_ONE;
}

void gveh_tank_station_clear(gveh_tank_station_state *s, const gveh_tank_station_cfg *c)
{
    s->station = c->default_station;
    s->hatch_open = 0;
    s->view_limit = GVEH_FX_ONE;
    s->spot_bonus = 0;
}

void gveh_tank_station_step(const gveh_tank_station_cfg *c, gveh_tank_station_state *s, const gveh_input *in)
{
    if (in->tank_station_next > 0) {
        s->station++;
        if (s->station > GVEH_TANK_STATION_EXTERNAL) s->station = GVEH_TANK_STATION_DRIVER;
    }
    if (in->tank_hatch > 0 && c->hatch_open_allowed != 0) s->hatch_open = 1 - s->hatch_open;
    s->view_limit = GVEH_FX_ONE;
    s->spot_bonus = 0;
    if (s->station == GVEH_TANK_STATION_DRIVER) s->view_limit = c->driver_limit;
    if (s->station == GVEH_TANK_STATION_GUNNER) s->spot_bonus = c->gunner_zoom_bonus;
    if (s->station == GVEH_TANK_STATION_COMMANDER) s->spot_bonus = c->commander_spot_bonus;
    if (s->hatch_open != 0) s->spot_bonus += GVEH_FX_ONE / 2;
}
