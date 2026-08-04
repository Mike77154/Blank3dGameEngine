#ifndef B3D_EVENTS_H
#define B3D_EVENTS_H

#include "bolt3d/b3d_solver.h"

#ifdef __cplusplus
extern "C" {
#endif

#define B3D_EVENT_NONE 0
#define B3D_EVENT_SPAWN 1
#define B3D_EVENT_MOVE 2
#define B3D_EVENT_HIT_ENTITY 3
#define B3D_EVENT_HIT_WORLD 4
#define B3D_EVENT_DAMAGE 5
#define B3D_EVENT_EXPLODE 6
#define B3D_EVENT_BOUNCE 7
#define B3D_EVENT_EXPIRE 8
#define B3D_EVENT_DESTROY 9
#define B3D_EVENT_HITSCAN 10
#define B3D_EVENT_QUEUE_OVERFLOW 11
#define B3D_EVENT_SOLVER_BEGIN 12
#define B3D_EVENT_TRANSFORM_READ 13
#define B3D_EVENT_CONTACT 14
#define B3D_EVENT_OVERLAP 15
#define B3D_EVENT_BLOCKED 16
#define B3D_EVENT_SLIDE 17
#define B3D_EVENT_STICK 18
#define B3D_EVENT_TRANSFORM_SOLVED 19
#define B3D_EVENT_TRANSFORM_WRITE 20
#define B3D_EVENT_SOLVER_FAILED 21

#define B3D_DAMAGE_GENERIC 0
#define B3D_DAMAGE_BULLET 1
#define B3D_DAMAGE_SLASH 2
#define B3D_DAMAGE_PIERCE 3
#define B3D_DAMAGE_EXPLOSION 4
#define B3D_DAMAGE_FIRE 5
#define B3D_DAMAGE_ACID 6
#define B3D_DAMAGE_BLUNT 7
#define B3D_DAMAGE_CUSTOM 1000

typedef struct B3D_Event {
    int type;
    int projectile_id;
    int projectile_slot;
    int owner_id;
    int target_id;
    int target_slot;
    int target_layer;
    int target_user_type;
    int damage_type;
    int flags;
    int motion_mode;
    int response;
    int iteration;
    int binding_id;
    B3D_Fixed damage;
    B3D_Fixed stun;
    B3D_Fixed knockback;
    B3D_Fixed t;
    B3D_Vec3 position;
    B3D_Vec3 previous_position;
    B3D_Vec3 normal;
    B3D_Vec3 velocity;
    B3D_Transform previous_transform;
    B3D_Transform desired_transform;
    B3D_Transform solved_transform;
    void *source_user_ptr;
    void *target_user_ptr;
    void *user_ptr;
} B3D_Event;

B3D_API void b3d_event_clear(B3D_Event *event_value);

#ifdef __cplusplus
}
#endif

#endif
