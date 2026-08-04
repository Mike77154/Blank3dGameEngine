#ifndef CHUECKA89_H
#define CHUECKA89_H

/*
   chuecka89 - procedural mechanical weapon Foley synthesizer
   C89 core, fixed-point signal path, caller-owned/static storage.
   CC0-1.0
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef signed short ch89_i16;
typedef unsigned short ch89_u16;
typedef signed int ch89_i32;
typedef unsigned int ch89_u32;

#define CH89_Q15_ONE             32767
#define CH89_MAX_STROKES         24
#define CH89_EQ_BANDS            6
#define CH89_VOICES_PER_STROKE   2
#define CH89_VOICE_SH            0
#define CH89_VOICE_ECKT          1
#define CH89_MAX_SAMPLE_RATE     48000
#define CH89_CHORUS_BUFFER       2048
#define CH89_REVERB_A_BUFFER     2048
#define CH89_REVERB_B_BUFFER     3072

typedef enum ch89_result_e {
    CH89_OK = 0,
    CH89_BAD_ARGUMENT = -1,
    CH89_UNSUPPORTED_RATE = -2,
    CH89_TOO_MANY_STROKES = -3
} ch89_result;

typedef enum ch89_preset_e {
    CH89_PRESET_SHOTGUN_PUMP = 0,
    CH89_PRESET_SHOTGUN_INSERT,
    CH89_PRESET_SHOTGUN_MULTI_INSERT,
    CH89_PRESET_PISTOL_SLIDE,
    CH89_PRESET_MAGNUM_HEAVY_ACTION,
    CH89_PRESET_REVOLVER_CYLINDER,
    CH89_PRESET_GRENADE_LAUNCHER,
    CH89_PRESET_ROCKET_LAUNCHER,
    CH89_PRESET_SMG_FEED_BURST,
    CH89_PRESET_MACHINE_GUN_FEED_BURST,
    CH89_PRESET_ATOM_SH,
    CH89_PRESET_ATOM_ECKT,
    CH89_PRESET_ATOM_SHEKT,
    CH89_PRESET_COUNT
} ch89_preset;


typedef enum ch89_effect_flags_e {
    CH89_EFFECT_NONE = 0,
    CH89_EFFECT_DISTORTION = 1,
    CH89_EFFECT_EQ = 2,
    CH89_EFFECT_CHORUS = 4,
    CH89_EFFECT_REVERB = 8,
    CH89_EFFECT_ALL = 15
} ch89_effect_flags;

typedef enum ch89_noise_color_e {
    CH89_NOISE_RAW = 0,
    CH89_NOISE_DARK = 1,
    CH89_NOISE_BRIGHT = 2
} ch89_noise_color;

typedef struct ch89_layer_s {
    ch89_u16 delay_ms;
    ch89_u16 attack_ms;
    ch89_u16 decay_ms;
    ch89_u16 sustain_ms;
    ch89_u16 release_ms;
    ch89_i16 sustain_q15;
    ch89_i16 level_q15;
    ch89_u16 color;
} ch89_layer;

typedef struct ch89_voice_s {
    ch89_layer envelope;
    ch89_i16 eq_start_q15[CH89_EQ_BANDS];
    ch89_i16 eq_end_q15[CH89_EQ_BANDS];
    ch89_u16 eq_sweep_ms;
    ch89_i16 distortion_q15;
} ch89_voice;

typedef struct ch89_stroke_s {
    ch89_u16 start_ms;
    ch89_voice sh;
    ch89_voice eckt;
} ch89_stroke;

typedef struct ch89_gesture_s {
    ch89_u16 stroke_count;
    ch89_u16 effect_flags;
    ch89_u16 duration_ms;
    ch89_u16 chorus_depth_samples;
    ch89_u16 chorus_rate_step;
    ch89_i16 chorus_wet_q15;
    ch89_i16 reverb_wet_q15;
    ch89_i16 output_gain_q15;
    ch89_stroke strokes[CH89_MAX_STROKES];
} ch89_gesture;

typedef struct ch89_eq_state_s {
    ch89_i32 lp[5];
} ch89_eq_state;

typedef struct ch89_voice_state_s {
    ch89_eq_state eq;
    ch89_i32 dark_state;
    ch89_i32 bright_lp;
} ch89_voice_state;

typedef struct ch89_context_s {
    ch89_u32 sample_rate;
    ch89_u32 rng;
    ch89_voice_state voice_state[CH89_MAX_STROKES][CH89_VOICES_PER_STROKE];
    ch89_i16 chorus[CH89_CHORUS_BUFFER];
    ch89_u16 chorus_write;
    ch89_u16 chorus_phase;
    ch89_i16 reverb_a[CH89_REVERB_A_BUFFER];
    ch89_i16 reverb_b[CH89_REVERB_B_BUFFER];
    ch89_u16 reverb_a_write;
    ch89_u16 reverb_b_write;
} ch89_context;

ch89_result ch89_init(ch89_context *ctx, ch89_u32 sample_rate, ch89_u32 seed);
void ch89_reset(ch89_context *ctx, ch89_u32 seed);

ch89_result ch89_make_preset(
    ch89_preset preset,
    ch89_u16 repetitions,
    ch89_u16 intensity_q15,
    ch89_gesture *out_gesture
);

ch89_u32 ch89_required_frames(const ch89_gesture *gesture, ch89_u32 sample_rate);

ch89_result ch89_render(
    ch89_context *ctx,
    const ch89_gesture *gesture,
    ch89_i16 *out_samples,
    ch89_u32 frame_capacity,
    ch89_u32 *out_frames_written
);

void ch89_set_effect_flags(ch89_gesture *gesture, ch89_u16 effect_flags);

const char *ch89_preset_name(ch89_preset preset);

#ifdef __cplusplus
}
#endif

#endif
