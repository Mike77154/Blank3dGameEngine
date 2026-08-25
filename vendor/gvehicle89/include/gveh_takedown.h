#ifndef GVEH_TAKEDOWN_H
#define GVEH_TAKEDOWN_H

#include "gveh_math.h"

#define GVEH_TAKEDOWN_HEAVY_IMPACT 1u
#define GVEH_TAKEDOWN_SIDE_HIT     2u
#define GVEH_TAKEDOWN_CONFIRMED    4u

typedef struct gveh_takedown_cfg_s {
    gveh_fx speed_drop_min;
    gveh_fx side_hit_min;
    gveh_fx reward_boost;
    gveh_i16 cooldown_ticks;
} gveh_takedown_cfg;

typedef struct gveh_takedown_state_s {
    gveh_i32 takedowns;
    gveh_i16 cooldown;
    gveh_u16 flags;
    gveh_fx last_impact;
} gveh_takedown_state;

void gveh_takedown_default(gveh_takedown_cfg *cfg);
void gveh_takedown_clear(gveh_takedown_state *st);
gveh_i32 gveh_takedown_step(gveh_takedown_state *st, const gveh_takedown_cfg *cfg, gveh_fx prev_speed, gveh_fx new_speed, gveh_fx side_speed, gveh_fx aggression_signal);

#endif
