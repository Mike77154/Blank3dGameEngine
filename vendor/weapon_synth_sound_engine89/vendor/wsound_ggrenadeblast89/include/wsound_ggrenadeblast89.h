#ifndef WSOUND_GGRENADEBLAST89_H
#define WSOUND_GGRENADEBLAST89_H

#include "wsound_gtypes89.h"
#include "wsound_geq6_89.h"
#include "wsound_gdist89.h"
#include "wsound_gchorus89.h"
#include "wsound_greverb89.h"

#define WS_GGB89_VERSION_MAJOR 1
#define WS_GGB89_VERSION_MINOR 0
#define WS_GGB89_VERSION_PATCH 2
#define WS_GGB89_PRESET_COUNT 8

enum ws_ggb89_preset_e {
    WS_GGB89_M67_OPEN = 0,
    WS_GGB89_40MM_HE_OPEN = 1,
    WS_GGB89_40MM_HEDP_HARD = 2,
    WS_GGB89_INDOOR_CONFINED = 3,
    WS_GGB89_CONCRETE_IMPACT = 4,
    WS_GGB89_DIRT_IMPACT = 5,
    WS_GGB89_DISTANT = 6,
    WS_GGB89_ARCADE_HEAVY = 7
};

typedef struct ws_ggb89_params_s {
    ws_gu16 positive_ms;
    ws_gu16 negative_ms;
    ws_gu16 body_ms;
    ws_gu16 debris_ms;
    ws_gu16 total_ms;

    ws_gs16 main_noise_q15;
    ws_gs16 low_noise_q15;
    ws_gs16 high_noise_q15;
    ws_gs16 fragment_noise_q15;
    ws_gs16 debris_noise_q15;
    ws_gs16 sine_q15;
    ws_gs16 saw_q15;
    ws_gs16 negative_q15;

    ws_gu16 sine_start_hz;
    ws_gu16 sine_end_hz;
    ws_gu16 saw_start_hz;
    ws_gu16 saw_end_hz;

    ws_gu16 fragment_density;
    ws_gu16 fragment_window_ms;

    ws_gs16 eq_gain_q12[6];

    ws_gs16 dist_drive_q8;
    ws_gs16 dist_mix_q15;

    ws_gu16 chorus_base_ms_x10;
    ws_gu16 chorus_depth_ms_x10;
    ws_gu16 chorus_rate_millihz;
    ws_gs16 chorus_wet_q15;
    ws_gs16 chorus_feedback_q15;

    ws_gs16 reverb_feedback_q15;
    ws_gs16 reverb_damp_q15;
    ws_gs16 reverb_wet_q15;

    ws_gs16 output_gain_q15;
} ws_ggb89_params;

typedef struct ws_ggb89_s {
    ws_gu32 sample_rate;
    ws_gu32 rng_a;
    ws_gu32 rng_b;
    ws_gu32 rng_c;
    ws_gu32 rng_d;
    ws_gu32 rng_e;

    ws_ggb89_params params;

    ws_gu32 age;
    ws_gu32 total_samples;
    ws_gu32 pos_samples;
    ws_gu32 neg_samples;
    ws_gu32 body_samples;
    ws_gu32 debris_samples;
    ws_gu32 fragment_window_samples;
    int active;

    ws_gs32 env_positive;
    ws_gs32 env_negative;
    ws_gs32 env_body;
    ws_gs32 env_debris;
    ws_gs32 env_fragment;

    ws_gs16 pos_decay_q15;
    ws_gs16 neg_decay_q15;
    ws_gs16 body_decay_q15;
    ws_gs16 debris_decay_q15;
    ws_gs16 fragment_decay_q15;

    ws_gs32 main_noise_state;
    ws_gs32 low_noise_state;
    ws_gs32 debris_noise_state;
    ws_gs32 fragment_noise_state;
    ws_gs32 fragment_smooth_state;
    ws_gs32 fragment_gate;
    ws_gs32 high_noise_state;
    ws_gs32 saw_filter_state;
    ws_gs32 high_prev;
    ws_gs16 low_hold;
    ws_gs16 debris_hold;
    ws_gu16 low_div;
    ws_gu16 debris_div;

    ws_gu32 sine_phase;
    ws_gu32 saw_phase;
    ws_gu32 sine_inc;
    ws_gu32 saw_inc;
    ws_gu32 sine_inc_end;
    ws_gu32 saw_inc_end;
    ws_gu32 sine_inc_step;
    ws_gu32 saw_inc_step;

    ws_geq6_89 eq;
    ws_gdist89 dist;
    ws_gchorus89 chorus;
    ws_greverb89 reverb;
} ws_ggb89;

void ws_ggb89_init(ws_ggb89 *synth, ws_gu32 sample_rate, ws_gu32 seed);
void ws_ggb89_reset(ws_ggb89 *synth);
int ws_ggb89_get_preset(int preset, ws_ggb89_params *out_params);
void ws_ggb89_set_params(ws_ggb89 *synth, const ws_ggb89_params *params);
void ws_ggb89_trigger(ws_ggb89 *synth, int preset, ws_gs16 intensity_q15);
void ws_ggb89_trigger_custom(ws_ggb89 *synth,
                             const ws_ggb89_params *params,
                             ws_gs16 intensity_q15);
ws_gs16 ws_ggb89_process(ws_ggb89 *synth);
void ws_ggb89_process_block(ws_ggb89 *synth, ws_gs16 *output, ws_gu32 frames);
int ws_ggb89_is_active(const ws_ggb89 *synth);
const char *ws_ggb89_preset_name(int preset);

#endif
