#include "gsynthreport89.h"

static gv89_s16 gssr89_sat16(gv89_s32 value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (gv89_s16)value;
}

static gv89_s32 gssr89_mul_q15(gv89_s16 value, gv89_s16 gain)
{
    return ((gv89_s32)value * (gv89_s32)gain) >> 15;
}

static void gssr89_clear_handle(gv89_handle *handle)
{
    handle->index = GV89_INVALID_INDEX;
    handle->generation = 0U;
}

static void gssr89_zero_voice(gssr89_voice *voice)
{
    gssr89_clear_handle(&voice->handle);
    voice->sample_rate = 0U;
    voice->seed = 0U;
    voice->serial = 0U;
    voice->in_use = 0U;
}

void gssr89_defaults(gssr89_params *params, gpaah89_preset_id report_preset)
{
    if (params == 0) return;
    params->report_preset = report_preset;
    params->report_gain_q15 = 21400;
    params->body_gain_q15 = 18400;
    params->gas_gain_q15 = 10400;
    params->crack_gain_q15 = 7600;
    params->thump_gain_q15 = 7600;
    params->tail_gain_q15 = 13200;
    params->output_gain_q15 = 28000;
    params->brightness_q15 = 19000U;
    params->pitch_q16 = 65536U;
    params->cycle_q16 = 65536U;
    params->pan_q15 = 0;
    params->distance_q15 = 1800U;
    params->occlusion_q15 = 32767U;
    params->focus_q15 = 32767U;
    params->gain_q15 = 28600;
    params->priority_bias = 170;
    params->instance_key = 0U;
    params->instance_limit = 16U;

    switch (report_preset) {
    case GPAAH89_PRESET_PISTOL:
        params->body_preset = GWB89_PRESET_PISTOL;
        params->gas_preset = GMG89_PRESET_PISTOL;
        params->crack_preset = GBC89_PRESET_MEDIUM;
        params->tail_preset = GLT89_PRESET_CORRIDOR;
        params->thump_preset = GCT89_PRESET_SUBTLE;
        params->thump_gain_q15 = 4200;
        break;
    case GPAAH89_PRESET_MAGNUM:
        params->body_preset = GWB89_PRESET_MAGNUM;
        params->gas_preset = GMG89_PRESET_MAGNUM;
        params->crack_preset = GBC89_PRESET_NEAR;
        params->tail_preset = GLT89_PRESET_WAREHOUSE;
        params->thump_preset = GCT89_PRESET_ACTION;
        params->body_gain_q15 = 18000;
        params->thump_gain_q15 = 7600;
        break;
    case GPAAH89_PRESET_SHOTGUN:
        params->body_preset = GWB89_PRESET_SHOTGUN;
        params->gas_preset = GMG89_PRESET_SHOTGUN;
        params->crack_preset = GBC89_PRESET_FAR;
        params->tail_preset = GLT89_PRESET_WAREHOUSE;
        params->thump_preset = GCT89_PRESET_SHOTGUN;
        params->body_gain_q15 = 18000;
        params->crack_gain_q15 = 3000;
        params->thump_gain_q15 = 9400;
        break;
    case GPAAH89_PRESET_METRALLA:
    case GPAAH89_PRESET_GATLING:
        params->body_preset = GWB89_PRESET_RIFLE;
        params->gas_preset = GMG89_PRESET_RIFLE;
        params->crack_preset = GBC89_PRESET_RIFLE_PASS;
        params->tail_preset = GLT89_PRESET_EXTERIOR;
        params->thump_preset = GCT89_PRESET_ACTION;
        params->instance_limit = 32U;
        break;
    case GPAAH89_PRESET_ROCKET_LAUNCHER:
        params->body_preset = GWB89_PRESET_LAUNCHER;
        params->gas_preset = GMG89_PRESET_LAUNCHER;
        params->crack_preset = GBC89_PRESET_FAR;
        params->tail_preset = GLT89_PRESET_EXTERIOR;
        params->thump_preset = GCT89_PRESET_LAUNCHER;
        params->body_gain_q15 = 17200;
        params->gas_gain_q15 = 12800;
        params->crack_gain_q15 = 2800;
        params->thump_gain_q15 = 11000;
        params->instance_limit = 6U;
        break;
    case GPAAH89_PRESET_SNIPER_RIFLE:
        params->body_preset = GWB89_PRESET_SNIPER;
        params->gas_preset = GMG89_PRESET_SNIPER;
        params->crack_preset = GBC89_PRESET_SNIPER_PASS;
        params->tail_preset = GLT89_PRESET_EXTERIOR;
        params->thump_preset = GCT89_PRESET_SNIPER;
        params->crack_gain_q15 = 9800;
        params->thump_gain_q15 = 9200;
        params->instance_limit = 8U;
        break;
    default:
        params->body_preset = GWB89_PRESET_RIFLE;
        params->gas_preset = GMG89_PRESET_RIFLE;
        params->crack_preset = GBC89_PRESET_RIFLE_PASS;
        params->tail_preset = GLT89_PRESET_EXTERIOR;
        params->thump_preset = GCT89_PRESET_ACTION;
        break;
    }
}


static gv89_s16 gssr89_scale_gain(gv89_s16 base, gv89_u16 control)
{
    gv89_s32 value;
    value = ((gv89_s32)base * (gv89_s32)control) / 24576;
    if (value > 32767) value = 32767;
    if (value < 0) value = 0;
    return (gv89_s16)value;
}

static gpaah89_preset_id gssr89_profile_preset(const wsounddna89_profile *profile)
{
    if (profile == 0) return GPAAH89_PRESET_PISTOL;
    switch (profile->id) {
    case WSOUNDDNA89_PROFILE_COMPACT_PISTOL:
    case WSOUNDDNA89_PROFILE_SERVICE_PISTOL:
        return GPAAH89_PRESET_PISTOL;
    case WSOUNDDNA89_PROFILE_MAGNUM:
        return GPAAH89_PRESET_MAGNUM;
    case WSOUNDDNA89_PROFILE_SMG:
    case WSOUNDDNA89_PROFILE_CARBINE:
    case WSOUNDDNA89_PROFILE_RIFLE:
        return GPAAH89_PRESET_METRALLA;
    case WSOUNDDNA89_PROFILE_SNIPER:
        return GPAAH89_PRESET_SNIPER_RIFLE;
    case WSOUNDDNA89_PROFILE_SHOTGUN:
        return GPAAH89_PRESET_SHOTGUN;
    case WSOUNDDNA89_PROFILE_HEAVY:
        return GPAAH89_PRESET_GATLING;
    case WSOUNDDNA89_PROFILE_LAUNCHER:
        return GPAAH89_PRESET_ROCKET_LAUNCHER;
    default:
        return GPAAH89_PRESET_PISTOL;
    }
}

void gssr89_apply_dna(gssr89_params *params,
                      const wsounddna89_profile *profile,
                      const wsounddna89_shot *shot,
                      wsounddna89_mode mode)
{
    gpaah89_preset_id preset;
    gv89_s16 mode_gain;
    if (params == 0 || profile == 0 || shot == 0) return;
    preset = gssr89_profile_preset(profile);
    gssr89_defaults(params, preset);
    params->report_gain_q15 = gssr89_scale_gain(params->report_gain_q15, shot->pressure_q15);
    params->body_gain_q15 = gssr89_scale_gain(params->body_gain_q15, shot->energy_q15);
    params->gas_gain_q15 = gssr89_scale_gain(params->gas_gain_q15, shot->gas_q15);
    params->crack_gain_q15 = gssr89_scale_gain(params->crack_gain_q15, shot->crack_q15);
    params->thump_gain_q15 = gssr89_scale_gain(params->thump_gain_q15, shot->thump_q15);
    params->tail_gain_q15 = gssr89_scale_gain(params->tail_gain_q15, shot->tail_q15);
    params->brightness_q15 = shot->brightness_q15;
    params->pitch_q16 = shot->pitch_q16;
    params->cycle_q16 = shot->cycle_q16;
    mode_gain = mode == WSOUNDDNA89_MODE_REALISTIC ? 25200 :
                (mode == WSOUNDDNA89_MODE_HYBRID ? 28000 : 30000);
    params->output_gain_q15 = mode_gain;
}

int gssr89_validate_params(const gssr89_params *params)
{
    if (params == 0) return 0;
    if ((int)params->report_preset < 0 || params->report_preset >= GPAAH89_PRESET_COUNT) return 0;
    if ((int)params->body_preset < 0 || params->body_preset >= GWB89_PRESET_COUNT) return 0;
    if ((int)params->gas_preset < 0 || params->gas_preset >= GMG89_PRESET_COUNT) return 0;
    if ((int)params->crack_preset < 0 || params->crack_preset >= GBC89_PRESET_COUNT) return 0;
    if ((int)params->tail_preset < 0 || params->tail_preset >= GLT89_PRESET_COUNT) return 0;
    if ((int)params->thump_preset < 0 || params->thump_preset >= GCT89_PRESET_COUNT) return 0;
    return 1;
}

int gssr89_init(gssr89_context *ctx, gssr89_voice *storage,
                gv89_u16 capacity, gv89_u32 sample_rate)
{
    gv89_u16 i;
    if (ctx == 0 || sample_rate == 0U || sample_rate > 48000U) return 0;
    if (capacity != 0U && storage == 0) return 0;
    ctx->voices = storage;
    ctx->capacity = capacity;
    ctx->sample_rate = sample_rate;
    ctx->serial_counter = 0U;
    for (i = 0U; i < capacity; ++i) gssr89_zero_voice(&storage[i]);
    return 1;
}

static int gssr89_handle_dead(const gwv89_context *handler, gv89_handle handle)
{
    if (handle.index == GV89_INVALID_INDEX) return 1;
    return !gv89_is_handle_active(&handler->voices, handle);
}

static gssr89_voice *gssr89_find_voice(gssr89_context *ctx,
                                        gwv89_context *handler)
{
    gv89_u16 i;
    gv89_u16 oldest_index;
    gv89_u32 oldest_serial;
    if (ctx == 0 || handler == 0 || ctx->capacity == 0U) return 0;
    oldest_index = 0U;
    oldest_serial = 0xFFFFFFFFUL;
    for (i = 0U; i < ctx->capacity; ++i) {
        if (!ctx->voices[i].in_use) return &ctx->voices[i];
        if (gssr89_handle_dead(handler, ctx->voices[i].handle)) {
            ctx->voices[i].in_use = 0U;
            return &ctx->voices[i];
        }
        if (ctx->voices[i].serial < oldest_serial) {
            oldest_serial = ctx->voices[i].serial;
            oldest_index = i;
        }
    }

    /* Heapless deterministic voice stealing: preserve the newest transient
       and sacrifice only the oldest report tail when the pool is saturated. */
    (void)gv89_stop(&handler->voices, ctx->voices[oldest_index].handle, 0U);
    ctx->voices[oldest_index].in_use = 0U;
    gssr89_clear_handle(&ctx->voices[oldest_index].handle);
    return &ctx->voices[oldest_index];
}


static gv89_u16 gssr89_scale_u16(gv89_u16 value, gv89_u32 scale_q16,
                                 gv89_u16 minimum, gv89_u16 maximum)
{
    gv89_u32 v;
    v = ((gv89_u32)value * scale_q16) >> 16;
    if (v < minimum) v = minimum;
    if (v > maximum) v = maximum;
    return (gv89_u16)v;
}

static void gssr89_apply_variation_to_presets(const gssr89_params *params,
                                               gpaah89_preset *report,
                                               gwb89_preset *body,
                                               gmg89_preset *gas,
                                               gbc89_preset *crack,
                                               glt89_preset *tail,
                                               gct89_preset *thump)
{
    gv89_u16 i;
    gv89_u32 inverse_pitch;
    gv89_u32 cycle;
    gv89_u32 bright;
    if (params == 0) return;
    inverse_pitch = params->pitch_q16 == 0U ? 65536U :
                    (gv89_u32)(4294967295U / params->pitch_q16);
    cycle = params->cycle_q16 == 0U ? 65536U : params->cycle_q16;
    bright = params->brightness_q15;
    for (i = 0U; i < GPAAH89_NOISE_OSCS; ++i) {
        report->envelope[i].decay_ms = gssr89_scale_u16(
            report->envelope[i].decay_ms, cycle, 1U, 2000U);
        report->envelope[i].release_ms = gssr89_scale_u16(
            report->envelope[i].release_ms, cycle, 1U, 3000U);
    }
    for (i = 3U; i < GPAAH89_EQ_BANDS; ++i) {
        gv89_s32 gain;
        gain = ((gv89_s32)report->eq[i].gain_q14 *
                (gv89_s32)(16384U + bright / 2U)) >> 14;
        if (gain > 32767) gain = 32767;
        report->eq[i].gain_q14 = (gpaah89_s16)gain;
    }
    body->exciter_ms = gssr89_scale_u16(body->exciter_ms, cycle, 1U, 250U);
    body->tail_ms = gssr89_scale_u16(body->tail_ms, cycle, 10U, 3000U);
    for (i = 0U; i < body->mode_count; ++i)
        body->mode[i].delay_at_44100 = gssr89_scale_u16(
            body->mode[i].delay_at_44100, inverse_pitch, 2U, GWB89_MAX_DELAY - 1U);
    gas->low.decay_ms = gssr89_scale_u16(gas->low.decay_ms, cycle, 1U, 2000U);
    gas->mid.decay_ms = gssr89_scale_u16(gas->mid.decay_ms, cycle, 1U, 2000U);
    gas->high.decay_ms = gssr89_scale_u16(gas->high.decay_ms, cycle, 1U, 2000U);
    gas->mid_high_hz = gssr89_scale_u16(gas->mid_high_hz, params->pitch_q16, 300U, 16000U);
    gas->high_cut_hz = gssr89_scale_u16(gas->high_cut_hz, params->pitch_q16, 1000U, 20000U);
    crack->nwave_us = gssr89_scale_u16(crack->nwave_us, inverse_pitch, 20U, 2000U);
    crack->air_cut_hz = gssr89_scale_u16(crack->air_cut_hz, params->pitch_q16, 500U, 18000U);
    tail->bounded_tail_ms = gssr89_scale_u16(tail->bounded_tail_ms, cycle, 30U, 6000U);
    thump->duration_ms = gssr89_scale_u16(thump->duration_ms, cycle, 10U, 2000U);
    thump->start_hz = gssr89_scale_u16(thump->start_hz, params->pitch_q16, 20U, 400U);
    thump->end_hz = gssr89_scale_u16(thump->end_hz, params->pitch_q16, 15U, 300U);
}

static int gssr89_start_voice(gssr89_voice *voice)
{
    gpaah89_preset report_preset;
    gwb89_preset body_preset;
    gmg89_preset gas_preset;
    gbc89_preset crack_preset;
    glt89_preset tail_preset;
    gct89_preset thump_preset;
    if (!gssr89_validate_params(&voice->params)) return 0;
    if (!gpaah89_get_preset(voice->params.report_preset, &report_preset)) return 0;
    if (!gwb89_get_preset(voice->params.body_preset, &body_preset)) return 0;
    if (!gmg89_get_preset(voice->params.gas_preset, &gas_preset)) return 0;
    if (!gbc89_get_preset(voice->params.crack_preset, &crack_preset)) return 0;
    if (!glt89_get_preset(voice->params.tail_preset, &tail_preset)) return 0;
    if (!gct89_get_preset(voice->params.thump_preset, &thump_preset)) return 0;
    gssr89_apply_variation_to_presets(&voice->params, &report_preset,
                                      &body_preset, &gas_preset, &crack_preset,
                                      &tail_preset, &thump_preset);
    if (!gpaah89_init(&voice->report, voice->sample_rate, &report_preset, voice->seed + 1U)) return 0;
    if (!gwb89_init(&voice->body, voice->sample_rate, &body_preset, voice->seed + 2U)) return 0;
    if (!gmg89_init(&voice->gas, voice->sample_rate, &gas_preset, voice->seed + 3U)) return 0;
    if (!gbc89_init(&voice->crack, voice->sample_rate, &crack_preset, voice->seed + 4U)) return 0;
    if (!glt89_init(&voice->tail, voice->sample_rate, &tail_preset)) return 0;
    if (!gct89_init(&voice->thump, voice->sample_rate, &thump_preset, voice->seed + 5U)) return 0;
    gpaah89_trigger(&voice->report, voice->seed + 11U);
    gwb89_trigger(&voice->body, 30000, voice->seed + 12U);
    gmg89_trigger(&voice->gas, 29200, voice->seed + 13U);
    gbc89_trigger(&voice->crack, 0U, 28600, voice->seed + 14U);
    gct89_trigger(&voice->thump, 28600, voice->seed + 15U);
    voice->in_use = 1U;
    return 1;
}

static void gssr89_provider_stop(void *user)
{
    gssr89_voice *voice;
    voice = (gssr89_voice *)user;
    if (voice != 0) voice->in_use = 0U;
}

static gv89_s16 gssr89_provider_process(void *user)
{
    gssr89_voice *voice;
    gpaah89_s16 dry;
    gv89_s16 body;
    gv89_s16 gas;
    gv89_s16 crack;
    gv89_s16 thump;
    gv89_s16 tail;
    gv89_s16 tail_input;
    gv89_s32 mix;
    voice = (gssr89_voice *)user;
    if (voice == 0 || !voice->in_use) return 0;
    dry = 0;
    (void)gpaah89_render_mono(&voice->report, &dry, 1U);
    body = gwb89_process_sample(&voice->body, (gwb89_s16)dry);
    gas = gmg89_process_sample(&voice->gas);
    crack = gbc89_process_sample(&voice->crack);
    thump = gct89_process_sample(&voice->thump);
    mix = gssr89_mul_q15((gv89_s16)dry, voice->params.report_gain_q15);
    mix += gssr89_mul_q15(body, voice->params.body_gain_q15);
    mix += gssr89_mul_q15(gas, voice->params.gas_gain_q15);
    mix += gssr89_mul_q15(crack, voice->params.crack_gain_q15);
    mix += gssr89_mul_q15(thump, voice->params.thump_gain_q15);
    tail_input = gssr89_sat16(mix / 2);
    tail = glt89_process_sample(&voice->tail, tail_input);
    mix += gssr89_mul_q15((gv89_s16)(tail - tail_input), voice->params.tail_gain_q15);
    mix = (mix * (gv89_s32)voice->params.output_gain_q15) >> 15;
    if (!gpaah89_is_active(&voice->report) && !gwb89_is_active(&voice->body) &&
        !gmg89_is_active(&voice->gas) && !gbc89_is_active(&voice->crack) &&
        !gct89_is_active(&voice->thump) && !glt89_is_active(&voice->tail))
        voice->in_use = 0U;
    return gssr89_sat16(mix);
}

static int gssr89_provider_active(const void *user)
{
    const gssr89_voice *voice;
    voice = (const gssr89_voice *)user;
    return voice != 0 && voice->in_use;
}

static void gssr89_provider_advance(void *user, gv89_u32 frames)
{
    gv89_u32 i;
    gssr89_voice *voice;
    voice = (gssr89_voice *)user;
    if (voice == 0) return;
    for (i = 0U; i < frames && voice->in_use; ++i)
        (void)gssr89_provider_process(voice);
}

static void gssr89_provider_restart(void *user)
{
    gssr89_voice *voice;
    voice = (gssr89_voice *)user;
    if (voice != 0) (void)gssr89_start_voice(voice);
}

static gv89_u16 gssr89_provider_level(const void *user)
{
    const gssr89_voice *voice;
    voice = (const gssr89_voice *)user;
    if (voice == 0 || !voice->in_use) return 0U;
    return 30000U;
}

gv89_result gssr89_play(gssr89_context *ctx, gwv89_context *handler,
                         const gssr89_params *params, gv89_u32 seed,
                         gv89_handle *out_handle)
{
    gssr89_voice *voice;
    gwv89_event_desc event_desc;
    gv89_provider_ex provider;
    gv89_result result;
    if (ctx == 0 || handler == 0 || params == 0 || out_handle == 0)
        return GV89_BAD_ARGUMENT;
    if (!gssr89_validate_params(params)) return GV89_BAD_ARGUMENT;
    voice = gssr89_find_voice(ctx, handler);
    if (voice == 0) return GV89_NO_VOICE;
    voice->params = *params;
    voice->sample_rate = ctx->sample_rate;
    voice->seed = seed;
    voice->serial = ++ctx->serial_counter;
    if (!gssr89_start_voice(voice)) {
        voice->in_use = 0U;
        return GV89_BAD_ARGUMENT;
    }
    provider.base.user = voice;
    provider.base.process_mono = gssr89_provider_process;
    provider.base.is_active = gssr89_provider_active;
    provider.base.stop = gssr89_provider_stop;
    provider.advance_frames = gssr89_provider_advance;
    provider.restart = gssr89_provider_restart;
    provider.estimated_level_q15 = gssr89_provider_level;
    provider.physical_state_changed = 0;
    gwv89_event_default(&event_desc, GWV89_EVENT_REPORT);
    event_desc.gain_q15 = params->gain_q15;
    event_desc.screen_x_q15 = params->pan_q15;
    event_desc.distance_q15 = params->distance_q15;
    event_desc.occlusion_q15 = params->occlusion_q15;
    event_desc.focus_q15 = params->focus_q15;
    event_desc.priority_bias = params->priority_bias;
    event_desc.instance_key = params->instance_key;
    event_desc.instance_limit = params->instance_limit;
    result = gwv89_play_ex(handler, &event_desc, &provider, out_handle);
    if (result != GV89_OK) {
        voice->in_use = 0U;
        gssr89_clear_handle(&voice->handle);
        return result;
    }
    voice->handle = *out_handle;
    return GV89_OK;
}

void gssr89_reset(gssr89_context *ctx, gwv89_context *handler)
{
    gv89_u16 i;
    if (ctx == 0) return;
    for (i = 0U; i < ctx->capacity; ++i) {
        if (ctx->voices[i].in_use && handler != 0)
            (void)gv89_stop(&handler->voices, ctx->voices[i].handle, 0U);
        gssr89_zero_voice(&ctx->voices[i]);
    }
    ctx->serial_counter = 0U;
}

gv89_u16 gssr89_active_count(const gssr89_context *ctx)
{
    gv89_u16 i;
    gv89_u16 count;
    if (ctx == 0) return 0U;
    count = 0U;
    for (i = 0U; i < ctx->capacity; ++i)
        if (ctx->voices[i].in_use) ++count;
    return count;
}

gv89_u32 gssr89_voice_bytes(void) { return (gv89_u32)sizeof(gssr89_voice); }
gv89_u32 gssr89_context_bytes(void) { return (gv89_u32)sizeof(gssr89_context); }
