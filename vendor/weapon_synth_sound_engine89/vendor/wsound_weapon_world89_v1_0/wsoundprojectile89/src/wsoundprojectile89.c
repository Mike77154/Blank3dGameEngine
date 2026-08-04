#include "wsoundprojectile89.h"

static wsound89_i16 wp_sat16(wsound89_i32 v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (wsound89_i16)v;
}

static wsound89_i16 wp_noise(wsoundprojectile89_context *ctx)
{
    ctx->state = ctx->state * 1103515245U + 12345U;
    return (wsound89_i16)((ctx->state >> 16) & 65535U);
}

wsound89_result wsoundprojectile89_init(wsoundprojectile89_context *ctx, wsound89_u32 sample_rate, wsound89_u32 seed)
{
    if (ctx == 0 || sample_rate < 8000U) return WSOUND89_EINVAL;
    ctx->sample_rate = sample_rate;
    ctx->frame = 0U;
    ctx->total_frames = 0U;
    ctx->state = seed == 0U ? 0x5750524AU : seed;
    ctx->low = 0;
    ctx->band = 0;
    ctx->amplitude_q15 = 0;
    ctx->mode = 0U;
    ctx->active = 0U;
    return WSOUND89_OK;
}

wsound89_result wsoundprojectile89_trigger(wsoundprojectile89_context *ctx, wsoundprojectile89_mode mode,
                                           wsound89_u16 amplitude_q15, wsound89_u16 proximity_q15)
{
    wsound89_u32 base_ms;
    if (ctx == 0) return WSOUND89_EINVAL;
    if ((wsound89_u32)mode > (wsound89_u32)WSOUNDPROJECTILE89_TRACER_FLYBY) return WSOUND89_EINVAL;
    if (amplitude_q15 > 32767U) amplitude_q15 = 32767U;
    if (proximity_q15 > 32767U) proximity_q15 = 32767U;
    if (mode == WSOUNDPROJECTILE89_SUPERSONIC_NWAVE) base_ms = 13U;
    else if (mode == WSOUNDPROJECTILE89_NEAR_MISS_SNAP) base_ms = 7U;
    else if (mode == WSOUNDPROJECTILE89_PELLET_SWARM) base_ms = 42U;
    else if (mode == WSOUNDPROJECTILE89_TRACER_FLYBY) base_ms = 120U;
    else base_ms = 90U;
    base_ms += ((32767U - proximity_q15) * base_ms) / 65535U;
    ctx->total_frames = (base_ms * ctx->sample_rate) / 1000U;
    if (ctx->total_frames < 4U) ctx->total_frames = 4U;
    ctx->frame = 0U;
    ctx->amplitude_q15 = (wsound89_i16)amplitude_q15;
    ctx->mode = (wsound89_u8)mode;
    ctx->low = 0;
    ctx->band = 0;
    ctx->active = 1U;
    return WSOUND89_OK;
}

wsound89_i16 wsoundprojectile89_process_sample(wsoundprojectile89_context *ctx)
{
    wsound89_i32 out;
    wsound89_i32 x;
    wsound89_i32 env;
    wsound89_u32 q;
    wsound89_i16 n;
    if (ctx == 0 || ctx->active == 0U) return 0;
    q = (ctx->frame * 32767U) / ctx->total_frames;
    out = 0;
    if (ctx->mode == WSOUNDPROJECTILE89_SUPERSONIC_NWAVE || ctx->mode == WSOUNDPROJECTILE89_NEAR_MISS_SNAP) {
        if (q < 16384U) x = 32767 - (wsound89_i32)(q * 4U);
        else x = -32767 + (wsound89_i32)((q - 16384U) * 4U);
        if (ctx->frame == 0U) x = 32767;
        if (ctx->frame + 1U == ctx->total_frames) x = -32767;
        out = (x * ctx->amplitude_q15) >> 15;
        n = wp_noise(ctx);
        out += ((wsound89_i32)n * (ctx->amplitude_q15 >> 4)) >> 15;
    } else {
        n = wp_noise(ctx);
        ctx->low += ((wsound89_i32)n - ctx->low) >> 4;
        ctx->band += (ctx->low - ctx->band) >> 2;
        x = (wsound89_i32)n - ctx->band;
        if (q < 8192U) env = (wsound89_i32)q * 4;
        else env = 32767 - (wsound89_i32)(((q - 8192U) * 32767U) / 24575U);
        if (env < 0) env = 0;
        out = (((x * env) >> 15) * ctx->amplitude_q15) >> 15;
        if (ctx->mode == WSOUNDPROJECTILE89_PELLET_SWARM && (ctx->frame % 73U) == 0U) out += ctx->amplitude_q15 / 2;
        if (ctx->mode == WSOUNDPROJECTILE89_TRACER_FLYBY) out += (ctx->band * env) >> 16;
    }
    ctx->frame++;
    if (ctx->frame >= ctx->total_frames) ctx->active = 0U;
    return wp_sat16(out);
}

int wsoundprojectile89_is_active(const wsoundprojectile89_context *ctx)
{
    return ctx != 0 && ctx->active != 0U;
}
