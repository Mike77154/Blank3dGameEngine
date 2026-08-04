#include "airdiver89.h"

#include <string.h>

static int airdiver89_valid(const airdiver89_context *context)
{
    return context && context->providers.math.add &&
           context->providers.math.sub && context->providers.math.mul &&
           context->providers.math.abs_value &&
           context->providers.math.length3 &&
           context->providers.math.normalize3 &&
           context->providers.transform.get_position &&
           context->providers.transform.set_position;
}

static int airdiver89_commit(airdiver89_context *context,
                             void *actor,
                             vm89_vec3 *desired,
                             int clamp_to_floor,
                             vm89_scalar floor_y)
{
    vm89_vec3 resolved;
    if (clamp_to_floor && desired->y < floor_y) desired->y = floor_y;
    resolved = *desired;
    if (context->providers.physics.move_position &&
        context->providers.physics.move_position(
            context->providers.physics.user, actor, desired,
            clamp_to_floor, floor_y, &resolved)) return 1;
    return context->providers.transform.set_position(
        context->providers.transform.user, actor, desired);
}

void airdiver89_init(airdiver89_context *context,
                     const vm89_provider_bundle *providers)
{
    if (!context) return;
    memset(context, 0, sizeof(*context));
    if (providers) context->providers = *providers;
}

void airdiver89_state_reset(airdiver89_state *state)
{
    if (!state) return;
    memset(state, 0, sizeof(*state));
}

int airdiver89_save_position(airdiver89_context *context,
                             airdiver89_state *state,
                             void *actor)
{
    if (!airdiver89_valid(context) || !state || !actor) return 0;
    if (!context->providers.transform.get_position(
            context->providers.transform.user, actor,
            &state->return_position)) return 0;
    state->return_position_valid = 1;
    return 1;
}

int airdiver89_move_to_point(airdiver89_context *context,
                             void *actor,
                             const vm89_vec3 *target,
                             vm89_scalar speed,
                             vm89_scalar dt,
                             vm89_scalar floor_y,
                             int clamp_to_floor,
                             int *out_reached)
{
    vm89_vec3 position;
    vm89_vec3 delta;
    vm89_vec3 direction;
    vm89_vec3 movement;
    vm89_scalar distance;
    vm89_scalar step;
    vm89_math_provider *math;
    if (out_reached) *out_reached = 0;
    if (!airdiver89_valid(context) || !actor || !target || dt <= 0)
        return 0;
    math = &context->providers.math;
    speed = math->abs_value(math->user, speed);
    if (!context->providers.transform.get_position(
            context->providers.transform.user, actor, &position)) return 0;
    delta.x = math->sub(math->user, target->x, position.x);
    delta.y = math->sub(math->user, target->y, position.y);
    delta.z = math->sub(math->user, target->z, position.z);
    distance = math->length3(math->user, &delta);
    step = math->mul(math->user, speed, dt);
    if (distance <= math->epsilon || step >= distance) {
        position = *target;
        if (!airdiver89_commit(context, actor, &position,
                               clamp_to_floor, floor_y)) return 0;
        if (out_reached) *out_reached = 1;
        return 1;
    }
    if (!math->normalize3(math->user, &delta, &direction)) return 0;
    movement.x = math->mul(math->user, direction.x, step);
    movement.y = math->mul(math->user, direction.y, step);
    movement.z = math->mul(math->user, direction.z, step);
    position.x = math->add(math->user, position.x, movement.x);
    position.y = math->add(math->user, position.y, movement.y);
    position.z = math->add(math->user, position.z, movement.z);
    return airdiver89_commit(context, actor, &position,
                             clamp_to_floor, floor_y);
}

int airdiver89_descend_to_y(airdiver89_context *context,
                            void *actor,
                            vm89_scalar target_y,
                            vm89_scalar speed,
                            vm89_scalar dt,
                            vm89_scalar floor_y,
                            int *out_reached)
{
    vm89_vec3 target;
    if (!airdiver89_valid(context) || !actor) return 0;
    if (!context->providers.transform.get_position(
            context->providers.transform.user, actor, &target)) return 0;
    if (target_y < floor_y) target_y = floor_y;
    if (target.y <= target_y) {
        target.y = target_y;
        (void)airdiver89_commit(context, actor, &target, 1, floor_y);
        if (out_reached) *out_reached = 1;
        return 1;
    }
    target.y = target_y;
    return airdiver89_move_to_point(context, actor, &target, speed, dt,
                                    floor_y, 1, out_reached);
}

int airdiver89_ram_target(airdiver89_context *context,
                          void *actor,
                          const vm89_vec3 *target,
                          vm89_scalar speed,
                          vm89_scalar dt,
                          vm89_scalar floor_y,
                          int *out_reached)
{
    return airdiver89_move_to_point(context, actor, target, speed, dt,
                                    floor_y, 1, out_reached);
}

int airdiver89_return_saved(airdiver89_context *context,
                            airdiver89_state *state,
                            void *actor,
                            vm89_scalar speed,
                            vm89_scalar dt,
                            vm89_scalar floor_y,
                            int *out_reached)
{
    if (!state || !state->return_position_valid) return 0;
    return airdiver89_move_to_point(context, actor,
                                    &state->return_position,
                                    speed, dt, floor_y, 0, out_reached);
}

int airdiver89_saved_position(const airdiver89_state *state,
                              vm89_vec3 *out_position)
{
    if (!state || !state->return_position_valid || !out_position) return 0;
    *out_position = state->return_position;
    return 1;
}
