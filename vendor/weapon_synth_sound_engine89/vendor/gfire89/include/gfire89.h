#ifndef GFIRE89_H
#define GFIRE89_H

/*
    gfire89 v1.2 weapon-tuned
    Minimal procedural fire and flamethrower synthesizer.

    C89, fixed-point, caller-owned state, no heap allocation,
    no float or double, no math library.

    PCM output: signed 16-bit mono.
    Controls: Q0.15 in the range 0..32767.
*/

#ifdef __cplusplus
extern "C" {
#endif

typedef signed short gfire89_s16;
typedef unsigned short gfire89_u16;
typedef signed int gfire89_s32;
typedef unsigned int gfire89_u32;


#define GFIRE89_VERSION_MAJOR 1
#define GFIRE89_VERSION_MINOR 2
#define GFIRE89_VERSION_PATCH 0

#define GFIRE89_Q15_ZERO 0
#define GFIRE89_Q15_HALF 16384
#define GFIRE89_Q15_ONE 32767

#define GFIRE89_PRESET_FLAMETHROWER 0
#define GFIRE89_PRESET_TORCH 1
#define GFIRE89_PRESET_CAMPFIRE 2
#define GFIRE89_PRESET_BONFIRE 3
#define GFIRE89_PRESET_DEBRIS 4
#define GFIRE89_PRESET_COUNT 5

typedef struct gfire89_preset_s {
    gfire89_s16 roar_gain_q15;
    gfire89_s16 body_gain_q15;
    gfire89_s16 jet_gain_q15;
    gfire89_s16 hiss_gain_q15;
    gfire89_s16 crackle_gain_q15;
    gfire89_s16 burst_gain_q15;
    gfire89_s16 pulse_depth_q15;
    gfire89_s16 drive_q15;
    gfire89_s16 brightness_q15;
    gfire89_u16 crackle_rate_hz;
    gfire89_u16 vortex_rate_hz;
    gfire89_u16 crackle_decay_min;
    gfire89_u16 crackle_decay_max;
} gfire89_preset;

typedef struct gfire89_s {
    gfire89_u32 rng;
    gfire89_s32 sample_rate;

    gfire89_s32 gate_q15;
    gfire89_s32 env_q15;
    gfire89_s32 intensity_q15;
    gfire89_s32 airflow_q15;
    gfire89_s32 crackle_q15;
    gfire89_s32 size_q15;
    gfire89_s32 output_gain_q15;

    gfire89_s32 pressure_q15;
    gfire89_s32 drive_q15;
    gfire89_s32 brightness_q15;

    gfire89_s32 lp_roar_1;
    gfire89_s32 lp_roar_2;
    gfire89_s32 lp_body_fast;
    gfire89_s32 lp_body_slow;
    gfire89_s32 lp_hiss;
    gfire89_s32 lp_jet_fast;
    gfire89_s32 lp_jet_slow;
    gfire89_s32 mod_slow;
    gfire89_s32 mod_fast;

    gfire89_u32 chug_phase;
    gfire89_u32 flutter_phase;
    gfire89_u32 chug_increment;
    gfire89_u32 flutter_increment;

    gfire89_s32 vortex_countdown;
    gfire89_s32 vortex_env_q15;
    gfire89_s32 vortex_decay_shift;

    gfire89_s32 crack_env_q15;
    gfire89_s32 crack_impulse;
    gfire89_s32 crack_countdown;
    gfire89_s32 crack_decay_shift;
    gfire89_s32 cluster_left;
    gfire89_s32 cluster_gap;

    gfire89_s32 burst_env_q15;
    gfire89_s32 previous_gate;

    gfire89_s32 eq_low_state;
    gfire89_s32 eq_mid_state;

    gfire89_s32 shift_roar_1;
    gfire89_s32 shift_roar_2;
    gfire89_s32 shift_body_fast;
    gfire89_s32 shift_body_slow;
    gfire89_s32 shift_hiss;
    gfire89_s32 shift_jet_fast;
    gfire89_s32 shift_jet_slow;
    gfire89_s32 shift_mod_slow;
    gfire89_s32 shift_mod_fast;
    gfire89_s32 shift_eq_low;
    gfire89_s32 shift_eq_mid;

    gfire89_preset preset;
} gfire89;

void gfire89_init(gfire89 *fire, gfire89_s32 sample_rate, gfire89_u32 seed);
void gfire89_set_preset(gfire89 *fire, gfire89_s32 preset_id);

void gfire89_set_controls(
    gfire89 *fire,
    gfire89_s32 intensity_q15,
    gfire89_s32 airflow_q15,
    gfire89_s32 crackle_q15,
    gfire89_s32 size_q15
);

/*
    pressure: amount of non-tonal chug and burner flutter
    drive: soft saturation of the turbulent combustion bus
    brightness: high-band spectral weight
*/
void gfire89_set_detail(
    gfire89 *fire,
    gfire89_s32 pressure_q15,
    gfire89_s32 drive_q15,
    gfire89_s32 brightness_q15
);

void gfire89_set_gate(gfire89 *fire, gfire89_s32 gate_on);
void gfire89_set_output_gain(gfire89 *fire, gfire89_s32 gain_q15);

gfire89_s16 gfire89_process_sample(gfire89 *fire);
void gfire89_render_s16(gfire89 *fire, gfire89_s16 *dst, gfire89_s32 frames);

#ifdef __cplusplus
}
#endif

#endif
