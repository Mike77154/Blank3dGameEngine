#ifndef GGUNTUBEROTATOR89_H
#define GGUNTUBEROTATOR89_H

/*
 * GGunTuberotator89
 * Dry rotating-barrel / tube-rotor sound synthesizer.
 * Strict C89, fixed-point only, no heap allocation.
 *
 * The caller owns the entire context and output buffers.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef signed short ggtr89_i16;
typedef unsigned short ggtr89_u16;
typedef signed int ggtr89_i32;
typedef unsigned int ggtr89_u32;

typedef char ggtr89_requires_16_bit_short[(sizeof(short) >= 2) ? 1 : -1];
typedef char ggtr89_requires_32_bit_int[(sizeof(int) >= 4) ? 1 : -1];

#define GGTR89_Q12_ONE 4096
#define GGTR89_Q15_ONE 32767
#define GGTR89_EQ_BANDS 6
#define GGTR89_REVERB_CAPACITY 2048

#define GGTR89_PRESET_LIGHT   0
#define GGTR89_PRESET_MEDIUM  1
#define GGTR89_PRESET_HEAVY   2

#define GGTR89_STAGE_IDLE   0
#define GGTR89_STAGE_START  1
#define GGTR89_STAGE_LOOP   2
#define GGTR89_STAGE_STOP   3

typedef struct ggtr89_config_s {
    ggtr89_i32 sample_rate;
    ggtr89_i32 barrel_count;
    ggtr89_i32 initial_rpm;
    ggtr89_i32 target_rpm;
    ggtr89_i32 start_ms;
    ggtr89_i32 stop_ms;
    ggtr89_i32 sine_hz;
    ggtr89_i32 noise_decay_ms;
    ggtr89_i32 low_noise_decay_ms;
    ggtr89_i32 sine_decay_ms;
    ggtr89_i32 noise_gain_q12;
    ggtr89_i32 low_noise_gain_q12;
    ggtr89_i32 sine_gain_q12;
    ggtr89_i32 pulse_jitter_q15;
    ggtr89_i32 eq_gain_q12[GGTR89_EQ_BANDS];
    ggtr89_i32 drive_q12;
    ggtr89_i32 distortion_mix_q15;
    ggtr89_i32 reverb_wet_q15;
    ggtr89_i32 reverb_feedback_q15;
    ggtr89_i32 output_gain_q12;
} ggtr89_config;

typedef struct ggtr89_context_s {
    ggtr89_config cfg;

    ggtr89_i32 stage;
    ggtr89_i32 stage_frame;
    ggtr89_i32 stage_total_frames;
    ggtr89_i32 rpm_q16;
    ggtr89_i32 stop_start_rpm_q16;
    ggtr89_u32 passage_phase_q16;
    ggtr89_u32 sine_phase_q16;

    ggtr89_i32 env_noise_q15;
    ggtr89_i32 env_low_q15;
    ggtr89_i32 env_sine_q15;
    ggtr89_i32 decay_noise_q15;
    ggtr89_i32 decay_low_q15;
    ggtr89_i32 decay_sine_q15;

    ggtr89_u32 rng;
    ggtr89_i32 low_noise_hold;
    ggtr89_i32 low_noise_lp;
    ggtr89_i32 low_noise_div;
    ggtr89_i32 pulse_gain_q15;

    ggtr89_i32 eq_lp[5];
    ggtr89_i32 eq_alpha_q15[5];

    ggtr89_i16 reverb_a[GGTR89_REVERB_CAPACITY];
    ggtr89_i16 reverb_b[GGTR89_REVERB_CAPACITY];
    ggtr89_i32 reverb_pos_a;
    ggtr89_i32 reverb_pos_b;
    ggtr89_i32 reverb_len_a;
    ggtr89_i32 reverb_len_b;
    ggtr89_i32 reverb_damp;
} ggtr89_context;

void ggtr89_config_preset(ggtr89_config *cfg, ggtr89_i32 sample_rate,
                          ggtr89_i32 preset);
int ggtr89_init(ggtr89_context *ctx, const ggtr89_config *cfg);
void ggtr89_reset(ggtr89_context *ctx);
void ggtr89_start(ggtr89_context *ctx);
void ggtr89_stop(ggtr89_context *ctx);
void ggtr89_force_loop(ggtr89_context *ctx);
void ggtr89_set_eq_gain(ggtr89_context *ctx, ggtr89_i32 band,
                        ggtr89_i32 gain_q12);
void ggtr89_render(ggtr89_context *ctx, ggtr89_i16 *output,
                   ggtr89_i32 frame_count);
int ggtr89_is_active(const ggtr89_context *ctx);
int ggtr89_get_stage(const ggtr89_context *ctx);

#ifdef __cplusplus
}
#endif

#endif
