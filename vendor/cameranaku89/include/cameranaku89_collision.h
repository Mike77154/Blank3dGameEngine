/*
    cameranaku89_collision.h
    CC0 1.0 Universal.
    Optional collision helpers: decollider, deoccluder, confiner and fallback strategies.
*/
#ifndef CAMERANAKU89_COLLISION_H
#define CAMERANAKU89_COLLISION_H

#include "cameranaku89.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CNK_COLLISION_NONE = 0,
    CNK_COLLISION_PULL_FORWARD = 1,
    CNK_COLLISION_PRESERVE_HEIGHT = 2,
    CNK_COLLISION_SLIDE = 3,
    CNK_COLLISION_SHOULDER_SWAP = 4,
    CNK_COLLISION_CUT_TO_BACKUP = 5
};

enum {
    CNK_COLLISION_FLAG_DECOLLIDER = 1,
    CNK_COLLISION_FLAG_DEOCCLUDER = 2,
    CNK_COLLISION_FLAG_CONFINER_BOX = 4
};

typedef struct cnk_confiner_box_s {
    cnk_vec3 minv;
    cnk_vec3 maxv;
    int enabled;
} cnk_confiner_box;

typedef struct cnk_collision_settings_s {
    int flags;
    int strategy;
    cnk_fx probe_radius;
    cnk_fx skin;
    cnk_fx slide_step;
    cnk_fx shoulder_swap_distance;
    cnk_vec3 backup_offset;
    cnk_confiner_box box;
} cnk_collision_settings;

CNK_API void cnk_collision_default(cnk_collision_settings *settings);
CNK_API void cnk_collision_set_strategy(cnk_collision_settings *settings, int strategy);
CNK_API void cnk_collision_set_confiner_box(cnk_collision_settings *settings, cnk_vec3 minv, cnk_vec3 maxv);
CNK_API void cnk_collision_disable_confiner(cnk_collision_settings *settings);
CNK_API void cnk_collision_set_fallback(cnk_collision_settings *settings, cnk_vec3 backup_offset, cnk_fx shoulder_swap_distance, cnk_fx slide_step);
CNK_API int cnk_collision_resolve_provider(cnk_state *state, const cnk_collision_settings *settings, cnk_world_probe_fn probe, void *probe_user, const cnk_transform_provider *provider);
CNK_API int cnk_collision_resolve(cnk_state *state, const cnk_collision_settings *settings, cnk_world_probe_fn probe, void *probe_user);
CNK_API int cnk_collision_line_obstructed(const cnk_state *state, cnk_fx radius, cnk_world_probe_fn probe, void *probe_user);

#ifdef __cplusplus
}
#endif

#endif
