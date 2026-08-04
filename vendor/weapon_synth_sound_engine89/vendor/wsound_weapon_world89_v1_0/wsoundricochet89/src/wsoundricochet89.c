#include "wsoundricochet89.h"

static wsound89_i16 wri_sat16(wsound89_i32 v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (wsound89_i16)v;
}

static wsound89_i16 wri_noise(wsoundricochet89_context *ctx)
{
    ctx->state = ctx->state * 1103515245U + 12345U;
    return (wsound89_i16)((ctx->state >> 16) & 65535U);
}

wsound89_result wsoundricochet89_init(wsoundricochet89_context *ctx, wsound89_u32 seed)
{
    wsound89_u16 i;
    if (ctx == 0) return WSOUND89_EINVAL;
    for (i = 0U; i < 32U; ++i) ctx->delay[i] = 0;
    ctx->state = seed == 0U ? 0x57524943U : seed;
    ctx->frame = 0U;
    ctx->total_frames = 0U;
    ctx->envelope = 0;
    ctx->index = 0U;
    ctx->delay_length = 6U;
    ctx->target_length = 20U;
    ctx->feedback_q15 = 28000;
    ctx->noise_q15 = 8000;
    ctx->active = 0U;
    return WSOUND89_OK;
}

wsound89_result wsoundricochet89_trigger(wsoundricochet89_context *ctx, wsound89_u16 energy_q15,
                                         wsound89_u16 grazing_q15, wsound89_u16 roughness_q15,
                                         wsound89_u32 sample_rate)
{
    wsound89_u16 i;
    wsound89_u32 ms;
    if (ctx == 0 || sample_rate < 8000U) return WSOUND89_EINVAL;
    if (energy_q15 > 32767U) energy_q15 = 32767U;
    if (grazing_q15 > 32767U) grazing_q15 = 32767U;
    if (roughness_q15 > 32767U) roughness_q15 = 32767U;
    for (i = 0U; i < 32U; ++i) ctx->delay[i] = 0;
    ms = 45U + ((wsound89_u32)grazing_q15 * 110U) / 32767U;
    ctx->total_frames = (sample_rate * ms) / 1000U;
    ctx->frame = 0U;
    ctx->envelope = energy_q15;
    ctx->index = 0U;
    ctx->delay_length = 4U + (roughness_q15 >> 13);
    ctx->target_length = 15U + (grazing_q15 >> 11);
    if (ctx->target_length > 31U) ctx->target_length = 31U;
    ctx->feedback_q15 = (wsound89_i16)(25000 + (grazing_q15 >> 2));
    ctx->noise_q15 = (wsound89_i16)(5000 + (roughness_q15 >> 1));
    ctx->active = 1U;
    return WSOUND89_OK;
}

wsound89_i16 wsoundricochet89_process_sample(wsoundricochet89_context *ctx)
{
    wsound89_i32 delayed;
    wsound89_i32 input;
    wsound89_i32 out;
    wsound89_i16 n;
    if (ctx == 0 || ctx->active == 0U) return 0;
    n = wri_noise(ctx);
    delayed = ctx->delay[ctx->index];
    input = ((wsound89_i32)n * ctx->noise_q15) >> 15;
    input = (input * ctx->envelope) >> 15;
    if (ctx->frame == 0U) input += ctx->envelope;
    if ((ctx->frame % 211U) == 0U) input += ctx->envelope >> 2;
    out = delayed + (input >> 1);
    ctx->delay[ctx->index] = wri_sat16(input + ((delayed * ctx->feedback_q15) >> 15));
    ctx->index++;
    if (ctx->index >= ctx->delay_length) ctx->index = 0U;
    if ((ctx->frame & 63U) == 0U && ctx->delay_length < ctx->target_length) ctx->delay_length++;
    ctx->envelope -= (ctx->envelope >> 8) + 7;
    ctx->frame++;
    if (ctx->frame >= ctx->total_frames || ctx->envelope < 20) ctx->active = 0U;
    return wri_sat16(out);
}

int wsoundricochet89_is_active(const wsoundricochet89_context *ctx)
{
    return ctx != 0 && ctx->active != 0U;
}
