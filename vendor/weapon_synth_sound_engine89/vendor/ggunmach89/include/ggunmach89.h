#ifndef GGUNMACH89_H
#define GGUNMACH89_H

/*
 * ggunmach89 v1.0
 * Fixed-point rotary gun motor/mechanism synthesizer.
 *
 * C89, no malloc/realloc/free, no float/double, caller-owned state.
 * License: CC0-1.0
 */

#ifdef __cplusplus
extern "C" {
#endif

#define GGM89_VERSION_MAJOR 1
#define GGM89_VERSION_MINOR 0
#define GGM89_VERSION_PATCH 0

#define GGM89_Q15_ONE 32767
#define GGM89_Q12_ONE 4096
#define GGM89_EQ_BANDS 6
#define GGM89_EQ_SPLITS 5
#define GGM89_REVERB_SAMPLES 4096

#define GGM89_PRESET_GENERIC 0
#define GGM89_PRESET_M134D 1
#define GGM89_PRESET_M197 2
#define GGM89_PRESET_M61 3
#define GGM89_PRESET_GAU8 4
#define GGM89_PRESET_GAU22 5
#define GGM89_PRESET_COUNT 6

#define GGM89_STATE_OFF 0
#define GGM89_STATE_SPINUP 1
#define GGM89_STATE_RUNNING 2
#define GGM89_STATE_SPINDOWN 3

typedef signed short ggm89_s16;
typedef unsigned short ggm89_u16;
typedef signed long ggm89_s32;
typedef unsigned long ggm89_u32;

typedef struct ggm89_config_s {
    ggm89_u16 nominal_spm;
    ggm89_u16 barrel_count;
    ggm89_u16 spinup_ms;
    ggm89_u16 spindown_ms;

    ggm89_u16 sine_harmonic;
    ggm89_u16 saw_harmonic;
    ggm89_u16 vibrato_harmonic;

    ggm89_s16 sine_gain_q15;
    ggm89_s16 saw_gain_q15;
    ggm89_s16 noise_gain_q15;
    ggm89_s16 cam_gain_q15;

    ggm89_s16 tremolo_depth_q15;
    ggm89_s16 vibrato_depth_q15;
    ggm89_s16 load_dip_q15;

    ggm89_s16 distortion_drive_q12;
    ggm89_s16 reverb_mix_q15;
    ggm89_s16 reverb_feedback_q15;

    ggm89_s16 eq_gain_q12[GGM89_EQ_BANDS];
} ggm89_config;

typedef struct ggm89_state_s {
    ggm89_config config;
    ggm89_u32 sample_rate;

    ggm89_u32 phase_sine;
    ggm89_u32 phase_saw;
    ggm89_u32 phase_rotor;
    ggm89_u32 phase_cam;
    ggm89_u32 phase_vibrato;

    ggm89_u32 inc_sine;
    ggm89_u32 inc_saw;
    ggm89_u32 inc_rotor;
    ggm89_u32 inc_cam;
    ggm89_u32 inc_vibrato;

    ggm89_u32 noise_state;
    ggm89_s32 noise_lp;
    ggm89_s16 noise_alpha_q15;
    ggm89_s16 eq_alpha_q15[GGM89_EQ_SPLITS];
    ggm89_s32 eq_lp[GGM89_EQ_SPLITS];

    ggm89_u32 speed_q16;
    ggm89_u32 target_speed_q16;
    ggm89_u32 ramp_up_step;
    ggm89_u32 ramp_up_rem;
    ggm89_u32 ramp_up_den;
    ggm89_u32 ramp_up_error;
    ggm89_u32 ramp_down_step;
    ggm89_u32 ramp_down_rem;
    ggm89_u32 ramp_down_den;
    ggm89_u32 ramp_down_error;

    ggm89_s32 cam_env_q15;
    ggm89_s32 firing_load_q15;
    ggm89_s32 current_rpm;
    ggm89_u16 control_countdown;
    ggm89_u16 mode;

    ggm89_s16 reverb_buffer[GGM89_REVERB_SAMPLES];
    ggm89_u16 reverb_index;
} ggm89_state;

void ggm89_config_preset(ggm89_config *config, int preset_id);
int ggm89_init(ggm89_state *state,
               const ggm89_config *config,
               ggm89_u32 sample_rate);
void ggm89_reset(ggm89_state *state);

void ggm89_start(ggm89_state *state);
void ggm89_stop(ggm89_state *state);
void ggm89_set_target_speed_q15(ggm89_state *state, ggm89_s16 speed_q15);
void ggm89_set_firing_load_q15(ggm89_state *state, ggm89_s16 load_q15);
void ggm89_set_eq_gain_q12(ggm89_state *state,
                           int band,
                           ggm89_s16 gain_q12);
void ggm89_set_distortion_drive_q12(ggm89_state *state,
                                    ggm89_s16 drive_q12);
void ggm89_set_reverb_q15(ggm89_state *state,
                          ggm89_s16 mix_q15,
                          ggm89_s16 feedback_q15);

void ggm89_render_mono(ggm89_state *state,
                       ggm89_s16 *output,
                       ggm89_u16 frame_count);

ggm89_u16 ggm89_get_mode(const ggm89_state *state);
ggm89_u16 ggm89_get_current_rpm(const ggm89_state *state);
ggm89_u32 ggm89_state_size_bytes(void);
const char *ggm89_preset_name(int preset_id);
const char *ggm89_version_string(void);

#ifdef __cplusplus
}
#endif

#endif
