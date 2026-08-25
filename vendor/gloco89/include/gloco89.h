#ifndef GLOCO89_H
#define GLOCO89_H

/*
   gloco89 - C89 fixed-point 3D locomotion kernel
   caminar / correr / sprint / strafe / evade / slide-lite

   Constraints:
   - C89
   - no malloc/realloc/free
   - no float/double
   - no heap ownership
   - deterministic fixed point, Q8.8
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef GLOCO_MAX_ACTORS
#define GLOCO_MAX_ACTORS      64
#endif
#ifndef GLOCO_MAX_PROFILES
#define GLOCO_MAX_PROFILES    GLOCO_MAX_ACTORS
#endif
#define GLOCO_FX_SHIFT         8
#define GLOCO_FX_ONE         256L
#define GLOCO_FX_HALF        128L
#define GLOCO_TRUE             1
#define GLOCO_FALSE            0

#define GLOCO_INPUT_WALK       0x0001u
#define GLOCO_INPUT_RUN        0x0002u
#define GLOCO_INPUT_SPRINT     0x0004u
#define GLOCO_INPUT_AIM        0x0008u
#define GLOCO_INPUT_EVADE      0x0010u
#define GLOCO_INPUT_CROUCH     0x0020u
#define GLOCO_INPUT_SLIDE      0x0040u
#define GLOCO_INPUT_HARD_STOP  0x0080u

#define GLOCO_FLAG_GROUNDED    0x0001u
#define GLOCO_FLAG_BLOCKED     0x0002u
#define GLOCO_FLAG_STEEP       0x0004u
#define GLOCO_FLAG_EVADING     0x0008u
#define GLOCO_FLAG_SPRINTING   0x0010u
#define GLOCO_FLAG_AIMING      0x0020u
#define GLOCO_FLAG_SLIDING     0x0040u
#define GLOCO_FLAG_STEP_LEFT   0x0080u
#define GLOCO_FLAG_STEP_RIGHT  0x0100u

#define GLOCO_STATE_IDLE       0
#define GLOCO_STATE_WALK       1
#define GLOCO_STATE_RUN        2
#define GLOCO_STATE_SPRINT     3
#define GLOCO_STATE_AIM_MOVE   4
#define GLOCO_STATE_EVADE      5
#define GLOCO_STATE_SLIDE      6
#define GLOCO_STATE_AIR        7

#define GLOCO_EVENT_STATE      1
#define GLOCO_EVENT_STEP_L     2
#define GLOCO_EVENT_STEP_R     3
#define GLOCO_EVENT_EVADE_ON   4
#define GLOCO_EVENT_EVADE_OFF  5
#define GLOCO_EVENT_HARD_STOP  6
#define GLOCO_EVENT_LAND       7

#define GLOCO_PROFILE_DEFAULT  0
#define GLOCO_PROFILE_TACTICAL 1
#define GLOCO_PROFILE_ARCADE   2
#define GLOCO_PROFILE_HEAVY    3

#define GLOCO_OK               0
#define GLOCO_ERR_FULL        -1
#define GLOCO_ERR_BAD_ID      -2
#define GLOCO_ERR_BAD_PROFILE -3

#define GLOCO_PROVIDER_FALLBACK 0
#define GLOCO_PROVIDER_HANDLED  1

#define GLOCO_MOTION_MODE_MOVE      0
#define GLOCO_MOTION_MODE_BRAKE     1
#define GLOCO_MOTION_MODE_HARD_STOP 2
#define GLOCO_MOTION_MODE_EVADE     3
#define GLOCO_MOTION_MODE_SLIDE     4

#define GLOCO_IMPULSE_EVADE         1

/* Q8.8 fixed point. Example: 1.0 = 256, 2.5 = 640. */
typedef signed long GLOCO_FX;
typedef unsigned char GLOCO_U8;
typedef unsigned short GLOCO_U16;
typedef signed short GLOCO_S16;
typedef signed long GLOCO_I32;

typedef struct GLOCO_Vec3Tag {
    GLOCO_FX x;
    GLOCO_FX y;
    GLOCO_FX z;
} GLOCO_Vec3;

typedef struct GLOCO_InputTag {
    GLOCO_S16 move_x;       /* -256..256 local/camera right axis */
    GLOCO_S16 move_z;       /* -256..256 local/camera forward axis */
    GLOCO_S16 evade_x;      /* optional evade direction, -256..256 */
    GLOCO_S16 evade_z;
    GLOCO_U16 buttons;      /* GLOCO_INPUT_* */
    GLOCO_FX speed_override; /* >0 overrides profile locomotion speed */
    GLOCO_Vec3 basis_fwd;   /* camera/actor/world basis, Y ignored by default */
    GLOCO_Vec3 basis_right; /* camera/actor/world basis, Y ignored by default */
} GLOCO_Input;

typedef struct GLOCO_ProfileTag {
    GLOCO_FX walk_speed;
    GLOCO_FX run_speed;
    GLOCO_FX sprint_speed;
    GLOCO_FX aim_speed;
    GLOCO_FX crouch_speed;

    GLOCO_FX acceleration;
    GLOCO_FX sprint_acceleration;
    GLOCO_FX braking;
    GLOCO_FX hard_braking;
    GLOCO_FX ground_friction;
    GLOCO_FX air_control;

    GLOCO_FX side_scale;
    GLOCO_FX back_scale;
    GLOCO_FX aim_side_scale;

    GLOCO_FX turn_rate;       /* vector turn, units per sec, fixed */
    GLOCO_FX yaw_lag;         /* small steering smoothing */

    GLOCO_FX gravity;
    GLOCO_FX terminal_fall;
    GLOCO_FX ground_snap;
    GLOCO_FX max_slope_dot;   /* dot(up,normal) threshold, fixed */
    GLOCO_FX capsule_radius;
    GLOCO_FX capsule_height;

    GLOCO_FX evade_speed;
    GLOCO_U16 evade_active_ms;
    GLOCO_U16 evade_recover_ms;
    GLOCO_FX evade_friction;
    GLOCO_FX evade_control;
    GLOCO_FX evade_stamina_cost;

    GLOCO_FX slide_min_speed;
    GLOCO_FX slide_friction;
    GLOCO_U16 slide_ms;

    GLOCO_FX stamina_max;
    GLOCO_FX stamina_sprint_drain;
    GLOCO_FX stamina_recover;
    GLOCO_FX stamina_min_sprint;

    GLOCO_FX stride_walk;
    GLOCO_FX stride_run;
    GLOCO_FX stride_sprint;
} GLOCO_Profile;

typedef struct GLOCO_ProbeResultTag {
    GLOCO_Vec3 corrected_pos;
    GLOCO_Vec3 ground_normal;
    GLOCO_U16 flags;
} GLOCO_ProbeResult;

typedef int (*GLOCO_WorldProbeFn)(void *user,
                                  const GLOCO_Vec3 *from,
                                  const GLOCO_Vec3 *to,
                                  GLOCO_FX radius,
                                  GLOCO_FX height,
                                  GLOCO_ProbeResult *out_result);

typedef void (*GLOCO_EventFn)(void *user, int actor_id, int event_id, int value);

typedef struct GLOCO_ActorTag {
    GLOCO_U8 alive;
    GLOCO_U8 profile_id;
    GLOCO_U8 state;
    GLOCO_U8 prev_state;
    GLOCO_U16 flags;

    GLOCO_Vec3 pos;
    GLOCO_Vec3 vel;
    GLOCO_Vec3 facing;
    GLOCO_Vec3 ground_normal;

    GLOCO_U16 evade_timer_ms;
    GLOCO_U16 evade_recover_ms;
    GLOCO_U16 slide_timer_ms;
    GLOCO_U8 evade_consumed;
    GLOCO_U8 step_side;
    GLOCO_U8 hard_stop_latched;

    GLOCO_FX stamina;
    GLOCO_FX stride_phase;
    GLOCO_FX last_ground_y;
} GLOCO_Actor;

/*
   Provider-facing motion request. gloco89 still owns locomotion policy/state;
   a movement backend may consume the request and write actor velocity/facing.
*/
typedef struct GLOCO_MotionIntentTag {
    GLOCO_Vec3 desired_dir;
    GLOCO_Vec3 target_velocity;
    GLOCO_FX target_speed;
    GLOCO_FX acceleration;
    GLOCO_FX friction;
    GLOCO_FX turn_rate;
    GLOCO_U16 buttons;
    GLOCO_U8 mode;
    GLOCO_U8 has_input;
} GLOCO_MotionIntent;

typedef int (*GLOCO_MovementHorizontalFn)(void *user,
                                           int actor_id,
                                           GLOCO_Actor *actor,
                                           const GLOCO_Profile *profile,
                                           const GLOCO_Input *input,
                                           const GLOCO_MotionIntent *intent,
                                           GLOCO_U16 dt_ms);

typedef int (*GLOCO_MovementVerticalFn)(void *user,
                                         int actor_id,
                                         GLOCO_Actor *actor,
                                         const GLOCO_Profile *profile,
                                         GLOCO_U16 dt_ms);

typedef int (*GLOCO_MovementIntegrateFn)(void *user,
                                          int actor_id,
                                          const GLOCO_Actor *actor,
                                          GLOCO_U16 dt_ms,
                                          const GLOCO_Vec3 *from,
                                          const GLOCO_Vec3 *builtin_to,
                                          GLOCO_Vec3 *out_to);

typedef int (*GLOCO_MovementImpulseFn)(void *user,
                                        int actor_id,
                                        GLOCO_Actor *actor,
                                        int impulse_id,
                                        const GLOCO_Vec3 *builtin_velocity);

typedef void (*GLOCO_MovementStopFn)(void *user,
                                      int actor_id,
                                      GLOCO_Actor *actor);

typedef struct GLOCO_MovementProviderTag {
    void *user;
    GLOCO_MovementHorizontalFn horizontal;
    GLOCO_MovementVerticalFn vertical;
    GLOCO_MovementIntegrateFn integrate;
    GLOCO_MovementImpulseFn impulse;
    GLOCO_MovementStopFn stop;
} GLOCO_MovementProvider;

typedef int (*GLOCO_PhysicsMoveFn)(void *user,
                                    int actor_id,
                                    GLOCO_Actor *actor,
                                    const GLOCO_Profile *profile,
                                    GLOCO_U16 dt_ms,
                                    const GLOCO_Vec3 *from,
                                    const GLOCO_Vec3 *to,
                                    GLOCO_ProbeResult *out_result);

typedef void (*GLOCO_PhysicsActorFn)(void *user,
                                      int actor_id,
                                      const GLOCO_Actor *actor,
                                      const GLOCO_Profile *profile);

typedef void (*GLOCO_PhysicsTeleportFn)(void *user,
                                         int actor_id,
                                         const GLOCO_Actor *actor,
                                         const GLOCO_Vec3 *position);

typedef void (*GLOCO_PhysicsPostMoveFn)(void *user,
                                         int actor_id,
                                         GLOCO_Actor *actor,
                                         const GLOCO_Profile *profile,
                                         const GLOCO_ProbeResult *result);

typedef struct GLOCO_PhysicsProviderTag {
    void *user;
    GLOCO_PhysicsMoveFn move;
    GLOCO_PhysicsActorFn actor_create;
    GLOCO_PhysicsActorFn actor_destroy;
    GLOCO_PhysicsTeleportFn teleport;
    GLOCO_PhysicsPostMoveFn post_move;
} GLOCO_PhysicsProvider;

/* Optional external numeric authority. The actor field remains a mirror so
   standalone code and inspection keep working. Providers return HANDLED when
   they own the value, FALLBACK to use the actor-local field. */
typedef int (*GLOCO_StatsGetStaminaFn)(void *user, int actor_id,
                                        GLOCO_FX *out_stamina);
typedef int (*GLOCO_StatsSetStaminaFn)(void *user, int actor_id,
                                        GLOCO_FX stamina);
typedef int (*GLOCO_StatsConsumeStaminaFn)(void *user, int actor_id,
                                            GLOCO_FX amount,
                                            GLOCO_FX *out_stamina);

typedef struct GLOCO_StatsProviderTag {
    void *user;
    GLOCO_StatsGetStaminaFn get_stamina;
    GLOCO_StatsSetStaminaFn set_stamina;
    GLOCO_StatsConsumeStaminaFn consume_stamina;
} GLOCO_StatsProvider;

typedef struct GLOCO_ContextTag {
    GLOCO_Actor actors[GLOCO_MAX_ACTORS];
    GLOCO_Profile profiles[GLOCO_MAX_PROFILES];
    GLOCO_U8 profile_alive[GLOCO_MAX_PROFILES];
    GLOCO_WorldProbeFn world_probe;
    GLOCO_EventFn event_fn;
    void *user;
    GLOCO_MovementProvider movement_provider;
    GLOCO_PhysicsProvider physics_provider;
    GLOCO_StatsProvider stats_provider;
} GLOCO_Context;

GLOCO_FX gloco_fx_from_int(int v);
int gloco_fx_to_int(GLOCO_FX v);
GLOCO_FX gloco_fx_mul(GLOCO_FX a, GLOCO_FX b);
GLOCO_FX gloco_fx_div(GLOCO_FX a, GLOCO_FX b);
GLOCO_FX gloco_fx_abs(GLOCO_FX v);
GLOCO_FX gloco_fx_clamp(GLOCO_FX v, GLOCO_FX lo, GLOCO_FX hi);

GLOCO_Vec3 gloco_v3(GLOCO_FX x, GLOCO_FX y, GLOCO_FX z);
GLOCO_Vec3 gloco_v3_add(GLOCO_Vec3 a, GLOCO_Vec3 b);
GLOCO_Vec3 gloco_v3_sub(GLOCO_Vec3 a, GLOCO_Vec3 b);
GLOCO_Vec3 gloco_v3_scale(GLOCO_Vec3 a, GLOCO_FX s);
GLOCO_FX gloco_v3_dot(GLOCO_Vec3 a, GLOCO_Vec3 b);
GLOCO_FX gloco_v3_len_approx(GLOCO_Vec3 a);
GLOCO_Vec3 gloco_v3_norm_approx(GLOCO_Vec3 a);
GLOCO_Vec3 gloco_v3_move_towards(GLOCO_Vec3 cur, GLOCO_Vec3 target, GLOCO_FX max_delta);

void gloco_init(GLOCO_Context *ctx);
void gloco_set_callbacks(GLOCO_Context *ctx, GLOCO_WorldProbeFn probe, GLOCO_EventFn event_fn, void *user);
void gloco_movement_provider_init(GLOCO_MovementProvider *provider);
void gloco_physics_provider_init(GLOCO_PhysicsProvider *provider);
void gloco_stats_provider_init(GLOCO_StatsProvider *provider);
void gloco_set_movement_provider(GLOCO_Context *ctx, const GLOCO_MovementProvider *provider);
void gloco_set_physics_provider(GLOCO_Context *ctx, const GLOCO_PhysicsProvider *provider);
void gloco_set_stats_provider(GLOCO_Context *ctx, const GLOCO_StatsProvider *provider);
void gloco_profile_defaults(GLOCO_Profile *p, int preset);
int gloco_set_profile(GLOCO_Context *ctx, int profile_id, const GLOCO_Profile *profile);
int gloco_actor_create(GLOCO_Context *ctx, int profile_id, const GLOCO_Vec3 *pos, const GLOCO_Vec3 *facing);
int gloco_actor_destroy(GLOCO_Context *ctx, int actor_id);
GLOCO_Actor *gloco_actor_get(GLOCO_Context *ctx, int actor_id);
const GLOCO_Actor *gloco_actor_get_const(const GLOCO_Context *ctx, int actor_id);
int gloco_actor_set_profile(GLOCO_Context *ctx, int actor_id, int profile_id);
int gloco_actor_teleport(GLOCO_Context *ctx, int actor_id, const GLOCO_Vec3 *pos);
int gloco_actor_stop(GLOCO_Context *ctx, int actor_id);
int gloco_update_actor(GLOCO_Context *ctx, int actor_id, const GLOCO_Input *input, GLOCO_U16 dt_ms);
void gloco_update_all(GLOCO_Context *ctx, const GLOCO_Input *inputs, GLOCO_U16 dt_ms);

const char *gloco_state_name(int state_id);

#ifdef __cplusplus
}
#endif

#endif
