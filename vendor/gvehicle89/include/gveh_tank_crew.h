#ifndef GVEH_TANK_CREW_H
#define GVEH_TANK_CREW_H

#include "gveh_tank_armor.h"

#define GVEH_TANK_CREW_COMMANDER 0
#define GVEH_TANK_CREW_GUNNER    1
#define GVEH_TANK_CREW_LOADER    2
#define GVEH_TANK_CREW_DRIVER    3
#define GVEH_TANK_CREW_COUNT     4

typedef struct gveh_tank_crew_cfg_s {
    gveh_fx skill[GVEH_TANK_CREW_COUNT];
    gveh_fx morale;
    gveh_i16 autoloader;
    gveh_i16 crew_count;
} gveh_tank_crew_cfg;

typedef struct gveh_tank_crew_state_s {
    gveh_fx health[GVEH_TANK_CREW_COUNT];
    gveh_fx shock;
    gveh_i16 wounded[GVEH_TANK_CREW_COUNT];
    gveh_i16 alive_count;
    gveh_i16 bail;
} gveh_tank_crew_state;

void gveh_tank_crew_default(gveh_tank_crew_cfg *c);
void gveh_tank_crew_autoloader(gveh_tank_crew_cfg *c);
void gveh_tank_crew_clear(gveh_tank_crew_state *s, const gveh_tank_crew_cfg *c);
void gveh_tank_crew_apply_hit(const gveh_tank_crew_cfg *c, gveh_tank_crew_state *s, const gveh_tank_hit *hit, gveh_i32 seed);
gveh_fx gveh_tank_crew_reload_mul(const gveh_tank_crew_cfg *c, const gveh_tank_crew_state *s);
gveh_fx gveh_tank_crew_aim_mul(const gveh_tank_crew_cfg *c, const gveh_tank_crew_state *s);

#endif
