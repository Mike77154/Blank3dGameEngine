/* cameranaku89_virtual.c - CC0 1.0 Universal. */
#include "cameranaku89_virtual.h"

static cnk_fx cnk_vcam_len_approx(cnk_vec3 v)
{
    cnk_fx ax;
    cnk_fx ay;
    cnk_fx az;
    cnk_fx hi;
    ax = cnk_fx_abs(v.x);
    ay = cnk_fx_abs(v.y);
    az = cnk_fx_abs(v.z);
    hi = ax;
    if (ay > hi) { hi = ay; }
    if (az > hi) { hi = az; }
    return hi + ((ax + ay + az - hi) >> 2);
}

CNK_API void cnk_virtual_camera_init(cnk_virtual_camera *vcam, int id)
{
    if (vcam == 0) {
        return;
    }
    vcam->id = id;
    vcam->state = CNK_VCAM_STANDBY;
    vcam->priority = 0;
    vcam->priority_bias = 0;
    vcam->min_live_ticks = 0;
    vcam->active_ticks = 0;
    cnk_camera_reset(&vcam->camera);
    cnk_collision_default(&vcam->collision);
    cnk_spring_arm_default(&vcam->spring_arm);
    vcam->shot.quality = 0;
    vcam->shot.distance_error = 0;
    vcam->shot.center_error_x = 0;
    vcam->shot.center_error_y = 0;
    vcam->shot.target_visible = 0;
    vcam->shot.target_on_screen = 0;
    vcam->shot.obstructed = 0;
    cnk_composer_default(&vcam->composer);
    vcam->target_group = 0;
    vcam->use_spring_arm = 0;
    vcam->use_collision_ext = 1;
    vcam->use_composer = 1;
}

CNK_API void cnk_virtual_camera_set_profile(cnk_virtual_camera *vcam, const cnk_profile *profile)
{
    if (vcam == 0 || profile == 0) {
        return;
    }
    cnk_camera_apply_profile(&vcam->camera, profile);
    vcam->spring_arm.target_length = profile->distance;
    vcam->spring_arm.current_length = profile->distance;
    cnk_spring_arm_set_limits(&vcam->spring_arm, profile->min_distance, profile->max_distance);
    vcam->collision.probe_radius = profile->collision_radius;
}

CNK_API void cnk_virtual_camera_set_transform_provider(cnk_virtual_camera *vcam, const cnk_transform_provider *provider)
{
    if (vcam == 0) {
        return;
    }
    cnk_camera_set_transform_provider(&vcam->camera, provider);
}

CNK_API void cnk_virtual_camera_set_target(cnk_virtual_camera *vcam, cnk_target target)
{
    if (vcam == 0) {
        return;
    }
    vcam->target_group = 0;
    vcam->camera.target = target;
}

CNK_API void cnk_virtual_camera_set_target_group(cnk_virtual_camera *vcam, cnk_target_group *group)
{
    if (vcam == 0) {
        return;
    }
    vcam->target_group = group;
}

CNK_API void cnk_virtual_camera_set_priority(cnk_virtual_camera *vcam, int priority)
{
    if (vcam == 0) {
        return;
    }
    vcam->priority = priority;
}

CNK_API void cnk_virtual_camera_set_priority_bias(cnk_virtual_camera *vcam, int priority_bias)
{
    if (vcam == 0) {
        return;
    }
    vcam->priority_bias = priority_bias;
}

CNK_API void cnk_virtual_camera_enable_spring_arm(cnk_virtual_camera *vcam, int enabled)
{
    if (vcam == 0) {
        return;
    }
    vcam->use_spring_arm = enabled != 0 ? 1 : 0;
}

CNK_API void cnk_virtual_camera_enable_collision_ext(cnk_virtual_camera *vcam, int enabled)
{
    if (vcam == 0) {
        return;
    }
    vcam->use_collision_ext = enabled != 0 ? 1 : 0;
}

CNK_API void cnk_virtual_camera_enable_composer(cnk_virtual_camera *vcam, int enabled)
{
    if (vcam == 0) {
        return;
    }
    vcam->use_composer = enabled != 0 ? 1 : 0;
}

CNK_API void cnk_virtual_camera_set_composer(cnk_virtual_camera *vcam, const cnk_composer *composer)
{
    if (vcam == 0 || composer == 0) {
        return;
    }
    vcam->composer = *composer;
    vcam->use_composer = composer->enabled != 0 ? 1 : 0;
}

CNK_API cnk_shot_report cnk_virtual_camera_score(cnk_virtual_camera *vcam, cnk_world_probe_fn probe, void *probe_user)
{
    cnk_shot_report report;
    cnk_target target;
    cnk_fx depth;
    int sx;
    int sy;
    int cx;
    int cy;
    cnk_fx dist;
    cnk_fx desired_dist;
    report.quality = 0;
    report.distance_error = 0;
    report.center_error_x = 0;
    report.center_error_y = 0;
    report.target_visible = 0;
    report.target_on_screen = 0;
    report.obstructed = 0;
    if (vcam == 0 || vcam->state == CNK_VCAM_DISABLED) {
        return report;
    }
    target = vcam->camera.target;
    report.quality = CNK_FX_FROM_INT(1000 + ((vcam->priority + vcam->priority_bias) * 100));
    if (target.valid == 0) {
        report.quality -= CNK_FX_FROM_INT(600);
        return report;
    }
    if (cnk_camera_project_point(&vcam->camera, target.pos, &sx, &sy, &depth) != 0) {
        report.target_on_screen = 1;
        report.target_visible = 1;
        report.quality += CNK_FX_FROM_INT(400);
        cx = vcam->camera.state.lens.viewport_w / 2;
        cy = vcam->camera.state.lens.viewport_h / 2;
        report.center_error_x = CNK_FX_FROM_INT(sx - cx);
        report.center_error_y = CNK_FX_FROM_INT(sy - cy);
        report.quality -= cnk_fx_abs(report.center_error_x) >> 2;
        report.quality -= cnk_fx_abs(report.center_error_y) >> 2;
    } else {
        report.quality -= CNK_FX_FROM_INT(500);
    }
    dist = cnk_vcam_len_approx(cnk_vec3_sub(vcam->camera.state.pose.pos, target.pos));
    desired_dist = vcam->camera.profile.distance;
    report.distance_error = cnk_fx_abs(dist - desired_dist);
    report.quality -= report.distance_error;
    if (probe != 0 && cnk_collision_line_obstructed(&vcam->camera.state, vcam->collision.probe_radius, probe, probe_user) != 0) {
        report.obstructed = 1;
        report.target_visible = 0;
        report.quality -= CNK_FX_FROM_INT(700);
    }
    if (vcam->use_composer != 0) {
        cnk_composer_report crep;
        crep = cnk_composer_score_camera(&vcam->composer, &vcam->camera, target.pos, report.obstructed);
        report.quality += crep.quality_delta;
        report.center_error_x = crep.error_x;
        report.center_error_y = crep.error_y;
    }
    return report;
}

CNK_API void cnk_virtual_camera_update(cnk_virtual_camera *vcam, int dt_ticks, cnk_world_probe_fn probe, void *probe_user)
{
    cnk_target grouped;
    if (vcam == 0 || vcam->state == CNK_VCAM_DISABLED) {
        return;
    }
    if (dt_ticks <= 0) {
        dt_ticks = 1;
    }
    if (vcam->target_group != 0) {
        if (cnk_target_group_as_target(vcam->target_group, &grouped) != 0) {
            vcam->camera.target = grouped;
        }
    }
    cnk_camera_update(&vcam->camera, dt_ticks, probe, probe_user);
    if (vcam->use_spring_arm != 0 && (vcam->camera.profile.mode == CNK_MODE_FOLLOW || vcam->camera.profile.mode == CNK_MODE_ORBIT)) {
        cnk_spring_arm_apply_provider(&vcam->spring_arm, &vcam->camera.state, vcam->camera.state.look_at, probe, probe_user, cnk_camera_get_transform_provider(&vcam->camera));
    }
    if (vcam->use_collision_ext != 0) {
        cnk_collision_resolve_provider(&vcam->camera.state, &vcam->collision, probe, probe_user, cnk_camera_get_transform_provider(&vcam->camera));
    }
    vcam->shot = cnk_virtual_camera_score(vcam, probe, probe_user);
    if (vcam->use_composer != 0 && vcam->camera.target.valid != 0) {
        cnk_composer_report crep;
        crep = cnk_composer_score_camera(&vcam->composer, &vcam->camera, vcam->camera.target.pos, vcam->shot.obstructed);
        cnk_composer_apply_viewport_offset(&vcam->composer, &vcam->camera.state, &crep);
    }
    vcam->active_ticks += dt_ticks;
}
