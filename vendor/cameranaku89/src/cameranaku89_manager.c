/* cameranaku89_manager.c - CC0 1.0 Universal. */
#include "cameranaku89_manager.h"

static cnk_fx cnk_mgr_ease(cnk_fx t)
{
    cnk_fx tt;
    cnk_fx three_minus_2t;
    t = cnk_fx_clamp(t, 0, CNK_ONE);
    tt = cnk_fx_mul(t, t);
    three_minus_2t = CNK_FX_FROM_INT(3) - cnk_fx_mul(CNK_FX_FROM_INT(2), t);
    return cnk_fx_mul(tt, three_minus_2t);
}

static cnk_state cnk_mgr_state_lerp(cnk_state a, cnk_state b, cnk_fx t, const cnk_transform_provider *provider)
{
    cnk_state out;
    out.pose.pos = cnk_vec3_lerp(a.pose.pos, b.pose.pos, t);
    out.pose.yaw_deg = cnk_fx_lerp(a.pose.yaw_deg, b.pose.yaw_deg, t);
    out.pose.pitch_deg = cnk_fx_lerp(a.pose.pitch_deg, b.pose.pitch_deg, t);
    out.pose.roll_deg = cnk_fx_lerp(a.pose.roll_deg, b.pose.roll_deg, t);
    out.lens = a.lens;
    out.lens.fov_y_deg = cnk_fx_lerp(a.lens.fov_y_deg, b.lens.fov_y_deg, t);
    out.lens.ortho_height = cnk_fx_lerp(a.lens.ortho_height, b.lens.ortho_height, t);
    out.lens.frustum_offset_x = cnk_fx_lerp(a.lens.frustum_offset_x, b.lens.frustum_offset_x, t);
    out.lens.frustum_offset_y = cnk_fx_lerp(a.lens.frustum_offset_y, b.lens.frustum_offset_y, t);
    out.lens.viewport_offset_x = cnk_fx_lerp(a.lens.viewport_offset_x, b.lens.viewport_offset_x, t);
    out.lens.viewport_offset_y = cnk_fx_lerp(a.lens.viewport_offset_y, b.lens.viewport_offset_y, t);
    out.look_at = cnk_vec3_lerp(a.look_at, b.look_at, t);
    out.basis = cnk_basis_from_angles_provider(provider, out.pose.yaw_deg, out.pose.pitch_deg, out.pose.roll_deg);
    out.valid = CNK_TRUE;
    return out;
}

static int cnk_mgr_raw_best_index(cnk_camera_manager *mgr)
{
    int i;
    int best;
    cnk_fx best_quality;
    best = -1;
    best_quality = -CNK_FX_FROM_INT(32000);
    if (mgr == 0) {
        return -1;
    }
    for (i = 0; i < CNK_MAX_MANAGER_VCAMS; ++i) {
        if (mgr->used[i] != 0 && mgr->vcams[i].state != CNK_VCAM_DISABLED) {
            if (best < 0 || mgr->vcams[i].shot.quality > best_quality) {
                best = i;
                best_quality = mgr->vcams[i].shot.quality;
            } else if (mgr->vcams[i].shot.quality == best_quality && mgr->vcams[i].priority > mgr->vcams[best].priority) {
                best = i;
                best_quality = mgr->vcams[i].shot.quality;
            }
        }
    }
    return best;
}

CNK_API void cnk_camera_manager_init(cnk_camera_manager *mgr)
{
    int i;
    if (mgr == 0) {
        return;
    }
    for (i = 0; i < CNK_MAX_MANAGER_VCAMS; ++i) {
        mgr->used[i] = 0;
        mgr->live_ticks[i] = 0;
        cnk_virtual_camera_init(&mgr->vcams[i], i);
        mgr->vcams[i].state = CNK_VCAM_DISABLED;
    }
    mgr->active_index = -1;
    mgr->previous_index = -1;
    mgr->default_blend_ticks = 12;
    mgr->blend_curve = CNK_BLEND_EASE_IN_OUT;
    mgr->blend_active = 0;
    mgr->blend_time_ticks = 0;
    mgr->blend_duration_ticks = 0;
    mgr->forced_vcam_id = -1;
    mgr->force_ticks = 0;
    mgr->switch_quality_margin = CNK_FX_FROM_INT(80);
    cnk_camera_reset(&mgr->vcams[0].camera);
    mgr->final_state = mgr->vcams[0].camera.state;
    mgr->blend_from = mgr->final_state;
    mgr->blend_to = mgr->final_state;
    mgr->frame_counter = 0UL;
    cnk_transform_provider_clear(&mgr->transform_provider);
}

CNK_API cnk_virtual_camera *cnk_camera_manager_alloc(cnk_camera_manager *mgr, int id)
{
    int i;
    if (mgr == 0) {
        return 0;
    }
    for (i = 0; i < CNK_MAX_MANAGER_VCAMS; ++i) {
        if (mgr->used[i] == 0) {
            mgr->used[i] = 1;
            mgr->live_ticks[i] = 0;
            cnk_virtual_camera_init(&mgr->vcams[i], id);
            if (cnk_transform_provider_is_active(&mgr->transform_provider) != 0) {
                cnk_virtual_camera_set_transform_provider(&mgr->vcams[i], &mgr->transform_provider);
            }
            mgr->vcams[i].state = CNK_VCAM_STANDBY;
            return &mgr->vcams[i];
        }
    }
    return 0;
}

CNK_API cnk_virtual_camera *cnk_camera_manager_get(cnk_camera_manager *mgr, int index)
{
    if (mgr == 0 || index < 0 || index >= CNK_MAX_MANAGER_VCAMS || mgr->used[index] == 0) {
        return 0;
    }
    return &mgr->vcams[index];
}

CNK_API int cnk_camera_manager_index_by_id(cnk_camera_manager *mgr, int id)
{
    int i;
    if (mgr == 0) {
        return -1;
    }
    for (i = 0; i < CNK_MAX_MANAGER_VCAMS; ++i) {
        if (mgr->used[i] != 0 && mgr->vcams[i].id == id) {
            return i;
        }
    }
    return -1;
}

CNK_API cnk_virtual_camera *cnk_camera_manager_find_by_id(cnk_camera_manager *mgr, int id)
{
    int index;
    index = cnk_camera_manager_index_by_id(mgr, id);
    if (index < 0) {
        return 0;
    }
    return &mgr->vcams[index];
}

CNK_API int cnk_camera_manager_best_index(cnk_camera_manager *mgr)
{
    int forced;
    int best;
    int active;
    if (mgr == 0) {
        return -1;
    }
    if (mgr->forced_vcam_id >= 0) {
        forced = cnk_camera_manager_index_by_id(mgr, mgr->forced_vcam_id);
        if (forced >= 0 && mgr->vcams[forced].state != CNK_VCAM_DISABLED) {
            return forced;
        }
    }
    best = cnk_mgr_raw_best_index(mgr);
    active = mgr->active_index;
    if (best >= 0 && active >= 0 && active < CNK_MAX_MANAGER_VCAMS && mgr->used[active] != 0 && best != active) {
        if (mgr->vcams[active].state != CNK_VCAM_DISABLED) {
            if (mgr->live_ticks[active] < mgr->vcams[active].min_live_ticks) {
                return active;
            }
            if (mgr->vcams[best].shot.quality < mgr->vcams[active].shot.quality + mgr->switch_quality_margin) {
                return active;
            }
        }
    }
    return best;
}

CNK_API void cnk_camera_manager_set_switch_policy(cnk_camera_manager *mgr, int blend_ticks, int curve, cnk_fx quality_margin)
{
    if (mgr == 0) {
        return;
    }
    mgr->default_blend_ticks = blend_ticks < 0 ? 0 : blend_ticks;
    mgr->blend_curve = curve;
    mgr->switch_quality_margin = quality_margin < 0 ? 0 : quality_margin;
}

CNK_API int cnk_camera_manager_force_by_id(cnk_camera_manager *mgr, int id, int hold_ticks)
{
    int index;
    if (mgr == 0) {
        return CNK_FALSE;
    }
    index = cnk_camera_manager_index_by_id(mgr, id);
    if (index < 0) {
        return CNK_FALSE;
    }
    mgr->forced_vcam_id = id;
    mgr->force_ticks = hold_ticks;
    return CNK_TRUE;
}

CNK_API void cnk_camera_manager_clear_force(cnk_camera_manager *mgr)
{
    if (mgr == 0) {
        return;
    }
    mgr->forced_vcam_id = -1;
    mgr->force_ticks = 0;
}

CNK_API int cnk_camera_manager_set_vcam_state(cnk_camera_manager *mgr, int id, int state)
{
    cnk_virtual_camera *vcam;
    vcam = cnk_camera_manager_find_by_id(mgr, id);
    if (vcam == 0) {
        return CNK_FALSE;
    }
    vcam->state = state;
    return CNK_TRUE;
}


CNK_API void cnk_camera_manager_clear_priority_biases(cnk_camera_manager *mgr)
{
    int i;
    if (mgr == 0) {
        return;
    }
    for (i = 0; i < CNK_MAX_MANAGER_VCAMS; ++i) {
        if (mgr->used[i] != 0) {
            mgr->vcams[i].priority_bias = 0;
        }
    }
}

CNK_API void cnk_camera_manager_set_target_all(cnk_camera_manager *mgr, cnk_target target)
{
    int i;
    if (mgr == 0) {
        return;
    }
    for (i = 0; i < CNK_MAX_MANAGER_VCAMS; ++i) {
        if (mgr->used[i] != 0) {
            cnk_virtual_camera_set_target(&mgr->vcams[i], target);
        }
    }
}

CNK_API void cnk_camera_manager_set_transform_provider_all(cnk_camera_manager *mgr, const cnk_transform_provider *provider)
{
    int i;
    if (mgr == 0) {
        return;
    }
    if (provider == 0) {
        cnk_transform_provider_clear(&mgr->transform_provider);
    } else {
        mgr->transform_provider = *provider;
    }
    for (i = 0; i < CNK_MAX_MANAGER_VCAMS; ++i) {
        if (mgr->used[i] != 0) {
            cnk_virtual_camera_set_transform_provider(&mgr->vcams[i], provider);
        }
    }
}

CNK_API void cnk_camera_manager_update(cnk_camera_manager *mgr, int dt_ticks, cnk_world_probe_fn probe, void *probe_user)
{
    int i;
    int best;
    cnk_fx t;
    if (mgr == 0) {
        return;
    }
    if (dt_ticks <= 0) {
        dt_ticks = 1;
    }
    if (mgr->forced_vcam_id >= 0 && mgr->force_ticks > 0) {
        mgr->force_ticks -= dt_ticks;
        if (mgr->force_ticks <= 0) {
            cnk_camera_manager_clear_force(mgr);
        }
    }
    for (i = 0; i < CNK_MAX_MANAGER_VCAMS; ++i) {
        if (mgr->used[i] != 0 && mgr->vcams[i].state != CNK_VCAM_DISABLED) {
            cnk_virtual_camera_update(&mgr->vcams[i], dt_ticks, probe, probe_user);
        }
    }
    best = cnk_camera_manager_best_index(mgr);
    if (best < 0) {
        mgr->frame_counter += 1UL;
        return;
    }
    if (mgr->active_index != best) {
        mgr->previous_index = mgr->active_index;
        mgr->active_index = best;
        mgr->live_ticks[best] = 0;
        mgr->blend_from = mgr->final_state;
        mgr->blend_to = mgr->vcams[best].camera.state;
        mgr->blend_time_ticks = 0;
        mgr->blend_duration_ticks = mgr->default_blend_ticks;
        mgr->blend_active = mgr->blend_duration_ticks > 0 ? 1 : 0;
        if (mgr->previous_index >= 0 && mgr->previous_index < CNK_MAX_MANAGER_VCAMS) {
            mgr->vcams[mgr->previous_index].state = CNK_VCAM_STANDBY;
        }
        mgr->vcams[best].state = CNK_VCAM_LIVE;
    } else {
        mgr->blend_to = mgr->vcams[best].camera.state;
        mgr->live_ticks[best] += dt_ticks;
    }
    if (mgr->blend_active != 0) {
        mgr->blend_time_ticks += dt_ticks;
        if (mgr->blend_duration_ticks <= 0) {
            t = CNK_ONE;
        } else {
            t = CNK_FX_FRAC(mgr->blend_time_ticks, mgr->blend_duration_ticks);
        }
        t = cnk_fx_clamp(t, 0, CNK_ONE);
        if (mgr->blend_curve == CNK_BLEND_EASE_IN_OUT) {
            t = cnk_mgr_ease(t);
        }
        mgr->final_state = cnk_mgr_state_lerp(mgr->blend_from, mgr->blend_to, t, &mgr->transform_provider);
        if (mgr->blend_time_ticks >= mgr->blend_duration_ticks) {
            mgr->blend_active = 0;
        }
    } else {
        mgr->final_state = mgr->vcams[best].camera.state;
    }
    mgr->frame_counter += 1UL;
}

CNK_API cnk_state cnk_camera_manager_final_state(const cnk_camera_manager *mgr)
{
    cnk_state st;
    cnk_camera tmp;
    if (mgr == 0) {
        cnk_camera_reset(&tmp);
        return tmp.state;
    }
    st = mgr->final_state;
    return st;
}

CNK_API void cnk_camera_manager_emit_to_camera(const cnk_camera_manager *mgr, cnk_camera *cam)
{
    if (mgr == 0 || cam == 0) {
        return;
    }
    cam->previous_state = cam->state;
    cam->state = mgr->final_state;
    cam->profile.lens = mgr->final_state.lens;
    if (cnk_transform_provider_is_active(&mgr->transform_provider) != 0) {
        cnk_camera_set_transform_provider(cam, &mgr->transform_provider);
    }
}
