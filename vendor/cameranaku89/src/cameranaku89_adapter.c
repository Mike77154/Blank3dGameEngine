/* cameranaku89_adapter.c - CC0 1.0 Universal */
#include "cameranaku89_adapter.h"

CNK_API void cnk_adapter_receive_transform_provider(cnk_solver_bind *bind, const cnk_transform_provider *provider)
{
    if (bind == 0 || bind->camera == 0) {
        return;
    }
    cnk_camera_set_transform_provider(bind->camera, provider);
}

CNK_API void cnk_adapter_update_bind(cnk_solver_bind *bind, cnk_engine_adapter *adapter, int dt_ticks)
{
    cnk_engine_pose pose;
    int ok;
    if (bind == 0 || adapter == 0 || bind->camera == 0) {
        return;
    }
    if (bind->enabled == 0) {
        return;
    }
    if (adapter->read_target != 0) {
        ok = adapter->read_target(adapter->user, bind->target_id, &pose);
        if (ok != 0) {
            cnk_camera_set_target(bind->camera, pose.pos, pose.velocity, pose.yaw_deg, pose.pitch_deg, pose.roll_deg);
        }
    }
    cnk_camera_update(bind->camera, dt_ticks, adapter->probe_world, adapter->user);
    cnk_adapter_emit_camera(bind->camera, adapter);
}

CNK_API void cnk_adapter_emit_camera(cnk_camera *camera, cnk_engine_adapter *adapter)
{
    cnk_mat4 view;
    cnk_mat4 projection;
    if (camera == 0 || adapter == 0) {
        return;
    }
    if (adapter->write_camera == 0) {
        return;
    }
    view = cnk_camera_view_matrix(camera);
    projection = cnk_camera_projection_matrix(camera);
    adapter->write_camera(adapter->user, &camera->state, &view, &projection);
}
