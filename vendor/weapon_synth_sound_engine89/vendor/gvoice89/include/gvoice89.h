#ifndef GVOICE89_H
#define GVOICE89_H

/*
 * gvoice89 v2.0
 * Heapless C89 logical/physical voice manager and fixed-point stereo mixer.
 *
 * Core features:
 *   - 256 logical voices by default, up to 65534 caller-owned slots
 *   - configurable physical voice budget (up to 256 mixed voices)
 *   - virtual voice modes: continue, advance, pause, restart, kill
 *   - priority + audibility ranking with hysteresis and hold protection
 *   - deterministic voice stealing, per-group and per-instance limits
 *   - per-group physical reserves, 16 lightweight buses
 *   - anti-click attack/release and stolen-tail handoff
 *   - integer stereo limiter and extensive telemetry
 *   - no malloc/realloc/free, no float/double, no stdio/math dependency
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef signed short gv89_s16;
typedef signed int gv89_s32;
typedef unsigned int gv89_u32;
typedef unsigned short gv89_u16;
typedef unsigned char gv89_u8;

#define GV89_VERSION_MAJOR 2
#define GV89_VERSION_MINOR 0
#define GV89_VERSION_PATCH 0

#define GV89_Q15_ONE 32767
#define GV89_PAN_LEFT (-32767)
#define GV89_PAN_CENTER 0
#define GV89_PAN_RIGHT 32767
#define GV89_INVALID_INDEX 65535U
#define GV89_CAP_ANY 0xFFFFFFFFU

#define GV89_RECOMMENDED_LOGICAL_VOICES 256U
#define GV89_RECOMMENDED_PHYSICAL_VOICES 64U
#define GV89_MAX_PHYSICAL_VOICES 256U
#define GV89_MAX_TAIL_VOICES 256U
#define GV89_MAX_GROUP_RULES 32U
#define GV89_MAX_BUSES 16U

#define GV89_FLAG_NONE 0U
#define GV89_FLAG_NEVER_STEAL 1U
#define GV89_FLAG_NEVER_VIRTUAL 2U
#define GV89_FLAG_CRITICAL 4U
#define GV89_FLAG_LOOPING 8U
#define GV89_FLAG_ALLOW_PROTECTED_STEAL 16U

#define GV89_VOICE_INACTIVE 0U
#define GV89_VOICE_PHYSICAL 1U
#define GV89_VOICE_VIRTUAL 2U
#define GV89_VOICE_RESERVED 3U

typedef enum gv89_result_e {
    GV89_OK = 0,
    GV89_BAD_ARGUMENT = -1,
    GV89_NO_VOICE = -2,
    GV89_STALE_HANDLE = -3,
    GV89_NOT_RESERVED = -4,
    GV89_GROUP_LIMIT = -5,
    GV89_INSTANCE_LIMIT = -6,
    GV89_PHYSICAL_LIMIT = -7
} gv89_result;

#define GV89_STEAL_PRIORITY_QUIETEST GV89_STEAL_PRIORITY_AUDIBILITY

typedef enum gv89_steal_policy_e {
    GV89_STEAL_PRIORITY_AUDIBILITY = 0,
    GV89_STEAL_OLDEST = 1,
    GV89_STEAL_QUIETEST = 2,
    GV89_STEAL_REJECT = 3,
    GV89_STEAL_NEWEST = 4,
    GV89_STEAL_FURTHEST = 5,
    GV89_STEAL_HYBRID = 6
} gv89_steal_policy;

typedef enum gv89_virtual_behavior_e {
    GV89_VIRTUAL_CONTINUE = 0,
    GV89_VIRTUAL_ADVANCE = 1,
    GV89_VIRTUAL_PAUSE = 2,
    GV89_VIRTUAL_RESTART = 3,
    GV89_VIRTUAL_KILL = 4
} gv89_virtual_behavior;

typedef gv89_s16 (*gv89_process_mono_fn)(void *user);
typedef int (*gv89_is_active_fn)(const void *user);
typedef void (*gv89_stop_fn)(void *user);
typedef void (*gv89_advance_frames_fn)(void *user, gv89_u32 frames);
typedef void (*gv89_restart_fn)(void *user);
typedef gv89_u16 (*gv89_estimated_level_fn)(const void *user);
typedef void (*gv89_physical_state_fn)(void *user, int is_physical);

typedef struct gv89_provider_s {
    void *user;
    gv89_process_mono_fn process_mono;
    gv89_is_active_fn is_active;
    gv89_stop_fn stop;
} gv89_provider;

typedef struct gv89_provider_ex_s {
    gv89_provider base;
    gv89_advance_frames_fn advance_frames;
    gv89_restart_fn restart;
    gv89_estimated_level_fn estimated_level_q15;
    gv89_physical_state_fn physical_state_changed;
} gv89_provider_ex;

typedef struct gv89_handle_s {
    gv89_u16 index;
    gv89_u16 generation;
} gv89_handle;

typedef struct gv89_request_s {
    gv89_u16 priority;
    gv89_u16 group_id;
    gv89_u32 required_capabilities;
    gv89_steal_policy steal_policy;
    gv89_virtual_behavior virtual_behavior;
    gv89_u32 flags;
    gv89_u32 instance_key;
    gv89_u16 instance_limit;
    gv89_u16 bus_id;
    gv89_u16 steal_protect_ms;
    gv89_u16 minimum_physical_ms;
    gv89_u32 virtual_timeout_ms;
    gv89_u8 allow_higher_priority_steal;
    gv89_u8 allow_protected_steal;
} gv89_request;

typedef struct gv89_voice_params_s {
    gv89_s16 gain_q15;
    gv89_s16 pan_q15;
    gv89_u16 audibility_q15;
    gv89_u16 attack_ms;
    gv89_u16 release_ms;
} gv89_voice_params;

typedef struct gv89_reservation_s {
    gv89_u16 index;
    gv89_u16 generation;
    gv89_u8 valid;
} gv89_reservation;

typedef struct gv89_group_rule_s {
    gv89_u16 group_id;
    gv89_u16 max_logical;
    gv89_u16 physical_reserve;
    gv89_steal_policy steal_policy;
    gv89_u8 used;
} gv89_group_rule;

typedef struct gv89_bus_s {
    gv89_s16 gain_q15;
    gv89_s16 priority_bias;
    gv89_u8 mute;
} gv89_bus;

typedef struct gv89_voice_s {
    gv89_provider_ex provider;
    gv89_u32 capabilities;
    gv89_u32 flags;
    gv89_u32 instance_key;
    gv89_u32 age_frames;
    gv89_u32 virtual_age_frames;
    gv89_u32 physical_age_frames;
    gv89_u32 serial;
    gv89_u32 last_abs;
    gv89_u32 attack_frames;
    gv89_u32 attack_pos;
    gv89_u32 release_frames;
    gv89_u32 release_pos;
    gv89_u32 steal_protect_frames;
    gv89_u32 minimum_physical_frames;
    gv89_u32 virtual_timeout_frames;
    gv89_u16 generation;
    gv89_u16 priority;
    gv89_u16 group_id;
    gv89_u16 bus_id;
    gv89_u16 audibility_q15;
    gv89_s16 gain_q15;
    gv89_s16 pan_q15;
    gv89_s16 last_l;
    gv89_s16 last_r;
    gv89_s16 tail_l;
    gv89_s16 tail_r;
    gv89_u16 tail_frames;
    gv89_u16 tail_pos;
    gv89_virtual_behavior virtual_behavior;
    gv89_u8 active;
    gv89_u8 physical;
    gv89_u8 releasing;
    gv89_u8 reserved;
    gv89_u8 selected;
    gv89_u8 needs_restart;
} gv89_voice;

typedef struct gv89_stats_s {
    gv89_u32 starts;
    gv89_u32 natural_ends;
    gv89_u32 steals;
    gv89_u32 group_steals;
    gv89_u32 instance_steals;
    gv89_u32 rejects;
    gv89_u32 protected_rejects;
    gv89_u32 peak_logical;
    gv89_u32 peak_active;
    gv89_u32 peak_physical;
    gv89_u32 promotions;
    gv89_u32 demotions;
    gv89_u32 virtual_kills;
    gv89_u32 virtual_timeouts;
    gv89_u32 forced_virtuals;
    gv89_u32 rebalance_passes;
    gv89_u32 limiter_hits;
} gv89_stats;

typedef struct gv89_context_s {
    gv89_voice *voices;
    gv89_u16 capacity;
    gv89_u16 active_count;
    gv89_u16 physical_limit;
    gv89_u16 physical_count;
    gv89_u16 physical_indices[GV89_MAX_PHYSICAL_VOICES];
    gv89_u16 tail_indices[GV89_MAX_TAIL_VOICES];
    gv89_u16 tail_count;
    gv89_u32 sample_rate;
    gv89_u32 serial_counter;
    gv89_u32 rebalance_interval_frames;
    gv89_u32 rebalance_countdown;
    gv89_u16 physical_hysteresis_q15;
    gv89_u16 inaudible_threshold_q15;
    gv89_group_rule groups[GV89_MAX_GROUP_RULES];
    gv89_u16 group_rule_count;
    gv89_bus buses[GV89_MAX_BUSES];
    gv89_s16 master_gain_q15;
    gv89_s16 limiter_threshold;
    gv89_s16 limiter_gain_q15;
    gv89_u8 limiter_release_shift;
    gv89_u16 anti_click_frames;
    gv89_u16 batch_depth;
    gv89_u8 rebalance_pending;
    gv89_stats stats;
} gv89_context;

void gv89_provider_ex_from_basic(gv89_provider_ex *out_provider,
                                  const gv89_provider *basic);
void gv89_request_default(gv89_request *request);
void gv89_voice_params_default(gv89_voice_params *params);

gv89_result gv89_init(gv89_context *ctx,
                        gv89_voice *voice_storage,
                        gv89_u16 capacity,
                        gv89_u32 sample_rate);

gv89_result gv89_set_physical_limit(gv89_context *ctx, gv89_u16 limit);
void gv89_set_virtualization(gv89_context *ctx,
                              gv89_u16 inaudible_threshold_q15,
                              gv89_u16 physical_hysteresis_q15,
                              gv89_u16 rebalance_interval_frames);
void gv89_set_slot_capabilities(gv89_context *ctx,
                                 gv89_u16 index,
                                 gv89_u32 capabilities);

gv89_result gv89_set_group_rule(gv89_context *ctx,
                                  gv89_u16 group_id,
                                  gv89_u16 max_logical,
                                  gv89_u16 physical_reserve,
                                  gv89_steal_policy policy);
gv89_result gv89_set_group_limit(gv89_context *ctx,
                                  gv89_u16 group_id,
                                  gv89_u16 max_voices,
                                  gv89_steal_policy policy);

gv89_result gv89_set_bus(gv89_context *ctx,
                           gv89_u16 bus_id,
                           gv89_s16 gain_q15,
                           gv89_s16 priority_bias,
                           int mute);
void gv89_set_master(gv89_context *ctx,
                      gv89_s16 master_gain_q15,
                      gv89_s16 limiter_threshold,
                      gv89_u8 limiter_release_shift);

gv89_result gv89_reserve(gv89_context *ctx,
                           const gv89_request *request,
                           gv89_reservation *reservation);
gv89_result gv89_commit(gv89_context *ctx,
                          const gv89_reservation *reservation,
                          const gv89_provider *provider,
                          const gv89_voice_params *params,
                          gv89_handle *handle);
gv89_result gv89_commit_ex(gv89_context *ctx,
                             const gv89_reservation *reservation,
                             const gv89_provider_ex *provider,
                             const gv89_voice_params *params,
                             gv89_handle *handle);
void gv89_cancel_reservation(gv89_context *ctx,
                              const gv89_reservation *reservation);
gv89_result gv89_start(gv89_context *ctx,
                         const gv89_request *request,
                         const gv89_provider *provider,
                         const gv89_voice_params *params,
                         gv89_handle *handle);
gv89_result gv89_start_ex(gv89_context *ctx,
                            const gv89_request *request,
                            const gv89_provider_ex *provider,
                            const gv89_voice_params *params,
                            gv89_handle *handle);

gv89_result gv89_stop(gv89_context *ctx,
                        gv89_handle handle,
                        gv89_u16 release_ms);
void gv89_stop_group(gv89_context *ctx,
                      gv89_u16 group_id,
                      gv89_u16 release_ms);
void gv89_stop_all(gv89_context *ctx, gv89_u16 release_ms);
gv89_result gv89_set_voice_mix(gv89_context *ctx,
                                 gv89_handle handle,
                                 gv89_s16 gain_q15,
                                 gv89_s16 pan_q15);
gv89_result gv89_set_voice_audibility(gv89_context *ctx,
                                        gv89_handle handle,
                                        gv89_u16 audibility_q15);
gv89_result gv89_set_voice_priority(gv89_context *ctx,
                                      gv89_handle handle,
                                      gv89_u16 priority);

void gv89_begin_batch(gv89_context *ctx);
gv89_result gv89_end_batch(gv89_context *ctx);
void gv89_force_rebalance(gv89_context *ctx);
void gv89_process_stereo_sample(gv89_context *ctx,
                                 gv89_s16 *left,
                                 gv89_s16 *right);
gv89_u32 gv89_render_stereo(gv89_context *ctx,
                              gv89_s16 *interleaved_stereo,
                              gv89_u32 frames,
                              int accumulate);

int gv89_is_handle_active(const gv89_context *ctx, gv89_handle handle);
gv89_u8 gv89_voice_state(const gv89_context *ctx, gv89_handle handle);
gv89_u16 gv89_active_count(const gv89_context *ctx);
gv89_u16 gv89_physical_count(const gv89_context *ctx);
gv89_u16 gv89_virtual_count(const gv89_context *ctx);
void gv89_get_stats(const gv89_context *ctx, gv89_stats *stats);
gv89_u32 gv89_context_bytes(void);
gv89_u32 gv89_voice_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
