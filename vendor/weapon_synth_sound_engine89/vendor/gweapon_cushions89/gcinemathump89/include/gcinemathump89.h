#ifndef GCINEMATHUMP89_H
#define GCINEMATHUMP89_H

/* Cinematic low-frequency sweetener. C89, fixed point, no heap. */
#ifdef __cplusplus
extern "C" {
#endif

#define GCT89_VERSION_MAJOR 1
#define GCT89_VERSION_MINOR 0
#define GCT89_VERSION_PATCH 0
#define GCT89_Q15_ONE 32767
#define GCT89_MAX_SAMPLE_RATE 48000

typedef signed short gct89_s16;
typedef unsigned short gct89_u16;
typedef signed int gct89_s32;
typedef unsigned int gct89_u32;

typedef enum gct89_preset_id {
    GCT89_PRESET_SUBTLE = 0,
    GCT89_PRESET_ACTION,
    GCT89_PRESET_SHOTGUN,
    GCT89_PRESET_SNIPER,
    GCT89_PRESET_LAUNCHER,
    GCT89_PRESET_COUNT
} gct89_preset_id;

typedef struct gct89_preset {
    gct89_u16 start_hz;
    gct89_u16 end_hz;
    gct89_u16 duration_ms;
    gct89_s16 tone_q15;
    gct89_s16 brown_noise_q15;
    gct89_s16 click_q15;
    gct89_s16 drive_q15;
    gct89_s16 output_gain_q15;
} gct89_preset;

typedef struct gct89_context {
    gct89_u32 sample_rate;
    gct89_u32 rng;
    gct89_preset preset;
    gct89_u32 pos;
    gct89_u32 total_samples;
    gct89_u16 phase;
    gct89_s32 brown;
    gct89_s16 intensity_q15;
    gct89_u16 active;
} gct89_context;

int gct89_platform_ok(void);
int gct89_get_preset(gct89_preset_id id, gct89_preset *out_preset);
int gct89_init(gct89_context *ctx, gct89_u32 sample_rate,
               const gct89_preset *preset, gct89_u32 seed);
void gct89_reset(gct89_context *ctx, gct89_u32 seed);
void gct89_trigger(gct89_context *ctx, gct89_s16 intensity_q15,
                   gct89_u32 seed);
gct89_s16 gct89_process_sample(gct89_context *ctx);
gct89_u32 gct89_render(gct89_context *ctx, gct89_s16 *output,
                       gct89_u32 frames);
int gct89_is_active(const gct89_context *ctx);
const char *gct89_preset_name(gct89_preset_id id);
gct89_u32 gct89_context_bytes(void);

#ifdef __cplusplus
}
#endif
#endif
