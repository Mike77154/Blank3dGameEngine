#include "gklek89.h"

#define GKL89_U32_MASK 0xFFFFFFFFUL

static const gkl89_preset gkl89_presets[GKL89_PRESET_COUNT] = {
    {
        "dry_click", 34U, 3U,
        28500, 11800, 15600, 9000,
        32480, 32680, 19000, 4200,
        9U, 15U, 20500, 7200
    },
    {
        "steel_klek", 68U, 7U,
        30000, 16800, 20400, 22600,
        32620, 32720, 20500, 3600,
        11U, 17U, 26300, 11600
    },
    {
        "carrier_klek", 82U, 10U,
        28200, 18200, 17800, 28600,
        32670, 32738, 18400, 3000,
        17U, 23U, 27800, 9800
    },
    {
        "rear_stop", 92U, 8U,
        31700, 15600, 16600, 30600,
        32680, 32744, 16800, 2500,
        19U, 29U, 28600, 13200
    }
};

static gkl89_s16 gkl89_sat16(gkl89_s32 x)
{
    if (x > 32767L) return 32767;
    if (x < -32768L) return -32768;
    return (gkl89_s16)x;
}

static gkl89_s16 gkl89_mul_q15(gkl89_s16 a, gkl89_s16 b)
{
    return gkl89_sat16(((gkl89_s32)a * (gkl89_s32)b) >> 15);
}

static gkl89_u32 gkl89_rng_next(gkl89_u32 *state)
{
    gkl89_u32 x;
    x = *state & GKL89_U32_MASK;
    if (x == 0UL) x = 0x4B4C454BUL;
    x ^= (x << 13) & GKL89_U32_MASK;
    x ^= (x >> 17);
    x ^= (x << 5) & GKL89_U32_MASK;
    x &= GKL89_U32_MASK;
    *state = x;
    return x;
}

static gkl89_s16 gkl89_noise(gkl89_u32 *state)
{
    gkl89_u32 x;
    x = gkl89_rng_next(state);
    return (gkl89_s16)((gkl89_s32)((x >> 16) & 65535UL) - 32768L);
}

static void gkl89_zero(gkl89_s16 *p, unsigned short count)
{
    unsigned short i;
    for (i = 0U; i < count; ++i) p[i] = 0;
}

static gkl89_s16 gkl89_one_pole(gkl89_s16 state,
                                 gkl89_s16 input,
                                 gkl89_s16 alpha_q15)
{
    gkl89_s16 delta;
    delta = gkl89_sat16((gkl89_s32)input - (gkl89_s32)state);
    return gkl89_sat16((gkl89_s32)state +
                       (gkl89_s32)gkl89_mul_q15(delta, alpha_q15));
}

static gkl89_s16 gkl89_soft_drive(gkl89_s16 x, gkl89_s16 amount_q15)
{
    gkl89_s32 pre;
    gkl89_s32 a;
    gkl89_s32 sign;
    gkl89_s32 knee;
    gkl89_s32 curved;
    gkl89_s32 wet;
    gkl89_s32 out;

    pre = (gkl89_s32)x * (32768L + ((gkl89_s32)amount_q15 >> 1));
    pre >>= 15;
    sign = pre < 0L ? -1L : 1L;
    a = pre < 0L ? -pre : pre;
    knee = 22500L - (((gkl89_s32)amount_q15 * 7000L) >> 15);
    if (a <= knee) curved = a;
    else curved = knee + ((a - knee) * (32767L - knee)) /
                         ((a - knee) + (32767L - knee));
    curved *= sign;
    wet = 5000L + (((gkl89_s32)amount_q15 * 21000L) >> 15);
    out = ((gkl89_s32)x * (32767L - wet) + curved * wet) >> 15;
    return gkl89_sat16(out);
}

void gkl89_reset(gkl89_context *ctx, gkl89_u32 seed)
{
    if (ctx == 0) return;
    ctx->rng = seed == 0UL ? 0x4B4C454BUL : seed;
    ctx->delay_samples = 0UL;
    ctx->sample_clock = 0UL;
    ctx->end_clock = 0UL;
    ctx->rebound_sample = 0UL;
    ctx->noise_env_q15 = 0;
    ctx->ring_env_q15 = 0;
    ctx->previous_noise = 0;
    ctx->low_fast = 0;
    ctx->low_slow = 0;
    gkl89_zero(ctx->comb_a, GKL89_COMB_CAPACITY);
    gkl89_zero(ctx->comb_b, GKL89_COMB_CAPACITY);
    ctx->comb_pos_a = 0U;
    ctx->comb_pos_b = 0U;
    ctx->active = 0U;
}

void gkl89_init(gkl89_context *ctx, gkl89_u32 seed)
{
    if (ctx == 0) return;
    ctx->preset = gkl89_presets[GKL89_PRESET_STEEL_KLEK];
    gkl89_reset(ctx, seed);
}

int gkl89_get_preset(gkl89_preset_id preset_id, gkl89_preset *out_preset)
{
    if (out_preset == 0) return 0;
    if ((int)preset_id < 0 || preset_id >= GKL89_PRESET_COUNT) return 0;
    *out_preset = gkl89_presets[(int)preset_id];
    return 1;
}

const char *gkl89_preset_name(gkl89_preset_id preset_id)
{
    if ((int)preset_id < 0 || preset_id >= GKL89_PRESET_COUNT) {
        return "invalid";
    }
    return gkl89_presets[(int)preset_id].name;
}

int gkl89_trigger_custom(gkl89_context *ctx, const gkl89_preset *preset,
                         gkl89_u32 seed, unsigned short delay_ms)
{
    if (ctx == 0 || preset == 0) return 0;
    if (preset->comb_delay_a == 0U || preset->comb_delay_a >= GKL89_COMB_CAPACITY) return 0;
    if (preset->comb_delay_b == 0U || preset->comb_delay_b >= GKL89_COMB_CAPACITY) return 0;
    ctx->preset = *preset;
    gkl89_reset(ctx, seed);
    ctx->preset = *preset;
    ctx->delay_samples = (GKL89_SAMPLE_RATE * (gkl89_u32)delay_ms + 999UL) / 1000UL;
    ctx->end_clock = (GKL89_SAMPLE_RATE * (gkl89_u32)preset->duration_ms + 999UL) / 1000UL;
    ctx->rebound_sample = (GKL89_SAMPLE_RATE * (gkl89_u32)preset->rebound_ms + 999UL) / 1000UL;
    ctx->noise_env_q15 = GKL89_Q15_ONE;
    ctx->ring_env_q15 = GKL89_Q15_ONE;
    ctx->active = 1U;
    return 1;
}

int gkl89_trigger(gkl89_context *ctx, gkl89_preset_id preset_id,
                  gkl89_u32 seed, unsigned short delay_ms)
{
    gkl89_preset p;
    if (!gkl89_get_preset(preset_id, &p)) return 0;
    return gkl89_trigger_custom(ctx, &p, seed, delay_ms);
}

gkl89_s16 gkl89_process_sample(gkl89_context *ctx)
{
    gkl89_s16 raw_noise;
    gkl89_s16 high;
    gkl89_s16 band;
    gkl89_s16 shaped_noise;
    gkl89_s16 impulse;
    gkl89_s16 read_a;
    gkl89_s16 read_b;
    gkl89_s16 ring_in;
    gkl89_s16 ring;
    gkl89_s32 total;
    unsigned char pa;
    unsigned char pb;

    if (ctx == 0 || !ctx->active) return 0;
    if (ctx->delay_samples > 0UL) {
        ctx->delay_samples--;
        return 0;
    }
    if (ctx->sample_clock >= ctx->end_clock) {
        ctx->active = 0U;
        return 0;
    }

    raw_noise = gkl89_noise(&ctx->rng);
    ctx->low_fast = gkl89_one_pole(ctx->low_fast, raw_noise,
                                   ctx->preset.fast_alpha_q15);
    ctx->low_slow = gkl89_one_pole(ctx->low_slow, raw_noise,
                                   ctx->preset.slow_alpha_q15);
    high = gkl89_sat16((gkl89_s32)raw_noise - (gkl89_s32)ctx->previous_noise);
    band = gkl89_sat16((gkl89_s32)ctx->low_fast - (gkl89_s32)ctx->low_slow);
    ctx->previous_noise = raw_noise;
    shaped_noise = gkl89_sat16(((gkl89_s32)band * 7L +
                                  (gkl89_s32)high) >> 3);

    impulse = 0;
    if (ctx->sample_clock == 0UL) impulse = ctx->preset.impact_q15;
    else if (ctx->sample_clock == 1UL) impulse = (gkl89_s16)(-ctx->preset.impact_q15 / 2);
    else if (ctx->sample_clock == ctx->rebound_sample) impulse = ctx->preset.rebound_q15;
    else if (ctx->sample_clock == ctx->rebound_sample + 1UL) impulse = (gkl89_s16)(-ctx->preset.rebound_q15 / 3);

    pa = ctx->comb_pos_a;
    pb = ctx->comb_pos_b;
    read_a = ctx->comb_a[pa];
    read_b = ctx->comb_b[pb];

    ring_in = gkl89_sat16((gkl89_s32)impulse +
        (gkl89_s32)gkl89_mul_q15(shaped_noise,
                                  gkl89_mul_q15(ctx->noise_env_q15, 10500)));

    ctx->comb_a[pa] = gkl89_sat16((gkl89_s32)ring_in +
        (gkl89_s32)gkl89_mul_q15(read_a, ctx->preset.comb_feedback_q15));
    ctx->comb_b[pb] = gkl89_sat16((gkl89_s32)ring_in +
        (gkl89_s32)gkl89_mul_q15(read_b,
                                  (gkl89_s16)(ctx->preset.comb_feedback_q15 - 1200)));

    pa++;
    if (pa >= ctx->preset.comb_delay_a) pa = 0U;
    pb++;
    if (pb >= ctx->preset.comb_delay_b) pb = 0U;
    ctx->comb_pos_a = pa;
    ctx->comb_pos_b = pb;

    ring = gkl89_sat16(((gkl89_s32)read_a + (gkl89_s32)read_b) >> 1);
    total = (gkl89_s32)impulse;
    total += (gkl89_s32)gkl89_mul_q15(
        shaped_noise,
        gkl89_mul_q15(ctx->preset.noise_q15, ctx->noise_env_q15));
    total += (gkl89_s32)gkl89_mul_q15(
        ring,
        gkl89_mul_q15(ctx->preset.ring_q15, ctx->ring_env_q15));

    ctx->noise_env_q15 = gkl89_mul_q15(ctx->noise_env_q15,
                                       ctx->preset.noise_decay_q15);
    ctx->ring_env_q15 = gkl89_mul_q15(ctx->ring_env_q15,
                                      ctx->preset.ring_decay_q15);
    ctx->sample_clock++;

    return gkl89_soft_drive(gkl89_sat16(total), ctx->preset.drive_q15);
}

gkl89_u32 gkl89_render_mono(gkl89_context *ctx, gkl89_s16 *dst,
                            gkl89_u32 frames)
{
    gkl89_u32 i;
    if (ctx == 0 || dst == 0) return 0UL;
    for (i = 0UL; i < frames; ++i) dst[i] = gkl89_process_sample(ctx);
    return frames;
}

int gkl89_is_active(const gkl89_context *ctx)
{
    if (ctx == 0) return 0;
    return ctx->active ? 1 : 0;
}

gkl89_u32 gkl89_context_bytes(void)
{
    return (gkl89_u32)sizeof(gkl89_context);
}
