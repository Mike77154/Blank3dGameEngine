#include "wsrb89_internal.h"

static const wsrb89_s16 wsrb89_sine_table[256] = {
    0, 804, 1608, 2410, 3212, 4011, 4808, 5602,
    6393, 7179, 7962, 8739, 9512, 10278, 11039, 11793,
    12539, 13279, 14010, 14732, 15446, 16151, 16846, 17530,
    18204, 18868, 19519, 20159, 20787, 21403, 22005, 22594,
    23170, 23731, 24279, 24811, 25329, 25832, 26319, 26790,
    27245, 27683, 28105, 28510, 28898, 29268, 29621, 29956,
    30273, 30571, 30852, 31113, 31356, 31580, 31785, 31971,
    32137, 32285, 32412, 32521, 32609, 32678, 32728, 32757,
    32767, 32757, 32728, 32678, 32609, 32521, 32412, 32285,
    32137, 31971, 31785, 31580, 31356, 31113, 30852, 30571,
    30273, 29956, 29621, 29268, 28898, 28510, 28105, 27683,
    27245, 26790, 26319, 25832, 25329, 24811, 24279, 23731,
    23170, 22594, 22005, 21403, 20787, 20159, 19519, 18868,
    18204, 17530, 16846, 16151, 15446, 14732, 14010, 13279,
    12539, 11793, 11039, 10278, 9512, 8739, 7962, 7179,
    6393, 5602, 4808, 4011, 3212, 2410, 1608, 804,
    0, -804, -1608, -2410, -3212, -4011, -4808, -5602,
    -6393, -7179, -7962, -8739, -9512, -10278, -11039, -11793,
    -12539, -13279, -14010, -14732, -15446, -16151, -16846, -17530,
    -18204, -18868, -19519, -20159, -20787, -21403, -22005, -22594,
    -23170, -23731, -24279, -24811, -25329, -25832, -26319, -26790,
    -27245, -27683, -28105, -28510, -28898, -29268, -29621, -29956,
    -30273, -30571, -30852, -31113, -31356, -31580, -31785, -31971,
    -32137, -32285, -32412, -32521, -32609, -32678, -32728, -32757,
    -32767, -32757, -32728, -32678, -32609, -32521, -32412, -32285,
    -32137, -31971, -31785, -31580, -31356, -31113, -30852, -30571,
    -30273, -29956, -29621, -29268, -28898, -28510, -28105, -27683,
    -27245, -26790, -26319, -25832, -25329, -24811, -24279, -23731,
    -23170, -22594, -22005, -21403, -20787, -20159, -19519, -18868,
    -18204, -17530, -16846, -16151, -15446, -14732, -14010, -13279,
    -12539, -11793, -11039, -10278, -9512, -8739, -7962, -7179,
    -6393, -5602, -4808, -4011, -3212, -2410, -1608, -804
};

wsrb89_s16 wsrb89_clamp16(wsrb89_s32 value)
{
    if (value > 32767) {
        return 32767;
    }
    if (value < -32768) {
        return -32768;
    }
    return (wsrb89_s16)value;
}

wsrb89_s16 wsrb89_soft_clip(wsrb89_s32 value)
{
    wsrb89_s32 sign;
    wsrb89_s32 mag;
    sign = 1;
    mag = value;
    if (mag < 0) {
        sign = -1;
        mag = -mag;
    }
    if (mag > 32767) {
        mag = 32767;
    }
    if (mag > 16384) {
        mag = 16384 + ((mag - 16384) >> 1);
    }
    if (sign < 0) {
        mag = -mag;
    }
    return wsrb89_clamp16(mag);
}

wsrb89_s16 wsrb89_sine_q15(wsrb89_u16 phase)
{
    return wsrb89_sine_table[(phase >> 8) & 255U];
}

wsrb89_s16 wsrb89_saw_q15(wsrb89_u16 phase)
{
    return (wsrb89_s16)((wsrb89_s32)phase - 32768);
}

wsrb89_s16 wsrb89_noise_next(wsrb89_u32 *state)
{
    wsrb89_u32 value;
    *state = (*state * 1664525U) + 1013904223U;
    value = (*state >> 16) & 65535U;
    if (value >= 32768U) {
        return (wsrb89_s16)((wsrb89_s32)value - 65536);
    }
    return (wsrb89_s16)value;
}

wsrb89_s16 wsrb89_svf_coeff(wsrb89_u32 sample_rate, wsrb89_u16 cutoff_hz)
{
    wsrb89_u32 cutoff;
    wsrb89_u32 coeff;
    cutoff = cutoff_hz;
    if (cutoff > 9000U) {
        cutoff = 9000U;
    }
    if (sample_rate < 8000U) {
        sample_rate = 8000U;
    }
    coeff = (cutoff * 205887U) / sample_rate;
    if (coeff > 30000U) {
        coeff = 30000U;
    }
    if (coeff < 16U) {
        coeff = 16U;
    }
    return (wsrb89_s16)coeff;
}

wsrb89_s16 wsrb89_svf_process(wsrb89_svf *svf, wsrb89_s16 input,
                              wsrb89_u16 mode)
{
    wsrb89_s32 low;
    wsrb89_s32 band;
    wsrb89_s32 high;
    wsrb89_s32 notch;
    low = svf->low + (((wsrb89_s32)svf->coeff_q15 * svf->band) >> 15);
    high = (wsrb89_s32)input - low
         - (((wsrb89_s32)svf->damp_q15 * svf->band) >> 15);
    if (high > 65535) {
        high = 65535;
    }
    if (high < -65536) {
        high = -65536;
    }
    band = svf->band + (((wsrb89_s32)svf->coeff_q15 * high) >> 15);
    if (low > 65535) {
        low = 65535;
    }
    if (low < -65536) {
        low = -65536;
    }
    if (band > 65535) {
        band = 65535;
    }
    if (band < -65536) {
        band = -65536;
    }
    svf->low = low;
    svf->band = band;
    notch = low + high;
    if (mode == WSRB89_SVF_BAND) {
        return wsrb89_clamp16(band);
    }
    if (mode == WSRB89_SVF_HIGH) {
        return wsrb89_clamp16(high);
    }
    if (mode == WSRB89_SVF_NOTCH) {
        return wsrb89_clamp16(notch);
    }
    return wsrb89_clamp16(low);
}

void wsrb89_env_start(wsrb89_envelope *env, wsrb89_s16 from_q15,
                      wsrb89_s16 to_q15, wsrb89_u32 samples)
{
    wsrb89_s32 from_fp;
    wsrb89_s32 to_fp;
    from_fp = ((wsrb89_s32)from_q15) << 16;
    to_fp = ((wsrb89_s32)to_q15) << 16;
    env->level_q16 = from_fp;
    env->target_q16 = to_fp;
    if (samples == 0U) {
        env->step_q16 = 0;
        env->remaining = 0U;
        env->level_q16 = to_fp;
    } else {
        env->step_q16 = (to_fp - from_fp) / (wsrb89_s32)samples;
        env->remaining = samples;
    }
}

wsrb89_s16 wsrb89_env_tick(wsrb89_envelope *env)
{
    if (env->remaining > 0U) {
        env->level_q16 += env->step_q16;
        env->remaining--;
        if (env->remaining == 0U) {
            env->level_q16 = env->target_q16;
        }
    }
    return wsrb89_clamp16(env->level_q16 >> 16);
}
