/*
    cameranaku89_zones.h
    CC0 1.0 Universal.
    Static camera trigger/volume routing for fixed cameras, room cameras and cutscene handoffs.
*/
#ifndef CAMERANAKU89_ZONES_H
#define CAMERANAKU89_ZONES_H

#include "cameranaku89_manager.h"

#ifdef __cplusplus
extern "C" {
#endif

enum {
    CNK_ZONE_FORCE_CAMERA = 1,
    CNK_ZONE_ENABLE_CAMERA = 2,
    CNK_ZONE_DISABLE_ON_EXIT = 4,
    CNK_ZONE_APPLY_CONFINER = 8,
    CNK_ZONE_USE_BLEND = 16,
    CNK_ZONE_STICKY = 32
};

typedef struct cnk_camera_zone_s {
    int id;
    int vcam_id;
    int flags;
    int priority;
    int priority_bias;
    int force_ticks;
    int blend_ticks;
    cnk_vec3 minv;
    cnk_vec3 maxv;
    cnk_confiner_box confiner;
    int inside;
    int was_inside;
} cnk_camera_zone;

typedef struct cnk_camera_zone_report_s {
    int zone_index;
    int zone_id;
    int vcam_id;
    int entered;
    int exited;
    int inside;
} cnk_camera_zone_report;

typedef struct cnk_camera_zone_bank_s {
    cnk_camera_zone zones[CNK_MAX_ZONES];
    int count;
    int active_zone_index;
} cnk_camera_zone_bank;

CNK_API void cnk_camera_zone_bank_clear(cnk_camera_zone_bank *bank);
CNK_API int cnk_camera_zone_add_box(cnk_camera_zone_bank *bank, int id, int vcam_id, cnk_vec3 minv, cnk_vec3 maxv, int priority, int flags);
CNK_API void cnk_camera_zone_set_bias(cnk_camera_zone_bank *bank, int index, int priority_bias, int force_ticks, int blend_ticks);
CNK_API void cnk_camera_zone_set_confiner(cnk_camera_zone_bank *bank, int index, cnk_vec3 minv, cnk_vec3 maxv);
CNK_API int cnk_camera_zone_contains(const cnk_camera_zone *zone, cnk_vec3 point);
CNK_API cnk_camera_zone_report cnk_camera_zone_bank_update(cnk_camera_zone_bank *bank, cnk_vec3 point);
CNK_API int cnk_camera_zone_bank_apply(cnk_camera_zone_bank *bank, cnk_camera_manager *mgr, cnk_vec3 point);

#ifdef __cplusplus
}
#endif

#endif
