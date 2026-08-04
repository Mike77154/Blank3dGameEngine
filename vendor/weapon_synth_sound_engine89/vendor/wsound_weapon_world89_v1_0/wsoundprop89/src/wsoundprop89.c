#include "wsoundprop89.h"

static wsound89_i16 wprop_sat16(wsound89_i32 v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (wsound89_i16)v;
}

wsound89_u32 wsoundprop89_required_samples(wsound89_u32 sample_rate, wsound89_u32 max_distance_cm,
                                           wsound89_u32 speed_cm_s)
{
    wsound89_u32 q;
    wsound89_u32 r;
    if (sample_rate == 0U || speed_cm_s == 0U) return 0U;
    q = max_distance_cm / speed_cm_s;
    r = max_distance_cm % speed_cm_s;
    return q * sample_rate + (r * sample_rate) / speed_cm_s + 2U;
}

wsound89_result wsoundprop89_init(wsoundprop89_context *ctx, wsound89_u32 sample_rate,
                                  wsound89_i16 *delay_memory, wsound89_u32 delay_samples)
{
    wsound89_u32 i;
    if (ctx == 0 || delay_memory == 0 || delay_samples < 2U || sample_rate < 8000U) return WSOUND89_EINVAL;
    ctx->delay = delay_memory;
    ctx->capacity = delay_samples;
    ctx->write_pos = 0U;
    ctx->delay_samples = 0U;
    ctx->sample_rate = sample_rate;
    ctx->lowpass = 0;
    ctx->gain_q15 = 32767;
    ctx->directivity_q15 = 32767;
    ctx->occlusion_q15 = 32767;
    ctx->lowpass_shift = 0U;
    for (i = 0U; i < delay_samples; ++i) delay_memory[i] = 0;
    return WSOUND89_OK;
}

wsound89_result wsoundprop89_set_path(wsoundprop89_context *ctx, wsound89_u32 distance_cm,
                                      wsound89_u32 speed_cm_s, wsound89_i16 muzzle_dot_q15,
                                      wsound89_u16 occlusion_q15)
{
    wsound89_u32 required;
    wsound89_u32 near_cm;
    wsound89_i32 directivity;
    wsound89_i32 gain;
    if (ctx == 0 || speed_cm_s == 0U) return WSOUND89_EINVAL;
    required = wsoundprop89_required_samples(ctx->sample_rate, distance_cm, speed_cm_s);
    if (required == 0U || required - 2U >= ctx->capacity) return WSOUND89_ECAPACITY;
    ctx->delay_samples = required - 2U;
    near_cm = 140U;
    gain = (wsound89_i32)((near_cm * 32767U) / (near_cm + distance_cm));
    if (gain < 1000) gain = 1000;
    directivity = 19660 + (((wsound89_i32)muzzle_dot_q15 + 32768) * 13107) / 65535;
    if (directivity < 9000) directivity = 9000;
    if (directivity > 32767) directivity = 32767;
    if (occlusion_q15 > 32767U) occlusion_q15 = 32767U;
    ctx->gain_q15 = (wsound89_i16)gain;
    ctx->directivity_q15 = (wsound89_i16)directivity;
    ctx->occlusion_q15 = (wsound89_i16)occlusion_q15;
    if (distance_cm > 18000U || occlusion_q15 < 12000U) ctx->lowpass_shift = 5U;
    else if (distance_cm > 8000U || occlusion_q15 < 22000U) ctx->lowpass_shift = 4U;
    else if (distance_cm > 2500U) ctx->lowpass_shift = 3U;
    else if (distance_cm > 800U) ctx->lowpass_shift = 2U;
    else ctx->lowpass_shift = 0U;
    return WSOUND89_OK;
}

wsound89_i16 wsoundprop89_process_sample(wsoundprop89_context *ctx, wsound89_i16 input)
{
    wsound89_u32 read_pos;
    wsound89_i32 sample;
    if (ctx == 0 || ctx->delay == 0 || ctx->capacity == 0U) return 0;
    ctx->delay[ctx->write_pos] = input;
    if (ctx->write_pos >= ctx->delay_samples) read_pos = ctx->write_pos - ctx->delay_samples;
    else read_pos = ctx->capacity + ctx->write_pos - ctx->delay_samples;
    if (read_pos >= ctx->capacity) read_pos %= ctx->capacity;
    sample = ctx->delay[read_pos];
    ctx->write_pos++;
    if (ctx->write_pos >= ctx->capacity) ctx->write_pos = 0U;
    if (ctx->lowpass_shift != 0U) {
        ctx->lowpass += (sample - ctx->lowpass) >> ctx->lowpass_shift;
        sample = ctx->lowpass;
    }
    sample = (sample * ctx->gain_q15) >> 15;
    sample = (sample * ctx->directivity_q15) >> 15;
    sample = (sample * ctx->occlusion_q15) >> 15;
    return wprop_sat16(sample);
}

void wsoundprop89_reset(wsoundprop89_context *ctx)
{
    wsound89_u32 i;
    if (ctx == 0 || ctx->delay == 0) return;
    for (i = 0U; i < ctx->capacity; ++i) ctx->delay[i] = 0;
    ctx->write_pos = 0U;
    ctx->lowpass = 0;
}
