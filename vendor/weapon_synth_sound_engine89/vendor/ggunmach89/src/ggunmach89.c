#include "ggunmach89.h"

#include <string.h>

#define GGM89_PHASE_MASK 65535UL
#define GGM89_FULL_SPEED_Q16 65536UL
#define GGM89_CONTROL_PERIOD 16
#define GGM89_MAX_SAMPLE_RATE 96000UL
#define GGM89_MIN_SAMPLE_RATE 8000UL

static const ggm89_s16 ggm89_sine_table[256] = {
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

static const ggm89_u16 ggm89_eq_split_hz[GGM89_EQ_SPLITS] = {
    120, 360, 1000, 2800, 7000
};

static ggm89_s32 ggm89_clip_s32(ggm89_s32 value,
                                ggm89_s32 low,
                                ggm89_s32 high)
{
    if (value < low) {
        return low;
    }
    if (value > high) {
        return high;
    }
    return value;
}

static ggm89_s16 ggm89_clip_s16(ggm89_s32 value)
{
    if (value < -32768L) {
        return (ggm89_s16)-32768;
    }
    if (value > 32767L) {
        return (ggm89_s16)32767;
    }
    return (ggm89_s16)value;
}

static ggm89_s32 ggm89_mul_q15(ggm89_s32 a, ggm89_s32 b)
{
    return (a * b) / 32768L;
}

static ggm89_s32 ggm89_mul_q12(ggm89_s32 a, ggm89_s32 b)
{
    return (a * b) / 4096L;
}

static ggm89_s16 ggm89_sine(ggm89_u32 phase)
{
    ggm89_u32 index;

    index = (phase >> 8) & 255UL;
    return ggm89_sine_table[index];
}

static ggm89_s16 ggm89_saw(ggm89_u32 phase)
{
    return (ggm89_s16)((ggm89_s32)(phase & GGM89_PHASE_MASK) - 32768L);
}

static ggm89_s16 ggm89_triangle(ggm89_u32 phase)
{
    ggm89_s32 p;
    ggm89_s32 value;

    p = (ggm89_s32)(phase & GGM89_PHASE_MASK);
    if (p < 32768L) {
        value = (p * 2L) - 32767L;
    } else {
        value = 98303L - (p * 2L);
    }
    return ggm89_clip_s16(value);
}

static ggm89_u32 ggm89_noise_next(ggm89_u32 *state)
{
    ggm89_u32 x;

    x = *state & 0xffffffffUL;
    if (x == 0UL) {
        x = 0x6d2b79f5UL;
    }
    x ^= (x << 13) & 0xffffffffUL;
    x ^= (x >> 17);
    x ^= (x << 5) & 0xffffffffUL;
    x &= 0xffffffffUL;
    *state = x;
    return x;
}

static ggm89_s16 ggm89_noise_s16(ggm89_u32 *state)
{
    ggm89_u32 x;

    x = ggm89_noise_next(state);
    return (ggm89_s16)((x >> 16) & 65535UL);
}

static ggm89_s32 ggm89_onepole(ggm89_s32 state,
                               ggm89_s32 input,
                               ggm89_s16 alpha_q15)
{
    ggm89_s32 delta;

    delta = input - state;
    return state + ggm89_mul_q15(delta, (ggm89_s32)alpha_q15);
}

static ggm89_u32 ggm89_inc_from_millihz(ggm89_u32 millihz,
                                        ggm89_u32 sample_rate)
{
    ggm89_u32 whole_hz;
    ggm89_u32 rem_millihz;
    ggm89_u32 increment;
    ggm89_u32 denominator;

    whole_hz = millihz / 1000UL;
    rem_millihz = millihz % 1000UL;
    increment = (whole_hz * 65536UL) / sample_rate;
    denominator = sample_rate * 1000UL;
    increment += (rem_millihz * 65536UL) / denominator;
    return increment;
}

static ggm89_s16 ggm89_alpha_for_hz(ggm89_u32 hz,
                                    ggm89_u32 sample_rate)
{
    ggm89_u32 numerator;
    ggm89_u32 denominator;
    ggm89_u32 alpha;

    /*
     * Integer approximation of:
     * alpha = (2*pi*fc) / (fs + 2*pi*fc)
     * represented as Q1.15.
     */
    numerator = hz * 205887UL;
    denominator = sample_rate + (hz * 6UL);
    if (denominator == 0UL) {
        return 1;
    }
    alpha = numerator / denominator;
    if (alpha < 1UL) {
        alpha = 1UL;
    }
    if (alpha > 32767UL) {
        alpha = 32767UL;
    }
    return (ggm89_s16)alpha;
}

static void ggm89_setup_ramp(ggm89_u32 sample_rate,
                             ggm89_u16 milliseconds,
                             ggm89_u32 *step,
                             ggm89_u32 *remainder,
                             ggm89_u32 *denominator)
{
    ggm89_u32 ms;
    ggm89_u32 den;
    ggm89_u32 numerator;

    ms = (ggm89_u32)milliseconds;
    if (ms < 1UL) {
        ms = 1UL;
    }
    if (ms > 15000UL) {
        ms = 15000UL;
    }
    den = ms * sample_rate;
    if (den == 0UL) {
        den = 1UL;
    }
    numerator = 65536000UL;
    *step = numerator / den;
    *remainder = numerator % den;
    *denominator = den;
}

static void ggm89_advance_speed(ggm89_state *state)
{
    ggm89_u32 amount;

    if (state->speed_q16 < state->target_speed_q16) {
        amount = state->ramp_up_step;
        state->ramp_up_error += state->ramp_up_rem;
        if (state->ramp_up_error >= state->ramp_up_den) {
            state->ramp_up_error -= state->ramp_up_den;
            amount += 1UL;
        }
        if (amount < 1UL && state->ramp_up_rem == 0UL) {
            amount = 1UL;
        }
        if (state->speed_q16 + amount >= state->target_speed_q16) {
            state->speed_q16 = state->target_speed_q16;
        } else {
            state->speed_q16 += amount;
        }
    } else if (state->speed_q16 > state->target_speed_q16) {
        amount = state->ramp_down_step;
        state->ramp_down_error += state->ramp_down_rem;
        if (state->ramp_down_error >= state->ramp_down_den) {
            state->ramp_down_error -= state->ramp_down_den;
            amount += 1UL;
        }
        if (amount < 1UL && state->ramp_down_rem == 0UL) {
            amount = 1UL;
        }
        if (state->speed_q16 <= state->target_speed_q16 + amount) {
            state->speed_q16 = state->target_speed_q16;
        } else {
            state->speed_q16 -= amount;
        }
    }

    if (state->speed_q16 == 0UL &&
        state->target_speed_q16 == 0UL) {
        state->mode = GGM89_STATE_OFF;
    } else if (state->speed_q16 < state->target_speed_q16) {
        state->mode = GGM89_STATE_SPINUP;
    } else if (state->speed_q16 > state->target_speed_q16) {
        state->mode = GGM89_STATE_SPINDOWN;
    } else {
        state->mode = GGM89_STATE_RUNNING;
    }
}

static void ggm89_update_control(ggm89_state *state)
{
    ggm89_u32 rpm;
    ggm89_u32 rotor_millihz;
    ggm89_u32 cam_millihz;
    ggm89_u32 sine_millihz;
    ggm89_u32 saw_millihz;
    ggm89_u32 vibrato_millihz;
    ggm89_s32 vibrato;
    ggm89_s32 load_dip;
    ggm89_u32 effective_rpm;

    rpm = ((ggm89_u32)state->config.nominal_spm *
           state->speed_q16) / GGM89_FULL_SPEED_Q16;

    load_dip = ggm89_mul_q15(state->firing_load_q15,
                             state->config.load_dip_q15);
    effective_rpm = rpm;
    if (load_dip > 0L) {
        effective_rpm = rpm -
            (ggm89_u32)ggm89_mul_q15((ggm89_s32)rpm, load_dip);
    }

    state->current_rpm = (ggm89_s32)effective_rpm;

    if (state->config.barrel_count == 0U) {
        rotor_millihz = 0UL;
    } else {
        rotor_millihz =
            (effective_rpm * 1000UL) /
            (60UL * (ggm89_u32)state->config.barrel_count);
    }
    cam_millihz = (effective_rpm * 1000UL) / 60UL;

    sine_millihz =
        rotor_millihz * (ggm89_u32)state->config.sine_harmonic;
    saw_millihz =
        rotor_millihz * (ggm89_u32)state->config.saw_harmonic;
    vibrato_millihz =
        rotor_millihz * (ggm89_u32)state->config.vibrato_harmonic;

    vibrato = ggm89_mul_q15(
        (ggm89_s32)ggm89_triangle(state->phase_vibrato),
        (ggm89_s32)state->config.vibrato_depth_q15);

    sine_millihz = (ggm89_u32)ggm89_clip_s32(
        (ggm89_s32)sine_millihz +
        ggm89_mul_q15((ggm89_s32)sine_millihz, vibrato),
        0L,
        2000000L);
    saw_millihz = (ggm89_u32)ggm89_clip_s32(
        (ggm89_s32)saw_millihz +
        ggm89_mul_q15((ggm89_s32)saw_millihz, vibrato),
        0L,
        4000000L);

    state->inc_rotor =
        ggm89_inc_from_millihz(rotor_millihz, state->sample_rate);
    state->inc_cam =
        ggm89_inc_from_millihz(cam_millihz, state->sample_rate);
    state->inc_sine =
        ggm89_inc_from_millihz(sine_millihz, state->sample_rate);
    state->inc_saw =
        ggm89_inc_from_millihz(saw_millihz, state->sample_rate);
    state->inc_vibrato =
        ggm89_inc_from_millihz(vibrato_millihz,
                               state->sample_rate);

    state->noise_alpha_q15 = (ggm89_s16)(
        1200L +
        (ggm89_s32)((state->speed_q16 * 11000UL) /
                    GGM89_FULL_SPEED_Q16));
}

static ggm89_s32 ggm89_apply_eq(ggm89_state *state,
                                ggm89_s32 input)
{
    ggm89_s32 bands[GGM89_EQ_BANDS];
    ggm89_s32 sum;
    int i;

    for (i = 0; i < GGM89_EQ_SPLITS; ++i) {
        state->eq_lp[i] = ggm89_onepole(
            state->eq_lp[i],
            input,
            state->eq_alpha_q15[i]);
    }

    bands[0] = state->eq_lp[0];
    for (i = 1; i < GGM89_EQ_SPLITS; ++i) {
        bands[i] = state->eq_lp[i] - state->eq_lp[i - 1];
    }
    bands[5] = input - state->eq_lp[4];

    sum = 0L;
    for (i = 0; i < GGM89_EQ_BANDS; ++i) {
        sum += ggm89_mul_q12(
            bands[i],
            state->config.eq_gain_q12[i]);
    }
    return ggm89_clip_s32(sum, -65536L, 65535L);
}

static ggm89_s32 ggm89_apply_distortion(ggm89_state *state,
                                        ggm89_s32 input)
{
    ggm89_s32 driven;
    ggm89_s32 threshold;
    ggm89_s32 excess;

    driven = ggm89_mul_q12(
        input,
        state->config.distortion_drive_q12);
    threshold = 18000L;

    if (driven > threshold) {
        excess = driven - threshold;
        driven = threshold + (excess / 5L);
    } else if (driven < -threshold) {
        excess = (-driven) - threshold;
        driven = -threshold - (excess / 5L);
    }

    return ggm89_clip_s32(driven, -32768L, 32767L);
}

static ggm89_s32 ggm89_reverb_tap(const ggm89_state *state,
                                  ggm89_u16 delay)
{
    ggm89_u16 index;

    index = (ggm89_u16)(
        (state->reverb_index + GGM89_REVERB_SAMPLES - delay) %
        GGM89_REVERB_SAMPLES);
    return (ggm89_s32)state->reverb_buffer[index];
}

static ggm89_u16 ggm89_scaled_delay(ggm89_u32 sample_rate,
                                    ggm89_u16 base_delay)
{
    ggm89_u32 delay;

    delay = ((ggm89_u32)base_delay * sample_rate) / 44100UL;
    if (delay < 1UL) {
        delay = 1UL;
    }
    if (delay >= GGM89_REVERB_SAMPLES) {
        delay = GGM89_REVERB_SAMPLES - 1UL;
    }
    return (ggm89_u16)delay;
}

static ggm89_s32 ggm89_apply_reverb(ggm89_state *state,
                                    ggm89_s32 input)
{
    ggm89_s32 wet;
    ggm89_s32 feedback_sample;
    ggm89_s32 output;
    ggm89_s16 d0;
    ggm89_s16 d1;
    ggm89_s16 d2;
    ggm89_s16 d3;

    d0 = ggm89_scaled_delay(state->sample_rate, 571U);
    d1 = ggm89_scaled_delay(state->sample_rate, 947U);
    d2 = ggm89_scaled_delay(state->sample_rate, 1531U);
    d3 = ggm89_scaled_delay(state->sample_rate, 2633U);

    wet = ggm89_reverb_tap(state, (ggm89_u16)d0);
    wet += ggm89_reverb_tap(state, (ggm89_u16)d1);
    wet += ggm89_reverb_tap(state, (ggm89_u16)d2);
    wet += ggm89_reverb_tap(state, (ggm89_u16)d3);
    wet /= 4L;

    feedback_sample = input +
        ggm89_mul_q15(wet, state->config.reverb_feedback_q15);
    state->reverb_buffer[state->reverb_index] =
        ggm89_clip_s16(feedback_sample);

    state->reverb_index = (ggm89_u16)(
        (state->reverb_index + 1U) % GGM89_REVERB_SAMPLES);

    output = input +
        ggm89_mul_q15(wet - input, state->config.reverb_mix_q15);
    return ggm89_clip_s32(output, -32768L, 32767L);
}

static void ggm89_set_common(ggm89_config *config)
{
    int i;

    memset(config, 0, sizeof(*config));
    config->nominal_spm = 2400U;
    config->barrel_count = 6U;
    config->spinup_ms = 650U;
    config->spindown_ms = 800U;
    config->sine_harmonic = 8U;
    config->saw_harmonic = 24U;
    config->vibrato_harmonic = 2U;
    config->sine_gain_q15 = 10500;
    config->saw_gain_q15 = 9500;
    config->noise_gain_q15 = 9000;
    config->cam_gain_q15 = 7200;
    config->tremolo_depth_q15 = 9000;
    config->vibrato_depth_q15 = 650;
    config->load_dip_q15 = 950;
    config->distortion_drive_q12 = 5500;
    config->reverb_mix_q15 = 2800;
    config->reverb_feedback_q15 = 6200;
    for (i = 0; i < GGM89_EQ_BANDS; ++i) {
        config->eq_gain_q12[i] = GGM89_Q12_ONE;
    }
}

void ggm89_config_preset(ggm89_config *config, int preset_id)
{
    if (config == 0) {
        return;
    }

    ggm89_set_common(config);

    switch (preset_id) {
    case GGM89_PRESET_M134D:
        config->nominal_spm = 3000U;
        config->barrel_count = 6U;
        config->spinup_ms = 480U;
        config->spindown_ms = 700U;
        config->sine_harmonic = 8U;
        config->saw_harmonic = 28U;
        config->sine_gain_q15 = 9500;
        config->saw_gain_q15 = 11000;
        config->noise_gain_q15 = 9200;
        config->cam_gain_q15 = 7600;
        config->tremolo_depth_q15 = 8500;
        config->vibrato_depth_q15 = 520;
        config->distortion_drive_q12 = 5900;
        config->eq_gain_q12[0] = 3000;
        config->eq_gain_q12[1] = 3900;
        config->eq_gain_q12[2] = 4700;
        config->eq_gain_q12[3] = 5000;
        config->eq_gain_q12[4] = 3600;
        config->eq_gain_q12[5] = 2200;
        break;

    case GGM89_PRESET_M197:
        config->nominal_spm = 1500U;
        config->barrel_count = 3U;
        config->spinup_ms = 620U;
        config->spindown_ms = 850U;
        config->sine_harmonic = 7U;
        config->saw_harmonic = 18U;
        config->sine_gain_q15 = 11000;
        config->saw_gain_q15 = 9200;
        config->noise_gain_q15 = 8200;
        config->cam_gain_q15 = 10500;
        config->tremolo_depth_q15 = 11500;
        config->vibrato_depth_q15 = 780;
        config->distortion_drive_q12 = 5700;
        config->eq_gain_q12[0] = 3500;
        config->eq_gain_q12[1] = 4400;
        config->eq_gain_q12[2] = 4900;
        config->eq_gain_q12[3] = 4300;
        config->eq_gain_q12[4] = 3000;
        config->eq_gain_q12[5] = 1800;
        break;

    case GGM89_PRESET_M61:
        config->nominal_spm = 6000U;
        config->barrel_count = 6U;
        config->spinup_ms = 350U;
        config->spindown_ms = 520U;
        config->sine_harmonic = 6U;
        config->saw_harmonic = 24U;
        config->sine_gain_q15 = 9000;
        config->saw_gain_q15 = 12000;
        config->noise_gain_q15 = 10000;
        config->cam_gain_q15 = 6800;
        config->tremolo_depth_q15 = 7200;
        config->vibrato_depth_q15 = 430;
        config->distortion_drive_q12 = 6200;
        config->eq_gain_q12[0] = 2600;
        config->eq_gain_q12[1] = 3500;
        config->eq_gain_q12[2] = 4500;
        config->eq_gain_q12[3] = 5200;
        config->eq_gain_q12[4] = 4100;
        config->eq_gain_q12[5] = 2600;
        break;

    case GGM89_PRESET_GAU8:
        config->nominal_spm = 3900U;
        config->barrel_count = 7U;
        config->spinup_ms = 850U;
        config->spindown_ms = 1050U;
        config->sine_harmonic = 7U;
        config->saw_harmonic = 18U;
        config->sine_gain_q15 = 13500;
        config->saw_gain_q15 = 7800;
        config->noise_gain_q15 = 10500;
        config->cam_gain_q15 = 9200;
        config->tremolo_depth_q15 = 9800;
        config->vibrato_depth_q15 = 850;
        config->load_dip_q15 = 1500;
        config->distortion_drive_q12 = 6500;
        config->reverb_mix_q15 = 3800;
        config->eq_gain_q12[0] = 5000;
        config->eq_gain_q12[1] = 5200;
        config->eq_gain_q12[2] = 4700;
        config->eq_gain_q12[3] = 3500;
        config->eq_gain_q12[4] = 2400;
        config->eq_gain_q12[5] = 1500;
        break;

    case GGM89_PRESET_GAU22:
        config->nominal_spm = 3300U;
        config->barrel_count = 4U;
        config->spinup_ms = 450U;
        config->spindown_ms = 650U;
        config->sine_harmonic = 7U;
        config->saw_harmonic = 26U;
        config->sine_gain_q15 = 9200;
        config->saw_gain_q15 = 11200;
        config->noise_gain_q15 = 9400;
        config->cam_gain_q15 = 8200;
        config->tremolo_depth_q15 = 8000;
        config->vibrato_depth_q15 = 560;
        config->distortion_drive_q12 = 6000;
        config->eq_gain_q12[0] = 3000;
        config->eq_gain_q12[1] = 3900;
        config->eq_gain_q12[2] = 4800;
        config->eq_gain_q12[3] = 5000;
        config->eq_gain_q12[4] = 3500;
        config->eq_gain_q12[5] = 2100;
        break;

    case GGM89_PRESET_GENERIC:
    default:
        break;
    }
}

int ggm89_init(ggm89_state *state,
               const ggm89_config *config,
               ggm89_u32 sample_rate)
{
    int i;

    if (state == 0 || config == 0) {
        return 0;
    }
    if (sample_rate < GGM89_MIN_SAMPLE_RATE ||
        sample_rate > GGM89_MAX_SAMPLE_RATE) {
        return 0;
    }
    if (config->barrel_count < 1U ||
        config->barrel_count > 16U ||
        config->nominal_spm < 60U ||
        config->nominal_spm > 12000U) {
        return 0;
    }

    memset(state, 0, sizeof(*state));
    state->config = *config;
    state->sample_rate = sample_rate;
    state->noise_state = 0x6d2b79f5UL;
    state->mode = GGM89_STATE_OFF;

    ggm89_setup_ramp(sample_rate,
                     config->spinup_ms,
                     &state->ramp_up_step,
                     &state->ramp_up_rem,
                     &state->ramp_up_den);
    ggm89_setup_ramp(sample_rate,
                     config->spindown_ms,
                     &state->ramp_down_step,
                     &state->ramp_down_rem,
                     &state->ramp_down_den);

    for (i = 0; i < GGM89_EQ_SPLITS; ++i) {
        state->eq_alpha_q15[i] = ggm89_alpha_for_hz(
            (ggm89_u32)ggm89_eq_split_hz[i],
            sample_rate);
    }
    state->noise_alpha_q15 = 1200;
    state->control_countdown = 0U;
    return 1;
}

void ggm89_reset(ggm89_state *state)
{
    ggm89_config config;
    ggm89_u32 sample_rate;

    if (state == 0) {
        return;
    }
    config = state->config;
    sample_rate = state->sample_rate;
    (void)ggm89_init(state, &config, sample_rate);
}

void ggm89_start(ggm89_state *state)
{
    if (state == 0) {
        return;
    }
    state->target_speed_q16 = GGM89_FULL_SPEED_Q16;
    state->mode = GGM89_STATE_SPINUP;
}

void ggm89_stop(ggm89_state *state)
{
    if (state == 0) {
        return;
    }
    state->target_speed_q16 = 0UL;
    if (state->speed_q16 > 0UL) {
        state->mode = GGM89_STATE_SPINDOWN;
    }
}

void ggm89_set_target_speed_q15(ggm89_state *state,
                                ggm89_s16 speed_q15)
{
    ggm89_s32 speed;

    if (state == 0) {
        return;
    }
    speed = (ggm89_s32)speed_q15;
    if (speed < 0L) {
        speed = 0L;
    }
    if (speed > 32767L) {
        speed = 32767L;
    }
    state->target_speed_q16 = (ggm89_u32)speed * 2UL;
}

void ggm89_set_firing_load_q15(ggm89_state *state,
                               ggm89_s16 load_q15)
{
    ggm89_s32 load;

    if (state == 0) {
        return;
    }
    load = (ggm89_s32)load_q15;
    state->firing_load_q15 =
        ggm89_clip_s32(load, 0L, 32767L);
}

void ggm89_set_eq_gain_q12(ggm89_state *state,
                           int band,
                           ggm89_s16 gain_q12)
{
    ggm89_s32 gain;

    if (state == 0 ||
        band < 0 ||
        band >= GGM89_EQ_BANDS) {
        return;
    }
    gain = (ggm89_s32)gain_q12;
    gain = ggm89_clip_s32(gain, 0L, 8192L);
    state->config.eq_gain_q12[band] = (ggm89_s16)gain;
}

void ggm89_set_distortion_drive_q12(ggm89_state *state,
                                    ggm89_s16 drive_q12)
{
    ggm89_s32 drive;

    if (state == 0) {
        return;
    }
    drive = (ggm89_s32)drive_q12;
    drive = ggm89_clip_s32(drive, 1024L, 12288L);
    state->config.distortion_drive_q12 = (ggm89_s16)drive;
}

void ggm89_set_reverb_q15(ggm89_state *state,
                          ggm89_s16 mix_q15,
                          ggm89_s16 feedback_q15)
{
    if (state == 0) {
        return;
    }
    state->config.reverb_mix_q15 = (ggm89_s16)
        ggm89_clip_s32((ggm89_s32)mix_q15, 0L, 16384L);
    state->config.reverb_feedback_q15 = (ggm89_s16)
        ggm89_clip_s32((ggm89_s32)feedback_q15, 0L, 24576L);
}

void ggm89_render_mono(ggm89_state *state,
                       ggm89_s16 *output,
                       ggm89_u16 frame_count)
{
    ggm89_u16 i;
    ggm89_s32 sine_sample;
    ggm89_s32 saw_sample;
    ggm89_s32 noise_sample;
    ggm89_s32 filtered_noise;
    ggm89_s32 friction_noise;
    ggm89_s32 cam_exciter;
    ggm89_s32 cam_sample;
    ggm89_s32 rotor_lfo;
    ggm89_s32 tremolo;
    ggm89_s32 mix;
    ggm89_s32 speed_q15;
    ggm89_u32 old_cam_phase;

    if (state == 0 || output == 0) {
        return;
    }

    for (i = 0U; i < frame_count; ++i) {
        ggm89_advance_speed(state);

        if (state->control_countdown == 0U) {
            ggm89_update_control(state);
            state->control_countdown = GGM89_CONTROL_PERIOD;
        }
        state->control_countdown -= 1U;

        state->phase_sine =
            (state->phase_sine + state->inc_sine) &
            GGM89_PHASE_MASK;
        state->phase_saw =
            (state->phase_saw + state->inc_saw) &
            GGM89_PHASE_MASK;
        state->phase_rotor =
            (state->phase_rotor + state->inc_rotor) &
            GGM89_PHASE_MASK;
        state->phase_vibrato =
            (state->phase_vibrato + state->inc_vibrato) &
            GGM89_PHASE_MASK;

        old_cam_phase = state->phase_cam;
        state->phase_cam =
            (state->phase_cam + state->inc_cam) &
            GGM89_PHASE_MASK;
        if (state->phase_cam < old_cam_phase &&
            state->speed_q16 > 256UL) {
            state->cam_env_q15 = 7000L +
                ggm89_mul_q15(
                    state->firing_load_q15,
                    24000L);
        }

        sine_sample = (ggm89_s32)ggm89_sine(state->phase_sine);
        saw_sample = (ggm89_s32)ggm89_saw(state->phase_saw);
        noise_sample = (ggm89_s32)ggm89_noise_s16(
            &state->noise_state);

        state->noise_lp = ggm89_onepole(
            state->noise_lp,
            noise_sample,
            state->noise_alpha_q15);
        filtered_noise = state->noise_lp;
        friction_noise = filtered_noise +
            ((noise_sample - filtered_noise) / 5L);

        cam_exciter = (noise_sample / 2L) + (saw_sample / 3L);
        cam_sample = ggm89_mul_q15(
            cam_exciter,
            state->cam_env_q15);
        state->cam_env_q15 -= state->cam_env_q15 / 28L;
        if (state->cam_env_q15 < 2L) {
            state->cam_env_q15 = 0L;
        }

        rotor_lfo = (ggm89_s32)ggm89_sine(state->phase_rotor);
        tremolo = 32767L -
            ((ggm89_s32)state->config.tremolo_depth_q15 / 2L);
        tremolo += ggm89_mul_q15(
            rotor_lfo,
            (ggm89_s32)state->config.tremolo_depth_q15 / 2L);

        mix = ggm89_mul_q15(
            sine_sample,
            state->config.sine_gain_q15);
        mix += ggm89_mul_q15(
            saw_sample,
            state->config.saw_gain_q15);
        mix += ggm89_mul_q15(
            friction_noise,
            state->config.noise_gain_q15);
        mix += ggm89_mul_q15(
            cam_sample,
            state->config.cam_gain_q15);

        mix = ggm89_mul_q15(mix, tremolo);
        speed_q15 = (ggm89_s32)(state->speed_q16 >> 1);
        mix = ggm89_mul_q15(mix, speed_q15);

        mix = ggm89_apply_eq(state, mix);
        mix = ggm89_apply_distortion(state, mix);
        mix = ggm89_apply_reverb(state, mix);
        output[i] = ggm89_clip_s16(mix);
    }
}

ggm89_u16 ggm89_get_mode(const ggm89_state *state)
{
    if (state == 0) {
        return GGM89_STATE_OFF;
    }
    return state->mode;
}

ggm89_u16 ggm89_get_current_rpm(const ggm89_state *state)
{
    if (state == 0 || state->current_rpm < 0L) {
        return 0U;
    }
    if (state->current_rpm > 65535L) {
        return 65535U;
    }
    return (ggm89_u16)state->current_rpm;
}

ggm89_u32 ggm89_state_size_bytes(void)
{
    return (ggm89_u32)sizeof(ggm89_state);
}

const char *ggm89_preset_name(int preset_id)
{
    switch (preset_id) {
    case GGM89_PRESET_M134D:
        return "M134D electric six-barrel";
    case GGM89_PRESET_M197:
        return "M197 three-barrel";
    case GGM89_PRESET_M61:
        return "M61 Vulcan six-barrel";
    case GGM89_PRESET_GAU8:
        return "GAU-8 heavy seven-barrel";
    case GGM89_PRESET_GAU22:
        return "GAU-22 four-barrel";
    case GGM89_PRESET_GENERIC:
    default:
        return "Generic rotary mechanism";
    }
}

const char *ggm89_version_string(void)
{
    return "ggunmach89 1.0.0";
}
