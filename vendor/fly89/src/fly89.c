#include "fly89.h"

#include <string.h>

static int fly89_valid(const fly89_context *context)
{
    return context && context->providers.math.add &&
           context->providers.math.sub && context->providers.math.mul &&
           context->providers.math.abs_value &&
           context->providers.transform.get_position &&
           context->providers.transform.set_position;
}

static int fly89_get_floor(fly89_context *context, void *actor,
                           vm89_scalar supplied_floor,
                           vm89_scalar *out_floor)
{
    vm89_scalar provider_floor;
    if (!out_floor) return 0;
    *out_floor = supplied_floor;
    if (context && context->providers.physics.get_floor_y &&
        context->providers.physics.get_floor_y(
            context->providers.physics.user, actor, &provider_floor)) {
        *out_floor = provider_floor;
    }
    return 1;
}

static int fly89_commit(fly89_context *context, void *actor,
                        const vm89_vec3 *desired,
                        int clamp_to_floor,
                        vm89_scalar floor_y,
                        vm89_vec3 *out_resolved)
{
    vm89_vec3 resolved;
    if (!context || !desired) return 0;
    resolved = *desired;
    if (clamp_to_floor && resolved.y < floor_y) resolved.y = floor_y;
    if (context->providers.physics.move_position &&
        context->providers.physics.move_position(
            context->providers.physics.user, actor, &resolved,
            clamp_to_floor, floor_y, &resolved)) {
        if (out_resolved) *out_resolved = resolved;
        return 1;
    }
    if (!context->providers.transform.set_position(
            context->providers.transform.user, actor, &resolved)) return 0;
    if (out_resolved) *out_resolved = resolved;
    return 1;
}

void fly89_init(fly89_context *context,
                const vm89_provider_bundle *providers)
{
    if (!context) return;
    memset(context, 0, sizeof(*context));
    if (providers) context->providers = *providers;
}

int fly89_move_vertical(fly89_context *context,
                        void *actor,
                        vm89_scalar speed,
                        vm89_scalar dt,
                        int direction,
                        vm89_scalar floor_y)
{
    vm89_vec3 position;
    vm89_scalar step;
    vm89_scalar actual_floor;
    vm89_math_provider *math;
    if (!fly89_valid(context) || !actor || direction == 0 || dt <= 0)
        return 0;
    math = &context->providers.math;
    speed = math->abs_value(math->user, speed);
    step = math->mul(math->user, speed, dt);
    if (!context->providers.transform.get_position(
            context->providers.transform.user, actor, &position)) return 0;
    (void)fly89_get_floor(context, actor, floor_y, &actual_floor);
    if (direction < 0)
        position.y = math->sub(math->user, position.y, step);
    else
        position.y = math->add(math->user, position.y, step);
    return fly89_commit(context, actor, &position,
                        direction < 0, actual_floor, 0);
}

int fly89_move_to_y(fly89_context *context,
                    void *actor,
                    vm89_scalar target_y,
                    vm89_scalar speed,
                    vm89_scalar dt,
                    vm89_scalar floor_y,
                    int *out_reached)
{
    vm89_vec3 position;
    vm89_scalar delta;
    vm89_scalar step;
    vm89_scalar actual_floor;
    vm89_math_provider *math;
    int reached;
    if (out_reached) *out_reached = 0;
    if (!fly89_valid(context) || !actor || dt <= 0) return 0;
    math = &context->providers.math;
    speed = math->abs_value(math->user, speed);
    if (!context->providers.transform.get_position(
            context->providers.transform.user, actor, &position)) return 0;
    (void)fly89_get_floor(context, actor, floor_y, &actual_floor);
    if (target_y < actual_floor) target_y = actual_floor;
    delta = math->sub(math->user, target_y, position.y);
    step = math->mul(math->user, speed, dt);
    reached = math->abs_value(math->user, delta) <= step;
    if (reached) {
        position.y = target_y;
    } else if (delta < 0) {
        position.y = math->sub(math->user, position.y, step);
    } else {
        position.y = math->add(math->user, position.y, step);
    }
    if (!fly89_commit(context, actor, &position, 1, actual_floor, 0))
        return 0;
    if (out_reached) *out_reached = reached;
    return 1;
}

int fly89_grounded(fly89_context *context,
                   void *actor,
                   vm89_scalar floor_y)
{
    vm89_vec3 position;
    vm89_scalar actual_floor;
    int grounded;
    if (!fly89_valid(context) || !actor) return 0;
    (void)fly89_get_floor(context, actor, floor_y, &actual_floor);
    if (context->providers.physics.is_grounded &&
        context->providers.physics.is_grounded(
            context->providers.physics.user, actor, actual_floor,
            &grounded)) return grounded != 0;
    if (!context->providers.transform.get_position(
            context->providers.transform.user, actor, &position)) return 0;
    return position.y <= actual_floor;
}

vm89_scalar fly89_height(fly89_context *context,
                         void *actor,
                         vm89_scalar floor_y)
{
    vm89_vec3 position;
    vm89_scalar actual_floor;
    if (!fly89_valid(context) || !actor) return 0;
    (void)fly89_get_floor(context, actor, floor_y, &actual_floor);
    if (!context->providers.transform.get_position(
            context->providers.transform.user, actor, &position)) return 0;
    if (position.y <= actual_floor) return 0;
    return context->providers.math.sub(context->providers.math.user,
                                       position.y, actual_floor);
}
