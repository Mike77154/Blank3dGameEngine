/*
    cameranaku89_adapter.h
    CC0 1.0 Universal.
    Optional engine bridge. The core solver does not depend on entities, physics, or renderer.
*/
#ifndef CAMERANAKU89_ADAPTER_H
#define CAMERANAKU89_ADAPTER_H

#include "cameranaku89.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct cnk_engine_pose_s {
    cnk_vec3 pos;
    cnk_vec3 velocity;
    cnk_fx yaw_deg;
    cnk_fx pitch_deg;
    cnk_fx roll_deg;
} cnk_engine_pose;

typedef int (*cnk_read_target_fn)(void *user, int target_id, cnk_engine_pose *out_pose);
typedef void (*cnk_write_camera_fn)(void *user, const cnk_state *state, const cnk_mat4 *view, const cnk_mat4 *projection);

typedef struct cnk_engine_adapter_s {
    void *user;
    cnk_read_target_fn read_target;
    cnk_write_camera_fn write_camera;
    cnk_world_probe_fn probe_world;
} cnk_engine_adapter;

typedef struct cnk_solver_bind_s {
    cnk_camera *camera;
    int target_id;
    int enabled;
} cnk_solver_bind;

CNK_API void cnk_adapter_receive_transform_provider(cnk_solver_bind *bind, const cnk_transform_provider *provider);
CNK_API void cnk_adapter_update_bind(cnk_solver_bind *bind, cnk_engine_adapter *adapter, int dt_ticks);
CNK_API void cnk_adapter_emit_camera(cnk_camera *camera, cnk_engine_adapter *adapter);

#ifdef __cplusplus
}
#endif

#endif
