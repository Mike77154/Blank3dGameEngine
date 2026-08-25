#ifndef GVEH_CRASHBREAKER_H
#define GVEH_CRASHBREAKER_H

#include "gveh_body.h"

#define GVEH_CRASHBREAKER_READY  1u
#define GVEH_CRASHBREAKER_FIRED  2u

typedef struct gveh_crashbreaker_cfg_s {
    gveh_fx capacity;
    gveh_fx start_charge;
    gveh_fx gain_crash;
    gveh_fx gain_risk;
    gveh_fx impulse;
    gveh_fx lift;
    gveh_i16 cooldown_ticks;
} gveh_crashbreaker_cfg;

typedef struct gveh_crashbreaker_state_s {
    gveh_fx charge;
    gveh_i16 cooldown;
    gveh_u16 flags;
} gveh_crashbreaker_state;

void gveh_crashbreaker_default(gveh_crashbreaker_cfg *cfg);
void gveh_crashbreaker_clear(gveh_crashbreaker_state *st, const gveh_crashbreaker_cfg *cfg);
void gveh_crashbreaker_charge(gveh_crashbreaker_state *st, const gveh_crashbreaker_cfg *cfg, gveh_fx crash_impact, gveh_fx risk_signal);
gveh_i32 gveh_crashbreaker_fire(gveh_crashbreaker_state *st, const gveh_crashbreaker_cfg *cfg, gveh_body *body, gveh_basis basis, gveh_fx request);

#endif
