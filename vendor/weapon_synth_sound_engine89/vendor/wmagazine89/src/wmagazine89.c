#include "wmagazine89.h"

/* No stdlib, stdio, string, math, float, double, heap, or global mutable state. */

#define WMAG89_INTERNAL_Q15_ONE 32767
#define WMAG89_INTERNAL_PI2_X1000 6283

typedef struct WMag89PresetDef {
    int time_scale_q15;
    int low_gain_q15;
    int mid_gain_q15;
    int high_gain_q15;
    int roughness_q15;
    int body_gain_q15;
    int spring_gain_q15;
    int tail_gain_q15;
    int mode_freq[WMAG89_MODES];
    int mode_decay[WMAG89_MODES];
    int mode_gain[WMAG89_MODES];
    int eq_gain[WMAG89_EQ_BANDS];
    int drive_q8;
    int distortion_mix_q15;
    int chorus_base_ms;
    int chorus_depth_ms;
    int chorus_mix_q15;
    int reverb_feedback_q15;
    int reverb_damping_q15;
    int reverb_mix_q15;
    int output_gain_q15;
} WMag89PresetDef;

static const short wmag89_sine_table[256] = {
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

static const WMag89PresetDef wmag89_presets[WMAG89_PRESET_COUNT] = {
    {
        24576, 7000, 30000, 15500, 18000, 25500, 18000, 9000,
        {720, 1480, 3180, 6100},
        {32718, 32702, 32670, 32620},
        {25000, 22000, 15500, 9000},
        {27000, 30000, 32767, 32767, 30000, 27000},
        340, 9000, 2, 1, 2600, 22600, 12000, 3900, 28500
    },
    {
        26214, 11000, 30500, 8500, 14500, 23500, 12000, 7000,
        {560, 1050, 2280, 4200},
        {32690, 32655, 32605, 32540},
        {25000, 20500, 12000, 6000},
        {29000, 32000, 32767, 31000, 26000, 21000},
        300, 6500, 2, 1, 1800, 21500, 14500, 3200, 30000
    },
    {
        36044, 9000, 30500, 15000, 23000, 28500, 22500, 10500,
        {420, 860, 1880, 4050},
        {32730, 32717, 32682, 32635},
        {27500, 24500, 17500, 10500},
        {28000, 31000, 32767, 32767, 30500, 26000},
        350, 10000, 3, 1, 2900, 23500, 11000, 4300, 27500
    },
    {
        32767, 8000, 30500, 16500, 19000, 27500, 18000, 9500,
        {360, 970, 2520, 5750},
        {32728, 32712, 32674, 32618},
        {27000, 24000, 17000, 10000},
        {27500, 30500, 32767, 32767, 31500, 28500},
        345, 8500, 3, 1, 2400, 23000, 11500, 4000, 28000
    },
    {
        34406, 12500, 31000, 9000, 16500, 27000, 14500, 8000,
        {300, 690, 1580, 3320},
        {32722, 32695, 32635, 32575},
        {28500, 22000, 13000, 6500},
        {30000, 32767, 32767, 30500, 25500, 20500},
        310, 7000, 3, 1, 1800, 22000, 14500, 3400, 30000
    },
    {
        42598, 15500, 30000, 10500, 17500, 30000, 17000, 10000,
        {235, 515, 1120, 2480},
        {32742, 32728, 32692, 32630},
        {30000, 26000, 17500, 9000},
        {31000, 32767, 32767, 31500, 27000, 22000},
        320, 7500, 4, 1, 1500, 24000, 12500, 4700, 28000
    },
    {
        45875, 19000, 29500, 9500, 26000, 32000, 26000, 12000,
        {170, 355, 760, 1770},
        {32748, 32738, 32712, 32665},
        {32767, 29000, 22000, 12500},
        {32767, 32767, 31500, 28500, 23500, 19000},
        360, 10500, 4, 2, 2200, 24500, 12000, 5200, 25000
    }
};

static const WMag89Event wmag89_insert_events[] = {
    {0, 6, 25000, WMAG89_EVENT_BURST, 1},
    {10, 7, 20500, WMAG89_EVENT_BURST, 1},
    {20, 8, 17500, WMAG89_EVENT_BURST, 1},
    {2, 34, 15500, WMAG89_EVENT_SCRAPE, 1},
    {35, 9, 31000, WMAG89_EVENT_CATCH, 2},
    {40, 16, 13500, WMAG89_EVENT_SPRING, 2},
    {33, 18, 12000, WMAG89_EVENT_THUMP, 0}
};

static const WMag89Event wmag89_remove_events[] = {
    {0, 7, 22000, WMAG89_EVENT_CATCH, 2},
    {3, 42, 17500, WMAG89_EVENT_SCRAPE, 1},
    {12, 7, 14500, WMAG89_EVENT_BURST, 1},
    {28, 8, 12500, WMAG89_EVENT_BURST, 1},
    {44, 12, 19000, WMAG89_EVENT_SPRING, 2},
    {42, 20, 10500, WMAG89_EVENT_THUMP, 0}
};

static const WMag89Event wmag89_seat_events[] = {
    {0, 18, 29000, WMAG89_EVENT_THUMP, 0},
    {3, 7, 17000, WMAG89_EVENT_BURST, 1},
    {8, 10, 26500, WMAG89_EVENT_CATCH, 2},
    {15, 18, 12500, WMAG89_EVENT_SPRING, 2}
};

static const WMag89Event wmag89_tug_events[] = {
    {0, 22, 15500, WMAG89_EVENT_SCRAPE, 1},
    {4, 8, 12500, WMAG89_EVENT_BURST, 1},
    {22, 8, 22000, WMAG89_EVENT_CATCH, 2},
    {24, 14, 9000, WMAG89_EVENT_THUMP, 0}
};

static const WMag89Event wmag89_rattle_events[] = {
    {0, 9, 17000, WMAG89_EVENT_SPRING, 2},
    {11, 7, 13500, WMAG89_EVENT_CATCH, 1},
    {22, 10, 16500, WMAG89_EVENT_SPRING, 2},
    {36, 8, 12500, WMAG89_EVENT_CATCH, 1},
    {49, 12, 15500, WMAG89_EVENT_SPRING, 2},
    {63, 16, 9500, WMAG89_EVENT_THUMP, 0}
};

static void wmag89_zero_all(WMag89State *state)
{
    volatile unsigned char *bytes;
    unsigned long index;
    unsigned long count;

    bytes = (volatile unsigned char *)state;
    count = (unsigned long)sizeof(WMag89State);
    for (index = 0UL; index < count; ++index) {
        bytes[index] = 0U;
    }
}

static int wmag89_clamp_int(int value, int lo, int hi)
{
    if (value < lo) {
        return lo;
    }
    if (value > hi) {
        return hi;
    }
    return value;
}

static short wmag89_clamp_short(int value)
{
    if (value < -32768) {
        return (short)-32768;
    }
    if (value > 32767) {
        return (short)32767;
    }
    return (short)value;
}

static int wmag89_q15_mul(int a, int b)
{
    return (a * b) >> 15;
}

static int wmag89_abs_int(int value)
{
    if (value < 0) {
        return -value;
    }
    return value;
}

static int wmag89_ms_to_samples(const WMag89State *state, int ms)
{
    return (ms * state->sample_rate) / 1000;
}

static int wmag89_onepole_alpha(int sample_rate, int cutoff_hz)
{
    int numerator;
    int denominator;
    int alpha;

    if (cutoff_hz <= 0) {
        return 0;
    }
    numerator = cutoff_hz * 205881;
    denominator = sample_rate + ((cutoff_hz * WMAG89_INTERNAL_PI2_X1000) / 1000);
    if (denominator <= 0) {
        return 0;
    }
    alpha = numerator / denominator;
    return wmag89_clamp_int(alpha, 1, 32760);
}

static int wmag89_onepole(int input, int *memory, int alpha_q15)
{
    int delta;
    delta = input - *memory;
    *memory += wmag89_q15_mul(delta, alpha_q15);
    return *memory;
}

static int wmag89_noise(WMag89State *state)
{
    unsigned int x;
    x = state->rng;
    if (x == 0U) {
        x = 0x6D2B79F5U;
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    state->rng = x;
    return (int)(x >> 16) - 32768;
}

static int wmag89_event_envelope(const WMag89State *state,
                                 const WMag89Event *event,
                                 int local_sample)
{
    int duration;
    int attack;
    int release;
    int remain;
    int envelope;
    int period;
    int phase;
    int tri;

    duration = wmag89_ms_to_samples(state, event->duration_ms);
    if (duration < 1 || local_sample < 0 || local_sample >= duration) {
        return 0;
    }

    if (event->type == WMAG89_EVENT_SCRAPE) {
        attack = wmag89_ms_to_samples(state, 3);
        release = wmag89_ms_to_samples(state, 6);
        if (attack < 1) {
            attack = 1;
        }
        if (release < 1) {
            release = 1;
        }
        if (local_sample < attack) {
            envelope = (local_sample * 32767) / attack;
        } else if (local_sample > duration - release) {
            remain = duration - local_sample;
            envelope = (remain * 32767) / release;
        } else {
            envelope = 32767;
        }

        period = wmag89_ms_to_samples(state, 5);
        if (period < 4) {
            period = 4;
        }
        phase = local_sample % period;
        if (phase < period / 2) {
            tri = (phase * 32767) / (period / 2);
        } else {
            tri = ((period - phase) * 32767) / (period - (period / 2));
        }
        tri = 15500 + (tri >> 1);
        return wmag89_q15_mul(envelope, tri);
    }

    attack = state->sample_rate / 2000;
    if (event->type == WMAG89_EVENT_THUMP) {
        attack = state->sample_rate / 1000;
    }
    if (attack < 1) {
        attack = 1;
    }
    if (attack >= duration) {
        attack = 1;
    }

    if (local_sample < attack) {
        return (local_sample * 32767) / attack;
    }

    remain = duration - local_sample;
    return (remain * 32767) / (duration - attack);
}

static int wmag89_soft_saturate(int input, int drive_q8)
{
    int value;
    int sign;
    int magnitude;
    int knee;

    value = (input * drive_q8) >> 8;
    sign = 1;
    if (value < 0) {
        sign = -1;
        value = -value;
    }
    magnitude = value;
    knee = 18000;
    if (magnitude > knee) {
        magnitude = knee + ((magnitude - knee) >> 2);
    }
    if (magnitude > 32767) {
        magnitude = 32767;
    }
    return magnitude * sign;
}

static int wmag89_process_distortion(WMag89State *state, int input)
{
    int wet;
    int dry_part;
    int wet_part;

    wet = wmag89_soft_saturate(input, state->distortion_drive_q8);
    dry_part = wmag89_q15_mul(input, 32767 - state->distortion_mix_q15);
    wet_part = wmag89_q15_mul(wet, state->distortion_mix_q15);
    return dry_part + wet_part;
}

static int wmag89_process_eq(WMag89State *state, int input)
{
    int lp0;
    int lp1;
    int lp2;
    int lp3;
    int lp4;
    int bands[WMAG89_EQ_BANDS];
    int output;
    int index;

    lp0 = wmag89_onepole(input, &state->eq_lp[0], state->eq_alpha[0]);
    lp1 = wmag89_onepole(input, &state->eq_lp[1], state->eq_alpha[1]);
    lp2 = wmag89_onepole(input, &state->eq_lp[2], state->eq_alpha[2]);
    lp3 = wmag89_onepole(input, &state->eq_lp[3], state->eq_alpha[3]);
    lp4 = wmag89_onepole(input, &state->eq_lp[4], state->eq_alpha[4]);

    bands[0] = lp0;
    bands[1] = lp1 - lp0;
    bands[2] = lp2 - lp1;
    bands[3] = lp3 - lp2;
    bands[4] = lp4 - lp3;
    bands[5] = input - lp4;

    output = 0;
    for (index = 0; index < WMAG89_EQ_BANDS; ++index) {
        output += wmag89_q15_mul(bands[index], state->eq_gain_q15[index]);
    }
    return output;
}

static int wmag89_process_chorus(WMag89State *state, int input)
{
    unsigned int p;
    int tri;
    int delay;
    int read_index;
    int delayed;
    int dry_part;
    int wet_part;

    state->chorus_buffer[state->chorus_write] = wmag89_clamp_short(input);

    p = state->chorus_phase & 65535U;
    if (p < 32768U) {
        tri = (int)p;
    } else {
        tri = (int)(65535U - p);
    }
    delay = state->chorus_base_samples +
            ((tri * state->chorus_depth_samples) >> 15);
    delay = wmag89_clamp_int(delay, 1, WMAG89_CHORUS_SAMPLES - 1);

    read_index = state->chorus_write - delay;
    if (read_index < 0) {
        read_index += WMAG89_CHORUS_SAMPLES;
    }
    delayed = (int)state->chorus_buffer[read_index];

    ++state->chorus_write;
    if (state->chorus_write >= WMAG89_CHORUS_SAMPLES) {
        state->chorus_write = 0;
    }
    state->chorus_phase = (state->chorus_phase + 1U) & 65535U;

    dry_part = wmag89_q15_mul(input, 32767 - state->chorus_mix_q15);
    wet_part = wmag89_q15_mul(delayed, state->chorus_mix_q15);
    return dry_part + wet_part;
}

static int wmag89_comb_step(short *buffer, int length, int *position,
                            int input, int feedback_q15,
                            int damping_q15, int *damp_memory)
{
    int delayed;
    int damped;
    int write_value;

    delayed = (int)buffer[*position];
    damped = wmag89_q15_mul(delayed, 32767 - damping_q15) +
             wmag89_q15_mul(*damp_memory, damping_q15);
    *damp_memory = damped;
    write_value = input + wmag89_q15_mul(damped, feedback_q15);
    buffer[*position] = wmag89_clamp_short(write_value);
    ++(*position);
    if (*position >= length) {
        *position = 0;
    }
    return delayed;
}

static int wmag89_process_reverb(WMag89State *state, int input)
{
    int comb1;
    int comb2;
    int comb3;
    int comb_sum;
    int ap_delayed;
    int ap_output;
    int ap_write;
    int dry_part;
    int wet_part;
    int feedback2;
    int feedback3;

    feedback2 = state->reverb_feedback_q15 - 700;
    feedback3 = state->reverb_feedback_q15 - 1300;
    if (feedback2 < 0) {
        feedback2 = 0;
    }
    if (feedback3 < 0) {
        feedback3 = 0;
    }

    comb1 = wmag89_comb_step(state->reverb_comb1, WMAG89_REVERB_COMB1,
                            &state->reverb_pos1, input,
                            state->reverb_feedback_q15,
                            state->reverb_damping_q15,
                            &state->reverb_damp1);
    comb2 = wmag89_comb_step(state->reverb_comb2, WMAG89_REVERB_COMB2,
                            &state->reverb_pos2, input,
                            feedback2,
                            state->reverb_damping_q15,
                            &state->reverb_damp2);
    comb3 = wmag89_comb_step(state->reverb_comb3, WMAG89_REVERB_COMB3,
                            &state->reverb_pos3, input,
                            feedback3,
                            state->reverb_damping_q15,
                            &state->reverb_damp3);
    comb_sum = (comb1 + comb2 + comb3) / 3;

    ap_delayed = (int)state->reverb_allpass[state->reverb_pos_ap];
    ap_output = ap_delayed - comb_sum;
    ap_write = comb_sum + wmag89_q15_mul(ap_delayed, 16384);
    state->reverb_allpass[state->reverb_pos_ap] = wmag89_clamp_short(ap_write);
    ++state->reverb_pos_ap;
    if (state->reverb_pos_ap >= WMAG89_REVERB_ALLPASS) {
        state->reverb_pos_ap = 0;
    }

    dry_part = wmag89_q15_mul(input, 32767 - state->reverb_mix_q15);
    wet_part = wmag89_q15_mul(ap_output, state->reverb_mix_q15);
    return dry_part + wet_part;
}

static void wmag89_copy_events(WMag89State *state,
                               const WMag89Event *events,
                               int count)
{
    int index;
    int scaled_start;
    int scaled_duration;
    int jitter;

    if (count > WMAG89_MAX_EVENTS) {
        count = WMAG89_MAX_EVENTS;
    }
    state->event_count = count;
    for (index = 0; index < count; ++index) {
        scaled_start = wmag89_q15_mul(events[index].start_ms,
                                     state->time_scale_q15);
        scaled_duration = wmag89_q15_mul(events[index].duration_ms,
                                        state->time_scale_q15);
        jitter = ((wmag89_noise(state) >> 13) % 3);
        if (index == 0) {
            jitter = 0;
        }
        state->events[index] = events[index];
        state->events[index].start_ms = scaled_start + jitter;
        if (state->events[index].start_ms < 0) {
            state->events[index].start_ms = 0;
        }
        state->events[index].duration_ms = scaled_duration;
        if (state->events[index].duration_ms < 1) {
            state->events[index].duration_ms = 1;
        }
    }
}

static void wmag89_excitate_modes(WMag89State *state, int amount_q15,
                                  int event_type)
{
    int index;
    int scaled;
    int type_gain;

    if (event_type == WMAG89_EVENT_CATCH) {
        type_gain = 32767;
    } else if (event_type == WMAG89_EVENT_THUMP) {
        type_gain = 24500;
    } else if (event_type == WMAG89_EVENT_SPRING) {
        type_gain = 16500;
    } else {
        type_gain = 12500;
    }

    for (index = 0; index < WMAG89_MODES; ++index) {
        scaled = wmag89_q15_mul(amount_q15, state->modes[index].gain_q15);
        scaled = wmag89_q15_mul(scaled, type_gain);
        state->modes[index].amplitude_q15 += scaled;
        state->modes[index].amplitude_q15 =
            wmag89_clamp_int(state->modes[index].amplitude_q15, 0, 32767);
        state->modes[index].phase =
            (state->modes[index].phase + (state->rng & 255U)) & 65535U;
    }
}

static int wmag89_render_modes(WMag89State *state)
{
    int index;
    int sample;
    int output;
    unsigned int table_index;

    output = 0;
    for (index = 0; index < WMAG89_MODES; ++index) {
        table_index = (state->modes[index].phase >> 8) & 255U;
        sample = (int)wmag89_sine_table[table_index];
        output += wmag89_q15_mul(sample, state->modes[index].amplitude_q15);
        state->modes[index].phase =
            (state->modes[index].phase + state->modes[index].increment) & 65535U;
        state->modes[index].amplitude_q15 =
            wmag89_q15_mul(state->modes[index].amplitude_q15,
                           state->modes[index].decay_q15);
        if (state->modes[index].amplitude_q15 < 2) {
            state->modes[index].amplitude_q15 = 0;
        }
    }
    return wmag89_q15_mul(output, state->body_gain_q15);
}

static int wmag89_render_source(WMag89State *state)
{
    int noise;
    int lp_low;
    int lp_mid;
    int lp_high;
    int lp_air;
    int low_band;
    int mid_band;
    int high_band;
    int source;
    int index;
    int start;
    int local;
    int envelope;
    int event_gain;
    int colored;
    int event_sample;
    int rough;
    int rough_mod;
    int tail_start;
    int tail_length;
    int tail_local;
    int tail_env;

    noise = wmag89_noise(state);
    lp_low = wmag89_onepole(noise, &state->source_lp_low,
                            state->source_alpha_low);
    lp_mid = wmag89_onepole(noise, &state->source_lp_mid,
                            state->source_alpha_mid);
    lp_high = wmag89_onepole(noise, &state->source_lp_high,
                             state->source_alpha_high);
    lp_air = wmag89_onepole(noise, &state->source_lp_air,
                            state->source_alpha_air);

    low_band = lp_mid;
    mid_band = lp_high - lp_low;
    high_band = lp_air - lp_mid;
    source = 0;

    rough = wmag89_onepole(wmag89_abs_int(noise) - 16384,
                           &state->rough_state, 900);
    rough_mod = 24500 + wmag89_q15_mul(rough, state->roughness_q15);
    rough_mod = wmag89_clamp_int(rough_mod, 8000, 32767);

    for (index = 0; index < state->event_count; ++index) {
        start = wmag89_ms_to_samples(state, state->events[index].start_ms);
        local = state->sample_cursor - start;
        envelope = wmag89_event_envelope(state, &state->events[index], local);
        if (envelope <= 0) {
            continue;
        }

        if (state->events[index].color <= 0) {
            colored = wmag89_q15_mul(low_band, state->source_low_gain_q15);
        } else if (state->events[index].color == 1) {
            colored = wmag89_q15_mul(mid_band, state->source_mid_gain_q15);
        } else {
            colored = wmag89_q15_mul(high_band, state->source_high_gain_q15);
        }

        event_gain = wmag89_q15_mul(state->events[index].gain_q15,
                                   state->velocity_q15);
        event_sample = wmag89_q15_mul(colored, envelope);
        event_sample = wmag89_q15_mul(event_sample, event_gain);

        if (state->events[index].type == WMAG89_EVENT_SCRAPE) {
            event_sample = wmag89_q15_mul(event_sample, rough_mod);
        } else if (state->events[index].type == WMAG89_EVENT_SPRING) {
            event_sample = wmag89_q15_mul(event_sample,
                                         state->spring_gain_q15);
        } else if (state->events[index].type == WMAG89_EVENT_THUMP) {
            event_sample = wmag89_q15_mul(event_sample, 23000);
        }

        source += event_sample;

        if (state->sample_cursor == start &&
            (state->fired_mask & (1U << index)) == 0U) {
            wmag89_excitate_modes(state, event_gain,
                                  state->events[index].type);
            state->fired_mask |= (1U << index);
        }
    }

    tail_start = state->source_finished_sample - wmag89_ms_to_samples(state, 85);
    tail_length = state->source_finished_sample - tail_start;
    tail_local = state->sample_cursor - tail_start;
    if (tail_length > 0 && tail_local >= 0 && tail_local < tail_length) {
        tail_env = ((tail_length - tail_local) * 32767) / tail_length;
        event_sample = wmag89_q15_mul(mid_band, state->tail_gain_q15);
        event_sample = wmag89_q15_mul(event_sample, tail_env);
        source += event_sample;
    }

    return source;
}

static void wmag89_prepare_filter_coefficients(WMag89State *state)
{
    state->source_alpha_low = wmag89_onepole_alpha(state->sample_rate, 280);
    state->source_alpha_mid = wmag89_onepole_alpha(state->sample_rate, 1200);
    state->source_alpha_high = wmag89_onepole_alpha(state->sample_rate, 5200);
    state->source_alpha_air = wmag89_onepole_alpha(state->sample_rate, 7500);

    state->eq_alpha[0] = wmag89_onepole_alpha(state->sample_rate, 180);
    state->eq_alpha[1] = wmag89_onepole_alpha(state->sample_rate, 450);
    state->eq_alpha[2] = wmag89_onepole_alpha(state->sample_rate, 1000);
    state->eq_alpha[3] = wmag89_onepole_alpha(state->sample_rate, 2400);
    state->eq_alpha[4] = wmag89_onepole_alpha(state->sample_rate, 5500);
}

int wmag89_init(WMag89State *state, int sample_rate, unsigned int seed)
{
    if (state == (WMag89State *)0) {
        return WMAG89_ERR_NULL;
    }
    if (sample_rate < 8000 || sample_rate > 48000) {
        return WMAG89_ERR_RANGE;
    }

    wmag89_zero_all(state);
    state->sample_rate = sample_rate;
    state->rng = seed;
    if (state->rng == 0U) {
        state->rng = 0xA341316CU;
    }
    wmag89_prepare_filter_coefficients(state);
    return wmag89_set_preset(state, WMAG89_PRESET_PISTOL_METAL);
}

void wmag89_reset(WMag89State *state)
{
    int index;

    if (state == (WMag89State *)0) {
        return;
    }

    state->active = 0;
    state->sample_cursor = 0;
    state->event_count = 0;
    state->fired_mask = 0U;
    state->source_lp_low = 0;
    state->source_lp_mid = 0;
    state->source_lp_high = 0;
    state->source_lp_air = 0;
    state->rough_state = 0;
    state->source_finished_sample = 0;

    for (index = 0; index < WMAG89_MODES; ++index) {
        state->modes[index].phase = 0U;
        state->modes[index].amplitude_q15 = 0;
    }
    for (index = 0; index < 5; ++index) {
        state->eq_lp[index] = 0;
    }
    for (index = 0; index < WMAG89_CHORUS_SAMPLES; ++index) {
        state->chorus_buffer[index] = 0;
    }
    state->chorus_write = 0;
    state->chorus_phase = 0U;

    for (index = 0; index < WMAG89_REVERB_COMB1; ++index) {
        state->reverb_comb1[index] = 0;
    }
    for (index = 0; index < WMAG89_REVERB_COMB2; ++index) {
        state->reverb_comb2[index] = 0;
    }
    for (index = 0; index < WMAG89_REVERB_COMB3; ++index) {
        state->reverb_comb3[index] = 0;
    }
    for (index = 0; index < WMAG89_REVERB_ALLPASS; ++index) {
        state->reverb_allpass[index] = 0;
    }
    state->reverb_pos1 = 0;
    state->reverb_pos2 = 0;
    state->reverb_pos3 = 0;
    state->reverb_pos_ap = 0;
    state->reverb_damp1 = 0;
    state->reverb_damp2 = 0;
    state->reverb_damp3 = 0;
}

int wmag89_set_preset(WMag89State *state, int preset)
{
    const WMag89PresetDef *definition;
    int index;

    if (state == (WMag89State *)0) {
        return WMAG89_ERR_NULL;
    }
    if (preset < 0 || preset >= WMAG89_PRESET_COUNT) {
        return WMAG89_ERR_RANGE;
    }

    definition = &wmag89_presets[preset];
    state->preset = preset;
    state->time_scale_q15 = definition->time_scale_q15;
    state->source_low_gain_q15 = definition->low_gain_q15;
    state->source_mid_gain_q15 = definition->mid_gain_q15;
    state->source_high_gain_q15 = definition->high_gain_q15;
    state->roughness_q15 = definition->roughness_q15;
    state->body_gain_q15 = definition->body_gain_q15;
    state->spring_gain_q15 = definition->spring_gain_q15;
    state->tail_gain_q15 = definition->tail_gain_q15;

    for (index = 0; index < WMAG89_MODES; ++index) {
        state->modes[index].increment =
            (unsigned int)((definition->mode_freq[index] * 65536) /
                           state->sample_rate);
        state->modes[index].decay_q15 = definition->mode_decay[index];
        state->modes[index].gain_q15 = definition->mode_gain[index];
    }
    for (index = 0; index < WMAG89_EQ_BANDS; ++index) {
        state->eq_gain_q15[index] = definition->eq_gain[index];
    }

    state->distortion_drive_q8 = definition->drive_q8;
    state->distortion_mix_q15 = definition->distortion_mix_q15;
    wmag89_set_chorus(state, definition->chorus_base_ms,
                      definition->chorus_depth_ms,
                      definition->chorus_mix_q15);
    wmag89_set_reverb(state, definition->reverb_feedback_q15,
                      definition->reverb_damping_q15,
                      definition->reverb_mix_q15);
    state->output_gain_q15 = definition->output_gain_q15;
    return WMAG89_OK;
}

int wmag89_set_eq_gain(WMag89State *state, int band, int gain_q15)
{
    if (state == (WMag89State *)0) {
        return WMAG89_ERR_NULL;
    }
    if (band < 0 || band >= WMAG89_EQ_BANDS) {
        return WMAG89_ERR_RANGE;
    }
    state->eq_gain_q15[band] = wmag89_clamp_int(gain_q15, 0, 65535);
    return WMAG89_OK;
}

void wmag89_set_distortion(WMag89State *state, int drive_q8, int mix_q15)
{
    if (state == (WMag89State *)0) {
        return;
    }
    state->distortion_drive_q8 = wmag89_clamp_int(drive_q8, 128, 768);
    state->distortion_mix_q15 = wmag89_clamp_int(mix_q15, 0, 32767);
}

void wmag89_set_chorus(WMag89State *state, int base_ms, int depth_ms,
                       int mix_q15)
{
    int base_samples;
    int depth_samples;

    if (state == (WMag89State *)0) {
        return;
    }
    base_samples = wmag89_ms_to_samples(state, base_ms);
    depth_samples = wmag89_ms_to_samples(state, depth_ms);
    base_samples = wmag89_clamp_int(base_samples, 1,
                                    WMAG89_CHORUS_SAMPLES - 2);
    depth_samples = wmag89_clamp_int(depth_samples, 0,
                                     WMAG89_CHORUS_SAMPLES - 2 - base_samples);
    state->chorus_base_samples = base_samples;
    state->chorus_depth_samples = depth_samples;
    state->chorus_mix_q15 = wmag89_clamp_int(mix_q15, 0, 32767);
}

void wmag89_set_reverb(WMag89State *state, int feedback_q15,
                       int damping_q15, int mix_q15)
{
    if (state == (WMag89State *)0) {
        return;
    }
    state->reverb_feedback_q15 =
        wmag89_clamp_int(feedback_q15, 0, 30000);
    state->reverb_damping_q15 =
        wmag89_clamp_int(damping_q15, 0, 30000);
    state->reverb_mix_q15 = wmag89_clamp_int(mix_q15, 0, 32767);
}

void wmag89_set_output_gain(WMag89State *state, int gain_q15)
{
    if (state == (WMag89State *)0) {
        return;
    }
    state->output_gain_q15 = wmag89_clamp_int(gain_q15, 0, 65535);
}

int wmag89_trigger(WMag89State *state, int action, int velocity_q15)
{
    int index;
    int end_ms;
    int event_end;

    if (state == (WMag89State *)0) {
        return WMAG89_ERR_NULL;
    }
    if (action < 0 || action >= WMAG89_ACTION_COUNT) {
        return WMAG89_ERR_RANGE;
    }

    state->action = action;
    state->velocity_q15 = wmag89_clamp_int(velocity_q15, 1, 32767);
    state->sample_cursor = 0;
    state->fired_mask = 0U;
    state->active = 1;
    state->source_lp_low = 0;
    state->source_lp_mid = 0;
    state->source_lp_high = 0;
    state->source_lp_air = 0;
    state->rough_state = 0;

    for (index = 0; index < WMAG89_MODES; ++index) {
        state->modes[index].amplitude_q15 = 0;
        state->modes[index].phase = state->rng & 65535U;
    }

    if (action == WMAG89_ACTION_INSERT) {
        wmag89_copy_events(state, wmag89_insert_events,
            (int)(sizeof(wmag89_insert_events) / sizeof(wmag89_insert_events[0])));
    } else if (action == WMAG89_ACTION_REMOVE) {
        wmag89_copy_events(state, wmag89_remove_events,
            (int)(sizeof(wmag89_remove_events) / sizeof(wmag89_remove_events[0])));
    } else if (action == WMAG89_ACTION_SEAT_TAP) {
        wmag89_copy_events(state, wmag89_seat_events,
            (int)(sizeof(wmag89_seat_events) / sizeof(wmag89_seat_events[0])));
    } else if (action == WMAG89_ACTION_TUG_CHECK) {
        wmag89_copy_events(state, wmag89_tug_events,
            (int)(sizeof(wmag89_tug_events) / sizeof(wmag89_tug_events[0])));
    } else {
        wmag89_copy_events(state, wmag89_rattle_events,
            (int)(sizeof(wmag89_rattle_events) / sizeof(wmag89_rattle_events[0])));
    }

    end_ms = 0;
    for (index = 0; index < state->event_count; ++index) {
        event_end = state->events[index].start_ms +
                    state->events[index].duration_ms;
        if (event_end > end_ms) {
            end_ms = event_end;
        }
    }
    state->source_finished_sample =
        wmag89_ms_to_samples(state, end_ms + 85);
    return WMAG89_OK;
}

int wmag89_trigger_custom(WMag89State *state, const WMag89Event *events,
                          int event_count, int velocity_q15)
{
    int result;
    int index;
    int end_ms;
    int event_end;

    if (state == (WMag89State *)0 || events == (const WMag89Event *)0) {
        return WMAG89_ERR_NULL;
    }
    if (event_count < 1 || event_count > WMAG89_MAX_EVENTS) {
        return WMAG89_ERR_RANGE;
    }

    result = wmag89_trigger(state, WMAG89_ACTION_INSERT, velocity_q15);
    if (result != WMAG89_OK) {
        return result;
    }
    state->action = -1;
    wmag89_copy_events(state, events, event_count);

    end_ms = 0;
    for (index = 0; index < state->event_count; ++index) {
        event_end = state->events[index].start_ms +
                    state->events[index].duration_ms;
        if (event_end > end_ms) {
            end_ms = event_end;
        }
    }
    state->source_finished_sample =
        wmag89_ms_to_samples(state, end_ms + 85);
    return WMAG89_OK;
}

int wmag89_process_sample(WMag89State *state)
{
    int source;
    int modes;
    int mixed;
    int output;

    if (state == (WMag89State *)0) {
        return 0;
    }

    source = 0;
    if (state->active != 0) {
        source = wmag89_render_source(state);
    }
    modes = wmag89_render_modes(state);
    mixed = source + modes;
    mixed = wmag89_clamp_int(mixed, -65535, 65535);

    output = wmag89_process_distortion(state, mixed);
    output = wmag89_process_eq(state, output);
    output = wmag89_process_chorus(state, output);
    output = wmag89_process_reverb(state, output);
    output = wmag89_q15_mul(output, state->output_gain_q15);
    output = wmag89_clamp_int(output, -32768, 32767);

    if (state->active != 0) {
        ++state->sample_cursor;
        if (state->sample_cursor >= state->source_finished_sample) {
            state->active = 0;
        }
    }

    return output;
}

void wmag89_process_block(WMag89State *state, short *output, int frames)
{
    int index;

    if (state == (WMag89State *)0 || output == (short *)0 || frames <= 0) {
        return;
    }
    for (index = 0; index < frames; ++index) {
        output[index] = (short)wmag89_process_sample(state);
    }
}

int wmag89_is_active(const WMag89State *state)
{
    if (state == (const WMag89State *)0) {
        return 0;
    }
    return state->active;
}

const char *wmag89_preset_name(int preset)
{
    static const char * const names[WMAG89_PRESET_COUNT] = {
        "pistol_metal",
        "pistol_polymer",
        "smg_steel",
        "rifle_aluminum",
        "rifle_polymer",
        "sniper_box",
        "drum_heavy"
    };
    if (preset < 0 || preset >= WMAG89_PRESET_COUNT) {
        return "invalid";
    }
    return names[preset];
}

const char *wmag89_action_name(int action)
{
    static const char * const names[WMAG89_ACTION_COUNT] = {
        "insert",
        "remove",
        "seat_tap",
        "tug_check",
        "rattle"
    };
    if (action < 0 || action >= WMAG89_ACTION_COUNT) {
        return "invalid";
    }
    return names[action];
}
