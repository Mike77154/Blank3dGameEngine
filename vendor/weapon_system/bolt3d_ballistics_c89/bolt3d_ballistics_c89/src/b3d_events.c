#include "bolt3d/b3d_events.h"

void b3d_event_clear(B3D_Event *event_value)
{
    if (event_value == 0) {
        return;
    }

    event_value->type = B3D_EVENT_NONE;
    event_value->projectile_id = B3D_ID_NONE;
    event_value->projectile_slot = B3D_ID_NONE;
    event_value->owner_id = B3D_ID_NONE;
    event_value->target_id = B3D_ID_NONE;
    event_value->target_slot = B3D_ID_NONE;
    event_value->target_layer = 0;
    event_value->target_user_type = 0;
    event_value->damage_type = B3D_DAMAGE_GENERIC;
    event_value->flags = 0;
    event_value->motion_mode = B3D_MOTION_BALLISTIC;
    event_value->response = B3D_RESPONSE_IGNORE;
    event_value->iteration = 0;
    event_value->binding_id = B3D_ID_NONE;
    event_value->damage = 0;
    event_value->stun = 0;
    event_value->knockback = 0;
    event_value->t = 0;
    event_value->position = b3d_vec3_zero();
    event_value->previous_position = b3d_vec3_zero();
    event_value->normal = b3d_vec3_zero();
    event_value->velocity = b3d_vec3_zero();
    event_value->previous_transform = b3d_transform_identity();
    event_value->desired_transform = b3d_transform_identity();
    event_value->solved_transform = b3d_transform_identity();
    event_value->source_user_ptr = 0;
    event_value->target_user_ptr = 0;
    event_value->user_ptr = 0;
}
