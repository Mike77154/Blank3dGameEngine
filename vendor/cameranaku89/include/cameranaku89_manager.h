/*
    cameranaku89_manager.h
    CC0 1.0 Universal.
    Static virtual-camera manager that emits the final camera state.
*/
#ifndef CAMERANAKU89_MANAGER_H
#define CAMERANAKU89_MANAGER_H

#include "cameranaku89_virtual.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cnk_camera_manager_s {
    cnk_virtual_camera vcams[CNK_MAX_MANAGER_VCAMS];
    int used[CNK_MAX_MANAGER_VCAMS];
    int active_index;
    int previous_index;
    int default_blend_ticks;
    int blend_curve;
    int blend_active;
    int blend_time_ticks;
    int blend_duration_ticks;
    int live_ticks[CNK_MAX_MANAGER_VCAMS];
    int forced_vcam_id;
    int force_ticks;
    cnk_fx switch_quality_margin;
    cnk_state blend_from;
    cnk_state blend_to;
    cnk_state final_state;
    cnk_u32 frame_counter;
    cnk_transform_provider transform_provider;
} cnk_camera_manager;

CNK_API void cnk_camera_manager_init(cnk_camera_manager *mgr);
CNK_API cnk_virtual_camera *cnk_camera_manager_alloc(cnk_camera_manager *mgr, int id);
CNK_API cnk_virtual_camera *cnk_camera_manager_get(cnk_camera_manager *mgr, int index);
CNK_API cnk_virtual_camera *cnk_camera_manager_find_by_id(cnk_camera_manager *mgr, int id);
CNK_API int cnk_camera_manager_index_by_id(cnk_camera_manager *mgr, int id);
CNK_API int cnk_camera_manager_best_index(cnk_camera_manager *mgr);
CNK_API void cnk_camera_manager_set_switch_policy(cnk_camera_manager *mgr, int blend_ticks, int curve, cnk_fx quality_margin);
CNK_API int cnk_camera_manager_force_by_id(cnk_camera_manager *mgr, int id, int hold_ticks);
CNK_API void cnk_camera_manager_clear_force(cnk_camera_manager *mgr);
CNK_API int cnk_camera_manager_set_vcam_state(cnk_camera_manager *mgr, int id, int state);
CNK_API void cnk_camera_manager_clear_priority_biases(cnk_camera_manager *mgr);
CNK_API void cnk_camera_manager_set_target_all(cnk_camera_manager *mgr, cnk_target target);
CNK_API void cnk_camera_manager_set_transform_provider_all(cnk_camera_manager *mgr, const cnk_transform_provider *provider);
CNK_API void cnk_camera_manager_update(cnk_camera_manager *mgr, int dt_ticks, cnk_world_probe_fn probe, void *probe_user);
CNK_API cnk_state cnk_camera_manager_final_state(const cnk_camera_manager *mgr);
CNK_API void cnk_camera_manager_emit_to_camera(const cnk_camera_manager *mgr, cnk_camera *cam);

#ifdef __cplusplus
}
#endif

#endif
