#include "gveh_ai.h"

void gveh_input_clear(gveh_input *i)
{
    i->throttle = 0;
    i->brake = 0;
    i->steer = 0;
    i->handbrake = 0;
    i->nitro = 0;
    i->risk = 0;
    i->aggression = 0;
    i->crashbreak = 0;
    i->aftertouch_x = 0;
    i->aftertouch_z = 0;
    i->collective = 0;
    i->air_pitch = 0;
    i->air_roll = 0;
    i->air_yaw = 0;
    i->fire = 0;
    i->lock_on = 0;
    i->winch = 0;
    i->alt_hold = 0;
    i->tank_turret = 0;
    i->tank_gun = 0;
    i->tank_range = 0;
    i->tank_ammo_next = 0;
    i->tank_station_next = 0;
    i->tank_hatch = 0;
    i->tank_smoke = 0;
    i->tank_order = 0;
    i->tank_face = 0;
}

void gveh_ai_simple_drive(const gveh_ai_driver *ai, gveh_vec3 pos, gveh_basis basis, gveh_fx speed, gveh_input *out_input)
{
    gveh_vec3 to;
    gveh_fx f;
    gveh_fx r;

    to = gveh_v3_sub(ai->target, pos);
    f = gveh_v3_dot(to, basis.fwd);
    r = gveh_v3_dot(to, basis.right);
    if (f > 0) out_input->throttle = GVEH_FX_ONE;
    else out_input->brake = GVEH_FX_ONE / 2;
    if (speed > ai->target_speed) {
        out_input->throttle = GVEH_FX_ONE / 4;
        out_input->brake = GVEH_FX_ONE / 4;
    }
    out_input->steer = gveh_fx_clamp(r / 12, -GVEH_FX_ONE, GVEH_FX_ONE);
}
