#include "glatetail89.h"

static glt89_s16 glt89_sat16(glt89_s32 x)
{
    if (x > 32767) return (glt89_s16)32767;
    if (x < -32768) return (glt89_s16)-32768;
    return (glt89_s16)x;
}

static glt89_s16 glt89_mul_q15(glt89_s16 a, glt89_s16 b)
{
    return glt89_sat16(((glt89_s32)a * (glt89_s32)b) >> 15);
}

static void glt89_zero16(glt89_s16 *p, glt89_u32 n)
{
    glt89_u32 i;
    for (i = 0U; i < n; ++i) p[i] = 0;
}

static const glt89_preset glt89_presets[GLT89_PRESET_COUNT] = {
    {12,{37,43,53,61},23500,12500,16500,14500,32767,28000,650},
    {24,{67,79,97,113},26500,11200,17500,18000,32767,28000,1250},
    {38,{101,127,149,179},28600,9800,19000,22500,32767,28500,2300},
    {52,{137,163,191,227},29400,9000,19500,23500,32767,28500,2850},
    {28,{83,131,197,271},25200,7600,14500,13000,32767,28000,1500},
    {62,{181,223,269,317},30300,8200,21000,25500,32767,28800,3900}
};

int glt89_platform_ok(void)
{
    return (sizeof(glt89_s16) == 2U && sizeof(glt89_s32) >= 4U &&
            sizeof(glt89_u32) >= 4U) ? 1 : 0;
}

int glt89_get_preset(glt89_preset_id id, glt89_preset *out_preset)
{
    if (out_preset == 0) return 0;
    if ((int)id < 0 || id >= GLT89_PRESET_COUNT) return 0;
    *out_preset = glt89_presets[(int)id];
    return 1;
}

int glt89_init(glt89_context *ctx, glt89_u32 sample_rate,
               const glt89_preset *preset)
{
    glt89_u32 i;
    glt89_u32 d;
    if (ctx == 0 || preset == 0) return 0;
    if (!glt89_platform_ok()) return 0;
    if (sample_rate == 0U || sample_rate > GLT89_MAX_SAMPLE_RATE) return 0;
    ctx->sample_rate = sample_rate;
    ctx->preset = *preset;
    ctx->predelay_samples = (glt89_u16)((sample_rate * (glt89_u32)preset->predelay_ms) / 1000U);
    if (ctx->predelay_samples < 1U) ctx->predelay_samples = 1U;
    if (ctx->predelay_samples > GLT89_MAX_PREDELAY) ctx->predelay_samples = GLT89_MAX_PREDELAY;
    for (i = 0U; i < GLT89_LINES; ++i) {
        d = (sample_rate * (glt89_u32)preset->delay_ms[i]) / 1000U;
        if (d < 2U) d = 2U;
        if (d > GLT89_MAX_DELAY) d = GLT89_MAX_DELAY;
        ctx->runtime_delay[i] = (glt89_u16)d;
    }
    glt89_reset(ctx);
    return 1;
}

void glt89_reset(glt89_context *ctx)
{
    glt89_u32 i;
    if (ctx == 0) return;
    glt89_zero16(ctx->predelay, GLT89_MAX_PREDELAY);
    glt89_zero16(&ctx->delay[0][0], GLT89_LINES * GLT89_MAX_DELAY);
    glt89_zero16(ctx->damp_state, GLT89_LINES);
    ctx->predelay_write = 0U;
    for (i = 0U; i < GLT89_LINES; ++i) ctx->write_pos[i] = 0U;
    ctx->tail_remaining = 0U;
}

glt89_s16 glt89_process_sample(glt89_context *ctx, glt89_s16 input)
{
    glt89_s16 pre;
    glt89_s16 y[GLT89_LINES];
    glt89_s16 f[GLT89_LINES];
    glt89_s16 delta;
    glt89_s16 write;
    glt89_s32 sum;
    glt89_s32 out;
    glt89_u32 i;
    glt89_u16 p;
    if (ctx == 0) return input;

    pre = ctx->predelay[ctx->predelay_write];
    ctx->predelay[ctx->predelay_write] = input;
    ctx->predelay_write++;
    if (ctx->predelay_write >= ctx->predelay_samples) ctx->predelay_write = 0U;

    for (i = 0U; i < GLT89_LINES; ++i) {
        p = ctx->write_pos[i];
        y[i] = ctx->delay[i][p];
        delta = glt89_sat16((glt89_s32)y[i] - ctx->damp_state[i]);
        ctx->damp_state[i] = glt89_sat16((glt89_s32)ctx->damp_state[i] +
            glt89_mul_q15(delta, ctx->preset.damping_q15));
        y[i] = ctx->damp_state[i];
    }

    f[0] = glt89_sat16(((glt89_s32)y[0] + y[1] + y[2] + y[3]) >> 1);
    f[1] = glt89_sat16(((glt89_s32)y[0] - y[1] + y[2] - y[3]) >> 1);
    f[2] = glt89_sat16(((glt89_s32)y[0] + y[1] - y[2] - y[3]) >> 1);
    f[3] = glt89_sat16(((glt89_s32)y[0] - y[1] - y[2] + y[3]) >> 1);

    for (i = 0U; i < GLT89_LINES; ++i) {
        write = glt89_sat16((glt89_s32)glt89_mul_q15(pre, ctx->preset.input_gain_q15) +
                            glt89_mul_q15(f[i], ctx->preset.feedback_q15));
        p = ctx->write_pos[i];
        ctx->delay[i][p] = write;
        p++;
        if (p >= ctx->runtime_delay[i]) p = 0U;
        ctx->write_pos[i] = p;
    }

    sum = (glt89_s32)y[0] + y[1] + y[2] + y[3];
    sum >>= 2;
    out = (glt89_s32)glt89_mul_q15(input, ctx->preset.dry_q15) +
          glt89_mul_q15(glt89_sat16(sum), ctx->preset.wet_q15);
    out = glt89_mul_q15(glt89_sat16(out), ctx->preset.output_gain_q15);
    if (input != 0) {
        ctx->tail_remaining = (ctx->sample_rate * (glt89_u32)ctx->preset.bounded_tail_ms) / 1000U;
    } else if (ctx->tail_remaining > 0U) {
        ctx->tail_remaining--;
    }
    return glt89_sat16(out);
}

glt89_u32 glt89_process(glt89_context *ctx, const glt89_s16 *input,
                        glt89_s16 *output, glt89_u32 frames)
{
    glt89_u32 i;
    if (ctx == 0 || output == 0) return 0U;
    for (i = 0U; i < frames; ++i) {
        output[i] = glt89_process_sample(ctx, input ? input[i] : 0);
    }
    return frames;
}

void glt89_inject(glt89_context *ctx, glt89_s16 impulse)
{
    if (ctx == 0) return;
    ctx->predelay[ctx->predelay_write] = impulse;
    ctx->tail_remaining = (ctx->sample_rate * (glt89_u32)ctx->preset.bounded_tail_ms) / 1000U;
}

int glt89_is_active(const glt89_context *ctx)
{
    if (ctx == 0) return 0;
    return ctx->tail_remaining > 0U ? 1 : 0;
}

const char *glt89_preset_name(glt89_preset_id id)
{
    static const char *names[GLT89_PRESET_COUNT] = {
        "small_room", "corridor", "warehouse", "tunnel", "exterior", "cathedral"
    };
    if ((int)id < 0 || id >= GLT89_PRESET_COUNT) return "invalid";
    return names[(int)id];
}

glt89_u32 glt89_context_bytes(void)
{
    return (glt89_u32)sizeof(glt89_context);
}
