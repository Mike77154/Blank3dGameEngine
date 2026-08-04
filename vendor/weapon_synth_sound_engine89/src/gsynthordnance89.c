#include "gsynthordnance89.h"

static void gsso89_clear_handle(gv89_handle *handle)
{
    handle->index = GV89_INVALID_INDEX;
    handle->generation = 0U;
}

static int gsso89_common_valid(const gsso89_common_params *params)
{
    if (params == 0) return 0;
    if (params->distance_q15 > 32767U) return 0;
    if (params->occlusion_q15 > 32767U) return 0;
    if (params->focus_q15 > 32767U) return 0;
    return 1;
}

static int gsso89_bullet_valid(const gsso89_bullet_params *params)
{
    if (params == 0 || !gsso89_common_valid(&params->common)) return 0;
    return params->preset_id >= 0 && params->preset_id < GBA89_PRESET_COUNT;
}

static int gsso89_grenade_valid(const gsso89_grenade_params *params)
{
    if (params == 0 || !gsso89_common_valid(&params->common)) return 0;
    if (params->preset_id < 0 || params->preset_id >= WS_GGB89_PRESET_COUNT) return 0;
    return params->intensity_q15 >= 0;
}

static int gsso89_rocket_valid(const gsso89_rocket_params *params)
{
    if (params == 0 || !gsso89_common_valid(&params->common)) return 0;
    if (params->use_custom_params > 1U) return 0;
    if (!params->use_custom_params
        && params->preset_id >= WSRB89_PRESET_COUNT) return 0;
    return params->velocity_q15 <= 32767U;
}

static void gsso89_zero_bullet(gsso89_bullet_voice *voice)
{
    gsso89_clear_handle(&voice->handle);
    voice->seed = 0U;
    voice->serial = 0U;
    voice->phase_q16 = 0U;
    voice->step_q16 = 65536U;
    voice->sample_a = 0;
    voice->sample_b = 0;
    voice->primed = 0U;
    voice->in_use = 0U;
}

static void gsso89_zero_grenade(gsso89_grenade_voice *voice)
{
    gsso89_clear_handle(&voice->handle);
    voice->seed = 0U;
    voice->serial = 0U;
    voice->sample_rate = 0U;
    voice->in_use = 0U;
}

static void gsso89_zero_rocket(gsso89_rocket_voice *voice)
{
    gsso89_clear_handle(&voice->handle);
    voice->seed = 0U;
    voice->serial = 0U;
    voice->sample_rate = 0U;
    voice->in_use = 0U;
}

void gsso89_common_defaults(gsso89_common_params *params)
{
    if (params == 0) return;
    params->pan_q15 = 0;
    params->distance_q15 = 2500U;
    params->occlusion_q15 = 32767U;
    params->focus_q15 = 32767U;
    params->gain_q15 = 24500;
    params->priority_bias = 0;
    params->instance_key = 0U;
    params->instance_limit = 8U;
}

void gsso89_bullet_defaults(gsso89_bullet_params *params)
{
    if (params == 0) return;
    gsso89_common_defaults(&params->common);
    params->preset_id = GBA89_PRESET_CLOSE_RIFLE_PASS;
    params->common.gain_q15 = 24500;
    params->common.priority_bias = 85;
    params->common.instance_limit = 24U;
}

void gsso89_grenade_defaults(gsso89_grenade_params *params)
{
    if (params == 0) return;
    gsso89_common_defaults(&params->common);
    params->preset_id = WS_GGB89_M67_OPEN;
    params->intensity_q15 = 32767;
    params->common.gain_q15 = 31800;
    params->common.priority_bias = 190;
    params->common.instance_limit = 6U;
}

void gsso89_rocket_defaults(gsso89_rocket_params *params)
{
    if (params == 0) return;
    gsso89_common_defaults(&params->common);
    params->preset_id = WSRB89_PRESET_HEAVY_IMPACT;
    params->velocity_q15 = 31000U;
    params->use_custom_params = 0U;
    wsrb89_get_preset(&params->synth_params, params->preset_id);
    params->common.gain_q15 = 32200;
    params->common.priority_bias = 230;
    params->common.instance_limit = 4U;
}

void gsso89_rocket_load_preset(gsso89_rocket_params *params,
                                gv89_u16 preset_id)
{
    if (params == 0) return;
    if (preset_id >= WSRB89_PRESET_COUNT) {
        preset_id = WSRB89_PRESET_HEAVY_IMPACT;
    }
    params->preset_id = preset_id;
    wsrb89_get_preset(&params->synth_params, preset_id);
    params->use_custom_params = 0U;
}

void gsso89_rocket_enable_custom(gsso89_rocket_params *params)
{
    if (params == 0) return;
    params->use_custom_params = 1U;
}

int gsso89_init(gsso89_context *ctx,
                gsso89_bullet_voice *bullet_storage,
                gv89_u16 bullet_capacity,
                gsso89_grenade_voice *grenade_storage,
                gv89_u16 grenade_capacity,
                gsso89_rocket_voice *rocket_storage,
                gv89_u16 rocket_capacity,
                gv89_u32 sample_rate)
{
    gv89_u16 i;
    if (ctx == 0 || sample_rate < 8000U || sample_rate > 48000U) return 0;
    if (bullet_capacity != 0U && bullet_storage == 0) return 0;
    if (grenade_capacity != 0U && grenade_storage == 0) return 0;
    if (rocket_capacity != 0U && rocket_storage == 0) return 0;
    ctx->bullet_voices = bullet_storage;
    ctx->bullet_capacity = bullet_capacity;
    ctx->grenade_voices = grenade_storage;
    ctx->grenade_capacity = grenade_capacity;
    ctx->rocket_voices = rocket_storage;
    ctx->rocket_capacity = rocket_capacity;
    ctx->sample_rate = sample_rate;
    ctx->serial_counter = 0U;
    for (i = 0U; i < bullet_capacity; ++i) gsso89_zero_bullet(&bullet_storage[i]);
    for (i = 0U; i < grenade_capacity; ++i) gsso89_zero_grenade(&grenade_storage[i]);
    for (i = 0U; i < rocket_capacity; ++i) gsso89_zero_rocket(&rocket_storage[i]);
    return 1;
}

static int gsso89_handle_dead(const gwv89_context *handler, gv89_handle handle)
{
    if (handle.index == GV89_INVALID_INDEX) return 1;
    return !gv89_is_handle_active(&handler->voices, handle);
}

static gsso89_bullet_voice *gsso89_find_bullet(gsso89_context *ctx,
                                                const gwv89_context *handler)
{
    gv89_u16 i;
    for (i = 0U; i < ctx->bullet_capacity; ++i) {
        if (!ctx->bullet_voices[i].in_use) return &ctx->bullet_voices[i];
        if (gsso89_handle_dead(handler, ctx->bullet_voices[i].handle)) {
            ctx->bullet_voices[i].in_use = 0U;
            return &ctx->bullet_voices[i];
        }
    }
    return 0;
}

static gsso89_grenade_voice *gsso89_find_grenade(gsso89_context *ctx,
                                                  const gwv89_context *handler)
{
    gv89_u16 i;
    for (i = 0U; i < ctx->grenade_capacity; ++i) {
        if (!ctx->grenade_voices[i].in_use) return &ctx->grenade_voices[i];
        if (gsso89_handle_dead(handler, ctx->grenade_voices[i].handle)) {
            ctx->grenade_voices[i].in_use = 0U;
            return &ctx->grenade_voices[i];
        }
    }
    return 0;
}

static gsso89_rocket_voice *gsso89_find_rocket(gsso89_context *ctx,
                                                const gwv89_context *handler)
{
    gv89_u16 i;
    for (i = 0U; i < ctx->rocket_capacity; ++i) {
        if (!ctx->rocket_voices[i].in_use) return &ctx->rocket_voices[i];
        if (gsso89_handle_dead(handler, ctx->rocket_voices[i].handle)) {
            ctx->rocket_voices[i].in_use = 0U;
            return &ctx->rocket_voices[i];
        }
    }
    return 0;
}

static void gsso89_bullet_stop(void *user)
{
    gsso89_bullet_voice *voice;
    voice = (gsso89_bullet_voice *)user;
    if (voice == 0) return;
    gba89_stop(&voice->synth);
    voice->in_use = 0U;
}

static gv89_s16 gsso89_bullet_process(void *user)
{
    gsso89_bullet_voice *voice;
    gv89_s32 out;
    gv89_s32 delta;
    gv89_u32 frac;
    voice = (gsso89_bullet_voice *)user;
    if (voice == 0 || !voice->in_use) return 0;
    if (!voice->primed) {
        voice->sample_a = 0;
        voice->sample_b = 0;
        gba89_render_mono(&voice->synth, (gba89_s16 *)&voice->sample_a, 1UL);
        gba89_render_mono(&voice->synth, (gba89_s16 *)&voice->sample_b, 1UL);
        voice->primed = 1U;
    }
    frac = voice->phase_q16 & 65535U;
    delta = (gv89_s32)voice->sample_b - (gv89_s32)voice->sample_a;
    out = (gv89_s32)voice->sample_a +
          (gv89_s32)((delta * (gv89_s32)frac) >> 16);
    voice->phase_q16 += voice->step_q16;
    while (voice->phase_q16 >= 65536U) {
        voice->phase_q16 -= 65536U;
        voice->sample_a = voice->sample_b;
        voice->sample_b = 0;
        gba89_render_mono(&voice->synth, (gba89_s16 *)&voice->sample_b, 1UL);
        if (!gba89_is_active(&voice->synth)) {
            voice->in_use = 0U;
            break;
        }
    }
    if (out > 32767) out = 32767;
    if (out < -32768) out = -32768;
    return (gv89_s16)out;
}

static int gsso89_bullet_active(const void *user)
{
    const gsso89_bullet_voice *voice;
    voice = (const gsso89_bullet_voice *)user;
    return voice != 0 && voice->in_use && gba89_is_active(&voice->synth);
}

static void gsso89_bullet_advance(void *user, gv89_u32 frames)
{
    gsso89_bullet_voice *voice;
    gv89_u32 i;
    voice = (gsso89_bullet_voice *)user;
    if (voice == 0 || !voice->in_use) return;
    for (i = 0U; i < frames && voice->in_use; ++i)
        (void)gsso89_bullet_process(voice);
}

static void gsso89_bullet_restart(void *user)
{
    gsso89_bullet_voice *voice;
    voice = (gsso89_bullet_voice *)user;
    if (voice == 0) return;
    gba89_init(&voice->synth, (gba89_u32)voice->seed);
    gba89_trigger_preset(&voice->synth, (int)voice->params.preset_id);
    voice->phase_q16 = 0U;
    voice->sample_a = 0;
    voice->sample_b = 0;
    voice->primed = 0U;
    voice->in_use = 1U;
}

static gv89_u16 gsso89_bullet_level(const void *user)
{
    const gsso89_bullet_voice *voice;
    voice = (const gsso89_bullet_voice *)user;
    if (voice == 0 || !voice->in_use) return 0U;
    return 24500U;
}

static void gsso89_grenade_stop(void *user)
{
    gsso89_grenade_voice *voice;
    voice = (gsso89_grenade_voice *)user;
    if (voice == 0) return;
    ws_ggb89_reset(&voice->synth);
    voice->in_use = 0U;
}

static gv89_s16 gsso89_grenade_process(void *user)
{
    gsso89_grenade_voice *voice;
    ws_gs16 sample;
    voice = (gsso89_grenade_voice *)user;
    if (voice == 0 || !voice->in_use) return 0;
    sample = ws_ggb89_process(&voice->synth);
    if (!ws_ggb89_is_active(&voice->synth)) voice->in_use = 0U;
    return (gv89_s16)sample;
}

static int gsso89_grenade_active(const void *user)
{
    const gsso89_grenade_voice *voice;
    voice = (const gsso89_grenade_voice *)user;
    return voice != 0 && voice->in_use && ws_ggb89_is_active(&voice->synth);
}

static void gsso89_grenade_restart(void *user)
{
    gsso89_grenade_voice *voice;
    voice = (gsso89_grenade_voice *)user;
    if (voice == 0) return;
    ws_ggb89_init(&voice->synth, (ws_gu32)voice->sample_rate, (ws_gu32)voice->seed);
    ws_ggb89_trigger(&voice->synth, (int)voice->params.preset_id,
                     (ws_gs16)voice->params.intensity_q15);
    voice->in_use = 1U;
}

static gv89_u16 gsso89_grenade_level(const void *user)
{
    const gsso89_grenade_voice *voice;
    voice = (const gsso89_grenade_voice *)user;
    if (voice == 0 || !voice->in_use) return 0U;
    if (voice->params.intensity_q15 < 0) return 0U;
    return (gv89_u16)voice->params.intensity_q15;
}

static void gsso89_rocket_stop(void *user)
{
    gsso89_rocket_voice *voice;
    voice = (gsso89_rocket_voice *)user;
    if (voice == 0) return;
    wsrb89_reset(&voice->synth);
    voice->in_use = 0U;
}

static gv89_s16 gsso89_rocket_process(void *user)
{
    gsso89_rocket_voice *voice;
    wsrb89_s16 left;
    wsrb89_s16 right;
    gv89_s32 mono;
    voice = (gsso89_rocket_voice *)user;
    if (voice == 0 || !voice->in_use) return 0;
    left = 0;
    right = 0;
    wsrb89_render_stereo(&voice->synth, &left, &right, 1U);
    if (!wsrb89_is_active(&voice->synth)) voice->in_use = 0U;
    mono = ((gv89_s32)left + (gv89_s32)right) / 2;
    if (mono > 32767) mono = 32767;
    if (mono < -32768) mono = -32768;
    return (gv89_s16)mono;
}

static int gsso89_rocket_active(const void *user)
{
    const gsso89_rocket_voice *voice;
    voice = (const gsso89_rocket_voice *)user;
    return voice != 0 && voice->in_use && wsrb89_is_active(&voice->synth);
}

static void gsso89_rocket_restart(void *user)
{
    gsso89_rocket_voice *voice;
    wsrb89_params preset;
    voice = (gsso89_rocket_voice *)user;
    if (voice == 0) return;
    wsrb89_init(&voice->synth, &voice->workspace, voice->sample_rate, voice->seed);
    if (voice->params.use_custom_params) {
        preset = voice->params.synth_params;
    } else {
        wsrb89_get_preset(&preset, voice->params.preset_id);
    }
    wsrb89_set_params(&voice->synth, &preset);
    wsrb89_trigger(&voice->synth, voice->params.velocity_q15);
    voice->in_use = 1U;
}

static gv89_u16 gsso89_rocket_level(const void *user)
{
    const gsso89_rocket_voice *voice;
    voice = (const gsso89_rocket_voice *)user;
    if (voice == 0 || !voice->in_use) return 0U;
    return voice->params.velocity_q15;
}

static void gsso89_provider_common(gv89_provider_ex *provider, void *user,
                                   gv89_process_mono_fn process,
                                   gv89_is_active_fn active,
                                   gv89_stop_fn stop,
                                   gv89_advance_frames_fn advance,
                                   gv89_restart_fn restart,
                                   gv89_estimated_level_fn level)
{
    provider->base.user = user;
    provider->base.process_mono = process;
    provider->base.is_active = active;
    provider->base.stop = stop;
    provider->advance_frames = advance;
    provider->restart = restart;
    provider->estimated_level_q15 = level;
    provider->physical_state_changed = 0;
}

static void gsso89_desc_common(gwv89_event_desc *desc,
                               gwv89_event_class event_class,
                               const gsso89_common_params *params)
{
    gwv89_event_default(desc, event_class);
    desc->gain_q15 = params->gain_q15;
    desc->screen_x_q15 = params->pan_q15;
    desc->distance_q15 = params->distance_q15;
    desc->occlusion_q15 = params->occlusion_q15;
    desc->focus_q15 = params->focus_q15;
    desc->priority_bias = params->priority_bias;
    desc->instance_key = params->instance_key;
    desc->instance_limit = params->instance_limit;
    desc->allow_higher_priority_steal = 1U;
}

gv89_result gsso89_play_bullet(gsso89_context *ctx,
                                gwv89_context *handler,
                                const gsso89_bullet_params *params,
                                gv89_u32 seed,
                                gv89_handle *out_handle)
{
    gsso89_bullet_voice *voice;
    gwv89_event_desc desc;
    gv89_reservation reservation;
    gv89_voice_params resolved;
    gv89_provider_ex provider;
    gv89_result result;
    if (ctx == 0 || handler == 0 || !gsso89_bullet_valid(params)) return GV89_BAD_ARGUMENT;
    gsso89_desc_common(&desc, GWV89_EVENT_RICOCHET, &params->common);
    desc.attack_ms = 0U;
    desc.release_ms = 18U;
    desc.override_virtual_behavior = 1U;
    desc.virtual_behavior = GV89_VIRTUAL_ADVANCE;
    result = gwv89_reserve_event(handler, &desc, &reservation, &resolved);
    if (result != GV89_OK) return result;
    voice = gsso89_find_bullet(ctx, handler);
    if (voice == 0) {
        gwv89_cancel_event(handler, &reservation);
        return GV89_NO_VOICE;
    }
    voice->params = *params;
    voice->seed = seed;
    voice->phase_q16 = 0U;
    voice->step_q16 = (gv89_u32)(((gba89_u32)GBA89_SAMPLE_RATE << 16) / ctx->sample_rate);
    voice->sample_a = 0;
    voice->sample_b = 0;
    voice->primed = 0U;
    gba89_init(&voice->synth, (gba89_u32)seed);
    gba89_trigger_preset(&voice->synth, (int)params->preset_id);
    voice->in_use = 1U;
    voice->serial = ++ctx->serial_counter;
    gsso89_provider_common(&provider, voice, gsso89_bullet_process,
                           gsso89_bullet_active, gsso89_bullet_stop,
                           gsso89_bullet_advance, gsso89_bullet_restart,
                           gsso89_bullet_level);
    result = gwv89_commit_event_ex(handler, &reservation, &provider, &resolved,
                                   &voice->handle);
    if (result != GV89_OK) voice->in_use = 0U;
    else if (out_handle != 0) *out_handle = voice->handle;
    return result;
}

gv89_result gsso89_play_grenade(gsso89_context *ctx,
                                 gwv89_context *handler,
                                 const gsso89_grenade_params *params,
                                 gv89_u32 seed,
                                 gv89_handle *out_handle)
{
    gsso89_grenade_voice *voice;
    gwv89_event_desc desc;
    gv89_reservation reservation;
    gv89_voice_params resolved;
    gv89_provider_ex provider;
    gv89_result result;
    if (ctx == 0 || handler == 0 || !gsso89_grenade_valid(params)) return GV89_BAD_ARGUMENT;
    gsso89_desc_common(&desc, GWV89_EVENT_EXPLOSION, &params->common);
    desc.attack_ms = 0U;
    desc.release_ms = 90U;
    desc.add_flags |= GV89_FLAG_CRITICAL | GV89_FLAG_NEVER_VIRTUAL;
    result = gwv89_reserve_event(handler, &desc, &reservation, &resolved);
    if (result != GV89_OK) return result;
    voice = gsso89_find_grenade(ctx, handler);
    if (voice == 0) {
        gwv89_cancel_event(handler, &reservation);
        return GV89_NO_VOICE;
    }
    voice->params = *params;
    voice->seed = seed;
    voice->sample_rate = ctx->sample_rate;
    ws_ggb89_init(&voice->synth, (ws_gu32)ctx->sample_rate, (ws_gu32)seed);
    ws_ggb89_trigger(&voice->synth, (int)params->preset_id,
                     (ws_gs16)params->intensity_q15);
    voice->in_use = 1U;
    voice->serial = ++ctx->serial_counter;
    gsso89_provider_common(&provider, voice, gsso89_grenade_process,
                           gsso89_grenade_active, gsso89_grenade_stop,
                           0, gsso89_grenade_restart, gsso89_grenade_level);
    result = gwv89_commit_event_ex(handler, &reservation, &provider, &resolved,
                                   &voice->handle);
    if (result != GV89_OK) voice->in_use = 0U;
    else if (out_handle != 0) *out_handle = voice->handle;
    return result;
}

gv89_result gsso89_play_rocket(gsso89_context *ctx,
                                gwv89_context *handler,
                                const gsso89_rocket_params *params,
                                gv89_u32 seed,
                                gv89_handle *out_handle)
{
    gsso89_rocket_voice *voice;
    wsrb89_params preset;
    gwv89_event_desc desc;
    gv89_reservation reservation;
    gv89_voice_params resolved;
    gv89_provider_ex provider;
    gv89_result result;
    if (ctx == 0 || handler == 0 || !gsso89_rocket_valid(params)) return GV89_BAD_ARGUMENT;
    gsso89_desc_common(&desc, GWV89_EVENT_EXPLOSION, &params->common);
    desc.attack_ms = 0U;
    desc.release_ms = 120U;
    desc.add_flags |= GV89_FLAG_CRITICAL | GV89_FLAG_NEVER_VIRTUAL;
    result = gwv89_reserve_event(handler, &desc, &reservation, &resolved);
    if (result != GV89_OK) return result;
    voice = gsso89_find_rocket(ctx, handler);
    if (voice == 0) {
        gwv89_cancel_event(handler, &reservation);
        return GV89_NO_VOICE;
    }
    voice->params = *params;
    voice->seed = seed;
    voice->sample_rate = ctx->sample_rate;
    wsrb89_init(&voice->synth, &voice->workspace, ctx->sample_rate, seed);
    if (params->use_custom_params) {
        preset = params->synth_params;
    } else {
        wsrb89_get_preset(&preset, params->preset_id);
    }
    wsrb89_set_params(&voice->synth, &preset);
    wsrb89_trigger(&voice->synth, params->velocity_q15);
    voice->in_use = 1U;
    voice->serial = ++ctx->serial_counter;
    gsso89_provider_common(&provider, voice, gsso89_rocket_process,
                           gsso89_rocket_active, gsso89_rocket_stop,
                           0, gsso89_rocket_restart, gsso89_rocket_level);
    result = gwv89_commit_event_ex(handler, &reservation, &provider, &resolved,
                                   &voice->handle);
    if (result != GV89_OK) voice->in_use = 0U;
    else if (out_handle != 0) *out_handle = voice->handle;
    return result;
}

void gsso89_reset(gsso89_context *ctx, gwv89_context *handler)
{
    gv89_u16 i;
    if (ctx == 0) return;
    for (i = 0U; i < ctx->bullet_capacity; ++i) {
        if (handler != 0 && ctx->bullet_voices[i].handle.index != GV89_INVALID_INDEX)
            (void)gv89_stop(&handler->voices, ctx->bullet_voices[i].handle, 0U);
        gba89_stop(&ctx->bullet_voices[i].synth);
        gsso89_zero_bullet(&ctx->bullet_voices[i]);
    }
    for (i = 0U; i < ctx->grenade_capacity; ++i) {
        if (handler != 0 && ctx->grenade_voices[i].handle.index != GV89_INVALID_INDEX)
            (void)gv89_stop(&handler->voices, ctx->grenade_voices[i].handle, 0U);
        ws_ggb89_reset(&ctx->grenade_voices[i].synth);
        gsso89_zero_grenade(&ctx->grenade_voices[i]);
    }
    for (i = 0U; i < ctx->rocket_capacity; ++i) {
        if (handler != 0 && ctx->rocket_voices[i].handle.index != GV89_INVALID_INDEX)
            (void)gv89_stop(&handler->voices, ctx->rocket_voices[i].handle, 0U);
        wsrb89_reset(&ctx->rocket_voices[i].synth);
        gsso89_zero_rocket(&ctx->rocket_voices[i]);
    }
}

gv89_u16 gsso89_active_bullets(const gsso89_context *ctx)
{
    gv89_u16 i;
    gv89_u16 count;
    count = 0U;
    if (ctx == 0) return 0U;
    for (i = 0U; i < ctx->bullet_capacity; ++i)
        if (ctx->bullet_voices[i].in_use) ++count;
    return count;
}

gv89_u16 gsso89_active_grenades(const gsso89_context *ctx)
{
    gv89_u16 i;
    gv89_u16 count;
    count = 0U;
    if (ctx == 0) return 0U;
    for (i = 0U; i < ctx->grenade_capacity; ++i)
        if (ctx->grenade_voices[i].in_use) ++count;
    return count;
}

gv89_u16 gsso89_active_rockets(const gsso89_context *ctx)
{
    gv89_u16 i;
    gv89_u16 count;
    count = 0U;
    if (ctx == 0) return 0U;
    for (i = 0U; i < ctx->rocket_capacity; ++i)
        if (ctx->rocket_voices[i].in_use) ++count;
    return count;
}

gv89_u32 gsso89_bullet_voice_bytes(void)
{
    return (gv89_u32)sizeof(gsso89_bullet_voice);
}

gv89_u32 gsso89_grenade_voice_bytes(void)
{
    return (gv89_u32)sizeof(gsso89_grenade_voice);
}

gv89_u32 gsso89_rocket_voice_bytes(void)
{
    return (gv89_u32)sizeof(gsso89_rocket_voice);
}

gv89_u32 gsso89_context_bytes(void)
{
    return (gv89_u32)sizeof(gsso89_context);
}
