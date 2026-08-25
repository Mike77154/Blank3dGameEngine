#ifndef MOVEMENTBASEVERBS89_H
#define MOVEMENTBASEVERBS89_H

/*
 * 3d_movementbaseverbs89
 * C89, fixed-point, no heap.
 *
 * Base verbs describe geometric intent.
 * Game verbs describe locomotion intent and fall back to base verbs.
 * Every base movement/rotation verb is providerable.
 */

#include <limits.h>

#if INT_MAX < 2147483647
#error "3d_movementbaseverbs89 requires an int type of at least 32 bits"
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef signed int mbv89_fixed;

#define MBV89_FIXED_SHIFT 16
#define MBV89_FIXED_ONE   ((mbv89_fixed)65536)
#define MBV89_FIXED_HALF  ((mbv89_fixed)32768)

#define MBV89_OK          0
#define MBV89_UNHANDLED   0
#define MBV89_HANDLED     1
#define MBV89_ERROR      -1

typedef enum mbv89_base_verb {
    MBV89_BASE_MOVE_FORWARD = 0,
    MBV89_BASE_MOVE_BACKWARD,
    MBV89_BASE_MOVE_LEFT,
    MBV89_BASE_MOVE_RIGHT,
    MBV89_BASE_MOVE_UP,
    MBV89_BASE_MOVE_DOWN,
    MBV89_BASE_ROTATE_FORWARD,
    MBV89_BASE_ROTATE_BACKWARD,
    MBV89_BASE_ROTATE_LEFT,
    MBV89_BASE_ROTATE_RIGHT,
    MBV89_BASE_ROTATE_UP,
    MBV89_BASE_ROTATE_DOWN,
    MBV89_BASE_VERB_COUNT
} mbv89_base_verb;

typedef enum mbv89_game_verb {
    MBV89_GAME_WALK_FORWARD = 0,
    MBV89_GAME_WALK_BACKWARD,
    MBV89_GAME_STRAFE_LEFT,
    MBV89_GAME_STRAFE_RIGHT,
    MBV89_GAME_TURN_LEFT,
    MBV89_GAME_TURN_RIGHT,
    MBV89_GAME_FLY_UP,
    MBV89_GAME_FLY_DOWN,
    MBV89_GAME_RUN_FORWARD,
    MBV89_GAME_RUN_BACKWARD,
    MBV89_GAME_VERB_COUNT
} mbv89_game_verb;

typedef struct mbv89_vec3 {
    mbv89_fixed x;
    mbv89_fixed y;
    mbv89_fixed z;
} mbv89_vec3;

typedef struct mbv89_actor {
    mbv89_vec3 position;
    /* Q16.16 turns. 65536 == one complete turn. */
    mbv89_fixed pitch;
    mbv89_fixed yaw;
    mbv89_fixed roll;
    void *user;
} mbv89_actor;

struct mbv89_context;

/*
 * Base provider.
 * Return MBV89_HANDLED to suppress the built-in fallback.
 * Return MBV89_UNHANDLED to allow the built-in fallback.
 *
 * This is the main engine hook: transform systems, physics controllers,
 * ECS bridges, character controllers, editors, cameras, etc. can own the
 * actual implementation of every move_* and rotate_* verb.
 */
typedef int (*mbv89_base_provider_fn)(
    void *provider_user,
    struct mbv89_context *ctx,
    mbv89_actor *actor,
    mbv89_base_verb verb,
    mbv89_fixed amount
);

typedef struct mbv89_base_provider {
    mbv89_base_provider_fn perform;
    void *user;
} mbv89_base_provider;

/*
 * Game provider.
 * Used by locomotion, gravity, animation, root-motion, vehicles, networking,
 * stamina, slope logic, IK, etc. Return UNHANDLED to use the default mapping
 * to the base provider.
 */
typedef int (*mbv89_game_provider_fn)(
    void *provider_user,
    struct mbv89_context *ctx,
    mbv89_actor *actor,
    mbv89_game_verb verb
);

typedef struct mbv89_game_provider {
    mbv89_game_provider_fn perform;
    void *user;
} mbv89_game_provider;

typedef void (*mbv89_event_fn)(
    void *event_user,
    int is_game_verb,
    int verb,
    int entering
);

typedef struct mbv89_event_provider {
    mbv89_event_fn notify;
    void *user;
} mbv89_event_provider;

typedef struct mbv89_context {
    mbv89_base_provider base_provider;
    mbv89_game_provider game_provider;
    mbv89_event_provider event_provider;

    mbv89_fixed walk_step;
    mbv89_fixed strafe_step;
    mbv89_fixed run_step;
    mbv89_fixed fly_step;
    mbv89_fixed turn_step;
} mbv89_context;

void mbv89_actor_init(mbv89_actor *actor);
void mbv89_context_init(mbv89_context *ctx);
void mbv89_set_base_provider(mbv89_context *ctx, mbv89_base_provider_fn fn, void *user);
void mbv89_set_game_provider(mbv89_context *ctx, mbv89_game_provider_fn fn, void *user);
void mbv89_set_event_provider(mbv89_context *ctx, mbv89_event_fn fn, void *user);

/* Base verbs: providerable, amount-controlled. */
int mbv89_move_forward(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);
int mbv89_move_backward(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);
int mbv89_move_left(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);
int mbv89_move_right(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);
int mbv89_move_up(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);
int mbv89_move_down(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);

int mbv89_rotate_forward(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);
int mbv89_rotate_backward(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);
int mbv89_rotate_left(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);
int mbv89_rotate_right(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);
int mbv89_rotate_up(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);
int mbv89_rotate_down(mbv89_context *ctx, mbv89_actor *actor, mbv89_fixed amount);

/* Game verbs: providerable semantic layer, defaulting to base verbs. */
int mbv89_walk_forward(mbv89_context *ctx, mbv89_actor *actor);
int mbv89_walk_backward(mbv89_context *ctx, mbv89_actor *actor);
int mbv89_strafe_left(mbv89_context *ctx, mbv89_actor *actor);
int mbv89_strafe_right(mbv89_context *ctx, mbv89_actor *actor);
int mbv89_turn_left(mbv89_context *ctx, mbv89_actor *actor);
int mbv89_turn_right(mbv89_context *ctx, mbv89_actor *actor);
int mbv89_fly_up(mbv89_context *ctx, mbv89_actor *actor);
int mbv89_fly_down(mbv89_context *ctx, mbv89_actor *actor);
int mbv89_run_forward(mbv89_context *ctx, mbv89_actor *actor);
int mbv89_run_backward(mbv89_context *ctx, mbv89_actor *actor);

const char *mbv89_base_verb_name(mbv89_base_verb verb);
const char *mbv89_game_verb_name(mbv89_game_verb verb);

/*
 * Text vocabulary helpers. They are intentionally gameplay-agnostic: the
 * library only recognizes its own movement verbs and aliases.
 */
int mbv89_base_verb_from_name(const char *name, mbv89_base_verb *out_verb);
int mbv89_game_verb_from_name(const char *name, mbv89_game_verb *out_verb);

/* Enum dispatch helpers for hosts that resolve names/config externally. */
int mbv89_perform_base(mbv89_context *ctx, mbv89_actor *actor,
                       mbv89_base_verb verb, mbv89_fixed amount);
int mbv89_perform_game(mbv89_context *ctx, mbv89_actor *actor,
                       mbv89_game_verb verb);

#ifdef __cplusplus
}
#endif

#endif
