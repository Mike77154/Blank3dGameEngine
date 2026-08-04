#include "blank3d_cameranaku.h"

#include <stdio.h>
#include <string.h>

static cnk_fx b3d_q20_to_cnk(g3d_fix value)
{
    return (cnk_fx)(value / 16L);
}

static g3d_fix b3d_cnk_to_q20(cnk_fx value)
{
    return (g3d_fix)(value * 16L);
}

static gwp89_fx b3d_q20_to_q16(g3d_fix value)
{
    return (gwp89_fx)(value * 16L);
}

static cnk_fx b3d_gamlib_yaw_to_cnk(g3d_fix yaw)
{
    g3d_fix logical;
    logical = g3d_fix_wrap_angle_deg(
        g3d_fix_sub_sat(yaw, G3D_FIX_FROM_INT(180)));
    return b3d_q20_to_cnk(logical);
}

static cnk_vec3 b3d_vec3_to_cnk(Vec3 value)
{
    return cnk_vec3_make(b3d_q20_to_cnk(value.x),
                         b3d_q20_to_cnk(value.y),
                         b3d_q20_to_cnk(value.z));
}

static Vec3 b3d_cnk_to_vec3(cnk_vec3 value)
{
    return gamlib_vec3(b3d_cnk_to_q20(value.x),
                       b3d_cnk_to_q20(value.y),
                       b3d_cnk_to_q20(value.z));
}

static GWP89_Vec3 b3d_vec3_to_gwp(Vec3 value)
{
    return gwp89_v3(b3d_q20_to_q16(value.x),
                    b3d_q20_to_q16(value.y),
                    b3d_q20_to_q16(value.z));
}

static int b3d_cnk_provider_move(void *user,
                                 cnk_vec3 position,
                                 cnk_vec3 delta,
                                 cnk_vec3 *out_position)
{
    Blank3DCameraNaku *camera;
    Vec3 a;
    Vec3 b;
    Vec3 out;
    camera = (Blank3DCameraNaku *)user;
    if (!out_position) return 0;
    a = b3d_cnk_to_vec3(position);
    b = b3d_cnk_to_vec3(delta);
    gamlib_vec3_add(&out, &a, &b);
    *out_position = b3d_vec3_to_cnk(out);
    if (camera) camera->provider_move_calls += 1UL;
    return 1;
}

static int b3d_cnk_provider_scale(void *user,
                                  cnk_vec3 value,
                                  cnk_vec3 scale,
                                  cnk_vec3 *out_value)
{
    Blank3DCameraNaku *camera;
    Vec3 v;
    Vec3 s;
    Vec3 out;
    camera = (Blank3DCameraNaku *)user;
    if (!out_value) return 0;
    v = b3d_cnk_to_vec3(value);
    s = b3d_cnk_to_vec3(scale);
    out.x = g3d_fix_mul(v.x, s.x);
    out.y = g3d_fix_mul(v.y, s.y);
    out.z = g3d_fix_mul(v.z, s.z);
    *out_value = b3d_vec3_to_cnk(out);
    if (camera) camera->provider_scale_calls += 1UL;
    return 1;
}

static int b3d_cnk_provider_rotate(void *user,
                                   cnk_vec3 value,
                                   cnk_vec3 euler_deg,
                                   cnk_vec3 *out_value)
{
    Blank3DCameraNaku *camera;
    Transform transform;
    Vec3 input;
    Vec3 right;
    Vec3 up;
    Vec3 gamlib_forward;
    Vec3 positive_z;
    Vec3 term;
    Vec3 out;
    camera = (Blank3DCameraNaku *)user;
    if (!out_value) return 0;

    transform_init(&transform);
    transform.rotation.y = b3d_cnk_to_q20(euler_deg.x);
    transform.rotation.x = b3d_cnk_to_q20(euler_deg.y);
    transform.rotation.z = b3d_cnk_to_q20(euler_deg.z);
    transform_get_local_axes(&transform, &right, &up, &gamlib_forward);

    /* Gamlib3D calls local -Z "forward". Cameranaku uses local +Z as its
       canonical forward, so invert only that basis leg at the ABI boundary. */
    positive_z = gamlib_vec3(g3d_fix_neg_sat(gamlib_forward.x),
                             g3d_fix_neg_sat(gamlib_forward.y),
                             g3d_fix_neg_sat(gamlib_forward.z));
    input = b3d_cnk_to_vec3(value);
    out = gamlib_vec3(0, 0, 0);
    gamlib_vec3_scale(&term, &right, input.x);
    gamlib_vec3_add(&out, &out, &term);
    gamlib_vec3_scale(&term, &up, input.y);
    gamlib_vec3_add(&out, &out, &term);
    gamlib_vec3_scale(&term, &positive_z, input.z);
    gamlib_vec3_add(&out, &out, &term);

    *out_value = b3d_vec3_to_cnk(out);
    if (camera) camera->provider_rotate_calls += 1UL;
    return 1;
}

static void b3d_cnk_lens(cnk_profile *profile,
                         int width,
                         int height,
                         g3d_fix fov,
                         g3d_fix near_clip,
                         g3d_fix far_clip)
{
    if (!profile) return;
    cnk_lens_perspective(&profile->lens,
                         width,
                         height,
                         b3d_q20_to_cnk(fov),
                         b3d_q20_to_cnk(near_clip),
                         b3d_q20_to_cnk(far_clip));
}

static void b3d_cnk_apply_current_profile(Blank3DCameraNaku *camera)
{
    if (!camera) return;
    if (camera->mode == B3D_CNK_CAMERA_FPS) {
        cnk_camera_apply_profile(&camera->camera, &camera->fps_profile);
        camera->view_style = GWP89_VIEW_FPS;
    } else {
        cnk_camera_apply_profile(&camera->camera, &camera->tps_profile);
        camera->view_style = GWP89_VIEW_OVER_SHOULDER;
    }
    cnk_camera_set_transform_provider(&camera->camera,
                                      &camera->transform_provider);
}

void blank3d_cameranaku_init(Blank3DCameraNaku *camera,
                              int width,
                              int height,
                              g3d_fix fov,
                              g3d_fix near_clip,
                              g3d_fix far_clip)
{
    if (!camera) return;
    memset(camera, 0, sizeof(*camera));
    camera->width = width > 0 ? width : 640;
    camera->height = height > 0 ? height : 480;
    camera->mode = B3D_CNK_CAMERA_TPS;
    camera->camera_id = B3D_CNK_CAMERA_ID;
    camera->yaw = 0;
    camera->pitch = 0;
    camera->pitch_min = G3D_FIX_FROM_INT(-85);
    camera->pitch_max = G3D_FIX_FROM_INT(85);
    camera->fov = fov;
    camera->zoom_fx = GWP89_FIX_ONE;
    camera->target_position = gamlib_vec3(0, 0, 0);
    camera->target_velocity = gamlib_vec3(0, 0, 0);

    cnk_transform_provider_clear(&camera->transform_provider);
    cnk_transform_provider_set(&camera->transform_provider,
                               camera,
                               b3d_cnk_provider_move,
                               b3d_cnk_provider_scale,
                               b3d_cnk_provider_rotate);
    cnk_transform_provider_set_mode(&camera->transform_provider,
                                    CNK_TRANSFORM_MODE_RECEIVE_PROVIDER);

    cnk_camera_reset(&camera->camera);
    cnk_profile_style_fps(&camera->fps_profile);
    cnk_profile_style_orbit(&camera->tps_profile);
    b3d_cnk_lens(&camera->fps_profile, camera->width, camera->height,
                 fov, near_clip, far_clip);
    b3d_cnk_lens(&camera->tps_profile, camera->width, camera->height,
                 fov, near_clip, far_clip);
    camera->fps_profile.pos_lag = CNK_ONE;
    camera->fps_profile.rot_lag = CNK_ONE;
    camera->fps_profile.fov_lag = CNK_ONE;
    camera->tps_profile.pos_lag = CNK_FX_FRAC(45, 100);
    camera->tps_profile.rot_lag = CNK_FX_FRAC(55, 100);
    camera->tps_profile.fov_lag = CNK_FX_FRAC(55, 100);
    b3d_cnk_apply_current_profile(camera);
    camera->initialized = 1;
    strcpy(camera->status,
           "Cameranaku89 v3.4 receive-provider ready: four feeder axes negated + Gamlib3D TRS");
}

void blank3d_cameranaku_configure(Blank3DCameraNaku *camera,
                                   g3d_fix eye_height,
                                   g3d_fix camera_height,
                                   g3d_fix camera_distance,
                                   g3d_fix shoulder,
                                   g3d_fix pitch_min,
                                   g3d_fix pitch_max,
                                   g3d_fix fov,
                                   g3d_fix near_clip,
                                   g3d_fix far_clip)
{
    if (!camera) return;
    camera->pitch_min = pitch_min;
    camera->pitch_max = pitch_max;
    camera->shoulder = shoulder;
    camera->fov = fov;

    cnk_profile_style_fps(&camera->fps_profile);
    camera->fps_profile.pivot_offset =
        cnk_vec3_make(0, b3d_q20_to_cnk(eye_height), 0);
    camera->fps_profile.min_pitch_deg = b3d_q20_to_cnk(pitch_min);
    camera->fps_profile.max_pitch_deg = b3d_q20_to_cnk(pitch_max);
    camera->fps_profile.pos_lag = CNK_ONE;
    camera->fps_profile.rot_lag = CNK_ONE;
    camera->fps_profile.fov_lag = CNK_ONE;
    b3d_cnk_lens(&camera->fps_profile, camera->width, camera->height,
                 fov, near_clip, far_clip);

    cnk_profile_style_orbit(&camera->tps_profile);
    camera->tps_profile.pivot_offset =
        cnk_vec3_make(0, b3d_q20_to_cnk(camera_height), 0);
    camera->tps_profile.look_offset = cnk_vec3_make(0, 0, 0);
    camera->tps_profile.distance = b3d_q20_to_cnk(camera_distance);
    camera->tps_profile.min_distance = CNK_FX_FRAC(1, 2);
    camera->tps_profile.max_distance = CNK_FX_FROM_INT(64);
    camera->tps_profile.min_pitch_deg = b3d_q20_to_cnk(pitch_min);
    camera->tps_profile.max_pitch_deg = b3d_q20_to_cnk(pitch_max);
    camera->tps_profile.pos_lag = CNK_FX_FRAC(45, 100);
    camera->tps_profile.rot_lag = CNK_FX_FRAC(55, 100);
    camera->tps_profile.fov_lag = CNK_FX_FRAC(55, 100);
    b3d_cnk_lens(&camera->tps_profile, camera->width, camera->height,
                 fov, near_clip, far_clip);
    b3d_cnk_apply_current_profile(camera);
}

void blank3d_cameranaku_set_mode(Blank3DCameraNaku *camera, int mode)
{
    if (!camera) return;
    camera->mode = mode == B3D_CNK_CAMERA_FPS
                 ? B3D_CNK_CAMERA_FPS : B3D_CNK_CAMERA_TPS;
    b3d_cnk_apply_current_profile(camera);
    camera->camera.state.pose.yaw_deg = b3d_gamlib_yaw_to_cnk(camera->yaw);
    camera->camera.state.pose.pitch_deg = b3d_q20_to_cnk(camera->pitch);
}

void blank3d_cameranaku_set_angles(Blank3DCameraNaku *camera,
                                    g3d_fix yaw,
                                    g3d_fix pitch)
{
    if (!camera) return;
    camera->yaw = g3d_fix_wrap_angle_deg(yaw);
    camera->pitch = g3d_fix_clamp(pitch,
                                  camera->pitch_min,
                                  camera->pitch_max);
}

void blank3d_cameranaku_add_look(Blank3DCameraNaku *camera,
                                 g3d_fix yaw_delta,
                                 g3d_fix pitch_delta)
{
    if (!camera) return;
    blank3d_cameranaku_set_angles(camera,
        g3d_fix_add_sat(camera->yaw, yaw_delta),
        g3d_fix_add_sat(camera->pitch, pitch_delta));
}

static g3d_fix b3d_cnk_feeder_magnitude(g3d_fix magnitude)
{
    return magnitude < 0 ? g3d_fix_neg_sat(magnitude) : magnitude;
}

void blank3d_cameranaku_feed_left(Blank3DCameraNaku *camera,
                                   g3d_fix magnitude)
{
    /* Negated feeder: old left delta was negative; Naku needs positive. */
    blank3d_cameranaku_add_look(camera,
        b3d_cnk_feeder_magnitude(magnitude), 0);
}

void blank3d_cameranaku_feed_right(Blank3DCameraNaku *camera,
                                    g3d_fix magnitude)
{
    /* Negated feeder: old right delta was positive; Naku needs negative. */
    blank3d_cameranaku_add_look(camera,
        g3d_fix_neg_sat(b3d_cnk_feeder_magnitude(magnitude)), 0);
}

void blank3d_cameranaku_feed_up(Blank3DCameraNaku *camera,
                                 g3d_fix magnitude)
{
    /* Negated feeder: old up delta was positive; Naku needs negative. */
    blank3d_cameranaku_add_look(camera, 0,
        g3d_fix_neg_sat(b3d_cnk_feeder_magnitude(magnitude)));
}

void blank3d_cameranaku_feed_down(Blank3DCameraNaku *camera,
                                   g3d_fix magnitude)
{
    /* Negated feeder: old down delta was negative; Naku needs positive. */
    blank3d_cameranaku_add_look(camera, 0,
        b3d_cnk_feeder_magnitude(magnitude));
}

void blank3d_cameranaku_set_target(Blank3DCameraNaku *camera,
                                    const Vec3 *position,
                                    const Vec3 *velocity)
{
    if (!camera || !position) return;
    camera->target_position = *position;
    camera->target_velocity = velocity ? *velocity : gamlib_vec3(0, 0, 0);
}

void blank3d_cameranaku_set_fov(Blank3DCameraNaku *camera, g3d_fix fov)
{
    if (!camera || fov <= 0) return;
    camera->fov = fov;
    camera->fps_profile.lens.fov_y_deg = b3d_q20_to_cnk(fov);
    camera->tps_profile.lens.fov_y_deg = b3d_q20_to_cnk(fov);
    cnk_camera_set_fov(&camera->camera, b3d_q20_to_cnk(fov));
}

void blank3d_cameranaku_set_zoom(Blank3DCameraNaku *camera, g3d_fix zoom_fx)
{
    if (!camera) return;
    camera->zoom_fx = zoom_fx > 0 ? zoom_fx : GWP89_FIX_ONE;
}

void blank3d_cameranaku_set_sway(Blank3DCameraNaku *camera,
                                  g3d_fix yaw_offset,
                                  g3d_fix pitch_offset)
{
    if (!camera) return;
    camera->sway_yaw = yaw_offset;
    camera->sway_pitch = pitch_offset;
}

void blank3d_cameranaku_add_recoil(Blank3DCameraNaku *camera,
                                    g3d_fix pitch_delta,
                                    g3d_fix shake_amplitude)
{
    cnk_fx shake;
    if (!camera) return;
    blank3d_cameranaku_add_look(camera, 0, pitch_delta);
    shake = b3d_q20_to_cnk(shake_amplitude);
    if (shake > 0) {
        (void)cnk_camera_add_shake_ex(&camera->camera,
                                      90,
                                      shake,
                                      shake,
                                      shake >> 1,
                                      32,
                                      camera->camera.frame_counter + 17UL,
                                      CNK_SHAKE_CAMERA_LOCAL,
                                      b3d_gamlib_yaw_to_cnk(camera->yaw));
    }
}

void blank3d_cameranaku_resize(Blank3DCameraNaku *camera,
                                int width,
                                int height)
{
    if (!camera) return;
    if (width > 0) camera->width = width;
    if (height > 0) camera->height = height;
    cnk_camera_set_viewport(&camera->camera, camera->width, camera->height);
    camera->fps_profile.lens.viewport_w = camera->width;
    camera->fps_profile.lens.viewport_h = camera->height;
    camera->tps_profile.lens.viewport_w = camera->width;
    camera->tps_profile.lens.viewport_h = camera->height;
}

void blank3d_cameranaku_update(Blank3DCameraNaku *camera,
                                unsigned short dt_ms)
{
    cnk_vec3 position;
    cnk_vec3 velocity;
    cnk_basis effective_basis;
    cnk_vec3 eye;
    cnk_vec3 shoulder_delta;
    cnk_fx yaw;
    cnk_fx pitch;
    int ticks;

    if (!camera || !camera->initialized) return;
    position = b3d_vec3_to_cnk(camera->target_position);
    velocity = b3d_vec3_to_cnk(camera->target_velocity);
    yaw = b3d_gamlib_yaw_to_cnk(camera->yaw);
    pitch = b3d_q20_to_cnk(camera->pitch);
    cnk_camera_set_target(&camera->camera,
                          position,
                          velocity,
                          yaw,
                          pitch,
                          0);
    cnk_camera_set_fov(&camera->camera, b3d_q20_to_cnk(camera->fov));

    if (camera->mode == B3D_CNK_CAMERA_TPS) {
        /* Orbit is the CamNaku profile that preserves full mouse pitch in TPS. */
        camera->camera.state.pose.yaw_deg = yaw;
        camera->camera.state.pose.pitch_deg = pitch;
    }
    ticks = dt_ms > 0 ? (int)dt_ms : 1;
    cnk_camera_update(&camera->camera, ticks, 0, 0);

    effective_basis = cnk_basis_from_angles_provider(
        &camera->transform_provider,
        yaw + b3d_q20_to_cnk(camera->sway_yaw),
        pitch + b3d_q20_to_cnk(camera->sway_pitch),
        0);
    eye = camera->camera.state.pose.pos;
    if (camera->mode == B3D_CNK_CAMERA_TPS && camera->shoulder != 0) {
        shoulder_delta = cnk_transform_scale_uniform(
            &camera->transform_provider,
            effective_basis.right,
            b3d_q20_to_cnk(camera->shoulder));
        eye = cnk_transform_move(&camera->transform_provider,
                                 eye,
                                 shoulder_delta);
    }

    camera->eye = b3d_cnk_to_vec3(eye);
    camera->forward = b3d_cnk_to_vec3(effective_basis.forward);
    camera->right = b3d_cnk_to_vec3(effective_basis.right);
    camera->up = b3d_cnk_to_vec3(effective_basis.up);
    sprintf(camera->status,
             "CamNaku provider active | TRS %lu/%lu/%lu | weapon camera %lu",
             camera->provider_move_calls,
             camera->provider_scale_calls,
             camera->provider_rotate_calls,
             camera->weapon_provider_calls);
}

void blank3d_cameranaku_get_view(const Blank3DCameraNaku *camera,
                                  Vec3 *eye,
                                  Vec3 *forward,
                                  Vec3 *right,
                                  Vec3 *up)
{
    if (!camera) return;
    if (eye) *eye = camera->eye;
    if (forward) *forward = camera->forward;
    if (right) *right = camera->right;
    if (up) *up = camera->up;
}

int blank3d_cameranaku_weapon_provider(void *context,
                                       GWP89_ProviderPacket *packet)
{
    Blank3DCameraNaku *camera;
    camera = (Blank3DCameraNaku *)context;
    if (!camera || !packet) return GWP89_PROVIDER_PASS;
    if (packet->phase != GWP89_PHASE_PRE ||
        packet->operation != GWP89_OP_GET_CAMERA)
        return GWP89_PROVIDER_PASS;

    /* CamaraNaku is the player camera. NPC weapon actors must preserve the
       camera/socket basis supplied by their own AI aiming bridge; otherwise
       every enemy inherits the player's view and can fire vertically or in
       an unrelated direction. */
    if (packet->actor_id != 1 || packet->actor_kind != 1)
        return GWP89_PROVIDER_PASS;

    packet->camera.valid = 1;
    packet->camera.camera_id = camera->camera_id;
    packet->camera.view_style = camera->view_style;
    packet->camera.origin = b3d_vec3_to_gwp(camera->eye);
    packet->camera.forward = b3d_vec3_to_gwp(camera->forward);
    packet->camera.right = b3d_vec3_to_gwp(camera->right);
    packet->camera.up = b3d_vec3_to_gwp(camera->up);
    packet->camera.zoom_fx = b3d_q20_to_q16(camera->zoom_fx);
    camera->weapon_provider_calls += 1UL;
    return GWP89_PROVIDER_HANDLED;
}

const char *blank3d_cameranaku_status(const Blank3DCameraNaku *camera)
{
    return camera ? camera->status : "Cameranaku89 unavailable";
}
