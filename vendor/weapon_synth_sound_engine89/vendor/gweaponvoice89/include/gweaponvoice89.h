#ifndef GWEAPONVOICE89_H
#define GWEAPONVOICE89_H

/*
 * gweaponvoice89 v2.0
 * Weapon-aware policy layer for gvoice89 v2.0.
 * Defaults target 256 logical / 64 physical voices.
 */

#include "gvoice89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GWV89_VERSION_MAJOR 2
#define GWV89_VERSION_MINOR 1
#define GWV89_VERSION_PATCH 0

#define GWV89_GROUP_REPORT 1U
#define GWV89_GROUP_MECHANISM 2U
#define GWV89_GROUP_FOLEY 3U
#define GWV89_GROUP_CASING 4U
#define GWV89_GROUP_RICOCHET 5U
#define GWV89_GROUP_IMPACT 6U
#define GWV89_GROUP_EXPLOSION 7U
#define GWV89_GROUP_AMBIENCE 8U

typedef enum gwv89_event_class_e {
    GWV89_EVENT_REPORT = 0,
    GWV89_EVENT_MECHANISM = 1,
    GWV89_EVENT_FOLEY = 2,
    GWV89_EVENT_CASING = 3,
    GWV89_EVENT_RICOCHET = 4,
    GWV89_EVENT_IMPACT = 5,
    GWV89_EVENT_EXPLOSION = 6,
    GWV89_EVENT_AMBIENCE = 7,
    GWV89_EVENT_CLASS_COUNT = 8
} gwv89_event_class;


typedef enum gwv89_mix_profile_e {
    GWV89_MIX_REALISTIC = 0,
    GWV89_MIX_HYBRID = 1,
    GWV89_MIX_CINEMATIC = 2,
    GWV89_MIX_PROFILE_COUNT = 3
} gwv89_mix_profile;

typedef struct gwv89_class_config_s {
    gv89_u16 group_id;
    gv89_u16 priority;
    gv89_u16 max_logical;
    gv89_u16 physical_reserve;
    gv89_s16 gain_q15;
    gv89_u16 distance_priority_penalty;
    gv89_u16 default_instance_limit;
    gv89_u16 steal_protect_ms;
    gv89_u16 minimum_physical_ms;
    gv89_u32 virtual_timeout_ms;
    gv89_steal_policy steal_policy;
    gv89_virtual_behavior virtual_behavior;
    gv89_u32 flags;
    gv89_u16 bus_id;
} gwv89_class_config;

typedef struct gwv89_event_desc_s {
    gwv89_event_class event_class;
    gv89_u32 required_capabilities;
    gv89_s16 gain_q15;
    gv89_s16 screen_x_q15;
    gv89_u16 distance_q15;
    gv89_u16 occlusion_q15;
    gv89_u16 focus_q15;
    gv89_s16 priority_bias;
    gv89_u32 instance_key;
    gv89_u16 instance_limit;
    gv89_u16 attack_ms;
    gv89_u16 release_ms;
    gv89_u32 add_flags;
    gv89_u8 override_virtual_behavior;
    gv89_virtual_behavior virtual_behavior;
    gv89_u8 allow_higher_priority_steal;
    gv89_u8 allow_protected_steal;
} gwv89_event_desc;

typedef struct gwv89_context_s {
    gv89_context voices;
    gwv89_class_config classes[GWV89_EVENT_CLASS_COUNT];
} gwv89_context;

void gwv89_event_default(gwv89_event_desc *desc,
                          gwv89_event_class event_class);

gv89_result gwv89_init(gwv89_context *ctx,
                         gv89_voice *voice_storage,
                         gv89_u16 logical_capacity,
                         gv89_u32 sample_rate);

gv89_result gwv89_init_ex(gwv89_context *ctx,
                            gv89_voice *voice_storage,
                            gv89_u16 logical_capacity,
                            gv89_u16 physical_limit,
                            gv89_u32 sample_rate);

gv89_result gwv89_set_class(gwv89_context *ctx,
                              gwv89_event_class event_class,
                              const gwv89_class_config *config);

gv89_result gwv89_set_physical_limit(gwv89_context *ctx,
                                       gv89_u16 physical_limit);

/* Apply a physically ordered weapon mix: reports/explosions dominate,
 * mechanisms remain close-detail, and ambience stays behind the event. */
gv89_result gwv89_apply_mix_profile(gwv89_context *ctx,
                                      gwv89_mix_profile profile);
void gwv89_set_slot_capabilities(gwv89_context *ctx,
                                  gv89_u16 index,
                                  gv89_u32 capabilities);
void gwv89_begin_batch(gwv89_context *ctx);
gv89_result gwv89_end_batch(gwv89_context *ctx);

gv89_result gwv89_reserve_event(gwv89_context *ctx,
                                  const gwv89_event_desc *desc,
                                  gv89_reservation *reservation,
                                  gv89_voice_params *resolved_params);

gv89_result gwv89_commit_event(gwv89_context *ctx,
                                 const gv89_reservation *reservation,
                                 const gv89_provider *provider,
                                 const gv89_voice_params *resolved_params,
                                 gv89_handle *handle);

gv89_result gwv89_commit_event_ex(gwv89_context *ctx,
                                    const gv89_reservation *reservation,
                                    const gv89_provider_ex *provider,
                                    const gv89_voice_params *resolved_params,
                                    gv89_handle *handle);

gv89_result gwv89_play(gwv89_context *ctx,
                         const gwv89_event_desc *desc,
                         const gv89_provider *provider,
                         gv89_handle *handle);

gv89_result gwv89_play_ex(gwv89_context *ctx,
                            const gwv89_event_desc *desc,
                            const gv89_provider_ex *provider,
                            gv89_handle *handle);

void gwv89_cancel_event(gwv89_context *ctx,
                         const gv89_reservation *reservation);

gv89_result gwv89_set_event_spatial(gwv89_context *ctx,
                                      gv89_handle handle,
                                      gv89_s16 screen_x_q15,
                                      gv89_u16 distance_q15,
                                      gv89_u16 occlusion_q15,
                                      gv89_u16 focus_q15);

void gwv89_process_stereo_sample(gwv89_context *ctx,
                                  gv89_s16 *left,
                                  gv89_s16 *right);
gv89_u32 gwv89_render_stereo(gwv89_context *ctx,
                               gv89_s16 *interleaved_stereo,
                               gv89_u32 frames,
                               int accumulate);

void gwv89_get_stats(const gwv89_context *ctx, gv89_stats *stats);
gv89_u16 gwv89_active_count(const gwv89_context *ctx);
gv89_u16 gwv89_physical_count(const gwv89_context *ctx);
gv89_u16 gwv89_virtual_count(const gwv89_context *ctx);
gv89_u32 gwv89_context_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
