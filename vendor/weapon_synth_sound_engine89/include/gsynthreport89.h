#ifndef GSYNTHREPORT89_H
#define GSYNTHREPORT89_H

/*
 * gsynthreport89 v1.0
 * Heapless provider bridge for complete procedural weapon reports.
 * C89, fixed point, caller-owned pools, no allocation.
 */

#include "gweaponvoice89.h"
#include "gpaah89.h"
#include "gweaponbody89.h"
#include "gmuzzlegas89.h"
#include "gballisticcrack89.h"
#include "glatetail89.h"
#include "gcinemathump89.h"
#include "wsounddna89.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GSSR89_VERSION_MAJOR 1
#define GSSR89_VERSION_MINOR 0
#define GSSR89_VERSION_PATCH 0

typedef struct gssr89_params_s {
    gpaah89_preset_id report_preset;
    gwb89_preset_id body_preset;
    gmg89_preset_id gas_preset;
    gbc89_preset_id crack_preset;
    glt89_preset_id tail_preset;
    gct89_preset_id thump_preset;
    gv89_s16 report_gain_q15;
    gv89_s16 body_gain_q15;
    gv89_s16 gas_gain_q15;
    gv89_s16 crack_gain_q15;
    gv89_s16 thump_gain_q15;
    gv89_s16 tail_gain_q15;
    gv89_s16 output_gain_q15;
    gv89_u16 brightness_q15;
    gv89_u32 pitch_q16;
    gv89_u32 cycle_q16;
    gv89_s16 pan_q15;
    gv89_u16 distance_q15;
    gv89_u16 occlusion_q15;
    gv89_u16 focus_q15;
    gv89_s16 gain_q15;
    gv89_s16 priority_bias;
    gv89_u32 instance_key;
    gv89_u16 instance_limit;
} gssr89_params;

typedef struct gssr89_voice_s {
    gpaah89_state report;
    gwb89_context body;
    gmg89_context gas;
    gbc89_context crack;
    glt89_context tail;
    gct89_context thump;
    gssr89_params params;
    gv89_handle handle;
    gv89_u32 sample_rate;
    gv89_u32 seed;
    gv89_u32 serial;
    gv89_u8 in_use;
} gssr89_voice;

typedef struct gssr89_context_s {
    gssr89_voice *voices;
    gv89_u16 capacity;
    gv89_u32 sample_rate;
    gv89_u32 serial_counter;
} gssr89_context;

void gssr89_defaults(gssr89_params *params, gpaah89_preset_id report_preset);
void gssr89_apply_dna(gssr89_params *params,
                      const wsounddna89_profile *profile,
                      const wsounddna89_shot *shot,
                      wsounddna89_mode mode);
int gssr89_validate_params(const gssr89_params *params);
int gssr89_init(gssr89_context *ctx, gssr89_voice *storage,
                gv89_u16 capacity, gv89_u32 sample_rate);
void gssr89_reset(gssr89_context *ctx, gwv89_context *handler);
gv89_result gssr89_play(gssr89_context *ctx, gwv89_context *handler,
                         const gssr89_params *params, gv89_u32 seed,
                         gv89_handle *out_handle);
gv89_u16 gssr89_active_count(const gssr89_context *ctx);
gv89_u32 gssr89_voice_bytes(void);
gv89_u32 gssr89_context_bytes(void);

#ifdef __cplusplus
}
#endif
#endif
