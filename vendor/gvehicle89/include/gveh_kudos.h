#ifndef GVEH_KUDOS_H
#define GVEH_KUDOS_H

#include "gveh_math.h"

#define GVEH_KUDOS_DRIFT      1u
#define GVEH_KUDOS_AIR        2u
#define GVEH_KUDOS_CLEAN      4u
#define GVEH_KUDOS_RISK       8u
#define GVEH_KUDOS_COMBO     16u

typedef struct gveh_kudos_cfg_s {
    gveh_fx drift_side_min;
    gveh_fx drift_speed_min;
    gveh_fx air_speed_min;
    gveh_fx risk_min;
    gveh_fx combo_window;
    gveh_i32 drift_points;
    gveh_i32 air_points;
    gveh_i32 clean_points;
    gveh_i32 risk_points;
    gveh_i32 crash_penalty;
} gveh_kudos_cfg;

typedef struct gveh_kudos_state_s {
    gveh_i32 score;
    gveh_i32 combo;
    gveh_fx combo_timer;
    gveh_u16 flags;
    gveh_i32 last_event;
} gveh_kudos_state;

void gveh_kudos_default(gveh_kudos_cfg *cfg);
void gveh_kudos_clear(gveh_kudos_state *st);
void gveh_kudos_step(gveh_kudos_state *st, const gveh_kudos_cfg *cfg, gveh_fx speed_fwd, gveh_fx speed_side, gveh_fx grounded_ratio, gveh_fx risk_signal, gveh_i32 crashed, gveh_fx dt);

#endif
