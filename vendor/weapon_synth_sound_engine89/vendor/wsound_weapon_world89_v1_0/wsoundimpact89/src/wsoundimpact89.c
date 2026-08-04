#include "wsoundimpact89.h"

static wsound89_i16 wi_sat16(wsound89_i32 v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (wsound89_i16)v;
}

static wsound89_i16 wi_noise(wsoundimpact89_context *ctx)
{
    ctx->state = ctx->state * 1664525U + 1013904223U;
    return (wsound89_i16)((ctx->state >> 16) & 65535U);
}

wsound89_result wsoundimpact89_init(wsoundimpact89_context *ctx, wsound89_u32 seed)
{
    wsound89_u16 m;
    wsound89_u16 i;
    if (ctx == 0) return WSOUND89_EINVAL;
    ctx->state = seed == 0U ? 0x57494D50U : seed;
    ctx->envelope = 0;
    ctx->noise_gain_q15 = 0;
    ctx->master_q15 = 25000;
    ctx->active = 0U;
    for (m = 0U; m < WSOUNDIMPACT89_MODES; ++m) {
        ctx->mode[m].index = 0U;
        ctx->mode[m].length = 8U;
        ctx->mode[m].feedback_q15 = 0;
        ctx->mode[m].gain_q15 = 0;
        for (i = 0U; i < WSOUNDIMPACT89_DELAY_MAX; ++i) ctx->mode[m].delay[i] = 0;
    }
    return WSOUND89_OK;
}

wsound89_result wsoundimpact89_trigger(wsoundimpact89_context *ctx, wsoundimpact89_material material,
                                       wsound89_u16 energy_q15, wsound89_u16 size_q15)
{
    static const wsound89_u8 lengths[6][WSOUNDIMPACT89_MODES] = {
        { 23, 17, 11, 7 }, { 19, 13, 9, 6 }, { 24, 18, 13, 9 },
        { 20, 15, 11, 8 }, { 22, 14, 9, 5 }, { 21, 16, 12, 9 }
    };
    static const wsound89_i16 feedback[6][WSOUNDIMPACT89_MODES] = {
        { 30000, 29200, 27800, 26000 }, { 25200, 23000, 20500, 18000 },
        { 28600, 27000, 24800, 22000 }, { 18800, 16800, 14800, 12800 },
        { 30600, 29800, 28600, 27000 }, { 19600, 17600, 15600, 13600 }
    };
    static const wsound89_i16 gains[6][WSOUNDIMPACT89_MODES] = {
        { 12000, 9800, 7600, 5200 }, { 9000, 7600, 5900, 4300 },
        { 10800, 9000, 7000, 5000 }, { 6500, 5400, 4300, 3200 },
        { 12500, 10300, 8100, 5700 }, { 7200, 5900, 4600, 3500 }
    };
    static const wsound89_i16 noise_gain[6] = { 9000, 18000, 12000, 23000, 16000, 14000 };
    wsound89_u16 m;
    wsound89_u16 i;
    wsound89_i32 scale;
    if (ctx == 0 || (wsound89_u32)material > (wsound89_u32)WSOUNDIMPACT89_FLESH) return WSOUND89_EINVAL;
    if (energy_q15 > 32767U) energy_q15 = 32767U;
    if (size_q15 > 32767U) size_q15 = 32767U;
    scale = 24576 + ((wsound89_i32)size_q15 >> 2);
    for (m = 0U; m < WSOUNDIMPACT89_MODES; ++m) {
        ctx->mode[m].length = lengths[(wsound89_u16)material][m];
        ctx->mode[m].feedback_q15 = feedback[(wsound89_u16)material][m];
        ctx->mode[m].gain_q15 = gains[(wsound89_u16)material][m];
        ctx->mode[m].index = 0U;
        for (i = 0U; i < WSOUNDIMPACT89_DELAY_MAX; ++i) ctx->mode[m].delay[i] = 0;
    }
    ctx->envelope = energy_q15;
    ctx->noise_gain_q15 = (wsound89_i16)((noise_gain[(wsound89_u16)material] * scale) >> 15);
    ctx->active = 1U;
    return WSOUND89_OK;
}

wsound89_i16 wsoundimpact89_process_sample(wsoundimpact89_context *ctx)
{
    wsound89_i32 excitation;
    wsound89_i32 sum;
    wsound89_i32 delayed;
    wsound89_i32 write_value;
    wsound89_i16 n;
    wsound89_u16 m;
    wsoundimpact89_mode *mode;
    if (ctx == 0 || ctx->active == 0U) return 0;
    n = wi_noise(ctx);
    excitation = ((wsound89_i32)n * ctx->noise_gain_q15) >> 15;
    excitation = (excitation * ctx->envelope) >> 15;
    if (ctx->envelope > 26000) excitation += ctx->envelope / 2;
    sum = 0;
    for (m = 0U; m < WSOUNDIMPACT89_MODES; ++m) {
        mode = &ctx->mode[m];
        delayed = mode->delay[mode->index];
        write_value = excitation + ((delayed * mode->feedback_q15) >> 15);
        mode->delay[mode->index] = wi_sat16(write_value);
        mode->index++;
        if (mode->index >= mode->length) mode->index = 0U;
        sum += (delayed * mode->gain_q15) >> 15;
    }
    sum = (sum * ctx->master_q15) >> 15;
    ctx->envelope -= (ctx->envelope >> 7) + 12;
    if (ctx->envelope < 24) { ctx->envelope = 0; ctx->active = 0U; }
    return wi_sat16(sum + (excitation >> 2));
}

int wsoundimpact89_is_active(const wsoundimpact89_context *ctx)
{
    return ctx != 0 && ctx->active != 0U;
}
