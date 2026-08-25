#ifndef GBLOCK3D89_H
#define GBLOCK3D89_H

/*
   gblock3d89 - fixed point 3D block resolver
   C89, fixed arenas only; caller owns all engine integration.
   The caller owns world movement, animation, damage application and rendering.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GBLOCK3D89_MAX_ACTORS
#define GBLOCK3D89_MAX_ACTORS 64
#endif

#define GBLK_FIX_ONE 1024L
#define GBLK_TRUE 1
#define GBLK_FALSE 0

#define GBLK_EVENT_BLOCKED 1
#define GBLK_EVENT_CHIPPED 2
#define GBLK_EVENT_BROKEN 3
#define GBLK_EVENT_PASSED 4
#define GBLK_EVENT_RECOIL 5

#define GBLK_FLAG_UNBLOCKABLE 1
#define GBLK_FLAG_GUARD_BREAKER 2
#define GBLK_FLAG_IGNORE_DIR 4

typedef long GBLK_Fix;

typedef struct GBLK_Vec3_s {
    GBLK_Fix x;
    GBLK_Fix y;
    GBLK_Fix z;
} GBLK_Vec3;

typedef struct GBLK_Profile_s {
    int coverage_cos;          /* fixed cosine. 512 ~= 60 deg, 0 ~= 90 deg. */
    int stamina_cost_mul;      /* attack stamina damage multiplier, 1024 = 1x */
    int posture_cost_mul;      /* attack posture damage multiplier */
    int chip_mul;              /* blocked damage leak, 0..1024 */
    int recoil_frames;
    int blockstun_bonus;
    int min_stamina_to_block;
    int max_stamina;
    int stamina_regen_per_frame;
    int max_posture;
    int posture_regen_per_frame;
} GBLK_Profile;

typedef struct GBLK_Attack_s {
    int attack_id;
    int attacker_id;
    int damage;
    int stamina_damage;
    int posture_damage;
    int blockstun_frames;
    int breaker_power;
    unsigned int flags;
    GBLK_Vec3 source_dir;      /* normalized direction from defender toward threat */
} GBLK_Attack;

typedef struct GBLK_Result_s {
    int accepted;
    int blocked;
    int broken;
    int damage_to_apply;
    int stamina_cost;
    int posture_cost;
    int blockstun_frames;
    int recoil_frames;
    int dot;
    int event_code;
} GBLK_Result;

typedef void (*GBLK_EventFn)(int actor_id, int other_id, int event_code, int amount, void *user);

typedef struct GBLK_Actor_s {
    int used;
    int actor_id;
    int active;
    int stamina;
    int posture;
    int cooldown_frames;
    GBLK_Vec3 facing_dir;
    GBLK_Profile profile;
} GBLK_Actor;

typedef struct GBLK_Context_s {
    GBLK_Actor actors[GBLOCK3D89_MAX_ACTORS];
    GBLK_EventFn event_fn;
    void *event_user;
} GBLK_Context;

void gblk_default_profile(GBLK_Profile *p);
void gblk_init(GBLK_Context *ctx);
void gblk_set_event_callback(GBLK_Context *ctx, GBLK_EventFn fn, void *user);
int gblk_create_actor(GBLK_Context *ctx, int actor_id, const GBLK_Profile *profile);
int gblk_remove_actor(GBLK_Context *ctx, int actor_id);
int gblk_set_active(GBLK_Context *ctx, int actor_id, int active);
int gblk_set_facing(GBLK_Context *ctx, int actor_id, GBLK_Fix x, GBLK_Fix y, GBLK_Fix z);
int gblk_set_resources(GBLK_Context *ctx, int actor_id, int stamina, int posture);
void gblk_update(GBLK_Context *ctx, int frames);
int gblk_try_block(GBLK_Context *ctx, int actor_id, const GBLK_Attack *atk, GBLK_Result *out_result);
int gblk_get_stamina(const GBLK_Context *ctx, int actor_id);
int gblk_get_posture(const GBLK_Context *ctx, int actor_id);
GBLK_Fix gblk_fix_mul(GBLK_Fix a, GBLK_Fix b);
int gblk_vec3_dot_fix(const GBLK_Vec3 *a, const GBLK_Vec3 *b);

#ifdef __cplusplus
}
#endif

#endif
