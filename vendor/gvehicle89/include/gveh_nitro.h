#ifndef GVEH_NITRO_H
#define GVEH_NITRO_H

#include "gveh_math.h"

#define GVEH_NITRO_FLAG_REFILL_WHEN_CLEAN  1u
#define GVEH_NITRO_FLAG_ALLOW_AIR_USE       2u

typedef struct gveh_nitro_cfg_s {
    gveh_fx capacity;
    gveh_fx start_charge;
    gveh_fx burn_rate;
    gveh_fx refill_rate;
    gveh_fx force;
    gveh_fx min_speed;
    gveh_i16 cooldown_ticks;
    gveh_u16 flags;
} gveh_nitro_cfg;

typedef struct gveh_nitro_state_s {
    gveh_fx charge;
    gveh_fx last_force;
    gveh_i16 active;
    gveh_i16 cooldown;
} gveh_nitro_state;

void gveh_nitro_default(gveh_nitro_cfg *cfg);
void gveh_nitro_empty(gveh_nitro_state *st);
void gveh_nitro_init(gveh_nitro_state *st, const gveh_nitro_cfg *cfg);
gveh_fx gveh_nitro_step(gveh_nitro_state *st, const gveh_nitro_cfg *cfg, gveh_fx request, gveh_fx speed_abs, gveh_fx clean_factor, gveh_i32 grounded, gveh_fx dt);
void gveh_nitro_add(gveh_nitro_state *st, const gveh_nitro_cfg *cfg, gveh_fx amount);

#endif
