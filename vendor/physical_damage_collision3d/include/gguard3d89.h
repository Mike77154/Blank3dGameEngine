#ifndef GGUARD3D89_H
#define GGUARD3D89_H

/*
   gguard3d89 - held 3D guard stance and guard gauge manager.
   C89, fixed arenas only; caller owns all engine integration.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GGUARD3D89_MAX_ACTORS
#define GGUARD3D89_MAX_ACTORS 64
#endif

#define GGRD_FIX_ONE 1024L
#define GGRD_TRUE 1
#define GGRD_FALSE 0

#define GGRD_STATE_IDLE 0
#define GGRD_STATE_RAISE 1
#define GGRD_STATE_HELD 2
#define GGRD_STATE_DROP 3
#define GGRD_STATE_BROKEN 4

#define GGRD_EVENT_RAISE 1
#define GGRD_EVENT_HELD 2
#define GGRD_EVENT_DROP 3
#define GGRD_EVENT_ABSORB 4
#define GGRD_EVENT_BREAK 5
#define GGRD_EVENT_REGEN 6

#define GGRD_FLAG_UNGUARDABLE 1
#define GGRD_FLAG_HEAVY 2
#define GGRD_FLAG_LOW 4
#define GGRD_FLAG_HIGH 8

typedef long GGRD_Fix;

typedef struct GGRD_Vec3_s {
    GGRD_Fix x;
    GGRD_Fix y;
    GGRD_Fix z;
} GGRD_Vec3;

typedef struct GGRD_Profile_s {
    int raise_frames;
    int drop_frames;
    int min_hold_frames;
    int break_recovery_frames;
    int max_guard;
    int guard_regen_idle;
    int guard_regen_held;
    int guard_drain_held;
    int guard_damage_mul;
    int chip_mul;
    int coverage_cos;
    unsigned int lane_mask;      /* high/mid/low permissions from attack flags */
} GGRD_Profile;

typedef struct GGRD_Attack_s {
    int attack_id;
    int attacker_id;
    int damage;
    int guard_damage;
    int blockstun_frames;
    unsigned int flags;
    GGRD_Vec3 source_dir;
} GGRD_Attack;

typedef struct GGRD_Result_s {
    int accepted;
    int guarded;
    int guard_broken;
    int damage_to_apply;
    int guard_damage_taken;
    int blockstun_frames;
    int state_after;
    int dot;
} GGRD_Result;

typedef void (*GGRD_EventFn)(int actor_id, int other_id, int event_code, int amount, void *user);

typedef struct GGRD_Actor_s {
    int used;
    int actor_id;
    int state;
    int guard_value;
    int state_timer;
    int hold_frames;
    GGRD_Vec3 facing_dir;
    GGRD_Profile profile;
} GGRD_Actor;

typedef struct GGRD_Context_s {
    GGRD_Actor actors[GGUARD3D89_MAX_ACTORS];
    GGRD_EventFn event_fn;
    void *event_user;
} GGRD_Context;

void ggrd_default_profile(GGRD_Profile *p);
void ggrd_init(GGRD_Context *ctx);
void ggrd_set_event_callback(GGRD_Context *ctx, GGRD_EventFn fn, void *user);
int ggrd_create_actor(GGRD_Context *ctx, int actor_id, const GGRD_Profile *profile);
int ggrd_remove_actor(GGRD_Context *ctx, int actor_id);
int ggrd_set_facing(GGRD_Context *ctx, int actor_id, GGRD_Fix x, GGRD_Fix y, GGRD_Fix z);
int ggrd_set_guard_value(GGRD_Context *ctx, int actor_id, int value);
void ggrd_update(GGRD_Context *ctx, int frames, int actor_id, int hold_input);
void ggrd_update_all(GGRD_Context *ctx, int frames);
int ggrd_try_guard(GGRD_Context *ctx, int actor_id, const GGRD_Attack *atk, GGRD_Result *out_result);
int ggrd_get_state(const GGRD_Context *ctx, int actor_id);
int ggrd_get_guard_value(const GGRD_Context *ctx, int actor_id);
GGRD_Fix ggrd_fix_mul(GGRD_Fix a, GGRD_Fix b);
int ggrd_vec3_dot_fix(const GGRD_Vec3 *a, const GGRD_Vec3 *b);

#ifdef __cplusplus
}
#endif

#endif
