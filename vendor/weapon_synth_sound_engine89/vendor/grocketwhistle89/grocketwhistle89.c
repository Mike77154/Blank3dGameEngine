#include "grocketwhistle89.h"

static const gwh89_s16 gwh89_sine_lut[257] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602, 6393, 7179, 7962, 8739,
    9512, 10278, 11039, 11793, 12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
    18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594, 23170, 23731, 24279, 24811,
    25329, 25832, 26319, 26790, 27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521,
    32609, 32678, 32728, 32757, 32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285,
    32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571, 30273, 29956, 29621, 29268,
    28898, 28510, 28105, 27683, 27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
    23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868, 18204, 17530, 16846, 16151,
    15446, 14732, 14010, 13279, 12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179,
    6393, 5602, 4808, 4011, 3212, 2410, 1608, 804, 0, -804, -1608, -2410,
    -3212, -4011, -4808, -5602, -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
    -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530, -18204, -18868, -19519, -20159,
    -20787, -21403, -22005, -22594, -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790,
    -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956, -30273, -30571, -30852, -31113,
    -31356, -31580, -31785, -31971, -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
    -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285, -32137, -31971, -31785, -31580,
    -31356, -31113, -30852, -30571, -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683,
    -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731, -23170, -22594, -22005, -21403,
    -20787, -20159, -19519, -18868, -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    -12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179, -6393, -5602, -4808, -4011,
    -3212, -2410, -1608, -804, 0
};

static const gwh89_s32 gwh89_eq_crossovers_hz[GWH89_EQ_CROSSOVERS] = {
    180, 420, 900, 1800, 3600
};

static const char *gwh89_eq_band_names[GWH89_EQ_BANDS] = {
    "sub <180 Hz",
    "body 180-420 Hz",
    "low whistle 420-900 Hz",
    "fundamental 900-1800 Hz",
    "edge 1800-3600 Hz",
    "air >3600 Hz"
};

static const gwh89_s32
gwh89_output_eq_crossovers_hz[GWH89_OUTPUT_EQ_CROSSOVERS] = {
    120, 300, 900, 2400, 6000
};

static const char *gwh89_output_eq_band_names[GWH89_OUTPUT_EQ_BANDS] = {
    "rumble <120 Hz",
    "low body 120-300 Hz",
    "core 300-900 Hz",
    "whistle 900-2400 Hz",
    "edge 2400-6000 Hz",
    "air >6000 Hz"
};

static const gwh89_preset gwh89_presets[GWH89_PRESET_COUNT] = {
    {
        "RPG7 sustained airy",
        760, 1680, 1180, 310,
        38, 170, 150, 1120, 280,
        22800, 14200, 24400,
        18, 78, 30000, 7600,
        5400, 260, 730, 150,
        6500, 2700, 4300, 260, 1150, 850, 6200, 13, 31,
        410, 430, 670, 52, 7600,
        420, 15600,
        {21000, 27000, 34000, 39000, 37200, 31500},
        1, 2400, 17400, 7600, 220,
        {2000, 11000, 23500, 33500, 38500, 36500}
    },
    {
        "SMAW short airy rocket",
        980, 2050, 1420, 390,
        24, 105, 90, 620, 220,
        23000, 15800, 24600,
        10, 52, 29600, 9000,
        6100, 300, 910, 180,
        7600, 3300, 5200, 340, 1400, 980, 7200, 11, 27,
        520, 390, 625, 46, 7200,
        470, 15300,
        {19000, 25500, 33000, 40000, 38500, 33000},
        1, 2100, 16600, 7900, 180,
        {1500, 9500, 22000, 33000, 40000, 38000}
    },
    {
        "Javelin soft two-stage airy",
        570, 1390, 1040, 260,
        72, 250, 210, 1650, 410,
        21600, 12000, 24200,
        92, 145, 30400, 6600,
        4300, 190, 510, 120,
        5000, 1800, 3200, 180, 850, 720, 5200, 17, 37,
        330, 520, 760, 58, 8200,
        350, 16300,
        {20500, 27500, 35000, 38500, 36000, 30000},
        1, 2700, 18000, 7200, 270,
        {3000, 13000, 25500, 34000, 37000, 33000}
    },
    {
        "AT4 fast airy flight",
        1120, 2380, 1540, 470,
        18, 72, 54, 410, 180,
        23500, 13600, 24800,
        5, 32, 29200, 10200,
        6800, 270, 1120, 135,
        8500, 4200, 6200, 420, 1550, 1100, 7800, 9, 23,
        650, 330, 570, 38, 6800,
        500, 14800,
        {17500, 24500, 32500, 41000, 40000, 34500},
        1, 1800, 15800, 8200, 160,
        {1200, 8500, 20500, 33000, 42000, 39000}
    },
    {
        "Guided missile airy flyby",
        520, 1840, 1360, 220,
        60, 420, 260, 2200, 540,
        22000, 14200, 24400,
        48, 135, 30600, 6500,
        3900, 220, 420, 165,
        7200, 3100, 5000, 300, 1200, 820, 7200, 15, 35,
        280, 560, 840, 66, 9000,
        390, 16000,
        {18500, 25500, 34000, 40500, 39500, 34000},
        1, 2900, 18400, 7000, 300,
        {1800, 10500, 22500, 33500, 41000, 39000}
    },
    {
        "Heavy rocket airy low whistle",
        330, 940, 720, 145,
        48, 260, 210, 1450, 450,
        23800, 16300, 24000,
        25, 105, 31000, 5300,
        3200, 340, 610, 200,
        4500, 1600, 3000, 180, 800, 650, 4700, 19, 41,
        250, 650, 940, 74, 9000,
        520, 14600,
        {25000, 34500, 41000, 36500, 32000, 27500},
        1, 3200, 18800, 6600, 330,
        {5000, 16000, 28500, 34000, 36000, 32000}
    },
    {
        "Mexican chifladora air reference",
        2500, 6200, 4750, 900,
        12, 180, 220, 1300, 430,
        19000, 16600, 17600,
        0, 28, 28200, 12200,
        6800, 460, 1300, 250,
        11800, 6700, 9200, 620, 2300, 1350, 10400, 7, 21,
        620, 300, 520, 35, 9200,
        460, 14800,
        {4000, 9000, 17000, 30000, 45000, 47000},
        1, 3200, 18400, 8800, 370,
        {800, 4000, 11000, 25000, 42000, 44000}
    }
};

static gwh89_s32 gwh89_clamp_s32(
    gwh89_s32 value,
    gwh89_s32 minimum,
    gwh89_s32 maximum
)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static gwh89_s16 gwh89_clamp_s16(gwh89_s32 value)
{
    if (value > 32767) {
        value = 32767;
    } else if (value < -32768) {
        value = -32768;
    }
    return (gwh89_s16)value;
}

static gwh89_s32 gwh89_abs_s32(gwh89_s32 value)
{
    if (value < 0) {
        return -value;
    }
    return value;
}

static gwh89_s32 gwh89_ms_to_samples(
    gwh89_s32 milliseconds,
    gwh89_s32 sample_rate
)
{
    if (milliseconds <= 0 || sample_rate <= 0) {
        return 0;
    }
    if (milliseconds > 30000) {
        milliseconds = 30000;
    }
    return (milliseconds * sample_rate) / 1000;
}

static gwh89_s32 gwh89_mul_q15(gwh89_s32 a, gwh89_s32 b)
{
    return (a * b) >> 15;
}

static gwh89_s32 gwh89_lerp_q15(
    gwh89_s32 a,
    gwh89_s32 b,
    gwh89_s32 t_q15
)
{
    gwh89_s32 delta;
    delta = b - a;
    return a + ((delta * t_q15) >> 15);
}

static gwh89_s32 gwh89_ratio_q15(
    gwh89_s32 numerator,
    gwh89_s32 denominator
)
{
    gwh89_s32 result;
    gwh89_s32 remainder;
    gwh89_s32 i;
    if (denominator <= 0) {
        return GWH89_Q15_ONE;
    }
    if (numerator <= 0) {
        return 0;
    }
    if (numerator >= denominator) {
        return GWH89_Q15_ONE;
    }

    result = 0;
    remainder = numerator;
    for (i = 0; i < 15; ++i) {
        remainder <<= 1;
        result <<= 1;
        if (remainder >= denominator) {
            remainder -= denominator;
            result |= 1;
        }
    }
    return gwh89_clamp_s32(result, 0, GWH89_Q15_ONE);
}

static gwh89_s32 gwh89_triangle(gwh89_u32 phase)
{
    gwh89_s32 p;
    p = (gwh89_s32)(phase & 65535U);
    if (p < 32768) {
        return (p << 1) - 32768;
    }
    return 98303 - (p << 1);
}

static gwh89_u32 gwh89_lfo_increment(
    gwh89_s32 rate_millihz,
    gwh89_s32 sample_rate
)
{
    gwh89_s32 denominator;
    gwh89_s32 increment;
    if (rate_millihz <= 0 || sample_rate <= 0) {
        return 0U;
    }
    denominator = sample_rate * 1000;
    increment = (rate_millihz * 65536) / denominator;
    if (increment < 1) {
        increment = 1;
    }
    return (gwh89_u32)increment;
}

static gwh89_u32 gwh89_tone_increment(
    gwh89_s32 frequency_hz,
    gwh89_s32 sample_rate
)
{
    gwh89_s32 increment;
    if (frequency_hz <= 0 || sample_rate <= 0) {
        return 0U;
    }
    frequency_hz = gwh89_clamp_s32(frequency_hz, 1, sample_rate / 3);
    increment = (frequency_hz * 65536) / sample_rate;
    if (increment < 1) {
        increment = 1;
    }
    return (gwh89_u32)increment;
}

static gwh89_s32 gwh89_sine(gwh89_u32 phase)
{
    gwh89_s32 index;
    gwh89_s32 fraction;
    gwh89_s32 a;
    gwh89_s32 b;
    index = (gwh89_s32)((phase >> 8) & 255U);
    fraction = (gwh89_s32)(phase & 255U);
    a = (gwh89_s32)gwh89_sine_lut[index];
    b = (gwh89_s32)gwh89_sine_lut[index + 1];
    return a + (((b - a) * fraction) >> 8);
}

static gwh89_u32 gwh89_xorshift32(gwh89_u32 *state)
{
    gwh89_u32 x;
    x = *state;
    if (x == 0U) {
        x = 0x6D2B79F5U;
    }
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static gwh89_s32 gwh89_noise_sample(gwh89_state *state)
{
    gwh89_u32 value;
    value = gwh89_xorshift32(&state->noise_state);
    return (gwh89_s32)((value >> 16) & 65535U) - 32768;
}

static gwh89_s32 gwh89_highpass(
    gwh89_s32 input,
    gwh89_s32 coeff_q15,
    gwh89_s32 *previous_input,
    gwh89_s32 *previous_output
)
{
    gwh89_s32 value;
    value = *previous_output + input - *previous_input;
    value = gwh89_clamp_s32(value, -65535, 65535);
    value = gwh89_mul_q15(value, coeff_q15);
    *previous_input = input;
    *previous_output = value;
    return value;
}

static gwh89_s32 gwh89_lowpass(
    gwh89_s32 input,
    gwh89_s32 coeff_q15,
    gwh89_s32 *previous_output
)
{
    gwh89_s32 delta;
    delta = input - *previous_output;
    *previous_output += gwh89_mul_q15(delta, coeff_q15);
    return *previous_output;
}

static gwh89_s32 gwh89_filter_air(
    gwh89_state *state,
    gwh89_s32 input
)
{
    gwh89_s32 value;
    const gwh89_preset *preset;
    preset = state->preset;
    value = gwh89_highpass(
        input,
        preset->air_hp_coeff_q15,
        &state->noise_prev_x_1,
        &state->noise_hp_y_1
    );
    value = gwh89_highpass(
        value,
        preset->air_hp_coeff_q15,
        &state->noise_prev_x_2,
        &state->noise_hp_y_2
    );
    value = gwh89_lowpass(
        value,
        preset->air_lp_coeff_q15,
        &state->noise_lp_y_1
    );
    value = gwh89_lowpass(
        value,
        preset->air_lp_coeff_q15,
        &state->noise_lp_y_2
    );
    return value;
}

static gwh89_s32 gwh89_soft_clip(
    gwh89_s32 input,
    gwh89_s32 drive_q8,
    gwh89_s32 knee
)
{
    gwh89_s32 driven;
    gwh89_s32 magnitude;
    gwh89_s32 sign;
    driven = (input * drive_q8) >> 8;
    sign = 1;
    if (driven < 0) {
        sign = -1;
        magnitude = -driven;
    } else {
        magnitude = driven;
    }
    knee = gwh89_clamp_s32(knee, 4096, 30000);
    if (magnitude > knee) {
        magnitude = knee + ((magnitude - knee) >> 2);
    }
    if (magnitude > 32767) {
        magnitude = 32767;
    }
    return magnitude * sign;
}

static gwh89_s32 gwh89_mul_gain_q15(
    gwh89_s32 value,
    gwh89_s32 gain_q15
)
{
    value = gwh89_clamp_s32(value, -65535, 65535);
    gain_q15 = gwh89_clamp_s32(gain_q15, 0, 65535);
    return ((value >> 1) * gain_q15) >> 14;
}

static void gwh89_update_filter_coefficients(
    gwh89_state *state,
    const gwh89_s32 *crossovers_hz,
    gwh89_s32 crossover_count,
    gwh89_s32 *coeff_q15
)
{
    gwh89_s32 i;
    gwh89_s32 cutoff;
    gwh89_s32 angular;
    gwh89_s32 maximum;
    if (state == 0 || crossovers_hz == 0 || coeff_q15 == 0) {
        return;
    }
    maximum = state->sample_rate / 3;
    for (i = 0; i < crossover_count; ++i) {
        cutoff = crossovers_hz[i];
        if (cutoff > maximum) {
            cutoff = maximum;
        }
        angular = (cutoff * 201) >> 5;
        coeff_q15[i] = gwh89_ratio_q15(
            angular,
            state->sample_rate + angular
        );
        if (coeff_q15[i] < 1) {
            coeff_q15[i] = 1;
        }
    }
}

static void gwh89_update_eq_coefficients(gwh89_state *state)
{
    gwh89_update_filter_coefficients(
        state,
        gwh89_eq_crossovers_hz,
        GWH89_EQ_CROSSOVERS,
        state->eq_coeff_q15
    );
}

static void gwh89_update_output_eq_coefficients(gwh89_state *state)
{
    gwh89_update_filter_coefficients(
        state,
        gwh89_output_eq_crossovers_hz,
        GWH89_OUTPUT_EQ_CROSSOVERS,
        state->output_eq_coeff_q15
    );
}

static void gwh89_clear_eq_memory(gwh89_state *state)
{
    gwh89_s32 i;
    for (i = 0; i < GWH89_EQ_CROSSOVERS; ++i) {
        state->eq_lp_left[i] = 0;
        state->eq_lp_right[i] = 0;
    }
}

static void gwh89_clear_output_eq_memory(gwh89_state *state)
{
    gwh89_s32 i;
    for (i = 0; i < GWH89_OUTPUT_EQ_CROSSOVERS; ++i) {
        state->output_eq_lp_left[i] = 0;
        state->output_eq_lp_right[i] = 0;
    }
}

static gwh89_s32 gwh89_apply_six_band_eq(
    gwh89_s32 input,
    gwh89_s32 enabled,
    const gwh89_s32 *coeff_q15,
    const gwh89_s32 *gain_q15,
    gwh89_s32 *lowpass_state
)
{
    gwh89_s32 band[GWH89_EQ_BANDS];
    gwh89_s32 output;
    gwh89_s32 i;
    if (!enabled) {
        return input;
    }

    for (i = 0; i < GWH89_EQ_CROSSOVERS; ++i) {
        lowpass_state[i] = gwh89_lowpass(
            input,
            coeff_q15[i],
            &lowpass_state[i]
        );
    }

    band[0] = lowpass_state[0];
    for (i = 1; i < GWH89_EQ_CROSSOVERS; ++i) {
        band[i] = lowpass_state[i] - lowpass_state[i - 1];
    }
    band[GWH89_EQ_BANDS - 1] =
        input - lowpass_state[GWH89_EQ_CROSSOVERS - 1];

    output = 0;
    for (i = 0; i < GWH89_EQ_BANDS; ++i) {
        output += gwh89_mul_gain_q15(band[i], gain_q15[i]);
    }
    return gwh89_clamp_s32(output, -131072, 131071);
}

static gwh89_s32 gwh89_apply_eq_channel(
    gwh89_state *state,
    gwh89_s32 input,
    gwh89_s32 *lowpass_state
)
{
    return gwh89_apply_six_band_eq(
        input,
        state->eq_enabled,
        state->eq_coeff_q15,
        state->eq_gain_q15,
        lowpass_state
    );
}

static gwh89_s32 gwh89_apply_output_eq_channel(
    gwh89_state *state,
    gwh89_s32 input,
    gwh89_s32 *lowpass_state
)
{
    return gwh89_apply_six_band_eq(
        input,
        state->output_eq_enabled,
        state->output_eq_coeff_q15,
        state->output_eq_gain_q15,
        lowpass_state
    );
}

static void gwh89_clear_reverb_memory(gwh89_state *state)
{
    gwh89_s32 i;
    for (i = 0; i < GWH89_REVERB_COMB1_BUFFER; ++i) {
        state->reverb_comb1[i] = 0;
    }
    for (i = 0; i < GWH89_REVERB_COMB2_BUFFER; ++i) {
        state->reverb_comb2[i] = 0;
    }
    for (i = 0; i < GWH89_REVERB_COMB3_BUFFER; ++i) {
        state->reverb_comb3[i] = 0;
    }
    state->reverb_comb1_index = 0;
    state->reverb_comb2_index = 0;
    state->reverb_comb3_index = 0;
    state->reverb_comb1_damp = 0;
    state->reverb_comb2_damp = 0;
    state->reverb_comb3_damp = 0;
    state->reverb_tail_age_samples = 0;
}

static void gwh89_update_reverb_lengths(gwh89_state *state)
{
    state->reverb_comb1_length = gwh89_ms_to_samples(
        23,
        state->sample_rate
    );
    state->reverb_comb2_length = gwh89_ms_to_samples(
        31,
        state->sample_rate
    );
    state->reverb_comb3_length = gwh89_ms_to_samples(
        43,
        state->sample_rate
    );
    state->reverb_comb1_length = gwh89_clamp_s32(
        state->reverb_comb1_length,
        31,
        GWH89_REVERB_COMB1_BUFFER - 1
    );
    state->reverb_comb2_length = gwh89_clamp_s32(
        state->reverb_comb2_length,
        37,
        GWH89_REVERB_COMB2_BUFFER - 1
    );
    state->reverb_comb3_length = gwh89_clamp_s32(
        state->reverb_comb3_length,
        43,
        GWH89_REVERB_COMB3_BUFFER - 1
    );
}

static gwh89_s32 gwh89_process_reverb_comb(
    gwh89_s16 *buffer,
    gwh89_s32 length,
    gwh89_s32 *index,
    gwh89_s32 *damp_state,
    gwh89_s32 input,
    gwh89_s32 feedback_q15,
    gwh89_s32 damping_q15
)
{
    gwh89_s32 delayed;
    gwh89_s32 filtered;
    gwh89_s32 write_value;
    delayed = buffer[*index];
    filtered = *damp_state;
    filtered += gwh89_mul_q15(delayed - filtered, damping_q15);
    *damp_state = filtered;
    write_value = input + gwh89_mul_q15(filtered, feedback_q15);
    buffer[*index] = gwh89_clamp_s16(write_value);
    *index += 1;
    if (*index >= length) {
        *index = 0;
    }
    return delayed;
}

static void gwh89_process_reverb(
    gwh89_state *state,
    gwh89_s32 input_left,
    gwh89_s32 input_right,
    gwh89_s32 *output_left,
    gwh89_s32 *output_right
)
{
    gwh89_s32 mono_input;
    gwh89_s32 comb1;
    gwh89_s32 comb2;
    gwh89_s32 comb3;
    gwh89_s32 wet_left;
    gwh89_s32 wet_right;
    gwh89_s32 dry_q15;

    if (!state->reverb_enabled || state->reverb_wet_q15 <= 0) {
        *output_left = input_left;
        *output_right = input_right;
        return;
    }

    mono_input = (input_left + input_right) >> 2;
    comb1 = gwh89_process_reverb_comb(
        state->reverb_comb1,
        state->reverb_comb1_length,
        &state->reverb_comb1_index,
        &state->reverb_comb1_damp,
        mono_input,
        state->reverb_feedback_q15,
        state->reverb_damping_q15
    );
    comb2 = gwh89_process_reverb_comb(
        state->reverb_comb2,
        state->reverb_comb2_length,
        &state->reverb_comb2_index,
        &state->reverb_comb2_damp,
        mono_input,
        state->reverb_feedback_q15 - 900,
        state->reverb_damping_q15 + 600
    );
    comb3 = gwh89_process_reverb_comb(
        state->reverb_comb3,
        state->reverb_comb3_length,
        &state->reverb_comb3_index,
        &state->reverb_comb3_damp,
        mono_input,
        state->reverb_feedback_q15 - 1700,
        state->reverb_damping_q15 + 1100
    );

    wet_left = (comb1 + comb2 - comb3) >> 1;
    wet_right = (comb1 - comb2 + comb3) >> 1;
    dry_q15 = GWH89_Q15_ONE - state->reverb_wet_q15;
    *output_left = gwh89_mul_q15(input_left, dry_q15);
    *output_left += gwh89_mul_q15(wet_left, state->reverb_wet_q15);
    *output_right = gwh89_mul_q15(input_right, dry_q15);
    *output_right += gwh89_mul_q15(wet_right, state->reverb_wet_q15);
}

static gwh89_s32 gwh89_get_macro_frequency(const gwh89_state *state)
{
    const gwh89_preset *preset;
    gwh89_s32 t;
    gwh89_s32 age;
    gwh89_s32 frequency;
    preset = state->preset;
    if (preset == 0) {
        return 0;
    }

    if (state->stage == GWH89_STAGE_RELEASE) {
        t = gwh89_ratio_q15(
            state->release_age_samples,
            state->release_samples
        );
        return gwh89_lerp_q15(
            state->release_start_freq_hz,
            preset->end_freq_hz,
            t
        );
    }

    age = state->age_samples;
    if (age < state->rise_samples) {
        t = gwh89_ratio_q15(age, state->rise_samples);
        frequency = gwh89_lerp_q15(
            preset->start_freq_hz,
            preset->peak_freq_hz,
            t
        );
    } else if (age < state->rise_samples + state->settle_samples) {
        t = gwh89_ratio_q15(
            age - state->rise_samples,
            state->settle_samples
        );
        frequency = gwh89_lerp_q15(
            preset->peak_freq_hz,
            preset->sustain_freq_hz,
            t
        );
    } else {
        frequency = preset->sustain_freq_hz;
    }
    return frequency;
}

static gwh89_s32 gwh89_apply_modulation(
    const gwh89_state *state,
    gwh89_s32 base_frequency
)
{
    const gwh89_preset *preset;
    gwh89_s32 vibrato;
    gwh89_s32 drift;
    gwh89_s32 modulation_q15;
    gwh89_s32 frequency;
    gwh89_s32 denominator;
    preset = state->preset;

    vibrato = gwh89_triangle(state->vibrato_phase);
    vibrato = gwh89_mul_q15(vibrato, preset->vibrato_depth_q15);

    drift = gwh89_triangle(state->drift_phase);
    drift = gwh89_mul_q15(drift, preset->drift_depth_q15);

    modulation_q15 = vibrato + drift;
    modulation_q15 = gwh89_clamp_s32(modulation_q15, -8192, 8192);

    frequency = base_frequency;
    frequency += gwh89_mul_q15(frequency, modulation_q15);
    frequency = gwh89_mul_q15(frequency, state->pitch_scale_q15);

    denominator = 343 + state->radial_velocity_mps;
    denominator = gwh89_clamp_s32(denominator, 93, 593);
    frequency = (frequency * 343) / denominator;

    frequency = gwh89_clamp_s32(
        frequency,
        20,
        (state->sample_rate / 3)
    );
    return frequency;
}

static gwh89_s32 gwh89_get_amplitude_envelope(const gwh89_state *state)
{
    gwh89_s32 envelope;
    if (state->stage == GWH89_STAGE_RELEASE) {
        envelope = GWH89_Q15_ONE - gwh89_ratio_q15(
            state->release_age_samples,
            state->release_samples
        );
        return gwh89_clamp_s32(envelope, 0, GWH89_Q15_ONE);
    }
    if (state->age_samples < state->attack_samples) {
        return gwh89_ratio_q15(
            state->age_samples,
            state->attack_samples
        );
    }
    return GWH89_Q15_ONE;
}

static gwh89_s32 gwh89_get_air_envelope(const gwh89_state *state)
{
    gwh89_s32 local_age;
    gwh89_s32 base_envelope;
    base_envelope = gwh89_get_amplitude_envelope(state);
    if (state->age_samples < state->air_delay_samples) {
        return 0;
    }
    local_age = state->age_samples - state->air_delay_samples;
    if (local_age < state->air_attack_samples) {
        return gwh89_mul_q15(
            base_envelope,
            gwh89_ratio_q15(local_age, state->air_attack_samples)
        );
    }
    return base_envelope;
}

static void gwh89_clear_chorus(gwh89_state *state)
{
    gwh89_s32 i;
    for (i = 0; i < GWH89_CHORUS_BUFFER_SAMPLES; ++i) {
        state->chorus_buffer[i] = 0;
    }
    state->chorus_write_index = 0;
}

static void gwh89_clear_air_buffer(gwh89_state *state)
{
    gwh89_s32 i;
    for (i = 0; i < GWH89_AIR_BUFFER_SAMPLES; ++i) {
        state->air_buffer[i] = 0;
    }
    state->air_write_index = 0;
}

static void gwh89_advance_time(gwh89_state *state)
{
    gwh89_s32 auto_release_point;
    if (state->stage == GWH89_STAGE_ACTIVE) {
        state->age_samples += 1;
        auto_release_point = state->rise_samples;
        auto_release_point += state->settle_samples;
        auto_release_point += state->auto_hold_samples;
        if (state->auto_hold_samples > 0 &&
            state->age_samples >= auto_release_point) {
            gwh89_release(state);
        }
    } else if (state->stage == GWH89_STAGE_RELEASE) {
        state->release_age_samples += 1;
        if (state->release_age_samples >= state->release_samples) {
            if (state->reverb_enabled &&
                state->reverb_wet_q15 > 0 &&
                state->reverb_tail_samples > 0) {
                state->stage = GWH89_STAGE_REVERB_TAIL;
                state->reverb_tail_age_samples = 0;
            } else {
                state->stage = GWH89_STAGE_DONE;
            }
        }
    } else if (state->stage == GWH89_STAGE_REVERB_TAIL) {
        state->reverb_tail_age_samples += 1;
        if (state->reverb_tail_age_samples >=
            state->reverb_tail_samples) {
            state->stage = GWH89_STAGE_DONE;
        }
    }
}

static void gwh89_render_stereo_sample_internal(
    gwh89_state *state,
    gwh89_s16 *left,
    gwh89_s16 *right
)
{
    const gwh89_preset *preset;
    gwh89_s32 frequency;
    gwh89_s32 tone;
    gwh89_s32 tone_base;
    gwh89_s32 harmonic2;
    gwh89_s32 harmonic3;
    gwh89_s32 air;
    gwh89_s32 aerated_tone;
    gwh89_s32 raw_noise;
    gwh89_s32 rough_pitch;
    gwh89_s32 amplitude_mod_q15;
    gwh89_s32 mixed;
    gwh89_s32 amp_env;
    gwh89_s32 air_env;
    gwh89_s32 chorus_triangle;
    gwh89_s32 modulation_samples;
    gwh89_s32 delay_left;
    gwh89_s32 delay_right;
    gwh89_s32 read_left;
    gwh89_s32 read_right;
    gwh89_s32 wet_left;
    gwh89_s32 wet_right;
    gwh89_s32 dry_mix;
    gwh89_s32 wet_mix;
    gwh89_s32 output_left;
    gwh89_s32 output_right;
    gwh89_s32 air_delay_left;
    gwh89_s32 air_delay_right;
    gwh89_s32 air_read_left;
    gwh89_s32 air_read_right;
    gwh89_s32 air_spatial_left;
    gwh89_s32 air_spatial_right;
    gwh89_s32 eq_left;
    gwh89_s32 eq_right;
    gwh89_s32 reverb_left;
    gwh89_s32 reverb_right;
    gwh89_s32 final_left;
    gwh89_s32 final_right;
    gwh89_s32 source_active;
    gwh89_u32 increment;
    gwh89_u32 harmonic2_increment;
    gwh89_u32 harmonic3_increment;

    if (state == 0 || left == 0 || right == 0) {
        return;
    }
    if (state->stage == GWH89_STAGE_IDLE ||
        state->stage == GWH89_STAGE_DONE ||
        state->preset == 0) {
        *left = 0;
        *right = 0;
        return;
    }

    preset = state->preset;
    source_active = state->stage == GWH89_STAGE_ACTIVE ||
                    state->stage == GWH89_STAGE_RELEASE;
    output_left = 0;
    output_right = 0;

    if (source_active) {
        amp_env = gwh89_get_amplitude_envelope(state);
        air_env = gwh89_get_air_envelope(state);
        raw_noise = gwh89_noise_sample(state);

        state->roughness_value += gwh89_mul_q15(
            raw_noise - state->roughness_value,
            preset->roughness_slew_q15
        );

        frequency = gwh89_get_macro_frequency(state);
        frequency = gwh89_apply_modulation(state, frequency);
        rough_pitch = gwh89_mul_q15(
            state->roughness_value,
            preset->pitch_roughness_q15
        );
        frequency += gwh89_mul_q15(frequency, rough_pitch);
        frequency = gwh89_clamp_s32(
            frequency,
            20,
            state->sample_rate / 3
        );

        increment = gwh89_tone_increment(frequency, state->sample_rate);
        harmonic2_increment = gwh89_tone_increment(
            frequency * 2,
            state->sample_rate
        );
        harmonic3_increment = gwh89_tone_increment(
            frequency * 3,
            state->sample_rate
        );

        tone_base = gwh89_sine(state->tone_phase);
        harmonic2 = gwh89_sine(state->harmonic2_phase);
        harmonic3 = gwh89_sine(state->harmonic3_phase);
        state->tone_phase = (state->tone_phase + increment) & 65535U;
        state->harmonic2_phase = (
            state->harmonic2_phase + harmonic2_increment
        ) & 65535U;
        state->harmonic3_phase = (
            state->harmonic3_phase + harmonic3_increment
        ) & 65535U;

        tone = gwh89_mul_q15(tone_base, preset->tone_gain_q15);
        harmonic2 = gwh89_mul_q15(
            harmonic2,
            preset->harmonic2_gain_q15
        );
        harmonic3 = gwh89_mul_q15(
            harmonic3,
            preset->harmonic3_gain_q15
        );
        tone += harmonic2 + harmonic3;

        amplitude_mod_q15 = GWH89_Q15_ONE;
        amplitude_mod_q15 += gwh89_mul_q15(
            state->roughness_value,
            preset->amplitude_roughness_q15
        );
        amplitude_mod_q15 = gwh89_clamp_s32(
            amplitude_mod_q15,
            24576,
            40959
        );
        tone = gwh89_mul_q15(tone, amplitude_mod_q15);
        tone = gwh89_mul_q15(tone, amp_env);

        air = gwh89_filter_air(state, raw_noise);
        air = gwh89_mul_q15(air, preset->air_gain_q15);
        air = gwh89_mul_q15(air, air_env);

        aerated_tone = gwh89_mul_q15(
            air,
            gwh89_abs_s32(tone_base)
        );
        aerated_tone = gwh89_mul_q15(
            aerated_tone,
            preset->aerated_tone_gain_q15
        );

        mixed = tone + air + aerated_tone;
        mixed = gwh89_soft_clip(
            mixed,
            preset->drive_q8,
            preset->soft_clip_knee
        );
        mixed = gwh89_mul_q15(mixed, preset->master_gain_q15);
        mixed = gwh89_mul_q15(mixed, state->distance_gain_q15);
        mixed = gwh89_clamp_s16(mixed);

        state->chorus_buffer[state->chorus_write_index] =
            (gwh89_s16)mixed;
        state->air_buffer[state->air_write_index] =
            gwh89_clamp_s16(air);

        chorus_triangle = gwh89_triangle(state->chorus_phase);
        modulation_samples = (
            chorus_triangle * preset->chorus_depth_samples
        ) >> 15;

        delay_left = preset->chorus_delay_left_samples +
                     modulation_samples;
        delay_right = preset->chorus_delay_right_samples -
                      modulation_samples;
        delay_left = gwh89_clamp_s32(
            delay_left,
            1,
            GWH89_CHORUS_BUFFER_SAMPLES - 1
        );
        delay_right = gwh89_clamp_s32(
            delay_right,
            1,
            GWH89_CHORUS_BUFFER_SAMPLES - 1
        );

        read_left = (
            state->chorus_write_index - delay_left
        ) & GWH89_CHORUS_BUFFER_MASK;
        read_right = (
            state->chorus_write_index - delay_right
        ) & GWH89_CHORUS_BUFFER_MASK;

        wet_left = state->chorus_buffer[read_left];
        wet_right = state->chorus_buffer[read_right];

        wet_mix = preset->chorus_mix_q15;
        dry_mix = GWH89_Q15_ONE - wet_mix;

        output_left = gwh89_mul_q15(mixed, dry_mix);
        output_left += gwh89_mul_q15(wet_left, wet_mix);
        output_right = gwh89_mul_q15(mixed, dry_mix);
        output_right += gwh89_mul_q15(wet_right, wet_mix);

        modulation_samples = (chorus_triangle * 3) >> 15;
        air_delay_left = preset->air_delay_left_samples +
                         modulation_samples;
        air_delay_right = preset->air_delay_right_samples -
                          modulation_samples;
        air_delay_left = gwh89_clamp_s32(
            air_delay_left,
            1,
            GWH89_AIR_BUFFER_SAMPLES - 1
        );
        air_delay_right = gwh89_clamp_s32(
            air_delay_right,
            1,
            GWH89_AIR_BUFFER_SAMPLES - 1
        );
        air_read_left = (
            state->air_write_index - air_delay_left
        ) & GWH89_AIR_BUFFER_MASK;
        air_read_right = (
            state->air_write_index - air_delay_right
        ) & GWH89_AIR_BUFFER_MASK;
        air_spatial_left = state->air_buffer[air_read_left];
        air_spatial_right = state->air_buffer[air_read_right];
        output_left += gwh89_mul_q15(
            air_spatial_left,
            preset->air_spatial_mix_q15
        );
        output_right += gwh89_mul_q15(
            air_spatial_right,
            preset->air_spatial_mix_q15
        );

        state->chorus_write_index = (
            state->chorus_write_index + 1
        ) & GWH89_CHORUS_BUFFER_MASK;
        state->air_write_index = (
            state->air_write_index + 1
        ) & GWH89_AIR_BUFFER_MASK;

        state->vibrato_phase = (
            state->vibrato_phase +
            gwh89_lfo_increment(
                preset->vibrato_rate_millihz,
                state->sample_rate
            )
        ) & 65535U;

        state->drift_phase = (
            state->drift_phase +
            gwh89_lfo_increment(
                preset->drift_rate_millihz,
                state->sample_rate
            )
        ) & 65535U;

        state->chorus_phase = (
            state->chorus_phase +
            gwh89_lfo_increment(
                preset->chorus_rate_millihz,
                state->sample_rate
            )
        ) & 65535U;
    }

    eq_left = gwh89_apply_eq_channel(
        state,
        output_left,
        state->eq_lp_left
    );
    eq_right = gwh89_apply_eq_channel(
        state,
        output_right,
        state->eq_lp_right
    );
    eq_left = gwh89_clamp_s16(eq_left);
    eq_right = gwh89_clamp_s16(eq_right);

    gwh89_process_reverb(
        state,
        eq_left,
        eq_right,
        &reverb_left,
        &reverb_right
    );

    final_left = gwh89_apply_output_eq_channel(
        state,
        reverb_left,
        state->output_eq_lp_left
    );
    final_right = gwh89_apply_output_eq_channel(
        state,
        reverb_right,
        state->output_eq_lp_right
    );

    *left = gwh89_clamp_s16(final_left);
    *right = gwh89_clamp_s16(final_right);
    gwh89_advance_time(state);
}

const gwh89_preset *gwh89_get_preset(gwh89_s32 preset_id)
{
    if (preset_id < 0 || preset_id >= GWH89_PRESET_COUNT) {
        return 0;
    }
    return &gwh89_presets[preset_id];
}

const char *gwh89_get_preset_name(gwh89_s32 preset_id)
{
    const gwh89_preset *preset;
    preset = gwh89_get_preset(preset_id);
    if (preset == 0) {
        return "invalid";
    }
    return preset->name;
}

const char *gwh89_get_eq_band_name(gwh89_s32 band)
{
    if (band < 0 || band >= GWH89_EQ_BANDS) {
        return "invalid";
    }
    return gwh89_eq_band_names[band];
}

gwh89_s32 gwh89_get_eq_crossover_hz(gwh89_s32 crossover_index)
{
    if (crossover_index < 0 ||
        crossover_index >= GWH89_EQ_CROSSOVERS) {
        return 0;
    }
    return gwh89_eq_crossovers_hz[crossover_index];
}

const char *gwh89_get_output_eq_band_name(gwh89_s32 band)
{
    if (band < 0 || band >= GWH89_OUTPUT_EQ_BANDS) {
        return "invalid";
    }
    return gwh89_output_eq_band_names[band];
}

gwh89_s32 gwh89_get_output_eq_crossover_hz(
    gwh89_s32 crossover_index
)
{
    if (crossover_index < 0 ||
        crossover_index >= GWH89_OUTPUT_EQ_CROSSOVERS) {
        return 0;
    }
    return gwh89_output_eq_crossovers_hz[crossover_index];
}

void gwh89_init(gwh89_state *state, gwh89_s32 sample_rate, gwh89_u32 seed)
{
    if (state == 0) {
        return;
    }
    state->sample_rate = gwh89_clamp_s32(sample_rate, 8000, 96000);
    state->preset = 0;
    state->noise_state = seed;
    if (state->noise_state == 0U) {
        state->noise_state = 0x6D2B79F5U;
    }
    state->distance_gain_q15 = GWH89_Q15_ONE;
    state->pitch_scale_q15 = GWH89_Q15_ONE;
    state->radial_velocity_mps = 0;
    gwh89_reset(state);
}

void gwh89_clear_effect_memory(gwh89_state *state)
{
    if (state == 0) {
        return;
    }
    gwh89_clear_chorus(state);
    gwh89_clear_air_buffer(state);
    gwh89_clear_eq_memory(state);
    gwh89_clear_reverb_memory(state);
    gwh89_clear_output_eq_memory(state);
}

void gwh89_reset(gwh89_state *state)
{
    gwh89_s32 i;
    if (state == 0) {
        return;
    }
    state->preset = 0;
    state->tone_phase = 0U;
    state->harmonic2_phase = 8192U;
    state->harmonic3_phase = 24576U;
    state->vibrato_phase = 0U;
    state->drift_phase = 16384U;
    state->chorus_phase = 8192U;
    state->age_samples = 0;
    state->release_age_samples = 0;
    state->release_start_freq_hz = 0;
    state->stage = GWH89_STAGE_IDLE;
    state->attack_samples = 0;
    state->rise_samples = 0;
    state->settle_samples = 0;
    state->auto_hold_samples = 0;
    state->release_samples = 1;
    state->air_delay_samples = 0;
    state->air_attack_samples = 0;
    state->roughness_value = 0;
    state->noise_prev_x_1 = 0;
    state->noise_hp_y_1 = 0;
    state->noise_prev_x_2 = 0;
    state->noise_hp_y_2 = 0;
    state->noise_lp_y_1 = 0;
    state->noise_lp_y_2 = 0;

    state->eq_enabled = 1;
    for (i = 0; i < GWH89_EQ_BANDS; ++i) {
        state->eq_gain_q15[i] = GWH89_Q15_ONE;
    }
    gwh89_update_eq_coefficients(state);

    state->output_eq_enabled = 1;
    for (i = 0; i < GWH89_OUTPUT_EQ_BANDS; ++i) {
        state->output_eq_gain_q15[i] = GWH89_Q15_ONE;
    }
    gwh89_update_output_eq_coefficients(state);

    state->reverb_enabled = 0;
    state->reverb_wet_q15 = 0;
    state->reverb_feedback_q15 = 16000;
    state->reverb_damping_q15 = 7000;
    state->reverb_tail_samples = 0;
    state->reverb_tail_age_samples = 0;
    gwh89_update_reverb_lengths(state);
    gwh89_clear_effect_memory(state);
}

void gwh89_trigger(gwh89_state *state, const gwh89_preset *preset)
{
    gwh89_s32 i;
    if (state == 0 || preset == 0) {
        return;
    }
    state->preset = preset;
    state->tone_phase = 0U;
    state->harmonic2_phase = 8192U;
    state->harmonic3_phase = 24576U;
    state->vibrato_phase = 0U;
    state->drift_phase = 16384U;
    state->chorus_phase = 8192U;
    state->age_samples = 0;
    state->release_age_samples = 0;
    state->release_start_freq_hz = preset->sustain_freq_hz;
    state->stage = GWH89_STAGE_ACTIVE;

    state->attack_samples = gwh89_ms_to_samples(
        preset->attack_ms,
        state->sample_rate
    );
    state->rise_samples = gwh89_ms_to_samples(
        preset->rise_ms,
        state->sample_rate
    );
    state->settle_samples = gwh89_ms_to_samples(
        preset->settle_ms,
        state->sample_rate
    );
    state->auto_hold_samples = gwh89_ms_to_samples(
        preset->auto_hold_ms,
        state->sample_rate
    );
    state->release_samples = gwh89_ms_to_samples(
        preset->release_ms,
        state->sample_rate
    );
    state->air_delay_samples = gwh89_ms_to_samples(
        preset->air_delay_ms,
        state->sample_rate
    );
    state->air_attack_samples = gwh89_ms_to_samples(
        preset->air_attack_ms,
        state->sample_rate
    );

    if (state->attack_samples < 1) {
        state->attack_samples = 1;
    }
    if (state->rise_samples < 1) {
        state->rise_samples = 1;
    }
    if (state->settle_samples < 1) {
        state->settle_samples = 1;
    }
    if (state->release_samples < 1) {
        state->release_samples = 1;
    }
    if (state->air_attack_samples < 1) {
        state->air_attack_samples = 1;
    }

    state->eq_enabled = 1;
    for (i = 0; i < GWH89_EQ_BANDS; ++i) {
        state->eq_gain_q15[i] = gwh89_clamp_s32(
            preset->eq_gain_q15[i],
            0,
            65535
        );
    }
    gwh89_update_eq_coefficients(state);

    state->output_eq_enabled = 1;
    for (i = 0; i < GWH89_OUTPUT_EQ_BANDS; ++i) {
        state->output_eq_gain_q15[i] = gwh89_clamp_s32(
            preset->output_eq_gain_q15[i],
            0,
            65535
        );
    }
    gwh89_update_output_eq_coefficients(state);

    state->reverb_enabled = preset->reverb_enabled ? 1 : 0;
    state->reverb_wet_q15 = gwh89_clamp_s32(
        preset->reverb_wet_q15,
        0,
        12000
    );
    state->reverb_feedback_q15 = gwh89_clamp_s32(
        preset->reverb_feedback_q15,
        4000,
        26000
    );
    state->reverb_damping_q15 = gwh89_clamp_s32(
        preset->reverb_damping_q15,
        1000,
        20000
    );
    state->reverb_tail_samples = gwh89_ms_to_samples(
        preset->reverb_tail_ms,
        state->sample_rate
    );
    gwh89_update_reverb_lengths(state);

    state->roughness_value = 0;
    state->noise_prev_x_1 = 0;
    state->noise_hp_y_1 = 0;
    state->noise_prev_x_2 = 0;
    state->noise_hp_y_2 = 0;
    state->noise_lp_y_1 = 0;
    state->noise_lp_y_2 = 0;
    gwh89_clear_effect_memory(state);
}

void gwh89_trigger_preset(gwh89_state *state, gwh89_s32 preset_id)
{
    gwh89_trigger(state, gwh89_get_preset(preset_id));
}

void gwh89_release(gwh89_state *state)
{
    if (state == 0 || state->preset == 0) {
        return;
    }
    if (state->stage != GWH89_STAGE_ACTIVE) {
        return;
    }
    state->release_start_freq_hz = gwh89_get_macro_frequency(state);
    state->release_age_samples = 0;
    state->stage = GWH89_STAGE_RELEASE;
}

void gwh89_set_motion(
    gwh89_state *state,
    gwh89_s32 radial_velocity_mps,
    gwh89_s32 distance_gain_q15
)
{
    if (state == 0) {
        return;
    }
    state->radial_velocity_mps = gwh89_clamp_s32(
        radial_velocity_mps,
        -250,
        250
    );
    state->distance_gain_q15 = gwh89_clamp_s32(
        distance_gain_q15,
        0,
        GWH89_Q15_ONE
    );
}

void gwh89_set_pitch_scale_q15(
    gwh89_state *state,
    gwh89_s32 pitch_scale_q15
)
{
    if (state == 0) {
        return;
    }
    state->pitch_scale_q15 = gwh89_clamp_s32(
        pitch_scale_q15,
        8192,
        65535
    );
}

void gwh89_set_auto_hold_ms(gwh89_state *state, gwh89_s32 hold_ms)
{
    if (state == 0) {
        return;
    }
    state->auto_hold_samples = gwh89_ms_to_samples(
        hold_ms,
        state->sample_rate
    );
}

void gwh89_set_eq_enabled(gwh89_state *state, gwh89_s32 enabled)
{
    if (state == 0) {
        return;
    }
    state->eq_enabled = enabled ? 1 : 0;
}

void gwh89_set_eq_band_gain_q15(
    gwh89_state *state,
    gwh89_s32 band,
    gwh89_s32 gain_q15
)
{
    if (state == 0 || band < 0 || band >= GWH89_EQ_BANDS) {
        return;
    }
    state->eq_gain_q15[band] = gwh89_clamp_s32(
        gain_q15,
        0,
        65535
    );
}

void gwh89_set_eq_gains_q15(
    gwh89_state *state,
    const gwh89_s32 gains_q15[GWH89_EQ_BANDS]
)
{
    gwh89_s32 i;
    if (state == 0 || gains_q15 == 0) {
        return;
    }
    for (i = 0; i < GWH89_EQ_BANDS; ++i) {
        gwh89_set_eq_band_gain_q15(state, i, gains_q15[i]);
    }
}

void gwh89_set_eq_flat(gwh89_state *state)
{
    gwh89_s32 i;
    if (state == 0) {
        return;
    }
    for (i = 0; i < GWH89_EQ_BANDS; ++i) {
        state->eq_gain_q15[i] = GWH89_Q15_ONE;
    }
}

void gwh89_set_output_eq_enabled(
    gwh89_state *state,
    gwh89_s32 enabled
)
{
    if (state == 0) {
        return;
    }
    state->output_eq_enabled = enabled ? 1 : 0;
}

void gwh89_set_output_eq_band_gain_q15(
    gwh89_state *state,
    gwh89_s32 band,
    gwh89_s32 gain_q15
)
{
    if (state == 0 ||
        band < 0 || band >= GWH89_OUTPUT_EQ_BANDS) {
        return;
    }
    state->output_eq_gain_q15[band] = gwh89_clamp_s32(
        gain_q15,
        0,
        65535
    );
}

void gwh89_set_output_eq_gains_q15(
    gwh89_state *state,
    const gwh89_s32 gains_q15[GWH89_OUTPUT_EQ_BANDS]
)
{
    gwh89_s32 i;
    if (state == 0 || gains_q15 == 0) {
        return;
    }
    for (i = 0; i < GWH89_OUTPUT_EQ_BANDS; ++i) {
        gwh89_set_output_eq_band_gain_q15(
            state,
            i,
            gains_q15[i]
        );
    }
}

void gwh89_set_output_eq_flat(gwh89_state *state)
{
    gwh89_s32 i;
    if (state == 0) {
        return;
    }
    for (i = 0; i < GWH89_OUTPUT_EQ_BANDS; ++i) {
        state->output_eq_gain_q15[i] = GWH89_Q15_ONE;
    }
}

void gwh89_set_reverb_enabled(gwh89_state *state, gwh89_s32 enabled)
{
    if (state == 0) {
        return;
    }
    state->reverb_enabled = enabled ? 1 : 0;
    if (!state->reverb_enabled &&
        state->stage == GWH89_STAGE_REVERB_TAIL) {
        state->stage = GWH89_STAGE_DONE;
    }
}

void gwh89_set_reverb(
    gwh89_state *state,
    gwh89_s32 enabled,
    gwh89_s32 wet_q15,
    gwh89_s32 feedback_q15,
    gwh89_s32 damping_q15,
    gwh89_s32 tail_ms
)
{
    if (state == 0) {
        return;
    }
    state->reverb_enabled = enabled ? 1 : 0;
    state->reverb_wet_q15 = gwh89_clamp_s32(wet_q15, 0, 12000);
    state->reverb_feedback_q15 = gwh89_clamp_s32(
        feedback_q15,
        4000,
        26000
    );
    state->reverb_damping_q15 = gwh89_clamp_s32(
        damping_q15,
        1000,
        20000
    );
    state->reverb_tail_samples = gwh89_ms_to_samples(
        gwh89_clamp_s32(tail_ms, 0, 2000),
        state->sample_rate
    );
    if (!state->reverb_enabled &&
        state->stage == GWH89_STAGE_REVERB_TAIL) {
        state->stage = GWH89_STAGE_DONE;
    }
}

gwh89_s32 gwh89_is_active(const gwh89_state *state)
{
    if (state == 0) {
        return 0;
    }
    return state->stage == GWH89_STAGE_ACTIVE ||
           state->stage == GWH89_STAGE_RELEASE ||
           state->stage == GWH89_STAGE_REVERB_TAIL;
}

gwh89_s32 gwh89_get_current_frequency_hz(const gwh89_state *state)
{
    gwh89_s32 frequency;
    if (state == 0 || state->preset == 0) {
        return 0;
    }
    if (state->stage != GWH89_STAGE_ACTIVE &&
        state->stage != GWH89_STAGE_RELEASE) {
        return 0;
    }
    frequency = gwh89_get_macro_frequency(state);
    return gwh89_apply_modulation(state, frequency);
}

gwh89_s32 gwh89_get_estimated_total_samples(const gwh89_state *state)
{
    gwh89_s32 total;
    if (state == 0 || state->preset == 0) {
        return 0;
    }
    if (state->auto_hold_samples <= 0) {
        return -1;
    }
    total = state->rise_samples;
    total += state->settle_samples;
    total += state->auto_hold_samples;
    total += state->release_samples;
    if (state->reverb_enabled && state->reverb_wet_q15 > 0) {
        total += state->reverb_tail_samples;
    }
    return total;
}

gwh89_s16 gwh89_render_mono_sample(gwh89_state *state)
{
    gwh89_s16 left;
    gwh89_s16 right;
    gwh89_s32 mono;
    left = 0;
    right = 0;
    gwh89_render_stereo_sample_internal(state, &left, &right);
    mono = ((gwh89_s32)left + (gwh89_s32)right) >> 1;
    return gwh89_clamp_s16(mono);
}

void gwh89_render_mono(
    gwh89_state *state,
    gwh89_s16 *out,
    gwh89_s32 frames
)
{
    gwh89_s32 i;
    if (state == 0 || out == 0 || frames <= 0) {
        return;
    }
    for (i = 0; i < frames; ++i) {
        out[i] = gwh89_render_mono_sample(state);
    }
}

void gwh89_render_stereo(
    gwh89_state *state,
    gwh89_s16 *out_interleaved,
    gwh89_s32 frames
)
{
    gwh89_s32 i;
    gwh89_s16 left;
    gwh89_s16 right;
    if (state == 0 || out_interleaved == 0 || frames <= 0) {
        return;
    }
    for (i = 0; i < frames; ++i) {
        left = 0;
        right = 0;
        gwh89_render_stereo_sample_internal(state, &left, &right);
        out_interleaved[i * 2] = left;
        out_interleaved[i * 2 + 1] = right;
    }
}
