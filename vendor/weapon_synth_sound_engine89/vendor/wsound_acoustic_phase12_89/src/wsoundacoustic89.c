#include "wsoundacoustic89.h"

static wsound89_i16 wa89_sat16(wsound89_i32 v)
{
    if (v > 32767) return 32767;
    if (v < -32768) return -32768;
    return (wsound89_i16)v;
}

static wsound89_i32 wa89_abs32(wsound89_i32 v)
{
    return v < 0 ? -v : v;
}

static wsound89_u16 wa89_clamp_u15(wsound89_u32 v)
{
    return (wsound89_u16)(v > 32767U ? 32767U : v);
}

static wsound89_u32 wa89_ms(wsound89_u32 rate, wsound89_u32 ms)
{
    wsound89_u32 q;
    wsound89_u32 r;
    q = rate / 1000U;
    r = rate % 1000U;
    return q * ms + (r * ms) / 1000U;
}

static wsound89_u32 wa89_rng(wsound89_u32 *state)
{
    wsound89_u32 x;
    x = *state;
    if (x == 0U) x = 0x6D2B79F5U;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    *state = x;
    return x;
}

static wsound89_i16 wa89_pan_l(wsound89_i16 pan)
{
    wsound89_i32 v;
    v = 32767 - (wsound89_i32)pan;
    if (v > 65534) v = 65534;
    if (v < 0) v = 0;
    return (wsound89_i16)(v >> 1);
}

static wsound89_i16 wa89_pan_r(wsound89_i16 pan)
{
    wsound89_i32 v;
    v = 32767 + (wsound89_i32)pan;
    if (v > 65534) v = 65534;
    if (v < 0) v = 0;
    return (wsound89_i16)(v >> 1);
}

wsound89_u32 wsounda89_required_direct_samples(wsound89_u32 sample_rate,
                                                wsound89_u32 max_distance_cm,
                                                wsound89_u32 speed_cm_s)
{
    wsound89_u32 q;
    wsound89_u32 r;
    if (sample_rate < 8000U || speed_cm_s == 0U) return 0U;
    q = max_distance_cm / speed_cm_s;
    r = max_distance_cm % speed_cm_s;
    return q * sample_rate + (r * sample_rate) / speed_cm_s + 4U;
}

wsound89_u32 wsounda89_required_world_samples(wsound89_u32 sample_rate)
{
    if (sample_rate < 8000U) return 0U;
    return wa89_ms(sample_rate, 80U) + 16U;
}

void wsounda89_material_defaults(wsounda89_material material,
                                 wsounda89_material_params *params)
{
    static const wsound89_u16 absorb[WSOUNDA89_MATERIAL_COUNT][WSOUNDA89_BANDS] = {
        { 100, 160, 260 },
        { 600, 1200, 2200 },
        { 900, 1900, 3900 },
        { 2500, 5200, 8700 },
        { 3200, 6700, 11200 },
        { 1800, 3200, 5200 },
        { 300, 700, 1200 },
        { 6500, 12000, 18500 },
        { 7800, 15400, 24000 },
        { 900, 1800, 3400 },
        { 10500, 21000, 28500 }
    };
    static const wsound89_u16 transmit[WSOUNDA89_MATERIAL_COUNT][WSOUNDA89_BANDS] = {
        { 32767, 32767, 32767 },
        { 5100, 2300, 900 },
        { 6200, 3000, 1100 },
        { 10500, 6900, 3300 },
        { 9200, 5600, 2600 },
        { 12800, 7200, 3800 },
        { 4200, 1600, 500 },
        { 9000, 4800, 1700 },
        { 10800, 5700, 2200 },
        { 1600, 700, 300 },
        { 12100, 7100, 3200 }
    };
    static const wsound89_u16 scatter[WSOUNDA89_MATERIAL_COUNT] = {
        0, 7600, 9400, 13700, 10500, 2600, 1500, 20500, 27000, 700, 24500
    };
    wsound89_u16 i;
    if (params == 0) return;
    if ((wsound89_u32)material >= WSOUNDA89_MATERIAL_COUNT) material = WSOUNDA89_MATERIAL_AIR;
    for (i = 0U; i < WSOUNDA89_BANDS; ++i) {
        params->absorb_q15[i] = absorb[(wsound89_u16)material][i];
        params->transmit_q15[i] = transmit[(wsound89_u16)material][i];
    }
    params->scatter_q15 = scatter[(wsound89_u16)material];
}

void wsounda89_path_defaults(wsounda89_path_params *params)
{
    if (params == 0) return;
    params->distance_cm = 0U;
    params->speed_cm_s = 34300U;
    params->source_dot_q15 = 32767;
    params->occlusion_q15 = 32767U;
    params->source = WSOUNDA89_SOURCE_REPORT;
    params->air_absorb_q15[0] = 120U;
    params->air_absorb_q15[1] = 480U;
    params->air_absorb_q15[2] = 1600U;
}

void wsounda89_portal_defaults(wsounda89_portal_params *params)
{
    if (params == 0) return;
    params->delay_samples = 0U;
    params->opening_q15 = 32767U;
    params->transmit_q15[0] = 32767U;
    params->transmit_q15[1] = 32767U;
    params->transmit_q15[2] = 32767U;
    params->pan_q15 = 0;
}

static void wa89_profile_apply(wsounda89_context *ctx)
{
    if (ctx->profile == WSOUNDA89_REALISTIC) {
        ctx->translation_gain_q15 = 3600;
        ctx->direct_wet_q15 = 32767;
        ctx->early_wet_q15 = 9800;
        ctx->late_wet_q15 = 7600;
        ctx->master_gain_q15 = 28600;
        ctx->limiter_threshold = 30600;
    } else if (ctx->profile == WSOUNDA89_CINEMATIC) {
        ctx->translation_gain_q15 = 14500;
        ctx->direct_wet_q15 = 32767;
        ctx->early_wet_q15 = 17500;
        ctx->late_wet_q15 = 16500;
        ctx->master_gain_q15 = 30500;
        ctx->limiter_threshold = 28600;
    } else {
        ctx->translation_gain_q15 = 9800;
        ctx->direct_wet_q15 = 32767;
        ctx->early_wet_q15 = 15000;
        ctx->late_wet_q15 = 13700;
        ctx->master_gain_q15 = 31000;
        ctx->limiter_threshold = 29600;
    }
}

void wsounda89_set_profile(wsounda89_context *ctx, wsounda89_profile profile)
{
    if (ctx == 0) return;
    if ((wsound89_u32)profile > (wsound89_u32)WSOUNDA89_CINEMATIC) profile = WSOUNDA89_HYBRID;
    ctx->profile = profile;
    wa89_profile_apply(ctx);
}

static wsound89_result wa89_configure_space(wsounda89_context *ctx,
                                             wsounda89_space space)
{
    static const wsound89_u16 early_ms[WSOUNDA89_SPACE_COUNT][WSOUNDA89_EARLY_TAPS] = {
        { 5, 9, 14, 21, 31, 43, 57, 73 },
        { 8, 17, 29, 43, 61, 83, 109, 139 },
        { 13, 27, 43, 67, 97, 131, 173, 223 },
        { 17, 37, 63, 91, 127, 163, 201, 247 },
        { 9, 23, 41, 67, 97, 139, 181, 239 },
        { 12, 31, 57, 89, 127, 169, 211, 251 },
        { 7, 19, 37, 61, 91, 127, 173, 229 },
        { 31, 67, 109, 151, 193, 227, 247, 259 }
    };
    static const wsound89_i16 early_gain[WSOUNDA89_SPACE_COUNT][WSOUNDA89_EARLY_TAPS] = {
        { 11200, 9800, 8600, 7400, 6200, 5200, 4300, 3400 },
        { 10400, 9500, 8500, 7600, 6700, 5900, 5100, 4300 },
        { 9600, 8900, 8100, 7400, 6700, 6000, 5300, 4600 },
        { 9800, 9200, 8500, 7800, 7100, 6400, 5700, 5000 },
        { 6600, 5200, 4100, 3200, 2500, 1900, 1400, 1000 },
        { 9000, 8200, 7400, 6600, 5800, 5000, 4200, 3500 },
        { 7200, 6200, 5300, 4500, 3700, 3000, 2400, 1900 },
        { 8200, 7500, 6800, 6100, 5400, 4700, 4000, 3300 }
    };
    static const wsound89_i16 pans[WSOUNDA89_EARLY_TAPS] = {
        -26000, 22000, -17000, 13000, -9000, 6000, -3000, 18000
    };
    static const wsound89_u16 late_ms[WSOUNDA89_SPACE_COUNT][WSOUNDA89_LATE_LINES] = {
        { 37, 43, 53, 61 },
        { 59, 71, 83, 97 },
        { 89, 107, 131, 157 },
        { 101, 127, 151, 181 },
        { 47, 67, 89, 113 },
        { 73, 97, 127, 163 },
        { 61, 83, 109, 139 },
        { 113, 151, 193, 239 }
    };
    static const wsound89_i16 feedback[WSOUNDA89_SPACE_COUNT][WSOUNDA89_LATE_LINES] = {
        { 17500, 18100, 18700, 19300 },
        { 20500, 21100, 21700, 22300 },
        { 23100, 23700, 24300, 24900 },
        { 23900, 24500, 25100, 25700 },
        { 6500, 7600, 8700, 9800 },
        { 19100, 19900, 20700, 21500 },
        { 14200, 15100, 16000, 16900 },
        { 20500, 21400, 22300, 23200 }
    };
    static const wsound89_i16 damping[WSOUNDA89_SPACE_COUNT][WSOUNDA89_LATE_LINES] = {
        { 13200, 12600, 12000, 11400 },
        { 11800, 11200, 10600, 10000 },
        { 9700, 9200, 8700, 8200 },
        { 8800, 8400, 8000, 7600 },
        { 17600, 16800, 16000, 15200 },
        { 10400, 9800, 9200, 8600 },
        { 15400, 14600, 13800, 13000 },
        { 11200, 10400, 9600, 8800 }
    };
    static const wsound89_i16 late_pan[WSOUNDA89_LATE_LINES] = { -20000, 17000, -9000, 12000 };
    wsound89_u16 i;
    wsound89_u16 b;
    wsound89_u32 offset;
    wsound89_u32 len;
    wsound89_u32 required;
    wsound89_u32 gain;
    wsound89_u32 absorb;
    if (ctx == 0 || (wsound89_u32)space >= WSOUNDA89_SPACE_COUNT) return WSOUND89_EINVAL;
    required = wsounda89_required_world_samples(ctx->sample_rate);
    if (ctx->world_capacity < required) return WSOUND89_ECAPACITY;
    ctx->space = space;
    for (i = 0U; i < WSOUNDA89_EARLY_TAPS; ++i) {
        ctx->early_delay[i] = wa89_ms(ctx->sample_rate, early_ms[(wsound89_u16)space][i]);
        if (ctx->early_delay[i] >= ctx->world_capacity) ctx->early_delay[i] = ctx->world_capacity - 1U;
        ctx->early_pan_q15[i] = pans[i];
        for (b = 0U; b < WSOUNDA89_BANDS; ++b) {
            absorb = ctx->material_params.absorb_q15[b];
            gain = (wsound89_u32)early_gain[(wsound89_u16)space][i];
            gain = (gain * (32767U - absorb)) >> 15;
            if (b == 2U) gain = (gain * 27000U) >> 15;
            ctx->early_gain_q15[i][b] = (wsound89_i16)gain;
        }
    }
    offset = 0U;
    for (i = 0U; i < WSOUNDA89_LATE_LINES; ++i) {
        len = wa89_ms(ctx->sample_rate, late_ms[(wsound89_u16)space][i]);
        if (len < 17U) len = 17U;
        if (len >= ctx->world_capacity) len = ctx->world_capacity - 1U;
        ctx->late_offset[i] = offset;
        ctx->late_length[i] = len;
        ctx->late_pos[i] = 0U;
        ctx->late_damp[i] = 0;
        gain = 32767U - ((ctx->material_params.absorb_q15[1] +
                           ctx->material_params.absorb_q15[2]) >> 2);
        ctx->late_feedback_q15[i] = (wsound89_i16)(((wsound89_i32)feedback[(wsound89_u16)space][i] * gain) >> 15);
        gain = 32767U - (ctx->material_params.absorb_q15[2] >> 1);
        ctx->late_damping_q15[i] = (wsound89_i16)(((wsound89_i32)damping[(wsound89_u16)space][i] * gain) >> 15);
        ctx->late_pan_q15[i] = late_pan[i];
    }
    if (space == WSOUNDA89_SPACE_FIELD || space == WSOUNDA89_SPACE_FOREST) {
        ctx->ground_delay = wa89_ms(ctx->sample_rate, space == WSOUNDA89_SPACE_FIELD ? 11U : 17U);
        ctx->ground_gain_q15 = space == WSOUNDA89_SPACE_FIELD ? -7600 : -4200;
        if (ctx->ground_delay >= ctx->world_capacity) ctx->ground_delay = ctx->world_capacity - 1U;
        ctx->ground_high_q15 = space == WSOUNDA89_SPACE_FIELD ? 19000 : 10500;
    } else if (space == WSOUNDA89_SPACE_URBAN) {
        ctx->ground_delay = wa89_ms(ctx->sample_rate, 8U);
        ctx->ground_gain_q15 = -5200;
        if (ctx->ground_delay >= ctx->world_capacity) ctx->ground_delay = ctx->world_capacity - 1U;
        ctx->ground_high_q15 = 22500;
    } else {
        ctx->ground_delay = 0U;
        ctx->ground_gain_q15 = 0;
        ctx->ground_high_q15 = 32767;
    }
    return WSOUND89_OK;
}

wsound89_result wsounda89_set_space(wsounda89_context *ctx,
                                    wsounda89_space space)
{
    return wa89_configure_space(ctx, space);
}

wsound89_result wsounda89_set_material(wsounda89_context *ctx,
                                       wsounda89_material material,
                                       wsound89_u16 thickness_q15)
{
    wsound89_u16 i;
    wsound89_u32 transmit;
    if (ctx == 0 || (wsound89_u32)material >= WSOUNDA89_MATERIAL_COUNT) return WSOUND89_EINVAL;
    if (thickness_q15 > 32767U) thickness_q15 = 32767U;
    ctx->material = material;
    wsounda89_material_defaults(material, &ctx->material_params);
    for (i = 0U; i < WSOUNDA89_BANDS; ++i) {
        transmit = ctx->material_params.transmit_q15[i];
        transmit = 32767U - (((32767U - transmit) * thickness_q15) >> 15);
        ctx->material_params.transmit_q15[i] = wa89_clamp_u15(transmit);
    }
    if (wa89_configure_space(ctx, ctx->space) != WSOUND89_OK) return WSOUND89_ECAPACITY;
    return wsounda89_set_path(ctx, &ctx->path);
}

static void wa89_directivity(wsounda89_source source, wsound89_i16 dot,
                             wsound89_i16 out_q15[WSOUNDA89_BANDS])
{
    static const wsound89_u16 omni[WSOUNDA89_BANDS] = { 27000, 27000, 27000 };
    static const wsound89_u16 strength[8][WSOUNDA89_BANDS] = {
        { 7000, 12000, 19000 },
        { 3000, 5000, 7000 },
        { 8000, 14000, 21000 },
        { 1500, 2500, 3500 },
        { 6000, 13000, 23000 },
        { 1000, 2000, 3000 },
        { 2500, 4500, 6500 },
        { 1000, 1800, 2600 }
    };
    wsound89_i32 normalized;
    wsound89_i32 value;
    wsound89_u16 i;
    if ((wsound89_u32)source > (wsound89_u32)WSOUNDA89_SOURCE_TAIL) source = WSOUNDA89_SOURCE_REPORT;
    normalized = ((wsound89_i32)dot + 32768) >> 1;
    for (i = 0U; i < WSOUNDA89_BANDS; ++i) {
        value = omni[i] - (wsound89_i32)strength[(wsound89_u16)source][i];
        value += (normalized * strength[(wsound89_u16)source][i]) >> 15;
        if (value < 3500) value = 3500;
        if (value > 32767) value = 32767;
        out_q15[i] = (wsound89_i16)value;
    }
}

wsound89_result wsounda89_set_path(wsounda89_context *ctx,
                                   const wsounda89_path_params *params)
{
    wsound89_u32 required;
    wsound89_u32 near_cm;
    wsound89_u32 gain;
    wsound89_u32 distance_m;
    wsound89_u32 loss;
    wsound89_u32 path;
    wsound89_u16 i;
    if (ctx == 0 || params == 0 || params->speed_cm_s == 0U) return WSOUND89_EINVAL;
    required = wsounda89_required_direct_samples(ctx->sample_rate, params->distance_cm,
                                                  params->speed_cm_s);
    if (required == 0U || required - 4U + ctx->portal.delay_samples >= ctx->direct_capacity) return WSOUND89_ECAPACITY;
    ctx->path = *params;
    if (ctx->path.occlusion_q15 > 32767U) ctx->path.occlusion_q15 = 32767U;
    ctx->direct_delay_samples = required - 4U + ctx->portal.delay_samples;
    near_cm = 120U;
    gain = (near_cm * 32767U) / (near_cm + params->distance_cm);
    ctx->distance_gain_q15 = (wsound89_i16)gain;
    wa89_directivity(params->source, params->source_dot_q15, ctx->directivity_q15);
    distance_m = params->distance_cm / 100U;
    for (i = 0U; i < WSOUNDA89_BANDS; ++i) {
        loss = ((wsound89_u32)params->air_absorb_q15[i] * distance_m) / 100U;
        if (loss > 30000U) loss = 30000U;
        path = 32767U - loss;
        path = (path * ctx->directivity_q15[i]) >> 15;
        path = (path * params->occlusion_q15) >> 15;
        loss = ctx->material_params.transmit_q15[i] +
               (((32767U - ctx->material_params.transmit_q15[i]) * params->occlusion_q15) >> 15);
        path = (path * loss) >> 15;
        path = (path * ctx->portal.transmit_q15[i]) >> 15;
        path = (path * ctx->portal.opening_q15) >> 15;
        ctx->path_gain_q15[i] = (wsound89_i16)path;
    }
    return WSOUND89_OK;
}

wsound89_result wsounda89_set_portal(wsounda89_context *ctx,
                                     const wsounda89_portal_params *params)
{
    wsound89_u16 i;
    if (ctx == 0 || params == 0) return WSOUND89_EINVAL;
    if (params->delay_samples >= ctx->direct_capacity) return WSOUND89_ECAPACITY;
    ctx->portal = *params;
    if (ctx->portal.opening_q15 > 32767U) ctx->portal.opening_q15 = 32767U;
    for (i = 0U; i < WSOUNDA89_BANDS; ++i) {
        if (ctx->portal.transmit_q15[i] > 32767U) ctx->portal.transmit_q15[i] = 32767U;
    }
    return wsounda89_set_path(ctx, &ctx->path);
}

void wsounda89_trigger_pressure(wsounda89_context *ctx,
                                wsounda89_pressure_kind kind,
                                wsound89_u16 energy_q15,
                                wsound89_i16 pan_q15,
                                wsound89_u32 seed)
{
    wsound89_u32 pos_ms;
    wsound89_u32 neg_ms;
    wsound89_i32 pos;
    wsound89_i32 neg;
    if (ctx == 0) return;
    if (energy_q15 > 32767U) energy_q15 = 32767U;
    pos_ms = 1U;
    neg_ms = 4U;
    pos = 15000;
    neg = 4300;
    if (kind == WSOUNDA89_PRESSURE_IMPACT) {
        pos_ms = 2U; neg_ms = 6U; pos = 12500; neg = 3800;
    } else if (kind == WSOUNDA89_PRESSURE_GRENADE) {
        pos_ms = 3U; neg_ms = 12U; pos = 18800; neg = 6200;
    } else if (kind == WSOUNDA89_PRESSURE_ROCKET) {
        pos_ms = 4U; neg_ms = 18U; pos = 22000; neg = 7600;
    }
    if (ctx->profile == WSOUNDA89_REALISTIC) pos = (pos * 24500) >> 15;
    else if (ctx->profile == WSOUNDA89_CINEMATIC) pos = (pos * 32767) >> 15;
    else pos = (pos * 29200) >> 15;
    ctx->pressure_positive_frames = wa89_ms(ctx->sample_rate, pos_ms);
    ctx->pressure_negative_frames = wa89_ms(ctx->sample_rate, neg_ms);
    if (ctx->pressure_positive_frames == 0U) ctx->pressure_positive_frames = 1U;
    if (ctx->pressure_negative_frames == 0U) ctx->pressure_negative_frames = 1U;
    ctx->pressure_positive_q15 = (wsound89_i16)((pos * energy_q15) >> 15);
    ctx->pressure_negative_q15 = (wsound89_i16)((neg * energy_q15) >> 15);
    ctx->pressure_pan_q15 = pan_q15;
    ctx->pressure_energy_q15 = energy_q15;
    ctx->pressure_frame = 0U;
    ctx->pressure_seed = seed == 0U ? 1U : seed;
    ctx->pressure_active = 1U;
}

static wsound89_i16 wa89_pressure_sample(wsounda89_context *ctx)
{
    wsound89_i32 value;
    wsound89_i32 remain;
    wsound89_i32 total;
    wsound89_i32 noise;
    wsound89_u32 f;
    if (!ctx->pressure_active) return 0;
    f = ctx->pressure_frame;
    if (f < ctx->pressure_positive_frames) {
        remain = (wsound89_i32)(ctx->pressure_positive_frames - f);
        total = (wsound89_i32)ctx->pressure_positive_frames;
        value = ((wsound89_i32)ctx->pressure_positive_q15 * remain) / total;
    } else {
        f -= ctx->pressure_positive_frames;
        if (f >= ctx->pressure_negative_frames) {
            ctx->pressure_active = 0U;
            return 0;
        }
        remain = (wsound89_i32)(ctx->pressure_negative_frames - f);
        total = (wsound89_i32)ctx->pressure_negative_frames;
        value = -(((wsound89_i32)ctx->pressure_negative_q15 * remain) / total);
    }
    noise = (wsound89_i32)((wa89_rng(&ctx->pressure_seed) >> 17) & 0x7FFFU) - 16384;
    noise = (noise * (wsound89_i32)ctx->pressure_energy_q15) >> 17;
    value += noise;
    ctx->pressure_frame++;
    return wa89_sat16(value);
}

static void wa89_split(wsounda89_context *ctx, wsound89_i32 sample,
                       wsound89_i32 *low, wsound89_i32 *mid, wsound89_i32 *high)
{
    ctx->split_low += (sample - ctx->split_low) >> 5;
    ctx->split_high += (sample - ctx->split_high) >> 2;
    *low = ctx->split_low;
    *high = sample - ctx->split_high;
    *mid = sample - *low - *high;
}

static void wa89_split_tap(wsounda89_context *ctx, wsound89_u16 index,
                           wsound89_i32 sample, wsound89_i32 *low,
                           wsound89_i32 *mid, wsound89_i32 *high)
{
    ctx->early_low_state[index] += (sample - ctx->early_low_state[index]) >> 5;
    ctx->early_high_state[index] += (sample - ctx->early_high_state[index]) >> 2;
    *low = ctx->early_low_state[index];
    *high = sample - ctx->early_high_state[index];
    *mid = sample - *low - *high;
}

static void wa89_master_split(wsound89_i32 sample, wsound89_i32 *low_state,
                              wsound89_i32 *high_state, wsound89_i32 *low,
                              wsound89_i32 *mid, wsound89_i32 *high)
{
    *low_state += (sample - *low_state) >> 5;
    *high_state += (sample - *high_state) >> 2;
    *low = *low_state;
    *high = sample - *high_state;
    *mid = sample - *low - *high;
}

static wsound89_i16 wa89_read_delay(const wsounda89_context *ctx,
                                    wsound89_u32 delay)
{
    wsound89_u32 pos;
    if (delay >= ctx->world_capacity) return 0;
    if (ctx->world_write >= delay) pos = ctx->world_write - delay;
    else pos = ctx->world_capacity + ctx->world_write - delay;
    if (pos >= ctx->world_capacity) pos %= ctx->world_capacity;
    return ctx->world_delay[pos];
}

static void wa89_process_late(wsounda89_context *ctx,
                              wsound89_i32 *left, wsound89_i32 *right,
                              wsound89_i32 *feedback_write)
{
    wsound89_u16 i;
    wsound89_i32 read;
    wsound89_i32 damp;
    wsound89_i32 gl;
    wsound89_i32 gr;
    wsound89_i32 fb;
    fb = 0;
    for (i = 0U; i < WSOUNDA89_LATE_LINES; ++i) {
        read = wa89_read_delay(ctx, ctx->late_length[i]);
        damp = ctx->late_damp[i];
        damp += ((read - damp) * ctx->late_damping_q15[i]) >> 15;
        ctx->late_damp[i] = damp;
        gl = wa89_pan_l(ctx->late_pan_q15[i]);
        gr = wa89_pan_r(ctx->late_pan_q15[i]);
        *left += (damp * gl) >> 15;
        *right += (damp * gr) >> 15;
        fb += (damp * ctx->late_feedback_q15[i]) >> 15;
    }
    *feedback_write = fb / (wsound89_i32)WSOUNDA89_LATE_LINES;
}

wsound89_result wsounda89_init(wsounda89_context *ctx,
                               wsound89_u32 sample_rate,
                               wsound89_i16 *direct_memory,
                               wsound89_u32 direct_samples,
                               wsound89_i16 *world_memory,
                               wsound89_u32 world_samples)
{
    wsound89_u32 i;
    wsounda89_path_params path;
    wsounda89_portal_params portal;
    if (ctx == 0 || direct_memory == 0 || world_memory == 0 || sample_rate < 8000U) return WSOUND89_EINVAL;
    if (direct_samples < 4U || world_samples < wsounda89_required_world_samples(sample_rate)) return WSOUND89_ECAPACITY;
    ctx->direct_delay = direct_memory;
    ctx->direct_capacity = direct_samples;
    ctx->world_delay = world_memory;
    ctx->world_capacity = world_samples;
    ctx->sample_rate = sample_rate;
    ctx->direct_write = 0U;
    ctx->world_write = 0U;
    ctx->direct_delay_samples = 0U;
    ctx->split_low = 0;
    ctx->split_high = 0;
    ctx->direct_low_state = 0;
    ctx->direct_mid_state = 0;
    ctx->direct_high_state = 0;
    for (i = 0U; i < WSOUNDA89_EARLY_TAPS; ++i) {
        ctx->early_low_state[i] = 0;
        ctx->early_high_state[i] = 0;
    }
    ctx->translation_low = 0;
    ctx->translation_dc = 0;
    ctx->duck_gain_q15 = 32767;
    ctx->limiter_gain_q15 = 32767;
    ctx->transient_env = 0;
    ctx->master_low_l = 0;
    ctx->master_high_l = 0;
    ctx->master_low_r = 0;
    ctx->master_high_r = 0;
    for (i = 0U; i < WSOUNDA89_BANDS; ++i) ctx->band_dynamics_q15[i] = 32767;
    ctx->pressure_active = 0U;
    ctx->enabled = 1U;
    for (i = 0U; i < direct_samples; ++i) direct_memory[i] = 0;
    for (i = 0U; i < world_samples; ++i) world_memory[i] = 0;
    wsounda89_material_defaults(WSOUNDA89_MATERIAL_CONCRETE, &ctx->material_params);
    ctx->material = WSOUNDA89_MATERIAL_CONCRETE;
    ctx->space = WSOUNDA89_SPACE_WAREHOUSE;
    wsounda89_set_profile(ctx, WSOUNDA89_HYBRID);
    wsounda89_portal_defaults(&portal);
    ctx->portal = portal;
    wsounda89_path_defaults(&path);
    ctx->path = path;
    if (wa89_configure_space(ctx, WSOUNDA89_SPACE_WAREHOUSE) != WSOUND89_OK) return WSOUND89_ECAPACITY;
    return wsounda89_set_path(ctx, &path);
}

void wsounda89_reset(wsounda89_context *ctx)
{
    wsound89_u32 i;
    if (ctx == 0) return;
    for (i = 0U; i < ctx->direct_capacity; ++i) ctx->direct_delay[i] = 0;
    for (i = 0U; i < ctx->world_capacity; ++i) ctx->world_delay[i] = 0;
    ctx->direct_write = 0U;
    ctx->world_write = 0U;
    ctx->split_low = 0;
    ctx->split_high = 0;
    for (i = 0U; i < WSOUNDA89_EARLY_TAPS; ++i) {
        ctx->early_low_state[i] = 0;
        ctx->early_high_state[i] = 0;
    }
    ctx->translation_low = 0;
    ctx->translation_dc = 0;
    ctx->duck_gain_q15 = 32767;
    ctx->limiter_gain_q15 = 32767;
    ctx->transient_env = 0;
    ctx->master_low_l = 0;
    ctx->master_high_l = 0;
    ctx->master_low_r = 0;
    ctx->master_high_r = 0;
    for (i = 0U; i < WSOUNDA89_BANDS; ++i) ctx->band_dynamics_q15[i] = 32767;
    ctx->pressure_active = 0U;
    for (i = 0U; i < WSOUNDA89_LATE_LINES; ++i) {
        ctx->late_pos[i] = 0U;
        ctx->late_damp[i] = 0;
    }
}

void wsounda89_set_enabled(wsounda89_context *ctx, int enabled)
{
    if (ctx != 0) ctx->enabled = enabled ? 1U : 0U;
}

void wsounda89_process_stereo(wsounda89_context *ctx,
                              wsound89_i16 in_left,
                              wsound89_i16 in_right,
                              wsound89_i16 *out_left,
                              wsound89_i16 *out_right)
{
    wsound89_i32 mono;
    wsound89_i32 side_l;
    wsound89_i32 side_r;
    wsound89_i32 delayed;
    wsound89_u32 read_pos;
    wsound89_i32 low;
    wsound89_i32 mid;
    wsound89_i32 high;
    wsound89_i32 direct;
    wsound89_i32 pressure;
    wsound89_i32 harmonic;
    wsound89_i32 early_l;
    wsound89_i32 early_r;
    wsound89_i32 late_l;
    wsound89_i32 late_r;
    wsound89_i32 ground;
    wsound89_i32 feedback_write;
    wsound89_i32 tap;
    wsound89_i32 gl;
    wsound89_i32 gr;
    wsound89_i32 output_l;
    wsound89_i32 output_r;
    wsound89_i32 peak;
    wsound89_i32 target;
    wsound89_i32 transient;
    wsound89_i32 portal_pan;
    wsound89_i32 pressure_path;
    wsound89_i32 ml;
    wsound89_i32 mm;
    wsound89_i32 mh;
    wsound89_i32 rl;
    wsound89_i32 rm;
    wsound89_i32 rh;
    wsound89_i32 band_peak;
    wsound89_i32 band_target;
    wsound89_i32 threshold;
    wsound89_u16 i;
    if (out_left == 0 || out_right == 0) return;
    if (ctx == 0 || !ctx->enabled) {
        *out_left = in_left;
        *out_right = in_right;
        return;
    }
    mono = ((wsound89_i32)in_left + (wsound89_i32)in_right) / 2;
    side_l = (wsound89_i32)in_left - mono;
    side_r = (wsound89_i32)in_right - mono;
    ctx->direct_delay[ctx->direct_write] = wa89_sat16(mono);
    if (ctx->direct_write >= ctx->direct_delay_samples) read_pos = ctx->direct_write - ctx->direct_delay_samples;
    else read_pos = ctx->direct_capacity + ctx->direct_write - ctx->direct_delay_samples;
    if (read_pos >= ctx->direct_capacity) read_pos %= ctx->direct_capacity;
    delayed = ctx->direct_delay[read_pos];
    ctx->direct_write++;
    if (ctx->direct_write >= ctx->direct_capacity) ctx->direct_write = 0U;

    wa89_split(ctx, delayed, &low, &mid, &high);
    low = (low * ctx->path_gain_q15[0]) >> 15;
    mid = (mid * ctx->path_gain_q15[1]) >> 15;
    high = (high * ctx->path_gain_q15[2]) >> 15;
    direct = low + mid + high;
    direct = (direct * ctx->distance_gain_q15) >> 15;
    side_l = (side_l * ctx->distance_gain_q15) >> 15;
    side_r = (side_r * ctx->distance_gain_q15) >> 15;
    side_l = (side_l * ctx->path_gain_q15[1]) >> 15;
    side_r = (side_r * ctx->path_gain_q15[1]) >> 15;

    ctx->translation_low += (low - ctx->translation_low) >> 4;
    harmonic = wa89_abs32(ctx->translation_low);
    ctx->translation_dc += (harmonic - ctx->translation_dc) >> 7;
    harmonic -= ctx->translation_dc;
    harmonic = (harmonic * ctx->translation_gain_q15) >> 15;

    pressure = wa89_pressure_sample(ctx);
    pressure_path = ((wsound89_i32)ctx->path_gain_q15[0] +
                     (wsound89_i32)ctx->path_gain_q15[1]) / 2;
    pressure = (pressure * ctx->distance_gain_q15) >> 15;
    pressure = (pressure * pressure_path) >> 15;
    transient = wa89_abs32(pressure) + (wa89_abs32(direct - ctx->transient_env) >> 1);
    ctx->transient_env += (direct - ctx->transient_env) >> 5;
    if (transient > 8500) ctx->duck_gain_q15 = ctx->profile == WSOUNDA89_CINEMATIC ? 8500 : 11200;
    else ctx->duck_gain_q15 += (32767 - ctx->duck_gain_q15) >> 10;

    early_l = 0;
    early_r = 0;
    for (i = 0U; i < WSOUNDA89_EARLY_TAPS; ++i) {
        tap = wa89_read_delay(ctx, ctx->early_delay[i]);
        wa89_split_tap(ctx, i, tap, &low, &mid, &high);
        tap = ((low * ctx->early_gain_q15[i][0]) >> 15) +
              ((mid * ctx->early_gain_q15[i][1]) >> 15) +
              ((high * ctx->early_gain_q15[i][2]) >> 15);
        gl = wa89_pan_l(ctx->early_pan_q15[i]);
        gr = wa89_pan_r(ctx->early_pan_q15[i]);
        early_l += (tap * gl) >> 15;
        early_r += (tap * gr) >> 15;
    }
    ground = 0;
    if (ctx->ground_delay != 0U) {
        ground = wa89_read_delay(ctx, ctx->ground_delay);
        ground = (ground * ctx->ground_gain_q15) >> 15;
        early_l += ground;
        early_r += ground;
    }
    late_l = 0;
    late_r = 0;
    feedback_write = 0;
    wa89_process_late(ctx, &late_l, &late_r, &feedback_write);
    ctx->world_delay[ctx->world_write] = wa89_sat16(direct + feedback_write);
    ctx->world_write++;
    if (ctx->world_write >= ctx->world_capacity) ctx->world_write = 0U;
    early_l = (early_l * ctx->early_wet_q15) >> 15;
    early_r = (early_r * ctx->early_wet_q15) >> 15;
    late_l = (late_l * ctx->late_wet_q15) >> 15;
    late_r = (late_r * ctx->late_wet_q15) >> 15;
    early_l = (early_l * ctx->duck_gain_q15) >> 15;
    early_r = (early_r * ctx->duck_gain_q15) >> 15;
    late_l = (late_l * ctx->duck_gain_q15) >> 15;
    late_r = (late_r * ctx->duck_gain_q15) >> 15;

    gl = wa89_pan_l(ctx->pressure_pan_q15);
    gr = wa89_pan_r(ctx->pressure_pan_q15);
    portal_pan = ((wsound89_i32)ctx->portal.pan_q15 *
                  (32767 - (wsound89_i32)ctx->portal.opening_q15)) >> 15;
    output_l = (((direct + harmonic) * ctx->direct_wet_q15) >> 15);
    output_r = output_l;
    output_l = (output_l * wa89_pan_l((wsound89_i16)portal_pan)) >> 14;
    output_r = (output_r * wa89_pan_r((wsound89_i16)portal_pan)) >> 14;
    output_l += (pressure * gl) >> 15;
    output_r += (pressure * gr) >> 15;
    output_l += early_l + late_l + side_l;
    output_r += early_r + late_r + side_r;
    output_l = (output_l * ctx->master_gain_q15) >> 15;
    output_r = (output_r * ctx->master_gain_q15) >> 15;

    wa89_master_split(output_l, &ctx->master_low_l, &ctx->master_high_l, &ml, &mm, &mh);
    wa89_master_split(output_r, &ctx->master_low_r, &ctx->master_high_r, &rl, &rm, &rh);
    for (i = 0U; i < WSOUNDA89_BANDS; ++i) {
        if (i == 0U) { band_peak = wa89_abs32(ml); if (wa89_abs32(rl) > band_peak) band_peak = wa89_abs32(rl); threshold = 22000; }
        else if (i == 1U) { band_peak = wa89_abs32(mm); if (wa89_abs32(rm) > band_peak) band_peak = wa89_abs32(rm); threshold = 25500; }
        else { band_peak = wa89_abs32(mh); if (wa89_abs32(rh) > band_peak) band_peak = wa89_abs32(rh); threshold = 23500; }
        if (band_peak > threshold && band_peak != 0) {
            band_target = (threshold * 32767) / band_peak;
            if (band_target < ctx->band_dynamics_q15[i]) ctx->band_dynamics_q15[i] = band_target;
        } else {
            ctx->band_dynamics_q15[i] += (32767 - ctx->band_dynamics_q15[i]) >> 9;
        }
    }
    ml = (ml * ctx->band_dynamics_q15[0]) >> 15;
    rl = (rl * ctx->band_dynamics_q15[0]) >> 15;
    mm = (mm * ctx->band_dynamics_q15[1]) >> 15;
    rm = (rm * ctx->band_dynamics_q15[1]) >> 15;
    mh = (mh * ctx->band_dynamics_q15[2]) >> 15;
    rh = (rh * ctx->band_dynamics_q15[2]) >> 15;
    output_l = ml + mm + mh;
    output_r = rl + rm + rh;

    peak = wa89_abs32(output_l);
    if (wa89_abs32(output_r) > peak) peak = wa89_abs32(output_r);
    if (peak > ctx->limiter_threshold && peak != 0) {
        target = ((wsound89_i32)ctx->limiter_threshold * 32767) / peak;
        if (target < ctx->limiter_gain_q15) ctx->limiter_gain_q15 = target;
    } else {
        ctx->limiter_gain_q15 += (32767 - ctx->limiter_gain_q15) >> 11;
    }
    output_l = (output_l * ctx->limiter_gain_q15) >> 15;
    output_r = (output_r * ctx->limiter_gain_q15) >> 15;
    *out_left = wa89_sat16(output_l);
    *out_right = wa89_sat16(output_r);
}
