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


static cnk_fx b3d_q20_unit_to_cnk(g3d_fix value)
{
    return b3d_q20_to_cnk(value);
}

static const Blank3DCameraProfile *b3d_cnk_profile(
    const Blank3DCameraNaku *camera)
{
    if (!camera) return (const Blank3DCameraProfile *)0;
    return blank3d_camera_catalog_current(&camera->catalog);
}

static void b3d_cnk_build_runtime_profile(Blank3DCameraNaku *camera,
                                          const Blank3DCameraProfile *source)
{
    if (!camera || !source) return;
    if (source->rig == B3D_CAMERA_RIG_FPS)
        cnk_profile_style_fps(&camera->active_profile);
    else
        cnk_profile_style_orbit(&camera->active_profile);

    camera->active_profile.pivot_offset = cnk_vec3_make(
        b3d_q20_to_cnk(source->pivot_x),
        b3d_q20_to_cnk(source->pivot_y),
        b3d_q20_to_cnk(source->pivot_z));
    camera->active_profile.look_offset = cnk_vec3_make(
        b3d_q20_to_cnk(source->look_x),
        b3d_q20_to_cnk(source->look_y),
        b3d_q20_to_cnk(source->look_z));
    camera->active_profile.distance = b3d_q20_to_cnk(source->distance);
    camera->active_profile.min_distance = b3d_q20_to_cnk(source->min_distance);
    camera->active_profile.max_distance = b3d_q20_to_cnk(source->max_distance);
    camera->active_profile.min_pitch_deg = b3d_q20_to_cnk(source->pitch_min);
    camera->active_profile.max_pitch_deg = b3d_q20_to_cnk(source->pitch_max);
    camera->active_profile.pos_lag = b3d_q20_unit_to_cnk(source->pos_lag);
    camera->active_profile.rot_lag = b3d_q20_unit_to_cnk(source->rot_lag);
    camera->active_profile.fov_lag = b3d_q20_unit_to_cnk(source->fov_lag);
    camera->active_profile.collision_radius = source->collision_enabled
        ? b3d_q20_to_cnk(source->collision_radius) : 0;
    if (source->collision_enabled)
        camera->active_profile.flags |= CNK_FLAG_KEEP_LINE_OF_SIGHT;
    else
        camera->active_profile.flags &= ~CNK_FLAG_KEEP_LINE_OF_SIGHT;
    b3d_cnk_lens(&camera->active_profile,
                 camera->width,
                 camera->height,
                 source->fov,
                 source->near_clip,
                 source->far_clip);
}

static void b3d_cnk_apply_current_profile(Blank3DCameraNaku *camera)
{
    const Blank3DCameraProfile *profile;
    if (!camera) return;
    profile = b3d_cnk_profile(camera);
    if (!profile) return;

    b3d_cnk_build_runtime_profile(camera, profile);
    cnk_camera_apply_profile(&camera->camera, &camera->active_profile);
    cnk_camera_set_transform_provider(&camera->camera,
                                      &camera->transform_provider);
    camera->view_style = profile->view_style;
    camera->pitch_min = profile->pitch_min;
    camera->pitch_max = profile->pitch_max;
    camera->pitch = g3d_fix_clamp(camera->pitch,
                                  camera->pitch_min,
                                  camera->pitch_max);
    camera->fov = profile->fov;
    if (profile->view_style == GWP89_VIEW_FPS)
        camera->mode = B3D_CNK_CAMERA_FPS;
    else if (profile->view_style == GWP89_VIEW_OVER_SHOULDER)
        camera->mode = B3D_CNK_CAMERA_OTS;
    else
        camera->mode = B3D_CNK_CAMERA_TPS;
    camera->camera.state.pose.yaw_deg = b3d_gamlib_yaw_to_cnk(camera->yaw);
    camera->camera.state.pose.pitch_deg = b3d_q20_to_cnk(camera->pitch);
}

void blank3d_cameranaku_init(Blank3DCameraNaku *camera,
                              int width,
                              int height,
                              g3d_fix fov,
                              g3d_fix near_clip,
                              g3d_fix far_clip)
{
    Blank3DCameraProfile *profile;
    int i;
    if (!camera) return;
    memset(camera, 0, sizeof(*camera));
    camera->width = width > 0 ? width : 640;
    camera->height = height > 0 ? height : 480;
    camera->camera_id = B3D_CNK_CAMERA_ID;
    camera->yaw = 0;
    camera->pitch = 0;
    camera->zoom_fx = GWP89_FIX_ONE;
    camera->target_position = gamlib_vec3(0, 0, 0);
    camera->target_velocity = gamlib_vec3(0, 0, 0);

    blank3d_camera_catalog_init(&camera->catalog);
    for (i = 0; i < camera->catalog.count; ++i) {
        profile = &camera->catalog.profiles[i];
        profile->fov = fov;
        profile->near_clip = near_clip;
        profile->far_clip = far_clip;
    }

    cnk_transform_provider_clear(&camera->transform_provider);
    cnk_transform_provider_set(&camera->transform_provider,
                               camera,
                               b3d_cnk_provider_move,
                               b3d_cnk_provider_scale,
                               b3d_cnk_provider_rotate);
    cnk_transform_provider_set_mode(&camera->transform_provider,
                                    CNK_TRANSFORM_MODE_RECEIVE_PROVIDER);
    cnk_camera_reset(&camera->camera);
    b3d_cnk_apply_current_profile(camera);
    camera->initialized = 1;
    strcpy(camera->status,
           "Cameranaku89 camera INI catalog ready");
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
    Blank3DCameraProfile *profile;
    int i;
    if (!camera) return;
    for (i = 0; i < camera->catalog.count; ++i) {
        profile = &camera->catalog.profiles[i];
        profile->pitch_min = pitch_min;
        profile->pitch_max = pitch_max;
        profile->fov = fov;
        profile->near_clip = near_clip;
        profile->far_clip = far_clip;
        if (profile->rig == B3D_CAMERA_RIG_FPS) {
            profile->pivot_y = eye_height;
        } else {
            profile->pivot_y = camera_height;
            profile->distance = camera_distance;
            if (profile->view_style == GWP89_VIEW_OVER_SHOULDER)
                profile->offset_right = shoulder;
        }
    }
    b3d_cnk_apply_current_profile(camera);
}

int blank3d_cameranaku_load_profiles(Blank3DCameraNaku *camera,
                                     const char *directory)
{
    int result;
    if (!camera || !directory) return 0;
    result = blank3d_camera_catalog_load(&camera->catalog, directory);
    b3d_cnk_apply_current_profile(camera);
    sprintf(camera->status, "CamNaku: %s",
            blank3d_camera_catalog_status(&camera->catalog));
    return result;
}

int blank3d_cameranaku_set_profile_index(Blank3DCameraNaku *camera,
                                         int index)
{
    if (!camera) return 0;
    if (!blank3d_camera_catalog_select_index(&camera->catalog, index))
        return 0;
    b3d_cnk_apply_current_profile(camera);
    return 1;
}

int blank3d_cameranaku_set_profile(Blank3DCameraNaku *camera,
                                   const char *profile_id)
{
    if (!camera || !profile_id) return 0;
    if (!blank3d_camera_catalog_select_id(&camera->catalog, profile_id))
        return 0;
    b3d_cnk_apply_current_profile(camera);
    return 1;
}

int blank3d_cameranaku_next_profile(Blank3DCameraNaku *camera)
{
    if (!camera) return 0;
    if (!blank3d_camera_catalog_next(&camera->catalog)) return 0;
    b3d_cnk_apply_current_profile(camera);
    return 1;
}

int blank3d_cameranaku_profile_count(const Blank3DCameraNaku *camera)
{
    return camera ? camera->catalog.count : 0;
}

int blank3d_cameranaku_profile_index(const Blank3DCameraNaku *camera)
{
    return camera ? camera->catalog.active_index : -1;
}

const Blank3DCameraProfile *blank3d_cameranaku_current_profile(
    const Blank3DCameraNaku *camera)
{
    return b3d_cnk_profile(camera);
}

const Blank3DCameraProfile *blank3d_cameranaku_profile_at(
    const Blank3DCameraNaku *camera,
    int index)
{
    return camera ? blank3d_camera_catalog_at(&camera->catalog, index)
                  : (const Blank3DCameraProfile *)0;
}

void blank3d_cameranaku_set_mode(Blank3DCameraNaku *camera, int mode)
{
    int view_style;
    int index;
    if (!camera) return;
    if (mode == B3D_CNK_CAMERA_FPS)
        view_style = GWP89_VIEW_FPS;
    else if (mode == B3D_CNK_CAMERA_OTS)
        view_style = GWP89_VIEW_OVER_SHOULDER;
    else
        view_style = GWP89_VIEW_THIRD_PERSON;
    index = blank3d_camera_catalog_find_view_style(&camera->catalog,
                                                   view_style);
    if (index >= 0) (void)blank3d_cameranaku_set_profile_index(camera, index);
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
    blank3d_cameranaku_add_look(camera,
        b3d_cnk_feeder_magnitude(magnitude), 0);
}

void blank3d_cameranaku_feed_right(Blank3DCameraNaku *camera,
                                    g3d_fix magnitude)
{
    blank3d_cameranaku_add_look(camera,
        g3d_fix_neg_sat(b3d_cnk_feeder_magnitude(magnitude)), 0);
}

void blank3d_cameranaku_feed_up(Blank3DCameraNaku *camera,
                                 g3d_fix magnitude)
{
    blank3d_cameranaku_add_look(camera, 0,
        g3d_fix_neg_sat(b3d_cnk_feeder_magnitude(magnitude)));
}

void blank3d_cameranaku_feed_down(Blank3DCameraNaku *camera,
                                   g3d_fix magnitude)
{
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
    camera->active_profile.lens.fov_y_deg = b3d_q20_to_cnk(fov);
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
    camera->active_profile.lens.viewport_w = camera->width;
    camera->active_profile.lens.viewport_h = camera->height;
}

void blank3d_cameranaku_update(Blank3DCameraNaku *camera,
                                unsigned short dt_ms)
{
    const Blank3DCameraProfile *profile;
    cnk_vec3 position;
    cnk_vec3 velocity;
    cnk_basis effective_basis;
    cnk_vec3 eye;
    cnk_vec3 delta;
    cnk_vec3 term;
    cnk_fx yaw;
    cnk_fx pitch;
    int ticks;

    if (!camera || !camera->initialized) return;
    profile = b3d_cnk_profile(camera);
    if (!profile) return;
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

    if (profile->rig == B3D_CAMERA_RIG_ORBIT) {
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
    delta = cnk_vec3_make(0, 0, 0);
    if (profile->offset_right != 0) {
        term = cnk_transform_scale_uniform(&camera->transform_provider,
                    effective_basis.right,
                    b3d_q20_to_cnk(profile->offset_right));
        delta = cnk_transform_move(&camera->transform_provider, delta, term);
    }
    if (profile->offset_up != 0) {
        term = cnk_transform_scale_uniform(&camera->transform_provider,
                    effective_basis.up,
                    b3d_q20_to_cnk(profile->offset_up));
        delta = cnk_transform_move(&camera->transform_provider, delta, term);
    }
    if (profile->offset_forward != 0) {
        term = cnk_transform_scale_uniform(&camera->transform_provider,
                    effective_basis.forward,
                    b3d_q20_to_cnk(profile->offset_forward));
        delta = cnk_transform_move(&camera->transform_provider, delta, term);
    }
    eye = cnk_transform_move(&camera->transform_provider, eye, delta);

    camera->eye = b3d_cnk_to_vec3(eye);
    camera->forward = b3d_cnk_to_vec3(effective_basis.forward);
    camera->right = b3d_cnk_to_vec3(effective_basis.right);
    camera->up = b3d_cnk_to_vec3(effective_basis.up);
    sprintf(camera->status,
             "CamNaku [%s] %d/%d | TRS %lu/%lu/%lu | weapon %lu",
             profile->id,
             camera->catalog.active_index + 1,
             camera->catalog.count,
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
