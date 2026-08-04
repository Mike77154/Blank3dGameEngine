#ifndef GROCKETWHISTLE89_H
#define GROCKETWHISTLE89_H

/*
    grocketwhistle89 v1.2
    Sustained rocket / missile trajectory whistle synthesizer.

    Core rules:
    - ISO C89 source style
    - integer fixed-point arithmetic only
    - no dynamic allocation
    - caller-owned state
    - no dependency on stdio, stdlib or math in the core
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef signed char gwh89_s8;
typedef unsigned char gwh89_u8;
typedef signed short gwh89_s16;
typedef unsigned short gwh89_u16;
typedef signed int gwh89_s32;
typedef unsigned int gwh89_u32;

typedef char gwh89_int_must_be_32_bits[(sizeof(gwh89_s32) == 4) ? 1 : -1];
typedef char gwh89_short_must_be_16_bits[(sizeof(gwh89_s16) == 2) ? 1 : -1];

#define GWH89_Q15_ONE                    32767
#define GWH89_CHORUS_BUFFER_SAMPLES      2048
#define GWH89_CHORUS_BUFFER_MASK         2047
#define GWH89_AIR_BUFFER_SAMPLES         256
#define GWH89_AIR_BUFFER_MASK            255
#define GWH89_EQ_BANDS                   6
#define GWH89_EQ_CROSSOVERS              5
#define GWH89_OUTPUT_EQ_BANDS            6
#define GWH89_OUTPUT_EQ_CROSSOVERS       5
#define GWH89_REVERB_COMB1_BUFFER        1536
#define GWH89_REVERB_COMB2_BUFFER        2048
#define GWH89_REVERB_COMB3_BUFFER        3072
#define GWH89_PRESET_COUNT               7

#define GWH89_PRESET_RPG7_SUSTAINED      0
#define GWH89_PRESET_SMAW_SHORT          1
#define GWH89_PRESET_JAVELIN_TWO_STAGE   2
#define GWH89_PRESET_AT4_FAST            3
#define GWH89_PRESET_GUIDED_FLYBY        4
#define GWH89_PRESET_HEAVY_ROCKET        5
#define GWH89_PRESET_CHIFLADORA_AIR_REF  6

#define GWH89_STAGE_IDLE                 0
#define GWH89_STAGE_ACTIVE               1
#define GWH89_STAGE_RELEASE              2
#define GWH89_STAGE_REVERB_TAIL          3
#define GWH89_STAGE_DONE                 4

#define GWH89_EQ_SUB_180                  0
#define GWH89_EQ_BODY_180_420             1
#define GWH89_EQ_LOW_WHISTLE_420_900      2
#define GWH89_EQ_FUNDAMENTAL_900_1800     3
#define GWH89_EQ_EDGE_1800_3600           4
#define GWH89_EQ_AIR_ABOVE_3600           5

#define GWH89_OUTPUT_EQ_RUMBLE_120        0
#define GWH89_OUTPUT_EQ_LOW_120_300       1
#define GWH89_OUTPUT_EQ_CORE_300_900      2
#define GWH89_OUTPUT_EQ_WHISTLE_900_2400  3
#define GWH89_OUTPUT_EQ_EDGE_2400_6000    4
#define GWH89_OUTPUT_EQ_AIR_ABOVE_6000    5

typedef struct gwh89_preset {
    const char *name;

    gwh89_s32 start_freq_hz;
    gwh89_s32 peak_freq_hz;
    gwh89_s32 sustain_freq_hz;
    gwh89_s32 end_freq_hz;

    gwh89_s32 attack_ms;
    gwh89_s32 rise_ms;
    gwh89_s32 settle_ms;
    gwh89_s32 auto_hold_ms;
    gwh89_s32 release_ms;

    gwh89_s32 tone_gain_q15;
    gwh89_s32 air_gain_q15;
    gwh89_s32 master_gain_q15;

    gwh89_s32 air_delay_ms;
    gwh89_s32 air_attack_ms;
    gwh89_s32 air_hp_coeff_q15;
    gwh89_s32 air_lp_coeff_q15;

    gwh89_s32 vibrato_rate_millihz;
    gwh89_s32 vibrato_depth_q15;
    gwh89_s32 drift_rate_millihz;
    gwh89_s32 drift_depth_q15;

    gwh89_s32 harmonic2_gain_q15;
    gwh89_s32 harmonic3_gain_q15;
    gwh89_s32 aerated_tone_gain_q15;
    gwh89_s32 pitch_roughness_q15;
    gwh89_s32 amplitude_roughness_q15;
    gwh89_s32 roughness_slew_q15;
    gwh89_s32 air_spatial_mix_q15;
    gwh89_s32 air_delay_left_samples;
    gwh89_s32 air_delay_right_samples;

    gwh89_s32 chorus_rate_millihz;
    gwh89_s32 chorus_delay_left_samples;
    gwh89_s32 chorus_delay_right_samples;
    gwh89_s32 chorus_depth_samples;
    gwh89_s32 chorus_mix_q15;

    gwh89_s32 drive_q8;
    gwh89_s32 soft_clip_knee;

    gwh89_s32 eq_gain_q15[GWH89_EQ_BANDS];

    gwh89_s32 reverb_enabled;
    gwh89_s32 reverb_wet_q15;
    gwh89_s32 reverb_feedback_q15;
    gwh89_s32 reverb_damping_q15;
    gwh89_s32 reverb_tail_ms;

    gwh89_s32 output_eq_gain_q15[GWH89_OUTPUT_EQ_BANDS];
} gwh89_preset;

typedef struct gwh89_state {
    gwh89_s32 sample_rate;
    const gwh89_preset *preset;

    gwh89_u32 tone_phase;
    gwh89_u32 harmonic2_phase;
    gwh89_u32 harmonic3_phase;
    gwh89_u32 vibrato_phase;
    gwh89_u32 drift_phase;
    gwh89_u32 chorus_phase;
    gwh89_u32 noise_state;

    gwh89_s32 age_samples;
    gwh89_s32 release_age_samples;
    gwh89_s32 release_start_freq_hz;
    gwh89_s32 stage;

    gwh89_s32 attack_samples;
    gwh89_s32 rise_samples;
    gwh89_s32 settle_samples;
    gwh89_s32 auto_hold_samples;
    gwh89_s32 release_samples;
    gwh89_s32 air_delay_samples;
    gwh89_s32 air_attack_samples;

    gwh89_s32 radial_velocity_mps;
    gwh89_s32 distance_gain_q15;
    gwh89_s32 pitch_scale_q15;

    gwh89_s32 roughness_value;
    gwh89_s32 noise_prev_x_1;
    gwh89_s32 noise_hp_y_1;
    gwh89_s32 noise_prev_x_2;
    gwh89_s32 noise_hp_y_2;
    gwh89_s32 noise_lp_y_1;
    gwh89_s32 noise_lp_y_2;

    gwh89_s16 chorus_buffer[GWH89_CHORUS_BUFFER_SAMPLES];
    gwh89_s32 chorus_write_index;

    gwh89_s16 air_buffer[GWH89_AIR_BUFFER_SAMPLES];
    gwh89_s32 air_write_index;

    gwh89_s32 eq_enabled;
    gwh89_s32 eq_coeff_q15[GWH89_EQ_CROSSOVERS];
    gwh89_s32 eq_gain_q15[GWH89_EQ_BANDS];
    gwh89_s32 eq_lp_left[GWH89_EQ_CROSSOVERS];
    gwh89_s32 eq_lp_right[GWH89_EQ_CROSSOVERS];

    gwh89_s32 reverb_enabled;
    gwh89_s32 reverb_wet_q15;
    gwh89_s32 reverb_feedback_q15;
    gwh89_s32 reverb_damping_q15;
    gwh89_s32 reverb_tail_samples;
    gwh89_s32 reverb_tail_age_samples;
    gwh89_s32 reverb_comb1_length;
    gwh89_s32 reverb_comb2_length;
    gwh89_s32 reverb_comb3_length;
    gwh89_s32 reverb_comb1_index;
    gwh89_s32 reverb_comb2_index;
    gwh89_s32 reverb_comb3_index;
    gwh89_s32 reverb_comb1_damp;
    gwh89_s32 reverb_comb2_damp;
    gwh89_s32 reverb_comb3_damp;
    gwh89_s16 reverb_comb1[GWH89_REVERB_COMB1_BUFFER];
    gwh89_s16 reverb_comb2[GWH89_REVERB_COMB2_BUFFER];
    gwh89_s16 reverb_comb3[GWH89_REVERB_COMB3_BUFFER];

    gwh89_s32 output_eq_enabled;
    gwh89_s32 output_eq_coeff_q15[GWH89_OUTPUT_EQ_CROSSOVERS];
    gwh89_s32 output_eq_gain_q15[GWH89_OUTPUT_EQ_BANDS];
    gwh89_s32 output_eq_lp_left[GWH89_OUTPUT_EQ_CROSSOVERS];
    gwh89_s32 output_eq_lp_right[GWH89_OUTPUT_EQ_CROSSOVERS];
} gwh89_state;

const gwh89_preset *gwh89_get_preset(gwh89_s32 preset_id);
const char *gwh89_get_preset_name(gwh89_s32 preset_id);
const char *gwh89_get_eq_band_name(gwh89_s32 band);
gwh89_s32 gwh89_get_eq_crossover_hz(gwh89_s32 crossover_index);
const char *gwh89_get_output_eq_band_name(gwh89_s32 band);
gwh89_s32 gwh89_get_output_eq_crossover_hz(gwh89_s32 crossover_index);

void gwh89_init(gwh89_state *state, gwh89_s32 sample_rate, gwh89_u32 seed);
void gwh89_reset(gwh89_state *state);
void gwh89_trigger(gwh89_state *state, const gwh89_preset *preset);
void gwh89_trigger_preset(gwh89_state *state, gwh89_s32 preset_id);
void gwh89_release(gwh89_state *state);

void gwh89_set_motion(
    gwh89_state *state,
    gwh89_s32 radial_velocity_mps,
    gwh89_s32 distance_gain_q15
);

void gwh89_set_pitch_scale_q15(gwh89_state *state, gwh89_s32 pitch_scale_q15);
void gwh89_set_auto_hold_ms(gwh89_state *state, gwh89_s32 hold_ms);

void gwh89_set_eq_enabled(gwh89_state *state, gwh89_s32 enabled);
void gwh89_set_eq_band_gain_q15(
    gwh89_state *state,
    gwh89_s32 band,
    gwh89_s32 gain_q15
);
void gwh89_set_eq_gains_q15(
    gwh89_state *state,
    const gwh89_s32 gains_q15[GWH89_EQ_BANDS]
);
void gwh89_set_eq_flat(gwh89_state *state);

void gwh89_set_output_eq_enabled(gwh89_state *state, gwh89_s32 enabled);
void gwh89_set_output_eq_band_gain_q15(
    gwh89_state *state,
    gwh89_s32 band,
    gwh89_s32 gain_q15
);
void gwh89_set_output_eq_gains_q15(
    gwh89_state *state,
    const gwh89_s32 gains_q15[GWH89_OUTPUT_EQ_BANDS]
);
void gwh89_set_output_eq_flat(gwh89_state *state);

void gwh89_set_reverb_enabled(gwh89_state *state, gwh89_s32 enabled);
void gwh89_set_reverb(
    gwh89_state *state,
    gwh89_s32 enabled,
    gwh89_s32 wet_q15,
    gwh89_s32 feedback_q15,
    gwh89_s32 damping_q15,
    gwh89_s32 tail_ms
);

void gwh89_clear_effect_memory(gwh89_state *state);

gwh89_s32 gwh89_is_active(const gwh89_state *state);
gwh89_s32 gwh89_get_current_frequency_hz(const gwh89_state *state);
gwh89_s32 gwh89_get_estimated_total_samples(const gwh89_state *state);

gwh89_s16 gwh89_render_mono_sample(gwh89_state *state);
void gwh89_render_mono(gwh89_state *state, gwh89_s16 *out, gwh89_s32 frames);
void gwh89_render_stereo(
    gwh89_state *state,
    gwh89_s16 *out_interleaved,
    gwh89_s32 frames
);

#ifdef __cplusplus
}
#endif

#endif
