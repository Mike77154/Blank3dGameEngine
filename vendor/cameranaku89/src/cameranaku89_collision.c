/* cameranaku89_collision.c - CC0 1.0 Universal. */
#include "cameranaku89_collision.h"

static cnk_fx cnk_col_min(cnk_fx a, cnk_fx b)
{
    return a < b ? a : b;
}

static cnk_fx cnk_col_max(cnk_fx a, cnk_fx b)
{
    return a > b ? a : b;
}

static cnk_vec3 cnk_col_clamp_vec3(cnk_vec3 v, cnk_vec3 lo, cnk_vec3 hi)
{
    cnk_vec3 out;
    out.x = cnk_fx_clamp(v.x, cnk_col_min(lo.x, hi.x), cnk_col_max(lo.x, hi.x));
    out.y = cnk_fx_clamp(v.y, cnk_col_min(lo.y, hi.y), cnk_col_max(lo.y, hi.y));
    out.z = cnk_fx_clamp(v.z, cnk_col_min(lo.z, hi.z), cnk_col_max(lo.z, hi.z));
    return out;
}

static cnk_vec3 cnk_col_apply_offset(cnk_basis b, cnk_vec3 origin, cnk_vec3 offset, const cnk_transform_provider *provider)
{
    cnk_vec3 out;
    out = origin;
    out = cnk_transform_move(provider, out, cnk_transform_scale_uniform(provider, b.right, offset.x));
    out = cnk_transform_move(provider, out, cnk_transform_scale_uniform(provider, b.up, offset.y));
    out = cnk_transform_move(provider, out, cnk_transform_scale_uniform(provider, b.forward, offset.z));
    return out;
}

static int cnk_col_probe_free(cnk_world_probe_fn probe, void *probe_user, cnk_vec3 from, cnk_vec3 to, cnk_fx radius)
{
    cnk_vec3 dummy;
    if (probe == 0) {
        return CNK_TRUE;
    }
    dummy = to;
    return probe(probe_user, from, to, radius, &dummy) == 0 ? CNK_TRUE : CNK_FALSE;
}

static int cnk_col_try_slide(cnk_state *state, const cnk_collision_settings *settings, cnk_world_probe_fn probe, void *probe_user, cnk_vec3 safe_pos, const cnk_transform_provider *provider)
{
    cnk_vec3 try_pos_a;
    cnk_vec3 try_pos_b;
    cnk_vec3 side;
    cnk_fx step;
    side = state->basis.right;
    step = settings->slide_step;
    if (step <= 0) {
        step = CNK_FX_FRAC(75, 100);
    }
    try_pos_a = cnk_transform_move(provider, safe_pos, cnk_transform_scale_uniform(provider, side, step));
    try_pos_b = cnk_transform_move(provider, safe_pos, cnk_transform_scale_uniform(provider, side, -step));
    if (cnk_col_probe_free(probe, probe_user, state->look_at, try_pos_a, settings->probe_radius + settings->skin) != 0) {
        state->pose.pos = try_pos_a;
        return CNK_TRUE;
    }
    if (cnk_col_probe_free(probe, probe_user, state->look_at, try_pos_b, settings->probe_radius + settings->skin) != 0) {
        state->pose.pos = try_pos_b;
        return CNK_TRUE;
    }
    return CNK_FALSE;
}

static int cnk_col_try_shoulder_swap(cnk_state *state, const cnk_collision_settings *settings, cnk_world_probe_fn probe, void *probe_user, cnk_vec3 safe_pos, const cnk_transform_provider *provider)
{
    cnk_vec3 side;
    cnk_vec3 try_pos;
    cnk_fx d;
    side = state->basis.right;
    d = settings->shoulder_swap_distance;
    if (d <= 0) {
        d = CNK_FX_FROM_INT(1);
    }
    try_pos = cnk_transform_move(provider, safe_pos, cnk_transform_scale_uniform(provider, side, -(d + d)));
    if (cnk_col_probe_free(probe, probe_user, state->look_at, try_pos, settings->probe_radius + settings->skin) != 0) {
        state->pose.pos = try_pos;
        return CNK_TRUE;
    }
    try_pos = cnk_transform_move(provider, safe_pos, cnk_transform_scale_uniform(provider, side, d + d));
    if (cnk_col_probe_free(probe, probe_user, state->look_at, try_pos, settings->probe_radius + settings->skin) != 0) {
        state->pose.pos = try_pos;
        return CNK_TRUE;
    }
    return CNK_FALSE;
}

static int cnk_col_try_backup(cnk_state *state, const cnk_collision_settings *settings, cnk_world_probe_fn probe, void *probe_user, const cnk_transform_provider *provider)
{
    cnk_vec3 try_pos;
    try_pos = cnk_col_apply_offset(state->basis, state->look_at, settings->backup_offset, provider);
    if (cnk_col_probe_free(probe, probe_user, state->look_at, try_pos, settings->probe_radius + settings->skin) != 0) {
        state->pose.pos = try_pos;
        return CNK_TRUE;
    }
    return CNK_FALSE;
}

CNK_API void cnk_collision_default(cnk_collision_settings *settings)
{
    if (settings == 0) {
        return;
    }
    settings->flags = CNK_COLLISION_FLAG_DECOLLIDER | CNK_COLLISION_FLAG_DEOCCLUDER;
    settings->strategy = CNK_COLLISION_PULL_FORWARD;
    settings->probe_radius = CNK_FX_FRAC(35, 100);
    settings->skin = CNK_FX_FRAC(5, 100);
    settings->slide_step = CNK_FX_FRAC(75, 100);
    settings->shoulder_swap_distance = CNK_FX_FROM_INT(1);
    settings->backup_offset = cnk_vec3_make(0, CNK_FX_FROM_INT(2), -CNK_FX_FROM_INT(4));
    settings->box.minv = cnk_vec3_make(-CNK_FX_FROM_INT(32767), -CNK_FX_FROM_INT(32767), -CNK_FX_FROM_INT(32767));
    settings->box.maxv = cnk_vec3_make(CNK_FX_FROM_INT(32767), CNK_FX_FROM_INT(32767), CNK_FX_FROM_INT(32767));
    settings->box.enabled = 0;
}

CNK_API void cnk_collision_set_strategy(cnk_collision_settings *settings, int strategy)
{
    if (settings == 0) {
        return;
    }
    settings->strategy = strategy;
}

CNK_API void cnk_collision_set_confiner_box(cnk_collision_settings *settings, cnk_vec3 minv, cnk_vec3 maxv)
{
    if (settings == 0) {
        return;
    }
    settings->box.minv = minv;
    settings->box.maxv = maxv;
    settings->box.enabled = 1;
    settings->flags |= CNK_COLLISION_FLAG_CONFINER_BOX;
}

CNK_API void cnk_collision_disable_confiner(cnk_collision_settings *settings)
{
    if (settings == 0) {
        return;
    }
    settings->box.enabled = 0;
    settings->flags &= ~CNK_COLLISION_FLAG_CONFINER_BOX;
}

CNK_API void cnk_collision_set_fallback(cnk_collision_settings *settings, cnk_vec3 backup_offset, cnk_fx shoulder_swap_distance, cnk_fx slide_step)
{
    if (settings == 0) {
        return;
    }
    settings->backup_offset = backup_offset;
    settings->shoulder_swap_distance = shoulder_swap_distance;
    settings->slide_step = slide_step;
}

CNK_API int cnk_collision_line_obstructed(const cnk_state *state, cnk_fx radius, cnk_world_probe_fn probe, void *probe_user)
{
    cnk_vec3 safe_pos;
    if (state == 0 || probe == 0) {
        return CNK_FALSE;
    }
    safe_pos = state->pose.pos;
    return probe(probe_user, state->look_at, state->pose.pos, radius, &safe_pos) != 0 ? CNK_TRUE : CNK_FALSE;
}

CNK_API int cnk_collision_resolve_provider(cnk_state *state, const cnk_collision_settings *settings, cnk_world_probe_fn probe, void *probe_user, const cnk_transform_provider *provider)
{
    cnk_vec3 safe_pos;
    cnk_vec3 old_pos;
    int changed;
    int solved;
    if (state == 0 || settings == 0) {
        return CNK_FALSE;
    }
    changed = CNK_FALSE;
    solved = CNK_FALSE;
    old_pos = state->pose.pos;
    if ((settings->flags & CNK_COLLISION_FLAG_CONFINER_BOX) != 0 && settings->box.enabled != 0) {
        state->pose.pos = cnk_col_clamp_vec3(state->pose.pos, settings->box.minv, settings->box.maxv);
        if (state->pose.pos.x != old_pos.x || state->pose.pos.y != old_pos.y || state->pose.pos.z != old_pos.z) {
            changed = CNK_TRUE;
        }
    }
    if (probe != 0 && ((settings->flags & CNK_COLLISION_FLAG_DECOLLIDER) != 0 || (settings->flags & CNK_COLLISION_FLAG_DEOCCLUDER) != 0)) {
        safe_pos = state->pose.pos;
        if (probe(probe_user, state->look_at, state->pose.pos, settings->probe_radius + settings->skin, &safe_pos) != 0) {
            if (settings->strategy == CNK_COLLISION_PRESERVE_HEIGHT) {
                safe_pos.y = state->pose.pos.y;
                state->pose.pos = safe_pos;
                solved = CNK_TRUE;
            } else if (settings->strategy == CNK_COLLISION_SLIDE) {
                solved = cnk_col_try_slide(state, settings, probe, probe_user, safe_pos, provider);
            } else if (settings->strategy == CNK_COLLISION_SHOULDER_SWAP) {
                solved = cnk_col_try_shoulder_swap(state, settings, probe, probe_user, safe_pos, provider);
            } else if (settings->strategy == CNK_COLLISION_CUT_TO_BACKUP) {
                solved = cnk_col_try_backup(state, settings, probe, probe_user, provider);
            }
            if (solved == CNK_FALSE) {
                state->pose.pos = safe_pos;
            }
            changed = CNK_TRUE;
        }
    }
    if (changed != 0) {
        state->pose = cnk_pose_look_at(state->pose.pos, state->look_at);
        state->basis = cnk_basis_from_angles_provider(provider, state->pose.yaw_deg, state->pose.pitch_deg, state->pose.roll_deg);
    }
    return changed;
}

CNK_API int cnk_collision_resolve(cnk_state *state, const cnk_collision_settings *settings, cnk_world_probe_fn probe, void *probe_user)
{
    return cnk_collision_resolve_provider(state, settings, probe, probe_user, 0);
}
