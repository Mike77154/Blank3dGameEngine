#include "gveh_takedown.h"

void gveh_takedown_default(gveh_takedown_cfg *cfg)
{
    cfg->speed_drop_min = gveh_fx_from_int(8);
    cfg->side_hit_min = gveh_fx_from_int(5);
    cfg->reward_boost = gveh_fx_from_int(35);
    cfg->cooldown_ticks = 30;
}

void gveh_takedown_clear(gveh_takedown_state *st)
{
    st->takedowns = 0;
    st->cooldown = 0;
    st->flags = 0;
    st->last_impact = 0;
}

gveh_i32 gveh_takedown_step(gveh_takedown_state *st, const gveh_takedown_cfg *cfg, gveh_fx prev_speed, gveh_fx new_speed, gveh_fx side_speed, gveh_fx aggression_signal)
{
    gveh_fx drop;
    gveh_i32 hit;
    st->flags = 0;
    st->last_impact = 0;
    if (st->cooldown > 0) st->cooldown--;
    drop = gveh_fx_abs(prev_speed) - gveh_fx_abs(new_speed);
    if (drop < 0) drop = 0;
    st->last_impact = drop;
    hit = 0;
    if (drop > cfg->speed_drop_min) {
        st->flags |= GVEH_TAKEDOWN_HEAVY_IMPACT;
        hit = 1;
    }
    if (gveh_fx_abs(side_speed) > cfg->side_hit_min && aggression_signal > GVEH_FX_HALF) {
        st->flags |= GVEH_TAKEDOWN_SIDE_HIT;
        hit = 1;
    }
    if (hit != 0 && st->cooldown == 0) {
        st->flags |= GVEH_TAKEDOWN_CONFIRMED;
        st->takedowns++;
        st->cooldown = cfg->cooldown_ticks;
        return 1;
    }
    return 0;
}
