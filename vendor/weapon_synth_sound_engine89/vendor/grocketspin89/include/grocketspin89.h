#ifndef GROCKETSPIN89_H
#define GROCKETSPIN89_H

#include <limits.h>

#if (USHRT_MAX != 65535U)
#error grocketspin89 requires 16-bit unsigned short
#endif

#if (UINT_MAX != 4294967295U)
#error grocketspin89 requires 32-bit unsigned int
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef signed short grs89_s16;
typedef unsigned short grs89_u16;
typedef signed int grs89_s32;
typedef unsigned int grs89_u32;

#define GRS89_EQ_BANDS 6
#define GRS89_REVERB_CAPACITY 4096

#define GRS89_PRESET_RPG7_SUSTAINER 0
#define GRS89_PRESET_HEAVY_ROCKET   1
#define GRS89_PRESET_FAST_MISSILE   2

typedef struct grs89_params {
    grs89_u32 seed;
    grs89_u16 ignition_delay_ms;
    grs89_u16 attack_ms;
    grs89_u16 sustain_ms;
    grs89_u16 release_ms;
    grs89_u16 drive_q8_8;
    grs89_s16 output_gain_q15;
    grs89_s16 eq_gain_q15[GRS89_EQ_BANDS];
    grs89_u16 spin_rate_millihz;
    grs89_s16 tremolo_depth_q15;
    grs89_u16 vibrato_rate_millihz;
    grs89_s16 vibrato_depth_q15;
    grs89_u16 pitch_rate_millihz;
    grs89_s16 pitch_depth_q15;
    grs89_s16 reverb_mix_q15;
    grs89_s16 reverb_feedback_q15;
} grs89_params;

typedef struct grs89_state {
    grs89_u32 sample_rate;
    grs89_u32 rng;
    grs89_u32 env_q31;
    grs89_u32 attack_step_q31;
    grs89_u32 release_step_q31;
    grs89_u32 stage_remaining;
    grs89_u32 tail_remaining;
    grs89_u32 spin_phase;
    grs89_u32 vibrato_phase;
    grs89_u32 pitch_phase;
    grs89_u32 spin_inc;
    grs89_u32 vibrato_inc;
    grs89_u32 pitch_inc;
    grs89_u16 stage;
    grs89_u16 reverb_index;
    grs89_u16 reverb_length;
    grs89_u16 reverb_tap0;
    grs89_u16 reverb_tap1;
    grs89_u16 reverb_tap2;
    grs89_u16 eq_alpha_q15[5];
    grs89_s32 lowpass_state[5];
    grs89_s16 reverb_buffer[GRS89_REVERB_CAPACITY];
    grs89_params params;
} grs89_state;

void grs89_params_preset(grs89_params *params, int preset_id);
int grs89_init(grs89_state *state, grs89_u32 sample_rate,
               const grs89_params *params);
void grs89_reset(grs89_state *state);
void grs89_trigger(grs89_state *state);
void grs89_stop(grs89_state *state);
int grs89_is_active(const grs89_state *state);
grs89_s16 grs89_process_sample(grs89_state *state);
void grs89_process_mono(grs89_state *state, grs89_s16 *output,
                        grs89_u32 sample_count);

#ifdef __cplusplus
}
#endif

#endif
