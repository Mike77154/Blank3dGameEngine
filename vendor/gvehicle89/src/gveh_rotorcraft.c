#include "gveh_rotorcraft.h"

void gveh_rotorcraft_default(gveh_rotorcraft_cfg *cfg)
{
    cfg->flags = 0;
    cfg->main_lift = 0;
    cfg->hover_collective = GVEH_FX_ONE / 2;
    cfg->rotor_ramp = GVEH_FX_ONE / 8;
    cfg->translational_lift = 0;
    cfg->cyclic_force = 0;
    cfg->pitch_power = 0;
    cfg->roll_power = 0;
    cfg->yaw_power = 0;
    cfg->body_drag = 0;
    cfg->altitude_hold_k = 0;
    cfg->target_altitude = gveh_fx_from_int(10);
}

void gveh_rotorcraft_thunder(gveh_rotorcraft_cfg *cfg)
{
    gveh_rotorcraft_default(cfg);
    cfg->flags = GVEH_ROTOR_FLAG_ENABLED | GVEH_ROTOR_FLAG_ARCADE;
    cfg->main_lift = gveh_fx_from_int(17000);
    cfg->hover_collective = GVEH_FX_ONE / 2;
    cfg->rotor_ramp = GVEH_FX_ONE / 4;
    cfg->translational_lift = gveh_fx_from_int(55);
    cfg->cyclic_force = gveh_fx_from_int(7200);
    cfg->pitch_power = gveh_fx_from_int(1400);
    cfg->roll_power = gveh_fx_from_int(1600);
    cfg->yaw_power = gveh_fx_from_int(1800);
    cfg->body_drag = GVEH_FX_ONE / 8;
    cfg->altitude_hold_k = gveh_fx_from_int(120);
    cfg->target_altitude = gveh_fx_from_int(9);
}

void gveh_rotorcraft_strike(gveh_rotorcraft_cfg *cfg)
{
    gveh_rotorcraft_thunder(cfg);
    cfg->flags = GVEH_ROTOR_FLAG_ENABLED | GVEH_ROTOR_FLAG_STRIKE | GVEH_ROTOR_FLAG_ALT_HOLD;
    cfg->main_lift = gveh_fx_from_int(21000);
    cfg->cyclic_force = gveh_fx_from_int(5200);
    cfg->yaw_power = gveh_fx_from_int(1200);
    cfg->body_drag = GVEH_FX_ONE / 5;
    cfg->target_altitude = gveh_fx_from_int(12);
}

void gveh_rotorcraft_sim(gveh_rotorcraft_cfg *cfg)
{
    gveh_rotorcraft_default(cfg);
    cfg->flags = GVEH_ROTOR_FLAG_ENABLED | GVEH_ROTOR_FLAG_ALT_HOLD;
    cfg->main_lift = gveh_fx_from_int(18500);
    cfg->hover_collective = GVEH_FX_ONE / 2;
    cfg->rotor_ramp = GVEH_FX_ONE / 12;
    cfg->translational_lift = gveh_fx_from_int(35);
    cfg->cyclic_force = gveh_fx_from_int(3800);
    cfg->pitch_power = gveh_fx_from_int(900);
    cfg->roll_power = gveh_fx_from_int(1100);
    cfg->yaw_power = gveh_fx_from_int(720);
    cfg->body_drag = GVEH_FX_ONE / 4;
    cfg->altitude_hold_k = gveh_fx_from_int(70);
    cfg->target_altitude = gveh_fx_from_int(18);
}

void gveh_rotorcraft_clear(gveh_rotorcraft_state *st)
{
    st->rotor_rpm = 0;
    st->lift = 0;
    st->forward_flow = 0;
    st->altitude_error = 0;
    st->low_rotor = 1;
}

void gveh_rotorcraft_apply(const gveh_rotorcraft_cfg *cfg, gveh_rotorcraft_state *st, gveh_body *body, gveh_basis basis, const gveh_input *in, gveh_fx dt)
{
    gveh_fx collective;
    gveh_fx target_rpm;
    gveh_fx lift;
    gveh_fx fwd_flow;
    gveh_fx alt_force;
    gveh_fx pitch_in;
    gveh_fx roll_in;
    gveh_fx yaw_in;
    gveh_vec3 planar;
    gveh_vec3 force;

    if ((cfg->flags & GVEH_ROTOR_FLAG_ENABLED) == 0u) return;
    collective = in->collective;
    if (collective == 0) collective = cfg->hover_collective + in->throttle / 3 - in->brake / 3;
    collective = gveh_fx_clamp(collective, 0, GVEH_FX_ONE);
    target_rpm = collective;
    st->rotor_rpm = gveh_fx_lerp(target_rpm, st->rotor_rpm, cfg->rotor_ramp);
    st->low_rotor = (st->rotor_rpm < GVEH_FX_ONE / 3) ? 1 : 0;

    fwd_flow = gveh_fx_abs(gveh_v3_dot(body->vel, basis.fwd));
    st->forward_flow = fwd_flow;
    lift = gveh_fx_mul(gveh_fx_mul(cfg->main_lift, collective), st->rotor_rpm);
    lift += gveh_fx_mul(fwd_flow, cfg->translational_lift);
    if (in->alt_hold > 0 || (cfg->flags & GVEH_ROTOR_FLAG_ALT_HOLD) != 0u) {
        st->altitude_error = cfg->target_altitude - body->pos.y;
        alt_force = gveh_fx_mul(st->altitude_error, cfg->altitude_hold_k);
        lift += gveh_fx_mul(alt_force, in->alt_hold > 0 ? in->alt_hold : GVEH_FX_ONE / 3);
    }
    st->lift = lift;
    gveh_body_add_force(body, gveh_v3(0, lift, 0));

    pitch_in = in->air_pitch;
    roll_in = in->air_roll;
    yaw_in = in->air_yaw;
    if (pitch_in == 0) pitch_in = in->throttle / 2;
    if (roll_in == 0) roll_in = in->steer;
    if (yaw_in == 0) yaw_in = in->steer / 2;
    force = gveh_v3_add(gveh_v3_scale(basis.fwd, gveh_fx_mul(pitch_in, cfg->cyclic_force)), gveh_v3_scale(basis.right, gveh_fx_mul(roll_in, cfg->cyclic_force)));
    gveh_body_add_force(body, force);
    body->torque.x += gveh_fx_mul(pitch_in, cfg->pitch_power);
    body->torque.z += gveh_fx_mul(-roll_in, cfg->roll_power);
    body->torque.y += gveh_fx_mul(yaw_in, cfg->yaw_power);

    planar = body->vel;
    planar.y = 0;
    gveh_body_add_force(body, gveh_v3_scale(planar, -cfg->body_drag));
    (void)dt;
}
