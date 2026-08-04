#include "jump89.h"

#include <string.h>

static int jump89_valid(const jump89_context *context)
{
    return context && context->providers.math.add &&
           context->providers.math.sub && context->providers.math.mul &&
           context->providers.math.abs_value &&
           context->providers.transform.get_position &&
           context->providers.transform.set_position;
}

static vm89_scalar jump89_floor(jump89_context *context,
                                void *actor,
                                vm89_scalar supplied)
{
    vm89_scalar floor_y;
    floor_y = supplied;
    if (context && context->providers.physics.get_floor_y)
        (void)context->providers.physics.get_floor_y(
            context->providers.physics.user, actor, &floor_y);
    return floor_y;
}

static int jump89_grounded(jump89_context *context,
                           void *actor,
                           vm89_scalar floor_y)
{
    vm89_vec3 position;
    int grounded;
    if (context->providers.physics.is_grounded &&
        context->providers.physics.is_grounded(
            context->providers.physics.user, actor, floor_y,
            &grounded)) return grounded != 0;
    if (!context->providers.transform.get_position(
            context->providers.transform.user, actor, &position)) return 0;
    return position.y <= floor_y;
}

static int jump89_commit(jump89_context *context,
                         void *actor,
                         vm89_vec3 *desired,
                         vm89_scalar floor_y)
{
    vm89_vec3 resolved;
    if (desired->y < floor_y) desired->y = floor_y;
    resolved = *desired;
    if (context->providers.physics.move_position &&
        context->providers.physics.move_position(
            context->providers.physics.user, actor, desired, 1,
            floor_y, &resolved)) return 1;
    return context->providers.transform.set_position(
        context->providers.transform.user, actor, desired);
}

void jump89_init(jump89_context *context,
                 const vm89_provider_bundle *providers)
{
    if (!context) return;
    memset(context, 0, sizeof(*context));
    if (providers) context->providers = *providers;
}

void jump89_state_reset(jump89_state *state)
{
    if (!state) return;
    state->active = 0;
    state->velocity_y = 0;
}

int jump89_start(jump89_context *context,
                 jump89_state *state,
                 void *actor,
                 vm89_scalar impulse,
                 vm89_scalar floor_y,
                 int gravity_enabled)
{
    vm89_scalar actual_floor;
    if (!jump89_valid(context) || !state || !actor || !gravity_enabled)
        return 0;
    if (state->active) return 0;
    actual_floor = jump89_floor(context, actor, floor_y);
    if (!jump89_grounded(context, actor, actual_floor)) return 0;
    state->velocity_y = context->providers.math.abs_value(
        context->providers.math.user, impulse);
    if (state->velocity_y <= 0) return 0;
    state->active = 1;
    return 1;
}

int jump89_tick(jump89_context *context,
                jump89_state *state,
                void *actor,
                vm89_scalar gravity_acceleration,
                vm89_scalar dt,
                vm89_scalar floor_y,
                int gravity_enabled)
{
    vm89_vec3 position;
    vm89_scalar actual_floor;
    vm89_scalar gravity_step;
    vm89_scalar movement_step;
    vm89_math_provider *math;
    if (!jump89_valid(context) || !state || !actor || !state->active)
        return 0;
    if (!gravity_enabled || dt <= 0) return 0;
    math = &context->providers.math;
    actual_floor = jump89_floor(context, actor, floor_y);
    gravity_acceleration = math->abs_value(math->user,
                                           gravity_acceleration);
    gravity_step = math->mul(math->user, gravity_acceleration, dt);
    state->velocity_y = math->sub(math->user,
                                  state->velocity_y, gravity_step);
    movement_step = math->mul(math->user, state->velocity_y, dt);
    if (!context->providers.transform.get_position(
            context->providers.transform.user, actor, &position)) return 0;
    position.y = math->add(math->user, position.y, movement_step);
    if (position.y <= actual_floor && state->velocity_y <= 0) {
        position.y = actual_floor;
        (void)jump89_commit(context, actor, &position, actual_floor);
        jump89_state_reset(state);
        return 1;
    }
    (void)jump89_commit(context, actor, &position, actual_floor);
    if (state->velocity_y <= 0 &&
        jump89_grounded(context, actor, actual_floor)) {
        jump89_state_reset(state);
    }
    return 1;
}

int jump89_is_active(const jump89_state *state)
{
    return state && state->active;
}

int jump89_is_falling(const jump89_state *state)
{
    return state && state->active && state->velocity_y < 0;
}
