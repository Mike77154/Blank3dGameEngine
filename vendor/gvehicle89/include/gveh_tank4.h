#ifndef GVEH_TANK4_H
#define GVEH_TANK4_H

#include "gveh_tank_tracks.h"
#include "gveh_tank_turret.h"
#include "gveh_tank_gun.h"
#include "gveh_tank_armor.h"
#include "gveh_tank_modules.h"
#include "gveh_tank_crew.h"
#include "gveh_tank_firecontrol.h"
#include "gveh_tank_station.h"
#include "gveh_tank_platoon.h"

typedef struct gveh_tank4_cfg_s {
    gveh_tank_tracks_cfg tracks;
    gveh_tank_turret_cfg turret;
    gveh_tank_gun_cfg gun;
    gveh_tank_armor_cfg armor;
    gveh_tank_modules_cfg modules;
    gveh_tank_crew_cfg crew;
    gveh_tank_firecontrol_cfg firecontrol;
    gveh_tank_station_cfg station;
    gveh_tank_platoon_cfg platoon;
    gveh_i16 enabled;
} gveh_tank4_cfg;

typedef struct gveh_tank4_state_s {
    gveh_tank_tracks_state tracks;
    gveh_tank_turret_state turret;
    gveh_tank_gun_state gun;
    gveh_tank_modules_state modules;
    gveh_tank_crew_state crew;
    gveh_tank_firecontrol_state firecontrol;
    gveh_tank_station_state station;
    gveh_tank_platoon_state platoon;
    gveh_tank_hit last_hit;
    gveh_i16 last_fired;
    gveh_i16 last_penetrated;
    gveh_i16 alive;
} gveh_tank4_state;

void gveh_tank4_default(gveh_tank4_cfg *c);
void gveh_tank4_modern(gveh_tank4_cfg *c);
void gveh_tank4_t72(gveh_tank4_cfg *c);
void gveh_tank4_tiger(gveh_tank4_cfg *c);
void gveh_tank4_sherman(gveh_tank4_cfg *c);
void gveh_tank4_destroyer(gveh_tank4_cfg *c);
void gveh_tank4_scout(gveh_tank4_cfg *c);
void gveh_tank4_clear(gveh_tank4_state *s, const gveh_tank4_cfg *c);
void gveh_tank4_step(const gveh_tank4_cfg *c, gveh_tank4_state *s, gveh_body *body, gveh_basis basis, const gveh_input *in, gveh_fx grounded_ratio, gveh_fx speed_abs, gveh_fx dt, gveh_i32 tick, gveh_fx_queue *fxq);

#endif
