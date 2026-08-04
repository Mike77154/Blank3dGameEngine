#ifndef WSOUNDACOUSTIC89_H
#define WSOUNDACOUSTIC89_H

#include "wsound89_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WSOUNDA89_VERSION_MAJOR 1
#define WSOUNDA89_VERSION_MINOR 0
#define WSOUNDA89_VERSION_PATCH 0
#define WSOUNDA89_EARLY_TAPS 8U
#define WSOUNDA89_LATE_LINES 4U
#define WSOUNDA89_BANDS 3U

typedef enum wsounda89_profile_e {
    WSOUNDA89_REALISTIC = 0,
    WSOUNDA89_HYBRID = 1,
    WSOUNDA89_CINEMATIC = 2
} wsounda89_profile;

typedef enum wsounda89_source_e {
    WSOUNDA89_SOURCE_REPORT = 0,
    WSOUNDA89_SOURCE_BODY = 1,
    WSOUNDA89_SOURCE_GAS = 2,
    WSOUNDA89_SOURCE_MECHANISM = 3,
    WSOUNDA89_SOURCE_PROJECTILE = 4,
    WSOUNDA89_SOURCE_IMPACT = 5,
    WSOUNDA89_SOURCE_EXPLOSION = 6,
    WSOUNDA89_SOURCE_TAIL = 7
} wsounda89_source;

typedef enum wsounda89_material_e {
    WSOUNDA89_MATERIAL_AIR = 0,
    WSOUNDA89_MATERIAL_CONCRETE = 1,
    WSOUNDA89_MATERIAL_BRICK = 2,
    WSOUNDA89_MATERIAL_WOOD = 3,
    WSOUNDA89_MATERIAL_DRYWALL = 4,
    WSOUNDA89_MATERIAL_GLASS = 5,
    WSOUNDA89_MATERIAL_METAL = 6,
    WSOUNDA89_MATERIAL_SOIL = 7,
    WSOUNDA89_MATERIAL_VEGETATION = 8,
    WSOUNDA89_MATERIAL_WATER = 9,
    WSOUNDA89_MATERIAL_FABRIC = 10,
    WSOUNDA89_MATERIAL_COUNT = 11
} wsounda89_material;

typedef enum wsounda89_space_e {
    WSOUNDA89_SPACE_SMALL = 0,
    WSOUNDA89_SPACE_CORRIDOR = 1,
    WSOUNDA89_SPACE_WAREHOUSE = 2,
    WSOUNDA89_SPACE_TUNNEL = 3,
    WSOUNDA89_SPACE_FIELD = 4,
    WSOUNDA89_SPACE_URBAN = 5,
    WSOUNDA89_SPACE_FOREST = 6,
    WSOUNDA89_SPACE_MOUNTAIN = 7,
    WSOUNDA89_SPACE_COUNT = 8
} wsounda89_space;

typedef enum wsounda89_pressure_kind_e {
    WSOUNDA89_PRESSURE_REPORT = 0,
    WSOUNDA89_PRESSURE_IMPACT = 1,
    WSOUNDA89_PRESSURE_GRENADE = 2,
    WSOUNDA89_PRESSURE_ROCKET = 3
} wsounda89_pressure_kind;

typedef struct wsounda89_material_params_s {
    wsound89_u16 absorb_q15[WSOUNDA89_BANDS];
    wsound89_u16 transmit_q15[WSOUNDA89_BANDS];
    wsound89_u16 scatter_q15;
} wsounda89_material_params;

typedef struct wsounda89_path_params_s {
    wsound89_u32 distance_cm;
    wsound89_u32 speed_cm_s;
    wsound89_i16 source_dot_q15;
    wsound89_u16 occlusion_q15;
    wsounda89_source source;
    wsound89_u16 air_absorb_q15[WSOUNDA89_BANDS];
} wsounda89_path_params;

typedef struct wsounda89_portal_params_s {
    wsound89_u32 delay_samples;
    wsound89_u16 opening_q15;
    wsound89_u16 transmit_q15[WSOUNDA89_BANDS];
    wsound89_i16 pan_q15;
} wsounda89_portal_params;

typedef struct wsounda89_context_s {
    wsound89_i16 *direct_delay;
    wsound89_u32 direct_capacity;
    wsound89_u32 direct_write;
    wsound89_u32 direct_delay_samples;
    wsound89_i16 *world_delay;
    wsound89_u32 world_capacity;
    wsound89_u32 world_write;
    wsound89_u32 sample_rate;

    wsounda89_profile profile;
    wsounda89_space space;
    wsounda89_material material;
    wsounda89_path_params path;
    wsounda89_portal_params portal;
    wsounda89_material_params material_params;

    wsound89_i16 distance_gain_q15;
    wsound89_i16 directivity_q15[WSOUNDA89_BANDS];
    wsound89_i16 path_gain_q15[WSOUNDA89_BANDS];

    wsound89_i32 split_low;
    wsound89_i32 split_high;
    wsound89_i32 direct_low_state;
    wsound89_i32 direct_mid_state;
    wsound89_i32 direct_high_state;
    wsound89_i32 early_low_state[WSOUNDA89_EARLY_TAPS];
    wsound89_i32 early_high_state[WSOUNDA89_EARLY_TAPS];

    wsound89_u32 early_delay[WSOUNDA89_EARLY_TAPS];
    wsound89_i16 early_gain_q15[WSOUNDA89_EARLY_TAPS][WSOUNDA89_BANDS];
    wsound89_i16 early_pan_q15[WSOUNDA89_EARLY_TAPS];

    wsound89_u32 late_offset[WSOUNDA89_LATE_LINES];
    wsound89_u32 late_length[WSOUNDA89_LATE_LINES];
    wsound89_u32 late_pos[WSOUNDA89_LATE_LINES];
    wsound89_i32 late_damp[WSOUNDA89_LATE_LINES];
    wsound89_i16 late_feedback_q15[WSOUNDA89_LATE_LINES];
    wsound89_i16 late_damping_q15[WSOUNDA89_LATE_LINES];
    wsound89_i16 late_pan_q15[WSOUNDA89_LATE_LINES];

    wsound89_u32 ground_delay;
    wsound89_i16 ground_gain_q15;
    wsound89_i16 ground_high_q15;

    wsound89_u32 pressure_frame;
    wsound89_u32 pressure_positive_frames;
    wsound89_u32 pressure_negative_frames;
    wsound89_i16 pressure_positive_q15;
    wsound89_i16 pressure_negative_q15;
    wsound89_i16 pressure_pan_q15;
    wsound89_u16 pressure_energy_q15;
    wsound89_u32 pressure_seed;
    wsound89_u8 pressure_active;

    wsound89_i32 translation_low;
    wsound89_i32 translation_dc;
    wsound89_i16 translation_gain_q15;

    wsound89_i32 duck_gain_q15;
    wsound89_i32 limiter_gain_q15;
    wsound89_i32 transient_env;
    wsound89_i32 master_low_l;
    wsound89_i32 master_high_l;
    wsound89_i32 master_low_r;
    wsound89_i32 master_high_r;
    wsound89_i32 band_dynamics_q15[WSOUNDA89_BANDS];
    wsound89_i16 direct_wet_q15;
    wsound89_i16 early_wet_q15;
    wsound89_i16 late_wet_q15;
    wsound89_i16 master_gain_q15;
    wsound89_i16 limiter_threshold;
    wsound89_u8 enabled;
} wsounda89_context;

wsound89_u32 wsounda89_required_direct_samples(wsound89_u32 sample_rate,
                                                wsound89_u32 max_distance_cm,
                                                wsound89_u32 speed_cm_s);
wsound89_u32 wsounda89_required_world_samples(wsound89_u32 sample_rate);
wsound89_result wsounda89_init(wsounda89_context *ctx,
                               wsound89_u32 sample_rate,
                               wsound89_i16 *direct_memory,
                               wsound89_u32 direct_samples,
                               wsound89_i16 *world_memory,
                               wsound89_u32 world_samples);
void wsounda89_reset(wsounda89_context *ctx);
void wsounda89_set_enabled(wsounda89_context *ctx, int enabled);
void wsounda89_set_profile(wsounda89_context *ctx, wsounda89_profile profile);
void wsounda89_material_defaults(wsounda89_material material,
                                 wsounda89_material_params *params);
wsound89_result wsounda89_set_material(wsounda89_context *ctx,
                                       wsounda89_material material,
                                       wsound89_u16 thickness_q15);
void wsounda89_path_defaults(wsounda89_path_params *params);
wsound89_result wsounda89_set_path(wsounda89_context *ctx,
                                   const wsounda89_path_params *params);
wsound89_result wsounda89_set_space(wsounda89_context *ctx,
                                    wsounda89_space space);
void wsounda89_portal_defaults(wsounda89_portal_params *params);
wsound89_result wsounda89_set_portal(wsounda89_context *ctx,
                                     const wsounda89_portal_params *params);
void wsounda89_trigger_pressure(wsounda89_context *ctx,
                                wsounda89_pressure_kind kind,
                                wsound89_u16 energy_q15,
                                wsound89_i16 pan_q15,
                                wsound89_u32 seed);
void wsounda89_process_stereo(wsounda89_context *ctx,
                              wsound89_i16 in_left,
                              wsound89_i16 in_right,
                              wsound89_i16 *out_left,
                              wsound89_i16 *out_right);

#ifdef __cplusplus
}
#endif
#endif
