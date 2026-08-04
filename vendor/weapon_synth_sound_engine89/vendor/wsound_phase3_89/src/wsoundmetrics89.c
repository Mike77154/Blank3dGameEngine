#include "wsoundmetrics89.h"

static wsound89_u32 wm89_sat_add_u32(wsound89_u32 a, wsound89_u32 b)
{
    if (0xFFFFFFFFU - a < b) return 0xFFFFFFFFU;
    return a + b;
}

static wsound89_u16 wm89_abs16(wsound89_i16 v)
{
    if (v == -32768) return 32768U;
    return (wsound89_u16)(v < 0 ? -v : v);
}

static wsound89_u16 wm89_clamp_u15(wsound89_i32 v)
{
    if (v < 0) return 0U;
    if (v > 32767) return 32767U;
    return (wsound89_u16)v;
}

static wsound89_u16 wm89_ratio_q15(wsound89_u32 num, wsound89_u32 den)
{
    if (den == 0U) return 0U;
    while (den > 131071U) {
        den >>= 1;
        num >>= 1;
    }
    if (num > den) num = den;
    return (wsound89_u16)((num * 32767U) / den);
}

static wsound89_u16 wm89_alpha(wsound89_u32 rate, wsound89_u32 hz)
{
    wsound89_u32 v;
    if (rate == 0U) return 0U;
    v = (hz * 205887U) / rate;
    if (v > 32767U) v = 32767U;
    if (v < 1U) v = 1U;
    return (wsound89_u16)v;
}

static wsound89_u32 wm89_isqrt(wsound89_u32 x)
{
    wsound89_u32 op;
    wsound89_u32 res;
    wsound89_u32 one;
    op = x;
    res = 0U;
    one = 1U << 30;
    while (one > op) one >>= 2;
    while (one != 0U) {
        if (op >= res + one) {
            op -= res + one;
            res = (res >> 1) + one;
        } else {
            res >>= 1;
        }
        one >>= 2;
    }
    return res;
}

wsound89_result wsoundmetrics89_init(wsoundmetrics89_context *ctx,
                                     wsound89_u32 sample_rate)
{
    if (ctx == 0 || sample_rate < 8000U || sample_rate > 96000U)
        return WSOUND89_EINVAL;
    ctx->sample_rate = sample_rate;
    ctx->low_alpha_q15 = wm89_alpha(sample_rate, 250U);
    ctx->high_alpha_q15 = wm89_alpha(sample_rate, 2400U);
    ctx->enabled = 1U;
    wsoundmetrics89_reset(ctx);
    ctx->enabled = 1U;
    return WSOUND89_OK;
}

void wsoundmetrics89_reset(wsoundmetrics89_context *ctx)
{
    wsound89_u16 low_alpha;
    wsound89_u16 high_alpha;
    wsound89_u32 rate;
    wsound89_u8 enabled;
    if (ctx == 0) return;
    rate = ctx->sample_rate;
    low_alpha = ctx->low_alpha_q15;
    high_alpha = ctx->high_alpha_q15;
    enabled = ctx->enabled;
    ctx->sample_rate = rate;
    ctx->frames = 0U;
    ctx->analysis_samples = 0U;
    ctx->square_sum = 0U;
    ctx->band_energy[0] = 0U;
    ctx->band_energy[1] = 0U;
    ctx->band_energy[2] = 0U;
    ctx->peak_frame = 0U;
    ctx->onset_frame = WSOUNDM89_INVALID_FRAME;
    ctx->decay_frame = WSOUNDM89_INVALID_FRAME;
    ctx->clipped_samples = 0U;
    ctx->zero_crossings = 0U;
    ctx->low_state = 0;
    ctx->high_state = 0;
    ctx->envelope = 0;
    ctx->previous = 0;
    ctx->peak_abs = 0U;
    ctx->low_alpha_q15 = low_alpha;
    ctx->high_alpha_q15 = high_alpha;
    ctx->square_phase = 0U;
    ctx->band_phase = 0U;
    ctx->enabled = enabled;
}

void wsoundmetrics89_set_enabled(wsoundmetrics89_context *ctx, int enabled)
{
    if (ctx != 0) ctx->enabled = enabled ? 1U : 0U;
}

void wsoundmetrics89_push_mono(wsoundmetrics89_context *ctx,
                               wsound89_i16 sample)
{
    wsound89_u16 a;
    wsound89_i32 low;
    wsound89_i32 mid;
    wsound89_i32 high;
    wsound89_u32 scaled;
    wsound89_u32 threshold;
    if (ctx == 0 || !ctx->enabled) return;
    a = wm89_abs16(sample);
    if (a >= 32767U) ctx->clipped_samples++;
    if (a > ctx->peak_abs) {
        ctx->peak_abs = a;
        ctx->peak_frame = ctx->frames;
        ctx->decay_frame = WSOUNDM89_INVALID_FRAME;
    }
    if (ctx->onset_frame == WSOUNDM89_INVALID_FRAME && a >= 384U)
        ctx->onset_frame = ctx->frames;
    if ((sample < 0 && ctx->previous >= 0) || (sample >= 0 && ctx->previous < 0))
        ctx->zero_crossings++;
    ctx->previous = sample;
    ctx->low_state += ((wsound89_i32)ctx->low_alpha_q15 *
                      ((wsound89_i32)sample - ctx->low_state)) >> 15;
    ctx->high_state += ((wsound89_i32)ctx->high_alpha_q15 *
                       ((wsound89_i32)sample - ctx->high_state)) >> 15;
    low = ctx->low_state;
    mid = ctx->high_state - ctx->low_state;
    high = (wsound89_i32)sample - ctx->high_state;
    if (ctx->band_phase == 0U) {
        ctx->band_energy[0] = wm89_sat_add_u32(ctx->band_energy[0],
            (wsound89_u32)(low < 0 ? -low : low));
        ctx->band_energy[1] = wm89_sat_add_u32(ctx->band_energy[1],
            (wsound89_u32)(mid < 0 ? -mid : mid));
        ctx->band_energy[2] = wm89_sat_add_u32(ctx->band_energy[2],
            (wsound89_u32)(high < 0 ? -high : high));
    }
    ctx->band_phase = (wsound89_u8)((ctx->band_phase + 1U) & 7U);
    if (ctx->square_phase == 0U) {
        scaled = (wsound89_u32)(a >> 6);
        ctx->square_sum = wm89_sat_add_u32(ctx->square_sum, scaled * scaled);
        ctx->analysis_samples++;
    }
    ctx->square_phase = (wsound89_u8)((ctx->square_phase + 1U) & 63U);
    if ((wsound89_i32)a > ctx->envelope)
        ctx->envelope = a;
    else
        ctx->envelope -= ctx->envelope >> 8;
    threshold = (wsound89_u32)ctx->peak_abs / 10U;
    if (threshold < 256U) threshold = 256U;
    if (ctx->frames >= ctx->peak_frame &&
        (wsound89_u32)ctx->envelope >= threshold)
        ctx->decay_frame = ctx->frames;
    ctx->frames++;
}

void wsoundmetrics89_push_stereo(wsoundmetrics89_context *ctx,
                                 wsound89_i16 left,
                                 wsound89_i16 right)
{
    wsound89_i32 mono;
    mono = ((wsound89_i32)left + (wsound89_i32)right) / 2;
    wsoundmetrics89_push_mono(ctx, (wsound89_i16)mono);
}

wsound89_result wsoundmetrics89_finish(const wsoundmetrics89_context *ctx,
                                       wsoundmetrics89_result *r)
{
    wsound89_u32 mean_sq;
    wsound89_u32 rms;
    wsound89_u32 total_band;
    wsound89_u32 onset;
    wsound89_u32 decay;
    wsound89_u32 attack_frames;
    if (ctx == 0 || r == 0 || ctx->sample_rate == 0U) return WSOUND89_EINVAL;
    r->frames = ctx->frames;
    r->peak_abs = ctx->peak_abs;
    mean_sq = ctx->analysis_samples == 0U ? 0U :
              ctx->square_sum / ctx->analysis_samples;
    rms = wm89_isqrt(mean_sq) << 6;
    if (rms > 32767U) rms = 32767U;
    r->rms_q15 = (wsound89_u16)rms;
    r->crest_q8 = rms == 0U ? 0U :
        (wsound89_u16)(((wsound89_u32)ctx->peak_abs * 256U) / rms);
    total_band = ctx->band_energy[0] + ctx->band_energy[1];
    if (0xFFFFFFFFU - total_band < ctx->band_energy[2])
        total_band = 0xFFFFFFFFU;
    else
        total_band += ctx->band_energy[2];
    if (total_band == 0U) {
        r->band_ratio_q15[0] = 0U;
        r->band_ratio_q15[1] = 0U;
        r->band_ratio_q15[2] = 0U;
    } else {
        r->band_ratio_q15[0] = wm89_ratio_q15(ctx->band_energy[0], total_band);
        r->band_ratio_q15[1] = wm89_ratio_q15(ctx->band_energy[1], total_band);
        r->band_ratio_q15[2] = wm89_ratio_q15(ctx->band_energy[2], total_band);
    }
    r->brightness_q15 = wm89_clamp_u15(
        (wsound89_i32)r->band_ratio_q15[2] +
        (wsound89_i32)r->band_ratio_q15[1] / 3);
    onset = ctx->onset_frame == WSOUNDM89_INVALID_FRAME ? 0U : ctx->onset_frame;
    attack_frames = ctx->peak_frame >= onset ? ctx->peak_frame - onset : 0U;
    r->attack_ms_x10 = (wsound89_u16)((attack_frames * 10000U) /
                                      ctx->sample_rate);
    decay = ctx->decay_frame == WSOUNDM89_INVALID_FRAME ? ctx->frames :
            ctx->decay_frame;
    r->decay_ms = decay <= ctx->peak_frame ? 0U :
        (wsound89_u16)(((decay - ctx->peak_frame) * 1000U) / ctx->sample_rate);
    r->zero_cross_q15 = wm89_ratio_q15(ctx->zero_crossings, ctx->frames);
    r->peak_frame = ctx->peak_frame;
    r->clipped_samples = ctx->clipped_samples;
    return WSOUND89_OK;
}

void wsoundmetrics89_target_defaults(const wsounddna89_profile *p,
                                     wsounddna89_mode mode,
                                     wsoundmetrics89_target *t)
{
    wsound89_i32 low;
    wsound89_i32 mid;
    wsound89_i32 high;
    wsound89_i32 tolerance;
    wsound89_u16 decay;
    if (t == 0) return;
    low = 10500;
    mid = 13500;
    high = 8750;
    decay = 500U;
    if (p != 0) {
        if (p->weapon_class == WSOUNDDNA89_SHOTGUN) {
            low = 14500; mid = 12500; high = 5750; decay = 700U;
        } else if (p->weapon_class == WSOUNDDNA89_RIFLE) {
            low = 9000; mid = 13500; high = 10250; decay = 650U;
        } else if (p->weapon_class == WSOUNDDNA89_MACHINE) {
            low = 8500; mid = 14500; high = 9750; decay = 420U;
        } else if (p->weapon_class == WSOUNDDNA89_HEAVY) {
            low = 16000; mid = 11000; high = 5750; decay = 950U;
        }
        if (p->suppressor_q15 > 8000U) {
            high -= 2800; low += 1800; mid += 1000;
        }
        if (p->barrel_length_mm < 220U) {
            high += 1600; low -= 900; mid -= 700;
        }
    }
    if (mode == WSOUNDDNA89_MODE_CINEMATIC) {
        low += 2200; high -= 1000; decay = (wsound89_u16)(decay + 250U);
    } else if (mode == WSOUNDDNA89_MODE_REALISTIC) {
        decay = (wsound89_u16)((decay * 4U) / 5U);
    }
    tolerance = mode == WSOUNDDNA89_MODE_REALISTIC ? 7000 :
                (mode == WSOUNDDNA89_MODE_HYBRID ? 8500 : 9000);
    t->peak_min = mode == WSOUNDDNA89_MODE_REALISTIC ? 1800U : 2200U;
    t->peak_max = 32767U;
    t->crest_min_q8 = 280U;
    t->crest_max_q8 = 32767U;
    t->band_min_q15[0] = wm89_clamp_u15(low - tolerance);
    t->band_max_q15[0] = wm89_clamp_u15(low + tolerance);
    t->band_min_q15[1] = wm89_clamp_u15(mid - tolerance);
    t->band_max_q15[1] = wm89_clamp_u15(mid + tolerance);
    t->band_min_q15[2] = wm89_clamp_u15(high - tolerance);
    t->band_max_q15[2] = wm89_clamp_u15(high + tolerance);
    t->attack_max_ms_x10 = 80U;
    t->decay_min_ms = (wsound89_u16)(decay / 5U);
    t->decay_max_ms = (wsound89_u16)(decay * 4U);
    t->clipping_allowed = 0U;
}

wsound89_u16 wsoundmetrics89_validate(const wsoundmetrics89_result *r,
                                      const wsoundmetrics89_target *t)
{
    wsound89_u16 f;
    if (r == 0 || t == 0) return 0xFFFFU;
    f = 0U;
    if (r->peak_abs < t->peak_min || r->peak_abs > t->peak_max) f |= WSOUNDM89_FAIL_PEAK;
    if (r->crest_q8 < t->crest_min_q8 || r->crest_q8 > t->crest_max_q8) f |= WSOUNDM89_FAIL_CREST;
    if (r->band_ratio_q15[0] < t->band_min_q15[0] || r->band_ratio_q15[0] > t->band_max_q15[0]) f |= WSOUNDM89_FAIL_LOW_BAND;
    if (r->band_ratio_q15[1] < t->band_min_q15[1] || r->band_ratio_q15[1] > t->band_max_q15[1]) f |= WSOUNDM89_FAIL_MID_BAND;
    if (r->band_ratio_q15[2] < t->band_min_q15[2] || r->band_ratio_q15[2] > t->band_max_q15[2]) f |= WSOUNDM89_FAIL_HIGH_BAND;
    if (r->attack_ms_x10 > t->attack_max_ms_x10) f |= WSOUNDM89_FAIL_ATTACK;
    if (r->decay_ms < t->decay_min_ms || r->decay_ms > t->decay_max_ms) f |= WSOUNDM89_FAIL_DECAY;
    if (r->clipped_samples > (wsound89_u32)t->clipping_allowed) f |= WSOUNDM89_FAIL_CLIPPING;
    return f;
}

static wsound89_u16 wm89_range_score(wsound89_u16 v, wsound89_u16 lo,
                                     wsound89_u16 hi)
{
    wsound89_u32 d;
    wsound89_u32 span;
    if (v >= lo && v <= hi) return 32767U;
    span = hi > lo ? (wsound89_u32)(hi - lo) : 1U;
    d = v < lo ? (wsound89_u32)(lo - v) : (wsound89_u32)(v - hi);
    if (d >= span * 2U) return 0U;
    return (wsound89_u16)(32767U - (d * 32767U) / (span * 2U));
}

wsound89_u16 wsoundmetrics89_score_q15(const wsoundmetrics89_result *r,
                                       const wsoundmetrics89_target *t)
{
    wsound89_u32 sum;
    if (r == 0 || t == 0) return 0U;
    sum = wm89_range_score(r->peak_abs, t->peak_min, t->peak_max);
    sum += wm89_range_score(r->crest_q8, t->crest_min_q8, t->crest_max_q8);
    sum += wm89_range_score(r->band_ratio_q15[0], t->band_min_q15[0], t->band_max_q15[0]);
    sum += wm89_range_score(r->band_ratio_q15[1], t->band_min_q15[1], t->band_max_q15[1]);
    sum += wm89_range_score(r->band_ratio_q15[2], t->band_min_q15[2], t->band_max_q15[2]);
    sum += wm89_range_score(r->decay_ms, t->decay_min_ms, t->decay_max_ms);
    if (r->clipped_samples != 0U && t->clipping_allowed == 0U) sum /= 2U;
    return (wsound89_u16)(sum / 6U);
}

wsound89_u32 wsoundmetrics89_context_bytes(void)
{
    return (wsound89_u32)sizeof(wsoundmetrics89_context);
}
