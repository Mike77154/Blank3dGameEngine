#ifndef GGATLINGWHISTLE89_H
#define GGATLINGWHISTLE89_H

/*
 * ggatlingwhistle89
 * C89 fixed-point rotary-gun spool/body transition synthesizer.
 * v1.5: adds an indefinite A6 firing loop between matched spool transitions.
 *
 * Rules:
 * - No malloc/realloc/free.
 * - No heap ownership.
 * - No float/double.
 * - Caller owns all state and output buffers.
 * - Signed audio is Q15 / 16-bit PCM.
 */

#ifdef __cplusplus
extern "C" {
#endif

#define GGW89_EVENT_NONE       0
#define GGW89_EVENT_SPIN_UP    1
#define GGW89_EVENT_SPIN_DOWN  2
#define GGW89_EVENT_FIRE_LOOP  3

#define GGW89_CURVE_LINEAR     0
#define GGW89_CURVE_QUADRATIC  1
#define GGW89_CURVE_LATE       2

typedef struct GGW89_Config {
    unsigned long duration_ms;
    unsigned long tone_attack_ms;
    unsigned long noise_attack_ms;
    unsigned long release_ms;

    long start_hz;
    long end_hz;

    long tone_level_q15;
    long saw_level_q15;
    long body_level_q15;
    long upper_level_q15;
    long fm_level_q15;
    long noise_level_q15;
    long master_q15;

    long saw_pitch_ratio_q15;
    long body_pitch_ratio_q15;
    long upper_pitch_ratio_q15;
    long fm_carrier_ratio_q15;
    long fm_mod_ratio_q15;
    long fm_index_phase_q15;
    long fm_low_pass_hz;
    long saw_low_pass_hz;

    long noise_low_cut_hz;
    long noise_high_cut_hz;

    long eq_split1_hz;
    long eq_split2_hz;
    long eq_split3_hz;
    long eq_split4_hz;
    long eq_split5_hz;
    long eq_band1_gain_q15;
    long eq_band2_gain_q15;
    long eq_band3_gain_q15;
    long eq_band4_gain_q15;
    long eq_band5_gain_q15;
    long eq_band6_gain_q15;
    long output_low_pass_hz;

    long barrel_pass_depth_q15;

    long vibrato_millihz;
    long vibrato_end_millihz;
    long vibrato_depth_hz;
    long vibrato_depth_ratio_q15;
    long vibrato_harmonic_q15;

    int sweep_curve;
    int drive_q15;
    int loop_mode;
} GGW89_Config;

typedef struct GGW89_State {
    unsigned long sample_rate;
    unsigned long age_frames;
    unsigned long total_frames;
    unsigned long release_start_frame;
    unsigned long tone_attack_frames;
    unsigned long noise_attack_frames;

    unsigned long sweep_acc_q16;
    unsigned long sweep_step_q16;

    unsigned long sine_phase;
    unsigned long saw_phase;
    unsigned long body_phase;
    unsigned long upper_phase;
    unsigned long fm_carrier_phase;
    unsigned long fm_mod_phase;
    unsigned long lfo_phase;
    unsigned long lfo_inc;
    unsigned long lfo_start_inc;
    unsigned long lfo_end_inc;

    unsigned long lfsr;

    long tone_env_q15;
    long noise_env_q15;
    long tone_attack_step_q15;
    long noise_attack_step_q15;
    long release_step_q15;

    long noise_lp_low;
    long noise_lp_high;
    long saw_lp;
    long fm_lp;
    long eq_lp1;
    long eq_lp2;
    long eq_lp3;
    long eq_lp4;
    long eq_lp5;
    long output_lp;
    long noise_low_alpha_q15;
    long noise_high_alpha_q15;
    long saw_alpha_q15;
    long fm_alpha_q15;
    long eq_alpha1_q15;
    long eq_alpha2_q15;
    long eq_alpha3_q15;
    long eq_alpha4_q15;
    long eq_alpha5_q15;
    long output_alpha_q15;

    GGW89_Config cfg;
    int active;
    int event_id;
    int looping;
} GGW89_State;

void ggw89_init(GGW89_State *state, unsigned long sample_rate);
void ggw89_reset(GGW89_State *state);
void ggw89_get_default_config(int event_id, GGW89_Config *config);
void ggw89_trigger(GGW89_State *state, int event_id);
void ggw89_trigger_continuous(GGW89_State *state, int event_id);
void ggw89_trigger_custom(GGW89_State *state, int event_id,
                          const GGW89_Config *config);
short ggw89_process(GGW89_State *state);
void ggw89_render(GGW89_State *state, short *output,
                  unsigned long frame_count);
int ggw89_is_active(const GGW89_State *state);

#ifdef __cplusplus
}
#endif

#endif
