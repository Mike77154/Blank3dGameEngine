#ifndef GVEH_RISKBOOST_H
#define GVEH_RISKBOOST_H

#include "gveh_math.h"

#define GVEH_RISK_DRIFT      1u
#define GVEH_RISK_AIR        2u
#define GVEH_RISK_SIGNAL     4u
#define GVEH_RISK_CRASH      8u

typedef struct gveh_riskboost_cfg_s {
    gveh_fx capacity;
    gveh_fx burn_rate;
    gveh_fx drift_gain;
    gveh_fx air_gain;
    gveh_fx signal_gain;
    gveh_fx takedown_gain;
    gveh_fx boost_force;
} gveh_riskboost_cfg;

typedef struct gveh_riskboost_state_s {
    gveh_fx boost;
    gveh_fx last_force;
    gveh_u16 flags;
} gveh_riskboost_state;

void gveh_riskboost_default(gveh_riskboost_cfg *cfg);
void gveh_riskboost_clear(gveh_riskboost_state *st);
void gveh_riskboost_add(gveh_riskboost_state *st, const gveh_riskboost_cfg *cfg, gveh_fx amount);
gveh_fx gveh_riskboost_step(gveh_riskboost_state *st, const gveh_riskboost_cfg *cfg, gveh_fx request, gveh_fx speed_abs, gveh_fx side_abs, gveh_fx grounded_ratio, gveh_fx risk_signal, gveh_i32 takedown, gveh_fx dt);

#endif
