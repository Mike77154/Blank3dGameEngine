#include "gveh_airframe.h"

void gveh_airframe_default(gveh_airframe_cfg *cfg)
{
    cfg->flags = 0;
    cfg->thrust = 0;
    cfg->lift_k = 0;
    cfg->drag_k = 0;
    cfg->side_drag_k = 0;
    cfg->pitch_power = 0;
    cfg->roll_power = 0;
    cfg->yaw_power = 0;
    cfg->min_fly_speed = 0;
    cfg->stall_speed = 0;
    cfg->arcade_lift = 0;
    cfg->afterburner_thrust = 0;
    cfg->g_limit = 0;
}

void gveh_airframe_ace(gveh_airframe_cfg *cfg)
{
    gveh_airframe_default(cfg);
    cfg->flags = GVEH_AIRFRAME_FLAG_ENABLED | GVEH_AIRFRAME_FLAG_ARCADE | GVEH_AIRFRAME_FLAG_AFTERBURNER;
    cfg->thrust = gveh_fx_from_int(9500);
    cfg->lift_k = gveh_fx_from_int(5);
    cfg->drag_k = GVEH_FX_ONE / 18;
    cfg->side_drag_k = GVEH_FX_ONE / 5;
    cfg->pitch_power = gveh_fx_from_int(2200);
    cfg->roll_power = gveh_fx_from_int(2600);
    cfg->yaw_power = gveh_fx_from_int(900);
    cfg->min_fly_speed = gveh_fx_from_int(20);
    cfg->stall_speed = gveh_fx_from_int(14);
    cfg->arcade_lift = gveh_fx_from_int(1200);
    cfg->afterburner_thrust = gveh_fx_from_int(4200);
    cfg->g_limit = gveh_fx_from_int(9);
}

void gveh_airframe_lightplane(gveh_airframe_cfg *cfg)
{
    gveh_airframe_default(cfg);
    cfg->flags = GVEH_AIRFRAME_FLAG_ENABLED | GVEH_AIRFRAME_FLAG_STALL;
    cfg->thrust = gveh_fx_from_int(2400);
    cfg->lift_k = gveh_fx_from_int(8);
    cfg->drag_k = GVEH_FX_ONE / 10;
    cfg->side_drag_k = GVEH_FX_ONE / 3;
    cfg->pitch_power = gveh_fx_from_int(650);
    cfg->roll_power = gveh_fx_from_int(900);
    cfg->yaw_power = gveh_fx_from_int(320);
    cfg->min_fly_speed = gveh_fx_from_int(12);
    cfg->stall_speed = gveh_fx_from_int(10);
    cfg->arcade_lift = gveh_fx_from_int(120);
    cfg->afterburner_thrust = 0;
    cfg->g_limit = gveh_fx_from_int(4);
}

void gveh_airframe_clear(gveh_airframe_state *st)
{
    st->airspeed = 0;
    st->aoa = 0;
    st->lift = 0;
    st->drag = 0;
    st->throttle_lag = 0;
    st->stalled = 0;
    st->overspeed = 0;
}

static gveh_fx gveh_air_power2(gveh_fx v)
{
    v = gveh_fx_abs(v);
    if (v > gveh_fx_from_int(220)) v = gveh_fx_from_int(220);
    return gveh_fx_mul(v, v);
}

void gveh_airframe_apply(const gveh_airframe_cfg *cfg, gveh_airframe_state *st, gveh_body *body, gveh_basis basis, const gveh_input *in, gveh_fx dt)
{
    gveh_fx fwd_speed;
    gveh_fx up_speed;
    gveh_fx side_speed;
    gveh_fx speed2;
    gveh_fx thrust;
    gveh_fx lift;
    gveh_fx drag;
    gveh_fx control_scale;
    gveh_fx pitch_in;
    gveh_fx roll_in;
    gveh_fx yaw_in;
    gveh_vec3 force;

    if ((cfg->flags & GVEH_AIRFRAME_FLAG_ENABLED) == 0u) return;
    fwd_speed = gveh_v3_dot(body->vel, basis.fwd);
    up_speed = gveh_v3_dot(body->vel, basis.up);
    side_speed = gveh_v3_dot(body->vel, basis.right);
    st->airspeed = gveh_v3_len_approx(body->vel);
    st->aoa = gveh_fx_div(-up_speed, gveh_fx_abs(fwd_speed) + GVEH_FX_ONE);
    st->stalled = 0;

    st->throttle_lag = gveh_fx_lerp(in->throttle, st->throttle_lag, GVEH_FX_ONE / 5);
    thrust = gveh_fx_mul(cfg->thrust, st->throttle_lag);
    if (in->nitro > 0 && (cfg->flags & GVEH_AIRFRAME_FLAG_AFTERBURNER) != 0u) {
        thrust += gveh_fx_mul(cfg->afterburner_thrust, in->nitro);
    }
    if (in->brake > 0) thrust -= gveh_fx_mul(cfg->thrust / 2, in->brake);
    gveh_body_add_force(body, gveh_v3_scale(basis.fwd, thrust));

    speed2 = gveh_air_power2(fwd_speed);
    lift = gveh_fx_mul(speed2 / 32, cfg->lift_k);
    lift += gveh_fx_mul(cfg->arcade_lift, in->throttle);
    if ((cfg->flags & GVEH_AIRFRAME_FLAG_STALL) != 0u && gveh_fx_abs(fwd_speed) < cfg->stall_speed) {
        lift /= 3;
        st->stalled = 1;
    }
    st->lift = lift;
    gveh_body_add_force(body, gveh_v3_scale(basis.up, lift));

    drag = gveh_fx_mul(st->airspeed, cfg->drag_k);
    st->drag = drag;
    force = gveh_v3_scale(body->vel, -drag);
    force = gveh_v3_add(force, gveh_v3_scale(basis.right, -gveh_fx_mul(side_speed, cfg->side_drag_k)));
    gveh_body_add_force(body, force);

    if (st->airspeed < cfg->min_fly_speed) control_scale = gveh_fx_div(st->airspeed, cfg->min_fly_speed + GVEH_FX_ONE);
    else control_scale = GVEH_FX_ONE;
    pitch_in = in->air_pitch;
    roll_in = in->air_roll;
    yaw_in = in->air_yaw;
    if (roll_in == 0) roll_in = in->steer;
    if (yaw_in == 0) yaw_in = in->steer / 4;
    body->torque.x += gveh_fx_mul(gveh_fx_mul(pitch_in, cfg->pitch_power), control_scale);
    body->torque.z += gveh_fx_mul(gveh_fx_mul(-roll_in, cfg->roll_power), control_scale);
    body->torque.y += gveh_fx_mul(gveh_fx_mul(yaw_in, cfg->yaw_power), control_scale);
    (void)dt;
}
