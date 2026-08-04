#include "grocketspin89.h"
#include <string.h>

#define GRS89_STAGE_OFF     0U
#define GRS89_STAGE_DELAY   1U
#define GRS89_STAGE_ATTACK  2U
#define GRS89_STAGE_SUSTAIN 3U
#define GRS89_STAGE_RELEASE 4U

#define GRS89_PHASE_MASK 0x00FFFFFFUL
#define GRS89_ENV_MAX_Q31 2147418112UL

static grs89_s32 grs89_clamp_s32(grs89_s32 value,
                                  grs89_s32 minimum,
                                  grs89_s32 maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static grs89_s16 grs89_clamp_s16(grs89_s32 value)
{
    if (value < -32768L) {
        return (grs89_s16)-32768;
    }
    if (value > 32767L) {
        return (grs89_s16)32767;
    }
    return (grs89_s16)value;
}

static grs89_u32 grs89_ms_to_samples(grs89_u32 sample_rate,
                                      grs89_u16 milliseconds)
{
    grs89_u32 whole;
    grs89_u32 remainder;

    whole = sample_rate / 1000UL;
    remainder = sample_rate % 1000UL;
    return whole * (grs89_u32)milliseconds
         + (remainder * (grs89_u32)milliseconds) / 1000UL;
}

static grs89_u32 grs89_lfo_increment(grs89_u32 sample_rate,
                                      grs89_u16 rate_millihz)
{
    grs89_u32 numerator;

    numerator = (grs89_u32)rate_millihz * 16777UL;
    if (sample_rate == 0UL) {
        return 0UL;
    }
    return numerator / sample_rate;
}

static grs89_s32 grs89_triangle_q15(grs89_u32 phase)
{
    grs89_u32 p;
    grs89_s32 value;

    p = (phase >> 8) & 65535UL;
    if (p < 32768UL) {
        value = (grs89_s32)(p << 1) - 32768L;
    } else {
        value = 98303L - (grs89_s32)(p << 1);
    }
    return value;
}

static grs89_u16 grs89_alpha_q15(grs89_u32 sample_rate,
                                  grs89_u16 cutoff_hz)
{
    grs89_u32 numerator;
    grs89_u32 denominator;
    grs89_u32 result;

    numerator = 6283UL * (grs89_u32)cutoff_hz;
    denominator = sample_rate * 1000UL + numerator;
    denominator = (denominator + 16384UL) / 32768UL;
    if (denominator == 0UL) {
        return 1U;
    }
    result = numerator / denominator;
    if (result < 1UL) {
        result = 1UL;
    }
    if (result > 32767UL) {
        result = 32767UL;
    }
    return (grs89_u16)result;
}

static grs89_s32 grs89_onepole(grs89_s32 input,
                                grs89_s32 *memory,
                                grs89_u16 alpha_q15)
{
    grs89_s32 difference;
    grs89_s32 delta;

    difference = input - *memory;
    delta = (difference * (grs89_s32)alpha_q15) >> 15;
    *memory += delta;
    return *memory;
}

static grs89_s32 grs89_soft_clip(grs89_s32 input,
                                  grs89_u16 drive_q8_8)
{
    grs89_s32 driven;
    grs89_s32 magnitude;
    grs89_s32 output;
    int negative;

    input = grs89_clamp_s32(input, -32768L, 32767L);
    driven = (input * (grs89_s32)drive_q8_8) >> 8;
    negative = 0;
    if (driven < 0L) {
        negative = 1;
        magnitude = -driven;
    } else {
        magnitude = driven;
    }

    if (magnitude <= 16384L) {
        output = magnitude;
    } else if (magnitude < 49152L) {
        output = 16384L + ((magnitude - 16384L) >> 1);
    } else {
        output = 32767L;
    }

    if (negative != 0) {
        output = -output;
    }
    return output;
}

static grs89_s32 grs89_reverb(grs89_state *state, grs89_s32 dry)
{
    grs89_u16 index;
    grs89_u16 length;
    grs89_u16 i0;
    grs89_u16 i1;
    grs89_u16 i2;
    grs89_s32 d0;
    grs89_s32 d1;
    grs89_s32 d2;
    grs89_s32 wet;
    grs89_s32 write_value;
    grs89_s32 mixed;
    grs89_s32 mix;

    index = state->reverb_index;
    length = state->reverb_length;

    i0 = (grs89_u16)((index + length - state->reverb_tap0) % length);
    i1 = (grs89_u16)((index + length - state->reverb_tap1) % length);
    i2 = (grs89_u16)((index + length - state->reverb_tap2) % length);

    d0 = (grs89_s32)state->reverb_buffer[i0];
    d1 = (grs89_s32)state->reverb_buffer[i1];
    d2 = (grs89_s32)state->reverb_buffer[i2];

    wet = (d0 * 16384L + d1 * 9830L + d2 * 6553L) >> 15;
    write_value = dry
                + ((wet * (grs89_s32)state->params.reverb_feedback_q15)
                   >> 15);
    state->reverb_buffer[index] = grs89_clamp_s16(write_value);

    index++;
    if (index >= length) {
        index = 0U;
    }
    state->reverb_index = index;

    mix = (grs89_s32)state->params.reverb_mix_q15;
    mixed = ((dry * (32767L - mix)) + (wet * mix)) >> 15;
    return mixed;
}

static grs89_s32 grs89_envelope_step(grs89_state *state)
{
    grs89_u32 next_stage_samples;
    grs89_s32 env_q15;

    if (state->stage == GRS89_STAGE_OFF) {
        return 0L;
    }

    if (state->stage == GRS89_STAGE_DELAY) {
        if (state->stage_remaining > 0UL) {
            state->stage_remaining--;
            return 0L;
        }
        state->stage = GRS89_STAGE_ATTACK;
        next_stage_samples = grs89_ms_to_samples(state->sample_rate,
                                                  state->params.attack_ms);
        state->stage_remaining = next_stage_samples;
        if (next_stage_samples == 0UL) {
            state->env_q31 = GRS89_ENV_MAX_Q31;
            state->stage = GRS89_STAGE_SUSTAIN;
            state->stage_remaining = grs89_ms_to_samples(
                state->sample_rate, state->params.sustain_ms);
        }
    }

    if (state->stage == GRS89_STAGE_ATTACK) {
        if (state->stage_remaining == 0UL) {
            state->env_q31 = GRS89_ENV_MAX_Q31;
            state->stage = GRS89_STAGE_SUSTAIN;
            state->stage_remaining = grs89_ms_to_samples(
                state->sample_rate, state->params.sustain_ms);
        } else {
            if (GRS89_ENV_MAX_Q31 - state->env_q31
                < state->attack_step_q31) {
                state->env_q31 = GRS89_ENV_MAX_Q31;
            } else {
                state->env_q31 += state->attack_step_q31;
            }
            state->stage_remaining--;
        }
    }

    if (state->stage == GRS89_STAGE_SUSTAIN) {
        state->env_q31 = GRS89_ENV_MAX_Q31;
        if (state->stage_remaining > 0UL) {
            state->stage_remaining--;
        } else {
            state->stage = GRS89_STAGE_RELEASE;
            state->stage_remaining = grs89_ms_to_samples(
                state->sample_rate, state->params.release_ms);
        }
    }

    if (state->stage == GRS89_STAGE_RELEASE) {
        if (state->stage_remaining == 0UL
            || state->env_q31 <= state->release_step_q31) {
            state->env_q31 = 0UL;
            state->stage = GRS89_STAGE_OFF;
            state->tail_remaining = grs89_ms_to_samples(
                state->sample_rate, 260U);
            return 0L;
        }
        state->env_q31 -= state->release_step_q31;
        state->stage_remaining--;
    }

    env_q15 = (grs89_s32)(state->env_q31 >> 16);
    if (env_q15 > 32767L) {
        env_q15 = 32767L;
    }
    return env_q15;
}

void grs89_params_preset(grs89_params *params, int preset_id)
{
    int i;

    if (params == 0) {
        return;
    }
    memset(params, 0, sizeof(*params));
    params->seed = 0x6D2B79F5UL;
    params->drive_q8_8 = 620U;
    params->output_gain_q15 = 21000;
    params->spin_rate_millihz = 12500U;
    params->tremolo_depth_q15 = 4600;
    params->vibrato_rate_millihz = 5200U;
    params->vibrato_depth_q15 = 2600;
    params->pitch_rate_millihz = 420U;
    params->pitch_depth_q15 = 3600;
    params->reverb_mix_q15 = 2300;
    params->reverb_feedback_q15 = 9200;
    for (i = 0; i < GRS89_EQ_BANDS; ++i) {
        params->eq_gain_q15[i] = 32767;
    }

    if (preset_id == GRS89_PRESET_HEAVY_ROCKET) {
        params->ignition_delay_ms = 40U;
        params->attack_ms = 25U;
        params->sustain_ms = 1350U;
        params->release_ms = 340U;
        params->drive_q8_8 = 700U;
        params->output_gain_q15 = 22500;
        params->eq_gain_q15[0] = 32767;
        params->eq_gain_q15[1] = 32000;
        params->eq_gain_q15[2] = 25500;
        params->eq_gain_q15[3] = 14500;
        params->eq_gain_q15[4] = 6500;
        params->eq_gain_q15[5] = 2100;
        params->spin_rate_millihz = 7600U;
        params->tremolo_depth_q15 = 5600;
        params->vibrato_rate_millihz = 3900U;
        params->vibrato_depth_q15 = 3000;
        params->pitch_rate_millihz = 300U;
        params->pitch_depth_q15 = 4200;
        params->reverb_mix_q15 = 3000;
        params->reverb_feedback_q15 = 11200;
    } else if (preset_id == GRS89_PRESET_FAST_MISSILE) {
        params->ignition_delay_ms = 0U;
        params->attack_ms = 10U;
        params->sustain_ms = 1650U;
        params->release_ms = 210U;
        params->drive_q8_8 = 560U;
        params->output_gain_q15 = 19500;
        params->eq_gain_q15[0] = 21000;
        params->eq_gain_q15[1] = 30000;
        params->eq_gain_q15[2] = 32767;
        params->eq_gain_q15[3] = 27500;
        params->eq_gain_q15[4] = 16000;
        params->eq_gain_q15[5] = 6500;
        params->spin_rate_millihz = 18800U;
        params->tremolo_depth_q15 = 3800;
        params->vibrato_rate_millihz = 6800U;
        params->vibrato_depth_q15 = 2200;
        params->pitch_rate_millihz = 610U;
        params->pitch_depth_q15 = 4700;
        params->reverb_mix_q15 = 1700;
        params->reverb_feedback_q15 = 7600;
    } else {
        params->ignition_delay_ms = 85U;
        params->attack_ms = 18U;
        params->sustain_ms = 820U;
        params->release_ms = 190U;
        params->eq_gain_q15[0] = 30000;
        params->eq_gain_q15[1] = 32767;
        params->eq_gain_q15[2] = 28000;
        params->eq_gain_q15[3] = 19000;
        params->eq_gain_q15[4] = 9000;
        params->eq_gain_q15[5] = 3500;
    }
}

int grs89_init(grs89_state *state, grs89_u32 sample_rate,
               const grs89_params *params)
{
    static const grs89_u16 cutoff_hz[5] = { 120U, 300U, 800U,
                                             2000U, 5000U };
    grs89_params local_params;
    grs89_u32 length;
    int i;

    if (state == 0) {
        return 0;
    }
    if (sample_rate < 8000UL || sample_rate > 48000UL) {
        return 0;
    }

    memset(state, 0, sizeof(*state));
    if (params == 0) {
        grs89_params_preset(&local_params, GRS89_PRESET_RPG7_SUSTAINER);
        state->params = local_params;
    } else {
        state->params = *params;
    }
    state->sample_rate = sample_rate;
    state->rng = state->params.seed;
    if (state->rng == 0UL) {
        state->rng = 1UL;
    }

    length = grs89_ms_to_samples(sample_rate, 85U);
    if (length < 512UL) {
        length = 512UL;
    }
    if (length > GRS89_REVERB_CAPACITY) {
        length = GRS89_REVERB_CAPACITY;
    }
    state->reverb_length = (grs89_u16)length;
    state->reverb_tap0 = (grs89_u16)grs89_ms_to_samples(sample_rate, 29U);
    state->reverb_tap1 = (grs89_u16)grs89_ms_to_samples(sample_rate, 43U);
    state->reverb_tap2 = (grs89_u16)grs89_ms_to_samples(sample_rate, 67U);
    if (state->reverb_tap0 >= state->reverb_length) {
        state->reverb_tap0 = (grs89_u16)(state->reverb_length / 3U);
    }
    if (state->reverb_tap1 >= state->reverb_length) {
        state->reverb_tap1 = (grs89_u16)(state->reverb_length / 2U);
    }
    if (state->reverb_tap2 >= state->reverb_length) {
        state->reverb_tap2 = (grs89_u16)((state->reverb_length * 3U) / 4U);
    }

    state->spin_inc = grs89_lfo_increment(sample_rate,
                                           state->params.spin_rate_millihz);
    state->vibrato_inc = grs89_lfo_increment(
        sample_rate, state->params.vibrato_rate_millihz);
    state->pitch_inc = grs89_lfo_increment(sample_rate,
                                            state->params.pitch_rate_millihz);
    for (i = 0; i < 5; ++i) {
        state->eq_alpha_q15[i] = grs89_alpha_q15(sample_rate, cutoff_hz[i]);
    }
    return 1;
}

void grs89_reset(grs89_state *state)
{
    grs89_params params;
    grs89_u32 sample_rate;

    if (state == 0) {
        return;
    }
    params = state->params;
    sample_rate = state->sample_rate;
    grs89_init(state, sample_rate, &params);
}

void grs89_trigger(grs89_state *state)
{
    grs89_u32 attack_samples;
    grs89_u32 release_samples;

    if (state == 0) {
        return;
    }

    attack_samples = grs89_ms_to_samples(state->sample_rate,
                                          state->params.attack_ms);
    release_samples = grs89_ms_to_samples(state->sample_rate,
                                           state->params.release_ms);
    state->attack_step_q31 = attack_samples == 0UL
                           ? GRS89_ENV_MAX_Q31
                           : GRS89_ENV_MAX_Q31 / attack_samples;
    state->release_step_q31 = release_samples == 0UL
                            ? GRS89_ENV_MAX_Q31
                            : GRS89_ENV_MAX_Q31 / release_samples;
    if (state->attack_step_q31 == 0UL) {
        state->attack_step_q31 = 1UL;
    }
    if (state->release_step_q31 == 0UL) {
        state->release_step_q31 = 1UL;
    }

    state->env_q31 = 0UL;
    state->stage = GRS89_STAGE_DELAY;
    state->stage_remaining = grs89_ms_to_samples(
        state->sample_rate, state->params.ignition_delay_ms);
    state->tail_remaining = 0UL;
    state->spin_phase = 0UL;
    state->vibrato_phase = 0x00400000UL;
    state->pitch_phase = 0x00800000UL;
}

void grs89_stop(grs89_state *state)
{
    grs89_u32 release_samples;

    if (state == 0 || state->stage == GRS89_STAGE_OFF) {
        return;
    }
    release_samples = grs89_ms_to_samples(state->sample_rate,
                                           state->params.release_ms);
    state->release_step_q31 = release_samples == 0UL
                            ? GRS89_ENV_MAX_Q31
                            : state->env_q31 / release_samples;
    if (state->release_step_q31 == 0UL) {
        state->release_step_q31 = 1UL;
    }
    state->stage = GRS89_STAGE_RELEASE;
    state->stage_remaining = release_samples;
}

int grs89_is_active(const grs89_state *state)
{
    if (state == 0) {
        return 0;
    }
    return state->stage != GRS89_STAGE_OFF || state->tail_remaining > 0UL;
}

grs89_s16 grs89_process_sample(grs89_state *state)
{
        grs89_s32 envelope;
    grs89_s32 pitch_lfo;
    grs89_s32 vibrato_lfo;
    grs89_s32 spin_lfo;
    grs89_s32 pitch_scale;
    grs89_u32 dynamic_spin_inc;
    grs89_s32 noise;
    grs89_s32 lowpass[5];
    grs89_s32 bands[GRS89_EQ_BANDS];
    grs89_s32 filtered;
    grs89_s32 gain;
    grs89_s32 vibrato_offset;
    grs89_s32 alpha_scale;
    grs89_u16 alpha;
    grs89_u16 dynamic_alpha;
    grs89_s32 tremolo_gain;
    grs89_s32 dry;
    grs89_s32 output;
    int i;

    if (state == 0) {
        return 0;
    }

    envelope = grs89_envelope_step(state);

    state->pitch_phase = (state->pitch_phase + state->pitch_inc)
                       & GRS89_PHASE_MASK;
    pitch_lfo = grs89_triangle_q15(state->pitch_phase);
    pitch_scale = 32767L
                + ((pitch_lfo * (grs89_s32)state->params.pitch_depth_q15)
                   >> 15);
    if (pitch_scale < 16384L) {
        pitch_scale = 16384L;
    }
    dynamic_spin_inc = ((state->spin_inc * (grs89_u32)pitch_scale) >> 15);
    state->spin_phase = (state->spin_phase + dynamic_spin_inc)
                      & GRS89_PHASE_MASK;
    state->vibrato_phase = (state->vibrato_phase + state->vibrato_inc)
                         & GRS89_PHASE_MASK;
    spin_lfo = grs89_triangle_q15(state->spin_phase);
    vibrato_lfo = grs89_triangle_q15(state->vibrato_phase);

    dry = 0L;
    if (envelope > 0L) {
        state->rng = state->rng * 1664525UL + 1013904223UL;
        noise = (grs89_s32)((state->rng >> 16) & 65535UL) - 32768L;

        alpha_scale = 32767L
                    + ((pitch_lfo
                        * (grs89_s32)state->params.pitch_depth_q15) >> 16);
        if (alpha_scale < 24576L) {
            alpha_scale = 24576L;
        }
        if (alpha_scale > 40959L) {
            alpha_scale = 40959L;
        }

        for (i = 0; i < 5; ++i) {
            alpha = state->eq_alpha_q15[i];
            dynamic_alpha = (grs89_u16)(((grs89_u32)alpha
                              * (grs89_u32)alpha_scale) >> 15);
            if (dynamic_alpha < 1U) {
                dynamic_alpha = 1U;
            }
            if (dynamic_alpha > 32767U) {
                dynamic_alpha = 32767U;
            }
            lowpass[i] = grs89_onepole(noise,
                                       &state->lowpass_state[i],
                                       dynamic_alpha);
        }

        bands[0] = lowpass[0];
        bands[1] = lowpass[1] - lowpass[0];
        bands[2] = lowpass[2] - lowpass[1];
        bands[3] = lowpass[3] - lowpass[2];
        bands[4] = lowpass[4] - lowpass[3];
        bands[5] = noise - lowpass[4];

        vibrato_offset = (vibrato_lfo
                         * (grs89_s32)state->params.vibrato_depth_q15) >> 15;
        filtered = 0L;
        for (i = 0; i < GRS89_EQ_BANDS; ++i) {
            gain = (grs89_s32)state->params.eq_gain_q15[i];
            if (i == 1) {
                gain += vibrato_offset;
            } else if (i == 2) {
                gain -= vibrato_offset;
            }
            gain = grs89_clamp_s32(gain, 0L, 32767L);
            filtered += (bands[i] * gain) >> 15;
        }

        tremolo_gain = 32767L
                     - ((grs89_s32)state->params.tremolo_depth_q15 >> 1)
                     + ((spin_lfo
                         * (grs89_s32)state->params.tremolo_depth_q15) >> 16);
        tremolo_gain = grs89_clamp_s32(tremolo_gain, 0L, 32767L);

        filtered = (filtered * tremolo_gain) >> 15;
        filtered = grs89_soft_clip(filtered, state->params.drive_q8_8);
        filtered = (filtered * envelope) >> 15;
        dry = (filtered * (grs89_s32)state->params.output_gain_q15) >> 15;
    }

    output = grs89_reverb(state, dry);
    if (state->stage == GRS89_STAGE_OFF && state->tail_remaining > 0UL) {
        state->tail_remaining--;
    }
    return grs89_clamp_s16(output);
}

void grs89_process_mono(grs89_state *state, grs89_s16 *output,
                        grs89_u32 sample_count)
{
    grs89_u32 i;

    if (state == 0 || output == 0) {
        return;
    }
    for (i = 0UL; i < sample_count; ++i) {
        output[i] = grs89_process_sample(state);
    }
}
