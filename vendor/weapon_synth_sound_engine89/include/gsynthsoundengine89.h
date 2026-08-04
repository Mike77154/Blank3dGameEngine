#ifndef GSYNTHSOUNDENGINE89_H
#define GSYNTHSOUNDENGINE89_H

/*
 * gsynthsoundengine89 v1.3
 * Heapless bridge that registers gtinkle89 and gfire89 as providers for
 * gweaponvoice89/gvoice89.
 *
 * C89, caller-owned storage, no malloc/realloc/free, no float/double.
 */

#include "gweaponvoice89.h"
#include "gtinkle89.h"
#include "gfire89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GSSE89_VERSION_MAJOR 1
#define GSSE89_VERSION_MINOR 3
#define GSSE89_VERSION_PATCH 0

#define GSSE89_INVALID_HANDLE_INDEX GV89_INVALID_INDEX

typedef enum gsse89_fire_role_e {
    GSSE89_FIRE_ROLE_AMBIENCE = 0,
    GSSE89_FIRE_ROLE_FLAMETHROWER = 1,
    GSSE89_FIRE_ROLE_EXPLOSIVE_DEBRIS = 2
} gsse89_fire_role;

typedef struct gsse89_casing_params_s {
    gt89_shell_type shell;
    gt89_surface_type surface;
    gt89_u8 velocity;
    gt89_u8 angular_velocity;
    gt89_u8 variation;
    gv89_s16 pan_q15;
    gv89_u16 distance_q15;
    gv89_u16 occlusion_q15;
    gv89_u16 focus_q15;
    gv89_s16 gain_q15;
    gv89_s16 priority_bias;
    gv89_u32 instance_key;
    gv89_u16 instance_limit;
} gsse89_casing_params;

typedef struct gsse89_fire_params_s {
    gsse89_fire_role role;
    gv89_s32 preset_id;
    gv89_s32 intensity_q15;
    gv89_s32 airflow_q15;
    gv89_s32 crackle_q15;
    gv89_s32 size_q15;
    gv89_s32 pressure_q15;
    gv89_s32 drive_q15;
    gv89_s32 brightness_q15;
    gv89_s32 output_gain_q15;
    gv89_s16 pan_q15;
    gv89_u16 distance_q15;
    gv89_u16 occlusion_q15;
    gv89_u16 focus_q15;
    gv89_s16 gain_q15;
    gv89_s16 priority_bias;
    gv89_u32 duration_ms; /* 0 = loop until explicitly stopped */
    gv89_u16 release_ms;
    gv89_u32 instance_key;
    gv89_u16 instance_limit;
} gsse89_fire_params;

typedef struct gsse89_casing_voice_s {
    gt89_context synth;
    gv89_handle handle;
    gv89_u32 serial;
    gv89_u8 in_use;
} gsse89_casing_voice;

typedef struct gsse89_fire_voice_s {
    gfire89 synth;
    gsse89_fire_params params;
    gv89_handle handle;
    gv89_u32 age_frames;
    gv89_u32 gate_off_frame;
    gv89_u32 total_frames;
    gv89_u32 seed;
    gv89_u32 serial;
    gv89_u8 in_use;
    gv89_u8 gate_closed;
} gsse89_fire_voice;

typedef struct gsse89_context_s {
    gsse89_casing_voice *casing_voices;
    gv89_u16 casing_capacity;
    gsse89_fire_voice *fire_voices;
    gv89_u16 fire_capacity;
    gv89_u32 sample_rate;
    gv89_u32 serial_counter;
} gsse89_context;

void gsse89_casing_defaults(gsse89_casing_params *params);
void gsse89_fire_defaults(gsse89_fire_params *params,
                          gsse89_fire_role role);

int gsse89_init(gsse89_context *ctx,
                 gsse89_casing_voice *casing_storage,
                 gv89_u16 casing_capacity,
                 gsse89_fire_voice *fire_storage,
                 gv89_u16 fire_capacity,
                 gv89_u32 sample_rate);

void gsse89_reset(gsse89_context *ctx, gwv89_context *handler);

gv89_result gsse89_play_casing(gsse89_context *ctx,
                                gwv89_context *handler,
                                const gsse89_casing_params *params,
                                gv89_u32 seed,
                                gv89_handle *out_handle);

gv89_result gsse89_play_fire(gsse89_context *ctx,
                              gwv89_context *handler,
                              const gsse89_fire_params *params,
                              gv89_u32 seed,
                              gv89_handle *out_handle);

void gsse89_stop_fire(gsse89_context *ctx,
                       gwv89_context *handler,
                       gv89_handle handle,
                       gv89_u16 release_ms);

gv89_u16 gsse89_active_casings(const gsse89_context *ctx);
gv89_u16 gsse89_active_fires(const gsse89_context *ctx);

gv89_u32 gsse89_casing_voice_bytes(void);
gv89_u32 gsse89_fire_voice_bytes(void);
gv89_u32 gsse89_context_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
