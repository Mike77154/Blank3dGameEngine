#include "shotpumpkin89.h"

#include <string.h>

#define SP89_Q15_ONE 32767
#define SP89_Q15_HALF 16384

static const unsigned short sp89_pull_pulse_ms[] = {
    0, 7, 15, 24, 35, 48, 64, 83
};
static const unsigned short sp89_pull_pulse_level[] = {
    25000, 21000, 23500, 19000, 22000, 17500, 20500, 15000
};
static const unsigned short sp89_pump_pulse_ms[] = {
    0, 6, 13, 21, 30, 41, 54, 70
};
static const unsigned short sp89_pump_pulse_level[] = {
    21500, 23500, 20500, 25000, 22000, 26500, 23500, 28500
};

#define SP89_PULL_PULSES 8U
#define SP89_PUMP_PULSES 8U

static signed short sp89_clip16(signed int value)
{
    if (value > 32767) {
        return (signed short)32767;
    }
    if (value < -32768) {
        return (signed short)-32768;
    }
    return (signed short)value;
}

static signed int sp89_abs_i(signed int value)
{
    if (value < 0) {
        return -value;
    }
    return value;
}

static signed int sp89_mul_q15_i(signed int a, unsigned short b)
{
    return (a * (signed int)b) / 32768;
}

static signed int sp89_gain_q8(signed int sample, signed short gain)
{
    return (sample * (signed int)gain) / 256;
}

static unsigned int sp89_ms_to_samples(unsigned int sample_rate,
                                        unsigned int ms)
{
    unsigned int whole;
    unsigned int rem;

    whole = sample_rate / 1000U;
    rem = sample_rate % 1000U;
    return whole * ms + (rem * ms) / 1000U;
}

static unsigned short sp89_filter_alpha(unsigned int hz,
                                         unsigned int sample_rate)
{
    unsigned int x_q15;
    unsigned int numerator;
    unsigned int alpha;

    if (sample_rate < 8000U) {
        sample_rate = 8000U;
    }
    if (hz > sample_rate / 3U) {
        hz = sample_rate / 3U;
    }
    if (hz < 1U) {
        hz = 1U;
    }

    /* alpha ~= (2*pi*f/fs) / (1 + 2*pi*f/fs), all in Q15. */
    x_q15 = (hz * 205887U) / sample_rate;
    numerator = x_q15 * 32768U;
    alpha = numerator / (32768U + x_q15);
    if (alpha > 32767U) {
        alpha = 32767U;
    }
    if (alpha < 1U) {
        alpha = 1U;
    }
    return (unsigned short)alpha;
}

static signed short sp89_onepole_lp(signed short *state,
                                     signed short input,
                                     unsigned short alpha)
{
    signed int difference;
    signed int next;

    difference = (signed int)input - (signed int)(*state);
    next = (signed int)(*state) + sp89_mul_q15_i(difference, alpha);
    *state = sp89_clip16(next);
    return *state;
}

static signed short sp89_noise(sp89_state *state)
{
    unsigned int x;
    signed int value;

    x = state->rng;
    if (x == 0U) {
        x = 0x6D2B79F5U;
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state->rng = x;

    value = (signed int)((x >> 16) & 65535U) - 32768;
    return (signed short)value;
}

static signed short sp89_soft_clip(signed int input, signed short drive_q8)
{
    signed int value;
    signed int sign;
    signed int magnitude;
    signed int knee;

    value = sp89_gain_q8(input, drive_q8);
    sign = 1;
    if (value < 0) {
        sign = -1;
        value = -value;
    }

    knee = 19661;
    magnitude = value;
    if (magnitude > knee) {
        magnitude = knee + (magnitude - knee) / 4;
    }
    if (magnitude > 32767) {
        magnitude = 32767;
    }

    return (signed short)(sign * magnitude);
}

static signed short sp89_dc_block(sp89_state *state, signed short input)
{
    signed int output;

    output = (signed int)input - (signed int)state->dc_x1;
    output += ((signed int)state->dc_y1 * 32604) / 32768;
    state->dc_x1 = input;
    state->dc_y1 = sp89_clip16(output);
    return state->dc_y1;
}

static signed short sp89_three_band_eq(sp89_state *state,
                                        signed short input)
{
    signed short low;
    signed short high_lp;
    signed int mid;
    signed int high;
    signed int mixed;

    low = sp89_onepole_lp(&state->eq_low_lp, input,
                          state->eq_low_alpha);
    high_lp = sp89_onepole_lp(&state->eq_high_lp, input,
                              state->eq_high_alpha);
    mid = (signed int)high_lp - (signed int)low;
    high = (signed int)input - (signed int)high_lp;

    mixed = sp89_gain_q8((signed int)low, state->cfg.eq_low_q8);
    mixed += sp89_gain_q8(mid, state->cfg.eq_mid_q8);
    mixed += sp89_gain_q8(high, state->cfg.eq_high_q8);

    return sp89_clip16(mixed);
}

static signed short sp89_six_band_eq(sp89_state *state,
                                      signed short input)
{
    signed short lp[SP89_EQ6_CROSSOVERS];
    signed int band;
    signed int previous;
    signed int mixed;
    unsigned int i;

    for (i = 0U; i < SP89_EQ6_CROSSOVERS; ++i) {
        lp[i] = sp89_onepole_lp(&state->eq6_lp[i], input,
                                state->eq6_alpha[i]);
    }

    mixed = sp89_gain_q8((signed int)lp[0],
                          state->cfg.eq6_gain_q8[0]);
    previous = (signed int)lp[0];
    for (i = 1U; i < SP89_EQ6_CROSSOVERS; ++i) {
        band = (signed int)lp[i] - previous;
        mixed += sp89_gain_q8(band, state->cfg.eq6_gain_q8[i]);
        previous = (signed int)lp[i];
    }
    band = (signed int)input - previous;
    mixed += sp89_gain_q8(band,
                           state->cfg.eq6_gain_q8[SP89_EQ6_BANDS - 1U]);

    return sp89_clip16(mixed);
}

static signed int sp89_triangle_lfo_q15(sp89_state *state)
{
    unsigned int phase16;
    signed int value;

    phase16 = state->chirr_lfo_phase >> 16;
    if (phase16 < 32768U) {
        value = (signed int)(phase16 * 2U) - 32767;
    } else {
        value = 98301 - (signed int)(phase16 * 2U);
    }
    state->chirr_lfo_phase += state->chirr_lfo_inc;
    return value;
}

static signed short sp89_chorus(sp89_state *state, signed short input)
{
    unsigned int phase16;
    unsigned int triangle;
    unsigned int delay;
    unsigned int read_pos;
    signed short delayed;
    signed int wet;
    signed int dry;
    signed int mixed;

    state->chorus_buffer[state->chorus_write] = input;

    phase16 = state->chorus_phase >> 16;
    if (phase16 < 32768U) {
        triangle = phase16;
    } else {
        triangle = 65535U - phase16;
    }
    delay = state->chorus_base_samples;
    delay += (state->chorus_depth_samples * triangle) / 32767U;
    if (delay >= SP89_CHORUS_CAP) {
        delay = SP89_CHORUS_CAP - 1U;
    }

    if (state->chorus_write >= delay) {
        read_pos = state->chorus_write - delay;
    } else {
        read_pos = SP89_CHORUS_CAP + state->chorus_write - delay;
    }
    delayed = state->chorus_buffer[read_pos];

    wet = sp89_mul_q15_i((signed int)delayed,
                         state->cfg.chorus_mix_q15);
    dry = sp89_mul_q15_i((signed int)input,
                         (unsigned short)(32767U -
                         state->cfg.chorus_mix_q15));
    mixed = dry + wet;

    state->chorus_write++;
    if (state->chorus_write >= SP89_CHORUS_CAP) {
        state->chorus_write = 0U;
    }
    state->chorus_phase += state->chorus_inc;

    return sp89_clip16(mixed);
}

static signed short sp89_reverb(sp89_state *state, signed short input)
{
    signed short a;
    signed short b;
    signed short c;
    signed int next_a;
    signed int next_b;
    signed int next_c;
    signed int room;
    signed int wet;
    signed int dry;

    a = state->reverb_a[state->reverb_a_pos];
    b = state->reverb_b[state->reverb_b_pos];
    c = state->reverb_c[state->reverb_c_pos];

    next_a = (signed int)input +
             sp89_mul_q15_i((signed int)a,
                            state->cfg.reverb_feedback_q15);
    next_b = (signed int)input +
             sp89_mul_q15_i((signed int)b,
                            state->cfg.reverb_feedback_q15 - 1000U);
    next_c = (signed int)input +
             sp89_mul_q15_i((signed int)c,
                            state->cfg.reverb_feedback_q15 - 1800U);

    state->reverb_a[state->reverb_a_pos] = sp89_clip16(next_a);
    state->reverb_b[state->reverb_b_pos] = sp89_clip16(next_b);
    state->reverb_c[state->reverb_c_pos] = sp89_clip16(next_c);

    state->reverb_a_pos++;
    state->reverb_b_pos++;
    state->reverb_c_pos++;
    if (state->reverb_a_pos >= state->reverb_a_len) {
        state->reverb_a_pos = 0U;
    }
    if (state->reverb_b_pos >= state->reverb_b_len) {
        state->reverb_b_pos = 0U;
    }
    if (state->reverb_c_pos >= state->reverb_c_len) {
        state->reverb_c_pos = 0U;
    }

    room = ((signed int)a + (signed int)b + (signed int)c) / 3;
    wet = sp89_mul_q15_i(room, state->cfg.reverb_mix_q15);
    dry = sp89_mul_q15_i((signed int)input,
                         (unsigned short)(32767U -
                         state->cfg.reverb_mix_q15));
    return sp89_clip16(dry + wet);
}

static unsigned int sp89_variation(sp89_state *state, unsigned int range)
{
    unsigned int value;

    if (range == 0U || state->cfg.variation == 0U) {
        return 0U;
    }
    value = (unsigned int)(sp89_noise(state) + 32768);
    return (value % (range + 1U));
}

static void sp89_update_derived(sp89_state *state)
{
    unsigned int rate;
    unsigned int feedback;
    unsigned int i;
    unsigned int base;
    unsigned int whole_hz;
    unsigned int rem_millihz;

    rate = state->cfg.sample_rate;
    if (rate < 8000U) {
        rate = 8000U;
    }
    if (rate > 48000U) {
        rate = 48000U;
    }
    state->cfg.sample_rate = rate;

    if (state->cfg.pull_ms < 20U) {
        state->cfg.pull_ms = 20U;
    }
    if (state->cfg.pull_ms > 500U) {
        state->cfg.pull_ms = 500U;
    }
    if (state->cfg.pump_ms < 20U) {
        state->cfg.pump_ms = 20U;
    }
    if (state->cfg.pump_ms > 500U) {
        state->cfg.pump_ms = 500U;
    }
    if (state->cfg.gap_ms > 250U) {
        state->cfg.gap_ms = 250U;
    }
    if (state->cfg.chorus_mix_q15 > 32767U) {
        state->cfg.chorus_mix_q15 = 32767U;
    }
    if (state->cfg.reverb_mix_q15 > 32767U) {
        state->cfg.reverb_mix_q15 = 32767U;
    }
    if (state->cfg.chorus_rate_millihz > 1000U) {
        state->cfg.chorus_rate_millihz = 1000U;
    }
    if (state->cfg.chirr_lfo_rate_millihz > 20000U) {
        state->cfg.chirr_lfo_rate_millihz = 20000U;
    }
    if (state->cfg.chirr_lfo_depth_q15 > 8192U) {
        state->cfg.chirr_lfo_depth_q15 = 8192U;
    }
    state->cfg.eq6_enabled = (unsigned short)(
        state->cfg.eq6_enabled != 0U);

    state->friction_hp_alpha = sp89_filter_alpha(
        state->cfg.friction_hp_hz, rate);
    state->friction_lp_alpha = sp89_filter_alpha(
        state->cfg.friction_lp_hz, rate);
    state->impact_hp_alpha = sp89_filter_alpha(
        state->cfg.impact_hp_hz, rate);
    state->chirr_hp_alpha = sp89_filter_alpha(
        state->cfg.chirr_hp_hz, rate);
    state->chirr_lp_alpha = sp89_filter_alpha(
        state->cfg.chirr_lp_hz, rate);
    state->eq_low_alpha = sp89_filter_alpha(
        state->cfg.eq_low_hz, rate);
    state->eq_high_alpha = sp89_filter_alpha(
        state->cfg.eq_high_hz, rate);
    for (i = 0U; i < SP89_EQ6_CROSSOVERS; ++i) {
        state->eq6_alpha[i] = sp89_filter_alpha(
            state->cfg.eq6_cross_hz[i], rate);
    }

    base = 4294967295U / rate;
    whole_hz = state->cfg.chirr_lfo_rate_millihz / 1000U;
    rem_millihz = state->cfg.chirr_lfo_rate_millihz % 1000U;
    state->chirr_lfo_inc = base * whole_hz;
    state->chirr_lfo_inc += (base * rem_millihz) / 1000U;
    if (state->cfg.chirr_lfo_rate_millihz != 0U &&
        state->chirr_lfo_inc == 0U) {
        state->chirr_lfo_inc = 1U;
    }

    state->chorus_base_samples = sp89_ms_to_samples(
        rate, state->cfg.chorus_base_ms);
    state->chorus_depth_samples = sp89_ms_to_samples(
        rate, state->cfg.chorus_depth_ms);
    if (state->chorus_base_samples < 1U) {
        state->chorus_base_samples = 1U;
    }
    if (state->chorus_base_samples >= SP89_CHORUS_CAP - 1U) {
        state->chorus_base_samples = SP89_CHORUS_CAP - 2U;
    }
    if (state->chorus_base_samples + state->chorus_depth_samples >=
        SP89_CHORUS_CAP) {
        state->chorus_depth_samples = SP89_CHORUS_CAP -
            state->chorus_base_samples - 1U;
    }

    state->chorus_inc = (state->cfg.chorus_rate_millihz * 4294967U) /
                         rate;
    if (state->chorus_inc == 0U) {
        state->chorus_inc = 1U;
    }

    state->reverb_a_len = sp89_ms_to_samples(rate, 13U);
    state->reverb_b_len = sp89_ms_to_samples(rate, 19U);
    state->reverb_c_len = sp89_ms_to_samples(rate, 29U);
    if (state->reverb_a_len >= SP89_REVERB_CAP) {
        state->reverb_a_len = SP89_REVERB_CAP - 1U;
    }
    if (state->reverb_b_len >= SP89_REVERB_CAP) {
        state->reverb_b_len = SP89_REVERB_CAP - 1U;
    }
    if (state->reverb_c_len >= SP89_REVERB_CAP) {
        state->reverb_c_len = SP89_REVERB_CAP - 1U;
    }
    if (state->reverb_a_len < 1U) {
        state->reverb_a_len = 1U;
    }
    if (state->reverb_b_len < 1U) {
        state->reverb_b_len = 1U;
    }
    if (state->reverb_c_len < 1U) {
        state->reverb_c_len = 1U;
    }

    feedback = state->cfg.reverb_feedback_q15;
    if (feedback > 30000U) {
        state->cfg.reverb_feedback_q15 = 30000U;
    }
    if (state->cfg.reverb_feedback_q15 < 3000U) {
        state->cfg.reverb_feedback_q15 = 3000U;
    }
}

void sp89_config_default(sp89_config *cfg, unsigned int sample_rate)
{
    static const signed short gains[SP89_EQ6_BANDS] = {
        142, 195, 285, 326, 296, 220
    };
    static const unsigned int crosses[SP89_EQ6_CROSSOVERS] = {
        160U, 380U, 850U, 1900U, 4300U
    };
    unsigned int i;

    if (cfg == 0) {
        return;
    }

    cfg->sample_rate = sample_rate;
    cfg->pull_ms = 108U;
    cfg->gap_ms = 22U;
    cfg->pump_ms = 92U;

    cfg->master_gain_q8 = 228;
    cfg->pull_gain_q8 = 245;
    cfg->pump_gain_q8 = 268;
    cfg->friction_gain_q8 = 168;
    cfg->impact_gain_q8 = 305;
    cfg->chirr_gain_q8 = 34;
    cfg->drive_q8 = 316;

    cfg->eq_low_q8 = 170;
    cfg->eq_mid_q8 = 292;
    cfg->eq_high_q8 = 230;
    cfg->eq_low_hz = 330U;
    cfg->eq_high_hz = 3600U;

    cfg->eq6_enabled = 1U;
    for (i = 0U; i < SP89_EQ6_BANDS; ++i) {
        cfg->eq6_gain_q8[i] = gains[i];
    }
    for (i = 0U; i < SP89_EQ6_CROSSOVERS; ++i) {
        cfg->eq6_cross_hz[i] = crosses[i];
    }

    cfg->friction_hp_hz = 420U;
    cfg->friction_lp_hz = 5200U;
    cfg->impact_hp_hz = 1350U;
    cfg->chirr_hp_hz = 2100U;
    cfg->chirr_lp_hz = 7600U;
    cfg->chirr_lfo_rate_millihz = 11800U;
    cfg->chirr_lfo_depth_q15 = 1350U;

    cfg->chorus_mix_q15 = 1800U;
    cfg->reverb_mix_q15 = 2700U;
    cfg->reverb_feedback_q15 = 14500U;
    cfg->chorus_rate_millihz = 730U;
    cfg->chorus_base_ms = 5U;
    cfg->chorus_depth_ms = 2U;

    cfg->seed = 0x89C0FFEEU;
    cfg->variation = 1U;
}

static void sp89_set_eq6(sp89_config *cfg,
                            signed short b0, signed short b1,
                            signed short b2, signed short b3,
                            signed short b4, signed short b5)
{
    cfg->eq6_gain_q8[0] = b0;
    cfg->eq6_gain_q8[1] = b1;
    cfg->eq6_gain_q8[2] = b2;
    cfg->eq6_gain_q8[3] = b3;
    cfg->eq6_gain_q8[4] = b4;
    cfg->eq6_gain_q8[5] = b5;
}

void sp89_config_preset(sp89_config *cfg, unsigned int sample_rate,
                        sp89_preset preset)
{
    sp89_config_default(cfg, sample_rate);
    if (cfg == 0) {
        return;
    }

    if (preset == SP89_PRESET_HEAVY) {
        cfg->pull_ms = 122U;
        cfg->gap_ms = 26U;
        cfg->pump_ms = 104U;
        cfg->friction_gain_q8 = 184;
        cfg->impact_gain_q8 = 340;
        cfg->chirr_gain_q8 = 28;
        sp89_set_eq6(cfg, 215, 255, 308, 330, 260, 180);
        cfg->drive_q8 = 365;
        cfg->reverb_mix_q15 = 3900U;
    } else if (preset == SP89_PRESET_OILED) {
        cfg->pull_ms = 96U;
        cfg->gap_ms = 18U;
        cfg->pump_ms = 84U;
        cfg->friction_gain_q8 = 126;
        cfg->impact_gain_q8 = 280;
        cfg->chirr_gain_q8 = 20;
        sp89_set_eq6(cfg, 122, 172, 248, 280, 245, 180);
        cfg->drive_q8 = 288;
        cfg->chorus_mix_q15 = 1050U;
        cfg->reverb_mix_q15 = 1850U;
        cfg->chirr_lfo_depth_q15 = 700U;
    } else if (preset == SP89_PRESET_WORN) {
        cfg->pull_ms = 132U;
        cfg->gap_ms = 30U;
        cfg->pump_ms = 112U;
        cfg->friction_gain_q8 = 225;
        cfg->impact_gain_q8 = 288;
        cfg->chirr_gain_q8 = 58;
        sp89_set_eq6(cfg, 132, 188, 298, 340, 326, 262);
        cfg->drive_q8 = 350;
        cfg->chorus_mix_q15 = 3000U;
        cfg->chirr_lfo_rate_millihz = 8700U;
        cfg->chirr_lfo_depth_q15 = 2500U;
        cfg->variation = 3U;
    } else if (preset == SP89_PRESET_CINEMATIC) {
        cfg->pull_ms = 116U;
        cfg->gap_ms = 34U;
        cfg->pump_ms = 100U;
        cfg->master_gain_q8 = 236;
        cfg->impact_gain_q8 = 370;
        cfg->chirr_gain_q8 = 42;
        sp89_set_eq6(cfg, 235, 280, 338, 350, 310, 225);
        cfg->drive_q8 = 395;
        cfg->chorus_mix_q15 = 4000U;
        cfg->reverb_mix_q15 = 6200U;
        cfg->reverb_feedback_q15 = 17400U;
    } else if (preset == SP89_PRESET_REMINGTON_870) {
        cfg->pull_ms = 111U;
        cfg->gap_ms = 21U;
        cfg->pump_ms = 94U;
        cfg->friction_gain_q8 = 160;
        cfg->impact_gain_q8 = 320;
        cfg->chirr_gain_q8 = 29;
        sp89_set_eq6(cfg, 185, 235, 304, 330, 280, 205);
        cfg->drive_q8 = 326;
        cfg->reverb_mix_q15 = 2450U;
        cfg->chirr_lfo_depth_q15 = 1050U;
    } else if (preset == SP89_PRESET_MOSSBERG_590) {
        cfg->pull_ms = 114U;
        cfg->gap_ms = 23U;
        cfg->pump_ms = 98U;
        cfg->friction_gain_q8 = 176;
        cfg->impact_gain_q8 = 316;
        cfg->chirr_gain_q8 = 45;
        sp89_set_eq6(cfg, 150, 205, 294, 336, 320, 252);
        cfg->drive_q8 = 332;
        cfg->chirr_lfo_rate_millihz = 10400U;
        cfg->chirr_lfo_depth_q15 = 1700U;
        cfg->reverb_mix_q15 = 2550U;
    } else if (preset == SP89_PRESET_WINCHESTER_1300) {
        cfg->pull_ms = 86U;
        cfg->gap_ms = 14U;
        cfg->pump_ms = 78U;
        cfg->friction_gain_q8 = 146;
        cfg->impact_gain_q8 = 334;
        cfg->chirr_gain_q8 = 48;
        sp89_set_eq6(cfg, 120, 180, 265, 325, 348, 276);
        cfg->drive_q8 = 340;
        cfg->chirr_lfo_rate_millihz = 14300U;
        cfg->chirr_lfo_depth_q15 = 1600U;
        cfg->reverb_mix_q15 = 2200U;
        cfg->variation = 0U;
    } else if (preset == SP89_PRESET_ITHACA_37) {
        cfg->pull_ms = 119U;
        cfg->gap_ms = 24U;
        cfg->pump_ms = 101U;
        cfg->friction_gain_q8 = 174;
        cfg->impact_gain_q8 = 326;
        cfg->chirr_gain_q8 = 23;
        sp89_set_eq6(cfg, 225, 275, 324, 314, 245, 168);
        cfg->drive_q8 = 330;
        cfg->reverb_mix_q15 = 2350U;
        cfg->chirr_lfo_depth_q15 = 850U;
    } else if (preset == SP89_PRESET_BENELLI_NOVA) {
        cfg->pull_ms = 101U;
        cfg->gap_ms = 18U;
        cfg->pump_ms = 88U;
        cfg->friction_gain_q8 = 150;
        cfg->impact_gain_q8 = 328;
        cfg->chirr_gain_q8 = 38;
        sp89_set_eq6(cfg, 140, 196, 290, 330, 304, 210);
        cfg->drive_q8 = 320;
        cfg->reverb_mix_q15 = 1850U;
        cfg->reverb_feedback_q15 = 12500U;
        cfg->chirr_lfo_rate_millihz = 12600U;
        cfg->chirr_lfo_depth_q15 = 1200U;
    }
}

void sp89_reset(sp89_state *state)
{
    sp89_config saved;

    if (state == 0) {
        return;
    }
    saved = state->cfg;
    memset(state, 0, sizeof(*state));
    state->cfg = saved;
    state->rng = state->cfg.seed;
    state->phase = SP89_PHASE_IDLE;
    sp89_update_derived(state);
}

void sp89_init(sp89_state *state, const sp89_config *cfg)
{
    if (state == 0 || cfg == 0) {
        return;
    }
    memset(state, 0, sizeof(*state));
    state->cfg = *cfg;
    state->rng = cfg->seed;
    state->phase = SP89_PHASE_IDLE;
    sp89_update_derived(state);
}

void sp89_set_config(sp89_state *state, const sp89_config *cfg)
{
    if (state == 0 || cfg == 0) {
        return;
    }
    state->cfg = *cfg;
    sp89_update_derived(state);
}

static void sp89_start_phase(sp89_state *state, sp89_phase phase)
{
    unsigned int ms;

    state->phase = phase;
    state->sample_pos = 0U;
    state->pulse_index = 0U;
    state->next_pulse_sample = 0U;
    state->burst_env = 0;
    state->burst_step = 0;
    state->impact_env = 0;
    state->impact_step = 0;
    state->impact_fired = 0U;

    if (phase == SP89_PHASE_PULL) {
        ms = state->cfg.pull_ms;
    } else if (phase == SP89_PHASE_PUMP) {
        ms = state->cfg.pump_ms;
    } else {
        ms = state->cfg.gap_ms;
    }
    state->phase_samples = sp89_ms_to_samples(state->cfg.sample_rate, ms);
    if (state->phase_samples < 1U) {
        state->phase_samples = 1U;
    }
}

void sp89_trigger_pull(sp89_state *state)
{
    if (state == 0) {
        return;
    }
    state->pending_pump = 0U;
    sp89_start_phase(state, SP89_PHASE_PULL);
}

void sp89_trigger_pump(sp89_state *state)
{
    if (state == 0) {
        return;
    }
    state->pending_pump = 0U;
    sp89_start_phase(state, SP89_PHASE_PUMP);
}

void sp89_trigger_cycle(sp89_state *state)
{
    if (state == 0) {
        return;
    }
    state->pending_pump = 1U;
    sp89_start_phase(state, SP89_PHASE_PULL);
}

static void sp89_spawn_pulse(sp89_state *state)
{
    unsigned int pulse_ms;
    unsigned int pulse_level;
    unsigned int pulse_count;
    unsigned int duration_ms;
    unsigned int jitter;
    unsigned int burst_ms;

    if (state->phase == SP89_PHASE_PULL) {
        pulse_count = SP89_PULL_PULSES;
        if (state->pulse_index >= pulse_count) {
            return;
        }
        pulse_ms = sp89_pull_pulse_ms[state->pulse_index];
        pulse_level = sp89_pull_pulse_level[state->pulse_index];
        duration_ms = state->cfg.pull_ms;
    } else {
        pulse_count = SP89_PUMP_PULSES;
        if (state->pulse_index >= pulse_count) {
            return;
        }
        pulse_ms = sp89_pump_pulse_ms[state->pulse_index];
        pulse_level = sp89_pump_pulse_level[state->pulse_index];
        duration_ms = state->cfg.pump_ms;
    }

    if (duration_ms > 0U) {
        pulse_ms = (pulse_ms * duration_ms) / 100U;
    }
    jitter = sp89_variation(state, state->cfg.variation);
    pulse_ms += jitter;

    if (state->sample_pos >= sp89_ms_to_samples(
        state->cfg.sample_rate, pulse_ms)) {
        state->burst_env = (signed int)pulse_level;
        burst_ms = 4U + (state->pulse_index & 1U);
        state->burst_step = (signed int)pulse_level /
            (signed int)sp89_ms_to_samples(state->cfg.sample_rate,
                                            burst_ms);
        if (state->burst_step < 1) {
            state->burst_step = 1;
        }
        state->pulse_index++;
        if (state->pulse_index < pulse_count) {
            if (state->phase == SP89_PHASE_PULL) {
                pulse_ms = sp89_pull_pulse_ms[state->pulse_index];
            } else {
                pulse_ms = sp89_pump_pulse_ms[state->pulse_index];
            }
            pulse_ms = (pulse_ms * duration_ms) / 100U;
            state->next_pulse_sample = sp89_ms_to_samples(
                state->cfg.sample_rate, pulse_ms);
        }
    }
}

static signed int sp89_friction_shape(const sp89_state *state)
{
    signed int progress;
    signed int triangle;
    signed int base;

    if (state->phase_samples == 0U) {
        return 0;
    }
    progress = (signed int)((state->sample_pos * 32767U) /
                            state->phase_samples);
    triangle = 32767 - sp89_abs_i(progress * 2 - 32767);
    base = 6200 + triangle / 3;
    if (state->phase == SP89_PHASE_PUMP) {
        base += progress / 7;
    } else {
        base += (32767 - progress) / 10;
    }
    return base;
}

static void sp89_maybe_impact(sp89_state *state)
{
    unsigned int lead;
    unsigned int length;
    signed int level;

    if (state->impact_fired != 0U) {
        return;
    }
    lead = sp89_ms_to_samples(state->cfg.sample_rate, 5U);
    if (lead >= state->phase_samples) {
        lead = 1U;
    }
    if (state->sample_pos + lead >= state->phase_samples) {
        state->impact_fired = 1U;
        if (state->phase == SP89_PHASE_PUMP) {
            level = 32767;
        } else {
            level = 27500;
        }
        state->impact_env = level;
        length = sp89_ms_to_samples(state->cfg.sample_rate, 5U);
        if (length < 1U) {
            length = 1U;
        }
        state->impact_step = level / (signed int)length;
        if (state->impact_step < 1) {
            state->impact_step = 1;
        }
    }
}

static signed short sp89_render_mechanics(sp89_state *state)
{
    signed short noise_a;
    signed short noise_b;
    signed short noise_c;
    signed short lp;
    signed short hp_lp;
    signed short friction;
    signed short impact_lp;
    signed short chirr_hp_lp;
    signed short chirr_band;
    signed int impact_hp;
    signed int friction_env;
    signed int friction_part;
    signed int impact_part;
    signed int chirr_part;
    signed int phase_gain;
    signed int lfo;
    signed int lfo_amount;
    signed int mixed;

    sp89_spawn_pulse(state);
    sp89_maybe_impact(state);

    noise_a = sp89_noise(state);
    noise_b = sp89_noise(state);
    noise_c = sp89_noise(state);

    hp_lp = sp89_onepole_lp(&state->friction_hp_lp, noise_a,
                            state->friction_hp_alpha);
    friction = sp89_clip16((signed int)noise_a - (signed int)hp_lp);
    lp = sp89_onepole_lp(&state->friction_lp, friction,
                         state->friction_lp_alpha);
    friction = lp;

    impact_lp = sp89_onepole_lp(&state->impact_hp_lp, noise_b,
                                state->impact_hp_alpha);
    impact_hp = (signed int)noise_b - (signed int)impact_lp;

    chirr_hp_lp = sp89_onepole_lp(&state->chirr_hp_lp, noise_c,
                                  state->chirr_hp_alpha);
    chirr_band = sp89_clip16((signed int)noise_c -
                             (signed int)chirr_hp_lp);
    chirr_band = sp89_onepole_lp(&state->chirr_lp, chirr_band,
                                 state->chirr_lp_alpha);

    friction_env = sp89_friction_shape(state) + state->burst_env;
    if (friction_env > 32767) {
        friction_env = 32767;
    }

    friction_part = sp89_mul_q15_i((signed int)friction,
                                   (unsigned short)friction_env);
    friction_part = sp89_gain_q8(friction_part,
                                 state->cfg.friction_gain_q8);

    impact_part = sp89_mul_q15_i(impact_hp,
                                 (unsigned short)state->impact_env);
    impact_part = sp89_gain_q8(impact_part,
                               state->cfg.impact_gain_q8);

    lfo = sp89_triangle_lfo_q15(state);
    lfo_amount = (lfo *
        (signed int)state->cfg.chirr_lfo_depth_q15) / 32768;
    chirr_part = sp89_mul_q15_i((signed int)chirr_band,
                                (unsigned short)friction_env);
    chirr_part = sp89_gain_q8(chirr_part,
                              state->cfg.chirr_gain_q8);
    chirr_part += (chirr_part * lfo_amount) / 32768;

    if (state->phase == SP89_PHASE_PULL) {
        phase_gain = state->cfg.pull_gain_q8;
    } else {
        phase_gain = state->cfg.pump_gain_q8;
    }

    mixed = sp89_gain_q8(friction_part + impact_part + chirr_part,
                         (signed short)phase_gain);

    if (state->burst_env > 0) {
        state->burst_env -= state->burst_step;
        if (state->burst_env < 0) {
            state->burst_env = 0;
        }
    }
    if (state->impact_env > 0) {
        state->impact_env -= state->impact_step;
        if (state->impact_env < 0) {
            state->impact_env = 0;
        }
    }

    return sp89_clip16(mixed);
}

static void sp89_advance_phase(sp89_state *state)
{
    state->sample_pos++;
    if (state->sample_pos < state->phase_samples) {
        return;
    }

    if (state->phase == SP89_PHASE_PULL && state->pending_pump != 0U) {
        sp89_start_phase(state, SP89_PHASE_GAP);
    } else if (state->phase == SP89_PHASE_GAP) {
        state->pending_pump = 0U;
        sp89_start_phase(state, SP89_PHASE_PUMP);
    } else {
        state->phase = SP89_PHASE_IDLE;
        state->sample_pos = 0U;
        state->phase_samples = 0U;
    }
}

sp89_sample sp89_process(sp89_state *state)
{
    signed short dry;
    signed short eq;
    signed short clipped;
    signed short chorused;
    signed short reverbed;
    signed int output;

    if (state == 0) {
        return 0;
    }

    if (state->phase == SP89_PHASE_PULL ||
        state->phase == SP89_PHASE_PUMP) {
        dry = sp89_render_mechanics(state);
    } else {
        dry = 0;
    }

    if (state->cfg.eq6_enabled != 0U) {
        eq = sp89_six_band_eq(state, dry);
    } else {
        eq = sp89_three_band_eq(state, dry);
    }
    clipped = sp89_soft_clip((signed int)eq, state->cfg.drive_q8);
    chorused = sp89_chorus(state, clipped);
    reverbed = sp89_reverb(state, chorused);
    output = sp89_gain_q8((signed int)reverbed,
                          state->cfg.master_gain_q8);
    output = sp89_dc_block(state, sp89_clip16(output));

    if (state->phase != SP89_PHASE_IDLE) {
        sp89_advance_phase(state);
    }

    return sp89_clip16(output);
}

void sp89_process_buffer(sp89_state *state, sp89_sample *output,
                         unsigned int count)
{
    unsigned int i;

    if (state == 0 || output == 0) {
        return;
    }
    for (i = 0U; i < count; ++i) {
        output[i] = sp89_process(state);
    }
}

int sp89_is_active(const sp89_state *state)
{
    if (state == 0) {
        return 0;
    }
    if (state->phase != SP89_PHASE_IDLE) {
        return 1;
    }
    return 0;
}

unsigned int sp89_state_bytes(void)
{
    return (unsigned int)sizeof(sp89_state);
}
