#include "gveh_riskboost.h"

void gveh_riskboost_default(gveh_riskboost_cfg *cfg)
{
    cfg->capacity = gveh_fx_from_int(100);
    cfg->burn_rate = gveh_fx_from_int(32);
    cfg->drift_gain = gveh_fx_from_int(18);
    cfg->air_gain = gveh_fx_from_int(12);
    cfg->signal_gain = gveh_fx_from_int(25);
    cfg->takedown_gain = gveh_fx_from_int(40);
    cfg->boost_force = gveh_fx_from_int(1800);
}

void gveh_riskboost_clear(gveh_riskboost_state *st)
{
    st->boost = 0;
    st->last_force = 0;
    st->flags = 0;
}

void gveh_riskboost_add(gveh_riskboost_state *st, const gveh_riskboost_cfg *cfg, gveh_fx amount)
{
    st->boost += amount;
    if (st->boost > cfg->capacity) st->boost = cfg->capacity;
    if (st->boost < 0) st->boost = 0;
}

gveh_fx gveh_riskboost_step(gveh_riskboost_state *st, const gveh_riskboost_cfg *cfg, gveh_fx request, gveh_fx speed_abs, gveh_fx side_abs, gveh_fx grounded_ratio, gveh_fx risk_signal, gveh_i32 takedown, gveh_fx dt)
{
    gveh_fx gain;
    gveh_fx burn;
    gveh_fx force;

    st->flags = 0;
    st->last_force = 0;
    gain = 0;
    if (speed_abs > gveh_fx_from_int(10) && side_abs > gveh_fx_from_int(4) && grounded_ratio > GVEH_FX_HALF) {
        gain += cfg->drift_gain;
        st->flags |= GVEH_RISK_DRIFT;
    }
    if (speed_abs > gveh_fx_from_int(12) && grounded_ratio < GVEH_FX_HALF) {
        gain += cfg->air_gain;
        st->flags |= GVEH_RISK_AIR;
    }
    if (risk_signal > 0) {
        gain += gveh_fx_mul(cfg->signal_gain, risk_signal);
        st->flags |= GVEH_RISK_SIGNAL;
    }
    if (takedown != 0) gain += cfg->takedown_gain;
    if (gain > 0) gveh_riskboost_add(st, cfg, gveh_fx_mul(gain, dt));

    if (request <= 0 || st->boost <= 0) return 0;
    burn = gveh_fx_mul(cfg->burn_rate, request);
    burn = gveh_fx_mul(burn, dt);
    if (burn < GVEH_FX_ONE / 16) burn = GVEH_FX_ONE / 16;
    if (burn > st->boost) burn = st->boost;
    st->boost -= burn;
    force = gveh_fx_mul(cfg->boost_force, request);
    st->last_force = force;
    return force;
}
