#include <stdio.h>
#include "3d_movementbaseverbs89.h"

typedef struct test_provider_state {
    int calls;
    mbv89_base_verb last_verb;
    mbv89_fixed last_amount;
} test_provider_state;

static int provider_capture(void *user, mbv89_context *ctx, mbv89_actor *actor,
                            mbv89_base_verb verb, mbv89_fixed amount)
{
    test_provider_state *state;
    (void)ctx;
    (void)actor;
    state = (test_provider_state *)user;
    state->calls += 1;
    state->last_verb = verb;
    state->last_amount = amount;
    return MBV89_HANDLED;
}

static int game_unhandled(void *user, mbv89_context *ctx, mbv89_actor *actor,
                          mbv89_game_verb verb)
{
    int *calls;
    (void)ctx;
    (void)actor;
    (void)verb;
    calls = (int *)user;
    *calls += 1;
    return MBV89_UNHANDLED;
}

static int check(int condition, const char *message)
{
    if (!condition) {
        printf("FAIL: %s\n", message);
        return 0;
    }
    return 1;
}

int main(void)
{
    mbv89_context ctx;
    mbv89_actor actor;
    test_provider_state base_state;
    int game_calls;
    int ok;

    ok = 1;
    mbv89_context_init(&ctx);
    mbv89_actor_init(&actor);

    ok &= check(mbv89_move_forward(&ctx, &actor, MBV89_FIXED_ONE) == MBV89_OK,
                "fallback move_forward returned error");
    ok &= check(actor.position.z == MBV89_FIXED_ONE,
                "fallback move_forward did not update +Z");

    ok &= check(mbv89_move_left(&ctx, &actor, MBV89_FIXED_HALF) == MBV89_OK,
                "fallback move_left returned error");
    ok &= check(actor.position.x == -MBV89_FIXED_HALF,
                "fallback move_left did not update -X");

    actor.yaw = 0;
    ok &= check(mbv89_turn_left(&ctx, &actor) == MBV89_OK,
                "default turn_left returned error");
    ok &= check(actor.yaw == ctx.turn_step,
                "turn_left must increase yaw in the canonical right-handed convention");

    actor.yaw = 0;
    ok &= check(mbv89_turn_right(&ctx, &actor) == MBV89_OK,
                "default turn_right returned error");
    ok &= check(actor.yaw == -ctx.turn_step,
                "turn_right must decrease yaw in the canonical right-handed convention");

    base_state.calls = 0;
    base_state.last_verb = MBV89_BASE_MOVE_FORWARD;
    base_state.last_amount = 0;
    mbv89_set_base_provider(&ctx, provider_capture, &base_state);

    actor.position.z = 0;
    ok &= check(mbv89_move_forward(&ctx, &actor, 1234) == MBV89_OK,
                "provider-backed move_forward returned error");
    ok &= check(base_state.calls == 1,
                "base provider was not called");
    ok &= check(base_state.last_verb == MBV89_BASE_MOVE_FORWARD,
                "base provider got wrong verb");
    ok &= check(base_state.last_amount == 1234,
                "base provider got wrong amount");
    ok &= check(actor.position.z == 0,
                "handled base provider should suppress fallback transform");

    game_calls = 0;
    mbv89_set_game_provider(&ctx, game_unhandled, &game_calls);
    ok &= check(mbv89_run_forward(&ctx, &actor) == MBV89_OK,
                "run_forward with unhandled game provider returned error");
    ok &= check(game_calls == 1,
                "game provider was not called");
    ok &= check(base_state.calls == 2,
                "game verb did not fall through into base provider");
    ok &= check(base_state.last_verb == MBV89_BASE_MOVE_FORWARD,
                "run_forward fallback did not use move_forward");
    ok &= check(base_state.last_amount == ctx.run_step,
                "run_forward fallback used wrong step");

    if (!ok) {
        return 1;
    }

    printf("PASS: 3d_movementbaseverbs89\n");
    return 0;
}
