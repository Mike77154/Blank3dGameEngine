#include "gveh_kudos.h"

void gveh_kudos_default(gveh_kudos_cfg *cfg)
{
    cfg->drift_side_min = gveh_fx_from_int(4);
    cfg->drift_speed_min = gveh_fx_from_int(10);
    cfg->air_speed_min = gveh_fx_from_int(12);
    cfg->risk_min = GVEH_FX_HALF;
    cfg->combo_window = gveh_fx_from_int(2);
    cfg->drift_points = 8;
    cfg->air_points = 5;
    cfg->clean_points = 1;
    cfg->risk_points = 10;
    cfg->crash_penalty = 30;
}

void gveh_kudos_clear(gveh_kudos_state *st)
{
    st->score = 0;
    st->combo = 0;
    st->combo_timer = 0;
    st->flags = 0;
    st->last_event = 0;
}

static void gveh_kudos_add(gveh_kudos_state *st, const gveh_kudos_cfg *cfg, gveh_i32 pts, gveh_u16 flag)
{
    gveh_i32 mul;
    st->flags |= flag;
    st->last_event += pts;
    st->combo_timer = cfg->combo_window;
    st->combo++;
    mul = 1 + (st->combo / 8);
    if (mul > 8) mul = 8;
    st->score += pts * mul;
}

void gveh_kudos_step(gveh_kudos_state *st, const gveh_kudos_cfg *cfg, gveh_fx speed_fwd, gveh_fx speed_side, gveh_fx grounded_ratio, gveh_fx risk_signal, gveh_i32 crashed, gveh_fx dt)
{
    gveh_fx af;
    gveh_fx as;

    st->flags = 0;
    st->last_event = 0;
    if (st->combo_timer > 0) {
        st->combo_timer -= dt;
        if (st->combo_timer <= 0) {
            st->combo_timer = 0;
            st->combo = 0;
        }
    }

    if (crashed != 0) {
        if (st->score > cfg->crash_penalty) st->score -= cfg->crash_penalty;
        else st->score = 0;
        st->combo = 0;
        st->combo_timer = 0;
        st->flags |= GVEH_KUDOS_CLEAN;
        return;
    }

    af = gveh_fx_abs(speed_fwd);
    as = gveh_fx_abs(speed_side);
    if (af > cfg->drift_speed_min && as > cfg->drift_side_min && grounded_ratio > GVEH_FX_HALF) {
        gveh_kudos_add(st, cfg, cfg->drift_points, GVEH_KUDOS_DRIFT);
    }
    if (af > cfg->air_speed_min && grounded_ratio < GVEH_FX_HALF) {
        gveh_kudos_add(st, cfg, cfg->air_points, GVEH_KUDOS_AIR);
    }
    if (risk_signal > cfg->risk_min) {
        gveh_kudos_add(st, cfg, cfg->risk_points, GVEH_KUDOS_RISK);
    }
    if (af > GVEH_FX_ONE && st->last_event == 0 && grounded_ratio > GVEH_FX_HALF) {
        st->score += cfg->clean_points;
        st->flags |= GVEH_KUDOS_CLEAN;
    }
    if (st->combo > 1) st->flags |= GVEH_KUDOS_COMBO;
}
