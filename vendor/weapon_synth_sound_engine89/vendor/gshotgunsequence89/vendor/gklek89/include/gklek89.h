#ifndef GKLEK89_H
#define GKLEK89_H

/*
 * gklek89 v1.0
 * Cheap dry mechanical "klek/click" synthesizer.
 * Strict C89, integer/fixed-point signal path, no heap, no float/double,
 * no math.h. Caller-owned context. Fixed 44100 Hz.
 *
 * Signal model:
 *   1) one-sample impact + tiny rebound,
 *   2) high/band-passed pseudo-random noise under a fast decay,
 *   3) two very short feedback combs for a damped metal/receiver ring.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define GKL89_VERSION_MAJOR 1
#define GKL89_VERSION_MINOR 0
#define GKL89_VERSION_PATCH 0
#define GKL89_SAMPLE_RATE 44100UL
#define GKL89_Q15_ONE 32767
#define GKL89_COMB_CAPACITY 32

typedef signed short gkl89_s16;
typedef signed long gkl89_s32;
typedef unsigned long gkl89_u32;

typedef enum gkl89_preset_id_e {
    GKL89_PRESET_DRY_CLICK = 0,
    GKL89_PRESET_STEEL_KLEK,
    GKL89_PRESET_CARRIER_KLEK,
    GKL89_PRESET_REAR_STOP,
    GKL89_PRESET_COUNT
} gkl89_preset_id;

typedef struct gkl89_preset_s {
    const char *name;
    unsigned short duration_ms;
    unsigned short rebound_ms;
    gkl89_s16 impact_q15;
    gkl89_s16 rebound_q15;
    gkl89_s16 noise_q15;
    gkl89_s16 ring_q15;
    gkl89_s16 noise_decay_q15;
    gkl89_s16 ring_decay_q15;
    gkl89_s16 fast_alpha_q15;
    gkl89_s16 slow_alpha_q15;
    unsigned char comb_delay_a;
    unsigned char comb_delay_b;
    gkl89_s16 comb_feedback_q15;
    gkl89_s16 drive_q15;
} gkl89_preset;

typedef struct gkl89_context_s {
    gkl89_preset preset;
    gkl89_u32 rng;
    gkl89_u32 delay_samples;
    gkl89_u32 sample_clock;
    gkl89_u32 end_clock;
    gkl89_u32 rebound_sample;
    gkl89_s16 noise_env_q15;
    gkl89_s16 ring_env_q15;
    gkl89_s16 previous_noise;
    gkl89_s16 low_fast;
    gkl89_s16 low_slow;
    gkl89_s16 comb_a[GKL89_COMB_CAPACITY];
    gkl89_s16 comb_b[GKL89_COMB_CAPACITY];
    unsigned char comb_pos_a;
    unsigned char comb_pos_b;
    unsigned char active;
} gkl89_context;

void gkl89_init(gkl89_context *ctx, gkl89_u32 seed);
void gkl89_reset(gkl89_context *ctx, gkl89_u32 seed);
int gkl89_get_preset(gkl89_preset_id preset_id, gkl89_preset *out_preset);
const char *gkl89_preset_name(gkl89_preset_id preset_id);

/* delay_ms places the whole transient later without an external scheduler. */
int gkl89_trigger(gkl89_context *ctx, gkl89_preset_id preset_id,
                  gkl89_u32 seed, unsigned short delay_ms);
int gkl89_trigger_custom(gkl89_context *ctx, const gkl89_preset *preset,
                         gkl89_u32 seed, unsigned short delay_ms);

gkl89_s16 gkl89_process_sample(gkl89_context *ctx);
gkl89_u32 gkl89_render_mono(gkl89_context *ctx, gkl89_s16 *dst,
                            gkl89_u32 frames);
int gkl89_is_active(const gkl89_context *ctx);
gkl89_u32 gkl89_context_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
