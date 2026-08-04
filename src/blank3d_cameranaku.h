#ifndef BLANK3D_CAMERANAKU_H
#define BLANK3D_CAMERANAKU_H

#include "gamlib3d_transform.h"
#include "gweapon89.h"
#include "cameranaku89_all.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_CNK_CAMERA_FPS 1
#define B3D_CNK_CAMERA_TPS 2
#define B3D_CNK_CAMERA_ID 34

typedef struct Blank3DCameraNakuTag {
    int initialized;
    int mode;
    int width;
    int height;
    int view_style;
    int camera_id;

    cnk_camera camera;
    cnk_profile fps_profile;
    cnk_profile tps_profile;
    cnk_transform_provider transform_provider;

    g3d_fix yaw;
    g3d_fix pitch;
    g3d_fix pitch_min;
    g3d_fix pitch_max;
    g3d_fix fov;
    g3d_fix sway_yaw;
    g3d_fix sway_pitch;
    g3d_fix shoulder;
    g3d_fix zoom_fx;

    Vec3 target_position;
    Vec3 target_velocity;
    Vec3 eye;
    Vec3 forward;
    Vec3 right;
    Vec3 up;

    unsigned long provider_move_calls;
    unsigned long provider_scale_calls;
    unsigned long provider_rotate_calls;
    unsigned long weapon_provider_calls;
    char status[192];
} Blank3DCameraNaku;

void blank3d_cameranaku_init(Blank3DCameraNaku *camera,
                              int width,
                              int height,
                              g3d_fix fov,
                              g3d_fix near_clip,
                              g3d_fix far_clip);

void blank3d_cameranaku_configure(Blank3DCameraNaku *camera,
                                   g3d_fix eye_height,
                                   g3d_fix camera_height,
                                   g3d_fix camera_distance,
                                   g3d_fix shoulder,
                                   g3d_fix pitch_min,
                                   g3d_fix pitch_max,
                                   g3d_fix fov,
                                   g3d_fix near_clip,
                                   g3d_fix far_clip);

void blank3d_cameranaku_set_mode(Blank3DCameraNaku *camera, int mode);
void blank3d_cameranaku_set_angles(Blank3DCameraNaku *camera,
                                    g3d_fix yaw,
                                    g3d_fix pitch);
void blank3d_cameranaku_add_look(Blank3DCameraNaku *camera,
                                 g3d_fix yaw_delta,
                                 g3d_fix pitch_delta);

/*
 * Four directional feeder bridges.
 *
 * Blank3D mouse semantics are translated here instead of changing Win32
 * mouse deltas or disabling Cameranaku's receive-provider mode.  The current
 * Gamlib3D/Cameranaku axis convention requires each semantic direction to
 * feed the opposite signed camera delta.
 */
void blank3d_cameranaku_feed_left(Blank3DCameraNaku *camera,
                                   g3d_fix magnitude);
void blank3d_cameranaku_feed_right(Blank3DCameraNaku *camera,
                                    g3d_fix magnitude);
void blank3d_cameranaku_feed_up(Blank3DCameraNaku *camera,
                                 g3d_fix magnitude);
void blank3d_cameranaku_feed_down(Blank3DCameraNaku *camera,
                                   g3d_fix magnitude);
void blank3d_cameranaku_set_target(Blank3DCameraNaku *camera,
                                    const Vec3 *position,
                                    const Vec3 *velocity);
void blank3d_cameranaku_set_fov(Blank3DCameraNaku *camera, g3d_fix fov);
void blank3d_cameranaku_set_zoom(Blank3DCameraNaku *camera, g3d_fix zoom_fx);
void blank3d_cameranaku_set_sway(Blank3DCameraNaku *camera,
                                  g3d_fix yaw_offset,
                                  g3d_fix pitch_offset);
void blank3d_cameranaku_add_recoil(Blank3DCameraNaku *camera,
                                    g3d_fix pitch_delta,
                                    g3d_fix shake_amplitude);
void blank3d_cameranaku_resize(Blank3DCameraNaku *camera,
                                int width,
                                int height);
void blank3d_cameranaku_update(Blank3DCameraNaku *camera,
                                unsigned short dt_ms);

void blank3d_cameranaku_get_view(const Blank3DCameraNaku *camera,
                                  Vec3 *eye,
                                  Vec3 *forward,
                                  Vec3 *right,
                                  Vec3 *up);

int blank3d_cameranaku_weapon_provider(void *context,
                                       GWP89_ProviderPacket *packet);
const char *blank3d_cameranaku_status(const Blank3DCameraNaku *camera);

#ifdef __cplusplus
}
#endif

#endif
