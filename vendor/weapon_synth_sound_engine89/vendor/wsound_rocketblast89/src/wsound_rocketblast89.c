#include "wsrb89_internal.h"

#define WSRB89_ENV_OFF 0U
#define WSRB89_ENV_ATTACK 1U
#define WSRB89_ENV_DECAY 2U
#define WSRB89_ENV_SUSTAIN 3U
#define WSRB89_ENV_RELEASE 4U

static void wsrb89_zero_bytes(void *ptr, wsrb89_u32 count)
{
    unsigned char *bytes;
    wsrb89_u32 i;
    bytes = (unsigned char *)ptr;
    i = 0U;
    while (i < count) {
        bytes[i] = 0U;
        i++;
    }
}

static wsrb89_u32 wsrb89_ms_to_samples(wsrb89_u32 sample_rate,
                                       wsrb89_u16 milliseconds)
{
    wsrb89_u32 whole;
    wsrb89_u32 remainder;
    whole = sample_rate / 1000U;
    remainder = sample_rate % 1000U;
    return (whole * (wsrb89_u32)milliseconds)
         + ((remainder * (wsrb89_u32)milliseconds) / 1000U);
}

static wsrb89_u16 wsrb89_scale_delay(wsrb89_u16 base_at_44100,
                                     wsrb89_u32 sample_rate,
                                     wsrb89_u16 capacity)
{
    wsrb89_u32 result;
    result = ((wsrb89_u32)base_at_44100 * sample_rate) / 44100U;
    if (result < 1U) {
        result = 1U;
    }
    if (result >= (wsrb89_u32)capacity) {
        result = (wsrb89_u32)capacity - 1U;
    }
    return (wsrb89_u16)result;
}

static void wsrb89_configure_filters(wsrb89_context *ctx)
{
    static const wsrb89_u16 eq_hz[WSRB89_EQ_BANDS] = {
        80U, 180U, 450U, 1200U, 3200U, 7200U
    };
    static const wsrb89_u16 early_base[WSRB89_EARLY_TAPS] = {
        529U, 1019U, 1637U, 2477U, 3613U
    };
    wsrb89_u16 i;
    wsrb89_u16 transient_cutoff;
    wsrb89_u32 chorus_samples;
    wsrb89_u32 chorus_depth;
    wsrb89_u32 numerator;

    ctx->main_svf.coeff_q15 = wsrb89_svf_coeff(ctx->sample_rate,
                                               ctx->params.svf_cutoff_hz);
    ctx->main_svf.damp_q15 = (wsrb89_s16)ctx->params.svf_damp_q15;

    transient_cutoff = (wsrb89_u16)(ctx->params.svf_cutoff_hz + 2200U);
    if (transient_cutoff > 7800U) {
        transient_cutoff = 7800U;
    }
    ctx->transient_svf.coeff_q15 = wsrb89_svf_coeff(ctx->sample_rate,
                                                    transient_cutoff);
    ctx->transient_svf.damp_q15 = 25800;

    ctx->rumble_svf.coeff_q15 = wsrb89_svf_coeff(
        ctx->sample_rate, ctx->params.rumble_cutoff_hz);
    ctx->rumble_svf.damp_q15 = (wsrb89_s16)ctx->params.rumble_damp_q15;
    ctx->crackle_svf.coeff_q15 = wsrb89_svf_coeff(
        ctx->sample_rate, ctx->params.crackle_cutoff_hz);
    ctx->crackle_svf.damp_q15 = (wsrb89_s16)ctx->params.crackle_damp_q15;

    i = 0U;
    while (i < WSRB89_EQ_BANDS) {
        ctx->eq_svf[i].coeff_q15 = wsrb89_svf_coeff(ctx->sample_rate,
                                                    eq_hz[i]);
        ctx->eq_svf[i].damp_q15 = 27600;
        ctx->transient_eq_svf[i].coeff_q15 = ctx->eq_svf[i].coeff_q15;
        ctx->transient_eq_svf[i].damp_q15 = 28200;
        i++;
    }

    i = 0U;
    while (i < WSRB89_EARLY_TAPS) {
        ctx->early_tap[i] = wsrb89_scale_delay(early_base[i],
                                               ctx->sample_rate,
                                               WSRB89_EARLY_CAP);
        i++;
    }

    chorus_samples = wsrb89_ms_to_samples(ctx->sample_rate,
                                          ctx->params.chorus_base_ms);
    chorus_depth = wsrb89_ms_to_samples(ctx->sample_rate,
                                        ctx->params.chorus_depth_ms);
    if (chorus_samples < 1U) {
        chorus_samples = 1U;
    }
    if (chorus_samples >= WSRB89_CHORUS_CAP) {
        chorus_samples = WSRB89_CHORUS_CAP - 1U;
    }
    if (chorus_depth >= chorus_samples) {
        if (chorus_samples > 1U) {
            chorus_depth = chorus_samples - 1U;
        } else {
            chorus_depth = 0U;
        }
    }
    ctx->chorus_base_samples = (wsrb89_u16)chorus_samples;
    ctx->chorus_depth_samples = (wsrb89_u16)chorus_depth;

    numerator = (wsrb89_u32)ctx->params.chorus_rate_millihz * 65536U;
    ctx->chorus_phase_den = ctx->sample_rate * 1000U;
    if (ctx->chorus_phase_den == 0U) {
        ctx->chorus_phase_den = 1U;
    }
    ctx->chorus_phase_step = (wsrb89_u16)(numerator / ctx->chorus_phase_den);
    ctx->chorus_phase_rem_step = numerator % ctx->chorus_phase_den;

    numerator = (wsrb89_u32)ctx->params.motion_lfo_rate_millihz * 65536U;
    ctx->motion_lfo_phase_den = ctx->sample_rate * 1000U;
    if (ctx->motion_lfo_phase_den == 0U) {
        ctx->motion_lfo_phase_den = 1U;
    }
    ctx->motion_lfo_phase_step = (wsrb89_u16)(
        numerator / ctx->motion_lfo_phase_den);
    ctx->motion_lfo_phase_rem_step = numerator
                                   % ctx->motion_lfo_phase_den;

    ctx->comb_l1_len = wsrb89_scale_delay(1499U, ctx->sample_rate,
                                          WSRB89_COMB_L1_CAP);
    ctx->comb_l2_len = wsrb89_scale_delay(2111U, ctx->sample_rate,
                                          WSRB89_COMB_L2_CAP);
    ctx->comb_l3_len = wsrb89_scale_delay(2671U, ctx->sample_rate,
                                          WSRB89_COMB_L3_CAP);
    ctx->comb_r1_len = wsrb89_scale_delay(1613U, ctx->sample_rate,
                                          WSRB89_COMB_R1_CAP);
    ctx->comb_r2_len = wsrb89_scale_delay(2237U, ctx->sample_rate,
                                          WSRB89_COMB_R2_CAP);
    ctx->comb_r3_len = wsrb89_scale_delay(2797U, ctx->sample_rate,
                                          WSRB89_COMB_R3_CAP);
    ctx->allpass_l1_len = wsrb89_scale_delay(347U, ctx->sample_rate,
                                             WSRB89_ALLPASS_L1_CAP);
    ctx->allpass_l2_len = wsrb89_scale_delay(557U, ctx->sample_rate,
                                             WSRB89_ALLPASS_L2_CAP);
    ctx->allpass_r1_len = wsrb89_scale_delay(379U, ctx->sample_rate,
                                             WSRB89_ALLPASS_R1_CAP);
    ctx->allpass_r2_len = wsrb89_scale_delay(593U, ctx->sample_rate,
                                             WSRB89_ALLPASS_R2_CAP);
}

static wsrb89_s16 wsrb89_master_tick(wsrb89_context *ctx)
{
    wsrb89_s16 current;
    wsrb89_s16 sustain;
    wsrb89_u32 samples;

    current = wsrb89_env_tick(&ctx->master_env);

    if ((ctx->master_env.stage == WSRB89_ENV_ATTACK)
        && (ctx->master_env.remaining == 0U)) {
        sustain = (wsrb89_s16)(((wsrb89_s32)current
                  * (wsrb89_s32)ctx->params.sustain_level_q15) >> 15);
        samples = wsrb89_ms_to_samples(ctx->sample_rate, ctx->params.decay_ms);
        wsrb89_env_start(&ctx->master_env, current, sustain, samples);
        ctx->master_env.stage = WSRB89_ENV_DECAY;
    } else if ((ctx->master_env.stage == WSRB89_ENV_DECAY)
               && (ctx->master_env.remaining == 0U)) {
        ctx->master_env.stage = WSRB89_ENV_SUSTAIN;
        ctx->master_env.sustain_remaining = wsrb89_ms_to_samples(
            ctx->sample_rate, ctx->params.sustain_ms);
    } else if (ctx->master_env.stage == WSRB89_ENV_SUSTAIN) {
        if (ctx->master_env.sustain_remaining > 0U) {
            ctx->master_env.sustain_remaining--;
        } else {
            samples = wsrb89_ms_to_samples(ctx->sample_rate,
                                            ctx->params.release_ms);
            wsrb89_env_start(&ctx->master_env, current, 0, samples);
            ctx->master_env.stage = WSRB89_ENV_RELEASE;
        }
    } else if ((ctx->master_env.stage == WSRB89_ENV_RELEASE)
               && (ctx->master_env.remaining == 0U)) {
        wsrb89_u32 tail_ms;
        ctx->master_env.stage = WSRB89_ENV_OFF;
        ctx->active = 0U;
        tail_ms = 250U + (((wsrb89_u32)ctx->params.reverb_feedback_q15
                         * 1750U) / 32767U);
        ctx->tail_remaining = (ctx->sample_rate / 1000U) * tail_ms;
        current = 0;
    }

    return current;
}

static wsrb89_s16 wsrb89_shock_shape(wsrb89_context *ctx)
{
    wsrb89_u32 positive_len;
    wsrb89_u32 negative_len;
    wsrb89_u32 age;
    wsrb89_u32 remaining;
    wsrb89_s32 norm_q15;
    wsrb89_s32 curved_q15;

    /* A blast front is not a slow oscillator attack.  At audio rates we
       represent its near-discontinuous rise with a full-scale first sample,
       then a short curved positive phase followed by a weaker rarefaction. */
    positive_len = wsrb89_ms_to_samples(ctx->sample_rate, 2U);
    negative_len = wsrb89_ms_to_samples(ctx->sample_rate, 7U);
    if (positive_len < 2U) {
        positive_len = 2U;
    }
    if (negative_len < 1U) {
        negative_len = 1U;
    }
    age = ctx->age_samples;
    if (age == 0U) {
        return 32767;
    }
    if (age < positive_len) {
        remaining = positive_len - age;
        norm_q15 = (wsrb89_s32)((remaining * 32767U) / positive_len);
        curved_q15 = (norm_q15 * norm_q15) >> 15;
        return wsrb89_clamp16(curved_q15);
    }
    age -= positive_len;
    if (age < negative_len) {
        remaining = negative_len - age;
        norm_q15 = (wsrb89_s32)((remaining * 32767U) / negative_len);
        curved_q15 = (norm_q15 * norm_q15) >> 15;
        return wsrb89_clamp16(-(curved_q15 >> 2));
    }
    return 0;
}

static wsrb89_s16 wsrb89_age_gate(wsrb89_context *ctx,
                                   wsrb89_u32 duration_ms)
{
    wsrb89_u32 length;
    wsrb89_u32 remaining;
    length = (ctx->sample_rate / 1000U) * duration_ms;
    if (length < 1U) {
        length = 1U;
    }
    if (ctx->age_samples >= length) {
        return 0;
    }
    remaining = length - ctx->age_samples;
    return (wsrb89_s16)((remaining * 32767U) / length);
}

static wsrb89_s16 wsrb89_age_attack_gate(wsrb89_context *ctx,
                                          wsrb89_u32 duration_ms)
{
    wsrb89_u32 length;
    length = (ctx->sample_rate / 1000U) * duration_ms;
    if (length < 1U) {
        length = 1U;
    }
    if (ctx->age_samples >= length) {
        return 32767;
    }
    return (wsrb89_s16)((ctx->age_samples * 32767U) / length);
}

static wsrb89_s16 wsrb89_eq6_process_bank(wsrb89_context *ctx,
                                           wsrb89_svf *bank,
                                           wsrb89_s16 input)
{
    wsrb89_s32 sum;
    wsrb89_s16 band;
    wsrb89_u16 i;
    sum = input;
    i = 0U;
    while (i < WSRB89_EQ_BANDS) {
        band = wsrb89_svf_process(&bank[i], input, WSRB89_SVF_BAND);
        sum += ((wsrb89_s32)band
             * (wsrb89_s32)ctx->params.eq_gain_q14[i]) >> 14;
        i++;
    }
    return wsrb89_clamp16(sum);
}

static wsrb89_s16 wsrb89_distort(wsrb89_context *ctx, wsrb89_s16 input)
{
    wsrb89_s32 driven;
    driven = ((wsrb89_s32)input * (wsrb89_s32)ctx->params.drive_q12) >> 12;
    return wsrb89_soft_clip(driven);
}

static wsrb89_u16 wsrb89_delay_index(wsrb89_u16 write_pos,
                                     wsrb89_s32 delay,
                                     wsrb89_u16 capacity)
{
    wsrb89_s32 index;
    if (delay < 1) {
        delay = 1;
    }
    if (delay >= (wsrb89_s32)capacity) {
        delay = (wsrb89_s32)capacity - 1;
    }
    index = (wsrb89_s32)write_pos - delay;
    while (index < 0) {
        index += capacity;
    }
    return (wsrb89_u16)index;
}

static void wsrb89_chorus_tick(wsrb89_context *ctx, wsrb89_s16 input,
                               wsrb89_s16 *left, wsrb89_s16 *right)
{
    wsrb89_s16 lfo_l;
    wsrb89_s16 lfo_r;
    wsrb89_s16 wet_l;
    wsrb89_s16 wet_r;
    wsrb89_s32 delay_l;
    wsrb89_s32 delay_r;
    wsrb89_s32 mix;
    wsrb89_s32 dry;
    wsrb89_u16 read_l;
    wsrb89_u16 read_r;

    lfo_l = wsrb89_sine_q15(ctx->chorus_phase);
    lfo_r = wsrb89_sine_q15((wsrb89_u16)(ctx->chorus_phase + 16384U));
    delay_l = (wsrb89_s32)ctx->chorus_base_samples
            + (((wsrb89_s32)lfo_l * ctx->chorus_depth_samples) >> 15);
    delay_r = (wsrb89_s32)ctx->chorus_base_samples
            + (((wsrb89_s32)lfo_r * ctx->chorus_depth_samples) >> 15);
    read_l = wsrb89_delay_index(ctx->chorus_write, delay_l,
                                  WSRB89_CHORUS_CAP);
    read_r = wsrb89_delay_index(ctx->chorus_write, delay_r,
                                  WSRB89_CHORUS_CAP);
    wet_l = ctx->workspace->chorus[read_l];
    wet_r = ctx->workspace->chorus[read_r];
    ctx->workspace->chorus[ctx->chorus_write] = input;
    ctx->chorus_write++;
    if (ctx->chorus_write >= WSRB89_CHORUS_CAP) {
        ctx->chorus_write = 0U;
    }

    mix = ctx->params.chorus_mix_q15;
    dry = 32767 - mix;
    *left = wsrb89_clamp16((((wsrb89_s32)input * dry)
            + ((wsrb89_s32)wet_l * mix)) >> 15);
    *right = wsrb89_clamp16((((wsrb89_s32)input * dry)
             + ((wsrb89_s32)wet_r * mix)) >> 15);

    ctx->chorus_phase = (wsrb89_u16)(ctx->chorus_phase
                          + ctx->chorus_phase_step);
    ctx->chorus_phase_rem += ctx->chorus_phase_rem_step;
    if (ctx->chorus_phase_rem >= ctx->chorus_phase_den) {
        ctx->chorus_phase_rem -= ctx->chorus_phase_den;
        ctx->chorus_phase++;
    }
}

static void wsrb89_early_tick(wsrb89_context *ctx, wsrb89_s16 input,
                              wsrb89_s16 *left, wsrb89_s16 *right)
{
    static const wsrb89_s16 gain_l[WSRB89_EARLY_TAPS] = {
        11796, 8192, -5898, 4587, -3277
    };
    static const wsrb89_s16 gain_r[WSRB89_EARLY_TAPS] = {
        9175, -7209, 7864, -3932, 3604
    };
    wsrb89_s32 sum_l;
    wsrb89_s32 sum_r;
    wsrb89_s32 shaped_l;
    wsrb89_s32 shaped_r;
    wsrb89_s16 delayed;
    wsrb89_u16 read_pos;
    wsrb89_u16 i;

    ctx->workspace->early[ctx->early_write] = input;
    sum_l = 0;
    sum_r = 0;
    i = 0U;
    while (i < WSRB89_EARLY_TAPS) {
        read_pos = wsrb89_delay_index(ctx->early_write,
                                      ctx->early_tap[i],
                                      WSRB89_EARLY_CAP);
        delayed = ctx->workspace->early[read_pos];
        sum_l += ((wsrb89_s32)delayed * gain_l[i]) >> 15;
        sum_r += ((wsrb89_s32)delayed * gain_r[i]) >> 15;
        i++;
    }
    ctx->early_write++;
    if (ctx->early_write >= WSRB89_EARLY_CAP) {
        ctx->early_write = 0U;
    }

    ctx->early_lp_l += (sum_l - ctx->early_lp_l) >> 1;
    ctx->early_lp_r += (sum_r - ctx->early_lp_r) >> 1;
    shaped_l = (sum_l + (ctx->early_lp_l * 2)) / 3;
    shaped_r = (sum_r + (ctx->early_lp_r * 2)) / 3;
    *left = wsrb89_clamp16(shaped_l);
    *right = wsrb89_clamp16(shaped_r);
}

static wsrb89_s16 wsrb89_comb_tick(wsrb89_s16 *buffer,
                                   wsrb89_u16 length,
                                   wsrb89_u16 *position,
                                   wsrb89_s32 *damp_state,
                                   wsrb89_s16 input,
                                   wsrb89_u16 feedback_q15,
                                   wsrb89_u16 damp_q15)
{
    wsrb89_s16 delayed;
    wsrb89_s32 damped;
    wsrb89_s32 stored;
    wsrb89_s32 inverse_damp;

    delayed = buffer[*position];
    inverse_damp = 32767 - (wsrb89_s32)damp_q15;
    damped = ((*damp_state * (wsrb89_s32)damp_q15)
           + ((wsrb89_s32)delayed * inverse_damp)) >> 15;
    *damp_state = damped;
    stored = (wsrb89_s32)input
           + ((damped * (wsrb89_s32)feedback_q15) >> 15);
    buffer[*position] = wsrb89_clamp16(stored);
    (*position)++;
    if (*position >= length) {
        *position = 0U;
    }
    return delayed;
}

static wsrb89_s16 wsrb89_allpass_tick(wsrb89_s16 *buffer,
                                      wsrb89_u16 length,
                                      wsrb89_u16 *position,
                                      wsrb89_s16 input)
{
    wsrb89_s16 delayed;
    wsrb89_s32 output;
    wsrb89_s32 stored;
    delayed = buffer[*position];
    output = (wsrb89_s32)delayed - input;
    stored = (wsrb89_s32)input
           + (((wsrb89_s32)delayed * 16384) >> 15);
    buffer[*position] = wsrb89_clamp16(stored);
    (*position)++;
    if (*position >= length) {
        *position = 0U;
    }
    return wsrb89_clamp16(output);
}

static void wsrb89_reverb_tick(wsrb89_context *ctx,
                               wsrb89_s16 in_l, wsrb89_s16 in_r,
                               wsrb89_s16 *out_l, wsrb89_s16 *out_r)
{
    wsrb89_s16 c_l1;
    wsrb89_s16 c_l2;
    wsrb89_s16 c_l3;
    wsrb89_s16 c_r1;
    wsrb89_s16 c_r2;
    wsrb89_s16 c_r3;
    wsrb89_s16 wet_l;
    wsrb89_s16 wet_r;
    wsrb89_s16 feed_l;
    wsrb89_s16 feed_r;

    feed_l = (wsrb89_s16)(((wsrb89_s32)in_l * 18432) >> 15);
    feed_r = (wsrb89_s16)(((wsrb89_s32)in_r * 18432) >> 15);
    c_l1 = wsrb89_comb_tick(ctx->workspace->comb_l1, ctx->comb_l1_len,
                            &ctx->comb_l1_pos, &ctx->comb_l1_damp,
                            feed_l, ctx->params.reverb_feedback_q15,
                            ctx->params.reverb_damp_q15);
    c_l2 = wsrb89_comb_tick(ctx->workspace->comb_l2, ctx->comb_l2_len,
                            &ctx->comb_l2_pos, &ctx->comb_l2_damp,
                            feed_l, ctx->params.reverb_feedback_q15,
                            ctx->params.reverb_damp_q15);
    c_l3 = wsrb89_comb_tick(ctx->workspace->comb_l3, ctx->comb_l3_len,
                            &ctx->comb_l3_pos, &ctx->comb_l3_damp,
                            feed_l, ctx->params.reverb_feedback_q15,
                            ctx->params.reverb_damp_q15);
    c_r1 = wsrb89_comb_tick(ctx->workspace->comb_r1, ctx->comb_r1_len,
                            &ctx->comb_r1_pos, &ctx->comb_r1_damp,
                            feed_r, ctx->params.reverb_feedback_q15,
                            ctx->params.reverb_damp_q15);
    c_r2 = wsrb89_comb_tick(ctx->workspace->comb_r2, ctx->comb_r2_len,
                            &ctx->comb_r2_pos, &ctx->comb_r2_damp,
                            feed_r, ctx->params.reverb_feedback_q15,
                            ctx->params.reverb_damp_q15);
    c_r3 = wsrb89_comb_tick(ctx->workspace->comb_r3, ctx->comb_r3_len,
                            &ctx->comb_r3_pos, &ctx->comb_r3_damp,
                            feed_r, ctx->params.reverb_feedback_q15,
                            ctx->params.reverb_damp_q15);
    wet_l = wsrb89_clamp16(((wsrb89_s32)c_l1 + c_l2 + c_l3) / 3);
    wet_r = wsrb89_clamp16(((wsrb89_s32)c_r1 + c_r2 + c_r3) / 3);
    wet_l = wsrb89_allpass_tick(ctx->workspace->allpass_l1,
                                ctx->allpass_l1_len,
                                &ctx->allpass_l1_pos, wet_l);
    wet_l = wsrb89_allpass_tick(ctx->workspace->allpass_l2,
                                ctx->allpass_l2_len,
                                &ctx->allpass_l2_pos, wet_l);
    wet_r = wsrb89_allpass_tick(ctx->workspace->allpass_r1,
                                ctx->allpass_r1_len,
                                &ctx->allpass_r1_pos, wet_r);
    wet_r = wsrb89_allpass_tick(ctx->workspace->allpass_r2,
                                ctx->allpass_r2_len,
                                &ctx->allpass_r2_pos, wet_r);
    *out_l = wet_l;
    *out_r = wet_r;
}

static wsrb89_s32 wsrb89_decay_signed_q15(wsrb89_s32 value,
                                           wsrb89_s32 coefficient)
{
    if (value < 0) {
        return -(((-value) * coefficient) >> 15);
    }
    return (value * coefficient) >> 15;
}

static wsrb89_s16 wsrb89_dc_block(wsrb89_s16 input,
                                  wsrb89_s32 *old_x,
                                  wsrb89_s32 *old_y)
{
    wsrb89_s32 output;
    output = (wsrb89_s32)input - *old_x
           + wsrb89_decay_signed_q15(*old_y, 32604);
    if ((input == 0) && (*old_x == 0)
        && (output > -2) && (output < 2)) {
        output = 0;
    }
    *old_x = input;
    *old_y = output;
    return wsrb89_clamp16(output);
}

static wsrb89_s16 wsrb89_motion_lfo_tick(wsrb89_context *ctx)
{
    wsrb89_s16 bipolar;
    wsrb89_s32 unipolar;
    wsrb89_s32 depth;
    wsrb89_s32 gain;

    bipolar = wsrb89_sine_q15(ctx->motion_lfo_phase);
    unipolar = ((wsrb89_s32)bipolar + 32767) >> 1;
    depth = ctx->params.motion_lfo_depth_q15;
    gain = (32767 - depth) + ((unipolar * depth) >> 15);

    ctx->motion_lfo_phase = (wsrb89_u16)(ctx->motion_lfo_phase
                              + ctx->motion_lfo_phase_step);
    ctx->motion_lfo_phase_rem += ctx->motion_lfo_phase_rem_step;
    if (ctx->motion_lfo_phase_rem >= ctx->motion_lfo_phase_den) {
        ctx->motion_lfo_phase_rem -= ctx->motion_lfo_phase_den;
        ctx->motion_lfo_phase++;
    }
    return wsrb89_clamp16(gain);
}

static wsrb89_s16 wsrb89_render_voice_sample(wsrb89_context *ctx,
                                               wsrb89_s16 *room_send)
{
    wsrb89_s16 master;
    wsrb89_s16 crack;
    wsrb89_s16 debris_env;
    wsrb89_s16 rumble_env;
    wsrb89_s16 crackle_env;
    wsrb89_s16 n0;
    wsrb89_s16 n1;
    wsrb89_s16 n2;
    wsrb89_s16 n3;
    wsrb89_s16 n4;
    wsrb89_s16 n5;
    wsrb89_s16 n6;
    wsrb89_s16 body;
    wsrb89_s16 body_low_octave;
    wsrb89_s16 body_high_octave;
    wsrb89_s16 debris;
    wsrb89_s16 rumble;
    wsrb89_s16 crackle;
    wsrb89_s16 rumble_filtered;
    wsrb89_s16 crackle_filtered;
    wsrb89_s16 motion_lfo_gain;
    wsrb89_s16 rumble_attack;
    wsrb89_s16 crackle_attack;
    wsrb89_s16 sine;
    wsrb89_s16 saw;
    wsrb89_s16 shock;
    wsrb89_s16 sub_gate;
    wsrb89_s16 sub_attack;
    wsrb89_s16 high_gate;
    wsrb89_s16 cloud_gate;
    wsrb89_s16 body_input;
    wsrb89_s16 body_driven;
    wsrb89_s16 body_filtered;
    wsrb89_s16 body_equalized;
    wsrb89_s16 transient_input;
    wsrb89_s16 transient_filtered;
    wsrb89_s16 transient_equalized;
    wsrb89_s32 body_sum;
    wsrb89_s32 transient_sum;
    wsrb89_s32 rumble_sum;
    wsrb89_s32 crackle_sum;
    wsrb89_s32 shock_sum;
    wsrb89_s32 final_sum;
    wsrb89_s32 room_sum;
    wsrb89_s32 cloud_sum;
    wsrb89_s32 cloud_raw;
    wsrb89_s32 sub_mix;
    wsrb89_s32 sub_mod;
    wsrb89_s32 sub_freq;
    wsrb89_s32 start_freq;
    wsrb89_s32 end_freq;
    wsrb89_u32 pitch_samples;
    wsrb89_u32 remaining;
    wsrb89_u32 phase_inc;
    wsrb89_s32 phase_step;
    wsrb89_s32 gate;
    wsrb89_u32 cloud_duration;

    master = wsrb89_master_tick(ctx);
    crack = wsrb89_env_tick(&ctx->crack_env);
    debris_env = wsrb89_env_tick(&ctx->debris_env);
    rumble_env = wsrb89_env_tick(&ctx->rumble_env);
    crackle_env = wsrb89_env_tick(&ctx->crackle_env);

    n0 = wsrb89_noise_next(&ctx->rng[WSRB89_NOISE_CRACK]);
    n1 = wsrb89_noise_next(&ctx->rng[WSRB89_NOISE_BODY]);
    n2 = wsrb89_noise_next(&ctx->rng[WSRB89_NOISE_DEBRIS]);
    n3 = wsrb89_noise_next(&ctx->rng[WSRB89_NOISE_BODY_LOW_OCTAVE]);
    n4 = wsrb89_noise_next(&ctx->rng[WSRB89_NOISE_BODY_HIGH_OCTAVE]);
    n5 = wsrb89_noise_next(&ctx->rng[WSRB89_NOISE_RUMBLE]);
    n6 = wsrb89_noise_next(&ctx->rng[WSRB89_NOISE_CRACKLE]);

    ctx->noise_body_state += ((wsrb89_s32)n1
                            - ctx->noise_body_state) >> 3;
    body = wsrb89_clamp16(ctx->noise_body_state
                         + ((wsrb89_s32)n1 >> 5));

    ctx->noise_body_low_octave_state += ((wsrb89_s32)n3
                          - ctx->noise_body_low_octave_state) >> 5;
    body_low_octave = wsrb89_soft_clip(
        ctx->noise_body_low_octave_state * 2);

    ctx->noise_body_high_octave_low += ((wsrb89_s32)n4
                         - ctx->noise_body_high_octave_low) >> 3;
    body_high_octave = wsrb89_clamp16((wsrb89_s32)n4
                         - ctx->noise_body_high_octave_low);

    ctx->noise_debris_low += ((wsrb89_s32)n2
                            - ctx->noise_debris_low) >> 4;
    debris = wsrb89_clamp16((wsrb89_s32)n2
                           - ctx->noise_debris_low);
    gate = ((ctx->rng[WSRB89_NOISE_DEBRIS] & 63U) < 8U) ? 32767 : 4096;
    debris = wsrb89_clamp16(((wsrb89_s32)debris * gate) >> 15);

    ctx->noise_rumble_state += ((wsrb89_s32)n5
                              - ctx->noise_rumble_state) >> 4;
    rumble = wsrb89_soft_clip(ctx->noise_rumble_state * 3);
    rumble_attack = wsrb89_age_attack_gate(ctx, 18U);
    motion_lfo_gain = wsrb89_motion_lfo_tick(ctx);
    rumble_sum = ((wsrb89_s32)rumble
               * ctx->params.noise_level_q15[WSRB89_NOISE_RUMBLE]) >> 15;
    rumble_sum = (rumble_sum * rumble_env) >> 15;
    rumble_sum = (rumble_sum * rumble_attack) >> 15;
    rumble_sum = (rumble_sum * motion_lfo_gain) >> 15;
    rumble_filtered = wsrb89_svf_process(&ctx->rumble_svf,
        wsrb89_soft_clip(rumble_sum * 2), WSRB89_SVF_LOW);

    ctx->noise_crackle_low += ((wsrb89_s32)n6
                             - ctx->noise_crackle_low) >> 4;
    crackle = wsrb89_clamp16((wsrb89_s32)n6
                            - ctx->noise_crackle_low);
    gate = ((ctx->rng[WSRB89_NOISE_CRACKLE] & 127U) < 14U)
         ? 32767 : 0;
    crackle = wsrb89_clamp16(((wsrb89_s32)crackle * gate) >> 15);
    crackle_attack = wsrb89_age_attack_gate(ctx, 9U);
    crackle_sum = ((wsrb89_s32)crackle
                * ctx->params.noise_level_q15[WSRB89_NOISE_CRACKLE]) >> 15;
    crackle_sum = (crackle_sum * crackle_env) >> 15;
    crackle_sum = (crackle_sum * crackle_attack) >> 15;
    crackle_filtered = wsrb89_svf_process(&ctx->crackle_svf,
        wsrb89_soft_clip(crackle_sum * 2), WSRB89_SVF_BAND);

    pitch_samples = wsrb89_ms_to_samples(ctx->sample_rate,
                                         ctx->params.sub_pitch_drop_ms);
    start_freq = ctx->params.sub_start_hz_q8;
    end_freq = ctx->params.sub_end_hz_q8;
    if ((pitch_samples > 0U) && (ctx->age_samples < pitch_samples)) {
        remaining = pitch_samples - ctx->age_samples;
        sub_freq = end_freq
                 + (((start_freq - end_freq) * (wsrb89_s32)remaining)
                 / (wsrb89_s32)pitch_samples);
    } else {
        sub_freq = end_freq;
    }
    phase_inc = ((wsrb89_u32)sub_freq * 256U) / ctx->sample_rate;
    phase_step = (wsrb89_s32)phase_inc + ((wsrb89_s32)n3 >> 12);
    if (phase_step < 1) {
        phase_step = 1;
    }
    if (phase_step > 65535) {
        phase_step = 65535;
    }
    ctx->sub_phase_inc = (wsrb89_u16)phase_step;
    ctx->sub_phase = (wsrb89_u16)(ctx->sub_phase + ctx->sub_phase_inc);
    sine = wsrb89_sine_q15(ctx->sub_phase);
    saw = wsrb89_saw_q15(ctx->sub_phase);
    shock = wsrb89_shock_shape(ctx);

    sub_gate = wsrb89_age_gate(ctx,
        (wsrb89_u32)ctx->params.sub_pitch_drop_ms + 180U);
    sub_gate = wsrb89_clamp16(((wsrb89_s32)sub_gate * sub_gate) >> 15);
    sub_attack = wsrb89_age_attack_gate(ctx, 14U);
    sub_mod = 24576 + ((wsrb89_s32)body_low_octave >> 2);
    if (sub_mod < 12288) {
        sub_mod = 12288;
    }
    if (sub_mod > 32767) {
        sub_mod = 32767;
    }
    sub_mix = (((wsrb89_s32)sine * ctx->params.sine_level_q15) >> 15)
            + ((((wsrb89_s32)saw * ctx->params.saw_level_q15) >> 15) >> 2);
    sub_mix = (sub_mix * sub_mod) >> 15;
    sub_mix = (sub_mix * sub_gate) >> 15;
    sub_mix = (sub_mix * sub_attack) >> 15;

    body_sum = ((((wsrb89_s32)body
           * ctx->params.noise_level_q15[WSRB89_NOISE_BODY]) >> 15)
          * master) >> 15;
    body_sum += ((((wsrb89_s32)body_low_octave
           * ctx->params.noise_level_q15[WSRB89_NOISE_BODY_LOW_OCTAVE])
          >> 15) * master) >> 15;
    body_sum += (sub_mix * master) >> 15;
    body_sum += ((wsrb89_s32)rumble_filtered * 3) >> 2;
    body_input = wsrb89_soft_clip(body_sum * 2);
    body_driven = wsrb89_distort(ctx, body_input);
    body_filtered = wsrb89_svf_process(&ctx->main_svf, body_driven,
                                        ctx->params.svf_mode);
    body_equalized = wsrb89_eq6_process_bank(ctx, ctx->eq_svf,
                                             body_filtered);

    high_gate = crack;
    transient_sum = ((((wsrb89_s32)n0
          * ctx->params.noise_level_q15[WSRB89_NOISE_CRACK]) >> 15)
          * crack) >> 15;
    transient_sum += (((((wsrb89_s32)body_high_octave
           * ctx->params.noise_level_q15[WSRB89_NOISE_BODY_HIGH_OCTAVE])
          >> 15) * high_gate) >> 15) >> 1;
    transient_sum += (((((wsrb89_s32)debris
           * ctx->params.noise_level_q15[WSRB89_NOISE_DEBRIS]) >> 15)
          * debris_env) >> 15) >> 1;
    transient_sum += crackle_filtered;
    transient_input = wsrb89_clamp16(transient_sum);
    transient_filtered = wsrb89_svf_process(&ctx->transient_svf,
                                             transient_input,
                                             WSRB89_SVF_LOW);
    transient_equalized = wsrb89_eq6_process_bank(
        ctx, ctx->transient_eq_svf, transient_filtered);

    shock_sum = ((((wsrb89_s32)shock
           * ctx->params.shock_level_q15) >> 15) * crack) >> 15;

    cloud_duration = (wsrb89_u32)ctx->params.decay_ms
                   + (wsrb89_u32)ctx->params.release_ms + 420U;
    cloud_gate = wsrb89_age_gate(ctx, cloud_duration);
    cloud_raw = ((wsrb89_s32)body >> 1)
              + (((wsrb89_s32)body_low_octave * 3) >> 2);
    ctx->room_cloud_state += (cloud_raw - ctx->room_cloud_state) >> 2;
    cloud_sum = (ctx->room_cloud_state * cloud_gate) >> 15;
    cloud_sum = (cloud_sum * 24576) >> 15;

    final_sum = ((wsrb89_s32)body_equalized * 3)
              + ((wsrb89_s32)transient_equalized * 2)
              + (shock_sum * 2)
              + ((wsrb89_s32)rumble_filtered * 2)
              + crackle_filtered;
    final_sum /= 10;

    room_sum = ((wsrb89_s32)body_equalized * 2)
             + (cloud_sum * 2)
             + transient_equalized
             + shock_sum
             + ((wsrb89_s32)rumble_filtered * 3)
             + crackle_filtered;
    room_sum /= 9;
    *room_send = wsrb89_soft_clip(room_sum);

    ctx->age_samples++;
    return wsrb89_soft_clip(final_sum);
}

void wsrb89_init(wsrb89_context *ctx, wsrb89_workspace *workspace,
                 wsrb89_u32 sample_rate, wsrb89_u32 seed)
{
    wsrb89_params preset;
    wsrb89_zero_bytes(ctx, (wsrb89_u32)sizeof(wsrb89_context));
    ctx->workspace = workspace;
    if (sample_rate < 8000U) {
        sample_rate = 8000U;
    }
    if (sample_rate > 48000U) {
        sample_rate = 48000U;
    }
    ctx->sample_rate = sample_rate;
    if (seed == 0U) {
        seed = 0xA341316CU;
    }
    ctx->rng[WSRB89_NOISE_CRACK] = seed;
    ctx->rng[WSRB89_NOISE_BODY] = seed ^ 0xC8013EA4U;
    ctx->rng[WSRB89_NOISE_DEBRIS] = seed ^ 0xAD90777DU;
    ctx->rng[WSRB89_NOISE_BODY_LOW_OCTAVE] = seed ^ 0x7E95761EU;
    ctx->rng[WSRB89_NOISE_BODY_HIGH_OCTAVE] = seed ^ 0x4B7A70E9U;
    ctx->rng[WSRB89_NOISE_RUMBLE] = seed ^ 0x91E10DA5U;
    ctx->rng[WSRB89_NOISE_CRACKLE] = seed ^ 0xD1B54A35U;
    ctx->motion_lfo_phase = (wsrb89_u16)(seed >> 16);
    if (workspace != (wsrb89_workspace *)0) {
        wsrb89_zero_bytes(workspace, (wsrb89_u32)sizeof(wsrb89_workspace));
    }
    wsrb89_get_preset(&preset, WSRB89_PRESET_HEAVY_IMPACT);
    wsrb89_set_params(ctx, &preset);
}

void wsrb89_reset(wsrb89_context *ctx)
{
    wsrb89_params params;
    wsrb89_workspace *workspace;
    wsrb89_u32 sample_rate;
    wsrb89_u32 rng[WSRB89_NOISE_OSCILLATORS];
    wsrb89_u16 i;

    params = ctx->params;
    workspace = ctx->workspace;
    sample_rate = ctx->sample_rate;
    i = 0U;
    while (i < WSRB89_NOISE_OSCILLATORS) {
        rng[i] = ctx->rng[i];
        i++;
    }
    wsrb89_zero_bytes(ctx, (wsrb89_u32)sizeof(wsrb89_context));
    ctx->params = params;
    ctx->workspace = workspace;
    ctx->sample_rate = sample_rate;
    i = 0U;
    while (i < WSRB89_NOISE_OSCILLATORS) {
        ctx->rng[i] = rng[i];
        i++;
    }
    if (workspace != (wsrb89_workspace *)0) {
        wsrb89_zero_bytes(workspace, (wsrb89_u32)sizeof(wsrb89_workspace));
    }
    wsrb89_configure_filters(ctx);
}

void wsrb89_set_params(wsrb89_context *ctx, const wsrb89_params *params)
{
    ctx->params = *params;
    if (ctx->params.sustain_level_q15 > 32767U) {
        ctx->params.sustain_level_q15 = 32767U;
    }
    if (ctx->params.svf_damp_q15 > 32000U) {
        ctx->params.svf_damp_q15 = 32000U;
    }
    if (ctx->params.svf_mode > WSRB89_SVF_NOTCH) {
        ctx->params.svf_mode = WSRB89_SVF_LOW;
    }
    if (ctx->params.rumble_damp_q15 > 32000U) {
        ctx->params.rumble_damp_q15 = 32000U;
    }
    if (ctx->params.crackle_damp_q15 > 32000U) {
        ctx->params.crackle_damp_q15 = 32000U;
    }
    if (ctx->params.motion_lfo_depth_q15 > 32767U) {
        ctx->params.motion_lfo_depth_q15 = 32767U;
    }
    if (ctx->params.drive_q12 < 4096U) {
        ctx->params.drive_q12 = 4096U;
    }
    if (ctx->params.chorus_mix_q15 > 32767U) {
        ctx->params.chorus_mix_q15 = 32767U;
    }
    if (ctx->params.reverb_mix_q15 > 32767U) {
        ctx->params.reverb_mix_q15 = 32767U;
    }
    if (ctx->params.reverb_feedback_q15 > 32000U) {
        ctx->params.reverb_feedback_q15 = 32000U;
    }
    if (ctx->params.reverb_damp_q15 > 32767U) {
        ctx->params.reverb_damp_q15 = 32767U;
    }
    if (ctx->params.output_gain_q12 > 8192U) {
        ctx->params.output_gain_q12 = 8192U;
    }
    wsrb89_configure_filters(ctx);
}

void wsrb89_trigger(wsrb89_context *ctx, wsrb89_u16 velocity_q15)
{
    wsrb89_u32 samples;
    wsrb89_s16 velocity;
    if (velocity_q15 > 32767U) {
        velocity_q15 = 32767U;
    }
    velocity = (wsrb89_s16)velocity_q15;
    ctx->age_samples = 0U;
    ctx->active = 1U;
    ctx->tail_remaining = 0U;
    ctx->sub_phase = 0U;
    samples = wsrb89_ms_to_samples(ctx->sample_rate, ctx->params.attack_ms);
    wsrb89_env_start(&ctx->master_env, 0, velocity, samples);
    ctx->master_env.stage = WSRB89_ENV_ATTACK;
    samples = wsrb89_ms_to_samples(ctx->sample_rate,
                                   ctx->params.crack_decay_ms);
    wsrb89_env_start(&ctx->crack_env, velocity, 0, samples);
    samples = wsrb89_ms_to_samples(ctx->sample_rate,
                                   ctx->params.debris_decay_ms);
    wsrb89_env_start(&ctx->debris_env, velocity, 0, samples);
    samples = wsrb89_ms_to_samples(ctx->sample_rate,
                                   ctx->params.rumble_decay_ms);
    wsrb89_env_start(&ctx->rumble_env, velocity, 0, samples);
    samples = wsrb89_ms_to_samples(ctx->sample_rate,
                                   ctx->params.crackle_decay_ms);
    wsrb89_env_start(&ctx->crackle_env, velocity, 0, samples);
}

void wsrb89_release(wsrb89_context *ctx)
{
    wsrb89_s16 current;
    wsrb89_u32 samples;
    if (ctx->master_env.stage == WSRB89_ENV_OFF) {
        return;
    }
    current = wsrb89_clamp16(ctx->master_env.level_q16 >> 16);
    samples = wsrb89_ms_to_samples(ctx->sample_rate, ctx->params.release_ms);
    wsrb89_env_start(&ctx->master_env, current, 0, samples);
    ctx->master_env.stage = WSRB89_ENV_RELEASE;
}

void wsrb89_render_stereo(wsrb89_context *ctx, wsrb89_s16 *left,
                          wsrb89_s16 *right, wsrb89_u32 frames)
{
    wsrb89_u32 i;
    wsrb89_s16 mono;
    wsrb89_s16 room_send;
    wsrb89_s16 chor_l;
    wsrb89_s16 chor_r;
    wsrb89_s16 early_l;
    wsrb89_s16 early_r;
    wsrb89_s16 late_l;
    wsrb89_s16 late_r;
    wsrb89_s16 mixed_l;
    wsrb89_s16 mixed_r;
    wsrb89_s32 room_mix;
    wsrb89_s32 late_mix;
    wsrb89_s32 sum_l;
    wsrb89_s32 sum_r;
    wsrb89_s32 gain;

    if (ctx->workspace == (wsrb89_workspace *)0) {
        i = 0U;
        while (i < frames) {
            left[i] = 0;
            right[i] = 0;
            i++;
        }
        return;
    }

    i = 0U;
    while (i < frames) {
        room_send = 0;
        if (ctx->active != 0U) {
            mono = wsrb89_render_voice_sample(ctx, &room_send);
        } else {
            mono = 0;
        }

        wsrb89_chorus_tick(ctx, room_send, &chor_l, &chor_r);
        wsrb89_early_tick(ctx, room_send, &early_l, &early_r);
        wsrb89_reverb_tick(ctx, chor_l, chor_r, &late_l, &late_r);

        room_mix = ctx->params.reverb_mix_q15;
        late_mix = room_mix + (room_mix >> 1);
        if (late_mix > 24576) {
            late_mix = 24576;
        }
        sum_l = (((wsrb89_s32)mono * 28672) >> 15)
              + (((wsrb89_s32)early_l * room_mix) >> 15)
              + (((wsrb89_s32)late_l * late_mix) >> 15);
        sum_r = (((wsrb89_s32)mono * 28672) >> 15)
              + (((wsrb89_s32)early_r * room_mix) >> 15)
              + (((wsrb89_s32)late_r * late_mix) >> 15);
        mixed_l = wsrb89_clamp16(sum_l);
        mixed_r = wsrb89_clamp16(sum_r);

        if ((ctx->active == 0U) && (ctx->tail_remaining > 0U)) {
            ctx->tail_remaining--;
        }
        gain = ctx->params.output_gain_q12;
        mixed_l = wsrb89_clamp16(((wsrb89_s32)mixed_l * gain) >> 12);
        mixed_r = wsrb89_clamp16(((wsrb89_s32)mixed_r * gain) >> 12);
        mixed_l = wsrb89_dc_block(mixed_l, &ctx->dc_x_l, &ctx->dc_y_l);
        mixed_r = wsrb89_dc_block(mixed_r, &ctx->dc_x_r, &ctx->dc_y_r);
        left[i] = wsrb89_soft_clip(mixed_l);
        right[i] = wsrb89_soft_clip(mixed_r);
        i++;
    }
}

wsrb89_u16 wsrb89_is_active(const wsrb89_context *ctx)
{
    return (wsrb89_u16)((ctx->active != 0U)
                       || (ctx->tail_remaining != 0U));
}

wsrb89_u32 wsrb89_workspace_bytes(void)
{
    return (wsrb89_u32)sizeof(wsrb89_workspace);
}

wsrb89_u32 wsrb89_context_bytes(void)
{
    return (wsrb89_u32)sizeof(wsrb89_context);
}
