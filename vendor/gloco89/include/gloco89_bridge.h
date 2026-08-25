#ifndef GLOCO89_BRIDGE_H
#define GLOCO89_BRIDGE_H

#include "gloco89.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
   Tiny bridge helpers: feed your renderer/physics/camera with stable data.
   The library never owns your engine objects. You map actor_id -> entity_id.
*/

typedef struct GLOCO_RenderPoseTag {
    GLOCO_Vec3 pos;
    GLOCO_Vec3 facing;
    GLOCO_Vec3 velocity;
    GLOCO_Vec3 ground_normal;
    GLOCO_U16 flags;
    GLOCO_U8 state;
    GLOCO_FX stride_phase;
} GLOCO_RenderPose;

int gloco_bridge_get_pose(const GLOCO_Context *ctx, int actor_id, GLOCO_RenderPose *out_pose);
GLOCO_Input gloco_bridge_make_camera_input(GLOCO_S16 move_x,
                                           GLOCO_S16 move_z,
                                           GLOCO_U16 buttons,
                                           GLOCO_Vec3 cam_fwd,
                                           GLOCO_Vec3 cam_right);
GLOCO_Input gloco_bridge_make_actor_input(GLOCO_S16 move_x,
                                          GLOCO_S16 move_z,
                                          GLOCO_U16 buttons,
                                          GLOCO_Vec3 actor_fwd,
                                          GLOCO_Vec3 actor_right);

#ifdef __cplusplus
}
#endif

#endif
