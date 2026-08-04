#include "wsoundroom89.h"

static wsound89_i16 wroom_sat16(wsound89_i32 v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (wsound89_i16)v;
}

static wsound89_u32 wroom_ms(wsound89_u32 rate, wsound89_u32 ms)
{
    return (rate * ms) / 1000U;
}

wsound89_u32 wsoundroom89_required_samples(wsound89_u32 sample_rate, wsoundroom89_preset preset)
{
    static const wsound89_u16 max_ms[5] = { 73, 137, 211, 193, 91 };
    if (sample_rate < 8000U || (wsound89_u32)preset > (wsound89_u32)WSOUNDROOM89_OUTDOOR) return 0U;
    return wroom_ms(sample_rate, max_ms[(wsound89_u16)preset]) + 2U;
}

wsound89_result wsoundroom89_init(wsoundroom89_context *ctx, wsound89_u32 sample_rate,
                                  wsoundroom89_preset preset, wsound89_i16 *memory,
                                  wsound89_u32 memory_samples)
{
    static const wsound89_u16 delays_ms[5][WSOUNDROOM89_TAPS] = {
        { 7, 13, 19, 31, 47, 71 },
        { 11, 23, 41, 67, 101, 137 },
        { 17, 37, 61, 103, 157, 211 },
        { 19, 43, 79, 113, 151, 193 },
        { 13, 29, 47, 61, 73, 89 }
    };
    static const wsound89_i16 gains[5][WSOUNDROOM89_TAPS] = {
        { 10000, 8700, 7600, 6200, 4800, 3600 },
        { 9200, 8200, 7300, 6400, 5400, 4500 },
        { 8600, 7900, 7000, 6200, 5500, 4800 },
        { 9000, 8400, 7600, 6800, 6000, 5200 },
        { 7000, 5600, 4300, 3200, 2400, 1700 }
    };
    static const wsound89_i16 pans[WSOUNDROOM89_TAPS] = { -24000, 21000, -13000, 9000, -5000, 17000 };
    static const wsound89_i16 feedback[5] = { 18200, 22200, 24800, 25400, 9000 };
    static const wsound89_i16 wet[5] = { 17000, 18400, 20500, 21500, 10500 };
    static const wsound89_i16 damping[5] = { 12000, 10500, 9000, 8200, 16000 };
    wsound89_u32 need;
    wsound89_u16 i;
    if (ctx == 0 || memory == 0) return WSOUND89_EINVAL;
    need = wsoundroom89_required_samples(sample_rate, preset);
    if (need == 0U || memory_samples < need) return WSOUND89_ECAPACITY;
    ctx->delay = memory;
    ctx->capacity = memory_samples;
    ctx->write_pos = 0U;
    for (i = 0U; i < WSOUNDROOM89_TAPS; ++i) {
        ctx->tap_delay[i] = wroom_ms(sample_rate, delays_ms[(wsound89_u16)preset][i]);
        ctx->tap_gain_q15[i] = gains[(wsound89_u16)preset][i];
        ctx->tap_pan_q15[i] = pans[i];
    }
    ctx->feedback_q15 = feedback[(wsound89_u16)preset];
    ctx->wet_q15 = wet[(wsound89_u16)preset];
    ctx->damping_q15 = damping[(wsound89_u16)preset];
    ctx->damp_state = 0;
    wsoundroom89_reset(ctx);
    return WSOUND89_OK;
}

void wsoundroom89_reset(wsoundroom89_context *ctx)
{
    wsound89_u32 i;
    if (ctx == 0 || ctx->delay == 0) return;
    for (i = 0U; i < ctx->capacity; ++i) ctx->delay[i] = 0;
    ctx->write_pos = 0U;
    ctx->damp_state = 0;
}

void wsoundroom89_process_sample(wsoundroom89_context *ctx, wsound89_i16 input,
                                 wsound89_i16 *out_left, wsound89_i16 *out_right)
{
    wsound89_u16 i;
    wsound89_u32 read_pos;
    wsound89_i32 tap;
    wsound89_i32 mono;
    wsound89_i32 left;
    wsound89_i32 right;
    wsound89_i32 gl;
    wsound89_i32 gr;
    wsound89_i32 write_value;
    if (out_left == 0 || out_right == 0) return;
    if (ctx == 0 || ctx->delay == 0) { *out_left = 0; *out_right = 0; return; }
    mono = 0;
    left = 0;
    right = 0;
    for (i = 0U; i < WSOUNDROOM89_TAPS; ++i) {
        if (ctx->write_pos >= ctx->tap_delay[i]) read_pos = ctx->write_pos - ctx->tap_delay[i];
        else read_pos = ctx->capacity + ctx->write_pos - ctx->tap_delay[i];
        tap = ctx->delay[read_pos];
        tap = (tap * ctx->tap_gain_q15[i]) >> 15;
        gl = 32767 - ctx->tap_pan_q15[i];
        gr = 32767 + ctx->tap_pan_q15[i];
        left += (tap * gl) >> 16;
        right += (tap * gr) >> 16;
        mono += tap;
    }
    mono /= (wsound89_i32)WSOUNDROOM89_TAPS;
    ctx->damp_state += ((mono - ctx->damp_state) * ctx->damping_q15) >> 15;
    write_value = input + ((ctx->damp_state * ctx->feedback_q15) >> 15);
    ctx->delay[ctx->write_pos] = wroom_sat16(write_value);
    ctx->write_pos++;
    if (ctx->write_pos >= ctx->capacity) ctx->write_pos = 0U;
    left = (left * ctx->wet_q15) >> 15;
    right = (right * ctx->wet_q15) >> 15;
    *out_left = wroom_sat16(left);
    *out_right = wroom_sat16(right);
}
