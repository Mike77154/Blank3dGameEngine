#include "gveh_crashbreaker.h"

void gveh_crashbreaker_default(gveh_crashbreaker_cfg *cfg)
{
    cfg->capacity = gveh_fx_from_int(100);
    cfg->start_charge = 0;
    cfg->gain_crash = gveh_fx_from_int(6);
    cfg->gain_risk = gveh_fx_from_int(10);
    cfg->impulse = gveh_fx_from_int(2600);
    cfg->lift = gveh_fx_from_int(1400);
    cfg->cooldown_ticks = 90;
}

void gveh_crashbreaker_clear(gveh_crashbreaker_state *st, const gveh_crashbreaker_cfg *cfg)
{
    st->charge = cfg->start_charge;
    st->cooldown = 0;
    st->flags = 0;
}

void gveh_crashbreaker_charge(gveh_crashbreaker_state *st, const gveh_crashbreaker_cfg *cfg, gveh_fx crash_impact, gveh_fx risk_signal)
{
    gveh_fx add;
    if (st->cooldown > 0) st->cooldown--;
    add = 0;
    if (crash_impact > 0) add += gveh_fx_mul(crash_impact, cfg->gain_crash);
    if (risk_signal > 0) add += gveh_fx_mul(risk_signal, cfg->gain_risk);
    st->charge += add;
    if (st->charge > cfg->capacity) st->charge = cfg->capacity;
    if (st->charge >= cfg->capacity) st->flags |= GVEH_CRASHBREAKER_READY;
    else st->flags &= (gveh_u16)(~GVEH_CRASHBREAKER_READY);
}

gveh_i32 gveh_crashbreaker_fire(gveh_crashbreaker_state *st, const gveh_crashbreaker_cfg *cfg, gveh_body *body, gveh_basis basis, gveh_fx request)
{
    gveh_vec3 f;
    if (request <= 0) return 0;
    if (st->cooldown > 0) return 0;
    if (st->charge < cfg->capacity) return 0;
    f = gveh_v3_add(gveh_v3_scale(basis.fwd, cfg->impulse), gveh_v3(0, cfg->lift, 0));
    gveh_body_add_force(body, f);
    gveh_body_add_yaw_torque(body, cfg->impulse / 16);
    st->charge = 0;
    st->cooldown = cfg->cooldown_ticks;
    st->flags = GVEH_CRASHBREAKER_FIRED;
    return 1;
}
