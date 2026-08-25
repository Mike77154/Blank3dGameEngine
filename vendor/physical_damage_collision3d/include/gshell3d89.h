#ifndef GSHELL3D89_H
#define GSHELL3D89_H

/*
   gshell3d89 - absorbent shell/armor/shield layers for 3D combat.
   C89, fixed arenas only; caller owns all engine integration.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GSHELL3D89_MAX_ACTORS
#define GSHELL3D89_MAX_ACTORS 48
#endif

#ifndef GSHELL3D89_MAX_LAYERS
#define GSHELL3D89_MAX_LAYERS 4
#endif

#define GSHL_FIX_ONE 1024L
#define GSHL_TRUE 1
#define GSHL_FALSE 0

#define GSHL_EVENT_ABSORB 1
#define GSHL_EVENT_LEAK 2
#define GSHL_EVENT_LAYER_BREAK 3
#define GSHL_EVENT_RECHARGE 4
#define GSHL_EVENT_BREACH 5

#define GSHL_DMG_BLUNT 1
#define GSHL_DMG_SLASH 2
#define GSHL_DMG_PIERCE 4
#define GSHL_DMG_FIRE 8
#define GSHL_DMG_ENERGY 16

#define GSHL_FLAG_BYPASS_SHELL 1
#define GSHL_FLAG_NO_RECHARGE_RESET 2

typedef long GSHL_Fix;

typedef struct GSHL_Vec3_s {
    GSHL_Fix x;
    GSHL_Fix y;
    GSHL_Fix z;
} GSHL_Vec3;

typedef struct GSHL_LayerProfile_s {
    int max_hp;
    int absorb_mul;             /* 1024 = try to absorb full incoming damage */
    int leak_mul;               /* minimum leak even when shell holds */
    int recharge_delay_frames;
    int recharge_per_frame;
    int break_recovery_frames;
    int coverage_cos;
    unsigned int damage_mask;
} GSHL_LayerProfile;

typedef struct GSHL_Damage_s {
    int attack_id;
    int attacker_id;
    int damage;
    unsigned int damage_type;
    unsigned int flags;
    GSHL_Vec3 source_dir;
} GSHL_Damage;

typedef struct GSHL_Result_s {
    int accepted;
    int absorbed;
    int leaked_damage;
    int broken_layer_index;
    int remaining_shell_hp;
    int dot;
} GSHL_Result;

typedef void (*GSHL_EventFn)(int actor_id, int other_id, int event_code, int amount, void *user);

typedef struct GSHL_Layer_s {
    int used;
    int hp;
    int recharge_timer;
    int broken_timer;
    GSHL_LayerProfile profile;
} GSHL_Layer;

typedef struct GSHL_Actor_s {
    int used;
    int actor_id;
    GSHL_Vec3 facing_dir;
    GSHL_Layer layers[GSHELL3D89_MAX_LAYERS];
} GSHL_Actor;

typedef struct GSHL_Context_s {
    GSHL_Actor actors[GSHELL3D89_MAX_ACTORS];
    GSHL_EventFn event_fn;
    void *event_user;
} GSHL_Context;

void gshl_default_layer_profile(GSHL_LayerProfile *p);
void gshl_init(GSHL_Context *ctx);
void gshl_set_event_callback(GSHL_Context *ctx, GSHL_EventFn fn, void *user);
int gshl_create_actor(GSHL_Context *ctx, int actor_id);
int gshl_remove_actor(GSHL_Context *ctx, int actor_id);
int gshl_set_facing(GSHL_Context *ctx, int actor_id, GSHL_Fix x, GSHL_Fix y, GSHL_Fix z);
int gshl_set_layer(GSHL_Context *ctx, int actor_id, int layer_index, const GSHL_LayerProfile *profile, int start_full);
int gshl_clear_layer(GSHL_Context *ctx, int actor_id, int layer_index);
void gshl_update(GSHL_Context *ctx, int frames);
int gshl_apply_damage(GSHL_Context *ctx, int actor_id, const GSHL_Damage *dmg, GSHL_Result *out_result);
int gshl_get_layer_hp(const GSHL_Context *ctx, int actor_id, int layer_index);
int gshl_get_total_hp(const GSHL_Context *ctx, int actor_id);
GSHL_Fix gshl_fix_mul(GSHL_Fix a, GSHL_Fix b);
int gshl_vec3_dot_fix(const GSHL_Vec3 *a, const GSHL_Vec3 *b);

#ifdef __cplusplus
}
#endif

#endif
