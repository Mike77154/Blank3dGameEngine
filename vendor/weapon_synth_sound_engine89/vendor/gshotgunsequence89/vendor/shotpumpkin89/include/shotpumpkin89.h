#ifndef SHOTPUMPKIN89_H
#define SHOTPUMPKIN89_H

/*
 * shotpumpkin89 - sample-free pump-action shotgun Foley synthesizer.
 * C89, fixed-point/integer only, caller-owned state, no heap.
 *
 * Public domain / CC0-1.0.
 */

#include <limits.h>

#if UINT_MAX < 4294967295U
#error shotpumpkin89 requires an unsigned int of at least 32 bits
#endif

#if SHRT_MAX < 32767
#error shotpumpkin89 requires a signed short of at least 16 bits
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define SHOTPUMPKIN89_VERSION_MAJOR 1
#define SHOTPUMPKIN89_VERSION_MINOR 1
#define SHOTPUMPKIN89_VERSION_PATCH 0

#define SP89_CHORUS_CAP 1024
#define SP89_REVERB_CAP 1024
#define SP89_EQ6_BANDS 6
#define SP89_EQ6_CROSSOVERS 5

typedef signed short sp89_sample;

typedef enum sp89_preset {
    SP89_PRESET_REALISTIC = 0,
    SP89_PRESET_HEAVY = 1,
    SP89_PRESET_OILED = 2,
    SP89_PRESET_WORN = 3,
    SP89_PRESET_CINEMATIC = 4,
    SP89_PRESET_REMINGTON_870 = 5,
    SP89_PRESET_MOSSBERG_590 = 6,
    SP89_PRESET_WINCHESTER_1300 = 7,
    SP89_PRESET_ITHACA_37 = 8,
    SP89_PRESET_BENELLI_NOVA = 9,
    SP89_PRESET_COUNT = 10
} sp89_preset;

typedef enum sp89_phase {
    SP89_PHASE_IDLE = 0,
    SP89_PHASE_PULL = 1,
    SP89_PHASE_GAP = 2,
    SP89_PHASE_PUMP = 3
} sp89_phase;

typedef struct sp89_config {
    unsigned int sample_rate;

    unsigned int pull_ms;
    unsigned int gap_ms;
    unsigned int pump_ms;

    /* Q8 linear gains: 256 = 1.0. */
    signed short master_gain_q8;
    signed short pull_gain_q8;
    signed short pump_gain_q8;
    signed short friction_gain_q8;
    signed short impact_gain_q8;
    signed short chirr_gain_q8;
    signed short drive_q8;

    /* Legacy 3-band EQ controls, retained for source compatibility. */
    signed short eq_low_q8;
    signed short eq_mid_q8;
    signed short eq_high_q8;
    unsigned int eq_low_hz;
    unsigned int eq_high_hz;

    /* Six-band crossover EQ. Set eq6_enabled to zero for legacy EQ. */
    unsigned short eq6_enabled;
    signed short eq6_gain_q8[SP89_EQ6_BANDS];
    unsigned int eq6_cross_hz[SP89_EQ6_CROSSOVERS];

    /* Filter frequencies in Hz. */
    unsigned int friction_hp_hz;
    unsigned int friction_lp_hz;
    unsigned int impact_hp_hz;
    unsigned int chirr_hp_hz;
    unsigned int chirr_lp_hz;

    /* Tiny rail-chirr LFO: rate in mHz, depth in Q15. */
    unsigned int chirr_lfo_rate_millihz;
    unsigned short chirr_lfo_depth_q15;

    /* Q15 wet mixes and feedback. */
    unsigned short chorus_mix_q15;
    unsigned short reverb_mix_q15;
    unsigned short reverb_feedback_q15;

    unsigned int chorus_rate_millihz;
    unsigned int chorus_base_ms;
    unsigned int chorus_depth_ms;

    unsigned int seed;
    unsigned int variation;
} sp89_config;

typedef struct sp89_state {
    sp89_config cfg;
    sp89_phase phase;

    unsigned int rng;
    unsigned int sample_pos;
    unsigned int phase_samples;
    unsigned int gap_samples;
    unsigned int pending_pump;
    unsigned int pulse_index;
    unsigned int next_pulse_sample;

    signed int burst_env;
    signed int burst_step;
    signed int impact_env;
    signed int impact_step;
    unsigned int impact_fired;

    signed short friction_lp;
    signed short friction_hp_lp;
    signed short impact_hp_lp;
    signed short chirr_hp_lp;
    signed short chirr_lp;
    signed short eq_low_lp;
    signed short eq_high_lp;
    signed short eq6_lp[SP89_EQ6_CROSSOVERS];
    signed short dc_x1;
    signed short dc_y1;

    unsigned short friction_lp_alpha;
    unsigned short friction_hp_alpha;
    unsigned short impact_hp_alpha;
    unsigned short chirr_hp_alpha;
    unsigned short chirr_lp_alpha;
    unsigned short eq_low_alpha;
    unsigned short eq_high_alpha;
    unsigned short eq6_alpha[SP89_EQ6_CROSSOVERS];

    unsigned int chirr_lfo_phase;
    unsigned int chirr_lfo_inc;

    sp89_sample chorus_buffer[SP89_CHORUS_CAP];
    unsigned int chorus_write;
    unsigned int chorus_phase;
    unsigned int chorus_inc;
    unsigned int chorus_base_samples;
    unsigned int chorus_depth_samples;

    sp89_sample reverb_a[SP89_REVERB_CAP];
    sp89_sample reverb_b[SP89_REVERB_CAP];
    sp89_sample reverb_c[SP89_REVERB_CAP];
    unsigned int reverb_a_pos;
    unsigned int reverb_b_pos;
    unsigned int reverb_c_pos;
    unsigned int reverb_a_len;
    unsigned int reverb_b_len;
    unsigned int reverb_c_len;
} sp89_state;

void sp89_config_default(sp89_config *cfg, unsigned int sample_rate);
void sp89_config_preset(sp89_config *cfg, unsigned int sample_rate,
                        sp89_preset preset);

void sp89_init(sp89_state *state, const sp89_config *cfg);
void sp89_reset(sp89_state *state);
void sp89_set_config(sp89_state *state, const sp89_config *cfg);

void sp89_trigger_pull(sp89_state *state);
void sp89_trigger_pump(sp89_state *state);
void sp89_trigger_cycle(sp89_state *state);

sp89_sample sp89_process(sp89_state *state);
void sp89_process_buffer(sp89_state *state, sp89_sample *output,
                         unsigned int count);

int sp89_is_active(const sp89_state *state);
unsigned int sp89_state_bytes(void);

#ifdef __cplusplus
}
#endif

#endif
