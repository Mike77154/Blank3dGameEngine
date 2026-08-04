/*
    cameranaku89_virtual.h
    CC0 1.0 Universal.
    Virtual cameras with priority and shot-quality scoring.
*/
#ifndef CAMERANAKU89_VIRTUAL_H
#define CAMERANAKU89_VIRTUAL_H

#include "cameranaku89.h"
#include "cameranaku89_collision.h"
#include "cameranaku89_spring_arm.h"
#include "cameranaku89_target_group.h"
#include "cameranaku89_composer.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CNK_VCAM_DISABLED = 0,
    CNK_VCAM_STANDBY = 1,
    CNK_VCAM_LIVE = 2
};

typedef struct cnk_shot_report_s {
    cnk_fx quality;
    cnk_fx distance_error;
    cnk_fx center_error_x;
    cnk_fx center_error_y;
    int target_visible;
    int target_on_screen;
    int obstructed;
} cnk_shot_report;

typedef struct cnk_virtual_camera_s {
    int id;
    int state;
    int priority;
    int priority_bias;
    int min_live_ticks;
    int active_ticks;
    cnk_camera camera;
    cnk_collision_settings collision;
    cnk_spring_arm spring_arm;
    cnk_shot_report shot;
    cnk_composer composer;
    cnk_target_group *target_group;
    int use_spring_arm;
    int use_collision_ext;
    int use_composer;
} cnk_virtual_camera;

CNK_API void cnk_virtual_camera_init(cnk_virtual_camera *vcam, int id);
CNK_API void cnk_virtual_camera_set_profile(cnk_virtual_camera *vcam, const cnk_profile *profile);
CNK_API void cnk_virtual_camera_set_target(cnk_virtual_camera *vcam, cnk_target target);
CNK_API void cnk_virtual_camera_set_transform_provider(cnk_virtual_camera *vcam, const cnk_transform_provider *provider);
CNK_API void cnk_virtual_camera_set_target_group(cnk_virtual_camera *vcam, cnk_target_group *group);
CNK_API void cnk_virtual_camera_set_priority(cnk_virtual_camera *vcam, int priority);
CNK_API void cnk_virtual_camera_set_priority_bias(cnk_virtual_camera *vcam, int priority_bias);
CNK_API void cnk_virtual_camera_enable_spring_arm(cnk_virtual_camera *vcam, int enabled);
CNK_API void cnk_virtual_camera_enable_collision_ext(cnk_virtual_camera *vcam, int enabled);
CNK_API void cnk_virtual_camera_enable_composer(cnk_virtual_camera *vcam, int enabled);
CNK_API void cnk_virtual_camera_set_composer(cnk_virtual_camera *vcam, const cnk_composer *composer);
CNK_API void cnk_virtual_camera_update(cnk_virtual_camera *vcam, int dt_ticks, cnk_world_probe_fn probe, void *probe_user);
CNK_API cnk_shot_report cnk_virtual_camera_score(cnk_virtual_camera *vcam, cnk_world_probe_fn probe, void *probe_user);

#ifdef __cplusplus
}
#endif

#endif
