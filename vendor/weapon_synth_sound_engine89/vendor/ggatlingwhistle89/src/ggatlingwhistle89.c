#include "ggatlingwhistle89.h"

#define GGW89_Q15_ONE 32767L
#define GGW89_PHASE_MASK 65535UL

static const short ggw89_sine_lut[256] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602,
    6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
    18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
    27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
    32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
    32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285,
    32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
    30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683,
    27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
    23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868,
    18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
    12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179,
    6393, 5602, 4808, 4011, 3212, 2410, 1608, 804,
    0, -804, -1608, -2410, -3212, -4011, -4808, -5602,
    -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
    -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530,
    -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
    -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790,
    -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
    -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971,
    -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
    -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285,
    -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
    -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683,
    -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
    -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868,
    -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    -12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179,
    -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804
};

static long ggw89_clamp_long(long value, long low, long high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static unsigned long ggw89_ms_to_frames(unsigned long sample_rate,
                                         unsigned long ms)
{
    unsigned long whole;
    unsigned long remain;

    whole = sample_rate / 1000UL;
    remain = sample_rate % 1000UL;
    return (whole * ms) + ((remain * ms) / 1000UL);
}

static long ggw89_alpha_from_cutoff(unsigned long sample_rate, long cutoff_hz)
{
    unsigned long denominator;
    unsigned long numerator;
    unsigned long fs_over_six;

    if (cutoff_hz < 1L) {
        cutoff_hz = 1L;
    }
    if ((unsigned long)cutoff_hz > (sample_rate / 3UL)) {
        cutoff_hz = (long)(sample_rate / 3UL);
    }

    /*
     * Integer one-pole approximation:
     * alpha ~= fc / (fc + fs / 2*pi), with 2*pi approximated as 6.
     */
    fs_over_six = sample_rate / 6UL;
    denominator = (unsigned long)cutoff_hz + fs_over_six;
    numerator = ((unsigned long)cutoff_hz) << 15;

    if (denominator == 0UL) {
        return GGW89_Q15_ONE;
    }
    return (long)(numerator / denominator);
}

static short ggw89_sine(unsigned long phase)
{
    unsigned int index;

    index = (unsigned int)((phase >> 8) & 255UL);
    return ggw89_sine_lut[index];
}

static long ggw89_saw_q15(unsigned long phase)
{
    return (long)(phase & GGW89_PHASE_MASK) - 32768L;
}

static long ggw89_one_pole(long input, long *memory, long alpha_q15)
{
    *memory += ((input - *memory) * alpha_q15) >> 15;
    return *memory;
}

static long ggw89_noise_q15(GGW89_State *state)
{
    unsigned long bit;
    unsigned long value;

    value = state->lfsr & 65535UL;
    bit = ((value >> 0) ^ (value >> 2) ^
           (value >> 3) ^ (value >> 5)) & 1UL;
    value = (value >> 1) | (bit << 15);
    if (value == 0UL) {
        value = 0xACE1UL;
    }
    state->lfsr = value;

    return (long)(value & 65535UL) - 32768L;
}

static long ggw89_curve_q16(unsigned long progress_q16, int curve)
{
    unsigned long t;
    unsigned long t2;
    unsigned long inv;
    unsigned long inv2;

    t = progress_q16;
    if (t > 65535UL) {
        t = 65535UL;
    }

    if (curve == GGW89_CURVE_QUADRATIC) {
        t2 = (t * t) >> 16;
        return (long)t2;
    }
    if (curve == GGW89_CURVE_LATE) {
        inv = 65535UL - t;
        inv2 = (inv * inv) >> 16;
        return (long)(65535UL - inv2);
    }
    return (long)t;
}

static long ggw89_pitch_hz(GGW89_State *state)
{
    long curve_q16;
    long span;
    long pitch;
    long vibrato;
    long ratio_depth;
    long primary;
    long harmonic;
    long composite;
    unsigned long progress_q16;
    long inc_span;
    long current_inc;

    progress_q16 = state->sweep_acc_q16 >> 16;
    if (progress_q16 > 65535UL) {
        progress_q16 = 65535UL;
    }

    curve_q16 = ggw89_curve_q16(progress_q16,
                                state->cfg.sweep_curve);
    span = state->cfg.end_hz - state->cfg.start_hz;
    pitch = state->cfg.start_hz + ((span * curve_q16) / 65535L);

    /*
     * Restrained rotation-synchronous pitch drift.  A smooth sine is used
     * instead of the older steep triangle wobble; a very small second
     * harmonic only prevents sterile regularity.  Most rotary motion is
     * expressed as amplitude texture in the hiss/FM layers, not as a siren.
     */
    primary = (long)ggw89_sine(state->lfo_phase);
    harmonic = (long)ggw89_sine((state->lfo_phase << 1) &
                                GGW89_PHASE_MASK);
    harmonic = (harmonic * state->cfg.vibrato_harmonic_q15) >> 15;
    composite = ggw89_clamp_long(primary + harmonic,
                                 -32768L, 32767L);

    ratio_depth = (pitch * state->cfg.vibrato_depth_ratio_q15) >> 15;
    vibrato = (composite *
               (state->cfg.vibrato_depth_hz + ratio_depth)) >> 15;
    pitch += vibrato;

    inc_span = (long)state->lfo_end_inc -
               (long)state->lfo_start_inc;
    current_inc = (long)state->lfo_start_inc +
        ((inc_span * (long)progress_q16) / 65535L);
    if (current_inc < 0L) {
        current_inc = 0L;
    }
    state->lfo_inc = (unsigned long)current_inc;

    if (pitch < 20L) {
        pitch = 20L;
    }
    if ((unsigned long)pitch > (state->sample_rate / 3UL)) {
        pitch = (long)(state->sample_rate / 3UL);
    }
    return pitch;
}

static void ggw89_update_envelopes(GGW89_State *state)
{
    if (state->looping) {
        if (state->age_frames < state->tone_attack_frames) {
            state->tone_env_q15 += state->tone_attack_step_q15;
            if (state->tone_env_q15 > GGW89_Q15_ONE) {
                state->tone_env_q15 = GGW89_Q15_ONE;
            }
        } else {
            state->tone_env_q15 = GGW89_Q15_ONE;
        }

        if (state->age_frames < state->noise_attack_frames) {
            state->noise_env_q15 += state->noise_attack_step_q15;
            if (state->noise_env_q15 > GGW89_Q15_ONE) {
                state->noise_env_q15 = GGW89_Q15_ONE;
            }
        } else {
            state->noise_env_q15 = GGW89_Q15_ONE;
        }
        return;
    }

    if (state->age_frames < state->release_start_frame) {
        if (state->age_frames < state->tone_attack_frames) {
            state->tone_env_q15 += state->tone_attack_step_q15;
            if (state->tone_env_q15 > GGW89_Q15_ONE) {
                state->tone_env_q15 = GGW89_Q15_ONE;
            }
        } else {
            state->tone_env_q15 = GGW89_Q15_ONE;
        }

        if (state->age_frames < state->noise_attack_frames) {
            state->noise_env_q15 += state->noise_attack_step_q15;
            if (state->noise_env_q15 > GGW89_Q15_ONE) {
                state->noise_env_q15 = GGW89_Q15_ONE;
            }
        } else {
            state->noise_env_q15 = GGW89_Q15_ONE;
        }
    } else {
        state->tone_env_q15 -= state->release_step_q15;
        state->noise_env_q15 -= state->release_step_q15;
        if (state->tone_env_q15 < 0L) {
            state->tone_env_q15 = 0L;
        }
        if (state->noise_env_q15 < 0L) {
            state->noise_env_q15 = 0L;
        }
    }
}

static long ggw89_mid_air(GGW89_State *state, long noise)
{
    long high_pass;
    long mid;

    state->noise_lp_low +=
        ((noise - state->noise_lp_low) *
         state->noise_low_alpha_q15) >> 15;

    high_pass = noise - state->noise_lp_low;

    state->noise_lp_high +=
        ((high_pass - state->noise_lp_high) *
         state->noise_high_alpha_q15) >> 15;

    mid = state->noise_lp_high;
    return ggw89_clamp_long(mid, -32768L, 32767L);
}

static long ggw89_soft_drive(long sample, int drive_q15)
{
    long driven;
    long sign;
    long magnitude;
    long knee;
    long excess;

    if (drive_q15 < 0) {
        drive_q15 = 0;
    }
    driven = (sample * (long)drive_q15) >> 15;

    if (driven < 0L) {
        sign = -1L;
        magnitude = -driven;
    } else {
        sign = 1L;
        magnitude = driven;
    }

    knee = 24576L;
    if (magnitude > knee) {
        excess = magnitude - knee;
        magnitude = knee + (excess >> 2);
    }
    if (magnitude > 32767L) {
        magnitude = 32767L;
    }

    return magnitude * sign;
}

void ggw89_init(GGW89_State *state, unsigned long sample_rate)
{
    if (state == 0) {
        return;
    }
    if (sample_rate < 8000UL) {
        sample_rate = 8000UL;
    }

    state->sample_rate = sample_rate;
    state->lfsr = 0xACE1UL;
    ggw89_reset(state);
}

void ggw89_reset(GGW89_State *state)
{
    unsigned long sample_rate;
    unsigned long lfsr;

    if (state == 0) {
        return;
    }

    sample_rate = state->sample_rate;
    lfsr = state->lfsr;
    if (lfsr == 0UL) {
        lfsr = 0xACE1UL;
    }

    state->age_frames = 0UL;
    state->total_frames = 0UL;
    state->release_start_frame = 0UL;
    state->tone_attack_frames = 0UL;
    state->noise_attack_frames = 0UL;
    state->sweep_acc_q16 = 0UL;
    state->sweep_step_q16 = 0UL;
    state->sine_phase = 0UL;
    state->saw_phase = 0UL;
    state->body_phase = 0UL;
    state->upper_phase = 0UL;
    state->fm_carrier_phase = 0UL;
    state->fm_mod_phase = 0UL;
    state->lfo_phase = 0UL;
    state->lfo_inc = 0UL;
    state->lfo_start_inc = 0UL;
    state->lfo_end_inc = 0UL;
    state->tone_env_q15 = 0L;
    state->noise_env_q15 = 0L;
    state->tone_attack_step_q15 = 0L;
    state->noise_attack_step_q15 = 0L;
    state->release_step_q15 = 0L;
    state->noise_lp_low = 0L;
    state->noise_lp_high = 0L;
    state->saw_lp = 0L;
    state->fm_lp = 0L;
    state->eq_lp1 = 0L;
    state->eq_lp2 = 0L;
    state->eq_lp3 = 0L;
    state->eq_lp4 = 0L;
    state->eq_lp5 = 0L;
    state->output_lp = 0L;
    state->noise_low_alpha_q15 = 0L;
    state->noise_high_alpha_q15 = 0L;
    state->saw_alpha_q15 = 0L;
    state->fm_alpha_q15 = 0L;
    state->eq_alpha1_q15 = 0L;
    state->eq_alpha2_q15 = 0L;
    state->eq_alpha3_q15 = 0L;
    state->eq_alpha4_q15 = 0L;
    state->eq_alpha5_q15 = 0L;
    state->output_alpha_q15 = 0L;
    state->active = 0;
    state->event_id = GGW89_EVENT_NONE;
    state->looping = 0;
    state->sample_rate = sample_rate;
    state->lfsr = lfsr;
}

void ggw89_get_default_config(int event_id, GGW89_Config *config)
{
    if (config == 0) {
        return;
    }

    if (event_id == GGW89_EVENT_FIRE_LOOP) {
        /*
         * A6 is the sustained blade/air whistle.  The 0.5x body oscillator
         * lands on A5, while the upper and FM layers remain quiet texture.
         * start_hz and end_hz are identical so the loop has no pitch sweep.
         */
        config->duration_ms = 1000UL;
        config->tone_attack_ms = 18UL;
        config->noise_attack_ms = 95UL;
        config->release_ms = 1UL;
        config->start_hz = 1760L;
        config->end_hz = 1760L;

        config->tone_level_q15 = 18400L;
        config->saw_level_q15 = 430L;
        config->body_level_q15 = 2450L;
        config->upper_level_q15 = 700L;
        config->fm_level_q15 = 1380L;
        config->noise_level_q15 = 10600L;
        config->master_q15 = 21800L;

        config->saw_pitch_ratio_q15 = 32767L;
        config->body_pitch_ratio_q15 = 16384L;
        config->upper_pitch_ratio_q15 = 131071L;
        config->fm_carrier_ratio_q15 = 73728L;
        config->fm_mod_ratio_q15 = 45056L;
        config->fm_index_phase_q15 = 920L;
        config->fm_low_pass_hz = 4100L;
        config->saw_low_pass_hz = 760L;

        config->noise_low_cut_hz = 650L;
        config->noise_high_cut_hz = 5100L;

        config->eq_split1_hz = 180L;
        config->eq_split2_hz = 430L;
        config->eq_split3_hz = 880L;
        config->eq_split4_hz = 1820L;
        config->eq_split5_hz = 3600L;
        config->eq_band1_gain_q15 = 22000L;
        config->eq_band2_gain_q15 = 33000L;
        config->eq_band3_gain_q15 = 40500L;
        config->eq_band4_gain_q15 = 45500L;
        config->eq_band5_gain_q15 = 37000L;
        config->eq_band6_gain_q15 = 6200L;
        config->output_low_pass_hz = 5000L;

        /* About 13 Hz group rotation -> about 78 Hz six-barrel passage. */
        config->barrel_pass_depth_q15 = 2900L;

        config->vibrato_millihz = 13000L;
        config->vibrato_end_millihz = 13000L;
        config->vibrato_depth_hz = 1L;
        config->vibrato_depth_ratio_q15 = 28L;
        config->vibrato_harmonic_q15 = 700L;
        config->sweep_curve = GGW89_CURVE_LINEAR;
        config->drive_q15 = 41000;
        config->loop_mode = 1;
    } else if (event_id == GGW89_EVENT_SPIN_DOWN) {
        config->duration_ms = 670UL;
        config->tone_attack_ms = 14UL;
        config->noise_attack_ms = 105UL;
        config->release_ms = 155UL;
        config->start_hz = 1760L;
        config->end_hz = 350L;

        config->tone_level_q15 = 17600L;
        config->saw_level_q15 = 520L;
        config->body_level_q15 = 2700L;
        config->upper_level_q15 = 760L;
        config->fm_level_q15 = 1450L;
        config->noise_level_q15 = 9800L;
        config->master_q15 = 22400L;

        config->saw_pitch_ratio_q15 = 32767L;
        config->body_pitch_ratio_q15 = 16384L;
        config->upper_pitch_ratio_q15 = 131071L;
        config->fm_carrier_ratio_q15 = 73728L;
        config->fm_mod_ratio_q15 = 45056L;
        config->fm_index_phase_q15 = 980L;
        config->fm_low_pass_hz = 3900L;
        config->saw_low_pass_hz = 720L;

        config->noise_low_cut_hz = 560L;
        config->noise_high_cut_hz = 4800L;

        config->eq_split1_hz = 170L;
        config->eq_split2_hz = 400L;
        config->eq_split3_hz = 820L;
        config->eq_split4_hz = 1650L;
        config->eq_split5_hz = 3400L;
        config->eq_band1_gain_q15 = 23500L;
        config->eq_band2_gain_q15 = 34500L;
        config->eq_band3_gain_q15 = 44000L;
        config->eq_band4_gain_q15 = 42000L;
        config->eq_band5_gain_q15 = 35000L;
        config->eq_band6_gain_q15 = 6500L;
        config->output_low_pass_hz = 4800L;

        config->barrel_pass_depth_q15 = 3100L;

        config->vibrato_millihz = 16000L;
        config->vibrato_end_millihz = 6200L;
        config->vibrato_depth_hz = 3L;
        config->vibrato_depth_ratio_q15 = 150L;
        config->vibrato_harmonic_q15 = 900L;
        config->sweep_curve = GGW89_CURVE_QUADRATIC;
        config->drive_q15 = 41000;
        config->loop_mode = 0;
    } else {
        config->duration_ms = 610UL;
        config->tone_attack_ms = 24UL;
        config->noise_attack_ms = 135UL;
        config->release_ms = 95UL;
        config->start_hz = 300L;
        config->end_hz = 1760L;

        config->tone_level_q15 = 18000L;
        config->saw_level_q15 = 560L;
        config->body_level_q15 = 2850L;
        config->upper_level_q15 = 820L;
        config->fm_level_q15 = 1550L;
        config->noise_level_q15 = 10200L;
        config->master_q15 = 22400L;

        config->saw_pitch_ratio_q15 = 32767L;
        config->body_pitch_ratio_q15 = 16384L;
        config->upper_pitch_ratio_q15 = 131071L;
        config->fm_carrier_ratio_q15 = 73728L;
        config->fm_mod_ratio_q15 = 45056L;
        config->fm_index_phase_q15 = 1020L;
        config->fm_low_pass_hz = 4000L;
        config->saw_low_pass_hz = 750L;

        config->noise_low_cut_hz = 600L;
        config->noise_high_cut_hz = 4900L;

        config->eq_split1_hz = 180L;
        config->eq_split2_hz = 420L;
        config->eq_split3_hz = 850L;
        config->eq_split4_hz = 1700L;
        config->eq_split5_hz = 3500L;
        config->eq_band1_gain_q15 = 24000L;
        config->eq_band2_gain_q15 = 35000L;
        config->eq_band3_gain_q15 = 44500L;
        config->eq_band4_gain_q15 = 42500L;
        config->eq_band5_gain_q15 = 35500L;
        config->eq_band6_gain_q15 = 6800L;
        config->output_low_pass_hz = 4900L;

        config->barrel_pass_depth_q15 = 3300L;

        config->vibrato_millihz = 6800L;
        config->vibrato_end_millihz = 15500L;
        config->vibrato_depth_hz = 3L;
        config->vibrato_depth_ratio_q15 = 160L;
        config->vibrato_harmonic_q15 = 950L;
        config->sweep_curve = GGW89_CURVE_QUADRATIC;
        config->drive_q15 = 41500;
        config->loop_mode = 0;
    }
}

void ggw89_trigger(GGW89_State *state, int event_id)
{
    GGW89_Config config;

    ggw89_get_default_config(event_id, &config);
    ggw89_trigger_custom(state, event_id, &config);
}

void ggw89_trigger_continuous(GGW89_State *state, int event_id)
{
    GGW89_State previous;
    GGW89_Config config;

    if (state == 0) {
        return;
    }
    if (!state->active) {
        ggw89_trigger(state, event_id);
        return;
    }

    previous = *state;
    ggw89_get_default_config(event_id, &config);
    ggw89_trigger_custom(state, event_id, &config);

    /*
     * Preserve phase and filter history across pitch-matched event changes.
     * This is primarily intended for FIRE_LOOP -> SPIN_DOWN at A6.
     */
    state->sine_phase = previous.sine_phase;
    state->saw_phase = previous.saw_phase;
    state->body_phase = previous.body_phase;
    state->upper_phase = previous.upper_phase;
    state->fm_carrier_phase = previous.fm_carrier_phase;
    state->fm_mod_phase = previous.fm_mod_phase;
    state->lfo_phase = previous.lfo_phase;
    state->lfsr = previous.lfsr;

    state->tone_env_q15 = previous.tone_env_q15;
    state->noise_env_q15 = previous.noise_env_q15;
    state->noise_lp_low = previous.noise_lp_low;
    state->noise_lp_high = previous.noise_lp_high;
    state->saw_lp = previous.saw_lp;
    state->fm_lp = previous.fm_lp;
    state->eq_lp1 = previous.eq_lp1;
    state->eq_lp2 = previous.eq_lp2;
    state->eq_lp3 = previous.eq_lp3;
    state->eq_lp4 = previous.eq_lp4;
    state->eq_lp5 = previous.eq_lp5;
    state->output_lp = previous.output_lp;
}

void ggw89_trigger_custom(GGW89_State *state, int event_id,
                          const GGW89_Config *config)
{
    unsigned long release_frames;
    unsigned long lfo_numerator;
    unsigned long lfo_end_numerator;
    unsigned long denominator;

    if (state == 0 || config == 0) {
        return;
    }

    ggw89_reset(state);
    state->cfg = *config;
    state->event_id = event_id;
    state->looping = config->loop_mode ? 1 : 0;

    state->total_frames =
        ggw89_ms_to_frames(state->sample_rate, config->duration_ms);
    if (state->total_frames < 8UL) {
        state->total_frames = 8UL;
    }

    state->tone_attack_frames =
        ggw89_ms_to_frames(state->sample_rate, config->tone_attack_ms);
    state->noise_attack_frames =
        ggw89_ms_to_frames(state->sample_rate, config->noise_attack_ms);
    release_frames =
        ggw89_ms_to_frames(state->sample_rate, config->release_ms);

    if (state->tone_attack_frames < 1UL) {
        state->tone_attack_frames = 1UL;
    }
    if (state->noise_attack_frames < 1UL) {
        state->noise_attack_frames = 1UL;
    }
    if (release_frames < 1UL) {
        release_frames = 1UL;
    }
    if (release_frames >= state->total_frames) {
        release_frames = state->total_frames / 2UL;
    }

    state->release_start_frame = state->total_frames - release_frames;
    state->tone_attack_step_q15 =
        (long)((GGW89_Q15_ONE +
                (long)state->tone_attack_frames - 1L) /
               (long)state->tone_attack_frames);
    state->noise_attack_step_q15 =
        (long)((GGW89_Q15_ONE +
                (long)state->noise_attack_frames - 1L) /
               (long)state->noise_attack_frames);
    state->release_step_q15 =
        (long)((GGW89_Q15_ONE + (long)release_frames - 1L) /
               (long)release_frames);

    if (state->tone_attack_step_q15 < 1L) {
        state->tone_attack_step_q15 = 1L;
    }
    if (state->noise_attack_step_q15 < 1L) {
        state->noise_attack_step_q15 = 1L;
    }
    if (state->release_step_q15 < 1L) {
        state->release_step_q15 = 1L;
    }

    if (state->looping) {
        state->sweep_step_q16 = 0UL;
    } else {
        state->sweep_step_q16 =
            (65535UL << 16) / state->total_frames;
    }

    denominator = state->sample_rate * 1000UL;
    lfo_numerator = (unsigned long)config->vibrato_millihz * 65536UL;
    lfo_end_numerator =
        (unsigned long)config->vibrato_end_millihz * 65536UL;
    if (denominator == 0UL) {
        state->lfo_start_inc = 0UL;
        state->lfo_end_inc = 0UL;
    } else {
        state->lfo_start_inc = lfo_numerator / denominator;
        state->lfo_end_inc = lfo_end_numerator / denominator;
    }
    if (state->lfo_start_inc == 0UL &&
        config->vibrato_millihz > 0L) {
        state->lfo_start_inc = 1UL;
    }
    if (state->lfo_end_inc == 0UL &&
        config->vibrato_end_millihz > 0L) {
        state->lfo_end_inc = 1UL;
    }
    state->lfo_inc = state->lfo_start_inc;

    /* Deliberately offset harmonic phases to avoid a toy-organ stack. */
    state->saw_phase = 8192UL;
    state->body_phase = 16384UL;
    state->upper_phase = 32768UL;
    state->fm_carrier_phase = 12288UL;
    state->fm_mod_phase = 28672UL;

    state->noise_low_alpha_q15 =
        ggw89_alpha_from_cutoff(state->sample_rate,
                                config->noise_low_cut_hz);
    state->noise_high_alpha_q15 =
        ggw89_alpha_from_cutoff(state->sample_rate,
                                config->noise_high_cut_hz);
    state->saw_alpha_q15 =
        ggw89_alpha_from_cutoff(state->sample_rate,
                                config->saw_low_pass_hz);
    state->fm_alpha_q15 =
        ggw89_alpha_from_cutoff(state->sample_rate,
                                config->fm_low_pass_hz);
    state->eq_alpha1_q15 =
        ggw89_alpha_from_cutoff(state->sample_rate,
                                config->eq_split1_hz);
    state->eq_alpha2_q15 =
        ggw89_alpha_from_cutoff(state->sample_rate,
                                config->eq_split2_hz);
    state->eq_alpha3_q15 =
        ggw89_alpha_from_cutoff(state->sample_rate,
                                config->eq_split3_hz);
    state->eq_alpha4_q15 =
        ggw89_alpha_from_cutoff(state->sample_rate,
                                config->eq_split4_hz);
    state->eq_alpha5_q15 =
        ggw89_alpha_from_cutoff(state->sample_rate,
                                config->eq_split5_hz);
    state->output_alpha_q15 =
        ggw89_alpha_from_cutoff(state->sample_rate,
                                config->output_low_pass_hz);

    state->active = 1;
}

short ggw89_process(GGW89_State *state)
{
    long pitch_hz;
    unsigned long phase_inc;
    unsigned long saw_phase_inc;
    unsigned long body_phase_inc;
    unsigned long upper_phase_inc;
    unsigned long fm_carrier_phase_inc;
    unsigned long fm_mod_phase_inc;
    unsigned long saw_ratio_q15;
    unsigned long body_ratio_q15;
    unsigned long upper_ratio_q15;
    unsigned long fm_carrier_ratio_q15;
    unsigned long fm_mod_ratio_q15;
    long tone;
    long saw;
    long body;
    long upper;
    long fm_mod;
    long fm_phase_offset;
    long fm_metal;
    long noise;
    long air;
    long tone_part;
    long saw_part;
    long body_part;
    long upper_part;
    long fm_part;
    long noise_part;
    long pre_eq;
    long lp1;
    long lp2;
    long lp3;
    long lp4;
    long lp5;
    long band1;
    long band2;
    long band3;
    long band4;
    long band5;
    long band6;
    long part1;
    long part2;
    long part3;
    long part4;
    long part5;
    long part6;
    long barrel_phase;
    long barrel_wave;
    long barrel_gate_q15;
    long eq_mixed;
    long mixed;

    if (state == 0 || !state->active) {
        return (short)0;
    }

    ggw89_update_envelopes(state);
    pitch_hz = ggw89_pitch_hz(state);

    phase_inc = ((unsigned long)pitch_hz * 65536UL) /
                state->sample_rate;
    saw_ratio_q15 = (unsigned long)ggw89_clamp_long(
        state->cfg.saw_pitch_ratio_q15, 0L, 65535L);
    body_ratio_q15 = (unsigned long)ggw89_clamp_long(
        state->cfg.body_pitch_ratio_q15, 0L, 65535L);
    upper_ratio_q15 = (unsigned long)ggw89_clamp_long(
        state->cfg.upper_pitch_ratio_q15, 0L, 131071L);
    fm_carrier_ratio_q15 = (unsigned long)ggw89_clamp_long(
        state->cfg.fm_carrier_ratio_q15, 0L, 131071L);
    fm_mod_ratio_q15 = (unsigned long)ggw89_clamp_long(
        state->cfg.fm_mod_ratio_q15, 0L, 131071L);
    saw_phase_inc = (phase_inc * saw_ratio_q15) >> 15;
    body_phase_inc = (phase_inc * body_ratio_q15) >> 15;
    upper_phase_inc = (phase_inc * upper_ratio_q15) >> 15;
    fm_carrier_phase_inc =
        (phase_inc * fm_carrier_ratio_q15) >> 15;
    fm_mod_phase_inc = (phase_inc * fm_mod_ratio_q15) >> 15;

    state->sine_phase =
        (state->sine_phase + phase_inc) & GGW89_PHASE_MASK;
    state->saw_phase =
        (state->saw_phase + saw_phase_inc) & GGW89_PHASE_MASK;
    state->body_phase =
        (state->body_phase + body_phase_inc) & GGW89_PHASE_MASK;
    state->upper_phase =
        (state->upper_phase + upper_phase_inc) & GGW89_PHASE_MASK;
    state->fm_carrier_phase =
        (state->fm_carrier_phase + fm_carrier_phase_inc) &
        GGW89_PHASE_MASK;
    state->fm_mod_phase =
        (state->fm_mod_phase + fm_mod_phase_inc) & GGW89_PHASE_MASK;
    state->lfo_phase =
        (state->lfo_phase + state->lfo_inc) & GGW89_PHASE_MASK;

    tone = (long)ggw89_sine(state->sine_phase);
    saw = ggw89_saw_q15(state->saw_phase);
    saw = ggw89_one_pole(saw, &state->saw_lp, state->saw_alpha_q15);
    body = (long)ggw89_sine(state->body_phase);
    upper = (long)ggw89_sine(state->upper_phase);
    fm_mod = (long)ggw89_sine(state->fm_mod_phase);
    fm_phase_offset =
        (fm_mod * state->cfg.fm_index_phase_q15) >> 15;
    fm_metal = (long)ggw89_sine(
        (state->fm_carrier_phase + (unsigned long)fm_phase_offset) &
        GGW89_PHASE_MASK);
    fm_metal = ggw89_one_pole(fm_metal, &state->fm_lp,
                              state->fm_alpha_q15);
    noise = ggw89_noise_q15(state);
    air = ggw89_mid_air(state, noise);

    /*
     * Six-barrel passage modulation.  Six cycles per barrel-group turn
     * create a restrained 33-100 Hz mechanical flutter across the
     * documented 5.6-16.7 Hz rotation region.  It modulates the hiss and
     * high harmonic more than the fundamental, suggesting slots, feed,
     * barrel passage and structure-borne vibration instead of a clean synth.
     * The dominant central sine is deliberately excluded from this gate.
     */
    barrel_phase = (long)((state->lfo_phase * 6UL) &
                          GGW89_PHASE_MASK);
    barrel_wave = (long)ggw89_sine((unsigned long)barrel_phase);
    barrel_gate_q15 = GGW89_Q15_ONE -
        (state->cfg.barrel_pass_depth_q15 >> 1);
    barrel_gate_q15 +=
        (((barrel_wave + 32768L) >> 1) *
         state->cfg.barrel_pass_depth_q15) >> 15;
    barrel_gate_q15 = ggw89_clamp_long(barrel_gate_q15,
                                       0L, GGW89_Q15_ONE);

    tone_part = (tone * state->tone_env_q15) >> 15;
    tone_part = (tone_part * state->cfg.tone_level_q15) >> 15;

    saw_part = (saw * state->tone_env_q15) >> 15;
    saw_part = (saw_part * state->cfg.saw_level_q15) >> 15;
    saw_part = (saw_part *
                (28672L + (barrel_gate_q15 >> 3))) >> 15;

    body_part = (body * state->tone_env_q15) >> 15;
    body_part = (body_part * state->cfg.body_level_q15) >> 15;

    upper_part = (upper * state->tone_env_q15) >> 15;
    upper_part = (upper_part * state->cfg.upper_level_q15) >> 15;
    upper_part = (upper_part *
                  (28672L + (barrel_gate_q15 >> 3))) >> 15;

    /*
     * Narrow-index, inharmonic FM/phase-modulated metal skin.  It is mixed
     * well behind the central sine and filtered before the six-band EQ, so
     * it contributes rotor/airframe shimmer rather than an exposed DX-style
     * bell or science-fiction weapon tone.
     */
    fm_part = (fm_metal * state->tone_env_q15) >> 15;
    fm_part = (fm_part * state->cfg.fm_level_q15) >> 15;
    fm_part = (fm_part *
               (28672L + (barrel_gate_q15 >> 3))) >> 15;

    noise_part = (air * state->noise_env_q15) >> 15;
    noise_part = (noise_part * state->cfg.noise_level_q15) >> 15;
    noise_part = (noise_part * barrel_gate_q15) >> 15;

    pre_eq = tone_part + saw_part + body_part + upper_part +
             fm_part + noise_part;
    pre_eq = ggw89_clamp_long(pre_eq, -32768L, 32767L);

    lp1 = ggw89_one_pole(pre_eq, &state->eq_lp1,
                          state->eq_alpha1_q15);
    lp2 = ggw89_one_pole(pre_eq, &state->eq_lp2,
                          state->eq_alpha2_q15);
    lp3 = ggw89_one_pole(pre_eq, &state->eq_lp3,
                          state->eq_alpha3_q15);
    lp4 = ggw89_one_pole(pre_eq, &state->eq_lp4,
                          state->eq_alpha4_q15);
    lp5 = ggw89_one_pole(pre_eq, &state->eq_lp5,
                          state->eq_alpha5_q15);

    band1 = lp1;
    band2 = lp2 - lp1;
    band3 = lp3 - lp2;
    band4 = lp4 - lp3;
    band5 = lp5 - lp4;
    band6 = pre_eq - lp5;

    band1 = ggw89_clamp_long(band1, -32768L, 32767L);
    band2 = ggw89_clamp_long(band2, -32768L, 32767L);
    band3 = ggw89_clamp_long(band3, -32768L, 32767L);
    band4 = ggw89_clamp_long(band4, -32768L, 32767L);
    band5 = ggw89_clamp_long(band5, -32768L, 32767L);
    band6 = ggw89_clamp_long(band6, -32768L, 32767L);

    part1 = (band1 * state->cfg.eq_band1_gain_q15) >> 15;
    part2 = (band2 * state->cfg.eq_band2_gain_q15) >> 15;
    part3 = (band3 * state->cfg.eq_band3_gain_q15) >> 15;
    part4 = (band4 * state->cfg.eq_band4_gain_q15) >> 15;
    part5 = (band5 * state->cfg.eq_band5_gain_q15) >> 15;
    part6 = (band6 * state->cfg.eq_band6_gain_q15) >> 15;
    eq_mixed = part1 + part2 + part3 + part4 + part5 + part6;
    eq_mixed = ggw89_clamp_long(eq_mixed, -32768L, 32767L);

    mixed = ggw89_one_pole(eq_mixed, &state->output_lp,
                           state->output_alpha_q15);
    mixed = ggw89_soft_drive(mixed, state->cfg.drive_q15);
    mixed = (mixed * state->cfg.master_q15) >> 15;
    mixed = ggw89_clamp_long(mixed, -32768L, 32767L);

    if (state->looping) {
        if (state->age_frames < state->total_frames) {
            state->age_frames++;
        }
    } else {
        state->age_frames++;
        state->sweep_acc_q16 += state->sweep_step_q16;

        if (state->age_frames >= state->total_frames) {
            state->active = 0;
            state->event_id = GGW89_EVENT_NONE;
        }
    }

    return (short)mixed;
}

void ggw89_render(GGW89_State *state, short *output,
                  unsigned long frame_count)
{
    unsigned long i;

    if (output == 0) {
        return;
    }

    for (i = 0UL; i < frame_count; ++i) {
        output[i] = ggw89_process(state);
    }
}

int ggw89_is_active(const GGW89_State *state)
{
    if (state == 0) {
        return 0;
    }
    return state->active;
}
