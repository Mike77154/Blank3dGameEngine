/*
    cameranaku89.c
    CC0 1.0 Universal.
    C89 fixed-point camera solver. No malloc, no free, no float, no double.
*/
#include "cameranaku89.h"

static const int cnk_sin_0_90[91] = {
    0,4,9,13,18,22,27,31,36,40,44,49,53,58,62,66,
    71,75,79,83,88,92,96,100,104,108,112,116,120,124,128,132,
    136,139,143,147,150,154,158,161,165,168,171,175,178,181,184,187,
    190,193,196,199,202,204,207,210,212,215,217,219,222,224,226,228,
    230,232,234,236,238,240,241,243,245,246,248,249,250,251,252,253,
    254,254,255,255,256,256,256,256,256,256,256
};

static cnk_fx cnk_wrap_deg(cnk_fx deg)
{
    cnk_fx full;
    full = CNK_DEG(360);
    while (deg < 0) {
        deg += full;
    }
    while (deg >= full) {
        deg -= full;
    }
    return deg;
}

static cnk_fx cnk_ease_in_out(cnk_fx t)
{
    cnk_fx tt;
    cnk_fx three_minus_2t;
    t = cnk_fx_clamp(t, 0, CNK_ONE);
    tt = cnk_fx_mul(t, t);
    three_minus_2t = CNK_FX_FROM_INT(3) - cnk_fx_mul(CNK_FX_FROM_INT(2), t);
    return cnk_fx_mul(tt, three_minus_2t);
}

static cnk_fx cnk_approach(cnk_fx current, cnk_fx target, cnk_fx lag)
{
    cnk_fx t;
    t = cnk_fx_clamp(lag, 0, CNK_ONE);
    return cnk_fx_lerp(current, target, t);
}

static cnk_vec3 cnk_vec3_approach(cnk_vec3 current, cnk_vec3 target, cnk_fx lag)
{
    cnk_vec3 out;
    out.x = cnk_approach(current.x, target.x, lag);
    out.y = cnk_approach(current.y, target.y, lag);
    out.z = cnk_approach(current.z, target.z, lag);
    return out;
}

static cnk_u32 cnk_rng_next(cnk_u32 *seed)
{
    *seed = (*seed * 1664525UL) + 1013904223UL;
    return *seed;
}

static cnk_fx cnk_rng_signed_fx(cnk_u32 *seed)
{
    cnk_u32 r;
    int v;
    r = cnk_rng_next(seed);
    v = (int)((r >> 16) & 511UL);
    v -= 255;
    return (cnk_fx)v;
}

static cnk_fx cnk_inv_sqrt_rough(cnk_fx v)
{
    cnk_fx x;
    int i;
    if (v <= 0) {
        return CNK_ONE;
    }
    x = CNK_ONE;
    if (v > CNK_FX_FROM_INT(1)) {
        x = v;
    }
    for (i = 0; i < 5; ++i) {
        x = (x + cnk_fx_div(v, x)) >> 1;
        if (x <= 0) {
            x = CNK_ONE;
        }
    }
    return cnk_fx_div(CNK_ONE, x);
}

static const cnk_transform_provider *cnk_cam_provider(const cnk_camera *cam)
{
    if (cam == 0) {
        return 0;
    }
    if (cam->transform_provider.mode != CNK_TRANSFORM_MODE_RECEIVE_PROVIDER) {
        return 0;
    }
    return &cam->transform_provider;
}

static cnk_vec3 cnk_cam_move(const cnk_camera *cam, cnk_vec3 position, cnk_vec3 delta)
{
    return cnk_transform_move(cnk_cam_provider(cam), position, delta);
}

static cnk_vec3 cnk_cam_scale(const cnk_camera *cam, cnk_vec3 value, cnk_fx scale)
{
    return cnk_transform_scale_uniform(cnk_cam_provider(cam), value, scale);
}

static cnk_basis cnk_cam_basis(const cnk_camera *cam, cnk_fx yaw_deg, cnk_fx pitch_deg, cnk_fx roll_deg)
{
    return cnk_basis_from_angles_provider(cnk_cam_provider(cam), yaw_deg, pitch_deg, roll_deg);
}

static cnk_state cnk_state_lerp(cnk_state a, cnk_state b, cnk_fx t, const cnk_transform_provider *provider)
{
    cnk_state out;
    out.pose.pos = cnk_vec3_lerp(a.pose.pos, b.pose.pos, t);
    out.pose.yaw_deg = cnk_fx_lerp(a.pose.yaw_deg, b.pose.yaw_deg, t);
    out.pose.pitch_deg = cnk_fx_lerp(a.pose.pitch_deg, b.pose.pitch_deg, t);
    out.pose.roll_deg = cnk_fx_lerp(a.pose.roll_deg, b.pose.roll_deg, t);
    out.lens = a.lens;
    out.lens.fov_y_deg = cnk_fx_lerp(a.lens.fov_y_deg, b.lens.fov_y_deg, t);
    out.lens.ortho_height = cnk_fx_lerp(a.lens.ortho_height, b.lens.ortho_height, t);
    out.look_at = cnk_vec3_lerp(a.look_at, b.look_at, t);
    out.basis = cnk_basis_from_angles_provider(provider, out.pose.yaw_deg, out.pose.pitch_deg, out.pose.roll_deg);
    out.valid = CNK_TRUE;
    return out;
}

static cnk_vec3 cnk_basis_apply_offset(const cnk_camera *cam, cnk_basis basis, cnk_vec3 origin, cnk_vec3 offset)
{
    cnk_vec3 out;
    cnk_vec3 rx;
    cnk_vec3 uy;
    cnk_vec3 fz;
    rx = cnk_cam_scale(cam, basis.right, offset.x);
    uy = cnk_cam_scale(cam, basis.up, offset.y);
    fz = cnk_cam_scale(cam, basis.forward, offset.z);
    out = cnk_cam_move(cam, origin, rx);
    out = cnk_cam_move(cam, out, uy);
    out = cnk_cam_move(cam, out, fz);
    return out;
}

static cnk_vec3 cnk_target_pivot(const cnk_camera *cam)
{
    cnk_basis tb;
    cnk_vec3 pivot;
    tb = cnk_cam_basis(cam, cam->target.yaw_deg, cam->target.pitch_deg, cam->target.roll_deg);
    pivot = cnk_basis_apply_offset(cam, tb, cam->target.pos, cam->profile.pivot_offset);
    pivot = cnk_cam_move(cam, pivot, cam->profile.look_offset);
    return pivot;
}

static cnk_state cnk_solve_fps(cnk_camera *cam)
{
    cnk_state out;
    cnk_fx yaw;
    cnk_fx pitch;
    cnk_basis basis;
    yaw = cam->target.yaw_deg + cam->input_yaw_delta;
    pitch = cam->target.pitch_deg + cam->input_pitch_delta;
    if ((cam->profile.flags & CNK_FLAG_CLAMP_PITCH) != 0) {
        pitch = cnk_fx_clamp(pitch, cam->profile.min_pitch_deg, cam->profile.max_pitch_deg);
    }
    basis = cnk_cam_basis(cam, yaw, pitch, 0);
    out.pose.pos = cnk_basis_apply_offset(cam, basis, cam->target.pos, cam->profile.pivot_offset);
    out.pose.yaw_deg = yaw;
    out.pose.pitch_deg = pitch;
    out.pose.roll_deg = 0;
    out.lens = cam->profile.lens;
    out.lens.fov_y_deg += cam->input_fov_delta;
    cnk_lens_validate(&out.lens);
    out.basis = basis;
    out.look_at = cnk_cam_move(cam, out.pose.pos, basis.forward);
    out.valid = CNK_TRUE;
    return out;
}

static cnk_state cnk_solve_follow(cnk_camera *cam)
{
    cnk_state out;
    cnk_basis target_basis;
    cnk_vec3 pivot;
    cnk_vec3 desired;
    cnk_vec3 look;
    cnk_pose pose;
    cnk_fx yaw;
    cnk_fx dist;
    yaw = 0;
    if ((cam->profile.flags & CNK_FLAG_USE_TARGET_YAW) != 0) {
        yaw = cam->target.yaw_deg;
    }
    yaw += cam->input_yaw_delta;
    target_basis = cnk_cam_basis(cam, yaw, 0, 0);
    pivot = cnk_basis_apply_offset(cam, target_basis, cam->target.pos, cam->profile.pivot_offset);
    look = cnk_cam_move(cam, pivot, cam->profile.look_offset);
    dist = cam->profile.distance + cam->input_zoom_delta;
    dist = cnk_fx_clamp(dist, cam->profile.min_distance, cam->profile.max_distance);
    desired = pivot;
    desired = cnk_cam_move(cam, desired, cnk_cam_scale(cam, target_basis.forward, -dist));
    desired = cnk_cam_move(cam, desired, cnk_cam_scale(cam, target_basis.up, cam->profile.height));
    desired = cnk_cam_move(cam, desired, cnk_cam_scale(cam, target_basis.right, cam->profile.shoulder_x));
    desired = cnk_cam_move(cam, desired, cam->profile.camera_offset);
    pose = cnk_pose_look_at(desired, look);
    out.pose = pose;
    out.lens = cam->profile.lens;
    out.lens.fov_y_deg += cam->input_fov_delta;
    cnk_lens_validate(&out.lens);
    out.basis = cnk_cam_basis(cam, out.pose.yaw_deg, out.pose.pitch_deg, out.pose.roll_deg);
    out.look_at = look;
    out.valid = CNK_TRUE;
    return out;
}

static cnk_state cnk_solve_fixed(cnk_camera *cam)
{
    cnk_state out;
    cnk_pose pose;
    out = cam->state;
    out.lens = cam->profile.lens;
    out.lens.fov_y_deg += cam->input_fov_delta;
    cnk_lens_validate(&out.lens);
    out.look_at = cnk_target_pivot(cam);
    pose = cnk_pose_look_at(out.pose.pos, out.look_at);
    out.pose.yaw_deg = pose.yaw_deg;
    out.pose.pitch_deg = pose.pitch_deg;
    if ((cam->profile.flags & CNK_FLAG_LOCK_ROLL) != 0) {
        out.pose.roll_deg = 0;
    }
    out.basis = cnk_cam_basis(cam, out.pose.yaw_deg, out.pose.pitch_deg, out.pose.roll_deg);
    out.valid = CNK_TRUE;
    return out;
}

static cnk_state cnk_solve_orbit(cnk_camera *cam)
{
    cnk_state out;
    cnk_basis orbit_basis;
    cnk_vec3 pivot;
    cnk_vec3 desired;
    cnk_pose pose;
    cnk_fx yaw;
    cnk_fx pitch;
    cnk_fx dist;
    yaw = cam->state.pose.yaw_deg + cam->input_yaw_delta;
    pitch = cam->state.pose.pitch_deg + cam->input_pitch_delta;
    pitch = cnk_fx_clamp(pitch, -CNK_DEG(CNK_ORBIT_PITCH_LIMIT_DEG), CNK_DEG(CNK_ORBIT_PITCH_LIMIT_DEG));
    if ((cam->profile.flags & CNK_FLAG_CLAMP_PITCH) != 0) {
        pitch = cnk_fx_clamp(pitch, cam->profile.min_pitch_deg, cam->profile.max_pitch_deg);
    }
    dist = cam->profile.distance + cam->input_zoom_delta;
    dist = cnk_fx_clamp(dist, cam->profile.min_distance, cam->profile.max_distance);
    orbit_basis = cnk_cam_basis(cam, yaw, pitch, 0);
    pivot = cnk_target_pivot(cam);
    desired = cnk_cam_move(cam, pivot, cnk_cam_scale(cam, orbit_basis.forward, -dist));
    desired = cnk_cam_move(cam, desired, cam->profile.camera_offset);
    pose = cnk_pose_look_at(desired, pivot);
    out.pose = pose;
    out.lens = cam->profile.lens;
    out.lens.fov_y_deg += cam->input_fov_delta;
    cnk_lens_validate(&out.lens);
    out.basis = cnk_cam_basis(cam, out.pose.yaw_deg, out.pose.pitch_deg, out.pose.roll_deg);
    out.look_at = pivot;
    out.valid = CNK_TRUE;
    return out;
}

static cnk_state cnk_solve_track(cnk_camera *cam)
{
    cnk_state out;
    cnk_keyframe a;
    cnk_keyframe b;
    int i;
    int span;
    int local;
    cnk_fx t;
    cnk_pose look_pose;
    out = cam->state;
    if (cam->track.count <= 0) {
        return out;
    }
    if (cam->track.count == 1) {
        out.pose = cam->track.keys[0].pose;
        out.lens = cam->profile.lens;
        if (cam->track.keys[0].fov_y_deg > 0) {
            out.lens.fov_y_deg = cam->track.keys[0].fov_y_deg;
        }
        out.look_at = cam->track.keys[0].look_at;
        out.basis = cnk_cam_basis(cam, out.pose.yaw_deg, out.pose.pitch_deg, out.pose.roll_deg);
        out.valid = CNK_TRUE;
        return out;
    }
    i = 0;
    while (i < cam->track.count - 2 && cam->track.time_ticks >= cam->track.keys[i + 1].time_ticks) {
        ++i;
    }
    a = cam->track.keys[i];
    b = cam->track.keys[i + 1];
    span = b.time_ticks - a.time_ticks;
    if (span <= 0) {
        span = 1;
    }
    local = cam->track.time_ticks - a.time_ticks;
    if (local < 0) {
        local = 0;
    }
    if (local > span) {
        local = span;
    }
    t = CNK_FX_FRAC(local, span);
    t = cnk_ease_in_out(t);
    out.pose.pos = cnk_vec3_lerp(a.pose.pos, b.pose.pos, t);
    out.pose.yaw_deg = cnk_fx_lerp(a.pose.yaw_deg, b.pose.yaw_deg, t);
    out.pose.pitch_deg = cnk_fx_lerp(a.pose.pitch_deg, b.pose.pitch_deg, t);
    out.pose.roll_deg = cnk_fx_lerp(a.pose.roll_deg, b.pose.roll_deg, t);
    out.lens = cam->profile.lens;
    if (a.fov_y_deg > 0 && b.fov_y_deg > 0) {
        out.lens.fov_y_deg = cnk_fx_lerp(a.fov_y_deg, b.fov_y_deg, t);
    }
    out.look_at = cnk_vec3_lerp(a.look_at, b.look_at, t);
    if ((a.flags & CNK_KEY_LOOK_AT) != 0 || (b.flags & CNK_KEY_LOOK_AT) != 0) {
        look_pose = cnk_pose_look_at(out.pose.pos, out.look_at);
        out.pose.yaw_deg = look_pose.yaw_deg;
        out.pose.pitch_deg = look_pose.pitch_deg;
    }
    out.basis = cnk_cam_basis(cam, out.pose.yaw_deg, out.pose.pitch_deg, out.pose.roll_deg);
    out.valid = CNK_TRUE;
    return out;
}

static cnk_state cnk_solve_rail(cnk_camera *cam)
{
    cnk_state out;
    cnk_vec3 best;
    cnk_vec3 p;
    cnk_vec3 q;
    cnk_vec3 v;
    cnk_vec3 w;
    cnk_fx best_d;
    cnk_fx d;
    cnk_fx c1;
    cnk_fx c2;
    cnk_fx t;
    int i;
    int last;
    cnk_pose pose;
    out = cam->state;
    if (cam->rail.count <= 0) {
        return out;
    }
    if (cam->rail.count == 1) {
        out.pose.pos = cam->rail.points[0];
        out.look_at = cnk_target_pivot(cam);
        pose = cnk_pose_look_at(out.pose.pos, out.look_at);
        out.pose.yaw_deg = pose.yaw_deg;
        out.pose.pitch_deg = pose.pitch_deg;
        out.lens = cam->profile.lens;
        out.basis = cnk_cam_basis(cam, out.pose.yaw_deg, out.pose.pitch_deg, out.pose.roll_deg);
        out.valid = CNK_TRUE;
        return out;
    }
    best = cam->rail.points[0];
    best_d = CNK_FX_FROM_INT(2140000);
    last = cam->rail.closed ? cam->rail.count : cam->rail.count - 1;
    for (i = 0; i < last; ++i) {
        p = cam->rail.points[i];
        q = cam->rail.points[(i + 1) % cam->rail.count];
        v = cnk_vec3_sub(q, p);
        w = cnk_vec3_sub(cam->target.pos, p);
        c1 = cnk_vec3_dot(w, v);
        if (c1 <= 0) {
            t = 0;
        } else {
            c2 = cnk_vec3_dot(v, v);
            if (c2 <= c1) {
                t = CNK_ONE;
            } else {
                t = cnk_fx_div(c1, c2);
            }
        }
        q = cnk_vec3_lerp(p, q, t);
        d = cnk_vec3_dot(cnk_vec3_sub(cam->target.pos, q), cnk_vec3_sub(cam->target.pos, q));
        if (d < best_d) {
            best_d = d;
            best = q;
        }
    }
    out.pose.pos = best;
    out.look_at = cnk_target_pivot(cam);
    pose = cnk_pose_look_at(out.pose.pos, out.look_at);
    out.pose.yaw_deg = pose.yaw_deg;
    out.pose.pitch_deg = pose.pitch_deg;
    out.pose.roll_deg = 0;
    out.lens = cam->profile.lens;
    out.basis = cnk_cam_basis(cam, out.pose.yaw_deg, out.pose.pitch_deg, out.pose.roll_deg);
    out.valid = CNK_TRUE;
    return out;
}

static cnk_state cnk_solve_desired(cnk_camera *cam)
{
    if (cam->profile.mode == CNK_MODE_FPS) {
        return cnk_solve_fps(cam);
    }
    if (cam->profile.mode == CNK_MODE_FOLLOW) {
        return cnk_solve_follow(cam);
    }
    if (cam->profile.mode == CNK_MODE_FIXED) {
        return cnk_solve_fixed(cam);
    }
    if (cam->profile.mode == CNK_MODE_ORBIT) {
        return cnk_solve_orbit(cam);
    }
    if (cam->profile.mode == CNK_MODE_CUTSCENE) {
        return cnk_solve_track(cam);
    }
    if (cam->profile.mode == CNK_MODE_RAIL) {
        return cnk_solve_rail(cam);
    }
    return cam->state;
}

static void cnk_apply_collision(cnk_camera *cam, cnk_state *desired, cnk_world_probe_fn probe, void *probe_user)
{
    cnk_vec3 safe_pos;
    int changed;
    if (probe == 0) {
        return;
    }
    if ((cam->profile.flags & CNK_FLAG_KEEP_LINE_OF_SIGHT) == 0) {
        return;
    }
    safe_pos = desired->pose.pos;
    changed = probe(probe_user, desired->look_at, desired->pose.pos, cam->profile.collision_radius, &safe_pos);
    if (changed != 0) {
        desired->pose.pos = safe_pos;
        desired->pose = cnk_pose_look_at(desired->pose.pos, desired->look_at);
        desired->basis = cnk_cam_basis(cam, desired->pose.yaw_deg, desired->pose.pitch_deg, desired->pose.roll_deg);
    }
}

static void cnk_apply_shakes(cnk_camera *cam, cnk_state *st)
{
    int i;
    int sample;
    cnk_fx n;
    cnk_fx pos_amp;
    cnk_fx rot_amp;
    cnk_fx fov_amp;
    cnk_u32 seed;
    for (i = 0; i < CNK_MAX_SHAKES; ++i) {
        if (cam->shakes[i].active != 0) {
            if (cam->shakes[i].duration_ticks <= 0) {
                cam->shakes[i].active = 0;
            } else {
                sample = cam->shakes[i].time_ticks * cam->shakes[i].frequency_hz;
                seed = cam->shakes[i].seed + (cnk_u32)sample + ((cnk_u32)i * 97UL);
                pos_amp = cam->shakes[i].pos_amp;
                rot_amp = cam->shakes[i].rot_amp_deg;
                fov_amp = cam->shakes[i].fov_amp_deg;
                {
                    cnk_vec3 delta;
                    cnk_basis shake_basis;
                    cnk_fx nx;
                    cnk_fx ny;
                    cnk_fx nz;
                    nx = cnk_rng_signed_fx(&seed);
                    ny = cnk_rng_signed_fx(&seed);
                    nz = cnk_rng_signed_fx(&seed);
                    delta.x = cnk_fx_mul(pos_amp, nx);
                    delta.y = cnk_fx_mul(pos_amp, ny);
                    delta.z = cnk_fx_mul(pos_amp, nz);
                    if (cam->shakes[i].play_space == CNK_SHAKE_WORLD) {
                        st->pose.pos = cnk_cam_move(cam, st->pose.pos, delta);
                    } else {
                        if (cam->shakes[i].play_space == CNK_SHAKE_USER_ROTATED) {
                            shake_basis = cnk_cam_basis(cam, cam->shakes[i].user_yaw_deg, 0, 0);
                        } else {
                            shake_basis = st->basis;
                        }
                        st->pose.pos = cnk_cam_move(cam, st->pose.pos, cnk_cam_scale(cam, shake_basis.right, delta.x));
                        st->pose.pos = cnk_cam_move(cam, st->pose.pos, cnk_cam_scale(cam, shake_basis.up, delta.y));
                        st->pose.pos = cnk_cam_move(cam, st->pose.pos, cnk_cam_scale(cam, shake_basis.forward, delta.z));
                    }
                }
                n = cnk_rng_signed_fx(&seed);
                st->pose.yaw_deg += cnk_fx_mul(rot_amp, n);
                n = cnk_rng_signed_fx(&seed);
                st->pose.pitch_deg += cnk_fx_mul(rot_amp, n);
                n = cnk_rng_signed_fx(&seed);
                st->lens.fov_y_deg += cnk_fx_mul(fov_amp, n);
            }
        }
    }
    cnk_lens_validate(&st->lens);
    st->basis = cnk_cam_basis(cam, st->pose.yaw_deg, st->pose.pitch_deg, st->pose.roll_deg);
}

CNK_API cnk_fx cnk_fx_mul(cnk_fx a, cnk_fx b)
{
    return (a * b) >> CNK_FX_SHIFT;
}

CNK_API cnk_fx cnk_fx_div(cnk_fx a, cnk_fx b)
{
    if (b == 0) {
        if (a < 0) {
            return -CNK_FX_FROM_INT(32767);
        }
        return CNK_FX_FROM_INT(32767);
    }
    return (a << CNK_FX_SHIFT) / b;
}

CNK_API cnk_fx cnk_fx_abs(cnk_fx a)
{
    if (a < 0) {
        return -a;
    }
    return a;
}

CNK_API cnk_fx cnk_fx_clamp(cnk_fx v, cnk_fx lo, cnk_fx hi)
{
    if (v < lo) {
        return lo;
    }
    if (v > hi) {
        return hi;
    }
    return v;
}

CNK_API cnk_fx cnk_fx_lerp(cnk_fx a, cnk_fx b, cnk_fx t)
{
    return a + cnk_fx_mul(b - a, t);
}

CNK_API cnk_fx cnk_sin_deg(cnk_fx deg)
{
    int d;
    int q;
    int r;
    int value;
    deg = cnk_wrap_deg(deg);
    d = CNK_FX_TO_INT(deg + CNK_HALF);
    if (d >= 360) {
        d -= 360;
    }
    q = d / 90;
    r = d % 90;
    if (q == 0) {
        value = cnk_sin_0_90[r];
    } else if (q == 1) {
        value = cnk_sin_0_90[90 - r];
    } else if (q == 2) {
        value = -cnk_sin_0_90[r];
    } else {
        value = -cnk_sin_0_90[90 - r];
    }
    return (cnk_fx)value;
}

CNK_API cnk_fx cnk_cos_deg(cnk_fx deg)
{
    return cnk_sin_deg(deg + CNK_DEG(90));
}

CNK_API cnk_vec2 cnk_vec2_make(cnk_fx x, cnk_fx y)
{
    cnk_vec2 v;
    v.x = x;
    v.y = y;
    return v;
}

CNK_API cnk_vec3 cnk_vec3_make(cnk_fx x, cnk_fx y, cnk_fx z)
{
    cnk_vec3 v;
    v.x = x;
    v.y = y;
    v.z = z;
    return v;
}

CNK_API cnk_vec3 cnk_vec3_add(cnk_vec3 a, cnk_vec3 b)
{
    cnk_vec3 v;
    v.x = a.x + b.x;
    v.y = a.y + b.y;
    v.z = a.z + b.z;
    return v;
}

CNK_API cnk_vec3 cnk_vec3_sub(cnk_vec3 a, cnk_vec3 b)
{
    cnk_vec3 v;
    v.x = a.x - b.x;
    v.y = a.y - b.y;
    v.z = a.z - b.z;
    return v;
}

CNK_API cnk_vec3 cnk_vec3_scale(cnk_vec3 a, cnk_fx s)
{
    cnk_vec3 v;
    v.x = cnk_fx_mul(a.x, s);
    v.y = cnk_fx_mul(a.y, s);
    v.z = cnk_fx_mul(a.z, s);
    return v;
}

CNK_API cnk_vec3 cnk_vec3_lerp(cnk_vec3 a, cnk_vec3 b, cnk_fx t)
{
    cnk_vec3 v;
    v.x = cnk_fx_lerp(a.x, b.x, t);
    v.y = cnk_fx_lerp(a.y, b.y, t);
    v.z = cnk_fx_lerp(a.z, b.z, t);
    return v;
}

CNK_API cnk_fx cnk_vec3_dot(cnk_vec3 a, cnk_vec3 b)
{
    cnk_fx v;
    v = cnk_fx_mul(a.x, b.x);
    v += cnk_fx_mul(a.y, b.y);
    v += cnk_fx_mul(a.z, b.z);
    return v;
}

CNK_API cnk_vec3 cnk_vec3_cross(cnk_vec3 a, cnk_vec3 b)
{
    cnk_vec3 v;
    v.x = cnk_fx_mul(a.y, b.z) - cnk_fx_mul(a.z, b.y);
    v.y = cnk_fx_mul(a.z, b.x) - cnk_fx_mul(a.x, b.z);
    v.z = cnk_fx_mul(a.x, b.y) - cnk_fx_mul(a.y, b.x);
    return v;
}

CNK_API cnk_vec3 cnk_vec3_normalize_fast(cnk_vec3 a)
{
    cnk_fx len2;
    cnk_fx inv;
    len2 = cnk_vec3_dot(a, a);
    if (len2 <= 0) {
        return cnk_vec3_make(0, 0, CNK_ONE);
    }
    inv = cnk_inv_sqrt_rough(len2);
    return cnk_vec3_scale(a, inv);
}

CNK_API void cnk_transform_provider_clear(cnk_transform_provider *provider)
{
    if (provider == 0) {
        return;
    }
    provider->user = 0;
    provider->move = 0;
    provider->scale = 0;
    provider->rotate = 0;
    provider->mode = CNK_TRANSFORM_MODE_INTERNAL;
}

CNK_API void cnk_transform_provider_set(cnk_transform_provider *provider, void *user, cnk_transform_move_fn move_fn, cnk_transform_scale_fn scale_fn, cnk_transform_rotate_fn rotate_fn)
{
    if (provider == 0) {
        return;
    }
    provider->user = user;
    provider->move = move_fn;
    provider->scale = scale_fn;
    provider->rotate = rotate_fn;
    if (move_fn != 0 || scale_fn != 0 || rotate_fn != 0) {
        provider->mode = CNK_TRANSFORM_MODE_RECEIVE_PROVIDER;
    } else {
        provider->mode = CNK_TRANSFORM_MODE_INTERNAL;
    }
}

CNK_API void cnk_transform_provider_set_mode(cnk_transform_provider *provider, int mode)
{
    if (provider == 0) {
        return;
    }
    if (mode == CNK_TRANSFORM_MODE_RECEIVE_PROVIDER) {
        provider->mode = CNK_TRANSFORM_MODE_RECEIVE_PROVIDER;
    } else {
        provider->mode = CNK_TRANSFORM_MODE_INTERNAL;
    }
}

CNK_API int cnk_transform_provider_is_active(const cnk_transform_provider *provider)
{
    if (provider == 0) {
        return CNK_FALSE;
    }
    if (provider->mode != CNK_TRANSFORM_MODE_RECEIVE_PROVIDER) {
        return CNK_FALSE;
    }
    if (provider->move == 0 && provider->scale == 0 && provider->rotate == 0) {
        return CNK_FALSE;
    }
    return CNK_TRUE;
}

CNK_API cnk_vec3 cnk_transform_move(const cnk_transform_provider *provider, cnk_vec3 position, cnk_vec3 delta)
{
    cnk_vec3 out;
    if (cnk_transform_provider_is_active(provider) != 0 && provider->move != 0) {
        if (provider->move(provider->user, position, delta, &out) != 0) {
            return out;
        }
    }
    return cnk_vec3_add(position, delta);
}

CNK_API cnk_vec3 cnk_transform_scale(const cnk_transform_provider *provider, cnk_vec3 value, cnk_vec3 scale)
{
    cnk_vec3 out;
    if (cnk_transform_provider_is_active(provider) != 0 && provider->scale != 0) {
        if (provider->scale(provider->user, value, scale, &out) != 0) {
            return out;
        }
    }
    out.x = cnk_fx_mul(value.x, scale.x);
    out.y = cnk_fx_mul(value.y, scale.y);
    out.z = cnk_fx_mul(value.z, scale.z);
    return out;
}

CNK_API cnk_vec3 cnk_transform_scale_uniform(const cnk_transform_provider *provider, cnk_vec3 value, cnk_fx scale)
{
    return cnk_transform_scale(provider, value, cnk_vec3_make(scale, scale, scale));
}

CNK_API cnk_vec3 cnk_transform_rotate(const cnk_transform_provider *provider, cnk_vec3 value, cnk_vec3 euler_deg)
{
    cnk_vec3 out;
    cnk_basis basis;
    if (cnk_transform_provider_is_active(provider) != 0 && provider->rotate != 0) {
        if (provider->rotate(provider->user, value, euler_deg, &out) != 0) {
            return out;
        }
    }
    basis = cnk_basis_from_angles(euler_deg.x, euler_deg.y, euler_deg.z);
    out = cnk_vec3_scale(basis.right, value.x);
    out = cnk_vec3_add(out, cnk_vec3_scale(basis.up, value.y));
    out = cnk_vec3_add(out, cnk_vec3_scale(basis.forward, value.z));
    return out;
}

CNK_API cnk_vec3 cnk_transform_apply_trs(const cnk_transform_provider *provider, cnk_vec3 point, cnk_vec3 scale, cnk_vec3 euler_deg, cnk_vec3 translation)
{
    cnk_vec3 out;
    out = cnk_transform_scale(provider, point, scale);
    out = cnk_transform_rotate(provider, out, euler_deg);
    out = cnk_transform_move(provider, out, translation);
    return out;
}

CNK_API void cnk_lens_perspective(cnk_lens *lens, int w, int h, cnk_fx fov_y_deg, cnk_fx near_clip, cnk_fx far_clip)
{
    if (lens == 0) {
        return;
    }
    lens->projection = CNK_PROJ_PERSPECTIVE;
    lens->viewport_w = w;
    lens->viewport_h = h;
    lens->fov_y_deg = fov_y_deg;
    lens->near_clip = near_clip;
    lens->far_clip = far_clip;
    lens->ortho_height = CNK_FX_FROM_INT(10);
    lens->fov_axis = CNK_FOV_VERTICAL;
    lens->aspect_policy = CNK_KEEP_HEIGHT;
    lens->frustum_offset_x = 0;
    lens->frustum_offset_y = 0;
    lens->viewport_offset_x = 0;
    lens->viewport_offset_y = 0;
    lens->cull_mask = 0xffffffffUL;
    lens->effect_mask = 0;
    cnk_lens_validate(lens);
}

CNK_API void cnk_lens_orthographic(cnk_lens *lens, int w, int h, cnk_fx ortho_height, cnk_fx near_clip, cnk_fx far_clip)
{
    if (lens == 0) {
        return;
    }
    lens->projection = CNK_PROJ_ORTHOGRAPHIC;
    lens->viewport_w = w;
    lens->viewport_h = h;
    lens->fov_y_deg = CNK_DEG(75);
    lens->near_clip = near_clip;
    lens->far_clip = far_clip;
    lens->ortho_height = ortho_height;
    lens->fov_axis = CNK_FOV_VERTICAL;
    lens->aspect_policy = CNK_KEEP_HEIGHT;
    lens->frustum_offset_x = 0;
    lens->frustum_offset_y = 0;
    lens->viewport_offset_x = 0;
    lens->viewport_offset_y = 0;
    lens->cull_mask = 0xffffffffUL;
    lens->effect_mask = 0;
    cnk_lens_validate(lens);
}

CNK_API int cnk_lens_validate(cnk_lens *lens)
{
    int ok;
    ok = CNK_TRUE;
    if (lens == 0) {
        return CNK_FALSE;
    }
    if (lens->viewport_w <= 0) {
        lens->viewport_w = CNK_DEFAULT_VIEW_W;
        ok = CNK_FALSE;
    }
    if (lens->viewport_h <= 0) {
        lens->viewport_h = CNK_DEFAULT_VIEW_H;
        ok = CNK_FALSE;
    }
    lens->aspect = CNK_FX_FRAC(lens->viewport_w, lens->viewport_h);
    if (lens->projection != CNK_PROJ_PERSPECTIVE && lens->projection != CNK_PROJ_ORTHOGRAPHIC && lens->projection != CNK_PROJ_FRUSTUM) {
        lens->projection = CNK_PROJ_PERSPECTIVE;
        ok = CNK_FALSE;
    }
    if (lens->fov_y_deg < CNK_DEG(CNK_MIN_FOV_DEG)) {
        lens->fov_y_deg = CNK_DEG(CNK_MIN_FOV_DEG);
        ok = CNK_FALSE;
    }
    if (lens->fov_y_deg > CNK_DEG(CNK_MAX_FOV_DEG)) {
        lens->fov_y_deg = CNK_DEG(CNK_MAX_FOV_DEG);
        ok = CNK_FALSE;
    }
    if (lens->near_clip <= 0) {
        lens->near_clip = CNK_FX_FROM_INT(CNK_DEFAULT_NEAR_CLIP);
        ok = CNK_FALSE;
    }
    if (lens->far_clip <= lens->near_clip) {
        lens->far_clip = lens->near_clip + CNK_FX_FROM_INT(CNK_DEFAULT_FAR_CLIP);
        ok = CNK_FALSE;
    }
    if (lens->ortho_height <= 0) {
        lens->ortho_height = CNK_FX_FROM_INT(10);
        ok = CNK_FALSE;
    }
    if (lens->fov_axis != CNK_FOV_VERTICAL && lens->fov_axis != CNK_FOV_HORIZONTAL) {
        lens->fov_axis = CNK_FOV_VERTICAL;
        ok = CNK_FALSE;
    }
    if (lens->aspect_policy != CNK_KEEP_HEIGHT && lens->aspect_policy != CNK_KEEP_WIDTH) {
        lens->aspect_policy = CNK_KEEP_HEIGHT;
        ok = CNK_FALSE;
    }
    if (lens->cull_mask == 0) {
        lens->cull_mask = 0xffffffffUL;
    }
    return ok;
}

CNK_API void cnk_profile_default(cnk_profile *profile)
{
    if (profile == 0) {
        return;
    }
    profile->mode = CNK_MODE_FOLLOW;
    profile->style = CNK_STYLE_CUSTOM;
    cnk_lens_perspective(&profile->lens, CNK_DEFAULT_VIEW_W, CNK_DEFAULT_VIEW_H, CNK_DEG(60), CNK_FX_FROM_INT(CNK_DEFAULT_NEAR_CLIP), CNK_FX_FROM_INT(CNK_DEFAULT_FAR_CLIP));
    profile->pivot_offset = cnk_vec3_make(0, CNK_FX_FROM_INT(2), 0);
    profile->camera_offset = cnk_vec3_make(0, 0, 0);
    profile->look_offset = cnk_vec3_make(0, CNK_FX_FROM_INT(1), 0);
    profile->distance = CNK_FX_FROM_INT(8);
    profile->min_distance = CNK_FX_FROM_INT(2);
    profile->max_distance = CNK_FX_FROM_INT(64);
    profile->height = CNK_FX_FROM_INT(2);
    profile->shoulder_x = 0;
    profile->dead_zone_x = CNK_FX_FRAC(5, 100);
    profile->dead_zone_y = CNK_FX_FRAC(5, 100);
    profile->soft_zone_x = CNK_FX_FRAC(25, 100);
    profile->soft_zone_y = CNK_FX_FRAC(25, 100);
    profile->pos_lag = CNK_FX_FRAC(2, 10);
    profile->rot_lag = CNK_FX_FRAC(3, 10);
    profile->fov_lag = CNK_FX_FRAC(3, 10);
    profile->min_pitch_deg = -CNK_DEG(75);
    profile->max_pitch_deg = CNK_DEG(75);
    profile->collision_radius = CNK_FX_FRAC(35, 100);
    profile->flags = CNK_FLAG_LOCK_ROLL | CNK_FLAG_USE_TARGET_YAW | CNK_FLAG_KEEP_LINE_OF_SIGHT | CNK_FLAG_CLAMP_PITCH;
}

CNK_API int cnk_profile_validate(cnk_profile *profile)
{
    int ok;
    ok = CNK_TRUE;
    if (profile == 0) {
        return CNK_FALSE;
    }
    if (profile->mode < CNK_MODE_FREE || profile->mode > CNK_MODE_CUTSCENE) {
        profile->mode = CNK_MODE_FOLLOW;
        ok = CNK_FALSE;
    }
    if (profile->min_distance < 0) {
        profile->min_distance = 0;
        ok = CNK_FALSE;
    }
    if (profile->max_distance < profile->min_distance) {
        profile->max_distance = profile->min_distance;
        ok = CNK_FALSE;
    }
    profile->distance = cnk_fx_clamp(profile->distance, profile->min_distance, profile->max_distance);
    profile->pos_lag = cnk_fx_clamp(profile->pos_lag, 0, CNK_ONE);
    profile->rot_lag = cnk_fx_clamp(profile->rot_lag, 0, CNK_ONE);
    profile->fov_lag = cnk_fx_clamp(profile->fov_lag, 0, CNK_ONE);
    if (profile->min_pitch_deg > profile->max_pitch_deg) {
        profile->min_pitch_deg = -CNK_DEG(75);
        profile->max_pitch_deg = CNK_DEG(75);
        ok = CNK_FALSE;
    }
    if (profile->collision_radius < 0) {
        profile->collision_radius = 0;
        ok = CNK_FALSE;
    }
    if (cnk_lens_validate(&profile->lens) == CNK_FALSE) {
        ok = CNK_FALSE;
    }
    return ok;
}

CNK_API void cnk_profile_set_mode(cnk_profile *profile, int mode)
{
    if (profile == 0) {
        return;
    }
    profile->mode = mode;
    cnk_profile_validate(profile);
}

CNK_API void cnk_profile_set_style(cnk_profile *profile, int style)
{
    if (profile == 0) {
        return;
    }
    profile->style = style;
}

CNK_API void cnk_profile_set_lens(cnk_profile *profile, const cnk_lens *lens)
{
    if (profile == 0 || lens == 0) {
        return;
    }
    profile->lens = *lens;
    cnk_lens_validate(&profile->lens);
}

CNK_API void cnk_profile_set_follow_layout(cnk_profile *profile, cnk_vec3 pivot_offset, cnk_vec3 camera_offset, cnk_vec3 look_offset, cnk_fx distance, cnk_fx height, cnk_fx shoulder_x)
{
    if (profile == 0) {
        return;
    }
    profile->pivot_offset = pivot_offset;
    profile->camera_offset = camera_offset;
    profile->look_offset = look_offset;
    profile->distance = distance;
    profile->height = height;
    profile->shoulder_x = shoulder_x;
    cnk_profile_validate(profile);
}

CNK_API void cnk_profile_set_distance_limits(cnk_profile *profile, cnk_fx min_distance, cnk_fx max_distance)
{
    if (profile == 0) {
        return;
    }
    profile->min_distance = min_distance;
    profile->max_distance = max_distance;
    cnk_profile_validate(profile);
}

CNK_API void cnk_profile_set_lag(cnk_profile *profile, cnk_fx pos_lag, cnk_fx rot_lag, cnk_fx fov_lag)
{
    if (profile == 0) {
        return;
    }
    profile->pos_lag = pos_lag;
    profile->rot_lag = rot_lag;
    profile->fov_lag = fov_lag;
    cnk_profile_validate(profile);
}

CNK_API void cnk_profile_set_pitch_limits(cnk_profile *profile, cnk_fx min_pitch_deg, cnk_fx max_pitch_deg)
{
    if (profile == 0) {
        return;
    }
    profile->min_pitch_deg = min_pitch_deg;
    profile->max_pitch_deg = max_pitch_deg;
    cnk_profile_validate(profile);
}

CNK_API void cnk_profile_set_collision(cnk_profile *profile, cnk_fx radius, int keep_line_of_sight)
{
    if (profile == 0) {
        return;
    }
    profile->collision_radius = radius;
    if (keep_line_of_sight != 0) {
        profile->flags |= CNK_FLAG_KEEP_LINE_OF_SIGHT;
    } else {
        profile->flags &= ~CNK_FLAG_KEEP_LINE_OF_SIGHT;
    }
    cnk_profile_validate(profile);
}

CNK_API void cnk_camera_reset(cnk_camera *cam)
{
    cnk_profile p;
    int i;
    if (cam == 0) {
        return;
    }
    cnk_profile_default(&p);
    cam->profile = p;
    cam->target.pos = cnk_vec3_make(0, 0, 0);
    cam->target.velocity = cnk_vec3_make(0, 0, 0);
    cam->target.yaw_deg = 0;
    cam->target.pitch_deg = 0;
    cam->target.roll_deg = 0;
    cam->target.valid = CNK_FALSE;
    cam->state.pose.pos = cnk_vec3_make(0, CNK_FX_FROM_INT(2), -CNK_FX_FROM_INT(8));
    cam->state.pose.yaw_deg = 0;
    cam->state.pose.pitch_deg = 0;
    cam->state.pose.roll_deg = 0;
    cam->state.lens = p.lens;
    cam->state.look_at = cnk_vec3_make(0, CNK_FX_FROM_INT(2), 0);
    cam->state.basis = cnk_basis_from_angles(0, 0, 0);
    cam->state.valid = CNK_TRUE;
    cam->previous_state = cam->state;
    cnk_track_clear(&cam->track);
    cnk_rail_clear(&cam->rail);
    for (i = 0; i < CNK_MAX_SHAKES; ++i) {
        cam->shakes[i].active = 0;
        cam->shakes[i].time_ticks = 0;
        cam->shakes[i].duration_ticks = 0;
        cam->shakes[i].pos_amp = 0;
        cam->shakes[i].rot_amp_deg = 0;
        cam->shakes[i].fov_amp_deg = 0;
        cam->shakes[i].frequency_hz = 0;
        cam->shakes[i].seed = 0;
    }
    cam->blend_from = cam->state;
    cam->blend_to = cam->state;
    cam->blend_active = 0;
    cam->blend_time_ticks = 0;
    cam->blend_duration_ticks = 0;
    cam->blend_curve = CNK_BLEND_CUT;
    cam->input_yaw_delta = 0;
    cam->input_pitch_delta = 0;
    cam->input_zoom_delta = 0;
    cam->input_fov_delta = 0;
    cam->frame_counter = 0;
    cnk_transform_provider_clear(&cam->transform_provider);
}

CNK_API int cnk_camera_validate(cnk_camera *cam)
{
    int ok;
    if (cam == 0) {
        return CNK_FALSE;
    }
    ok = cnk_profile_validate(&cam->profile);
    if (cnk_lens_validate(&cam->state.lens) == CNK_FALSE) {
        ok = CNK_FALSE;
    }
    if (cam->blend_duration_ticks < 0) {
        cam->blend_duration_ticks = 0;
        ok = CNK_FALSE;
    }
    if (cam->blend_curve < CNK_BLEND_CUT || cam->blend_curve > CNK_BLEND_EASE_IN_OUT) {
        cam->blend_curve = CNK_BLEND_CUT;
        ok = CNK_FALSE;
    }
    return ok;
}

CNK_API void cnk_camera_init_perspective(cnk_camera *cam, int viewport_w, int viewport_h, cnk_fx fov_y_deg, cnk_fx near_clip, cnk_fx far_clip)
{
    if (cam == 0) {
        return;
    }
    cnk_camera_reset(cam);
    cnk_lens_perspective(&cam->profile.lens, viewport_w, viewport_h, fov_y_deg, near_clip, far_clip);
    cam->state.lens = cam->profile.lens;
    cnk_camera_validate(cam);
}

CNK_API void cnk_camera_init_orthographic(cnk_camera *cam, int viewport_w, int viewport_h, cnk_fx ortho_height, cnk_fx near_clip, cnk_fx far_clip)
{
    if (cam == 0) {
        return;
    }
    cnk_camera_reset(cam);
    cnk_lens_orthographic(&cam->profile.lens, viewport_w, viewport_h, ortho_height, near_clip, far_clip);
    cam->state.lens = cam->profile.lens;
    cnk_camera_validate(cam);
}

CNK_API void cnk_camera_init_orbit(cnk_camera *cam, int viewport_w, int viewport_h, cnk_fx distance, cnk_fx yaw_deg, cnk_fx pitch_deg, cnk_fx near_clip, cnk_fx far_clip)
{
    cnk_profile p;
    cnk_pose pose;
    if (cam == 0) {
        return;
    }
    cnk_camera_reset(cam);
    cnk_profile_default(&p);
    p.mode = CNK_MODE_ORBIT;
    p.style = CNK_STYLE_ORBIT;
    p.distance = distance;
    p.min_distance = 0;
    p.max_distance = CNK_FX_FROM_INT(4096);
    cnk_lens_perspective(&p.lens, viewport_w, viewport_h, CNK_DEG(60), near_clip, far_clip);
    cnk_profile_validate(&p);
    cnk_camera_apply_profile(cam, &p);
    pose.pos = cam->state.pose.pos;
    pose.yaw_deg = yaw_deg;
    pose.pitch_deg = pitch_deg;
    pose.roll_deg = 0;
    cnk_camera_set_pose(cam, pose);
    cam->state.lens = p.lens;
}

CNK_API void cnk_camera_apply_profile(cnk_camera *cam, const cnk_profile *profile)
{
    if (cam == 0 || profile == 0) {
        return;
    }
    cam->profile = *profile;
    cnk_profile_validate(&cam->profile);
    if (profile->mode == CNK_MODE_FIXED) {
        cam->state.pose.pos = profile->camera_offset;
        cam->state.look_at = profile->look_offset;
        cam->state.pose = cnk_pose_look_at(cam->state.pose.pos, cam->state.look_at);
        cam->state.lens = profile->lens;
        cam->state.basis = cnk_cam_basis(cam, cam->state.pose.yaw_deg, cam->state.pose.pitch_deg, cam->state.pose.roll_deg);
    }
}

CNK_API void cnk_camera_set_target(cnk_camera *cam, cnk_vec3 pos, cnk_vec3 velocity, cnk_fx yaw_deg, cnk_fx pitch_deg, cnk_fx roll_deg)
{
    if (cam == 0) {
        return;
    }
    cam->target.pos = pos;
    cam->target.velocity = velocity;
    cam->target.yaw_deg = yaw_deg;
    cam->target.pitch_deg = pitch_deg;
    cam->target.roll_deg = roll_deg;
    cam->target.valid = CNK_TRUE;
}

CNK_API void cnk_camera_set_transform_provider(cnk_camera *cam, const cnk_transform_provider *provider)
{
    if (cam == 0) {
        return;
    }
    if (provider == 0) {
        cnk_transform_provider_clear(&cam->transform_provider);
        cam->state.basis = cnk_basis_from_angles(cam->state.pose.yaw_deg, cam->state.pose.pitch_deg, cam->state.pose.roll_deg);
        return;
    }
    cam->transform_provider = *provider;
    if (cam->transform_provider.mode != CNK_TRANSFORM_MODE_RECEIVE_PROVIDER) {
        cam->transform_provider.mode = CNK_TRANSFORM_MODE_INTERNAL;
    }
    cam->state.basis = cnk_cam_basis(cam, cam->state.pose.yaw_deg, cam->state.pose.pitch_deg, cam->state.pose.roll_deg);
}

CNK_API void cnk_camera_clear_transform_provider(cnk_camera *cam)
{
    if (cam == 0) {
        return;
    }
    cnk_transform_provider_clear(&cam->transform_provider);
    cam->state.basis = cnk_basis_from_angles(cam->state.pose.yaw_deg, cam->state.pose.pitch_deg, cam->state.pose.roll_deg);
}

CNK_API const cnk_transform_provider *cnk_camera_get_transform_provider(const cnk_camera *cam)
{
    if (cam == 0) {
        return 0;
    }
    return &cam->transform_provider;
}

CNK_API void cnk_camera_set_pose(cnk_camera *cam, cnk_pose pose)
{
    if (cam == 0) {
        return;
    }
    cam->state.pose = pose;
    cam->state.basis = cnk_cam_basis(cam, pose.yaw_deg, pose.pitch_deg, pose.roll_deg);
}

CNK_API void cnk_camera_set_viewport(cnk_camera *cam, int w, int h)
{
    if (cam == 0) {
        return;
    }
    cam->profile.lens.viewport_w = w;
    cam->profile.lens.viewport_h = h;
    cnk_lens_validate(&cam->profile.lens);
    cam->state.lens.viewport_w = w;
    cam->state.lens.viewport_h = h;
    cnk_lens_validate(&cam->state.lens);
}

CNK_API void cnk_camera_set_clip(cnk_camera *cam, cnk_fx near_clip, cnk_fx far_clip)
{
    if (cam == 0) {
        return;
    }
    cam->profile.lens.near_clip = near_clip;
    cam->profile.lens.far_clip = far_clip;
    cam->state.lens.near_clip = near_clip;
    cam->state.lens.far_clip = far_clip;
    cnk_lens_validate(&cam->profile.lens);
    cnk_lens_validate(&cam->state.lens);
}

CNK_API void cnk_camera_set_fov(cnk_camera *cam, cnk_fx fov_y_deg)
{
    if (cam == 0) {
        return;
    }
    cam->profile.lens.fov_y_deg = fov_y_deg;
    cam->state.lens.fov_y_deg = fov_y_deg;
    cnk_lens_validate(&cam->profile.lens);
    cnk_lens_validate(&cam->state.lens);
}

CNK_API void cnk_camera_set_ortho_height(cnk_camera *cam, cnk_fx ortho_height)
{
    if (cam == 0) {
        return;
    }
    cam->profile.lens.ortho_height = ortho_height;
    cam->state.lens.ortho_height = ortho_height;
    cnk_lens_validate(&cam->profile.lens);
    cnk_lens_validate(&cam->state.lens);
}

CNK_API void cnk_camera_set_orbit_distance(cnk_camera *cam, cnk_fx distance)
{
    if (cam == 0) {
        return;
    }
    cam->profile.distance = distance;
    cnk_profile_validate(&cam->profile);
}

CNK_API void cnk_camera_add_input(cnk_camera *cam, cnk_fx yaw_delta_deg, cnk_fx pitch_delta_deg, cnk_fx zoom_delta, cnk_fx fov_delta_deg)
{
    if (cam == 0) {
        return;
    }
    cam->input_yaw_delta += yaw_delta_deg;
    cam->input_pitch_delta += pitch_delta_deg;
    cam->input_zoom_delta += zoom_delta;
    cam->input_fov_delta += fov_delta_deg;
}

CNK_API void cnk_camera_snap(cnk_camera *cam)
{
    cnk_state desired;
    if (cam == 0) {
        return;
    }
    desired = cnk_solve_desired(cam);
    cam->state = desired;
    cam->previous_state = desired;
}

CNK_API void cnk_camera_update(cnk_camera *cam, int dt_ticks, cnk_world_probe_fn probe, void *probe_user)
{
    cnk_state desired;
    cnk_state next;
    cnk_fx t;
    int i;
    if (cam == 0) {
        return;
    }
    if (dt_ticks <= 0) {
        dt_ticks = 1;
    }
    cam->previous_state = cam->state;
    if (cam->profile.mode == CNK_MODE_CUTSCENE && cam->track.count > 0) {
        cam->track.time_ticks += dt_ticks;
        if (cam->track.duration_ticks > 0 && cam->track.time_ticks > cam->track.duration_ticks) {
            if (cam->track.loop != 0) {
                cam->track.time_ticks = cam->track.time_ticks % cam->track.duration_ticks;
            } else {
                cam->track.time_ticks = cam->track.duration_ticks;
            }
        }
    }
    desired = cnk_solve_desired(cam);
    cnk_apply_collision(cam, &desired, probe, probe_user);
    next = cam->state;
    next.pose.pos = cnk_vec3_approach(cam->state.pose.pos, desired.pose.pos, cam->profile.pos_lag);
    next.pose.yaw_deg = cnk_approach(cam->state.pose.yaw_deg, desired.pose.yaw_deg, cam->profile.rot_lag);
    next.pose.pitch_deg = cnk_approach(cam->state.pose.pitch_deg, desired.pose.pitch_deg, cam->profile.rot_lag);
    next.pose.roll_deg = cnk_approach(cam->state.pose.roll_deg, desired.pose.roll_deg, cam->profile.rot_lag);
    next.lens = desired.lens;
    next.lens.fov_y_deg = cnk_approach(cam->state.lens.fov_y_deg, desired.lens.fov_y_deg, cam->profile.fov_lag);
    next.look_at = cnk_vec3_approach(cam->state.look_at, desired.look_at, cam->profile.rot_lag);
    next.basis = cnk_cam_basis(cam, next.pose.yaw_deg, next.pose.pitch_deg, next.pose.roll_deg);
    next.valid = CNK_TRUE;
    if (cam->blend_active != 0) {
        cam->blend_time_ticks += dt_ticks;
        if (cam->blend_duration_ticks <= 0) {
            t = CNK_ONE;
        } else {
            t = CNK_FX_FRAC(cam->blend_time_ticks, cam->blend_duration_ticks);
        }
        t = cnk_fx_clamp(t, 0, CNK_ONE);
        if (cam->blend_curve == CNK_BLEND_EASE_IN_OUT) {
            t = cnk_ease_in_out(t);
        }
        next = cnk_state_lerp(cam->blend_from, cam->blend_to, t, cnk_cam_provider(cam));
        if (cam->blend_time_ticks >= cam->blend_duration_ticks) {
            cam->blend_active = 0;
        }
    }
    for (i = 0; i < CNK_MAX_SHAKES; ++i) {
        if (cam->shakes[i].active != 0) {
            cam->shakes[i].time_ticks += dt_ticks;
            if (cam->shakes[i].time_ticks >= cam->shakes[i].duration_ticks) {
                cam->shakes[i].active = 0;
            }
        }
    }
    cnk_apply_shakes(cam, &next);
    cam->state = next;
    cam->input_yaw_delta = 0;
    cam->input_pitch_delta = 0;
    cam->input_zoom_delta = 0;
    cam->input_fov_delta = 0;
    cam->frame_counter += 1UL;
}

CNK_API void cnk_camera_start_blend(cnk_camera *cam, cnk_state to_state, int duration_ticks, int curve)
{
    if (cam == 0) {
        return;
    }
    cam->blend_from = cam->state;
    cam->blend_to = to_state;
    cam->blend_active = 1;
    cam->blend_time_ticks = 0;
    cam->blend_duration_ticks = duration_ticks;
    cam->blend_curve = curve;
    if (duration_ticks <= 0 || curve == CNK_BLEND_CUT) {
        cam->state = to_state;
        cam->blend_active = 0;
    }
}

CNK_API int cnk_camera_add_shake(cnk_camera *cam, int duration_ticks, cnk_fx pos_amp, cnk_fx rot_amp_deg, cnk_fx fov_amp_deg, int frequency_hz, cnk_u32 seed)
{
    return cnk_camera_add_shake_ex(cam, duration_ticks, pos_amp, rot_amp_deg, fov_amp_deg, frequency_hz, seed, CNK_SHAKE_CAMERA_LOCAL, 0);
}

CNK_API int cnk_camera_add_shake_ex(cnk_camera *cam, int duration_ticks, cnk_fx pos_amp, cnk_fx rot_amp_deg, cnk_fx fov_amp_deg, int frequency_hz, cnk_u32 seed, int play_space, cnk_fx user_yaw_deg)
{
    int i;
    if (cam == 0) {
        return -1;
    }
    for (i = 0; i < CNK_MAX_SHAKES; ++i) {
        if (cam->shakes[i].active == 0) {
            cam->shakes[i].active = 1;
            cam->shakes[i].time_ticks = 0;
            cam->shakes[i].duration_ticks = duration_ticks;
            cam->shakes[i].pos_amp = pos_amp;
            cam->shakes[i].rot_amp_deg = rot_amp_deg;
            cam->shakes[i].fov_amp_deg = fov_amp_deg;
            cam->shakes[i].frequency_hz = frequency_hz;
            cam->shakes[i].seed = seed;
            cam->shakes[i].play_space = play_space;
            cam->shakes[i].user_yaw_deg = user_yaw_deg;
            return i;
        }
    }
    return -1;
}

CNK_API void cnk_camera_clear_shakes(cnk_camera *cam)
{
    int i;
    if (cam == 0) {
        return;
    }
    for (i = 0; i < CNK_MAX_SHAKES; ++i) {
        cam->shakes[i].active = 0;
    }
}

CNK_API void cnk_track_clear(cnk_track *track)
{
    if (track == 0) {
        return;
    }
    track->count = 0;
    track->time_ticks = 0;
    track->duration_ticks = 0;
    track->loop = 0;
}

CNK_API int cnk_track_add_key(cnk_track *track, const cnk_keyframe *key)
{
    int idx;
    if (track == 0 || key == 0) {
        return -1;
    }
    if (track->count >= CNK_MAX_KEYFRAMES) {
        return -1;
    }
    idx = track->count;
    track->keys[idx] = *key;
    track->count += 1;
    if (key->time_ticks > track->duration_ticks) {
        track->duration_ticks = key->time_ticks;
    }
    return idx;
}

CNK_API void cnk_track_set_loop(cnk_track *track, int loop)
{
    if (track == 0) {
        return;
    }
    track->loop = loop;
}

CNK_API void cnk_rail_clear(cnk_rail *rail)
{
    if (rail == 0) {
        return;
    }
    rail->count = 0;
    rail->cursor = 0;
    rail->t = 0;
    rail->closed = 0;
}

CNK_API int cnk_rail_add_point(cnk_rail *rail, cnk_vec3 point)
{
    int idx;
    if (rail == 0) {
        return -1;
    }
    if (rail->count >= CNK_MAX_KEYFRAMES) {
        return -1;
    }
    idx = rail->count;
    rail->points[idx] = point;
    rail->count += 1;
    return idx;
}

CNK_API cnk_mat4 cnk_camera_view_matrix(const cnk_camera *cam)
{
    cnk_mat4 mat;
    cnk_basis b;
    cnk_vec3 p;
    int i;
    for (i = 0; i < 16; ++i) {
        mat.m[i] = 0;
    }
    if (cam == 0) {
        return mat;
    }
    b = cam->state.basis;
    p = cam->state.pose.pos;
    mat.m[0] = b.right.x;
    mat.m[1] = b.right.y;
    mat.m[2] = b.right.z;
    mat.m[3] = -cnk_vec3_dot(p, b.right);
    mat.m[4] = b.up.x;
    mat.m[5] = b.up.y;
    mat.m[6] = b.up.z;
    mat.m[7] = -cnk_vec3_dot(p, b.up);
    mat.m[8] = b.forward.x;
    mat.m[9] = b.forward.y;
    mat.m[10] = b.forward.z;
    mat.m[11] = -cnk_vec3_dot(p, b.forward);
    mat.m[15] = CNK_ONE;
    return mat;
}

CNK_API cnk_mat4 cnk_camera_projection_matrix(const cnk_camera *cam)
{
    cnk_mat4 mat;
    cnk_lens lens;
    cnk_fx half_fov;
    cnk_fx tan_half;
    cnk_fx f;
    cnk_fx range;
    int i;
    for (i = 0; i < 16; ++i) {
        mat.m[i] = 0;
    }
    if (cam == 0) {
        return mat;
    }
    lens = cam->state.lens;
    cnk_lens_validate(&lens);
    if (lens.projection == CNK_PROJ_ORTHOGRAPHIC) {
        cnk_fx width;
        cnk_fx height;
        width = cnk_fx_mul(lens.ortho_height, lens.aspect);
        height = lens.ortho_height;
        mat.m[0] = cnk_fx_div(CNK_FX_FROM_INT(2), width);
        mat.m[5] = cnk_fx_div(CNK_FX_FROM_INT(2), height);
        range = lens.far_clip - lens.near_clip;
        mat.m[10] = cnk_fx_div(CNK_ONE, range);
        mat.m[11] = -cnk_fx_div(lens.near_clip, range);
        mat.m[15] = CNK_ONE;
    } else {
        half_fov = lens.fov_y_deg >> 1;
        tan_half = cnk_fx_div(cnk_sin_deg(half_fov), cnk_cos_deg(half_fov));
        if (tan_half <= 0) {
            tan_half = CNK_ONE;
        }
        f = cnk_fx_div(CNK_ONE, tan_half);
        mat.m[0] = cnk_fx_div(f, lens.aspect);
        mat.m[5] = f;
        mat.m[2] = lens.frustum_offset_x;
        mat.m[6] = lens.frustum_offset_y;
        range = lens.far_clip - lens.near_clip;
        mat.m[10] = cnk_fx_div(lens.far_clip, range);
        mat.m[11] = -cnk_fx_div(cnk_fx_mul(lens.far_clip, lens.near_clip), range);
        mat.m[14] = CNK_ONE;
    }
    return mat;
}

CNK_API int cnk_camera_world_to_view(const cnk_camera *cam, cnk_vec3 world, cnk_vec3 *out_view)
{
    cnk_vec3 d;
    cnk_basis b;
    if (cam == 0 || out_view == 0) {
        return CNK_FALSE;
    }
    b = cam->state.basis;
    d = cnk_transform_move(cnk_cam_provider(cam), world, cnk_vec3_make(-cam->state.pose.pos.x, -cam->state.pose.pos.y, -cam->state.pose.pos.z));
    out_view->x = cnk_vec3_dot(d, b.right);
    out_view->y = cnk_vec3_dot(d, b.up);
    out_view->z = cnk_vec3_dot(d, b.forward);
    return CNK_TRUE;
}

CNK_API int cnk_camera_project_point(const cnk_camera *cam, cnk_vec3 world, int *out_x, int *out_y, cnk_fx *out_depth)
{
    cnk_vec3 v;
    cnk_lens lens;
    cnk_fx half_w;
    cnk_fx half_h;
    cnk_fx sx;
    cnk_fx sy;
    cnk_fx half_fov;
    cnk_fx tan_half;
    cnk_fx focal_y;
    cnk_fx focal_x;
    if (cam == 0 || out_x == 0 || out_y == 0 || out_depth == 0) {
        return CNK_FALSE;
    }
    lens = cam->state.lens;
    cnk_lens_validate(&lens);
    cnk_camera_world_to_view(cam, world, &v);
    *out_depth = v.z;
    half_w = CNK_FX_FROM_INT(lens.viewport_w / 2);
    half_h = CNK_FX_FROM_INT(lens.viewport_h / 2);
    if (lens.projection == CNK_PROJ_ORTHOGRAPHIC) {
        cnk_fx ortho_w;
        ortho_w = cnk_fx_mul(lens.ortho_height, lens.aspect);
        sx = half_w + lens.viewport_offset_x + cnk_fx_mul(cnk_fx_div(v.x + lens.frustum_offset_x, ortho_w), CNK_FX_FROM_INT(lens.viewport_w));
        sy = half_h + lens.viewport_offset_y - cnk_fx_mul(cnk_fx_div(v.y + lens.frustum_offset_y, lens.ortho_height), CNK_FX_FROM_INT(lens.viewport_h));
        *out_x = CNK_FX_TO_INT(sx);
        *out_y = CNK_FX_TO_INT(sy);
        return CNK_TRUE;
    }
    if (v.z <= lens.near_clip) {
        *out_x = 0;
        *out_y = 0;
        return CNK_FALSE;
    }
    if (v.z >= lens.far_clip) {
        *out_x = 0;
        *out_y = 0;
        return CNK_FALSE;
    }
    half_fov = lens.fov_y_deg >> 1;
    tan_half = cnk_fx_div(cnk_sin_deg(half_fov), cnk_cos_deg(half_fov));
    if (tan_half <= 0) {
        tan_half = CNK_ONE;
    }
    focal_y = cnk_fx_div(half_h, tan_half);
    focal_x = focal_y;
    sx = half_w + lens.viewport_offset_x + cnk_fx_div(cnk_fx_mul(v.x + cnk_fx_mul(lens.frustum_offset_x, v.z), focal_x), v.z);
    sy = half_h + lens.viewport_offset_y - cnk_fx_div(cnk_fx_mul(v.y + cnk_fx_mul(lens.frustum_offset_y, v.z), focal_y), v.z);
    *out_x = CNK_FX_TO_INT(sx);
    *out_y = CNK_FX_TO_INT(sy);
    return CNK_TRUE;
}

CNK_API cnk_basis cnk_basis_from_angles(cnk_fx yaw_deg, cnk_fx pitch_deg, cnk_fx roll_deg)
{
    cnk_basis b;
    cnk_fx sy;
    cnk_fx cy;
    cnk_fx sp;
    cnk_fx cp;
    cnk_fx sr;
    cnk_fx cr;
    cnk_vec3 up0;
    cnk_vec3 right0;
    cnk_vec3 up_roll;
    cnk_vec3 right_roll;
    sy = cnk_sin_deg(yaw_deg);
    cy = cnk_cos_deg(yaw_deg);
    sp = cnk_sin_deg(pitch_deg);
    cp = cnk_cos_deg(pitch_deg);
    b.forward.x = cnk_fx_mul(sy, cp);
    b.forward.y = sp;
    b.forward.z = cnk_fx_mul(cy, cp);
    b.forward = cnk_vec3_normalize_fast(b.forward);
    right0.x = cy;
    right0.y = 0;
    right0.z = -sy;
    right0 = cnk_vec3_normalize_fast(right0);
    up0 = cnk_vec3_cross(b.forward, right0);
    up0 = cnk_vec3_normalize_fast(up0);
    if (roll_deg != 0) {
        sr = cnk_sin_deg(roll_deg);
        cr = cnk_cos_deg(roll_deg);
        right_roll = cnk_vec3_add(cnk_vec3_scale(right0, cr), cnk_vec3_scale(up0, sr));
        up_roll = cnk_vec3_add(cnk_vec3_scale(up0, cr), cnk_vec3_scale(right0, -sr));
        b.right = cnk_vec3_normalize_fast(right_roll);
        b.up = cnk_vec3_normalize_fast(up_roll);
    } else {
        b.right = right0;
        b.up = up0;
    }
    return b;
}

CNK_API cnk_basis cnk_basis_from_angles_provider(const cnk_transform_provider *provider, cnk_fx yaw_deg, cnk_fx pitch_deg, cnk_fx roll_deg)
{
    cnk_basis b;
    cnk_vec3 angles;
    if (cnk_transform_provider_is_active(provider) == 0 || provider->rotate == 0) {
        return cnk_basis_from_angles(yaw_deg, pitch_deg, roll_deg);
    }
    angles = cnk_vec3_make(yaw_deg, pitch_deg, roll_deg);
    b.right = cnk_transform_rotate(provider, cnk_vec3_make(CNK_ONE, 0, 0), angles);
    b.up = cnk_transform_rotate(provider, cnk_vec3_make(0, CNK_ONE, 0), angles);
    b.forward = cnk_transform_rotate(provider, cnk_vec3_make(0, 0, CNK_ONE), angles);
    b.right = cnk_vec3_normalize_fast(b.right);
    b.up = cnk_vec3_normalize_fast(b.up);
    b.forward = cnk_vec3_normalize_fast(b.forward);
    return b;
}

CNK_API cnk_pose cnk_pose_look_at(cnk_vec3 from, cnk_vec3 to)
{
    cnk_pose pose;
    cnk_vec3 d;
    cnk_fx ax;
    cnk_fx az;
    cnk_fx ay;
    int yaw;
    int pitch;
    d = cnk_vec3_sub(to, from);
    ax = cnk_fx_abs(d.x);
    az = cnk_fx_abs(d.z);
    ay = cnk_fx_abs(d.y);
    yaw = 0;
    if (ax > az) {
        if (d.x >= 0) {
            yaw = 90;
        } else {
            yaw = 270;
        }
        if (az > 0) {
            yaw += CNK_FX_TO_INT(cnk_fx_div(az, ax) * 45);
        }
    } else {
        if (d.z >= 0) {
            yaw = 0;
        } else {
            yaw = 180;
        }
        if (ax > 0) {
            if (d.x >= 0 && d.z >= 0) {
                yaw += CNK_FX_TO_INT(cnk_fx_div(ax, az) * 45);
            } else if (d.x < 0 && d.z >= 0) {
                yaw -= CNK_FX_TO_INT(cnk_fx_div(ax, az) * 45);
            } else if (d.x >= 0 && d.z < 0) {
                yaw -= CNK_FX_TO_INT(cnk_fx_div(ax, az) * 45);
            } else {
                yaw += CNK_FX_TO_INT(cnk_fx_div(ax, az) * 45);
            }
        }
    }
    if (yaw < 0) {
        yaw += 360;
    }
    pitch = 0;
    if (ay > 0) {
        cnk_fx flat;
        flat = ax + az;
        if (flat <= 0) {
            pitch = d.y >= 0 ? 89 : -89;
        } else {
            pitch = CNK_FX_TO_INT(cnk_fx_div(ay, flat) * 45);
            if (pitch > 89) {
                pitch = 89;
            }
            if (d.y < 0) {
                pitch = -pitch;
            }
        }
    }
    pose.pos = from;
    pose.yaw_deg = CNK_DEG(yaw);
    pose.pitch_deg = CNK_DEG(pitch);
    pose.roll_deg = 0;
    return pose;
}

CNK_API void cnk_arena_init(cnk_arena *arena)
{
    int i;
    if (arena == 0) {
        return;
    }
    for (i = 0; i < CNK_MAX_VCAMS; ++i) {
        arena->used[i] = 0;
        cnk_camera_reset(&arena->cameras[i]);
    }
}

CNK_API cnk_camera *cnk_arena_alloc_camera(cnk_arena *arena)
{
    int i;
    if (arena == 0) {
        return 0;
    }
    for (i = 0; i < CNK_MAX_VCAMS; ++i) {
        if (arena->used[i] == 0) {
            arena->used[i] = 1;
            cnk_camera_reset(&arena->cameras[i]);
            return &arena->cameras[i];
        }
    }
    return 0;
}

CNK_API void cnk_arena_free_camera(cnk_arena *arena, cnk_camera *cam)
{
    int i;
    if (arena == 0 || cam == 0) {
        return;
    }
    for (i = 0; i < CNK_MAX_VCAMS; ++i) {
        if (&arena->cameras[i] == cam) {
            arena->used[i] = 0;
            return;
        }
    }
}
