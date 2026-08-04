/* cameranaku89_spring_arm.c - CC0 1.0 Universal. */
#include "cameranaku89_spring_arm.h"

static cnk_fx cnk_sa_len_approx(cnk_vec3 v)
{
    cnk_fx ax;
    cnk_fx ay;
    cnk_fx az;
    cnk_fx hi;
    cnk_fx mid;
    cnk_fx lo;
    ax = cnk_fx_abs(v.x);
    ay = cnk_fx_abs(v.y);
    az = cnk_fx_abs(v.z);
    hi = ax;
    if (ay > hi) {
        hi = ay;
    }
    if (az > hi) {
        hi = az;
    }
    lo = ax;
    if (ay < lo) {
        lo = ay;
    }
    if (az < lo) {
        lo = az;
    }
    mid = ax + ay + az - hi - lo;
    return hi + (mid >> 1) + (lo >> 2);
}

CNK_API void cnk_spring_arm_default(cnk_spring_arm *arm)
{
    if (arm == 0) {
        return;
    }
    arm->target_length = CNK_FX_FROM_INT(8);
    arm->current_length = arm->target_length;
    arm->min_length = CNK_FX_FRAC(5, 10);
    arm->max_length = CNK_FX_FROM_INT(64);
    arm->probe_radius = CNK_FX_FRAC(35, 100);
    arm->return_lag = CNK_FX_FRAC(2, 10);
    arm->collision_lag = CNK_FX_FRAC(7, 10);
    arm->max_lag_distance = CNK_FX_FROM_INT(6);
    arm->collision_enabled = 1;
}

CNK_API void cnk_spring_arm_set_limits(cnk_spring_arm *arm, cnk_fx min_length, cnk_fx max_length)
{
    if (arm == 0) {
        return;
    }
    arm->min_length = min_length;
    arm->max_length = max_length;
    if (arm->max_length < arm->min_length) {
        arm->max_length = arm->min_length;
    }
    arm->target_length = cnk_fx_clamp(arm->target_length, arm->min_length, arm->max_length);
    arm->current_length = cnk_fx_clamp(arm->current_length, arm->min_length, arm->max_length);
}

CNK_API void cnk_spring_arm_set_lag(cnk_spring_arm *arm, cnk_fx return_lag, cnk_fx collision_lag)
{
    if (arm == 0) {
        return;
    }
    arm->return_lag = cnk_fx_clamp(return_lag, 0, CNK_ONE);
    arm->collision_lag = cnk_fx_clamp(collision_lag, 0, CNK_ONE);
}

CNK_API int cnk_spring_arm_apply_provider(cnk_spring_arm *arm, cnk_state *state, cnk_vec3 pivot, cnk_world_probe_fn probe, void *probe_user, const cnk_transform_provider *provider)
{
    cnk_vec3 desired_delta;
    cnk_vec3 dir;
    cnk_vec3 safe_pos;
    cnk_fx desired_length;
    cnk_fx safe_length;
    cnk_fx lag;
    int hit;
    if (arm == 0 || state == 0) {
        return CNK_FALSE;
    }
    desired_delta = cnk_transform_move(provider, state->pose.pos, cnk_vec3_make(-pivot.x, -pivot.y, -pivot.z));
    desired_length = cnk_sa_len_approx(desired_delta);
    if (desired_length <= 0) {
        return CNK_FALSE;
    }
    dir = cnk_vec3_normalize_fast(desired_delta);
    safe_length = desired_length;
    hit = CNK_FALSE;
    if (probe != 0 && arm->collision_enabled != 0) {
        safe_pos = state->pose.pos;
        if (probe(probe_user, pivot, state->pose.pos, arm->probe_radius, &safe_pos) != 0) {
            safe_length = cnk_sa_len_approx(cnk_vec3_sub(safe_pos, pivot));
            hit = CNK_TRUE;
        }
    }
    safe_length = cnk_fx_clamp(safe_length, arm->min_length, arm->max_length);
    arm->target_length = safe_length;
    lag = hit != 0 ? arm->collision_lag : arm->return_lag;
    arm->current_length = cnk_fx_lerp(arm->current_length, arm->target_length, lag);
    arm->current_length = cnk_fx_clamp(arm->current_length, arm->min_length, arm->max_length);
    state->pose.pos = cnk_transform_move(provider, pivot, cnk_transform_scale_uniform(provider, dir, arm->current_length));
    state->pose = cnk_pose_look_at(state->pose.pos, state->look_at);
    state->basis = cnk_basis_from_angles_provider(provider, state->pose.yaw_deg, state->pose.pitch_deg, state->pose.roll_deg);
    return hit;
}

CNK_API int cnk_spring_arm_apply(cnk_spring_arm *arm, cnk_state *state, cnk_vec3 pivot, cnk_world_probe_fn probe, void *probe_user)
{
    return cnk_spring_arm_apply_provider(arm, state, pivot, probe, probe_user, 0);
}
