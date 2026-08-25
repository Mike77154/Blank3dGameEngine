#include "gloco89_bridge.h"

int gloco_bridge_get_pose(const GLOCO_Context *ctx, int actor_id, GLOCO_RenderPose *out_pose)
{
    const GLOCO_Actor *a;
    if (!out_pose) return GLOCO_ERR_BAD_ID;
    a = gloco_actor_get_const(ctx, actor_id);
    if (!a) return GLOCO_ERR_BAD_ID;
    out_pose->pos = a->pos;
    out_pose->facing = a->facing;
    out_pose->velocity = a->vel;
    out_pose->ground_normal = a->ground_normal;
    out_pose->flags = a->flags;
    out_pose->state = a->state;
    out_pose->stride_phase = a->stride_phase;
    return GLOCO_OK;
}

GLOCO_Input gloco_bridge_make_camera_input(GLOCO_S16 move_x,
                                           GLOCO_S16 move_z,
                                           GLOCO_U16 buttons,
                                           GLOCO_Vec3 cam_fwd,
                                           GLOCO_Vec3 cam_right)
{
    GLOCO_Input in;
    in.move_x = move_x;
    in.move_z = move_z;
    in.evade_x = 0;
    in.evade_z = 0;
    in.buttons = buttons;
    in.speed_override = 0;
    in.basis_fwd = cam_fwd;
    in.basis_right = cam_right;
    return in;
}

GLOCO_Input gloco_bridge_make_actor_input(GLOCO_S16 move_x,
                                          GLOCO_S16 move_z,
                                          GLOCO_U16 buttons,
                                          GLOCO_Vec3 actor_fwd,
                                          GLOCO_Vec3 actor_right)
{
    return gloco_bridge_make_camera_input(move_x, move_z, buttons, actor_fwd, actor_right);
}
