#include <stdio.h>
#include "gloco89.h"
#include "gloco89_profiles.h"
#include "gloco89_bridge.h"

/*
   Provider demo. Nothing here is required by gloco89 itself.
   It shows how a host movement system and a host physics system can take over
   selected stages while the legacy callbacks remain available as fallback.
*/

static int host_horizontal(void *user,
                           int actor_id,
                           GLOCO_Actor *actor,
                           const GLOCO_Profile *profile,
                           const GLOCO_Input *input,
                           const GLOCO_MotionIntent *intent,
                           GLOCO_U16 dt_ms)
{
    (void)user;
    (void)actor_id;
    (void)profile;
    (void)input;
    (void)dt_ms;

    if (intent->mode == GLOCO_MOTION_MODE_MOVE) {
        actor->vel.x = intent->target_velocity.x;
        actor->vel.z = intent->target_velocity.z;
        if (intent->has_input) actor->facing = intent->desired_dir;
    } else if (intent->mode == GLOCO_MOTION_MODE_BRAKE ||
               intent->mode == GLOCO_MOTION_MODE_HARD_STOP) {
        actor->vel.x = 0;
        actor->vel.z = 0;
    } else if (intent->mode == GLOCO_MOTION_MODE_EVADE) {
        if (intent->has_input) {
            actor->vel.x = intent->target_velocity.x;
            actor->vel.z = intent->target_velocity.z;
        }
    } else if (intent->mode == GLOCO_MOTION_MODE_SLIDE) {
        /* Preserve the current slide velocity in this tiny demo. */
    }

    return GLOCO_PROVIDER_HANDLED;
}

static int host_integrate(void *user,
                          int actor_id,
                          const GLOCO_Actor *actor,
                          GLOCO_U16 dt_ms,
                          const GLOCO_Vec3 *from,
                          const GLOCO_Vec3 *builtin_to,
                          GLOCO_Vec3 *out_to)
{
    (void)user;
    (void)actor_id;
    (void)actor;
    (void)dt_ms;
    (void)from;
    *out_to = *builtin_to;
    return GLOCO_PROVIDER_HANDLED;
}

static int host_impulse(void *user,
                        int actor_id,
                        GLOCO_Actor *actor,
                        int impulse_id,
                        const GLOCO_Vec3 *builtin_velocity)
{
    (void)user;
    (void)actor_id;
    if (impulse_id != GLOCO_IMPULSE_EVADE) return GLOCO_PROVIDER_FALLBACK;
    actor->vel = *builtin_velocity;
    return GLOCO_PROVIDER_HANDLED;
}

static int host_physics_move(void *user,
                             int actor_id,
                             GLOCO_Actor *actor,
                             const GLOCO_Profile *profile,
                             GLOCO_U16 dt_ms,
                             const GLOCO_Vec3 *from,
                             const GLOCO_Vec3 *to,
                             GLOCO_ProbeResult *out_result)
{
    (void)user;
    (void)actor_id;
    (void)actor;
    (void)profile;
    (void)dt_ms;
    (void)from;

    out_result->corrected_pos = *to;
    out_result->ground_normal = gloco_v3(0, GLOCO_FX_ONE, 0);
    out_result->flags = 0;

    if (out_result->corrected_pos.y <= 0) {
        out_result->corrected_pos.y = 0;
        out_result->flags |= GLOCO_FLAG_GROUNDED;
    }
    return GLOCO_PROVIDER_HANDLED;
}

static void host_physics_create(void *user,
                                int actor_id,
                                const GLOCO_Actor *actor,
                                const GLOCO_Profile *profile)
{
    int *counter;
    (void)actor_id;
    (void)actor;
    (void)profile;
    counter = (int *)user;
    if (counter) *counter += 1;
}

int main(void)
{
    GLOCO_Context ctx;
    GLOCO_MovementProvider movement;
    GLOCO_PhysicsProvider physics;
    GLOCO_Input in;
    GLOCO_Vec3 pos;
    GLOCO_Vec3 fwd;
    GLOCO_Vec3 right;
    const GLOCO_Actor *actor_ptr;
    int actor;
    int physics_create_count;
    int i;

    physics_create_count = 0;
    gloco_init(&ctx);
    gloco_load_default_profile_bank(&ctx);

    gloco_movement_provider_init(&movement);
    movement.horizontal = host_horizontal;
    movement.integrate = host_integrate;
    movement.impulse = host_impulse;
    gloco_set_movement_provider(&ctx, &movement);

    gloco_physics_provider_init(&physics);
    physics.user = &physics_create_count;
    physics.move = host_physics_move;
    physics.actor_create = host_physics_create;
    gloco_set_physics_provider(&ctx, &physics);

    pos = gloco_v3(0, 0, 0);
    fwd = gloco_v3(0, 0, GLOCO_FX_ONE);
    right = gloco_v3(GLOCO_FX_ONE, 0, 0);
    actor = gloco_actor_create(&ctx, GLOCO_PROFILE_TACTICAL, &pos, &fwd);

    in = gloco_bridge_make_camera_input(0, 256, GLOCO_INPUT_RUN, fwd, right);
    for (i = 0; i < 10; ++i) gloco_update_actor(&ctx, actor, &in, 16);

    actor_ptr = gloco_actor_get_const(&ctx, actor);
    if (!actor_ptr) return 1;

    printf("provider_demo actor=%d create_count=%d state=%s pos_z=%ld vel_z=%ld\n",
           actor,
           physics_create_count,
           gloco_state_name(actor_ptr->state),
           actor_ptr->pos.z,
           actor_ptr->vel.z);

    if (physics_create_count != 1) return 2;
    if (actor_ptr->pos.z <= 0) return 3;
    if (actor_ptr->vel.z <= 0) return 4;
    return 0;
}
