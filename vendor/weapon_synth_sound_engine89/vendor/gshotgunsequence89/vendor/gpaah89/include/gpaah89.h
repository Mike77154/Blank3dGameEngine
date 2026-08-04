#ifndef GPAAH89_H
#define GPAAH89_H

/*
 * gpaah89 BASE PRESETS - procedural gunshot burst synthesizer
 * Strict C89 core, fixed-point arithmetic, caller-owned memory.
 *
 * Exact signal path:
 *   3 noise oscillators
 *   -> transient-only correlation of those same 3 streams
 *   -> independent no-attack envelopes
 *   -> 6 non-resonant band-pass EQ bands
 *   -> envelope-driven parallel distortion
 *   -> feed-forward stereo chorus
 *   -> 6-tap feed-forward FIR reverb
 */

#ifdef __cplusplus
extern "C" {
#endif

#define GPAAH89_VERSION_MAJOR 5
#define GPAAH89_VERSION_MINOR 0
#define GPAAH89_VERSION_PATCH 0

#define GPAAH89_Q15_ONE 32767
#define GPAAH89_EQ_BANDS 6
#define GPAAH89_NOISE_OSCS 3
#define GPAAH89_REVERB_TAPS 6
#define GPAAH89_CHORUS_BUFFER 2048
#define GPAAH89_REVERB_BUFFER 8192
#define GPAAH89_MAX_SAMPLE_RATE 48000

#define GPAAH89_NOISE_LOW 0
#define GPAAH89_NOISE_MID 1
#define GPAAH89_NOISE_HIGH 2

typedef signed short gpaah89_s16;
typedef unsigned short gpaah89_u16;
typedef signed int gpaah89_s32;
typedef unsigned int gpaah89_u32;

typedef enum gpaah89_preset_id {
    GPAAH89_PRESET_PISTOL = 0,
    GPAAH89_PRESET_MAGNUM,
    GPAAH89_PRESET_SHOTGUN,
    GPAAH89_PRESET_METRALLA,
    GPAAH89_PRESET_GATLING,
    GPAAH89_PRESET_ROCKET_LAUNCHER,
    GPAAH89_PRESET_SNIPER_RIFLE,
    GPAAH89_PRESET_COUNT
} gpaah89_preset_id;

typedef struct gpaah89_eq_band {
    gpaah89_u16 low_hz;
    gpaah89_u16 high_hz;
    /* Q14 permits non-resonant EQ boost up to just under 2.0. */
    gpaah89_s16 gain_q14;
} gpaah89_eq_band;

typedef struct gpaah89_noise_envelope {
    gpaah89_u16 decay_ms;
    gpaah89_u16 sustain_ms;
    gpaah89_u16 release_ms;
    gpaah89_s16 sustain_q15;
} gpaah89_noise_envelope;

typedef struct gpaah89_reverb_tap {
    gpaah89_u16 delay_ms;
    gpaah89_s16 gain_q15;
} gpaah89_reverb_tap;

typedef struct gpaah89_preset {
    gpaah89_s16 noise_mix_q15[GPAAH89_NOISE_OSCS];
    gpaah89_noise_envelope envelope[GPAAH89_NOISE_OSCS];

    /* Briefly aligns the existing three noise streams at the transient. */
    gpaah89_u16 transient_correlation_ms_x10;
    gpaah89_s16 transient_correlation_mix_q15;

    gpaah89_eq_band eq[GPAAH89_EQ_BANDS];

    gpaah89_u16 distortion_drive_start_q8;
    gpaah89_u16 distortion_drive_end_q8;
    gpaah89_s16 distortion_mix_q15;

    gpaah89_u16 chorus_left_ms_x10;
    gpaah89_u16 chorus_right_ms_x10;
    gpaah89_u16 chorus_depth_ms_x10;
    gpaah89_u16 chorus_rate_millihz;
    gpaah89_s16 chorus_mix_start_q15;
    gpaah89_s16 chorus_mix_end_q15;

    gpaah89_reverb_tap reverb[GPAAH89_REVERB_TAPS];
    gpaah89_s16 reverb_mix_q15;

    gpaah89_s16 output_gain_q15;
} gpaah89_preset;

typedef struct gpaah89_noise_osc {
    gpaah89_u32 rng;
    gpaah89_s32 target;
    gpaah89_s32 value;
    gpaah89_u16 counter;
    gpaah89_u16 period;
    gpaah89_u16 smooth_shift;
} gpaah89_noise_osc;

typedef struct gpaah89_band_state {
    gpaah89_s32 low_lp;
    gpaah89_s32 high_lp;
    gpaah89_s16 low_alpha_q15;
    gpaah89_s16 high_alpha_q15;
} gpaah89_band_state;

typedef struct gpaah89_envelope_state {
    gpaah89_u32 pos;
    gpaah89_u32 decay_samples;
    gpaah89_u32 sustain_samples;
    gpaah89_u32 release_samples;
    gpaah89_u16 stage;
    gpaah89_s32 level_q15;
} gpaah89_envelope_state;

typedef struct gpaah89_state {
    gpaah89_u32 sample_rate;
    gpaah89_preset preset;

    gpaah89_noise_osc noise[GPAAH89_NOISE_OSCS];
    gpaah89_envelope_state env[GPAAH89_NOISE_OSCS];
    gpaah89_band_state eq_state[GPAAH89_EQ_BANDS];

    gpaah89_s16 chorus_buffer[GPAAH89_CHORUS_BUFFER];
    gpaah89_u16 chorus_write;
    gpaah89_u32 chorus_phase;
    gpaah89_u32 chorus_phase_inc;

    gpaah89_s16 reverb_buffer[GPAAH89_REVERB_BUFFER];
    gpaah89_u16 reverb_write;

    gpaah89_u32 tail_pos;
    gpaah89_u32 trigger_pos;
    gpaah89_u32 correlation_samples;
    gpaah89_u16 active;
} gpaah89_state;

/* Returns 1 when the platform has the integer widths expected by the core. */
int gpaah89_platform_ok(void);

/* Copies one built-in BASE PRESET. Returns 1 on success. */
int gpaah89_get_preset(gpaah89_preset_id id, gpaah89_preset *out_preset);

/* Initializes a caller-owned state object. No allocation is performed. */
int gpaah89_init(gpaah89_state *state,
                 gpaah89_u32 sample_rate,
                 const gpaah89_preset *preset,
                 gpaah89_u32 seed);

/* Restarts all three no-attack envelopes and noise seeds. */
void gpaah89_trigger(gpaah89_state *state, gpaah89_u32 seed);

/* Renders signed 16-bit stereo interleaved PCM. Returns frames written. */
gpaah89_u32 gpaah89_render_stereo(gpaah89_state *state,
                                  gpaah89_s16 *stereo_interleaved,
                                  gpaah89_u32 frame_count);

/* Renders signed 16-bit mono PCM. Returns frames written. */
gpaah89_u32 gpaah89_render_mono(gpaah89_state *state,
                                gpaah89_s16 *mono,
                                gpaah89_u32 frame_count);

/* True while the dry envelopes or bounded wet tail are active. */
int gpaah89_is_active(const gpaah89_state *state);

#ifdef __cplusplus
}
#endif

#endif
