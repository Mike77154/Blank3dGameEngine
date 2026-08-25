#include <stdio.h>
#include "3d_movementbaseverbs89.h"

/* Example adapter owned by a hypothetical engine. */
typedef struct engine_body {
    mbv89_fixed x;
    mbv89_fixed y;
    mbv89_fixed z;
} engine_body;

static int engine_transform_provider(void *user, mbv89_context *ctx,
                                     mbv89_actor *actor,
                                     mbv89_base_verb verb,
                                     mbv89_fixed amount)
{
    engine_body *body;
    (void)ctx;
    (void)actor;

    body = (engine_body *)user;

    switch (verb) {
        case MBV89_BASE_MOVE_FORWARD:
            body->z += amount;
            return MBV89_HANDLED;
        case MBV89_BASE_MOVE_BACKWARD:
            body->z -= amount;
            return MBV89_HANDLED;
        case MBV89_BASE_MOVE_LEFT:
            body->x -= amount;
            return MBV89_HANDLED;
        case MBV89_BASE_MOVE_RIGHT:
            body->x += amount;
            return MBV89_HANDLED;
        case MBV89_BASE_MOVE_UP:
            body->y += amount;
            return MBV89_HANDLED;
        case MBV89_BASE_MOVE_DOWN:
            body->y -= amount;
            return MBV89_HANDLED;
        default:
            return MBV89_UNHANDLED;
    }
}

int main(void)
{
    mbv89_context verbs;
    mbv89_actor actor;
    engine_body body;

    mbv89_context_init(&verbs);
    mbv89_actor_init(&actor);

    body.x = 0;
    body.y = 0;
    body.z = 0;

    mbv89_set_base_provider(&verbs, engine_transform_provider, &body);

    mbv89_walk_forward(&verbs, &actor);
    mbv89_strafe_right(&verbs, &actor);
    mbv89_run_forward(&verbs, &actor);

    printf("engine body Q16.16: x=%d y=%d z=%d\n",
           (int)body.x, (int)body.y, (int)body.z);

    return 0;
}
