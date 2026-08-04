#ifndef GSYNTHORDNANCE89_H
#define GSYNTHORDNANCE89_H

/*
 * gsynthordnance89 v1.0
 * Heapless provider bridge for gbulletair89, wsound_ggrenadeblast89 and
 * wsound_rocketblast89 under gweaponvoice89.
 *
 * C89, caller-owned storage, no malloc/realloc/free, no float/double.
 */

#include "gweaponvoice89.h"
#include "gbulletair89.h"
#include "wsound_ggrenadeblast89.h"
#include "wsound_rocketblast89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GSSO89_VERSION_MAJOR 1
#define GSSO89_VERSION_MINOR 1
#define GSSO89_VERSION_PATCH 0

typedef struct gsso89_common_params_s {
    gv89_s16 pan_q15;
    gv89_u16 distance_q15;
    gv89_u16 occlusion_q15;
    gv89_u16 focus_q15;
    gv89_s16 gain_q15;
    gv89_s16 priority_bias;
    gv89_u32 instance_key;
    gv89_u16 instance_limit;
} gsso89_common_params;

typedef struct gsso89_bullet_params_s {
    gsso89_common_params common;
    gv89_s32 preset_id;
} gsso89_bullet_params;

typedef struct gsso89_grenade_params_s {
    gsso89_common_params common;
    gv89_s32 preset_id;
    gv89_s16 intensity_q15;
} gsso89_grenade_params;

typedef struct gsso89_rocket_params_s {
    gsso89_common_params common;
    gv89_u16 preset_id;
    gv89_u16 velocity_q15;
    gv89_u16 use_custom_params;
    wsrb89_params synth_params;
} gsso89_rocket_params;

typedef struct gsso89_bullet_voice_s {
    gba89_state synth;
    gsso89_bullet_params params;
    gv89_handle handle;
    gv89_u32 seed;
    gv89_u32 serial;
    gv89_u32 phase_q16;
    gv89_u32 step_q16;
    gv89_s16 sample_a;
    gv89_s16 sample_b;
    gv89_u8 primed;
    gv89_u8 in_use;
} gsso89_bullet_voice;

typedef struct gsso89_grenade_voice_s {
    ws_ggb89 synth;
    gsso89_grenade_params params;
    gv89_handle handle;
    gv89_u32 seed;
    gv89_u32 serial;
    gv89_u32 sample_rate;
    gv89_u8 in_use;
} gsso89_grenade_voice;

typedef struct gsso89_rocket_voice_s {
    wsrb89_context synth;
    wsrb89_workspace workspace;
    gsso89_rocket_params params;
    gv89_handle handle;
    gv89_u32 seed;
    gv89_u32 serial;
    gv89_u32 sample_rate;
    gv89_u8 in_use;
} gsso89_rocket_voice;

typedef struct gsso89_context_s {
    gsso89_bullet_voice *bullet_voices;
    gv89_u16 bullet_capacity;
    gsso89_grenade_voice *grenade_voices;
    gv89_u16 grenade_capacity;
    gsso89_rocket_voice *rocket_voices;
    gv89_u16 rocket_capacity;
    gv89_u32 sample_rate;
    gv89_u32 serial_counter;
} gsso89_context;

void gsso89_common_defaults(gsso89_common_params *params);
void gsso89_bullet_defaults(gsso89_bullet_params *params);
void gsso89_grenade_defaults(gsso89_grenade_params *params);
void gsso89_rocket_defaults(gsso89_rocket_params *params);
void gsso89_rocket_load_preset(gsso89_rocket_params *params,
                                gv89_u16 preset_id);
void gsso89_rocket_enable_custom(gsso89_rocket_params *params);

int gsso89_init(gsso89_context *ctx,
                gsso89_bullet_voice *bullet_storage,
                gv89_u16 bullet_capacity,
                gsso89_grenade_voice *grenade_storage,
                gv89_u16 grenade_capacity,
                gsso89_rocket_voice *rocket_storage,
                gv89_u16 rocket_capacity,
                gv89_u32 sample_rate);

void gsso89_reset(gsso89_context *ctx, gwv89_context *handler);

gv89_result gsso89_play_bullet(gsso89_context *ctx,
                                gwv89_context *handler,
                                const gsso89_bullet_params *params,
                                gv89_u32 seed,
                                gv89_handle *out_handle);

gv89_result gsso89_play_grenade(gsso89_context *ctx,
                                 gwv89_context *handler,
                                 const gsso89_grenade_params *params,
                                 gv89_u32 seed,
                                 gv89_handle *out_handle);

gv89_result gsso89_play_rocket(gsso89_context *ctx,
                                gwv89_context *handler,
                                const gsso89_rocket_params *params,
                                gv89_u32 seed,
                                gv89_handle *out_handle);

gv89_u16 gsso89_active_bullets(const gsso89_context *ctx);
gv89_u16 gsso89_active_grenades(const gsso89_context *ctx);
gv89_u16 gsso89_active_rockets(const gsso89_context *ctx);

gv89_u32 gsso89_bullet_voice_bytes(void);
gv89_u32 gsso89_grenade_voice_bytes(void);
gv89_u32 gsso89_rocket_voice_bytes(void);
gv89_u32 gsso89_context_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
