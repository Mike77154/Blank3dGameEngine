#ifndef GVEH_TANK_MODULES_H
#define GVEH_TANK_MODULES_H

#include "gveh_tank_armor.h"
#include "gveh_fx.h"

#define GVEH_TANK_MODULE_ENGINE       0
#define GVEH_TANK_MODULE_TRANS        1
#define GVEH_TANK_MODULE_AMMO         2
#define GVEH_TANK_MODULE_FUEL         3
#define GVEH_TANK_MODULE_TURRET_RING  4
#define GVEH_TANK_MODULE_GUN_BREECH   5
#define GVEH_TANK_MODULE_OPTICS       6
#define GVEH_TANK_MODULE_LEFT_TRACK   7
#define GVEH_TANK_MODULE_RIGHT_TRACK  8
#define GVEH_TANK_MODULE_COUNT        9

typedef struct gveh_tank_modules_cfg_s {
    gveh_fx max_health[GVEH_TANK_MODULE_COUNT];
    gveh_fx cookoff_threshold;
    gveh_fx fire_threshold;
} gveh_tank_modules_cfg;

typedef struct gveh_tank_modules_state_s {
    gveh_fx health[GVEH_TANK_MODULE_COUNT];
    gveh_i16 engine_dead;
    gveh_i16 turret_stuck;
    gveh_i16 gun_dead;
    gveh_i16 optics_bad;
    gveh_i16 ammo_cookoff;
    gveh_i16 fire;
    gveh_i16 last_module;
} gveh_tank_modules_state;

void gveh_tank_modules_default(gveh_tank_modules_cfg *c);
void gveh_tank_modules_clear(gveh_tank_modules_state *s, const gveh_tank_modules_cfg *c);
void gveh_tank_modules_apply_hit(const gveh_tank_modules_cfg *c, gveh_tank_modules_state *s, const gveh_tank_hit *hit, gveh_i32 seed, gveh_fx_queue *fxq, gveh_vec3 pos);

#endif
