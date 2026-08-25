#include "gveh_skill.h"

void gveh_skill_default(gveh_skill_cfg *cfg)
{
    cfg->max_skill = gveh_fx_from_int(100);
    cfg->gain_drive = GVEH_FX_ONE / 20;
    cfg->gain_drift = GVEH_FX_ONE / 8;
    cfg->gain_air = GVEH_FX_ONE / 6;
    cfg->steer_help = GVEH_FX_ONE / 3;
    cfg->throttle_smooth = GVEH_FX_ONE / 4;
    cfg->flags = GVEH_SKILL_FLAG_HELP_STEER | GVEH_SKILL_FLAG_HELP_GRIP | GVEH_SKILL_FLAG_HELP_EJECT;
}

void gveh_skill_clear(gveh_skill_state *st)
{
    st->driver = 0;
    st->drift = 0;
    st->stunt = 0;
    gveh_input_clear(&st->prev_input);
}

void gveh_skill_apply_input(gveh_skill_state *st, const gveh_skill_cfg *cfg, const gveh_input *in, gveh_input *out_input, gveh_fx speed_abs)
{
    gveh_fx skill_t;
    gveh_fx smooth;
    *out_input = *in;
    if (cfg->max_skill <= 0) return;
    skill_t = gveh_fx_div(st->driver, cfg->max_skill);
    skill_t = gveh_fx_clamp(skill_t, 0, GVEH_FX_ONE);
    if ((cfg->flags & GVEH_SKILL_FLAG_HELP_STEER) != 0u && speed_abs > gveh_fx_from_int(18)) {
        smooth = cfg->steer_help - gveh_fx_mul(cfg->steer_help, skill_t) / 2;
        out_input->steer = gveh_fx_lerp(in->steer, st->prev_input.steer, smooth);
    }
    smooth = cfg->throttle_smooth / 2;
    out_input->throttle = gveh_fx_lerp(in->throttle, st->prev_input.throttle, smooth);
    st->prev_input = *out_input;
}

void gveh_skill_step(gveh_skill_state *st, const gveh_skill_cfg *cfg, gveh_fx speed_abs, gveh_fx side_abs, gveh_fx grounded_ratio, gveh_fx dt)
{
    gveh_fx add;
    if (speed_abs > GVEH_FX_ONE) {
        add = gveh_fx_mul(cfg->gain_drive, dt);
        st->driver += add;
    }
    if (side_abs > gveh_fx_from_int(4) && speed_abs > gveh_fx_from_int(10)) {
        add = gveh_fx_mul(cfg->gain_drift, dt);
        st->drift += add;
    }
    if (grounded_ratio < GVEH_FX_HALF && speed_abs > gveh_fx_from_int(8)) {
        add = gveh_fx_mul(cfg->gain_air, dt);
        st->stunt += add;
    }
    st->driver = gveh_fx_clamp(st->driver, 0, cfg->max_skill);
    st->drift = gveh_fx_clamp(st->drift, 0, cfg->max_skill);
    st->stunt = gveh_fx_clamp(st->stunt, 0, cfg->max_skill);
}
