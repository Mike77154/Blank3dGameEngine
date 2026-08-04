#include "gweaponbody89.h"

#define GWB89_U32_MASK 0xFFFFFFFFU

static gwb89_s16 gwb89_sat16(gwb89_s32 x)
{
    if (x > 32767) return (gwb89_s16)32767;
    if (x < -32768) return (gwb89_s16)-32768;
    return (gwb89_s16)x;
}

static gwb89_s16 gwb89_mul_q15(gwb89_s16 a, gwb89_s16 b)
{
    return gwb89_sat16(((gwb89_s32)a * (gwb89_s32)b) >> 15);
}

static gwb89_u32 gwb89_rng_next(gwb89_u32 *state)
{
    gwb89_u32 x;
    x = *state;
    if (x == 0U) x = 0x6d2b79f5U;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x & GWB89_U32_MASK;
    return *state;
}

static gwb89_s16 gwb89_noise(gwb89_u32 *state)
{
    gwb89_u32 x;
    x = gwb89_rng_next(state);
    return (gwb89_s16)((gwb89_s32)((x >> 16) & 65535U) - 32768);
}

static void gwb89_zero16(gwb89_s16 *p, gwb89_u32 n)
{
    gwb89_u32 i;
    for (i = 0U; i < n; ++i) p[i] = 0;
}

static const gwb89_preset gwb89_presets[GWB89_PRESET_COUNT] = {
    {6, 4, 620, 12000, 25000, 27000, 5000, 26000, {
        {43, 28600, 7600, 7200}, {61, 28200, 8200, 6500},
        {79, 27600, 9000, 5900}, {103, 27000, 9800, 5200},
        {137, 26400, 10800, 4500}, {181, 25200, 12000, 3600},
        {0,0,0,0}, {0,0,0,0}}},
    {7, 5, 760, 14000, 26000, 28500, 4500, 26500, {
        {47, 29200, 7000, 7100}, {67, 28800, 7600, 6800},
        {89, 28300, 8300, 6200}, {113, 27900, 9000, 5600},
        {149, 27200, 10000, 5000}, {197, 26500, 11200, 4200},
        {251, 25200, 12600, 3200}, {0,0,0,0}}},
    {7, 8, 980, 16500, 27500, 30000, 3500, 27000, {
        {73, 29600, 7200, 7600}, {91, 29200, 7800, 7000},
        {119, 28700, 8500, 6500}, {157, 28100, 9300, 5900},
        {199, 27500, 10200, 5000}, {263, 26700, 11600, 4100},
        {337, 25500, 13200, 3100}, {0,0,0,0}}},
    {7, 5, 820, 13500, 27000, 29000, 4000, 27000, {
        {37, 29300, 6800, 7000}, {53, 29000, 7400, 6800},
        {71, 28600, 8000, 6300}, {97, 28100, 8800, 5700},
        {131, 27500, 9600, 5000}, {179, 26700, 11000, 4200},
        {239, 25400, 12600, 3200}, {0,0,0,0}}},
    {8, 6, 1250, 15000, 27500, 30500, 3000, 27200, {
        {31, 29800, 6500, 6600}, {41, 29600, 7000, 6500},
        {57, 29300, 7600, 6200}, {79, 28900, 8200, 5800},
        {109, 28400, 9000, 5200}, {151, 27800, 9900, 4600},
        {211, 26900, 11200, 3900}, {293, 25800, 13000, 3000}}},
    {7, 10, 1450, 18500, 28500, 31000, 2500, 27500, {
        {83, 29900, 7000, 7600}, {107, 29600, 7600, 7100},
        {139, 29200, 8300, 6500}, {181, 28700, 9200, 5800},
        {239, 28000, 10200, 5100}, {317, 27000, 11700, 4200},
        {401, 25800, 13600, 3200}, {0,0,0,0}}}
};

int gwb89_platform_ok(void)
{
    return (sizeof(gwb89_s16) == 2U && sizeof(gwb89_s32) >= 4U &&
            sizeof(gwb89_u32) >= 4U) ? 1 : 0;
}

int gwb89_get_preset(gwb89_preset_id id, gwb89_preset *out_preset)
{
    if (out_preset == 0) return 0;
    if ((int)id < 0 || id >= GWB89_PRESET_COUNT) return 0;
    *out_preset = gwb89_presets[(int)id];
    return 1;
}

int gwb89_init(gwb89_context *ctx, gwb89_u32 sample_rate,
               const gwb89_preset *preset, gwb89_u32 seed)
{
    gwb89_u32 i;
    gwb89_u32 d;
    if (ctx == 0 || preset == 0) return 0;
    if (!gwb89_platform_ok()) return 0;
    if (sample_rate == 0U || sample_rate > GWB89_MAX_SAMPLE_RATE) return 0;
    if (preset->mode_count == 0U || preset->mode_count > GWB89_MAX_MODES) return 0;
    ctx->sample_rate = sample_rate;
    ctx->preset = *preset;
    ctx->rng = seed ? seed : 1U;
    gwb89_zero16(&ctx->delay[0][0], GWB89_MAX_MODES * GWB89_MAX_DELAY);
    gwb89_zero16(ctx->damp_state, GWB89_MAX_MODES);
    for (i = 0U; i < GWB89_MAX_MODES; ++i) {
        ctx->write_pos[i] = 0U;
        d = ((gwb89_u32)ctx->preset.mode[i].delay_at_44100 * sample_rate + 22050U) / 44100U;
        if (d < 2U) d = 2U;
        if (d > GWB89_MAX_DELAY) d = GWB89_MAX_DELAY;
        ctx->runtime_delay[i] = (gwb89_u16)d;
    }
    ctx->exciter_pos = 0U;
    ctx->exciter_samples = 0U;
    ctx->tail_remaining = 0U;
    ctx->trigger_level_q15 = 0;
    return 1;
}

void gwb89_reset(gwb89_context *ctx, gwb89_u32 seed)
{
    gwb89_u32 i;
    if (ctx == 0) return;
    ctx->rng = seed ? seed : 1U;
    gwb89_zero16(&ctx->delay[0][0], GWB89_MAX_MODES * GWB89_MAX_DELAY);
    gwb89_zero16(ctx->damp_state, GWB89_MAX_MODES);
    for (i = 0U; i < GWB89_MAX_MODES; ++i) ctx->write_pos[i] = 0U;
    ctx->exciter_pos = 0U;
    ctx->exciter_samples = 0U;
    ctx->tail_remaining = 0U;
    ctx->trigger_level_q15 = 0;
}

void gwb89_trigger(gwb89_context *ctx, gwb89_s16 intensity_q15,
                   gwb89_u32 seed)
{
    if (ctx == 0) return;
    if (intensity_q15 < 0) intensity_q15 = 0;
    ctx->rng = seed ? seed : ctx->rng;
    ctx->trigger_level_q15 = intensity_q15;
    ctx->exciter_pos = 0U;
    ctx->exciter_samples = (ctx->sample_rate * (gwb89_u32)ctx->preset.exciter_ms) / 1000U;
    if (ctx->exciter_samples == 0U) ctx->exciter_samples = 1U;
    ctx->tail_remaining = (ctx->sample_rate * (gwb89_u32)ctx->preset.tail_ms) / 1000U;
}

gwb89_s16 gwb89_process_sample(gwb89_context *ctx, gwb89_s16 input)
{
    gwb89_s32 excite;
    gwb89_s32 sum;
    gwb89_s32 out;
    gwb89_s16 n;
    gwb89_s16 read;
    gwb89_s16 filtered;
    gwb89_s16 feedback;
    gwb89_s16 write;
    gwb89_s16 delta;
    gwb89_s16 env;
    gwb89_u32 remain;
    gwb89_u32 i;
    gwb89_u16 pos;
    gwb89_u16 delay;

    if (ctx == 0) return input;
    excite = gwb89_mul_q15(input, ctx->preset.input_gain_q15);
    if (ctx->exciter_pos < ctx->exciter_samples) {
        remain = ctx->exciter_samples - ctx->exciter_pos;
        env = (gwb89_s16)((remain * 32767U) / ctx->exciter_samples);
        n = gwb89_noise(&ctx->rng);
        excite += gwb89_mul_q15(gwb89_mul_q15(n, ctx->preset.exciter_noise_q15),
                                gwb89_mul_q15(env, ctx->trigger_level_q15));
        if (ctx->exciter_pos == 0U) excite += ctx->trigger_level_q15;
        ctx->exciter_pos++;
    }

    sum = 0;
    for (i = 0U; i < (gwb89_u32)ctx->preset.mode_count; ++i) {
        delay = ctx->runtime_delay[i];
        pos = ctx->write_pos[i];
        read = ctx->delay[i][pos];
        delta = gwb89_sat16((gwb89_s32)read - (gwb89_s32)ctx->damp_state[i]);
        ctx->damp_state[i] = gwb89_sat16((gwb89_s32)ctx->damp_state[i] +
            (gwb89_s32)gwb89_mul_q15(delta, ctx->preset.mode[i].damping_q15));
        filtered = ctx->damp_state[i];
        feedback = gwb89_mul_q15(filtered, ctx->preset.mode[i].feedback_q15);
        write = gwb89_sat16(excite + (gwb89_s32)feedback);
        ctx->delay[i][pos] = write;
        pos++;
        if (pos >= delay) pos = 0U;
        ctx->write_pos[i] = pos;
        sum += (gwb89_s32)gwb89_mul_q15(filtered, ctx->preset.mode[i].gain_q15);
    }
    sum /= (gwb89_s32)ctx->preset.mode_count;
    out = (gwb89_s32)gwb89_mul_q15(input, ctx->preset.dry_q15) +
          (gwb89_s32)gwb89_mul_q15(gwb89_sat16(sum), ctx->preset.wet_q15);
    out = gwb89_mul_q15(gwb89_sat16(out), ctx->preset.output_gain_q15);
    if (ctx->tail_remaining > 0U) ctx->tail_remaining--;
    return gwb89_sat16(out);
}

gwb89_u32 gwb89_process(gwb89_context *ctx, const gwb89_s16 *input,
                        gwb89_s16 *output, gwb89_u32 frames)
{
    gwb89_u32 i;
    if (ctx == 0 || output == 0) return 0U;
    for (i = 0U; i < frames; ++i) {
        output[i] = gwb89_process_sample(ctx, input ? input[i] : 0);
    }
    return frames;
}

int gwb89_is_active(const gwb89_context *ctx)
{
    if (ctx == 0) return 0;
    return (ctx->tail_remaining > 0U || ctx->exciter_pos < ctx->exciter_samples) ? 1 : 0;
}

const char *gwb89_preset_name(gwb89_preset_id id)
{
    static const char *names[GWB89_PRESET_COUNT] = {
        "pistol", "magnum", "shotgun", "rifle", "sniper", "launcher"
    };
    if ((int)id < 0 || id >= GWB89_PRESET_COUNT) return "invalid";
    return names[(int)id];
}

gwb89_u32 gwb89_context_bytes(void)
{
    return (gwb89_u32)sizeof(gwb89_context);
}
