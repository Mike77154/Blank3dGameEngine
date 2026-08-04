#include "gballisticcrack89.h"

static gbc89_s16 gbc89_sat16(gbc89_s32 x)
{
    if (x > 32767) return (gbc89_s16)32767;
    if (x < -32768) return (gbc89_s16)-32768;
    return (gbc89_s16)x;
}

static gbc89_s16 gbc89_mul_q15(gbc89_s16 a, gbc89_s16 b)
{
    return gbc89_sat16(((gbc89_s32)a * (gbc89_s32)b) >> 15);
}

static gbc89_u32 gbc89_rng_next(gbc89_u32 *state)
{
    gbc89_u32 x;
    x = *state;
    if (x == 0U) x = 0xa341316cU;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static gbc89_s16 gbc89_noise(gbc89_u32 *state)
{
    gbc89_u32 x;
    x = gbc89_rng_next(state);
    return (gbc89_s16)((gbc89_s32)((x >> 16) & 65535U) - 32768);
}

static gbc89_s16 gbc89_alpha(gbc89_u32 rate, gbc89_u16 hz)
{
    gbc89_u32 den;
    gbc89_u32 a;
    if (hz == 0U) return 0;
    den = (gbc89_u32)hz + rate / 6U;
    a = ((gbc89_u32)hz * 32767U) / den;
    if (a > 32767U) a = 32767U;
    return (gbc89_s16)a;
}

static const gbc89_preset gbc89_presets[GBC89_PRESET_COUNT] = {
    {620, 16, 30000, 10500, 7000, 7200, 27000},
    {950, 25, 25500, 9000, 7500, 5600, 27000},
    {1500, 40, 20500, 7200, 7800, 3800, 27000},
    {780, 22, 28000, 10000, 7600, 6500, 27500},
    {1050, 32, 31000, 11500, 8500, 7000, 28000}
};

int gbc89_platform_ok(void)
{
    return (sizeof(gbc89_s16) == 2U && sizeof(gbc89_s32) >= 4U &&
            sizeof(gbc89_u32) >= 4U) ? 1 : 0;
}

int gbc89_get_preset(gbc89_preset_id id, gbc89_preset *out_preset)
{
    if (out_preset == 0) return 0;
    if ((int)id < 0 || id >= GBC89_PRESET_COUNT) return 0;
    *out_preset = gbc89_presets[(int)id];
    return 1;
}

int gbc89_init(gbc89_context *ctx, gbc89_u32 sample_rate,
               const gbc89_preset *preset, gbc89_u32 seed)
{
    if (ctx == 0 || preset == 0) return 0;
    if (!gbc89_platform_ok()) return 0;
    if (sample_rate == 0U || sample_rate > GBC89_MAX_SAMPLE_RATE) return 0;
    ctx->sample_rate = sample_rate;
    ctx->preset = *preset;
    ctx->air_alpha_q15 = gbc89_alpha(sample_rate, preset->air_cut_hz);
    gbc89_reset(ctx, seed);
    return 1;
}

void gbc89_reset(gbc89_context *ctx, gbc89_u32 seed)
{
    if (ctx == 0) return;
    ctx->rng = seed ? seed : 1U;
    ctx->delay_remaining = 0U;
    ctx->wave_pos = 0U;
    ctx->wave_samples = 0U;
    ctx->air_pos = 0U;
    ctx->air_samples = 0U;
    ctx->air_lp = 0;
    ctx->intensity_q15 = 0;
    ctx->active = 0U;
}

void gbc89_trigger(gbc89_context *ctx, gbc89_u16 delay_ms,
                   gbc89_s16 intensity_q15, gbc89_u32 seed)
{
    if (ctx == 0) return;
    if (intensity_q15 < 0) intensity_q15 = 0;
    ctx->rng = seed ? seed : ctx->rng;
    ctx->delay_remaining = (ctx->sample_rate * (gbc89_u32)delay_ms) / 1000U;
    ctx->wave_samples = (ctx->sample_rate * (gbc89_u32)ctx->preset.nwave_us) / 1000000U;
    if (ctx->wave_samples < 4U) ctx->wave_samples = 4U;
    ctx->air_samples = (ctx->sample_rate * (gbc89_u32)ctx->preset.air_ms) / 1000U;
    if (ctx->air_samples < 1U) ctx->air_samples = 1U;
    ctx->wave_pos = 0U;
    ctx->air_pos = 0U;
    ctx->air_lp = 0;
    ctx->intensity_q15 = intensity_q15;
    ctx->active = 1U;
}

gbc89_s16 gbc89_process_sample(gbc89_context *ctx)
{
    gbc89_s32 wave;
    gbc89_s32 out;
    gbc89_s32 noise;
    gbc89_s32 hp;
    gbc89_s32 env;
    gbc89_s32 den;
    if (ctx == 0 || ctx->active == 0U) return 0;
    if (ctx->delay_remaining > 0U) {
        ctx->delay_remaining--;
        return 0;
    }
    out = 0;
    if (ctx->wave_pos < ctx->wave_samples) {
        den = (gbc89_s32)(ctx->wave_samples - 1U);
        wave = (gbc89_s32)ctx->preset.peak_q15 -
               ((2 * (gbc89_s32)ctx->preset.peak_q15 *
                 (gbc89_s32)ctx->wave_pos) / den);
        noise = gbc89_noise(&ctx->rng);
        if (ctx->wave_pos < 3U || ctx->wave_pos + 3U >= ctx->wave_samples) {
            wave += (noise * ctx->preset.edge_noise_q15) >> 15;
        }
        out += wave;
        ctx->wave_pos++;
    } else if (ctx->air_pos < ctx->air_samples) {
        noise = gbc89_noise(&ctx->rng);
        ctx->air_lp += ((noise - ctx->air_lp) * ctx->air_alpha_q15) >> 15;
        hp = noise - ctx->air_lp;
        env = ((gbc89_s32)(ctx->air_samples - ctx->air_pos) * 32767) /
              (gbc89_s32)ctx->air_samples;
        out += (((hp * ctx->preset.air_noise_q15) >> 15) * env) >> 15;
        ctx->air_pos++;
    } else {
        ctx->active = 0U;
    }
    out = (out * ctx->intensity_q15) >> 15;
    out = (out * ctx->preset.output_gain_q15) >> 15;
    return gbc89_mul_q15(gbc89_sat16(out), 30000);
}

gbc89_u32 gbc89_render(gbc89_context *ctx, gbc89_s16 *output,
                       gbc89_u32 frames)
{
    gbc89_u32 i;
    if (ctx == 0 || output == 0) return 0U;
    for (i = 0U; i < frames; ++i) output[i] = gbc89_process_sample(ctx);
    return frames;
}

int gbc89_is_active(const gbc89_context *ctx)
{
    if (ctx == 0) return 0;
    return ctx->active ? 1 : 0;
}

const char *gbc89_preset_name(gbc89_preset_id id)
{
    static const char *names[GBC89_PRESET_COUNT] = {
        "near", "medium", "far", "rifle_pass", "sniper_pass"
    };
    if ((int)id < 0 || id >= GBC89_PRESET_COUNT) return "invalid";
    return names[(int)id];
}

gbc89_u32 gbc89_context_bytes(void)
{
    return (gbc89_u32)sizeof(gbc89_context);
}
