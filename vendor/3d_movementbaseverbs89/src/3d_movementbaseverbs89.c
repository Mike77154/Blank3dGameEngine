#include "3d_movementbaseverbs89.h"

#include <string.h>

static void mbv89_event_base(mbv89_context *ctx, mbv89_base_verb verb, int entering)
{
    if (ctx != 0 && ctx->event_provider.notify != 0) {
        ctx->event_provider.notify(ctx->event_provider.user, 0, (int)verb, entering);
    }
}

static void mbv89_event_game(mbv89_context *ctx, mbv89_game_verb verb, int entering)
{
    if (ctx != 0 && ctx->event_provider.notify != 0) {
        ctx->event_provider.notify(ctx->event_provider.user, 1, (int)verb, entering);
    }
}

void mbv89_actor_init(mbv89_actor *actor)
{
    if (actor == 0) {
        return;
    }
    actor->position.x = 0;
    actor->position.y = 0;
    actor->position.z = 0;
    actor->pitch = 0;
    actor->yaw = 0;
    actor->roll = 0;
    actor->user = 0;
}

void mbv89_context_init(mbv89_context *ctx)
{
    if (ctx == 0) {
        return;
    }
    ctx->base_provider.perform = 0;
    ctx->base_provider.user = 0;
    ctx->game_provider.perform = 0;
    ctx->game_provider.user = 0;
    ctx->event_provider.notify = 0;
    ctx->event_provider.user = 0;

    ctx->walk_step = MBV89_FIXED_ONE;
    ctx->strafe_step = MBV89_FIXED_ONE;
    ctx->run_step = MBV89_FIXED_ONE * 2;
    ctx->fly_step = MBV89_FIXED_ONE;
    ctx->turn_step = 1024; /* 1/64 of a complete turn. */
}

void mbv89_set_base_provider(mbv89_context *ctx, mbv89_base_provider_fn fn, void *user)
{
    if (ctx == 0) {
        return;
    }
    ctx->base_provider.perform = fn;
    ctx->base_provider.user = user;
}

void mbv89_set_game_provider(mbv89_context *ctx, mbv89_game_provider_fn fn, void *user)
{
    if (ctx == 0) {
        return;
    }
    ctx->game_provider.perform = fn;
    ctx->game_provider.user = user;
}

void mbv89_set_event_provider(mbv89_context *ctx, mbv89_event_fn fn, void *user)
{
    if (ctx == 0) {
        return;
    }
    ctx->event_provider.notify = fn;
    ctx->event_provider.user = user;
}

static int mbv89_base_dispatch(mbv89_context *ctx, mbv89_actor *actor,
                               mbv89_base_verb verb, mbv89_fixed amount)
{
    int handled;

    if (ctx == 0 || actor == 0) {
        return MBV89_ERROR;
    }

    mbv89_event_base(ctx, verb, 1);

    handled = MBV89_UNHANDLED;
    if (ctx->base_provider.perform != 0) {
        handled = ctx->base_provider.perform(
            ctx->base_provider.user, ctx, actor, verb, amount
        );
    }

    if (handled == MBV89_HANDLED) {
        mbv89_event_base(ctx, verb, 0);
        return MBV89_OK;
    }
    if (handled < 0) {
        mbv89_event_base(ctx, verb, 0);
        return MBV89_ERROR;
    }

    switch (verb) {
        case MBV89_BASE_MOVE_FORWARD:
            actor->position.z += amount;
            break;
        case MBV89_BASE_MOVE_BACKWARD:
            actor->position.z -= amount;
            break;
        case MBV89_BASE_MOVE_LEFT:
            actor->position.x -= amount;
            break;
        case MBV89_BASE_MOVE_RIGHT:
            actor->position.x += amount;
            break;
        case MBV89_BASE_MOVE_UP:
            actor->position.y += amount;
            break;
        case MBV89_BASE_MOVE_DOWN:
            actor->position.y -= amount;
            break;

        /*
         * Fallback-only convention:
         * forward/backward -> pitch, left/right -> yaw, up/down -> roll.
         * A real engine is expected to provide its own transform convention.
         */
        case MBV89_BASE_ROTATE_FORWARD:
            actor->pitch += amount;
            break;
        case MBV89_BASE_ROTATE_BACKWARD:
            actor->pitch -= amount;
            break;
        case MBV89_BASE_ROTATE_LEFT:
            actor->yaw += amount;
            break;
        case MBV89_BASE_ROTATE_RIGHT:
            actor->yaw -= amount;
            break;
        case MBV89_BASE_ROTATE_UP:
            actor->roll += amount;
            break;
        case MBV89_BASE_ROTATE_DOWN:
            actor->roll -= amount;
            break;
        default:
            mbv89_event_base(ctx, verb, 0);
            return MBV89_ERROR;
    }

    mbv89_event_base(ctx, verb, 0);
    return MBV89_OK;
}

int mbv89_move_forward(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_MOVE_FORWARD, amount);
}

int mbv89_move_backward(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_MOVE_BACKWARD, amount);
}

int mbv89_move_left(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_MOVE_LEFT, amount);
}

int mbv89_move_right(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_MOVE_RIGHT, amount);
}

int mbv89_move_up(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_MOVE_UP, amount);
}

int mbv89_move_down(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_MOVE_DOWN, amount);
}

int mbv89_rotate_forward(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_ROTATE_FORWARD, amount);
}

int mbv89_rotate_backward(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_ROTATE_BACKWARD, amount);
}

int mbv89_rotate_left(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_ROTATE_LEFT, amount);
}

int mbv89_rotate_right(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_ROTATE_RIGHT, amount);
}

int mbv89_rotate_up(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_ROTATE_UP, amount);
}

int mbv89_rotate_down(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, MBV89_BASE_ROTATE_DOWN, amount);
}

static int mbv89_game_dispatch(mbv89_context *ctx, mbv89_actor *actor, mbv89_game_verb verb)
{
    int handled;
    int result;

    if (ctx == 0 || actor == 0) {
        return MBV89_ERROR;
    }

    mbv89_event_game(ctx, verb, 1);

    handled = MBV89_UNHANDLED;
    if (ctx->game_provider.perform != 0) {
        handled = ctx->game_provider.perform(
            ctx->game_provider.user, ctx, actor, verb
        );
    }

    if (handled == MBV89_HANDLED) {
        mbv89_event_game(ctx, verb, 0);
        return MBV89_OK;
    }
    if (handled < 0) {
        mbv89_event_game(ctx, verb, 0);
        return MBV89_ERROR;
    }

    result = MBV89_ERROR;
    switch (verb) {
        case MBV89_GAME_WALK_FORWARD:
            result = mbv89_move_forward(ctx, actor, ctx->walk_step);
            break;
        case MBV89_GAME_WALK_BACKWARD:
            result = mbv89_move_backward(ctx, actor, ctx->walk_step);
            break;
        case MBV89_GAME_STRAFE_LEFT:
            result = mbv89_move_left(ctx, actor, ctx->strafe_step);
            break;
        case MBV89_GAME_STRAFE_RIGHT:
            result = mbv89_move_right(ctx, actor, ctx->strafe_step);
            break;
        case MBV89_GAME_TURN_LEFT:
            result = mbv89_rotate_left(ctx, actor, ctx->turn_step);
            break;
        case MBV89_GAME_TURN_RIGHT:
            result = mbv89_rotate_right(ctx, actor, ctx->turn_step);
            break;
        case MBV89_GAME_FLY_UP:
            result = mbv89_move_up(ctx, actor, ctx->fly_step);
            break;
        case MBV89_GAME_FLY_DOWN:
            result = mbv89_move_down(ctx, actor, ctx->fly_step);
            break;
        case MBV89_GAME_RUN_FORWARD:
            result = mbv89_move_forward(ctx, actor, ctx->run_step);
            break;
        case MBV89_GAME_RUN_BACKWARD:
            result = mbv89_move_backward(ctx, actor, ctx->run_step);
            break;
        default:
            result = MBV89_ERROR;
            break;
    }

    mbv89_event_game(ctx, verb, 0);
    return result;
}

int mbv89_walk_forward(mbv89_context *ctx, mbv89_actor *actor)
{
    return mbv89_game_dispatch(ctx, actor, MBV89_GAME_WALK_FORWARD);
}

int mbv89_walk_backward(mbv89_context *ctx, mbv89_actor *actor)
{
    return mbv89_game_dispatch(ctx, actor, MBV89_GAME_WALK_BACKWARD);
}

int mbv89_strafe_left(mbv89_context *ctx, mbv89_actor *actor)
{
    return mbv89_game_dispatch(ctx, actor, MBV89_GAME_STRAFE_LEFT);
}

int mbv89_strafe_right(mbv89_context *ctx, mbv89_actor *actor)
{
    return mbv89_game_dispatch(ctx, actor, MBV89_GAME_STRAFE_RIGHT);
}

int mbv89_turn_left(mbv89_context *ctx, mbv89_actor *actor)
{
    return mbv89_game_dispatch(ctx, actor, MBV89_GAME_TURN_LEFT);
}

int mbv89_turn_right(mbv89_context *ctx, mbv89_actor *actor)
{
    return mbv89_game_dispatch(ctx, actor, MBV89_GAME_TURN_RIGHT);
}

int mbv89_fly_up(mbv89_context *ctx, mbv89_actor *actor)
{
    return mbv89_game_dispatch(ctx, actor, MBV89_GAME_FLY_UP);
}

int mbv89_fly_down(mbv89_context *ctx, mbv89_actor *actor)
{
    return mbv89_game_dispatch(ctx, actor, MBV89_GAME_FLY_DOWN);
}

int mbv89_run_forward(mbv89_context *ctx, mbv89_actor *actor)
{
    return mbv89_game_dispatch(ctx, actor, MBV89_GAME_RUN_FORWARD);
}

int mbv89_run_backward(mbv89_context *ctx, mbv89_actor *actor)
{
    return mbv89_game_dispatch(ctx, actor, MBV89_GAME_RUN_BACKWARD);
}

const char *mbv89_base_verb_name(mbv89_base_verb verb)
{
    static const char *names[MBV89_BASE_VERB_COUNT] = {
        "move_forward",
        "move_backward",
        "move_left",
        "move_right",
        "move_up",
        "move_down",
        "rotate_forward",
        "rotate_backward",
        "rotate_left",
        "rotate_right",
        "rotate_up",
        "rotate_down"
    };

    if ((int)verb < 0 || (int)verb >= (int)MBV89_BASE_VERB_COUNT) {
        return "unknown_base_verb";
    }
    return names[(int)verb];
}

const char *mbv89_game_verb_name(mbv89_game_verb verb)
{
    static const char *names[MBV89_GAME_VERB_COUNT] = {
        "walk_forward",
        "walk_backward",
        "strafe_left",
        "strafe_right",
        "turn_left",
        "turn_right",
        "fly_up",
        "fly_down",
        "run_forward",
        "run_backward"
    };

    if ((int)verb < 0 || (int)verb >= (int)MBV89_GAME_VERB_COUNT) {
        return "unknown_game_verb";
    }
    return names[(int)verb];
}


int mbv89_perform_base(mbv89_context *ctx, mbv89_actor *actor,
                       mbv89_base_verb verb, mbv89_fixed amount)
{
    return mbv89_base_dispatch(ctx, actor, verb, amount);
}

int mbv89_perform_game(mbv89_context *ctx, mbv89_actor *actor,
                       mbv89_game_verb verb)
{
    return mbv89_game_dispatch(ctx, actor, verb);
}

int mbv89_base_verb_from_name(const char *name, mbv89_base_verb *out_verb)
{
    unsigned int i;
    static const struct mbv89_base_name_entry {
        const char *name;
        mbv89_base_verb verb;
    } entries[] = {
        { "move_forward", MBV89_BASE_MOVE_FORWARD },
        { "move_foward", MBV89_BASE_MOVE_FORWARD },
        { "move_backward", MBV89_BASE_MOVE_BACKWARD },
        { "move_back", MBV89_BASE_MOVE_BACKWARD },
        { "move_left", MBV89_BASE_MOVE_LEFT },
        { "move_right", MBV89_BASE_MOVE_RIGHT },
        { "move_up", MBV89_BASE_MOVE_UP },
        { "move_down", MBV89_BASE_MOVE_DOWN },
        { "rotate_forward", MBV89_BASE_ROTATE_FORWARD },
        { "rotate_foward", MBV89_BASE_ROTATE_FORWARD },
        { "rotate_backward", MBV89_BASE_ROTATE_BACKWARD },
        { "rotate_back", MBV89_BASE_ROTATE_BACKWARD },
        { "rotate_left", MBV89_BASE_ROTATE_LEFT },
        { "rotate_right", MBV89_BASE_ROTATE_RIGHT },
        { "rotate_up", MBV89_BASE_ROTATE_UP },
        { "rotate_down", MBV89_BASE_ROTATE_DOWN }
    };

    if (name == 0 || out_verb == 0) return 0;
    for (i = 0U; i < (unsigned int)(sizeof(entries) / sizeof(entries[0])); ++i) {
        if (strcmp(name, entries[i].name) == 0) {
            *out_verb = entries[i].verb;
            return 1;
        }
    }
    return 0;
}

int mbv89_game_verb_from_name(const char *name, mbv89_game_verb *out_verb)
{
    unsigned int i;
    static const struct mbv89_game_name_entry {
        const char *name;
        mbv89_game_verb verb;
    } entries[] = {
        { "walk_forward", MBV89_GAME_WALK_FORWARD },
        { "walk_foward", MBV89_GAME_WALK_FORWARD },
        { "walk_backward", MBV89_GAME_WALK_BACKWARD },
        { "walk_back", MBV89_GAME_WALK_BACKWARD },
        { "strafe_left", MBV89_GAME_STRAFE_LEFT },
        { "strafe_right", MBV89_GAME_STRAFE_RIGHT },
        { "turn_left", MBV89_GAME_TURN_LEFT },
        { "turn_right", MBV89_GAME_TURN_RIGHT },
        { "fly_up", MBV89_GAME_FLY_UP },
        { "fly_down", MBV89_GAME_FLY_DOWN },
        { "run_forward", MBV89_GAME_RUN_FORWARD },
        { "run_foward", MBV89_GAME_RUN_FORWARD },
        { "run_backward", MBV89_GAME_RUN_BACKWARD },
        { "run_back", MBV89_GAME_RUN_BACKWARD }
    };

    if (name == 0 || out_verb == 0) return 0;
    for (i = 0U; i < (unsigned int)(sizeof(entries) / sizeof(entries[0])); ++i) {
        if (strcmp(name, entries[i].name) == 0) {
            *out_verb = entries[i].verb;
            return 1;
        }
    }
    return 0;
}
