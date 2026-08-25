#ifndef GVEH_TANK_STATION_H
#define GVEH_TANK_STATION_H

#include "gveh_ai.h"

#define GVEH_TANK_STATION_DRIVER    0
#define GVEH_TANK_STATION_GUNNER    1
#define GVEH_TANK_STATION_COMMANDER 2
#define GVEH_TANK_STATION_EXTERNAL  3

typedef struct gveh_tank_station_cfg_s {
    gveh_i16 default_station;
    gveh_i16 hatch_open_allowed;
    gveh_fx driver_limit;
    gveh_fx gunner_zoom_bonus;
    gveh_fx commander_spot_bonus;
} gveh_tank_station_cfg;

typedef struct gveh_tank_station_state_s {
    gveh_i16 station;
    gveh_i16 hatch_open;
    gveh_fx view_limit;
    gveh_fx spot_bonus;
} gveh_tank_station_state;

void gveh_tank_station_default(gveh_tank_station_cfg *c);
void gveh_tank_station_clear(gveh_tank_station_state *s, const gveh_tank_station_cfg *c);
void gveh_tank_station_step(const gveh_tank_station_cfg *c, gveh_tank_station_state *s, const gveh_input *in);

#endif
