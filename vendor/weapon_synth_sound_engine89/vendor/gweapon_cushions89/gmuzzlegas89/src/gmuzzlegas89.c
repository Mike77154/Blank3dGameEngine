#include "gmuzzlegas89.h"

static gmg89_s16 gmg89_sat16(gmg89_s32 x)
{
    if (x > 32767) return (gmg89_s16)32767;
    if (x < -32768) return (gmg89_s16)-32768;
    return (gmg89_s16)x;
}

static gmg89_s32 gmg89_clamp32(gmg89_s32 x, gmg89_s32 lo, gmg89_s32 hi)
{
    if (x < lo) return lo;
    if (x > hi) return hi;
    return x;
}

static gmg89_u32 gmg89_rng_next(gmg89_u32 *state)
{
    gmg89_u32 x;
    x = *state;
    if (x == 0U) x = 0x9e3779b9U;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static gmg89_s16 gmg89_noise(gmg89_u32 *state)
{
    gmg89_u32 x;
    x = gmg89_rng_next(state);
    return (gmg89_s16)((gmg89_s32)((x >> 16) & 65535U) - 32768);
}

static gmg89_s16 gmg89_alpha(gmg89_u32 rate, gmg89_u16 hz)
{
    gmg89_u32 den;
    gmg89_u32 a;
    if (hz == 0U) return 0;
    den = (gmg89_u32)hz + rate / 6U;
    if (den == 0U) return 0;
    a = ((gmg89_u32)hz * 32767U) / den;
    if (a > 32767U) a = 32767U;
    return (gmg89_s16)a;
}

static gmg89_s16 gmg89_env(gmg89_u32 pos, gmg89_u32 total)
{
    gmg89_u32 remain;
    gmg89_u32 q;
    if (total == 0U || pos >= total) return 0;
    remain = total - pos;
    q = (remain * 32767U) / total;
    return (gmg89_s16)q;
}

static gmg89_s32 gmg89_soft_clip(gmg89_s32 x, gmg89_s16 drive_q15)
{
    gmg89_s32 y;
    gmg89_s32 a;
    int neg;
    y = x + ((x * (gmg89_s32)drive_q15) >> 15);
    neg = y < 0;
    a = neg ? -y : y;
    if (a > 32767) a = 32767 + ((a - 32767) >> 3);
    if (a > 49151) a = 49151;
    return neg ? -a : a;
}

static const gmg89_preset gmg89_presets[GMG89_PRESET_COUNT] = {
    {{120,15000},{85,12500},{42,9000}, 260,700,3200,5200, 12000,3500,8000,25000},
    {{170,18500},{115,15000},{55,10500}, 220,620,3000,5600, 14500,4200,9500,25000},
    {{280,22000},{190,18000},{95,12500}, 170,500,2500,4800, 17500,5200,10500,25500},
    {{190,17000},{125,15500},{65,13500}, 210,650,3400,6500, 16000,4000,9000,25500},
    {{235,19000},{155,17500},{80,15000}, 190,580,3200,7200, 18000,4500,10500,25500},
    {{420,24500},{300,20500},{145,14000}, 120,380,2100,4200, 20500,6500,12000,26000}
};

int gmg89_platform_ok(void)
{
    return (sizeof(gmg89_s16) == 2U && sizeof(gmg89_s32) >= 4U &&
            sizeof(gmg89_u32) >= 4U) ? 1 : 0;
}

int gmg89_get_preset(gmg89_preset_id id, gmg89_preset *out_preset)
{
    if (out_preset == 0) return 0;
    if ((int)id < 0 || id >= GMG89_PRESET_COUNT) return 0;
    *out_preset = gmg89_presets[(int)id];
    return 1;
}

int gmg89_init(gmg89_context *ctx, gmg89_u32 sample_rate,
               const gmg89_preset *preset, gmg89_u32 seed)
{
    if (ctx == 0 || preset == 0) return 0;
    if (!gmg89_platform_ok()) return 0;
    if (sample_rate == 0U || sample_rate > GMG89_MAX_SAMPLE_RATE) return 0;
    ctx->sample_rate = sample_rate;
    ctx->preset = *preset;
    ctx->rng = seed ? seed : 1U;
    ctx->a_low = gmg89_alpha(sample_rate, preset->low_cut_hz);
    ctx->a_mid_low = gmg89_alpha(sample_rate, preset->mid_low_hz);
    ctx->a_mid_high = gmg89_alpha(sample_rate, preset->mid_high_hz);
    ctx->a_high = gmg89_alpha(sample_rate, preset->high_cut_hz);
    gmg89_reset(ctx, seed);
    return 1;
}

void gmg89_reset(gmg89_context *ctx, gmg89_u32 seed)
{
    if (ctx == 0) return;
    ctx->rng = seed ? seed : 1U;
    ctx->lp_low = 0;
    ctx->lp_mid_low = 0;
    ctx->lp_mid_high = 0;
    ctx->lp_high = 0;
    ctx->pos = 0U;
    ctx->low_samples = 0U;
    ctx->mid_samples = 0U;
    ctx->high_samples = 0U;
    ctx->intensity_q15 = 0;
}

void gmg89_trigger(gmg89_context *ctx, gmg89_s16 intensity_q15,
                   gmg89_u32 seed)
{
    if (ctx == 0) return;
    if (intensity_q15 < 0) intensity_q15 = 0;
    ctx->rng = seed ? seed : ctx->rng;
    ctx->pos = 0U;
    ctx->low_samples = (ctx->sample_rate * (gmg89_u32)ctx->preset.low.decay_ms) / 1000U;
    ctx->mid_samples = (ctx->sample_rate * (gmg89_u32)ctx->preset.mid.decay_ms) / 1000U;
    ctx->high_samples = (ctx->sample_rate * (gmg89_u32)ctx->preset.high.decay_ms) / 1000U;
    if (ctx->low_samples == 0U) ctx->low_samples = 1U;
    if (ctx->mid_samples == 0U) ctx->mid_samples = 1U;
    if (ctx->high_samples == 0U) ctx->high_samples = 1U;
    ctx->intensity_q15 = intensity_q15;
}

gmg89_s16 gmg89_process_sample(gmg89_context *ctx)
{
    gmg89_s16 raw;
    gmg89_s16 e_low;
    gmg89_s16 e_mid;
    gmg89_s16 e_high;
    gmg89_s32 delta;
    gmg89_s32 low;
    gmg89_s32 mid;
    gmg89_s32 high;
    gmg89_s32 out;
    gmg89_s32 asym;
    if (ctx == 0) return 0;
    if (!gmg89_is_active(ctx)) return 0;
    raw = gmg89_noise(&ctx->rng);
    delta = (gmg89_s32)raw - ctx->lp_low;
    ctx->lp_low += (delta * ctx->a_low) >> 15;
    delta = (gmg89_s32)raw - ctx->lp_mid_low;
    ctx->lp_mid_low += (delta * ctx->a_mid_low) >> 15;
    delta = (gmg89_s32)raw - ctx->lp_mid_high;
    ctx->lp_mid_high += (delta * ctx->a_mid_high) >> 15;
    delta = (gmg89_s32)raw - ctx->lp_high;
    ctx->lp_high += (delta * ctx->a_high) >> 15;

    e_low = gmg89_env(ctx->pos, ctx->low_samples);
    e_mid = gmg89_env(ctx->pos, ctx->mid_samples);
    e_high = gmg89_env(ctx->pos, ctx->high_samples);
    low = ((ctx->lp_low * ctx->preset.low.level_q15) >> 15) * e_low >> 15;
    mid = (((ctx->lp_mid_high - ctx->lp_mid_low) * ctx->preset.mid.level_q15) >> 15) * e_mid >> 15;
    high = (((gmg89_s32)raw - ctx->lp_high) * ctx->preset.high.level_q15 >> 15) * e_high >> 15;
    out = low + mid + high;
    if (ctx->pos < 12U) {
        out += ((gmg89_s32)ctx->preset.transient_q15 * (gmg89_s32)(12U - ctx->pos)) / 12;
    }
    asym = out > 0 ? out : -out;
    out += (asym * ctx->preset.asymmetry_q15) >> 16;
    out = gmg89_soft_clip(out, ctx->preset.drive_q15);
    out = (out * ctx->intensity_q15) >> 15;
    out = (out * ctx->preset.output_gain_q15) >> 15;
    ctx->pos++;
    return gmg89_sat16(gmg89_clamp32(out, -65535, 65535));
}

gmg89_u32 gmg89_render(gmg89_context *ctx, gmg89_s16 *output,
                       gmg89_u32 frames)
{
    gmg89_u32 i;
    if (ctx == 0 || output == 0) return 0U;
    for (i = 0U; i < frames; ++i) output[i] = gmg89_process_sample(ctx);
    return frames;
}

int gmg89_is_active(const gmg89_context *ctx)
{
    gmg89_u32 maxs;
    if (ctx == 0) return 0;
    maxs = ctx->low_samples;
    if (ctx->mid_samples > maxs) maxs = ctx->mid_samples;
    if (ctx->high_samples > maxs) maxs = ctx->high_samples;
    return ctx->pos < maxs ? 1 : 0;
}

const char *gmg89_preset_name(gmg89_preset_id id)
{
    static const char *names[GMG89_PRESET_COUNT] = {
        "pistol", "magnum", "shotgun", "rifle", "sniper", "launcher"
    };
    if ((int)id < 0 || id >= GMG89_PRESET_COUNT) return "invalid";
    return names[(int)id];
}

gmg89_u32 gmg89_context_bytes(void)
{
    return (gmg89_u32)sizeof(gmg89_context);
}
