#ifndef WSOUND_ROCKETBLAST89_H
#define WSOUND_ROCKETBLAST89_H

#ifdef __cplusplus
extern "C" {
#endif

/* C89 integer assumptions used by the fixed-point DSP core. */
typedef char wsrb89_require_16_bit_short[(sizeof(short) == 2U) ? 1 : -1];
typedef char wsrb89_require_32_bit_int[(sizeof(int) == 4U) ? 1 : -1];

typedef signed short wsrb89_s16;
typedef unsigned short wsrb89_u16;
typedef signed int wsrb89_s32;
typedef unsigned int wsrb89_u32;

#define WSRB89_VERSION_MAJOR 1
#define WSRB89_VERSION_MINOR 4
#define WSRB89_VERSION_PATCH 0

#define WSRB89_EQ_BANDS 6
#define WSRB89_NOISE_OSCILLATORS 7

#define WSRB89_NOISE_CRACK 0
#define WSRB89_NOISE_BODY 1
#define WSRB89_NOISE_DEBRIS 2
#define WSRB89_NOISE_BODY_LOW_OCTAVE 3
#define WSRB89_NOISE_BODY_HIGH_OCTAVE 4
#define WSRB89_NOISE_RUMBLE 5
#define WSRB89_NOISE_CRACKLE 6
#define WSRB89_CHORUS_CAP 2304
#define WSRB89_EARLY_CAP 4608
#define WSRB89_EARLY_TAPS 5
#define WSRB89_COMB_L1_CAP 1800
#define WSRB89_COMB_L2_CAP 2500
#define WSRB89_COMB_L3_CAP 2900
#define WSRB89_COMB_R1_CAP 1950
#define WSRB89_COMB_R2_CAP 2650
#define WSRB89_COMB_R3_CAP 3050
#define WSRB89_ALLPASS_L1_CAP 448
#define WSRB89_ALLPASS_L2_CAP 640
#define WSRB89_ALLPASS_R1_CAP 480
#define WSRB89_ALLPASS_R2_CAP 672

#define WSRB89_Q15_ONE 32767
#define WSRB89_Q14_ONE 16384
#define WSRB89_Q12_ONE 4096

#define WSRB89_SVF_LOW 0
#define WSRB89_SVF_BAND 1
#define WSRB89_SVF_HIGH 2
#define WSRB89_SVF_NOTCH 3

#define WSRB89_PRESET_HEAVY_IMPACT 0
#define WSRB89_PRESET_CONCRETE_PAAS 1
#define WSRB89_PRESET_METAL_STRIKE 2
#define WSRB89_PRESET_AIRBURST 3
#define WSRB89_PRESET_INDOOR_BUNKER 4
#define WSRB89_PRESET_DISTANT_PAAS 5
#define WSRB89_PRESET_COMPACT_RPG 6
#define WSRB89_PRESET_COUNT 7

typedef struct wsrb89_params_s {
    wsrb89_s16 noise_level_q15[WSRB89_NOISE_OSCILLATORS];
    wsrb89_s16 sine_level_q15;
    wsrb89_s16 saw_level_q15;
    wsrb89_s16 shock_level_q15;

    wsrb89_u16 sub_start_hz_q8;
    wsrb89_u16 sub_end_hz_q8;
    wsrb89_u16 sub_pitch_drop_ms;

    wsrb89_u16 attack_ms;
    wsrb89_u16 decay_ms;
    wsrb89_u16 sustain_level_q15;
    wsrb89_u16 sustain_ms;
    wsrb89_u16 release_ms;
    wsrb89_u16 crack_decay_ms;
    wsrb89_u16 debris_decay_ms;
    wsrb89_u16 rumble_decay_ms;
    wsrb89_u16 crackle_decay_ms;

    wsrb89_u16 svf_cutoff_hz;
    wsrb89_u16 svf_damp_q15;
    wsrb89_u16 svf_mode;
    wsrb89_u16 rumble_cutoff_hz;
    wsrb89_u16 rumble_damp_q15;
    wsrb89_u16 crackle_cutoff_hz;
    wsrb89_u16 crackle_damp_q15;

    wsrb89_u16 motion_lfo_rate_millihz;
    wsrb89_u16 motion_lfo_depth_q15;

    wsrb89_s16 eq_gain_q14[WSRB89_EQ_BANDS];

    wsrb89_u16 drive_q12;
    wsrb89_u16 chorus_mix_q15;
    wsrb89_u16 chorus_base_ms;
    wsrb89_u16 chorus_depth_ms;
    wsrb89_u16 chorus_rate_millihz;

    wsrb89_u16 reverb_mix_q15;
    wsrb89_u16 reverb_feedback_q15;
    wsrb89_u16 reverb_damp_q15;
    wsrb89_u16 output_gain_q12;
} wsrb89_params;

typedef struct wsrb89_envelope_s {
    wsrb89_s32 level_q16;
    wsrb89_s32 target_q16;
    wsrb89_s32 step_q16;
    wsrb89_u32 remaining;
    wsrb89_u32 sustain_remaining;
    wsrb89_u16 stage;
} wsrb89_envelope;

typedef struct wsrb89_svf_s {
    wsrb89_s32 low;
    wsrb89_s32 band;
    wsrb89_s16 coeff_q15;
    wsrb89_s16 damp_q15;
} wsrb89_svf;

typedef struct wsrb89_workspace_s {
    wsrb89_s16 chorus[WSRB89_CHORUS_CAP];
    wsrb89_s16 early[WSRB89_EARLY_CAP];
    wsrb89_s16 comb_l1[WSRB89_COMB_L1_CAP];
    wsrb89_s16 comb_l2[WSRB89_COMB_L2_CAP];
    wsrb89_s16 comb_l3[WSRB89_COMB_L3_CAP];
    wsrb89_s16 comb_r1[WSRB89_COMB_R1_CAP];
    wsrb89_s16 comb_r2[WSRB89_COMB_R2_CAP];
    wsrb89_s16 comb_r3[WSRB89_COMB_R3_CAP];
    wsrb89_s16 allpass_l1[WSRB89_ALLPASS_L1_CAP];
    wsrb89_s16 allpass_l2[WSRB89_ALLPASS_L2_CAP];
    wsrb89_s16 allpass_r1[WSRB89_ALLPASS_R1_CAP];
    wsrb89_s16 allpass_r2[WSRB89_ALLPASS_R2_CAP];
} wsrb89_workspace;

typedef struct wsrb89_context_s {
    wsrb89_params params;
    wsrb89_workspace *workspace;
    wsrb89_u32 sample_rate;
    wsrb89_u32 age_samples;
    wsrb89_u32 active;
    wsrb89_u32 tail_remaining;

    wsrb89_u32 rng[WSRB89_NOISE_OSCILLATORS];
    wsrb89_s32 noise_body_state;
    wsrb89_s32 noise_body_low_octave_state;
    wsrb89_s32 noise_body_high_octave_low;
    wsrb89_s32 noise_debris_low;
    wsrb89_s32 noise_rumble_state;
    wsrb89_s32 noise_crackle_low;
    wsrb89_s32 room_cloud_state;

    wsrb89_u16 sub_phase;
    wsrb89_u16 sub_phase_inc;

    wsrb89_envelope master_env;
    wsrb89_envelope crack_env;
    wsrb89_envelope debris_env;
    wsrb89_envelope rumble_env;
    wsrb89_envelope crackle_env;

    wsrb89_svf main_svf;
    wsrb89_svf transient_svf;
    wsrb89_svf rumble_svf;
    wsrb89_svf crackle_svf;
    wsrb89_svf eq_svf[WSRB89_EQ_BANDS];
    wsrb89_svf transient_eq_svf[WSRB89_EQ_BANDS];

    wsrb89_u16 motion_lfo_phase;
    wsrb89_u16 motion_lfo_phase_step;
    wsrb89_u32 motion_lfo_phase_rem;
    wsrb89_u32 motion_lfo_phase_rem_step;
    wsrb89_u32 motion_lfo_phase_den;

    wsrb89_u16 chorus_write;
    wsrb89_u16 chorus_base_samples;
    wsrb89_u16 chorus_depth_samples;
    wsrb89_u16 chorus_phase;
    wsrb89_u16 chorus_phase_step;
    wsrb89_u32 chorus_phase_rem;
    wsrb89_u32 chorus_phase_rem_step;
    wsrb89_u32 chorus_phase_den;

    wsrb89_u16 early_write;
    wsrb89_u16 early_tap[WSRB89_EARLY_TAPS];
    wsrb89_s32 early_lp_l;
    wsrb89_s32 early_lp_r;

    wsrb89_u16 comb_l1_len;
    wsrb89_u16 comb_l2_len;
    wsrb89_u16 comb_l3_len;
    wsrb89_u16 comb_r1_len;
    wsrb89_u16 comb_r2_len;
    wsrb89_u16 comb_r3_len;
    wsrb89_u16 allpass_l1_len;
    wsrb89_u16 allpass_l2_len;
    wsrb89_u16 allpass_r1_len;
    wsrb89_u16 allpass_r2_len;
    wsrb89_u16 comb_l1_pos;
    wsrb89_u16 comb_l2_pos;
    wsrb89_u16 comb_l3_pos;
    wsrb89_u16 comb_r1_pos;
    wsrb89_u16 comb_r2_pos;
    wsrb89_u16 comb_r3_pos;
    wsrb89_u16 allpass_l1_pos;
    wsrb89_u16 allpass_l2_pos;
    wsrb89_u16 allpass_r1_pos;
    wsrb89_u16 allpass_r2_pos;
    wsrb89_s32 comb_l1_damp;
    wsrb89_s32 comb_l2_damp;
    wsrb89_s32 comb_l3_damp;
    wsrb89_s32 comb_r1_damp;
    wsrb89_s32 comb_r2_damp;
    wsrb89_s32 comb_r3_damp;

    wsrb89_s32 dc_x_l;
    wsrb89_s32 dc_y_l;
    wsrb89_s32 dc_x_r;
    wsrb89_s32 dc_y_r;
} wsrb89_context;

void wsrb89_get_preset(wsrb89_params *out_params, wsrb89_u16 preset_id);
void wsrb89_init(wsrb89_context *ctx, wsrb89_workspace *workspace,
                 wsrb89_u32 sample_rate, wsrb89_u32 seed);
void wsrb89_reset(wsrb89_context *ctx);
void wsrb89_set_params(wsrb89_context *ctx, const wsrb89_params *params);
void wsrb89_trigger(wsrb89_context *ctx, wsrb89_u16 velocity_q15);
void wsrb89_release(wsrb89_context *ctx);
void wsrb89_render_stereo(wsrb89_context *ctx, wsrb89_s16 *left,
                          wsrb89_s16 *right, wsrb89_u32 frames);
wsrb89_u16 wsrb89_is_active(const wsrb89_context *ctx);
wsrb89_u32 wsrb89_workspace_bytes(void);
wsrb89_u32 wsrb89_context_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
