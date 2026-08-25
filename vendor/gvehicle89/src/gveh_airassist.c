#include "gveh_airassist.h"

void gveh_airassist_default(gveh_airassist_cfg *cfg)
{
    cfg->flags = 0;
    cfg->autolevel_pitch = 0;
    cfg->autolevel_roll = 0;
    cfg->stall_push = 0;
    cfg->max_pitch_rate = 0;
    cfg->max_roll_rate = 0;
    cfg->max_yaw_rate = 0;
    cfg->max_bank256 = 0;
}

void gveh_airassist_arcade(gveh_airassist_cfg *cfg)
{
    cfg->flags = GVEH_AIRASSIST_FLAG_ENABLED | GVEH_AIRASSIST_FLAG_AUTOLEVEL | GVEH_AIRASSIST_FLAG_STALL_GUARD | GVEH_AIRASSIST_FLAG_LIMIT_BANK | GVEH_AIRASSIST_FLAG_ARCADE_CAM;
    cfg->autolevel_pitch = gveh_fx_from_int(3);
    cfg->autolevel_roll = gveh_fx_from_int(5);
    cfg->stall_push = gveh_fx_from_int(320);
    cfg->max_pitch_rate = gveh_fx_from_int(8);
    cfg->max_roll_rate = gveh_fx_from_int(11);
    cfg->max_yaw_rate = gveh_fx_from_int(8);
    cfg->max_bank256 = 34;
}

void gveh_airassist_sim(gveh_airassist_cfg *cfg)
{
    cfg->flags = GVEH_AIRASSIST_FLAG_ENABLED | GVEH_AIRASSIST_FLAG_STALL_GUARD;
    cfg->autolevel_pitch = GVEH_FX_ONE;
    cfg->autolevel_roll = GVEH_FX_ONE;
    cfg->stall_push = gveh_fx_from_int(160);
    cfg->max_pitch_rate = gveh_fx_from_int(6);
    cfg->max_roll_rate = gveh_fx_from_int(8);
    cfg->max_yaw_rate = gveh_fx_from_int(5);
    cfg->max_bank256 = 42;
}

void gveh_airassist_apply(const gveh_airassist_cfg *cfg, gveh_body *body, const gveh_airframe_state *air, const gveh_rotorcraft_state *rotor, const gveh_input *in)
{
    if ((cfg->flags & GVEH_AIRASSIST_FLAG_ENABLED) == 0u) return;
    if ((cfg->flags & GVEH_AIRASSIST_FLAG_AUTOLEVEL) != 0u) {
        if (in->air_pitch == 0) body->torque.x += (gveh_fx)(-body->pitch * cfg->autolevel_pitch);
        if (in->air_roll == 0 && in->steer == 0) body->torque.z += (gveh_fx)(-body->roll * cfg->autolevel_roll);
    }
    if ((cfg->flags & GVEH_AIRASSIST_FLAG_STALL_GUARD) != 0u && air->stalled != 0) {
        body->torque.x -= cfg->stall_push;
    }
    if (rotor->low_rotor != 0 && rotor->lift > 0) {
        body->torque.z += cfg->stall_push / 4;
    }
    if (cfg->max_pitch_rate > 0) body->pitch_vel = (gveh_i32)gveh_fx_clamp((gveh_fx)body->pitch_vel, -cfg->max_pitch_rate, cfg->max_pitch_rate);
    if (cfg->max_roll_rate > 0) body->roll_vel = (gveh_i32)gveh_fx_clamp((gveh_fx)body->roll_vel, -cfg->max_roll_rate, cfg->max_roll_rate);
    if (cfg->max_yaw_rate > 0) body->yaw_vel = (gveh_i32)gveh_fx_clamp((gveh_fx)body->yaw_vel, -cfg->max_yaw_rate, cfg->max_yaw_rate);
    if ((cfg->flags & GVEH_AIRASSIST_FLAG_LIMIT_BANK) != 0u && cfg->max_bank256 > 0) {
        if (body->roll > cfg->max_bank256 && body->roll < 128) body->roll = cfg->max_bank256;
        if (body->roll < (256 - cfg->max_bank256) && body->roll > 128) body->roll = 256 - cfg->max_bank256;
    }
}
