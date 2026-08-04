#ifndef GPUMP89_H
#define GPUMP89_H

/*
 * gpump89 v2_real
 * Procedural/micrograin pump-action mechanism synthesizer.
 * ISO C89 core, fixed-point/integer only, no allocation.
 */

typedef signed char gpump89_i8;
typedef unsigned char gpump89_u8;
typedef signed short gpump89_i16;
typedef unsigned short gpump89_u16;
typedef signed long gpump89_i32;
typedef unsigned long gpump89_u32;

#define GPUMP89_Q15_ONE 32767
#define GPUMP89_MAX_VOICES 8
#define GPUMP89_MAX_EVENTS 16
#define GPUMP89_GRAIN_RATE 22050UL

enum gpump89_stage {
    GPUMP89_STAGE_UNLOCK = 0,
    GPUMP89_STAGE_FRICTION_REAR = 1,
    GPUMP89_STAGE_EXTRACTOR = 2,
    GPUMP89_STAGE_REAR_STOP = 3,
    GPUMP89_STAGE_SHELL_EJECT = 4,
    GPUMP89_STAGE_FRICTION_FORWARD = 5,
    GPUMP89_STAGE_CARRIER = 6,
    GPUMP89_STAGE_BATTERY = 7,
    GPUMP89_STAGE_LOCK = 8,
    GPUMP89_STAGE_COUNT = 9
};

enum gpump89_preset_id {
    GPUMP89_PRESET_TIGHT_DRY = 0,
    GPUMP89_PRESET_LOOSE_SERVICE = 1,
    GPUMP89_PRESET_HEAVY = 2,
    GPUMP89_PRESET_WITH_SHELL = 3,
    GPUMP89_PRESET_CINEMATIC_DRY = 4
};

typedef struct gpump89_grain {
    const gpump89_i8 *samples;
    gpump89_u32 length;
    gpump89_u32 sample_rate;
} gpump89_grain;

typedef int (*gpump89_grain_provider)(
    void *user,
    int stage,
    gpump89_grain *out_grain);

typedef struct gpump89_params {
    gpump89_u32 sample_rate;
    gpump89_u32 cycle_ms;
    gpump89_i32 force_q15;
    gpump89_i32 slide_q15;
    gpump89_i32 wear_q15;
    gpump89_i32 variation_q15;
    gpump89_i32 master_q15;
    gpump89_u32 shell_enabled;
    gpump89_u32 seed;
} gpump89_params;

typedef struct gpump89_voice {
    const gpump89_i8 *samples;
    gpump89_u32 length;
    gpump89_u32 position_q16;
    gpump89_u32 step_q16;
    gpump89_i32 gain_q15;
    gpump89_u8 active;
} gpump89_voice;

typedef struct gpump89_event {
    gpump89_u32 frame;
    gpump89_u32 duration_frames;
    gpump89_i32 gain_q15;
    gpump89_u8 stage;
} gpump89_event;

typedef struct gpump89_context {
    gpump89_params params;
    gpump89_voice voices[GPUMP89_MAX_VOICES];
    gpump89_event events[GPUMP89_MAX_EVENTS];
    gpump89_u32 cursor;
    gpump89_u32 total_frames;
    gpump89_u32 rng;
    gpump89_u8 event_count;
    gpump89_u8 event_index;
    gpump89_u8 active;
    gpump89_grain_provider provider;
    void *provider_user;
} gpump89_context;

void gpump89_params_default(gpump89_params *params, gpump89_u32 sample_rate);
int gpump89_preset(gpump89_params *params, int preset_id, gpump89_u32 sample_rate);

void gpump89_init(gpump89_context *ctx, gpump89_u32 sample_rate, gpump89_u32 seed);
void gpump89_set_provider(gpump89_context *ctx,
                          gpump89_grain_provider provider,
                          void *user);

void gpump89_start_cycle(gpump89_context *ctx, const gpump89_params *params);
void gpump89_trigger_stage(gpump89_context *ctx,
                           int stage,
                           gpump89_i32 intensity_q15,
                           gpump89_u32 duration_ms);
void gpump89_stop(gpump89_context *ctx);

gpump89_u32 gpump89_render_i16(gpump89_context *ctx,
                               gpump89_i16 *out,
                               gpump89_u32 frames);
int gpump89_is_active(const gpump89_context *ctx);
gpump89_u32 gpump89_total_frames(const gpump89_context *ctx);

#endif
