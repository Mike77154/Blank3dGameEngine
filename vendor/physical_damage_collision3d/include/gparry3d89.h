#ifndef GPARRY3D89_H
#define GPARRY3D89_H

/*
   gparry3d89 - timing-window parry/deflect resolver.
   C89, fixed arenas only; caller owns all engine integration.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GPARRY3D89_MAX_ACTORS
#define GPARRY3D89_MAX_ACTORS 64
#endif

#define GPRY_FIX_ONE 1024L
#define GPRY_TRUE 1
#define GPRY_FALSE 0

#define GPRY_STATE_IDLE 0
#define GPRY_STATE_STARTUP 1
#define GPRY_STATE_ACTIVE 2
#define GPRY_STATE_RECOVERY 3
#define GPRY_STATE_COOLDOWN 4

#define GPRY_RESULT_NONE 0
#define GPRY_RESULT_PERFECT 1
#define GPRY_RESULT_NORMAL 2
#define GPRY_RESULT_LATE 3
#define GPRY_RESULT_FAIL 4

#define GPRY_EVENT_PRESS 1
#define GPRY_EVENT_PERFECT 2
#define GPRY_EVENT_NORMAL 3
#define GPRY_EVENT_LATE 4
#define GPRY_EVENT_FAIL 5
#define GPRY_EVENT_SPAM_PENALTY 6

#define GPRY_FLAG_UNPARRYABLE 1
#define GPRY_FLAG_THRUST 2
#define GPRY_FLAG_PROJECTILE 4

typedef long GPRY_Fix;

typedef struct GPRY_Vec3_s {
    GPRY_Fix x;
    GPRY_Fix y;
    GPRY_Fix z;
} GPRY_Vec3;

typedef struct GPRY_Profile_s {
    int startup_frames;
    int perfect_frames;
    int normal_frames;
    int late_frames;
    int recovery_frames;
    int cooldown_frames;
    int coverage_cos;
    int posture_reward_perfect;
    int posture_reward_normal;
    int attacker_stagger_perfect;
    int attacker_stagger_normal;
    int anti_spam_limit;
    int anti_spam_extra_recovery;
    unsigned int parry_mask;
} GPRY_Profile;

typedef struct GPRY_Attack_s {
    int attack_id;
    int attacker_id;
    int damage;
    int posture_damage;
    unsigned int flags;
    GPRY_Vec3 source_dir;
} GPRY_Attack;

typedef struct GPRY_Result_s {
    int accepted;
    int result_code;
    int damage_to_apply;
    int defender_posture_delta;
    int attacker_stagger_frames;
    int recovery_frames;
    int dot;
} GPRY_Result;

typedef void (*GPRY_EventFn)(int actor_id, int other_id, int event_code, int amount, void *user);

typedef struct GPRY_Actor_s {
    int used;
    int actor_id;
    int state;
    int timer;
    int age_frames;
    int spam_count;
    int since_last_press;
    GPRY_Vec3 facing_dir;
    GPRY_Profile profile;
} GPRY_Actor;

typedef struct GPRY_Context_s {
    GPRY_Actor actors[GPARRY3D89_MAX_ACTORS];
    GPRY_EventFn event_fn;
    void *event_user;
} GPRY_Context;

void gpry_default_profile(GPRY_Profile *p);
void gpry_init(GPRY_Context *ctx);
void gpry_set_event_callback(GPRY_Context *ctx, GPRY_EventFn fn, void *user);
int gpry_create_actor(GPRY_Context *ctx, int actor_id, const GPRY_Profile *profile);
int gpry_remove_actor(GPRY_Context *ctx, int actor_id);
int gpry_set_facing(GPRY_Context *ctx, int actor_id, GPRY_Fix x, GPRY_Fix y, GPRY_Fix z);
int gpry_press(GPRY_Context *ctx, int actor_id);
void gpry_update(GPRY_Context *ctx, int frames);
int gpry_try_parry(GPRY_Context *ctx, int actor_id, const GPRY_Attack *atk, GPRY_Result *out_result);
int gpry_get_state(const GPRY_Context *ctx, int actor_id);
int gpry_get_age(const GPRY_Context *ctx, int actor_id);
GPRY_Fix gpry_fix_mul(GPRY_Fix a, GPRY_Fix b);
int gpry_vec3_dot_fix(const GPRY_Vec3 *a, const GPRY_Vec3 *b);

#ifdef __cplusplus
}
#endif

#endif
