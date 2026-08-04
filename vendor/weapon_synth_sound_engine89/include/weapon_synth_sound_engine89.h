#ifndef WEAPON_SYNTH_SOUND_ENGINE89_H
#define WEAPON_SYNTH_SOUND_ENGINE89_H

/*
 * weapon_synth_sound_engine89 v1.9.0
 * Unified C89/fixed-point event facade over all vendored synthesis modules.
 * Caller-owned memory only. No malloc/realloc/free, no float/double.
 */

#include "gsynthreport89.h"
#include "gsynthsoundengine89.h"
#include "gsynthordnance89.h"
#include "gsynthsoundexpansion89.h"
#include "gsynthworld89.h"
#include "wsoundmetrics89.h"

/* Direct low-level access remains public through these umbrella headers. */
#include "gpaah89.h"
#include "chuecka89.h"
#include "gpump89.h"
#include "shotpumpkin89.h"
#include "gweaponfoley89.h"
#include "gklek89.h"
#include "wmagazine89.h"
#include "grocketwhistle89.h"
#include "grocketspin89.h"
#include "ggunmach89.h"
#include "gguntuberotator89.h"
#include "ggatlingwhistle89.h"
#include "gshotgunsequence89.h"
#include "gshotguneq89.h"
#include "gtinkle89.h"
#include "gfire89.h"
#include "gfire89_fx.h"
#include "wsound_world89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WSSE89_VERSION_MAJOR 1
#define WSSE89_VERSION_MINOR 9
#define WSSE89_VERSION_PATCH 0

#define WSSE89_MAGAZINE_VOICE_CAPACITY 4U
#define WSSE89_ROCKET_WHISTLE_VOICE_CAPACITY 4U
#define WSSE89_ROCKET_SPIN_VOICE_CAPACITY 4U
#define WSSE89_GATLING_VOICE_CAPACITY 2U

typedef enum wsse89_result_e {
    WSSE89_OK = 0,
    WSSE89_EINVAL = -1,
    WSSE89_EVOICE = -2,
    WSSE89_ESYNTH = -3,
    WSSE89_EUNSUPPORTED = -4
} wsse89_result;

typedef struct wsse89_config_s {
    gv89_u32 sample_rate;
    gv89_u16 logical_voice_capacity;
    gv89_u16 physical_voice_limit;
    gv89_u32 seed;
    gv89_u16 expansion_mask;
} wsse89_config;

typedef struct wsse89_storage_s {
    gv89_voice *logical_voices;
    gssr89_voice *report_voices;
    gv89_u16 report_capacity;
    gsse89_casing_voice *casing_voices;
    gv89_u16 casing_capacity;
    gsse89_fire_voice *fire_voices;
    gv89_u16 fire_capacity;
    gsso89_bullet_voice *bullet_voices;
    gv89_u16 bullet_capacity;
    gsso89_grenade_voice *grenade_voices;
    gv89_u16 grenade_capacity;
    gsso89_rocket_voice *rocket_voices;
    gv89_u16 rocket_capacity;
    gssw89_projectile_voice *projectile_voices;
    gv89_u16 projectile_capacity;
    gssw89_impact_voice *impact_voices;
    gv89_u16 impact_capacity;
    gssw89_ricochet_voice *ricochet_voices;
    gv89_u16 ricochet_capacity;
    gssexp89_memory expansion_memory;
    gssw89_memory world_memory;
} wsse89_storage;

typedef struct wsse89_magazine_voice_s {
    WMag89State synth;
    gv89_u32 instance_key;
    gv89_u32 age_stamp;
    gv89_s16 pan_q15;
    gv89_s16 gain_q15;
    gv89_u8 active;
} wsse89_magazine_voice;

typedef struct wsse89_rocket_whistle_voice_s {
    gwh89_state synth;
    gv89_u32 instance_key;
    gv89_u32 age_stamp;
    gv89_s16 pan_q15;
    gv89_s16 gain_q15;
    gv89_u8 active;
} wsse89_rocket_whistle_voice;

typedef struct wsse89_rocket_spin_voice_s {
    grs89_state synth;
    gv89_u32 instance_key;
    gv89_u32 age_stamp;
    gv89_s16 pan_q15;
    gv89_s16 gain_q15;
    gv89_u8 active;
} wsse89_rocket_spin_voice;

typedef struct wsse89_gatling_voice_s {
    ggm89_state motor;
    ggtr89_context rotator;
    GGW89_State whistle;
    gv89_u32 instance_key;
    gv89_u32 age_stamp;
    gv89_s16 pan_q15;
    gv89_s16 gain_q15;
    gv89_s16 motor_gain_q15;
    gv89_s16 rotator_gain_q15;
    gv89_s16 whistle_gain_q15;
    gv89_u8 active;
    gv89_u8 firing;
} wsse89_gatling_voice;

typedef struct wsse89_context_s {
    gwv89_context handler;
    gssr89_context reports;
    gsse89_context base;
    gsso89_context ordnance;
    gssexp89_context expansion;
    gssw89_context world;
    gv89_u32 sample_rate;
    gv89_u32 seed_counter;
    wsoundmetrics89_context metrics;
    wsounddna89_shot last_dna_shot;
    wsse89_magazine_voice magazine_voices[WSSE89_MAGAZINE_VOICE_CAPACITY];
    wsse89_rocket_whistle_voice rocket_whistle_voices[WSSE89_ROCKET_WHISTLE_VOICE_CAPACITY];
    wsse89_rocket_spin_voice rocket_spin_voices[WSSE89_ROCKET_SPIN_VOICE_CAPACITY];
    wsse89_gatling_voice gatling_voices[WSSE89_GATLING_VOICE_CAPACITY];
    gv89_u32 auxiliary_age_counter;
    gv89_u8 has_last_dna;
    gv89_u8 initialized;
} wsse89_context;

typedef enum wsse89_event_type_e {
    WSSE89_EVENT_NONE = 0,
    WSSE89_EVENT_REPORT,
    WSSE89_EVENT_CASING,
    WSSE89_EVENT_FIRE_START,
    WSSE89_EVENT_BULLET_PASS,
    WSSE89_EVENT_GRENADE_BLAST,
    WSSE89_EVENT_ROCKET_BLAST,
    WSSE89_EVENT_PROJECTILE,
    WSSE89_EVENT_IMPACT,
    WSSE89_EVENT_RICOCHET,
    WSSE89_EVENT_STOP_HANDLE,
    WSSE89_EVENT_SET_HANDLE_SPATIAL,
    WSSE89_EVENT_EXPANSION_SHOT,
    WSSE89_EVENT_BELT_START,
    WSSE89_EVENT_BELT_STOP,
    WSSE89_EVENT_FRICTION_START,
    WSSE89_EVENT_FRICTION_STOP,
    WSSE89_EVENT_AERO_START,
    WSSE89_EVENT_AERO_STOP,
    WSSE89_EVENT_PARTICLES,
    WSSE89_EVENT_AMMO,
    WSSE89_EVENT_THERMAL_HEAT,
    WSSE89_EVENT_LISTENER_EXPOSE,
    WSSE89_EVENT_MASK_TRIGGER,
    WSSE89_EVENT_SPATIAL_AZIMUTH,
    WSSE89_EVENT_PORTAL_PATH,
    WSSE89_EVENT_OUTDOOR_MIX,
    WSSE89_EVENT_ACTION_START,
    WSSE89_EVENT_RECEIVER_EXCITE,
    WSSE89_EVENT_ACOUSTIC_ENABLE,
    WSSE89_EVENT_ACOUSTIC_PROFILE,
    WSSE89_EVENT_ACOUSTIC_PATH,
    WSSE89_EVENT_ACOUSTIC_MATERIAL,
    WSSE89_EVENT_ACOUSTIC_SPACE,
    WSSE89_EVENT_ACOUSTIC_PORTAL,
    WSSE89_EVENT_PRESSURE_TRIGGER,
    WSSE89_EVENT_WEAPON_PROFILE,
    WSSE89_EVENT_WEAPON_MODE,
    WSSE89_EVENT_WEAPON_FIRE,
    WSSE89_EVENT_METRICS_ENABLE,
    WSSE89_EVENT_METRICS_RESET,
    WSSE89_EVENT_MAGAZINE_ACTION,
    WSSE89_EVENT_ROCKET_WHISTLE_START,
    WSSE89_EVENT_ROCKET_WHISTLE_MOTION,
    WSSE89_EVENT_ROCKET_WHISTLE_FX,
    WSSE89_EVENT_ROCKET_WHISTLE_RELEASE,
    WSSE89_EVENT_ROCKET_WHISTLE_STOP,
    WSSE89_EVENT_ROCKET_SPIN_START,
    WSSE89_EVENT_ROCKET_SPIN_MOTION,
    WSSE89_EVENT_ROCKET_SPIN_STOP,
    WSSE89_EVENT_GATLING_START,
    WSSE89_EVENT_GATLING_FIRE_START,
    WSSE89_EVENT_GATLING_FIRE_STOP,
    WSSE89_EVENT_GATLING_STOP,
    WSSE89_EVENT_COUNT
} wsse89_event_type;

typedef struct wsse89_stop_event_s {
    gv89_handle handle;
    gv89_u16 release_ms;
} wsse89_stop_event;

typedef struct wsse89_spatial_event_s {
    gv89_handle handle;
    gv89_s16 pan_q15;
    gv89_u16 distance_q15;
    gv89_u16 occlusion_q15;
    gv89_u16 focus_q15;
} wsse89_spatial_event;

typedef struct wsse89_expansion_shot_event_s {
    wsoundmuzzledevice89_type device;
    wsound89_u16 energy_q15;
    wsound89_u16 distance_q15;
} wsse89_expansion_shot_event;

typedef struct wsse89_belt_event_s {
    wsound89_u16 rpm;
    wsound89_u16 tension_q15;
} wsse89_belt_event;

typedef struct wsse89_friction_event_s {
    wsoundfriction89_material material;
    wsound89_u16 speed_q15;
    wsound89_u16 pressure_q15;
    wsound89_u16 roughness_q15;
} wsse89_friction_event;

typedef struct wsse89_aero_event_s {
    wsoundaero89_mode mode;
    wsound89_u16 speed_q15;
    wsound89_u16 size_q15;
    wsound89_u32 duration_frames;
} wsse89_aero_event;

typedef struct wsse89_particles_event_s {
    wsoundparticles89_material material;
    wsound89_u16 density_q15;
    wsound89_u16 energy_q15;
} wsse89_particles_event;

typedef struct wsse89_ammo_event_s {
    wsoundammo89_type type;
    wsound89_u16 fill_q15;
    wsound89_u16 motion_q15;
} wsse89_ammo_event;

typedef struct wsse89_thermal_event_s {
    wsoundthermal89_material material;
    wsound89_u16 heat_q15;
} wsse89_thermal_event;

typedef struct wsse89_listener_event_s {
    wsound89_u16 energy_q15;
    wsound89_u16 distance_q15;
    wsoundlistener89_protection protection;
} wsse89_listener_event;

typedef struct wsse89_spatial_azimuth_event_s {
    wsound89_i16 pan_q15;
    wsound89_u16 itd_samples;
    wsound89_u16 shadow_q15;
} wsse89_spatial_azimuth_event;

typedef struct wsse89_portal_event_s {
    wsound89_u32 delay_samples;
    wsound89_u16 low_q15;
    wsound89_u16 mid_q15;
    wsound89_u16 high_q15;
    wsound89_u16 opening_q15;
} wsse89_portal_event;

typedef struct wsse89_outdoor_mix_event_s {
    wsound89_u16 send_q15;
    wsound89_u16 wet_q15;
} wsse89_outdoor_mix_event;

typedef struct wsse89_action_event_s {
    wsoundaction89_type action;
    wsound89_u32 speed_q16;
} wsse89_action_event;


typedef struct wsse89_acoustic_material_event_s {
    wsounda89_material material;
    wsound89_u16 thickness_q15;
} wsse89_acoustic_material_event;

typedef struct wsse89_pressure_event_s {
    wsounda89_pressure_kind kind;
    wsound89_u16 energy_q15;
    wsound89_i16 pan_q15;
} wsse89_pressure_event;


typedef struct wsse89_weapon_fire_event_s {
    gv89_s16 pan_q15;
    gv89_u16 distance_q15;
    gv89_u16 occlusion_q15;
    gv89_u16 focus_q15;
    gv89_s16 gain_q15;
    gv89_s16 priority_bias;
    gv89_u32 instance_key;
    gv89_u16 instance_limit;
    gv89_u16 pressure_energy_q15;
} wsse89_weapon_fire_event;

typedef struct wsse89_magazine_event_s {
    gv89_s32 preset;
    gv89_s32 action;
    gv89_u16 velocity_q15;
    gv89_s16 gain_q15;
    gv89_s16 pan_q15;
    gv89_u32 instance_key;
} wsse89_magazine_event;

typedef struct wsse89_rocket_whistle_start_event_s {
    gv89_s32 preset;
    gv89_s32 radial_velocity_mps;
    gv89_u16 distance_gain_q15;
    gv89_u16 pitch_scale_q15;
    gv89_u16 auto_hold_ms;
    gv89_s16 gain_q15;
    gv89_s16 pan_q15;
    gv89_u32 instance_key;
} wsse89_rocket_whistle_start_event;

typedef struct wsse89_rocket_whistle_motion_event_s {
    gv89_u32 instance_key;
    gv89_s32 radial_velocity_mps;
    gv89_u16 distance_gain_q15;
    gv89_s16 gain_q15;
    gv89_s16 pan_q15;
} wsse89_rocket_whistle_motion_event;

/* mode: 0 = keep preset, 1 = disable, 2 = apply custom values. */
typedef struct wsse89_rocket_whistle_fx_event_s {
    gv89_u32 instance_key;
    gv89_u8 primary_eq_mode;
    gv89_u8 output_eq_mode;
    gv89_u8 reverb_mode;
    gv89_u8 reserved;
    gv89_u16 primary_eq_gain_q15[GWH89_EQ_BANDS];
    gv89_u16 output_eq_gain_q15[GWH89_OUTPUT_EQ_BANDS];
    gv89_u16 reverb_wet_q15;
    gv89_u16 reverb_feedback_q15;
    gv89_u16 reverb_damping_q15;
    gv89_u16 reverb_tail_ms;
} wsse89_rocket_whistle_fx_event;

typedef struct wsse89_rocket_whistle_control_event_s {
    gv89_u32 instance_key;
} wsse89_rocket_whistle_control_event;

typedef struct wsse89_rocket_spin_start_event_s {
    gv89_s32 preset;
    gv89_u16 sustain_ms;
    gv89_u16 release_ms;
    gv89_s16 gain_q15;
    gv89_s16 pan_q15;
    gv89_u32 instance_key;
} wsse89_rocket_spin_start_event;

typedef struct wsse89_rocket_spin_motion_event_s {
    gv89_u32 instance_key;
    gv89_s16 gain_q15;
    gv89_s16 pan_q15;
} wsse89_rocket_spin_motion_event;

typedef struct wsse89_rocket_spin_control_event_s {
    gv89_u32 instance_key;
} wsse89_rocket_spin_control_event;

typedef struct wsse89_gatling_start_event_s {
    gv89_s32 motor_preset;
    gv89_s32 rotator_preset;
    gv89_s16 gain_q15;
    gv89_s16 motor_gain_q15;
    gv89_s16 rotator_gain_q15;
    gv89_s16 whistle_gain_q15;
    gv89_s16 pan_q15;
    gv89_u32 instance_key;
} wsse89_gatling_start_event;

typedef struct wsse89_gatling_fire_event_s {
    gv89_u32 instance_key;
    gv89_s16 firing_load_q15;
} wsse89_gatling_fire_event;

typedef struct wsse89_gatling_control_event_s {
    gv89_u32 instance_key;
} wsse89_gatling_control_event;

typedef struct wsse89_event_s {
    wsse89_event_type type;
    gv89_u32 seed; /* 0 = engine-generated deterministic seed */
    union {
        gssr89_params report;
        gsse89_casing_params casing;
        gsse89_fire_params fire;
        gsso89_bullet_params bullet;
        gsso89_grenade_params grenade;
        gsso89_rocket_params rocket;
        gssw89_projectile_params projectile;
        gssw89_impact_params impact;
        gssw89_ricochet_params ricochet;
        wsse89_stop_event stop;
        wsse89_spatial_event spatial;
        wsse89_expansion_shot_event expansion_shot;
        wsse89_belt_event belt;
        wsse89_friction_event friction;
        wsse89_aero_event aero;
        wsse89_particles_event particles;
        wsse89_ammo_event ammo;
        wsse89_thermal_event thermal;
        wsse89_listener_event listener;
        wsound89_u16 mask_energy_q15;
        wsse89_spatial_azimuth_event azimuth;
        wsse89_portal_event portal;
        wsse89_outdoor_mix_event outdoor_mix;
        wsse89_action_event action;
        wsound89_i16 receiver_impulse;
        wsound89_u16 acoustic_enabled;
        wsounda89_profile acoustic_profile;
        wsounda89_path_params acoustic_path;
        wsse89_acoustic_material_event acoustic_material;
        wsounda89_space acoustic_space;
        wsounda89_portal_params acoustic_portal;
        wsse89_pressure_event pressure;
        wsounddna89_profile weapon_profile;
        wsounddna89_mode weapon_mode;
        wsse89_weapon_fire_event weapon_fire;
        wsound89_u16 metrics_enabled;
        wsse89_magazine_event magazine;
        wsse89_rocket_whistle_start_event rocket_whistle_start;
        wsse89_rocket_whistle_motion_event rocket_whistle_motion;
        wsse89_rocket_whistle_fx_event rocket_whistle_fx;
        wsse89_rocket_whistle_control_event rocket_whistle_control;
        wsse89_rocket_spin_start_event rocket_spin_start;
        wsse89_rocket_spin_motion_event rocket_spin_motion;
        wsse89_rocket_spin_control_event rocket_spin_control;
        wsse89_gatling_start_event gatling_start;
        wsse89_gatling_fire_event gatling_fire;
        wsse89_gatling_control_event gatling_control;
    } data;
} wsse89_event;

typedef int (*wsse89_event_sink_fn)(void *user,
                                     const wsse89_event *event,
                                     gv89_handle *out_handle);

void wsse89_config_defaults(wsse89_config *config);
void wsse89_weapon_fire_defaults(wsse89_weapon_fire_event *event);
void wsse89_magazine_defaults(wsse89_magazine_event *event);
void wsse89_rocket_whistle_defaults(wsse89_rocket_whistle_start_event *event);
void wsse89_rocket_whistle_fx_defaults(wsse89_rocket_whistle_fx_event *event);
void wsse89_rocket_spin_defaults(wsse89_rocket_spin_start_event *event);
void wsse89_gatling_defaults(wsse89_gatling_start_event *event);
int wsse89_init(wsse89_context *ctx,
                const wsse89_config *config,
                const wsse89_storage *storage);
void wsse89_reset(wsse89_context *ctx);
int wsse89_dispatch(wsse89_context *ctx,
                    const wsse89_event *event,
                    gv89_handle *out_handle);
int wsse89_event_sink(void *user,
                      const wsse89_event *event,
                      gv89_handle *out_handle);
const char *wsse89_event_name(wsse89_event_type type);

gv89_u32 wsse89_render_stereo(wsse89_context *ctx,
                               gv89_s16 *interleaved_stereo,
                               gv89_u32 frames,
                               int accumulate);
void wsse89_process_stereo_sample(wsse89_context *ctx,
                                  gv89_s16 *left,
                                  gv89_s16 *right);
wsound89_result wsse89_advance_action(wsse89_context *ctx,
                                      wsound89_u32 frames,
                                      wsoundaction89_event *events,
                                      wsound89_u16 capacity,
                                      wsound89_u16 *written);
void wsse89_get_stats(const wsse89_context *ctx, gv89_stats *stats);
int wsse89_get_last_dna(const wsse89_context *ctx, wsounddna89_shot *shot);
wsound89_result wsse89_get_metrics(const wsse89_context *ctx,
                                    wsoundmetrics89_result *result);
wsound89_u16 wsse89_validate_metrics(const wsse89_context *ctx,
                                      wsoundmetrics89_target *target,
                                      wsoundmetrics89_result *result);
gv89_u32 wsse89_context_bytes(void);

#ifdef __cplusplus
}
#endif
#endif
