#ifndef GPROJECTILETRAVEL89_H
#define GPROJECTILETRAVEL89_H

/*
   gprojectiletravel89
   Tracks projectile travel by time, maximum distance and optional destination.

   C89, fixed point, fixed capacity, no heap, no float/double.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GPT89_MAX_PROJECTILES
#define GPT89_MAX_PROJECTILES 256
#endif

#define GPT89_OK             0
#define GPT89_ERR           -1
#define GPT89_BAD_ARG       -2
#define GPT89_FULL          -3
#define GPT89_NOT_FOUND     -4

#define GPT89_FIX_SHIFT      12
#define GPT89_FIX_ONE        (1L << GPT89_FIX_SHIFT)
#define GPT89_FIX_HALF       (1L << (GPT89_FIX_SHIFT - 1))

typedef long GPT89_Fx;

typedef struct GPT89_Vec3Tag {
    GPT89_Fx x;
    GPT89_Fx y;
    GPT89_Fx z;
} GPT89_Vec3;

enum GPT89_EndReasonTag {
    GPT89_END_NONE = 0,
    GPT89_END_TIMEOUT = 1,
    GPT89_END_MAX_DISTANCE = 2,
    GPT89_END_DESTINATION = 3,
    GPT89_END_MANUAL = 4
};

enum GPT89_ModeTag {
    GPT89_MODE_INTERNAL_MOVE = 0,
    GPT89_MODE_EXTERNAL_POSITION = 1
};

typedef struct GPT89_DescTag {
    int projectile_id;
    int owner_id;
    int weapon_id;
    int team_id;
    int user_tag;
    int mode;

    GPT89_Vec3 origin;
    GPT89_Vec3 direction;
    GPT89_Vec3 destination;
    int has_destination;

    GPT89_Fx speed_fx;
    GPT89_Fx max_distance_fx;
    GPT89_Fx arrival_radius_fx;
    unsigned long life_ms;
} GPT89_Desc;

typedef struct GPT89_StateTag {
    int active;
    int end_reason;
    int slot;

    int projectile_id;
    int owner_id;
    int weapon_id;
    int team_id;
    int user_tag;
    int mode;

    GPT89_Vec3 origin;
    GPT89_Vec3 previous_position;
    GPT89_Vec3 position;
    GPT89_Vec3 direction;
    GPT89_Vec3 destination;
    int has_destination;

    GPT89_Fx speed_fx;
    GPT89_Fx max_distance_fx;
    GPT89_Fx arrival_radius_fx;
    GPT89_Fx traveled_fx;
    unsigned long life_ms;
    unsigned long elapsed_ms;
} GPT89_State;

typedef void (*GPT89_EndFn)(void *ctx, const GPT89_State *state);

typedef struct GPT89_PoolTag {
    GPT89_State projectiles[GPT89_MAX_PROJECTILES];
    int active_count;
    GPT89_EndFn end_fn;
    void *end_ctx;
} GPT89_Pool;

GPT89_Fx   gpt89_fx_from_int(int value);
int        gpt89_fx_to_int_round(GPT89_Fx value);
GPT89_Vec3 gpt89_v3(GPT89_Fx x, GPT89_Fx y, GPT89_Fx z);
GPT89_Fx   gpt89_distance_approx(GPT89_Vec3 a, GPT89_Vec3 b);

void gpt89_desc_defaults(GPT89_Desc *desc);
void gpt89_pool_init(GPT89_Pool *pool);
void gpt89_set_end_hook(GPT89_Pool *pool, GPT89_EndFn end_fn, void *ctx);

int gpt89_spawn(GPT89_Pool *pool, const GPT89_Desc *desc);
int gpt89_kill(GPT89_Pool *pool, int slot, int reason);
int gpt89_update_one(GPT89_Pool *pool, int slot, unsigned long dt_ms);
int gpt89_update_all(GPT89_Pool *pool, unsigned long dt_ms);
int gpt89_observe_position(GPT89_Pool *pool,
                           int slot,
                           GPT89_Vec3 new_position,
                           unsigned long dt_ms);

const GPT89_State *gpt89_get(const GPT89_Pool *pool, int slot);
GPT89_State *gpt89_get_mutable(GPT89_Pool *pool, int slot);
GPT89_Fx gpt89_remaining_distance(const GPT89_State *state);
unsigned long gpt89_remaining_life_ms(const GPT89_State *state);

#ifdef __cplusplus
}
#endif

#endif
