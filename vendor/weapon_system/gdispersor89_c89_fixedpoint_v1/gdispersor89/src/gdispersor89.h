#ifndef GDISPERSOR89_H
#define GDISPERSOR89_H

/*
   gdispersor89
   Expands one weapon shot into N projectile launch requests.

   C89, fixed point, no heap, no float/double, no renderer, no collision.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GDP89_MAX_PROJECTILES
#define GDP89_MAX_PROJECTILES 64
#endif

#define GDP89_OK             0
#define GDP89_ERR           -1
#define GDP89_BAD_ARG       -2
#define GDP89_OUTPUT_FULL   -3

#define GDP89_FIX_SHIFT      12
#define GDP89_FIX_ONE        (1L << GDP89_FIX_SHIFT)
#define GDP89_FIX_HALF       (1L << (GDP89_FIX_SHIFT - 1))

typedef long GDP89_Fx;

typedef struct GDP89_Vec3Tag {
    GDP89_Fx x;
    GDP89_Fx y;
    GDP89_Fx z;
} GDP89_Vec3;

enum GDP89_ModeTag {
    GDP89_MODE_SINGLE = 0,
    GDP89_MODE_SPREAD = 1
};

enum GDP89_PatternTag {
    GDP89_PATTERN_BALANCED = 0,
    GDP89_PATTERN_RING = 1,
    GDP89_PATTERN_HASHED = 2
};

enum GDP89_DamageModeTag {
    GDP89_DAMAGE_PER_PROJECTILE = 0,
    GDP89_DAMAGE_SPLIT_TOTAL = 1
};

enum GDP89_ProjectileFlagsTag {
    GDP89_PROJECTILE_FIRST = 1,
    GDP89_PROJECTILE_LAST = 2,
    GDP89_PROJECTILE_CENTER = 4
};

typedef struct GDP89_RequestTag {
    int mode;
    int pattern;
    int damage_mode;
    int projectile_count;
    int center_first;
    int shot_seed;

    int actor_id;
    int actor_kind;
    int team_id;
    int weapon_id;
    int projectile_id;
    int user_tag;

    GDP89_Vec3 origin;
    GDP89_Vec3 forward;
    GDP89_Vec3 right;
    GDP89_Vec3 up;

    GDP89_Fx spread_fx;
    GDP89_Fx damage_fx;
    GDP89_Fx speed_fx;
    GDP89_Fx max_distance_fx;
    unsigned long life_ms;
} GDP89_Request;

typedef struct GDP89_ProjectileTag {
    int flags;
    int projectile_index;
    int projectile_count;

    int actor_id;
    int actor_kind;
    int team_id;
    int weapon_id;
    int projectile_id;
    int user_tag;

    GDP89_Vec3 origin;
    GDP89_Vec3 direction;

    GDP89_Fx damage_fx;
    GDP89_Fx speed_fx;
    GDP89_Fx max_distance_fx;
    unsigned long life_ms;
} GDP89_Projectile;

typedef int (*GDP89_EmitFn)(void *ctx, const GDP89_Projectile *projectile);

GDP89_Fx   gdp89_fx_from_int(int value);
GDP89_Fx   gdp89_spread_from_degrees(GDP89_Fx degrees_fx);
int        gdp89_fx_to_int_round(GDP89_Fx value);
GDP89_Vec3 gdp89_v3(GDP89_Fx x, GDP89_Fx y, GDP89_Fx z);
GDP89_Vec3 gdp89_v3_normalize(GDP89_Vec3 value);

void gdp89_request_defaults(GDP89_Request *request);
int  gdp89_projectile_count(const GDP89_Request *request);
int  gdp89_build(const GDP89_Request *request,
                 GDP89_Projectile *out_projectiles,
                 int out_capacity);
int  gdp89_emit(const GDP89_Request *request,
                GDP89_EmitFn emit_fn,
                void *emit_ctx);

#ifdef __cplusplus
}
#endif

#endif
