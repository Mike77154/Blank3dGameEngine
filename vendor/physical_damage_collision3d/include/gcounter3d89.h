#ifndef GCOUNTER3D89_H
#define GCOUNTER3D89_H

/*
   gcounter3d89 - counter-window/token system for 3D combat.
   C89, fixed arenas only; caller owns all engine integration.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GCOUNTER3D89_MAX_ACTORS
#define GCOUNTER3D89_MAX_ACTORS 64
#endif

#ifndef GCOUNTER3D89_MAX_TOKENS
#define GCOUNTER3D89_MAX_TOKENS 128
#endif

#define GCTR_FIX_ONE 1024L
#define GCTR_TRUE 1
#define GCTR_FALSE 0

#define GCTR_SRC_BLOCK 1
#define GCTR_SRC_GUARD 2
#define GCTR_SRC_PARRY 4
#define GCTR_SRC_SHELL 8
#define GCTR_SRC_HIT_ARMOR 16

#define GCTR_DEF_OK 1
#define GCTR_DEF_PERFECT 2
#define GCTR_DEF_BROKEN 4
#define GCTR_DEF_LATE 8

#define GCTR_EVENT_TOKEN 1
#define GCTR_EVENT_COUNTER 2
#define GCTR_EVENT_EXPIRE 3
#define GCTR_EVENT_DENIED 4
#define GCTR_EVENT_COOLDOWN 5

#define GCTR_COUNTER_LIGHT 1
#define GCTR_COUNTER_HEAVY 2
#define GCTR_COUNTER_THROW 3
#define GCTR_COUNTER_SCRIPT 4

typedef long GCTR_Fix;

typedef struct GCTR_Vec3_s {
    GCTR_Fix x;
    GCTR_Fix y;
    GCTR_Fix z;
} GCTR_Vec3;

typedef struct GCTR_Profile_s {
    int open_after_frames;
    int valid_frames;
    int cooldown_frames;
    int token_life_frames;
    int max_tokens_per_actor;
    int stamina_cost;
    int posture_cost;
    int require_perfect;
    int consume_on_fail;
    int range_sq;              /* fixed units squared, caller-defined scale */
    unsigned int allowed_sources;
    int counter_kind;
    int script_code;
} GCTR_Profile;

typedef struct GCTR_DefenseEvent_s {
    int defender_id;
    int attacker_id;
    int event_frame;
    unsigned int source_flags;
    unsigned int defense_result_flags;
    GCTR_Vec3 attacker_rel_pos;
} GCTR_DefenseEvent;

typedef struct GCTR_Request_s {
    int actor_id;
    int target_id;
    int current_frame;
    int stamina_available;
    int posture_available;
    GCTR_Vec3 target_rel_pos;
} GCTR_Request;

typedef struct GCTR_Result_s {
    int accepted;
    int denied_reason;
    int counter_kind;
    int script_code;
    int target_id;
    int token_age;
    int stamina_cost;
    int posture_cost;
} GCTR_Result;

typedef void (*GCTR_EventFn)(int actor_id, int other_id, int event_code, int amount, void *user);
typedef void (*GCTR_CounterFn)(int actor_id, int target_id, int counter_kind, int script_code, void *user);

typedef struct GCTR_Actor_s {
    int used;
    int actor_id;
    int cooldown_timer;
    GCTR_Profile profile;
} GCTR_Actor;

typedef struct GCTR_Token_s {
    int used;
    int owner_id;
    int target_id;
    int born_frame;
    int open_frame;
    int expire_frame;
    unsigned int source_flags;
    unsigned int defense_result_flags;
    GCTR_Vec3 attacker_rel_pos;
} GCTR_Token;

typedef struct GCTR_Context_s {
    GCTR_Actor actors[GCOUNTER3D89_MAX_ACTORS];
    GCTR_Token tokens[GCOUNTER3D89_MAX_TOKENS];
    GCTR_EventFn event_fn;
    GCTR_CounterFn counter_fn;
    void *event_user;
    void *counter_user;
} GCTR_Context;

void gctr_default_profile(GCTR_Profile *p);
void gctr_init(GCTR_Context *ctx);
void gctr_set_event_callback(GCTR_Context *ctx, GCTR_EventFn fn, void *user);
void gctr_set_counter_callback(GCTR_Context *ctx, GCTR_CounterFn fn, void *user);
int gctr_create_actor(GCTR_Context *ctx, int actor_id, const GCTR_Profile *profile);
int gctr_remove_actor(GCTR_Context *ctx, int actor_id);
void gctr_update(GCTR_Context *ctx, int frames, int current_frame);
int gctr_feed_defense_event(GCTR_Context *ctx, const GCTR_DefenseEvent *ev);
int gctr_try_counter(GCTR_Context *ctx, const GCTR_Request *req, GCTR_Result *out_result);
int gctr_count_tokens(const GCTR_Context *ctx, int actor_id);
GCTR_Fix gctr_fix_mul(GCTR_Fix a, GCTR_Fix b);
int gctr_vec3_len_sq_fix(const GCTR_Vec3 *v);

#ifdef __cplusplus
}
#endif

#endif
