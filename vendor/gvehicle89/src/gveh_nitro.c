#include "gveh_nitro.h"

void gveh_nitro_default(gveh_nitro_cfg *cfg)
{
    cfg->capacity = gveh_fx_from_int(100);
    cfg->start_charge = gveh_fx_from_int(50);
    cfg->burn_rate = gveh_fx_from_int(28);
    cfg->refill_rate = gveh_fx_from_int(6);
    cfg->force = gveh_fx_from_int(2600);
    cfg->min_speed = gveh_fx_from_int(2);
    cfg->cooldown_ticks = 10;
    cfg->flags = GVEH_NITRO_FLAG_REFILL_WHEN_CLEAN | GVEH_NITRO_FLAG_ALLOW_AIR_USE;
}

void gveh_nitro_empty(gveh_nitro_state *st)
{
    st->charge = 0;
    st->last_force = 0;
    st->active = GVEH_FALSE;
    st->cooldown = 0;
}

void gveh_nitro_init(gveh_nitro_state *st, const gveh_nitro_cfg *cfg)
{
    st->charge = gveh_fx_clamp(cfg->start_charge, 0, cfg->capacity);
    st->last_force = 0;
    st->active = GVEH_FALSE;
    st->cooldown = 0;
}

void gveh_nitro_add(gveh_nitro_state *st, const gveh_nitro_cfg *cfg, gveh_fx amount)
{
    st->charge += amount;
    if (st->charge > cfg->capacity) st->charge = cfg->capacity;
    if (st->charge < 0) st->charge = 0;
}

gveh_fx gveh_nitro_step(gveh_nitro_state *st, const gveh_nitro_cfg *cfg, gveh_fx request, gveh_fx speed_abs, gveh_fx clean_factor, gveh_i32 grounded, gveh_fx dt)
{
    gveh_fx burn;
    gveh_fx refill;
    gveh_fx out_force;

    st->last_force = 0;
    st->active = GVEH_FALSE;
    if (st->cooldown > 0) st->cooldown--;

    if ((cfg->flags & GVEH_NITRO_FLAG_REFILL_WHEN_CLEAN) != 0u) {
        refill = gveh_fx_mul(cfg->refill_rate, clean_factor);
        refill = gveh_fx_mul(refill, dt);
        gveh_nitro_add(st, cfg, refill);
    }

    if (request <= 0) return 0;
    if (st->cooldown > 0) return 0;
    if (st->charge <= 0) return 0;
    if (speed_abs < cfg->min_speed) return 0;
    if (grounded == 0 && (cfg->flags & GVEH_NITRO_FLAG_ALLOW_AIR_USE) == 0u) return 0;

    burn = gveh_fx_mul(cfg->burn_rate, request);
    burn = gveh_fx_mul(burn, dt);
    if (burn < GVEH_FX_ONE / 16) burn = GVEH_FX_ONE / 16;
    if (burn > st->charge) burn = st->charge;
    st->charge -= burn;
    if (st->charge <= 0) {
        st->charge = 0;
        st->cooldown = cfg->cooldown_ticks;
    }

    out_force = gveh_fx_mul(cfg->force, request);
    st->last_force = out_force;
    st->active = GVEH_TRUE;
    return out_force;
}
