#include "chuecka89.h"

static ch89_i32 ch89_abs32(ch89_i32 v)
{
    return (v < 0) ? -v : v;
}

static ch89_i16 ch89_clip16(ch89_i32 v)
{
    if (v > 32767) return (ch89_i16)32767;
    if (v < -32768) return (ch89_i16)-32768;
    return (ch89_i16)v;
}

static ch89_i32 ch89_mul_q15(ch89_i32 a, ch89_i32 b)
{
    return (a * b) / CH89_Q15_ONE;
}

static ch89_u32 ch89_ms_to_frames(ch89_u32 ms, ch89_u32 sample_rate)
{
    return (ms * sample_rate) / 1000U;
}

static void ch89_zero_i16(ch89_i16 *p, ch89_u32 count)
{
    ch89_u32 i;
    for (i = 0U; i < count; ++i) p[i] = 0;
}

static ch89_u32 ch89_rand(ch89_context *ctx)
{
    ch89_u32 x;
    x = ctx->rng;
    if (x == 0U) x = 0x6D2B79F5U;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    ctx->rng = x;
    return x;
}

static ch89_i32 ch89_noise(
    ch89_context *ctx,
    ch89_voice_state *state,
    ch89_u16 color
)
{
    ch89_i32 raw;
    raw = (ch89_i32)((ch89_rand(ctx) >> 16) & 65535U) - 32768;

    if (color == CH89_NOISE_DARK) {
        state->dark_state += (raw - state->dark_state) >> 3;
        return state->dark_state;
    }

    if (color == CH89_NOISE_BRIGHT) {
        state->bright_lp += (raw - state->bright_lp) >> 2;
        return raw - state->bright_lp;
    }

    return raw;
}

static ch89_u32 ch89_layer_frames(const ch89_layer *layer, ch89_u32 sample_rate)
{
    ch89_u32 total_ms;
    total_ms = (ch89_u32)layer->delay_ms;
    total_ms += (ch89_u32)layer->attack_ms;
    total_ms += (ch89_u32)layer->decay_ms;
    total_ms += (ch89_u32)layer->sustain_ms;
    total_ms += (ch89_u32)layer->release_ms;
    return ch89_ms_to_frames(total_ms, sample_rate);
}

static ch89_i32 ch89_envelope(
    const ch89_layer *layer,
    ch89_u32 local_frame,
    ch89_u32 sample_rate
)
{
    ch89_u32 delay;
    ch89_u32 attack;
    ch89_u32 decay;
    ch89_u32 sustain;
    ch89_u32 release;
    ch89_u32 p;
    ch89_i32 env;
    ch89_i32 sustain_level;

    delay = ch89_ms_to_frames((ch89_u32)layer->delay_ms, sample_rate);
    if (local_frame < delay) return 0;
    p = local_frame - delay;

    attack = ch89_ms_to_frames((ch89_u32)layer->attack_ms, sample_rate);
    decay = ch89_ms_to_frames((ch89_u32)layer->decay_ms, sample_rate);
    sustain = ch89_ms_to_frames((ch89_u32)layer->sustain_ms, sample_rate);
    release = ch89_ms_to_frames((ch89_u32)layer->release_ms, sample_rate);
    sustain_level = (ch89_i32)layer->sustain_q15;

    if (attack != 0U && p < attack) {
        return (ch89_i32)((p * (ch89_u32)CH89_Q15_ONE) / attack);
    }
    if (attack != 0U) p -= attack;

    if (decay != 0U && p < decay) {
        env = CH89_Q15_ONE -
            (ch89_i32)((p * (ch89_u32)(CH89_Q15_ONE - sustain_level)) / decay);
        return env;
    }
    if (decay != 0U) p -= decay;

    if (p < sustain) return sustain_level;
    p -= sustain;

    if (release != 0U && p < release) {
        return sustain_level -
            (ch89_i32)((p * (ch89_u32)sustain_level) / release);
    }

    return 0;
}

static ch89_i32 ch89_distort(ch89_i32 x, ch89_i16 amount_q15)
{
    ch89_i32 drive;
    ch89_i32 y;
    ch89_i32 sign;
    ch89_i32 mag;
    ch89_i32 threshold;

    drive = CH89_Q15_ONE + (ch89_i32)amount_q15;
    y = ch89_mul_q15(x, drive);
    sign = (y < 0) ? -1 : 1;
    mag = ch89_abs32(y);
    threshold = 9300;
    if (mag > threshold) {
        mag = threshold + ((mag - threshold) >> 2);
    }
    if (mag > 32767) mag = 32767;
    return sign * mag;
}

static ch89_i32 ch89_eq_process(
    ch89_context *ctx,
    ch89_eq_state *state,
    ch89_i32 x,
    const ch89_i16 gain_q15[CH89_EQ_BANDS]
)
{
    static const ch89_i16 alpha_44100[5] = {830, 2035, 4753, 10144, 18830};
    static const ch89_i16 alpha_48000[5] = {763, 1874, 4394, 9452, 17827};
    const ch89_i16 *alpha;
    ch89_i32 band[CH89_EQ_BANDS];
    ch89_i32 sum;
    ch89_u16 i;

    alpha = (ctx->sample_rate == 48000U) ? alpha_48000 : alpha_44100;

    for (i = 0U; i < 5U; ++i) {
        state->lp[i] += ch89_mul_q15(x - state->lp[i], alpha[i]);
    }

    band[0] = state->lp[0];
    band[1] = state->lp[1] - state->lp[0];
    band[2] = state->lp[2] - state->lp[1];
    band[3] = state->lp[3] - state->lp[2];
    band[4] = state->lp[4] - state->lp[3];
    band[5] = x - state->lp[4];

    sum = 0;
    for (i = 0U; i < CH89_EQ_BANDS; ++i) {
        sum += ch89_mul_q15(band[i], gain_q15[i]);
    }
    return sum;
}

static void ch89_voice_eq(
    const ch89_voice *voice,
    ch89_u32 local_frame,
    ch89_u32 sample_rate,
    ch89_i16 out_gain[CH89_EQ_BANDS]
)
{
    ch89_u32 delay;
    ch89_u32 sweep;
    ch89_u32 p;
    ch89_i32 mix;
    ch89_i32 d;
    ch89_u16 i;

    delay = ch89_ms_to_frames((ch89_u32)voice->envelope.delay_ms, sample_rate);
    if (local_frame <= delay || voice->eq_sweep_ms == 0U) {
        for (i = 0U; i < CH89_EQ_BANDS; ++i) {
            out_gain[i] = voice->eq_start_q15[i];
        }
        return;
    }

    sweep = ch89_ms_to_frames((ch89_u32)voice->eq_sweep_ms, sample_rate);
    if (sweep == 0U) sweep = 1U;
    p = local_frame - delay;
    if (p >= sweep) mix = CH89_Q15_ONE;
    else mix = (ch89_i32)((p * (ch89_u32)CH89_Q15_ONE) / sweep);

    for (i = 0U; i < CH89_EQ_BANDS; ++i) {
        d = (ch89_i32)voice->eq_end_q15[i] -
            (ch89_i32)voice->eq_start_q15[i];
        out_gain[i] = (ch89_i16)(
            (ch89_i32)voice->eq_start_q15[i] + ch89_mul_q15(d, mix)
        );
    }
}

static ch89_i32 ch89_voice_sample(
    ch89_context *ctx,
    ch89_voice_state *state,
    const ch89_voice *voice,
    ch89_u32 local_frame,
    ch89_u16 effect_flags
)
{
    ch89_i32 env;
    ch89_i32 n;
    ch89_i16 gains[CH89_EQ_BANDS];

    env = ch89_envelope(&voice->envelope, local_frame, ctx->sample_rate);
    if (env == 0) return 0;

    n = ch89_noise(ctx, state, voice->envelope.color);
    n = ch89_mul_q15(n, env);
    n = ch89_mul_q15(n, (ch89_i32)voice->envelope.level_q15);
    if ((effect_flags & CH89_EFFECT_DISTORTION) != 0U) {
        n = ch89_distort(n, voice->distortion_q15);
    }
    if ((effect_flags & CH89_EFFECT_EQ) != 0U) {
        ch89_voice_eq(voice, local_frame, ctx->sample_rate, gains);
        return ch89_eq_process(ctx, &state->eq, n, gains);
    }
    return n;
}

static ch89_i32 ch89_chorus_process(
    ch89_context *ctx,
    ch89_i32 input,
    const ch89_gesture *gesture
)
{
    ch89_u16 phase;
    ch89_u16 tri;
    ch89_u16 delay;
    ch89_u16 read_index;
    ch89_u16 base_delay;
    ch89_i32 delayed;
    ch89_i32 wet;
    ch89_i32 dry;

    ctx->chorus[ctx->chorus_write] = ch89_clip16(input);
    phase = ctx->chorus_phase;
    tri = (phase < 32768U) ? phase : (ch89_u16)(65535U - phase);
    delay = (ch89_u16)(((ch89_u32)tri * gesture->chorus_depth_samples) / 32768U);
    base_delay = (ch89_u16)(ctx->sample_rate / 125U);
    delay = (ch89_u16)(base_delay + delay);
    if (delay >= CH89_CHORUS_BUFFER) delay = CH89_CHORUS_BUFFER - 1U;

    if (ctx->chorus_write >= delay) {
        read_index = (ch89_u16)(ctx->chorus_write - delay);
    } else {
        read_index = (ch89_u16)(CH89_CHORUS_BUFFER + ctx->chorus_write - delay);
    }
    delayed = ctx->chorus[read_index];

    wet = ch89_mul_q15(delayed, gesture->chorus_wet_q15);
    dry = ch89_mul_q15(input, CH89_Q15_ONE - gesture->chorus_wet_q15);

    ctx->chorus_write++;
    if (ctx->chorus_write >= CH89_CHORUS_BUFFER) ctx->chorus_write = 0U;
    ctx->chorus_phase = (ch89_u16)(ctx->chorus_phase + gesture->chorus_rate_step);

    return dry + wet;
}

static ch89_i32 ch89_reverb_process(
    ch89_context *ctx,
    ch89_i32 input,
    const ch89_gesture *gesture
)
{
    ch89_i32 a;
    ch89_i32 b;
    ch89_i32 feedback_a;
    ch89_i32 feedback_b;
    ch89_i32 wet;
    ch89_i32 dry;
    ch89_i32 write_a;
    ch89_i32 write_b;
    ch89_u16 len_a;
    ch89_u16 len_b;

    len_a = (ctx->sample_rate == 48000U) ? 1631U : 1499U;
    len_b = (ctx->sample_rate == 48000U) ? 2293U : 2111U;
    a = ctx->reverb_a[ctx->reverb_a_write];
    b = ctx->reverb_b[ctx->reverb_b_write];

    feedback_a = ch89_mul_q15(a, 7200);
    feedback_b = ch89_mul_q15(b, 6100);
    write_a = input + feedback_a;
    write_b = input + feedback_b;
    ctx->reverb_a[ctx->reverb_a_write] = ch89_clip16(write_a);
    ctx->reverb_b[ctx->reverb_b_write] = ch89_clip16(write_b);

    ctx->reverb_a_write++;
    if (ctx->reverb_a_write >= len_a) ctx->reverb_a_write = 0U;
    ctx->reverb_b_write++;
    if (ctx->reverb_b_write >= len_b) ctx->reverb_b_write = 0U;

    wet = ch89_mul_q15((a + b) >> 1, gesture->reverb_wet_q15);
    dry = ch89_mul_q15(input, CH89_Q15_ONE - gesture->reverb_wet_q15);
    return dry + wet;
}

static void ch89_copy_eq(
    ch89_i16 dst[CH89_EQ_BANDS],
    const ch89_i16 src[CH89_EQ_BANDS]
)
{
    ch89_u16 i;
    for (i = 0U; i < CH89_EQ_BANDS; ++i) dst[i] = src[i];
}

static void ch89_eq_sh_open(ch89_i16 eq[CH89_EQ_BANDS])
{
    /* Physics pass: keep the mid-high scrape, trim the loose air band. */
    eq[0] = 1200; eq[1] = 5600; eq[2] = 15200;
    eq[3] = 32767; eq[4] = 27000; eq[5] = 8200;
}

static void ch89_eq_sh_close(ch89_i16 eq[CH89_EQ_BANDS])
{
    eq[0] = 2500; eq[1] = 10500; eq[2] = 25500;
    eq[3] = 27500; eq[4] = 13000; eq[5] = 2200;
}

static void ch89_eq_eckt_open(ch89_i16 eq[CH89_EQ_BANDS])
{
    eq[0] = 2200; eq[1] = 13000; eq[2] = 32767;
    eq[3] = 30000; eq[4] = 22000; eq[5] = 7000;
}

static void ch89_eq_eckt_close(ch89_i16 eq[CH89_EQ_BANDS])
{
    eq[0] = 4500; eq[1] = 24500; eq[2] = 32767;
    eq[3] = 17000; eq[4] = 6000; eq[5] = 1200;
}

static void ch89_eq_bright_open(ch89_i16 eq[CH89_EQ_BANDS])
{
    eq[0] = 900; eq[1] = 4200; eq[2] = 12500;
    eq[3] = 28000; eq[4] = 32767; eq[5] = 18000;
}

static void ch89_eq_bright_close(ch89_i16 eq[CH89_EQ_BANDS])
{
    eq[0] = 1600; eq[1] = 8000; eq[2] = 22000;
    eq[3] = 32767; eq[4] = 19000; eq[5] = 4500;
}

static void ch89_eq_dark_open(ch89_i16 eq[CH89_EQ_BANDS])
{
    eq[0] = 7500; eq[1] = 25500; eq[2] = 32767;
    eq[3] = 22000; eq[4] = 8500; eq[5] = 1800;
}

static void ch89_eq_dark_close(ch89_i16 eq[CH89_EQ_BANDS])
{
    eq[0] = 12000; eq[1] = 32767; eq[2] = 27000;
    eq[3] = 11000; eq[4] = 2800; eq[5] = 500;
}

static void ch89_eq_hollow_open(ch89_i16 eq[CH89_EQ_BANDS])
{
    eq[0] = 10500; eq[1] = 32767; eq[2] = 17500;
    eq[3] = 8500; eq[4] = 23500; eq[5] = 7000;
}

static void ch89_eq_hollow_close(ch89_i16 eq[CH89_EQ_BANDS])
{
    eq[0] = 14500; eq[1] = 30000; eq[2] = 12500;
    eq[3] = 5000; eq[4] = 11500; eq[5] = 1600;
}

static void ch89_set_layer(
    ch89_layer *layer,
    ch89_u16 delay_ms,
    ch89_u16 attack_ms,
    ch89_u16 decay_ms,
    ch89_u16 sustain_ms,
    ch89_u16 release_ms,
    ch89_i16 sustain_q15,
    ch89_i16 level_q15,
    ch89_u16 color
)
{
    layer->delay_ms = delay_ms;
    layer->attack_ms = attack_ms;
    layer->decay_ms = decay_ms;
    layer->sustain_ms = sustain_ms;
    layer->release_ms = release_ms;
    layer->sustain_q15 = sustain_q15;
    layer->level_q15 = level_q15;
    layer->color = color;
}

static void ch89_set_voice(
    ch89_voice *voice,
    ch89_u16 delay_ms,
    ch89_u16 attack_ms,
    ch89_u16 decay_ms,
    ch89_u16 sustain_ms,
    ch89_u16 release_ms,
    ch89_i16 sustain_q15,
    ch89_i16 level_q15,
    ch89_u16 color,
    const ch89_i16 eq_start[CH89_EQ_BANDS],
    const ch89_i16 eq_end[CH89_EQ_BANDS],
    ch89_u16 eq_sweep_ms,
    ch89_i16 distortion_q15
)
{
    ch89_set_layer(
        &voice->envelope,
        delay_ms,
        attack_ms,
        decay_ms,
        sustain_ms,
        release_ms,
        sustain_q15,
        level_q15,
        color
    );
    ch89_copy_eq(voice->eq_start_q15, eq_start);
    ch89_copy_eq(voice->eq_end_q15, eq_end);
    voice->eq_sweep_ms = eq_sweep_ms;
    voice->distortion_q15 = distortion_q15;
}

static void ch89_clear_gesture(ch89_gesture *g)
{
    ch89_u16 i;
    ch89_u16 j;
    ch89_u16 v;

    g->stroke_count = 0U;
    g->effect_flags = CH89_EFFECT_ALL;
    g->duration_ms = 0U;
    g->chorus_depth_samples = 14U;
    g->chorus_rate_step = 29U;
    g->chorus_wet_q15 = 1100;
    g->reverb_wet_q15 = 140;
    g->output_gain_q15 = 30200;

    for (i = 0U; i < CH89_MAX_STROKES; ++i) {
        g->strokes[i].start_ms = 0U;
        for (v = 0U; v < CH89_VOICES_PER_STROKE; ++v) {
            ch89_voice *voice;
            voice = (v == CH89_VOICE_SH) ?
                &g->strokes[i].sh : &g->strokes[i].eckt;
            ch89_set_layer(&voice->envelope, 0U, 0U, 0U, 0U, 0U,
                0, 0, CH89_NOISE_RAW);
            voice->eq_sweep_ms = 0U;
            voice->distortion_q15 = 0;
            for (j = 0U; j < CH89_EQ_BANDS; ++j) {
                voice->eq_start_q15[j] = 0;
                voice->eq_end_q15[j] = 0;
            }
        }
    }
}

static ch89_i16 ch89_scaled(ch89_i16 base, ch89_u16 intensity_q15)
{
    return (ch89_i16)ch89_mul_q15(base, intensity_q15);
}

static void ch89_select_eq_pair(
    ch89_u16 kind,
    ch89_i16 start_eq[CH89_EQ_BANDS],
    ch89_i16 end_eq[CH89_EQ_BANDS]
)
{
    if (kind == 0U) {
        ch89_eq_bright_open(start_eq);
        ch89_eq_bright_close(end_eq);
    } else if (kind == 1U) {
        ch89_eq_dark_open(start_eq);
        ch89_eq_dark_close(end_eq);
    } else if (kind == 2U) {
        ch89_eq_sh_open(start_eq);
        ch89_eq_sh_close(end_eq);
    } else {
        ch89_eq_hollow_open(start_eq);
        ch89_eq_hollow_close(end_eq);
    }
}

static ch89_result ch89_add_shekt(
    ch89_gesture *g,
    ch89_u16 start_ms,
    ch89_u16 sh_attack_ms,
    ch89_u16 sh_decay_ms,
    ch89_u16 sh_sustain_ms,
    ch89_u16 sh_release_ms,
    ch89_u16 eckt_delay_ms,
    ch89_u16 eckt_decay_ms,
    ch89_u16 eckt_release_ms,
    ch89_i16 sh_level,
    ch89_i16 eckt_level,
    ch89_u16 sh_color,
    ch89_u16 eckt_color,
    ch89_u16 sh_eq_kind,
    ch89_u16 eckt_eq_kind,
    ch89_i16 sh_distortion,
    ch89_i16 eckt_distortion
)
{
    ch89_stroke *stroke;
    ch89_i16 sh_start_eq[CH89_EQ_BANDS];
    ch89_i16 sh_end_eq[CH89_EQ_BANDS];
    ch89_i16 eckt_start_eq[CH89_EQ_BANDS];
    ch89_i16 eckt_end_eq[CH89_EQ_BANDS];
    ch89_u16 sh_end_ms;
    ch89_u16 eckt_end_ms;
    ch89_u16 end_ms;

    if (g->stroke_count >= CH89_MAX_STROKES) return CH89_TOO_MANY_STROKES;

    ch89_select_eq_pair(sh_eq_kind, sh_start_eq, sh_end_eq);
    if (eckt_eq_kind == 4U) {
        ch89_eq_eckt_open(eckt_start_eq);
        ch89_eq_eckt_close(eckt_end_eq);
    } else {
        ch89_select_eq_pair(eckt_eq_kind, eckt_start_eq, eckt_end_eq);
    }

    stroke = &g->strokes[g->stroke_count++];
    stroke->start_ms = start_ms;

    ch89_set_voice(
        &stroke->sh,
        0U,
        sh_attack_ms,
        sh_decay_ms,
        sh_sustain_ms,
        sh_release_ms,
        5200,
        sh_level,
        sh_color,
        sh_start_eq,
        sh_end_eq,
        (ch89_u16)(sh_attack_ms + sh_decay_ms),
        sh_distortion
    );

    ch89_set_voice(
        &stroke->eckt,
        eckt_delay_ms,
        0U,
        eckt_decay_ms,
        0U,
        eckt_release_ms,
        0,
        eckt_level,
        eckt_color,
        eckt_start_eq,
        eckt_end_eq,
        eckt_decay_ms,
        eckt_distortion
    );

    sh_end_ms = (ch89_u16)(
        sh_attack_ms + sh_decay_ms + sh_sustain_ms + sh_release_ms
    );
    eckt_end_ms = (ch89_u16)(
        eckt_delay_ms + eckt_decay_ms + eckt_release_ms
    );
    end_ms = (sh_end_ms > eckt_end_ms) ? sh_end_ms : eckt_end_ms;
    end_ms = (ch89_u16)(start_ms + end_ms + 24U);
    if (end_ms > g->duration_ms) g->duration_ms = end_ms;

    return CH89_OK;
}

static ch89_result ch89_add_sh_only(
    ch89_gesture *g,
    ch89_u16 start_ms,
    ch89_i16 level,
    ch89_u16 eq_kind
)
{
    ch89_result r;
    r = ch89_add_shekt(
        g, start_ms,
        30U, 62U, 7U, 12U,
        82U, 10U, 2U,
        level, 0,
        CH89_NOISE_BRIGHT, CH89_NOISE_RAW,
        eq_kind, 4U,
        15000, 0
    );
    return r;
}

static ch89_result ch89_add_eckt_only(
    ch89_gesture *g,
    ch89_u16 start_ms,
    ch89_i16 level
)
{
    ch89_result r;
    r = ch89_add_shekt(
        g, start_ms,
        0U, 0U, 0U, 0U,
        0U, 15U, 2U,
        0, level,
        CH89_NOISE_RAW, CH89_NOISE_RAW,
        2U, 4U,
        0, 20500
    );
    return r;
}

static ch89_result ch89_add_contact(
    ch89_gesture *g,
    ch89_u16 start_ms,
    ch89_u16 decay_ms,
    ch89_u16 release_ms,
    ch89_i16 level,
    ch89_u16 color,
    ch89_u16 eq_kind,
    ch89_i16 distortion_q15
)
{
    return ch89_add_shekt(
        g, start_ms,
        0U, 0U, 0U, 0U,
        0U, decay_ms, release_ms,
        0, level,
        CH89_NOISE_RAW, color,
        2U, eq_kind,
        0, distortion_q15
    );
}

static ch89_i16 ch89_timing_jitter(ch89_u16 index)
{
    static const ch89_i16 pattern[8] = {0, 3, -2, 1, -1, 2, -3, 1};
    return pattern[index & 7U];
}

ch89_result ch89_init(ch89_context *ctx, ch89_u32 sample_rate, ch89_u32 seed)
{
    if (ctx == 0) return CH89_BAD_ARGUMENT;
    if (sample_rate != 44100U && sample_rate != 48000U) {
        return CH89_UNSUPPORTED_RATE;
    }
    ctx->sample_rate = sample_rate;
    ch89_reset(ctx, seed);
    return CH89_OK;
}

void ch89_reset(ch89_context *ctx, ch89_u32 seed)
{
    ch89_u16 i;
    ch89_u16 j;
    ch89_u16 v;

    if (ctx == 0) return;
    ctx->rng = (seed == 0U) ? 0xC89C89A5U : seed;
    ctx->chorus_write = 0U;
    ctx->chorus_phase = 0U;
    ctx->reverb_a_write = 0U;
    ctx->reverb_b_write = 0U;

    for (i = 0U; i < CH89_MAX_STROKES; ++i) {
        for (v = 0U; v < CH89_VOICES_PER_STROKE; ++v) {
            ctx->voice_state[i][v].dark_state = 0;
            ctx->voice_state[i][v].bright_lp = 0;
            for (j = 0U; j < 5U; ++j) {
                ctx->voice_state[i][v].eq.lp[j] = 0;
            }
        }
    }

    ch89_zero_i16(ctx->chorus, CH89_CHORUS_BUFFER);
    ch89_zero_i16(ctx->reverb_a, CH89_REVERB_A_BUFFER);
    ch89_zero_i16(ctx->reverb_b, CH89_REVERB_B_BUFFER);
}

ch89_result ch89_make_preset(
    ch89_preset preset,
    ch89_u16 repetitions,
    ch89_u16 intensity_q15,
    ch89_gesture *g
)
{
    ch89_u16 i;
    ch89_u16 count;
    ch89_u16 interval;
    ch89_u16 start_ms;
    ch89_i16 jitter;
    ch89_i16 sh;
    ch89_i16 eckt;
    ch89_result r;

    if (g == 0) return CH89_BAD_ARGUMENT;
    if (preset < 0 || preset >= CH89_PRESET_COUNT) return CH89_BAD_ARGUMENT;
    if (intensity_q15 == 0U) intensity_q15 = CH89_Q15_ONE;
    if (intensity_q15 > CH89_Q15_ONE) intensity_q15 = CH89_Q15_ONE;

    ch89_clear_gesture(g);
    sh = ch89_scaled(19800, intensity_q15);
    eckt = ch89_scaled(30500, intensity_q15);
    r = CH89_OK;

    if (preset == CH89_PRESET_SHOTGUN_PUMP) {
        /* Rearward: slide friction, extraction/ejection, rear stop. */
        g->chorus_depth_samples = 11U;
        g->chorus_wet_q15 = 900;
        g->reverb_wet_q15 = 105;
        r = ch89_add_shekt(
            g, 0U,
            30U, 50U, 5U, 10U,
            74U, 12U, 2U,
            sh, eckt,
            CH89_NOISE_BRIGHT, CH89_NOISE_RAW,
            2U, 4U,
            14500, 21000
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 91U, 7U, 1U,
            ch89_scaled(10800, intensity_q15),
            CH89_NOISE_BRIGHT, 0U, 12500
        );
        if (r != CH89_OK) return r;

        /* Forward: carrier/round movement, chambering and bolt lock. */
        r = ch89_add_contact(
            g, 187U, 6U, 1U,
            ch89_scaled(7600, intensity_q15),
            CH89_NOISE_BRIGHT, 2U, 10500
        );
        if (r != CH89_OK) return r;
        r = ch89_add_shekt(
            g, 145U,
            24U, 48U, 5U, 10U,
            70U, 17U, 3U,
            ch89_scaled(20500, intensity_q15),
            ch89_scaled(32700, intensity_q15),
            CH89_NOISE_DARK, CH89_NOISE_DARK,
            1U, 1U,
            15800, 24000
        );
        g->duration_ms = (ch89_u16)(g->duration_ms + 48U);
    } else if (preset == CH89_PRESET_SHOTGUN_INSERT) {
        /* Shell body/rim slides past the stop, then the stop snaps home. */
        g->chorus_depth_samples = 9U;
        g->chorus_wet_q15 = 620;
        g->reverb_wet_q15 = 70;
        r = ch89_add_shekt(
            g, 0U,
            24U, 34U, 3U, 8U,
            56U, 11U, 2U,
            ch89_scaled(18600, intensity_q15),
            ch89_scaled(28600, intensity_q15),
            CH89_NOISE_DARK, CH89_NOISE_RAW,
            2U, 4U,
            13000, 19200
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 70U, 6U, 1U,
            ch89_scaled(8600, intensity_q15),
            CH89_NOISE_BRIGHT, 0U, 11500
        );
        g->duration_ms = (ch89_u16)(g->duration_ms + 38U);
    } else if (preset == CH89_PRESET_SHOTGUN_MULTI_INSERT) {
        count = (repetitions == 0U) ? 6U : repetitions;
        if (count > (CH89_MAX_STROKES / 2U)) {
            count = (CH89_MAX_STROKES / 2U);
        }
        interval = 145U;
        g->chorus_depth_samples = 8U;
        g->chorus_wet_q15 = 520;
        g->reverb_wet_q15 = 55;
        for (i = 0U; i < count; ++i) {
            jitter = ch89_timing_jitter(i);
            start_ms = (ch89_u16)((ch89_i32)(i * interval) + jitter);
            r = ch89_add_shekt(
                g, start_ms,
                17U, 28U, 2U, 7U,
                47U, 9U, 1U,
                ch89_scaled((ch89_i16)(15800 + ((i & 1U) ? 650 : 0)), intensity_q15),
                ch89_scaled((ch89_i16)(24400 + ((i % 3U) * 650)), intensity_q15),
                (i & 1U) ? CH89_NOISE_BRIGHT : CH89_NOISE_DARK,
                CH89_NOISE_RAW,
                (i & 1U) ? 0U : 2U,
                4U,
                12300, 18200
            );
            if (r != CH89_OK) return r;
            r = ch89_add_contact(
                g, (ch89_u16)(start_ms + 58U), 5U, 1U,
                ch89_scaled((ch89_i16)(6800 + ((i & 1U) ? 500 : 0)), intensity_q15),
                CH89_NOISE_BRIGHT, 0U, 10000
            );
            if (r != CH89_OK) return r;
        }
        g->duration_ms = (ch89_u16)(g->duration_ms + 42U);
    } else if (preset == CH89_PRESET_PISTOL_SLIDE) {
        /* Rearward extraction/ejection, then forward strip/chamber/battery. */
        g->chorus_depth_samples = 8U;
        g->chorus_wet_q15 = 500;
        g->reverb_wet_q15 = 55;
        r = ch89_add_shekt(
            g, 0U,
            20U, 36U, 3U, 8U,
            56U, 10U, 2U,
            ch89_scaled(17000, intensity_q15),
            ch89_scaled(27500, intensity_q15),
            CH89_NOISE_BRIGHT, CH89_NOISE_RAW,
            0U, 4U,
            12800, 19500
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 67U, 6U, 1U,
            ch89_scaled(9300, intensity_q15),
            CH89_NOISE_BRIGHT, 0U, 11200
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 118U, 5U, 1U,
            ch89_scaled(6500, intensity_q15),
            CH89_NOISE_BRIGHT, 2U, 9500
        );
        if (r != CH89_OK) return r;
        r = ch89_add_shekt(
            g, 92U,
            12U, 26U, 2U, 6U,
            36U, 13U, 2U,
            ch89_scaled(16000, intensity_q15),
            ch89_scaled(31500, intensity_q15),
            CH89_NOISE_DARK, CH89_NOISE_RAW,
            2U, 4U,
            14200, 22800
        );
        g->duration_ms = (ch89_u16)(g->duration_ms + 34U);
    } else if (preset == CH89_PRESET_MAGNUM_HEAVY_ACTION) {
        /* Heavy reciprocating action: longer travel, stiffer return, harder lock. */
        g->chorus_depth_samples = 9U;
        g->chorus_wet_q15 = 560;
        g->reverb_wet_q15 = 80;
        r = ch89_add_shekt(
            g, 0U,
            34U, 64U, 6U, 14U,
            94U, 18U, 3U,
            ch89_scaled(21500, intensity_q15),
            ch89_scaled(32700, intensity_q15),
            CH89_NOISE_DARK, CH89_NOISE_DARK,
            1U, 1U,
            17800, 24800
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 113U, 7U, 1U,
            ch89_scaled(9800, intensity_q15),
            CH89_NOISE_BRIGHT, 0U, 12000
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 190U, 7U, 1U,
            ch89_scaled(7200, intensity_q15),
            CH89_NOISE_BRIGHT, 2U, 10500
        );
        if (r != CH89_OK) return r;
        r = ch89_add_shekt(
            g, 152U,
            20U, 42U, 4U, 10U,
            58U, 21U, 3U,
            ch89_scaled(20500, intensity_q15),
            ch89_scaled(32700, intensity_q15),
            CH89_NOISE_DARK, CH89_NOISE_DARK,
            3U, 1U,
            19000, 26800
        );
        g->duration_ms = (ch89_u16)(g->duration_ms + 58U);
    } else if (preset == CH89_PRESET_REVOLVER_CYLINDER) {
        /* Hand/ratchet indexing followed by the cylinder stop entering its notch. */
        count = (repetitions == 0U) ? 6U : repetitions;
        if (count > 12U) count = 12U;
        g->chorus_depth_samples = 6U;
        g->chorus_wet_q15 = 320;
        g->reverb_wet_q15 = 35;
        interval = 61U;
        for (i = 0U; i < count; ++i) {
            jitter = ch89_timing_jitter(i);
            start_ms = (ch89_u16)((ch89_i32)(i * interval) + jitter);
            r = ch89_add_contact(
                g, start_ms, 6U, 1U,
                ch89_scaled((ch89_i16)(15000 + (i * 420)), intensity_q15),
                CH89_NOISE_BRIGHT, 0U, 13800
            );
            if (r != CH89_OK) return r;
        }
        r = ch89_add_contact(
            g, (ch89_u16)(count * interval + 18U), 16U, 2U,
            ch89_scaled(30500, intensity_q15),
            CH89_NOISE_DARK, 1U, 22000
        );
        g->duration_ms = (ch89_u16)(g->duration_ms + 34U);
    } else if (preset == CH89_PRESET_GRENADE_LAUNCHER) {
        /* M203-style sequence: release, slide open, insert, slide closed, lock. */
        g->chorus_depth_samples = 8U;
        g->chorus_wet_q15 = 520;
        g->reverb_wet_q15 = 65;
        r = ch89_add_contact(
            g, 0U, 8U, 1U,
            ch89_scaled(15000, intensity_q15),
            CH89_NOISE_BRIGHT, 0U, 15000
        );
        if (r != CH89_OK) return r;
        r = ch89_add_shekt(
            g, 18U,
            28U, 56U, 5U, 10U,
            76U, 14U, 2U,
            ch89_scaled(18500, intensity_q15),
            ch89_scaled(26000, intensity_q15),
            CH89_NOISE_BRIGHT, CH89_NOISE_RAW,
            3U, 4U,
            14500, 20500
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 122U, 6U, 1U,
            ch89_scaled(8200, intensity_q15),
            CH89_NOISE_BRIGHT, 2U, 10500
        );
        if (r != CH89_OK) return r;
        r = ch89_add_shekt(
            g, 154U,
            18U, 30U, 2U, 7U,
            47U, 11U, 2U,
            ch89_scaled(16000, intensity_q15),
            ch89_scaled(27500, intensity_q15),
            CH89_NOISE_DARK, CH89_NOISE_RAW,
            2U, 4U,
            13500, 20500
        );
        if (r != CH89_OK) return r;
        r = ch89_add_shekt(
            g, 228U,
            24U, 48U, 4U, 9U,
            68U, 20U, 3U,
            ch89_scaled(19000, intensity_q15),
            ch89_scaled(32700, intensity_q15),
            CH89_NOISE_DARK, CH89_NOISE_DARK,
            1U, 1U,
            17000, 27000
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 317U, 8U, 1U,
            ch89_scaled(11800, intensity_q15),
            CH89_NOISE_DARK, 1U, 16500
        );
        g->duration_ms = (ch89_u16)(g->duration_ms + 55U);
    } else if (preset == CH89_PRESET_ROCKET_LAUNCHER) {
        /* AT4-style preparation: sight cover, sight snap, cocking lever motion. */
        g->chorus_depth_samples = 8U;
        g->chorus_wet_q15 = 440;
        g->reverb_wet_q15 = 60;
        r = ch89_add_shekt(
            g, 0U,
            20U, 38U, 3U, 8U,
            52U, 10U, 2U,
            ch89_scaled(17000, intensity_q15),
            ch89_scaled(24500, intensity_q15),
            CH89_NOISE_DARK, CH89_NOISE_RAW,
            2U, 4U,
            13800, 19000
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 82U, 7U, 1U,
            ch89_scaled(12200, intensity_q15),
            CH89_NOISE_BRIGHT, 0U, 13500
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 132U, 10U, 2U,
            ch89_scaled(16800, intensity_q15),
            CH89_NOISE_DARK, 1U, 16500
        );
        if (r != CH89_OK) return r;
        r = ch89_add_shekt(
            g, 168U,
            16U, 30U, 2U, 7U,
            44U, 12U, 2U,
            ch89_scaled(16000, intensity_q15),
            ch89_scaled(28500, intensity_q15),
            CH89_NOISE_DARK, CH89_NOISE_DARK,
            3U, 1U,
            15500, 22800
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 226U, 6U, 1U,
            ch89_scaled(9600, intensity_q15),
            CH89_NOISE_BRIGHT, 0U, 12000
        );
        if (r != CH89_OK) return r;
        r = ch89_add_contact(
            g, 266U, 15U, 2U,
            ch89_scaled(28200, intensity_q15),
            CH89_NOISE_DARK, 1U, 23000
        );
        g->duration_ms = (ch89_u16)(g->duration_ms + 62U);
    } else if (preset == CH89_PRESET_SMG_FEED_BURST) {
        count = (repetitions == 0U) ? 10U : repetitions;
        if (count > (CH89_MAX_STROKES / 2U)) {
            count = (CH89_MAX_STROKES / 2U);
        }
        interval = 76U;
        g->chorus_depth_samples = 5U;
        g->chorus_wet_q15 = 260;
        g->reverb_wet_q15 = 25;
        for (i = 0U; i < count; ++i) {
            jitter = ch89_timing_jitter(i);
            start_ms = (ch89_u16)((ch89_i32)(i * interval) + jitter);
            r = ch89_add_contact(
                g, (ch89_u16)(start_ms + 9U), 4U, 1U,
                ch89_scaled((ch89_i16)(6200 + ((i & 1U) ? 450 : 0)), intensity_q15),
                CH89_NOISE_BRIGHT, 0U, 9800
            );
            if (r != CH89_OK) return r;
            r = ch89_add_shekt(
                g, start_ms,
                5U, 13U, 1U, 3U,
                18U, 7U, 1U,
                ch89_scaled(9000, intensity_q15),
                ch89_scaled(22500, intensity_q15),
                CH89_NOISE_BRIGHT, CH89_NOISE_RAW,
                (i & 1U) ? 0U : 2U,
                4U,
                12600, 19000
            );
            if (r != CH89_OK) return r;
        }
        g->duration_ms = (ch89_u16)(g->duration_ms + 28U);
    } else if (preset == CH89_PRESET_MACHINE_GUN_FEED_BURST) {
        count = (repetitions == 0U) ? 9U : repetitions;
        if (count > (CH89_MAX_STROKES / 2U)) {
            count = (CH89_MAX_STROKES / 2U);
        }
        interval = 94U;
        g->chorus_depth_samples = 5U;
        g->chorus_wet_q15 = 220;
        g->reverb_wet_q15 = 35;
        for (i = 0U; i < count; ++i) {
            jitter = ch89_timing_jitter(i);
            start_ms = (ch89_u16)((ch89_i32)(i * interval) + jitter);
            r = ch89_add_contact(
                g, (ch89_u16)(start_ms + 8U), 5U, 1U,
                ch89_scaled((ch89_i16)(7600 + ((i & 1U) ? 550 : 0)), intensity_q15),
                CH89_NOISE_BRIGHT, 0U, 11200
            );
            if (r != CH89_OK) return r;
            r = ch89_add_shekt(
                g, start_ms,
                8U, 22U, 2U, 5U,
                31U, 11U, 2U,
                ch89_scaled(12000, intensity_q15),
                ch89_scaled(28500, intensity_q15),
                CH89_NOISE_DARK, CH89_NOISE_RAW,
                (i & 1U) ? 1U : 3U,
                4U,
                15500, 22800
            );
            if (r != CH89_OK) return r;
        }
        g->duration_ms = (ch89_u16)(g->duration_ms + 38U);
    } else if (preset == CH89_PRESET_ATOM_SH) {
        g->chorus_depth_samples = 8U;
        g->chorus_wet_q15 = 650;
        g->reverb_wet_q15 = 55;
        r = ch89_add_sh_only(g, 0U, ch89_scaled(22000, intensity_q15), 2U);
        g->duration_ms = (ch89_u16)(g->duration_ms + 60U);
    } else if (preset == CH89_PRESET_ATOM_ECKT) {
        g->chorus_depth_samples = 6U;
        g->chorus_wet_q15 = 380;
        g->reverb_wet_q15 = 35;
        r = ch89_add_eckt_only(g, 0U, ch89_scaled(32000, intensity_q15));
        g->duration_ms = (ch89_u16)(g->duration_ms + 60U);
    } else if (preset == CH89_PRESET_ATOM_SHEKT) {
        /* User-approved atom: SH attack 30 ms and release 12 ms. */
        g->chorus_depth_samples = 8U;
        g->chorus_wet_q15 = 700;
        g->reverb_wet_q15 = 60;
        r = ch89_add_shekt(
            g, 0U,
            30U, 62U, 7U, 12U,
            82U, 15U, 2U,
            ch89_scaled(22000, intensity_q15),
            ch89_scaled(32000, intensity_q15),
            CH89_NOISE_BRIGHT, CH89_NOISE_RAW,
            2U, 4U,
            14500, 20500
        );
        g->duration_ms = (ch89_u16)(g->duration_ms + 60U);
    }

    return r;
}

ch89_u32 ch89_required_frames(const ch89_gesture *gesture, ch89_u32 sample_rate)
{
    if (gesture == 0) return 0U;
    return ch89_ms_to_frames((ch89_u32)gesture->duration_ms, sample_rate);
}

ch89_result ch89_render(
    ch89_context *ctx,
    const ch89_gesture *gesture,
    ch89_i16 *out_samples,
    ch89_u32 frame_capacity,
    ch89_u32 *out_frames_written
)
{
    ch89_u32 required;
    ch89_u32 frame;
    ch89_u16 i;
    ch89_u32 stroke_start;
    ch89_u32 local;
    ch89_u32 sh_frames;
    ch89_u32 eckt_frames;
    ch89_i32 x;
    ch89_i32 stroke_x;

    if (ctx == 0 || gesture == 0 || out_samples == 0) return CH89_BAD_ARGUMENT;
    required = ch89_required_frames(gesture, ctx->sample_rate);
    if (required > frame_capacity) return CH89_BAD_ARGUMENT;
    ch89_reset(ctx, ctx->rng);

    for (frame = 0U; frame < required; ++frame) {
        x = 0;
        for (i = 0U; i < gesture->stroke_count; ++i) {
            stroke_start = ch89_ms_to_frames(
                gesture->strokes[i].start_ms,
                ctx->sample_rate
            );
            if (frame < stroke_start) continue;
            local = frame - stroke_start;
            sh_frames = ch89_layer_frames(
                &gesture->strokes[i].sh.envelope,
                ctx->sample_rate
            );
            eckt_frames = ch89_layer_frames(
                &gesture->strokes[i].eckt.envelope,
                ctx->sample_rate
            );
            if (local >= sh_frames && local >= eckt_frames) continue;

            stroke_x = ch89_voice_sample(
                ctx,
                &ctx->voice_state[i][CH89_VOICE_SH],
                &gesture->strokes[i].sh,
                local,
                gesture->effect_flags
            );
            stroke_x += ch89_voice_sample(
                ctx,
                &ctx->voice_state[i][CH89_VOICE_ECKT],
                &gesture->strokes[i].eckt,
                local,
                gesture->effect_flags
            );
            x += stroke_x;
        }

        if ((gesture->effect_flags & CH89_EFFECT_CHORUS) != 0U) {
            x = ch89_chorus_process(ctx, x, gesture);
        }
        if ((gesture->effect_flags & CH89_EFFECT_REVERB) != 0U) {
            x = ch89_reverb_process(ctx, x, gesture);
        }
        x = ch89_mul_q15(x, gesture->output_gain_q15);
        out_samples[frame] = ch89_clip16(x);
    }

    if (out_frames_written != 0) *out_frames_written = required;
    return CH89_OK;
}

void ch89_set_effect_flags(ch89_gesture *gesture, ch89_u16 effect_flags)
{
    if (gesture == 0) return;
    gesture->effect_flags = (ch89_u16)(effect_flags & CH89_EFFECT_ALL);
}

const char *ch89_preset_name(ch89_preset preset)
{
    static const char *names[CH89_PRESET_COUNT] = {
        "shotgun_pump_chik_chok",
        "shotgun_insert_shuk",
        "shotgun_multi_insert_sheke",
        "pistol_slide",
        "magnum_heavy_action",
        "revolver_cylinder",
        "grenade_launcher_breech",
        "rocket_launcher_deploy",
        "smg_feed_burst",
        "machine_gun_feed_burst",
        "atom_sh",
        "atom_eckt",
        "atom_shekt"
    };
    if (preset < 0 || preset >= CH89_PRESET_COUNT) return "unknown";
    return names[preset];
}
