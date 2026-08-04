#include "wsoundreceiver89.h"

static wsound89_i16 wr_sat16(wsound89_i32 v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (wsound89_i16)v;
}

static void wr_clear(wsoundreceiver89_context *ctx)
{
    wsound89_u16 m;
    wsound89_u16 i;
    for (m = 0U; m < WSOUNDRECEIVER89_MODES; ++m) {
        for (i = 0U; i < WSOUNDRECEIVER89_DELAY_MAX; ++i) ctx->mode[m].delay[i] = 0;
        ctx->mode[m].index = 0U;
    }
    ctx->excitation = 0;
}

wsound89_result wsoundreceiver89_init(wsoundreceiver89_context *ctx, wsoundreceiver89_preset preset)
{
    static const wsound89_u8 lengths[5][WSOUNDRECEIVER89_MODES] = {
        { 31, 23, 17, 13, 9, 7 },
        { 29, 21, 15, 11, 8, 6 },
        { 32, 27, 19, 14, 10, 8 },
        { 30, 24, 18, 13, 10, 7 },
        { 32, 25, 18, 12, 8, 5 }
    };
    static const wsound89_i16 feedback[5][WSOUNDRECEIVER89_MODES] = {
        { 28600, 28000, 27000, 25800, 24400, 22600 },
        { 29200, 28600, 27800, 26500, 24800, 23200 },
        { 30000, 29400, 28600, 27200, 25200, 22600 },
        { 26600, 25800, 24800, 23200, 21400, 19600 },
        { 30400, 29800, 29000, 28000, 26400, 24600 }
    };
    static const wsound89_i16 gains[5][WSOUNDRECEIVER89_MODES] = {
        { 9000, 8000, 6800, 5600, 4200, 3000 },
        { 9800, 8600, 7200, 5900, 4500, 3300 },
        { 10800, 9400, 7600, 6100, 4300, 2800 },
        { 7600, 6500, 5400, 4300, 3200, 2200 },
        { 11800, 10000, 8200, 6600, 5000, 3500 }
    };
    wsound89_u16 m;
    if (ctx == 0) return WSOUND89_EINVAL;
    if ((wsound89_u32)preset > (wsound89_u32)WSOUNDRECEIVER89_HEAVY) return WSOUND89_EINVAL;
    for (m = 0U; m < WSOUNDRECEIVER89_MODES; ++m) {
        ctx->mode[m].length = lengths[(wsound89_u16)preset][m];
        ctx->mode[m].feedback_q15 = feedback[(wsound89_u16)preset][m];
        ctx->mode[m].gain_q15 = gains[(wsound89_u16)preset][m];
    }
    ctx->master_q15 = 24500;
    wr_clear(ctx);
    return WSOUND89_OK;
}

void wsoundreceiver89_reset(wsoundreceiver89_context *ctx)
{
    if (ctx != 0) wr_clear(ctx);
}

void wsoundreceiver89_excite(wsoundreceiver89_context *ctx, wsound89_i16 impulse)
{
    if (ctx != 0) ctx->excitation += impulse;
}

wsound89_i16 wsoundreceiver89_process_sample(wsoundreceiver89_context *ctx, wsound89_i16 input)
{
    wsound89_u16 m;
    wsound89_i32 excitation;
    wsound89_i32 sum;
    wsound89_i32 delayed;
    wsound89_i32 write_value;
    wsoundreceiver89_mode *mode;
    if (ctx == 0) return 0;
    excitation = (wsound89_i32)input + ctx->excitation;
    ctx->excitation = 0;
    sum = 0;
    for (m = 0U; m < WSOUNDRECEIVER89_MODES; ++m) {
        mode = &ctx->mode[m];
        delayed = mode->delay[mode->index];
        write_value = excitation + ((delayed * mode->feedback_q15) >> 15);
        mode->delay[mode->index] = wr_sat16(write_value);
        mode->index++;
        if (mode->index >= mode->length) mode->index = 0U;
        sum += (delayed * mode->gain_q15) >> 15;
    }
    sum = (sum * ctx->master_q15) >> 15;
    return wr_sat16(sum);
}
