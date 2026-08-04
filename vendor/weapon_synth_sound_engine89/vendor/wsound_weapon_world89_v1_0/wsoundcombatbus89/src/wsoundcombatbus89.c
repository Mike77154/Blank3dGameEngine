#include "wsoundcombatbus89.h"

static wsound89_i32 wcb_abs(wsound89_i32 v) { return v < 0 ? -v : v; }
static wsound89_i16 wcb_sat16(wsound89_i32 v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (wsound89_i16)v;
}

wsound89_result wsoundcombatbus89_init(wsoundcombatbus89_context *ctx)
{
    wsound89_u16 i;
    if (ctx == 0) return WSOUND89_EINVAL;
    for (i = 0U; i < WSOUNDCOMBATBUS89_BUSES; ++i) ctx->gain_q15[i] = 32767;
    ctx->gain_q15[WSOUNDCOMBATBUS89_ROOM] = 24500;
    ctx->gain_q15[WSOUNDCOMBATBUS89_CASING] = 20000;
    ctx->gain_q15[WSOUNDCOMBATBUS89_AMBIENCE] = 22000;
    ctx->limiter_gain_q15 = 32767;
    ctx->tail_gain_q15 = 32767;
    ctx->transient_threshold = 10500;
    ctx->limiter_threshold = 29200;
    ctx->tail_recovery_shift = 10U;
    ctx->limiter_release_shift = 11U;
    return WSOUND89_OK;
}

void wsoundcombatbus89_set_gain(wsoundcombatbus89_context *ctx, wsoundcombatbus89_bus bus,
                                wsound89_i16 gain_q15)
{
    if (ctx == 0 || (wsound89_u32)bus >= WSOUNDCOMBATBUS89_BUSES) return;
    if (gain_q15 < 0) gain_q15 = 0;
    ctx->gain_q15[(wsound89_u16)bus] = gain_q15;
}

void wsoundcombatbus89_process_sample(wsoundcombatbus89_context *ctx,
                                      const wsoundcombatbus89_frame *input,
                                      wsound89_i16 *out_left, wsound89_i16 *out_right)
{
    wsound89_i32 l;
    wsound89_i32 r;
    wsound89_i32 transient;
    wsound89_i32 peak;
    wsound89_i32 target;
    wsound89_i32 g;
    wsound89_u16 i;
    if (out_left == 0 || out_right == 0) return;
    if (ctx == 0 || input == 0) { *out_left = 0; *out_right = 0; return; }
    transient = wcb_abs(input->left[0]) + wcb_abs(input->right[0]);
    transient += wcb_abs(input->left[1]) + wcb_abs(input->right[1]);
    transient += wcb_abs(input->left[3]) + wcb_abs(input->right[3]);
    transient += wcb_abs(input->left[4]) + wcb_abs(input->right[4]);
    if (transient > ctx->transient_threshold * 2) ctx->tail_gain_q15 = 11500;
    else ctx->tail_gain_q15 += (32767 - ctx->tail_gain_q15) >> ctx->tail_recovery_shift;
    l = 0;
    r = 0;
    for (i = 0U; i < WSOUNDCOMBATBUS89_BUSES; ++i) {
        g = ctx->gain_q15[i];
        if (i == WSOUNDCOMBATBUS89_ROOM || i == WSOUNDCOMBATBUS89_AMBIENCE || i == WSOUNDCOMBATBUS89_CASING) g = (g * ctx->tail_gain_q15) >> 15;
        l += ((wsound89_i32)input->left[i] * g) >> 15;
        r += ((wsound89_i32)input->right[i] * g) >> 15;
    }
    peak = wcb_abs(l);
    if (wcb_abs(r) > peak) peak = wcb_abs(r);
    if (peak > ctx->limiter_threshold) {
        target = ((wsound89_i32)ctx->limiter_threshold * 32767) / peak;
        if (target < ctx->limiter_gain_q15) ctx->limiter_gain_q15 = target;
    } else {
        ctx->limiter_gain_q15 += (32767 - ctx->limiter_gain_q15) >> ctx->limiter_release_shift;
    }
    l = (l * ctx->limiter_gain_q15) >> 15;
    r = (r * ctx->limiter_gain_q15) >> 15;
    *out_left = wcb_sat16(l);
    *out_right = wcb_sat16(r);
}
