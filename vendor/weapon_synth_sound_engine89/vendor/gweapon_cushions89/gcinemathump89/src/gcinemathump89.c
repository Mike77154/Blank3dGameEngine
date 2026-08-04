#include "gcinemathump89.h"

static const gct89_s16 gct89_sine[256] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602, 6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530, 18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790, 27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971, 32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
    32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285, 32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
    30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683, 27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
    23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868, 18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
    12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179, 6393, 5602, 4808, 4011, 3212, 2410, 1608, 804,
    0, -804, -1608, -2410, -3212, -4011, -4808, -5602, -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
    -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530, -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
    -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790, -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
    -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971, -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
    -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285, -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
    -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683, -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
    -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868, -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    -12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179, -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804
};

static gct89_s16 gct89_sat16(gct89_s32 x)
{
    if (x > 32767) return (gct89_s16)32767;
    if (x < -32768) return (gct89_s16)-32768;
    return (gct89_s16)x;
}

static gct89_u32 gct89_rng_next(gct89_u32 *state)
{
    gct89_u32 x;
    x = *state;
    if (x == 0U) x = 0xc8013ea4U;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static gct89_s16 gct89_noise(gct89_u32 *state)
{
    gct89_u32 x;
    x = gct89_rng_next(state);
    return (gct89_s16)((gct89_s32)((x >> 16) & 65535U) - 32768);
}

static gct89_s32 gct89_soft_clip(gct89_s32 x, gct89_s16 drive_q15)
{
    gct89_s32 y;
    gct89_s32 a;
    int neg;
    y = x + ((x * drive_q15) >> 15);
    neg = y < 0;
    a = neg ? -y : y;
    if (a > 24576) a = 24576 + ((a - 24576) >> 3);
    if (a > 32767) a = 32767;
    return neg ? -a : a;
}

static const gct89_preset gct89_presets[GCT89_PRESET_COUNT] = {
    {72,42,120,21000,2600,2500,3500,24500},
    {92,38,170,25000,4200,4500,6500,25500},
    {88,34,210,27000,5200,6000,7600,26000},
    {105,31,260,28500,4800,7000,8200,26500},
    {74,27,340,30000,6500,8000,9800,27000}
};

int gct89_platform_ok(void)
{
    return (sizeof(gct89_s16) == 2U && sizeof(gct89_s32) >= 4U &&
            sizeof(gct89_u32) >= 4U) ? 1 : 0;
}

int gct89_get_preset(gct89_preset_id id, gct89_preset *out_preset)
{
    if (out_preset == 0) return 0;
    if ((int)id < 0 || id >= GCT89_PRESET_COUNT) return 0;
    *out_preset = gct89_presets[(int)id];
    return 1;
}

int gct89_init(gct89_context *ctx, gct89_u32 sample_rate,
               const gct89_preset *preset, gct89_u32 seed)
{
    if (ctx == 0 || preset == 0) return 0;
    if (!gct89_platform_ok()) return 0;
    if (sample_rate == 0U || sample_rate > GCT89_MAX_SAMPLE_RATE) return 0;
    ctx->sample_rate = sample_rate;
    ctx->preset = *preset;
    gct89_reset(ctx, seed);
    return 1;
}

void gct89_reset(gct89_context *ctx, gct89_u32 seed)
{
    if (ctx == 0) return;
    ctx->rng = seed ? seed : 1U;
    ctx->pos = 0U;
    ctx->total_samples = 0U;
    ctx->phase = 0U;
    ctx->brown = 0;
    ctx->intensity_q15 = 0;
    ctx->active = 0U;
}

void gct89_trigger(gct89_context *ctx, gct89_s16 intensity_q15,
                   gct89_u32 seed)
{
    if (ctx == 0) return;
    if (intensity_q15 < 0) intensity_q15 = 0;
    ctx->rng = seed ? seed : ctx->rng;
    ctx->pos = 0U;
    ctx->total_samples = (ctx->sample_rate * (gct89_u32)ctx->preset.duration_ms) / 1000U;
    if (ctx->total_samples < 1U) ctx->total_samples = 1U;
    ctx->phase = 0U;
    ctx->brown = 0;
    ctx->intensity_q15 = intensity_q15;
    ctx->active = 1U;
}

gct89_s16 gct89_process_sample(gct89_context *ctx)
{
    gct89_u32 remain;
    gct89_u32 env;
    gct89_u32 env2;
    gct89_u32 hz;
    gct89_u32 step;
    gct89_s32 tone;
    gct89_s32 noise;
    gct89_s32 out;
    gct89_u16 index;
    if (ctx == 0 || ctx->active == 0U) return 0;
    if (ctx->pos >= ctx->total_samples) {
        ctx->active = 0U;
        return 0;
    }
    remain = ctx->total_samples - ctx->pos;
    env = (remain * 32767U) / ctx->total_samples;
    env2 = (env * env) >> 15;
    hz = (gct89_u32)ctx->preset.end_hz +
         (((gct89_u32)(ctx->preset.start_hz - ctx->preset.end_hz) * remain) /
          ctx->total_samples);
    step = (hz * 65536U) / ctx->sample_rate;
    ctx->phase = (gct89_u16)(ctx->phase + (gct89_u16)step);
    index = (gct89_u16)(ctx->phase >> 8);
    tone = ((gct89_s32)gct89_sine[index] * ctx->preset.tone_q15) >> 15;
    tone = (tone * (gct89_s32)env2) >> 15;
    noise = gct89_noise(&ctx->rng);
    ctx->brown += noise >> 6;
    ctx->brown -= ctx->brown >> 7;
    if (ctx->brown > 32767) ctx->brown = 32767;
    if (ctx->brown < -32768) ctx->brown = -32768;
    noise = (ctx->brown * ctx->preset.brown_noise_q15) >> 15;
    noise = (noise * (gct89_s32)env) >> 15;
    out = tone + noise;
    if (ctx->pos < 5U) {
        out += ((gct89_s32)ctx->preset.click_q15 * (gct89_s32)(5U - ctx->pos)) / 5;
    }
    out = gct89_soft_clip(out, ctx->preset.drive_q15);
    out = (out * ctx->intensity_q15) >> 15;
    out = (out * ctx->preset.output_gain_q15) >> 15;
    ctx->pos++;
    return gct89_sat16(out);
}

gct89_u32 gct89_render(gct89_context *ctx, gct89_s16 *output,
                       gct89_u32 frames)
{
    gct89_u32 i;
    if (ctx == 0 || output == 0) return 0U;
    for (i = 0U; i < frames; ++i) output[i] = gct89_process_sample(ctx);
    return frames;
}

int gct89_is_active(const gct89_context *ctx)
{
    if (ctx == 0) return 0;
    return ctx->active ? 1 : 0;
}

const char *gct89_preset_name(gct89_preset_id id)
{
    static const char *names[GCT89_PRESET_COUNT] = {
        "subtle", "action", "shotgun", "sniper", "launcher"
    };
    if ((int)id < 0 || id >= GCT89_PRESET_COUNT) return "invalid";
    return names[(int)id];
}

gct89_u32 gct89_context_bytes(void)
{
    return (gct89_u32)sizeof(gct89_context);
}
