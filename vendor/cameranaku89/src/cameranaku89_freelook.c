/* cameranaku89_freelook.c - CC0 1.0 Universal. */
#include "cameranaku89_freelook.h"

static cnk_freelook_rig cnk_fl_lerp_rig(cnk_freelook_rig a, cnk_freelook_rig b, cnk_fx t)
{
    cnk_freelook_rig out;
    out.height = cnk_fx_lerp(a.height, b.height, t);
    out.radius = cnk_fx_lerp(a.radius, b.radius, t);
    out.fov_y_deg = cnk_fx_lerp(a.fov_y_deg, b.fov_y_deg, t);
    return out;
}

static cnk_freelook_rig cnk_fl_sample_rig(const cnk_freelook *fl)
{
    cnk_fx y;
    if (fl == 0) {
        cnk_freelook_rig r;
        r.height = 0;
        r.radius = CNK_FX_FROM_INT(8);
        r.fov_y_deg = CNK_DEG(60);
        return r;
    }
    y = cnk_fx_clamp(fl->y_axis, 0, CNK_ONE);
    if (y < CNK_HALF) {
        return cnk_fl_lerp_rig(fl->bottom, fl->middle, y << 1);
    }
    return cnk_fl_lerp_rig(fl->middle, fl->top, (y - CNK_HALF) << 1);
}

CNK_API void cnk_freelook_default(cnk_freelook *fl)
{
    if (fl == 0) {
        return;
    }
    fl->bottom.height = CNK_FX_FRAC(1, 2);
    fl->bottom.radius = CNK_FX_FROM_INT(6);
    fl->bottom.fov_y_deg = CNK_DEG(65);
    fl->middle.height = CNK_FX_FROM_INT(2);
    fl->middle.radius = CNK_FX_FROM_INT(8);
    fl->middle.fov_y_deg = CNK_DEG(60);
    fl->top.height = CNK_FX_FROM_INT(5);
    fl->top.radius = CNK_FX_FROM_INT(10);
    fl->top.fov_y_deg = CNK_DEG(55);
    fl->x_axis_yaw_deg = 0;
    fl->y_axis = CNK_HALF;
    fl->pivot_offset = cnk_vec3_make(0, CNK_FX_FROM_INT(1), 0);
    fl->look_offset = cnk_vec3_make(0, CNK_FX_FROM_INT(1), 0);
}

CNK_API void cnk_freelook_set_axis(cnk_freelook *fl, cnk_fx yaw_deg, cnk_fx y_axis)
{
    if (fl == 0) {
        return;
    }
    fl->x_axis_yaw_deg = yaw_deg;
    fl->y_axis = cnk_fx_clamp(y_axis, 0, CNK_ONE);
}

CNK_API cnk_state cnk_freelook_solve_provider(const cnk_freelook *fl, const cnk_target *target, const cnk_lens *lens, const cnk_transform_provider *provider)
{
    cnk_state out;
    cnk_freelook_rig rig;
    cnk_basis basis;
    cnk_vec3 pivot;
    cnk_vec3 pos;
    cnk_pose pose;
    cnk_lens local_lens;
    out.valid = 0;
    if (fl == 0 || target == 0 || target->valid == 0 || lens == 0) {
        return out;
    }
    rig = cnk_fl_sample_rig(fl);
    basis = cnk_basis_from_angles_provider(provider, fl->x_axis_yaw_deg, 0, 0);
    pivot = cnk_transform_move(provider, target->pos, fl->pivot_offset);
    pivot = cnk_transform_move(provider, pivot, fl->look_offset);
    pos = cnk_transform_move(provider, pivot, cnk_transform_scale_uniform(provider, basis.forward, -rig.radius));
    pos = cnk_transform_move(provider, pos, cnk_transform_scale_uniform(provider, basis.up, rig.height));
    pose = cnk_pose_look_at(pos, pivot);
    local_lens = *lens;
    local_lens.fov_y_deg = rig.fov_y_deg;
    cnk_lens_validate(&local_lens);
    out.pose = pose;
    out.lens = local_lens;
    out.basis = cnk_basis_from_angles_provider(provider, pose.yaw_deg, pose.pitch_deg, pose.roll_deg);
    out.look_at = pivot;
    out.valid = 1;
    return out;
}

CNK_API cnk_state cnk_freelook_solve(const cnk_freelook *fl, const cnk_target *target, const cnk_lens *lens)
{
    return cnk_freelook_solve_provider(fl, target, lens, 0);
}

CNK_API void cnk_camera_apply_freelook(cnk_camera *cam, const cnk_freelook *fl)
{
    cnk_state solved;
    if (cam == 0 || fl == 0) {
        return;
    }
    solved = cnk_freelook_solve_provider(fl, &cam->target, &cam->profile.lens, cnk_camera_get_transform_provider(cam));
    if (solved.valid != 0) {
        cam->previous_state = cam->state;
        cam->state = solved;
    }
}
