#ifndef FCASING89_H
#define FCASING89_H

/*
   FCASING89 - shooter-agnostic shell casing visual system.
   C89, fixed-point, no malloc/free/realloc/heap, no float/double.

   Unit convention:
   - All distances are fixed-point Q8 world units.
   - Velocities are Q8 world units / second.
   - Accelerations are Q8 world units / second^2.
   - Basis vectors are Q8 normalized direction vectors.
   - Rotation values are engine-defined Q8 angle units.
   - Scale is Q8, where FCASING89_FIX_ONE means 1.0.
*/

#ifdef __cplusplus
extern "C" {
#endif

#define FCASING89_VERSION_MAJOR 1
#define FCASING89_VERSION_MINOR 1
#define FCASING89_VERSION_PATCH 0

#ifndef FCASING89_MAX_CASINGS
#define FCASING89_MAX_CASINGS 96
#endif

#ifndef FCASING89_MAX_PROFILES
#define FCASING89_MAX_PROFILES 16
#endif

#ifndef FCASING89_MAX_EVENTS
#define FCASING89_MAX_EVENTS 16
#endif

#define FCASING89_FIX_SHIFT 8
#define FCASING89_FIX_ONE   (1L << FCASING89_FIX_SHIFT)
#define FCASING89_FIX_HALF  (1L << (FCASING89_FIX_SHIFT - 1))

#define FCASING89_MODE_AUTO      0
#define FCASING89_MODE_SIM       1
#define FCASING89_MODE_FAKE      2
#define FCASING89_MODE_OFF       3

#define FCASING89_STATE_FREE     0
#define FCASING89_STATE_SIM      1
#define FCASING89_STATE_FAKE     2
#define FCASING89_STATE_SLEEP    3

#define FCASING89_FLAG_SUPPRESSED_SOUND 1
#define FCASING89_FLAG_FORCE_SIM        2
#define FCASING89_FLAG_FORCE_FAKE       4
#define FCASING89_FLAG_NO_BOUNCE_SOUND  8
#define FCASING89_FLAG_CUSTOM_SCALE     16

#define FCASING89_EVENT_NONE      0
#define FCASING89_EVENT_BOUNCE    1
#define FCASING89_EVENT_DESPAWN   2
#define FCASING89_EVENT_BUDGET    3

#define FCASING89_PROFILE_PISTOL  0
#define FCASING89_PROFILE_RIFLE   1
#define FCASING89_PROFILE_SHOTGUN 2
#define FCASING89_PROFILE_HEAVY   3
#define FCASING89_PROFILE_TINY    4

#define FCASING89_PROFILE_NAME_LEN 24

#define FCASING89_PROVIDER_GRAVITY   1UL
#define FCASING89_PROVIDER_MOVE      2UL
#define FCASING89_PROVIDER_ROTATE    4UL
#define FCASING89_PROVIDER_SCALE     8UL
#define FCASING89_PROVIDER_COLLISION 16UL
#define FCASING89_PROVIDER_ALL       31UL

#define FCASING89_TRANSFORM_MOVE   1
#define FCASING89_TRANSFORM_ROTATE 2
#define FCASING89_TRANSFORM_SCALE  3

#define FCASING89_COLLISION_HIT             1
#define FCASING89_COLLISION_SLEEP           2
#define FCASING89_COLLISION_VELOCITY_VALID  4
#define FCASING89_COLLISION_NO_BOUNCE_EVENT 8

/* Q8 helpers for call sites that want readable integer constants. */
#define FCASING89_TO_FIX(x) ((long)((x) << FCASING89_FIX_SHIFT))
#define FCASING89_FROM_FIX(x) ((long)((x) >> FCASING89_FIX_SHIFT))

typedef long fc89_i32;
typedef unsigned long fc89_u32;
typedef unsigned short fc89_u16;
typedef unsigned char fc89_u8;
typedef signed char fc89_s8;

typedef struct FCasing89Vec3 {
    fc89_i32 x;
    fc89_i32 y;
    fc89_i32 z;
} FCasing89Vec3;

typedef struct FCasing89Profile {
    char name[FCASING89_PROFILE_NAME_LEN];

    fc89_i32 side_min;
    fc89_i32 side_max;
    fc89_i32 up_min;
    fc89_i32 up_max;
    fc89_i32 back_min;
    fc89_i32 back_max;

    fc89_i32 spin_min;
    fc89_i32 spin_max;
    fc89_i32 gravity;
    fc89_i32 bounce_q;
    fc89_i32 friction_q;
    fc89_i32 radius;
    fc89_i32 floor_y;

    fc89_u16 lifetime_ms;
    fc89_u16 sleep_ms;
    fc89_u16 fade_ms;
    fc89_u16 min_bounce_speed;

    fc89_u8 count_per_emit;
    fc89_u8 max_bounces;
    fc89_u8 render_model_id;
    fc89_u8 audio_id;
    fc89_u8 mode_hint;
    fc89_u8 reserved0;
    fc89_u8 reserved1;
    fc89_u8 reserved2;
} FCasing89Profile;

typedef struct FCasing89Config {
    fc89_u16 max_simulated;
    fc89_u16 max_fake;
    fc89_u8 max_spawn_per_emit;
    fc89_u8 max_spawn_per_frame;
    fc89_u8 recycle_when_full;
    fc89_u8 default_mode;

    fc89_i32 near_distance;
    fc89_i32 fake_distance;
    fc89_i32 audio_distance;
    fc89_i32 global_floor_y;
} FCasing89Config;

typedef struct FCasing89Spawn {
    FCasing89Vec3 origin;
    FCasing89Vec3 forward;
    FCasing89Vec3 right;
    FCasing89Vec3 up;

    fc89_u16 profile_id;
    fc89_u8 count;
    fc89_u8 importance;
    fc89_u8 flags;
    fc89_u8 seed_bias;

    fc89_i32 local_side_bias;
    fc89_i32 local_up_bias;
    fc89_i32 local_back_bias;

    /*
       Read only when FCASING89_FLAG_CUSTOM_SCALE is set.
       Zero/negative components are accepted because the renderer/provider
       may intentionally use them.
    */
    FCasing89Vec3 scale;
} FCasing89Spawn;

typedef struct FCasing89Camera {
    FCasing89Vec3 pos;
    fc89_u8 valid;
    fc89_u8 reserved0;
    fc89_u16 reserved1;
} FCasing89Camera;

typedef struct FCasing89RenderItem {
    FCasing89Vec3 pos;
    FCasing89Vec3 rot;
    fc89_u16 profile_id;
    fc89_u8 render_model_id;
    fc89_u8 alpha;
    fc89_u8 state;
    fc89_u8 bounce_count;
    fc89_u16 age_ms;

    /* Added in 1.1.0; Q8 scale, FCASING89_FIX_ONE is 1.0. */
    FCasing89Vec3 scale;
} FCasing89RenderItem;

typedef struct FCasing89Event {
    fc89_u8 type;
    fc89_u8 audio_id;
    fc89_u16 casing_index;
    FCasing89Vec3 pos;
    fc89_u8 bounce_count;
    fc89_u8 reserved0;
    fc89_u16 reserved1;
} FCasing89Event;

typedef struct FCasing89Stats {
    fc89_u16 active_total;
    fc89_u16 active_sim;
    fc89_u16 active_fake;
    fc89_u16 free_count;
    fc89_u32 emitted_total;
    fc89_u32 recycled_total;
    fc89_u32 dropped_total;
    fc89_u32 budget_fake_total;
} FCasing89Stats;

/*
   Provider callbacks are optional and independent.

   Return non-zero when the provider handled the request.
   Return zero to make FCASING89 use its original internal fallback.

   The library copies this table into FCasing89System. The user pointer is
   caller-owned and may point to static storage, an arena, or an engine object.
   FCASING89 never allocates or frees it.
*/
typedef struct FCasing89ProviderGravityQuery {
    FCasing89Vec3 position;
    FCasing89Vec3 velocity;
    FCasing89Vec3 fallback_acceleration;
    fc89_u16 casing_index;
    fc89_u16 profile_id;
    fc89_u16 dt_ms;
    fc89_u8 state;
    fc89_u8 bounce_count;
} FCasing89ProviderGravityQuery;

typedef struct FCasing89ProviderTransformQuery {
    FCasing89Vec3 current;
    FCasing89Vec3 delta;
    FCasing89Vec3 source;
    fc89_u16 casing_index;
    fc89_u16 profile_id;
    fc89_u16 dt_ms;
    fc89_u8 operation;
    fc89_u8 state;
} FCasing89ProviderTransformQuery;

typedef struct FCasing89ProviderCollisionQuery {
    FCasing89Vec3 old_position;
    FCasing89Vec3 proposed_position;
    FCasing89Vec3 velocity;
    fc89_i32 radius;
    fc89_u16 casing_index;
    fc89_u16 profile_id;
    fc89_u16 dt_ms;
    fc89_u8 state;
    fc89_u8 bounce_count;
} FCasing89ProviderCollisionQuery;

typedef struct FCasing89ProviderCollisionResult {
    FCasing89Vec3 position;
    FCasing89Vec3 normal;
    FCasing89Vec3 velocity;
    fc89_u8 flags;
    fc89_u8 reserved0;
    fc89_u16 reserved1;
} FCasing89ProviderCollisionResult;

typedef int (*FCasing89ProviderGravityFn)(
    void *user,
    const FCasing89ProviderGravityQuery *query,
    FCasing89Vec3 *out_acceleration);

typedef int (*FCasing89ProviderTransformFn)(
    void *user,
    const FCasing89ProviderTransformQuery *query,
    FCasing89Vec3 *out_value);

typedef int (*FCasing89ProviderCollisionFn)(
    void *user,
    const FCasing89ProviderCollisionQuery *query,
    FCasing89ProviderCollisionResult *out_result);

typedef struct FCasing89Provider {
    void *user;
    fc89_u32 enabled_mask;
    FCasing89ProviderGravityFn gravity;
    FCasing89ProviderTransformFn move;
    FCasing89ProviderTransformFn rotate;
    FCasing89ProviderTransformFn scale;
    FCasing89ProviderCollisionFn collision;
} FCasing89Provider;

typedef struct FCasing89Node {
    fc89_u8 state;
    fc89_u8 flags;
    fc89_u8 bounce_count;
    fc89_u8 alpha;
    fc89_u16 profile_id;
    fc89_u16 next_free;
    fc89_u16 age_ms;
    fc89_u16 sleep_age_ms;
    fc89_u16 order;
    fc89_u8 importance;
    fc89_u8 reserved0;
    FCasing89Vec3 pos;
    FCasing89Vec3 vel;
    FCasing89Vec3 rot;
    FCasing89Vec3 spin;

    /* Added in 1.1.0. */
    FCasing89Vec3 scale;
} FCasing89Node;

typedef struct FCasing89System {
    FCasing89Config cfg;
    FCasing89Profile profiles[FCASING89_MAX_PROFILES];
    FCasing89Node nodes[FCASING89_MAX_CASINGS];
    FCasing89Event events[FCASING89_MAX_EVENTS];
    FCasing89Camera camera;
    FCasing89Stats stats;
    fc89_u16 first_free;
    fc89_u16 event_head;
    fc89_u16 event_tail;
    fc89_u16 order_counter;
    fc89_u8 frame_spawned;
    fc89_u8 initialized;
    fc89_u16 reserved0;
    fc89_u32 rng;

    /* Added in 1.1.0. Copied by fcasing89_set_provider(). */
    FCasing89Provider provider;
} FCasing89System;

void fcasing89_default_config(FCasing89Config *cfg);
void fcasing89_default_profile(FCasing89Profile *profile, fc89_u16 type_id);
void fcasing89_provider_init(FCasing89Provider *provider);
void fcasing89_init(FCasing89System *sys, const FCasing89Config *cfg, fc89_u32 seed);
void fcasing89_reset(FCasing89System *sys);
void fcasing89_begin_frame(FCasing89System *sys);
void fcasing89_set_camera(FCasing89System *sys, const FCasing89Camera *camera);
void fcasing89_set_provider(FCasing89System *sys, const FCasing89Provider *provider);
void fcasing89_remove_provider(FCasing89System *sys);
fc89_u32 fcasing89_get_provider_mask(const FCasing89System *sys);
int fcasing89_set_profile(FCasing89System *sys, fc89_u16 profile_id, const FCasing89Profile *profile);
int fcasing89_emit(FCasing89System *sys, const FCasing89Spawn *spawn);
void fcasing89_update(FCasing89System *sys, fc89_u16 dt_ms);
int fcasing89_collect_render_items(const FCasing89System *sys, FCasing89RenderItem *out_items, int max_items);
int fcasing89_pop_event(FCasing89System *sys, FCasing89Event *out_event);
void fcasing89_get_stats(const FCasing89System *sys, FCasing89Stats *out_stats);

FCasing89Vec3 fcasing89_vec3(fc89_i32 x, fc89_i32 y, fc89_i32 z);
FCasing89Vec3 fcasing89_vec3_add(FCasing89Vec3 a, FCasing89Vec3 b);
FCasing89Vec3 fcasing89_vec3_scale_q(FCasing89Vec3 v, fc89_i32 q);
FCasing89Vec3 fcasing89_vec3_mul_q(FCasing89Vec3 a, FCasing89Vec3 b);
FCasing89Vec3 fcasing89_vec3_add_scaled_q(FCasing89Vec3 base, FCasing89Vec3 dir, fc89_i32 q);
fc89_i32 fcasing89_vec3_dot_q(FCasing89Vec3 a, FCasing89Vec3 b);
fc89_i32 fcasing89_fix_mul(fc89_i32 a, fc89_i32 b);
fc89_i32 fcasing89_abs_i32(fc89_i32 v);
fc89_i32 fcasing89_manhattan_units(FCasing89Vec3 a, FCasing89Vec3 b);

#ifdef __cplusplus
}
#endif

#endif
