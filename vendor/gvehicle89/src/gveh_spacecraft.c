#include "gveh_spacecraft.h"

void gveh_spacecraft_default(gveh_spacecraft_cfg *cfg)
{
    cfg->flags = 0;
    cfg->thrust = 0;
    cfg->boost_thrust = 0;
    cfg->pitch_power = 0;
    cfg->roll_power = 0;
    cfg->yaw_power = 0;
    cfg->damp_k = GVEH_FX_ONE / 8;
    cfg->shield_max = gveh_fx_from_int(100);
    cfg->shield_recharge = GVEH_FX_ONE / 2;
    cfg->laser_max = gveh_fx_from_int(100);
    cfg->laser_recharge = GVEH_FX_ONE;
    cfg->laser_cost = gveh_fx_from_int(8);
    cfg->heat_decay = GVEH_FX_ONE / 4;
}

void gveh_spacecraft_xwing(gveh_spacecraft_cfg *cfg)
{
    gveh_spacecraft_default(cfg);
    cfg->flags = GVEH_SPACE_FLAG_ENABLED | GVEH_SPACE_FLAG_ARCADE | GVEH_SPACE_FLAG_SHIELDS | GVEH_SPACE_FLAG_ENERGY | GVEH_SPACE_FLAG_INERTIA_DAMP | GVEH_SPACE_FLAG_SFOILS;
    cfg->thrust = gveh_fx_from_int(8200);
    cfg->boost_thrust = gveh_fx_from_int(2600);
    cfg->pitch_power = gveh_fx_from_int(1900);
    cfg->roll_power = gveh_fx_from_int(2600);
    cfg->yaw_power = gveh_fx_from_int(900);
    cfg->damp_k = GVEH_FX_ONE / 5;
}

void gveh_spacecraft_tie(gveh_spacecraft_cfg *cfg)
{
    gveh_spacecraft_default(cfg);
    cfg->flags = GVEH_SPACE_FLAG_ENABLED | GVEH_SPACE_FLAG_ARCADE | GVEH_SPACE_FLAG_ENERGY | GVEH_SPACE_FLAG_INERTIA_DAMP;
    cfg->thrust = gveh_fx_from_int(9400);
    cfg->boost_thrust = gveh_fx_from_int(3300);
    cfg->pitch_power = gveh_fx_from_int(2300);
    cfg->roll_power = gveh_fx_from_int(3100);
    cfg->yaw_power = gveh_fx_from_int(1100);
    cfg->shield_max = 0;
    cfg->damp_k = GVEH_FX_ONE / 4;
}

void gveh_spacecraft_bomber(gveh_spacecraft_cfg *cfg)
{
    gveh_spacecraft_xwing(cfg);
    cfg->thrust = gveh_fx_from_int(6200);
    cfg->boost_thrust = gveh_fx_from_int(1400);
    cfg->pitch_power = gveh_fx_from_int(1100);
    cfg->roll_power = gveh_fx_from_int(1300);
    cfg->yaw_power = gveh_fx_from_int(700);
    cfg->shield_max = gveh_fx_from_int(180);
    cfg->laser_max = gveh_fx_from_int(160);
}

void gveh_spacecraft_clear(gveh_spacecraft_state *st, const gveh_spacecraft_cfg *cfg)
{
    st->shield = cfg->shield_max;
    st->laser = cfg->laser_max;
    st->heat = 0;
    st->energy_engine = GVEH_FX_ONE;
    st->energy_weapon = GVEH_FX_ONE;
    st->energy_shield = GVEH_FX_ONE;
    st->overheated = 0;
    st->sfoils_open = 1;
    st->fired = 0;
}

void gveh_spacecraft_apply(gveh_spacecraft_state *st, const gveh_spacecraft_cfg *cfg, gveh_body *body, gveh_basis basis, const gveh_input *in, gveh_fx dt, gveh_fx_queue *fxq)
{
    gveh_fx throttle;
    gveh_fx thrust;
    gveh_fx recharge;
    gveh_vec3 damp;
    if ((cfg->flags & GVEH_SPACE_FLAG_ENABLED) == 0u) return;
    st->fired = 0;
    if ((cfg->flags & GVEH_SPACE_FLAG_ENERGY) != 0u) {
        st->energy_engine = GVEH_FX_ONE + in->nitro / 2 - in->brake / 4;
        st->energy_weapon = GVEH_FX_ONE + in->fire / 4 - in->nitro / 5;
        st->energy_shield = GVEH_FX_ONE + in->brake / 2 - in->fire / 5;
        st->energy_engine = gveh_fx_clamp(st->energy_engine, GVEH_FX_ONE / 4, GVEH_FX_ONE * 2);
        st->energy_weapon = gveh_fx_clamp(st->energy_weapon, GVEH_FX_ONE / 4, GVEH_FX_ONE * 2);
        st->energy_shield = gveh_fx_clamp(st->energy_shield, GVEH_FX_ONE / 4, GVEH_FX_ONE * 2);
    }
    if ((cfg->flags & GVEH_SPACE_FLAG_SFOILS) != 0u) st->sfoils_open = (in->handbrake > GVEH_FX_HALF) ? 0 : 1;
    throttle = in->throttle;
    if (throttle == 0) throttle = GVEH_FX_ONE / 2;
    thrust = gveh_fx_mul(cfg->thrust, gveh_fx_mul(throttle, st->energy_engine));
    if (in->nitro > 0) thrust += gveh_fx_mul(cfg->boost_thrust, in->nitro);
    gveh_body_add_force(body, gveh_v3_scale(basis.fwd, thrust));
    body->torque.x += gveh_fx_mul(in->air_pitch, cfg->pitch_power);
    body->torque.z += gveh_fx_mul(-in->air_roll, cfg->roll_power);
    body->torque.y += gveh_fx_mul(in->air_yaw + in->steer / 3, cfg->yaw_power);
    if ((cfg->flags & GVEH_SPACE_FLAG_INERTIA_DAMP) != 0u) {
        damp = gveh_v3_scale(body->vel, -cfg->damp_k);
        gveh_body_add_force(body, damp);
    }
    recharge = gveh_fx_mul(cfg->laser_recharge, st->energy_weapon);
    st->laser += gveh_fx_mul(recharge, dt);
    if (st->laser > cfg->laser_max) st->laser = cfg->laser_max;
    if ((cfg->flags & GVEH_SPACE_FLAG_SHIELDS) != 0u) {
        st->shield += gveh_fx_mul(gveh_fx_mul(cfg->shield_recharge, st->energy_shield), dt);
        if (st->shield > cfg->shield_max) st->shield = cfg->shield_max;
    }
    if (st->heat > 0) st->heat -= gveh_fx_mul(cfg->heat_decay, dt);
    if (st->heat < 0) st->heat = 0;
    st->overheated = (st->heat > gveh_fx_from_int(120)) ? 1 : 0;
    if (in->fire > 0 && st->laser >= cfg->laser_cost && st->overheated == 0) {
        st->laser -= cfg->laser_cost;
        st->heat += gveh_fx_from_int(6);
        st->fired = 1;
        gveh_fx_push(fxq, GVEH_FX_EVENT_ENGINE, 12, body->pos.x, body->pos.y, body->pos.z);
    }
}
