#ifndef GSYNTHWORLD89_H
#define GSYNTHWORLD89_H

/*
 * gsynthworld89 v1.1
 * Heapless bridge for projectile, impact, ricochet and acoustic-world modules.
 */

#include "gweaponvoice89.h"
#include "wsound_world89.h"
#include "wsoundacoustic89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GSSW89_VERSION_MAJOR 1
#define GSSW89_VERSION_MINOR 2
#define GSSW89_VERSION_PATCH 0

typedef struct gssw89_common_params_s {
    gv89_s16 pan_q15;
    gv89_u16 distance_q15;
    gv89_u16 occlusion_q15;
    gv89_u16 focus_q15;
    gv89_s16 gain_q15;
    gv89_s16 priority_bias;
    gv89_u32 instance_key;
    gv89_u16 instance_limit;
} gssw89_common_params;

typedef struct gssw89_projectile_params_s {
    gssw89_common_params common;
    wsoundprojectile89_mode mode;
    wsound89_u16 amplitude_q15;
    wsound89_u16 proximity_q15;
} gssw89_projectile_params;

typedef struct gssw89_impact_params_s {
    gssw89_common_params common;
    wsoundimpact89_material material;
    wsound89_u16 energy_q15;
    wsound89_u16 size_q15;
} gssw89_impact_params;

typedef struct gssw89_ricochet_params_s {
    gssw89_common_params common;
    wsound89_u16 energy_q15;
    wsound89_u16 grazing_q15;
    wsound89_u16 roughness_q15;
} gssw89_ricochet_params;

typedef struct gssw89_projectile_voice_s {
    wsoundprojectile89_context synth;
    gssw89_projectile_params params;
    gv89_handle handle;
    gv89_u32 seed;
    gv89_u8 in_use;
} gssw89_projectile_voice;

typedef struct gssw89_impact_voice_s {
    wsoundimpact89_context synth;
    gssw89_impact_params params;
    gv89_handle handle;
    gv89_u32 seed;
    gv89_u8 in_use;
} gssw89_impact_voice;

typedef struct gssw89_ricochet_voice_s {
    wsoundricochet89_context synth;
    gssw89_ricochet_params params;
    gv89_handle handle;
    gv89_u32 seed;
    gv89_u32 sample_rate;
    gv89_u8 in_use;
} gssw89_ricochet_voice;

typedef struct gssw89_memory_s {
    wsound89_i16 *room_delay;
    wsound89_u32 room_delay_samples;
    wsound89_i16 *prop_delay;
    wsound89_u32 prop_delay_samples;
} gssw89_memory;

typedef struct gssw89_context_s {
    gssw89_projectile_voice *projectile_voices;
    gv89_u16 projectile_capacity;
    gssw89_impact_voice *impact_voices;
    gv89_u16 impact_capacity;
    gssw89_ricochet_voice *ricochet_voices;
    gv89_u16 ricochet_capacity;
    wsounddna89_context dna;
    wsoundreceiver89_context receiver;
    wsoundprop89_context propagation;
    wsoundroom89_context room;
    wsoundaction89_context action;
    wsoundcombatbus89_context combat_bus;
    wsounda89_context acoustic;
    gv89_u32 sample_rate;
    gv89_s16 world_wet_q15;
    gv89_u8 propagation_enabled;
    gv89_u8 room_enabled;
    gv89_u8 receiver_enabled;
    gv89_u8 acoustic_enabled;
} gssw89_context;

void gssw89_common_defaults(gssw89_common_params *params);
void gssw89_projectile_defaults(gssw89_projectile_params *params);
void gssw89_impact_defaults(gssw89_impact_params *params);
void gssw89_ricochet_defaults(gssw89_ricochet_params *params);

int gssw89_init(gssw89_context *ctx,
                 gssw89_projectile_voice *projectile_storage,
                 gv89_u16 projectile_capacity,
                 gssw89_impact_voice *impact_storage,
                 gv89_u16 impact_capacity,
                 gssw89_ricochet_voice *ricochet_storage,
                 gv89_u16 ricochet_capacity,
                 gv89_u32 sample_rate,
                 gv89_u32 seed,
                 const gssw89_memory *memory);
void gssw89_reset(gssw89_context *ctx, gwv89_context *handler);

gv89_result gssw89_play_projectile(gssw89_context *ctx,
                                    gwv89_context *handler,
                                    const gssw89_projectile_params *params,
                                    gv89_u32 seed,
                                    gv89_handle *out_handle);
gv89_result gssw89_play_impact(gssw89_context *ctx,
                                gwv89_context *handler,
                                const gssw89_impact_params *params,
                                gv89_u32 seed,
                                gv89_handle *out_handle);
gv89_result gssw89_play_ricochet(gssw89_context *ctx,
                                  gwv89_context *handler,
                                  const gssw89_ricochet_params *params,
                                  gv89_u32 seed,
                                  gv89_handle *out_handle);

wsound89_result gssw89_set_room(gssw89_context *ctx,
                                wsoundroom89_preset preset,
                                wsound89_i16 *memory,
                                wsound89_u32 memory_samples);
wsound89_result gssw89_set_propagation(gssw89_context *ctx,
                                       wsound89_u32 distance_cm,
                                       wsound89_u32 speed_cm_s,
                                       wsound89_i16 muzzle_dot_q15,
                                       wsound89_u16 occlusion_q15);
void gssw89_enable_post(gssw89_context *ctx, int propagation,
                        int room, int receiver);
void gssw89_set_world_wet(gssw89_context *ctx, gv89_s16 wet_q15);
void gssw89_enable_acoustic(gssw89_context *ctx, int enabled);
void gssw89_set_acoustic_profile(gssw89_context *ctx, wsounda89_profile profile);
wsound89_result gssw89_set_acoustic_path(gssw89_context *ctx,
                                         const wsounda89_path_params *params);
wsound89_result gssw89_set_acoustic_material(gssw89_context *ctx,
                                             wsounda89_material material,
                                             wsound89_u16 thickness_q15);
wsound89_result gssw89_set_acoustic_space(gssw89_context *ctx,
                                          wsounda89_space space);
wsound89_result gssw89_set_acoustic_portal(gssw89_context *ctx,
                                           const wsounda89_portal_params *params);
void gssw89_trigger_pressure(gssw89_context *ctx,
                             wsounda89_pressure_kind kind,
                             wsound89_u16 energy_q15,
                             wsound89_i16 pan_q15,
                             wsound89_u32 seed);

void gssw89_process_post_stereo(gssw89_context *ctx,
                                gv89_s16 in_left, gv89_s16 in_right,
                                gv89_s16 *out_left, gv89_s16 *out_right);

wsound89_result gssw89_action_trigger(gssw89_context *ctx,
                                      wsoundaction89_type type,
                                      wsound89_u32 speed_q16);
wsound89_result gssw89_action_advance(gssw89_context *ctx,
                                      wsound89_u32 frames,
                                      wsoundaction89_event *events,
                                      wsound89_u16 capacity,
                                      wsound89_u16 *written);
wsound89_result gssw89_dna_set_profile(gssw89_context *ctx,
                                       const wsounddna89_profile *profile);
wsound89_result gssw89_dna_set_mode(gssw89_context *ctx,
                                    wsounddna89_mode mode);
wsound89_result gssw89_dna_next_profiled(gssw89_context *ctx,
                                         wsounddna89_shot *out_shot);
wsound89_result gssw89_dna_next(gssw89_context *ctx,
                                wsounddna89_class weapon_class,
                                wsound89_u16 base_energy_q15,
                                wsounddna89_shot *out_shot);
void gssw89_set_receiver(gssw89_context *ctx,
                         wsoundreceiver89_preset preset);
void gssw89_excite_receiver(gssw89_context *ctx, wsound89_i16 impulse);

gv89_u16 gssw89_active_projectiles(const gssw89_context *ctx);
gv89_u16 gssw89_active_impacts(const gssw89_context *ctx);
gv89_u16 gssw89_active_ricochets(const gssw89_context *ctx);
gv89_u32 gssw89_context_bytes(void);
gv89_u32 gssw89_projectile_voice_bytes(void);
gv89_u32 gssw89_impact_voice_bytes(void);
gv89_u32 gssw89_ricochet_voice_bytes(void);

#ifdef __cplusplus
}
#endif
#endif
