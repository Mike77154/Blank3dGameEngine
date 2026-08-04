#include "gsynthsoundengine89.h"

static gv89_s32 gsse89_clamp_q15(gv89_s32 value)
{
    if (value < 0) return 0;
    if (value > 32767) return 32767;
    return value;
}

static gv89_u32 gsse89_ms_to_frames_saturated(gv89_u32 ms,
                                               gv89_u32 sample_rate)
{
    gv89_u32 seconds;
    gv89_u32 remainder_ms;
    gv89_u32 base;
    gv89_u32 extra;
    seconds = ms / 1000U;
    remainder_ms = ms % 1000U;
    if (sample_rate == 0U) return 0U;
    if (seconds > 0xFFFFFFFFU / sample_rate) return 0xFFFFFFFFU;
    base = seconds * sample_rate;
    extra = (remainder_ms * sample_rate) / 1000U;
    if (base > 0xFFFFFFFFU - extra) return 0xFFFFFFFFU;
    return base + extra;
}

static int gsse89_common_spatial_valid(gv89_u16 distance_q15,
                                       gv89_u16 occlusion_q15,
                                       gv89_u16 focus_q15)
{
    return distance_q15 <= 32767U && occlusion_q15 <= 32767U &&
           focus_q15 <= 32767U;
}

static int gsse89_casing_params_valid(const gsse89_casing_params *params)
{
    if (params == 0) return 0;
    if ((int)params->shell < 0 || params->shell >= GT89_SHELL_COUNT) return 0;
    if ((int)params->surface < 0 || params->surface >= GT89_SURFACE_COUNT) return 0;
    return gsse89_common_spatial_valid(params->distance_q15,
                                       params->occlusion_q15,
                                       params->focus_q15);
}

static int gsse89_fire_params_valid(const gsse89_fire_params *params)
{
    if (params == 0) return 0;
    if ((int)params->role < (int)GSSE89_FIRE_ROLE_AMBIENCE ||
        (int)params->role > (int)GSSE89_FIRE_ROLE_EXPLOSIVE_DEBRIS) return 0;
    if (params->preset_id < 0 || params->preset_id >= GFIRE89_PRESET_COUNT) return 0;
    if (params->intensity_q15 < 0 || params->intensity_q15 > 32767) return 0;
    if (params->airflow_q15 < 0 || params->airflow_q15 > 32767) return 0;
    if (params->crackle_q15 < 0 || params->crackle_q15 > 32767) return 0;
    if (params->size_q15 < 0 || params->size_q15 > 32767) return 0;
    if (params->pressure_q15 < 0 || params->pressure_q15 > 32767) return 0;
    if (params->drive_q15 < 0 || params->drive_q15 > 32767) return 0;
    if (params->brightness_q15 < 0 || params->brightness_q15 > 32767) return 0;
    if (params->output_gain_q15 < 0 || params->output_gain_q15 > 32767) return 0;
    return gsse89_common_spatial_valid(params->distance_q15,
                                       params->occlusion_q15,
                                       params->focus_q15);
}

static gv89_s16 gsse89_mul_s16_q15(gv89_s16 value, gv89_s32 gain_q15)
{
    gv89_s32 result;
    result = ((gv89_s32)value * gain_q15) >> 15;
    if (result > 32767) result = 32767;
    if (result < -32768) result = -32768;
    return (gv89_s16)result;
}

static gv89_u16 gsse89_active_flamethrowers_internal(const gsse89_context *ctx)
{
    gv89_u16 i;
    gv89_u16 count;
    count = 0U;
    if (ctx == 0) return 0U;
    for (i = 0U; i < ctx->fire_capacity; ++i) {
        if (ctx->fire_voices[i].in_use &&
            ctx->fire_voices[i].params.role == GSSE89_FIRE_ROLE_FLAMETHROWER)
            ++count;
    }
    return count;
}

static void gsse89_zero_casing_voice(gsse89_casing_voice *voice)
{
    voice->handle.index = GV89_INVALID_INDEX;
    voice->handle.generation = 0U;
    voice->serial = 0U;
    voice->in_use = 0U;
}

static void gsse89_zero_fire_voice(gsse89_fire_voice *voice)
{
    voice->handle.index = GV89_INVALID_INDEX;
    voice->handle.generation = 0U;
    voice->age_frames = 0U;
    voice->gate_off_frame = 0U;
    voice->total_frames = 0U;
    voice->seed = 0U;
    voice->serial = 0U;
    voice->in_use = 0U;
    voice->gate_closed = 0U;
}

void gsse89_casing_defaults(gsse89_casing_params *params)
{
    if (params == 0) return;
    params->shell = GT89_SHELL_PISTOL_BRASS;
    params->surface = GT89_SURFACE_CONCRETE;
    params->velocity = 176U;
    params->angular_velocity = 196U;
    params->variation = 0U;
    params->pan_q15 = 0;
    params->distance_q15 = 3000U;
    params->occlusion_q15 = 32767U;
    params->focus_q15 = 32767U;
    params->gain_q15 = 24500;
    params->priority_bias = 0;
    params->instance_key = 0U;
    params->instance_limit = 12U;
}

void gsse89_fire_defaults(gsse89_fire_params *params,
                          gsse89_fire_role role)
{
    if (params == 0) return;
    params->role = role;
    params->preset_id = role == GSSE89_FIRE_ROLE_FLAMETHROWER ?
                        GFIRE89_PRESET_FLAMETHROWER : GFIRE89_PRESET_CAMPFIRE;
    params->intensity_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 30500 : 22000;
    params->airflow_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 22000 : 14500;
    params->crackle_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 26500 : 26000;
    params->size_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 30000 : 18000;
    params->pressure_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 31500 : 15000;
    params->drive_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 22500 : 9000;
    params->brightness_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 15500 : 16000;
    params->output_gain_q15 = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 26000 : 24500;
    params->pan_q15 = 0;
    params->distance_q15 = 6000U;
    params->occlusion_q15 = 32767U;
    params->focus_q15 = 32767U;
    params->gain_q15 = 22000;
    params->priority_bias = role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 90 : -20;
    params->duration_ms = role == GSSE89_FIRE_ROLE_AMBIENCE ? 0U : 1800U;
    params->release_ms = 180U;
    params->instance_key = 0U;
    params->instance_limit = role == GSSE89_FIRE_ROLE_AMBIENCE ? 8U : 4U;
}

int gsse89_init(gsse89_context *ctx,
                 gsse89_casing_voice *casing_storage,
                 gv89_u16 casing_capacity,
                 gsse89_fire_voice *fire_storage,
                 gv89_u16 fire_capacity,
                 gv89_u32 sample_rate)
{
    gv89_u16 i;
    if (ctx == 0 || sample_rate < 8000U || sample_rate > 48000U) return 0;
    if (casing_capacity != 0U && casing_storage == 0) return 0;
    if (fire_capacity != 0U && fire_storage == 0) return 0;
    ctx->casing_voices = casing_storage;
    ctx->casing_capacity = casing_capacity;
    ctx->fire_voices = fire_storage;
    ctx->fire_capacity = fire_capacity;
    ctx->sample_rate = sample_rate;
    ctx->serial_counter = 0U;
    for (i = 0U; i < casing_capacity; ++i) gsse89_zero_casing_voice(&casing_storage[i]);
    for (i = 0U; i < fire_capacity; ++i) gsse89_zero_fire_voice(&fire_storage[i]);
    return 1;
}

static void gsse89_casing_stop_cb(void *user)
{
    gsse89_casing_voice *voice;
    voice = (gsse89_casing_voice *)user;
    if (voice == 0) return;
    gt89_reset(&voice->synth);
    voice->in_use = 0U;
}

static gv89_s16 gsse89_casing_process_cb(void *user)
{
    gsse89_casing_voice *voice;
    gt89_s16 sample;
    voice = (gsse89_casing_voice *)user;
    if (voice == 0 || !voice->in_use) return 0;
    sample = 0;
    gt89_render_mono_i16(&voice->synth, &sample, 1U);
    if (gt89_active_voice_count(&voice->synth) == 0U) voice->in_use = 0U;
    return (gv89_s16)sample;
}

static int gsse89_casing_active_cb(const void *user)
{
    const gsse89_casing_voice *voice;
    voice = (const gsse89_casing_voice *)user;
    if (voice == 0 || !voice->in_use) return 0;
    return gt89_active_voice_count(&voice->synth) != 0U;
}

static void gsse89_casing_advance_cb(void *user, gv89_u32 frames)
{
    gsse89_casing_voice *voice;
    gt89_s16 sample;
    gv89_u32 i;
    voice = (gsse89_casing_voice *)user;
    if (voice == 0 || !voice->in_use) return;
    sample = 0;
    for (i = 0U; i < frames && voice->in_use; ++i) {
        gt89_render_mono_i16(&voice->synth, &sample, 1U);
        if (gt89_active_voice_count(&voice->synth) == 0U) voice->in_use = 0U;
    }
}

static gv89_u16 gsse89_casing_level_cb(const void *user)
{
    const gsse89_casing_voice *voice;
    gv89_u32 count;
    voice = (const gsse89_casing_voice *)user;
    if (voice == 0 || !voice->in_use) return 0U;
    count = gt89_active_voice_count(&voice->synth);
    if (count == 0U) return 0U;
    if (count > 2U) return 24000U;
    return 17000U;
}

static void gsse89_fire_stop_cb(void *user)
{
    gsse89_fire_voice *voice;
    voice = (gsse89_fire_voice *)user;
    if (voice == 0) return;
    gfire89_set_gate(&voice->synth, 0);
    voice->in_use = 0U;
}

static gv89_s16 gsse89_fire_process_cb(void *user)
{
    gsse89_fire_voice *voice;
    gfire89_s16 sample;
    voice = (gsse89_fire_voice *)user;
    if (voice == 0 || !voice->in_use) return 0;
    if (voice->gate_off_frame != 0U && !voice->gate_closed &&
        voice->age_frames >= voice->gate_off_frame) {
        gfire89_set_gate(&voice->synth, 0);
        voice->gate_closed = 1U;
    }
    sample = gfire89_process_sample(&voice->synth);
    ++voice->age_frames;
    if (voice->total_frames != 0U && voice->age_frames >= voice->total_frames) {
        voice->in_use = 0U;
    }
    return (gv89_s16)sample;
}

static int gsse89_fire_active_cb(const void *user)
{
    const gsse89_fire_voice *voice;
    voice = (const gsse89_fire_voice *)user;
    return voice != 0 && voice->in_use;
}

static void gsse89_fire_restart_cb(void *user)
{
    gsse89_fire_voice *voice;
    voice = (gsse89_fire_voice *)user;
    if (voice == 0) return;
    gfire89_init(&voice->synth, voice->synth.sample_rate, voice->seed);
    gfire89_set_preset(&voice->synth, voice->params.preset_id);
    gfire89_set_controls(&voice->synth,
                         voice->params.intensity_q15,
                         voice->params.airflow_q15,
                         voice->params.crackle_q15,
                         voice->params.size_q15);
    gfire89_set_detail(&voice->synth,
                       voice->params.pressure_q15,
                       voice->params.drive_q15,
                       voice->params.brightness_q15);
    gfire89_set_output_gain(&voice->synth, voice->params.output_gain_q15);
    gfire89_set_gate(&voice->synth, 1);
    voice->age_frames = 0U;
    voice->gate_closed = 0U;
    voice->in_use = 1U;
}

static gv89_u16 gsse89_fire_level_cb(const void *user)
{
    const gsse89_fire_voice *voice;
    gv89_s32 level;
    voice = (const gsse89_fire_voice *)user;
    if (voice == 0 || !voice->in_use) return 0U;
    level = voice->params.intensity_q15;
    if (level < 0) level = 0;
    if (level > 32767) level = 32767;
    return (gv89_u16)level;
}

static gsse89_casing_voice *gsse89_find_casing_slot(gsse89_context *ctx,
                                                     const gwv89_context *handler)
{
    gv89_u16 i;
    gsse89_casing_voice *voice;
    for (i = 0U; i < ctx->casing_capacity; ++i) {
        voice = &ctx->casing_voices[i];
        if (!voice->in_use) return voice;
        if (voice->handle.index != GV89_INVALID_INDEX &&
            !gv89_is_handle_active(&handler->voices, voice->handle)) {
            voice->in_use = 0U;
            return voice;
        }
    }
    return 0;
}

static gsse89_fire_voice *gsse89_find_fire_slot(gsse89_context *ctx,
                                                 const gwv89_context *handler)
{
    gv89_u16 i;
    gsse89_fire_voice *voice;
    for (i = 0U; i < ctx->fire_capacity; ++i) {
        voice = &ctx->fire_voices[i];
        if (!voice->in_use) return voice;
        if (voice->handle.index != GV89_INVALID_INDEX &&
            !gv89_is_handle_active(&handler->voices, voice->handle)) {
            voice->in_use = 0U;
            return voice;
        }
    }
    return 0;
}

void gsse89_reset(gsse89_context *ctx, gwv89_context *handler)
{
    gv89_u16 i;
    if (ctx == 0) return;
    for (i = 0U; i < ctx->casing_capacity; ++i) {
        if (handler != 0 && ctx->casing_voices[i].handle.index != GV89_INVALID_INDEX)
            (void)gv89_stop(&handler->voices, ctx->casing_voices[i].handle, 0U);
        gt89_reset(&ctx->casing_voices[i].synth);
        gsse89_zero_casing_voice(&ctx->casing_voices[i]);
    }
    for (i = 0U; i < ctx->fire_capacity; ++i) {
        if (handler != 0 && ctx->fire_voices[i].handle.index != GV89_INVALID_INDEX)
            (void)gv89_stop(&handler->voices, ctx->fire_voices[i].handle, 0U);
        gfire89_set_gate(&ctx->fire_voices[i].synth, 0);
        gsse89_zero_fire_voice(&ctx->fire_voices[i]);
    }
}

gv89_result gsse89_play_casing(gsse89_context *ctx,
                                gwv89_context *handler,
                                const gsse89_casing_params *params,
                                gv89_u32 seed,
                                gv89_handle *out_handle)
{
    gwv89_event_desc desc;
    gv89_reservation reservation;
    gv89_voice_params resolved;
    gv89_provider_ex provider;
    gsse89_casing_voice *voice;
    gt89_impact_params impact;
    gv89_result result;
    if (ctx == 0 || handler == 0 || !gsse89_casing_params_valid(params)) return GV89_BAD_ARGUMENT;
    gwv89_event_default(&desc, GWV89_EVENT_CASING);
    desc.gain_q15 = params->gain_q15;
    desc.screen_x_q15 = params->pan_q15;
    desc.distance_q15 = params->distance_q15;
    desc.occlusion_q15 = params->occlusion_q15;
    desc.focus_q15 = params->focus_q15;
    desc.priority_bias = params->priority_bias;
    desc.instance_key = params->instance_key;
    desc.instance_limit = params->instance_limit;
    desc.attack_ms = 0U;
    desc.release_ms = 10U;
    desc.override_virtual_behavior = 1U;
    desc.virtual_behavior = GV89_VIRTUAL_ADVANCE;
    result = gwv89_reserve_event(handler, &desc, &reservation, &resolved);
    if (result != GV89_OK) return result;
    voice = gsse89_find_casing_slot(ctx, handler);
    if (voice == 0) {
        gwv89_cancel_event(handler, &reservation);
        return GV89_NO_VOICE;
    }
    if (!gt89_init(&voice->synth, ctx->sample_rate, (gt89_u16)(seed ^ (seed >> 16)))) {
        gwv89_cancel_event(handler, &reservation);
        return GV89_BAD_ARGUMENT;
    }
    gt89_default_impact_params(&impact);
    impact.velocity = params->velocity;
    impact.angular_velocity = params->angular_velocity;
    impact.bounce_count = 255U;
    impact.variation = params->variation;
    impact.pan = 0;
    impact.hit_region = GT89_HIT_RANDOM;
    if (gt89_trigger_ex(&voice->synth, params->shell, params->surface, &impact) < 0) {
        gwv89_cancel_event(handler, &reservation);
        return GV89_BAD_ARGUMENT;
    }
    voice->in_use = 1U;
    voice->serial = ++ctx->serial_counter;
    provider.base.user = voice;
    provider.base.process_mono = gsse89_casing_process_cb;
    provider.base.is_active = gsse89_casing_active_cb;
    provider.base.stop = gsse89_casing_stop_cb;
    provider.advance_frames = gsse89_casing_advance_cb;
    provider.restart = 0;
    provider.estimated_level_q15 = gsse89_casing_level_cb;
    provider.physical_state_changed = 0;
    result = gwv89_commit_event_ex(handler, &reservation, &provider, &resolved,
                                   &voice->handle);
    if (result != GV89_OK) {
        voice->in_use = 0U;
        return result;
    }
    if (out_handle != 0) *out_handle = voice->handle;
    return GV89_OK;
}

gv89_result gsse89_play_fire(gsse89_context *ctx,
                              gwv89_context *handler,
                              const gsse89_fire_params *params,
                              gv89_u32 seed,
                              gv89_handle *out_handle)
{
    gwv89_event_desc desc;
    gv89_reservation reservation;
    gv89_voice_params resolved;
    gv89_provider_ex provider;
    gsse89_fire_voice *voice;
    gsse89_fire_params tuned;
    gwv89_event_class event_class;
    gv89_result result;
    gv89_s32 variation;
    gv89_u16 active_flames;
    if (ctx == 0 || handler == 0 || !gsse89_fire_params_valid(params)) return GV89_BAD_ARGUMENT;
    tuned = *params;
    if (tuned.role == GSSE89_FIRE_ROLE_FLAMETHROWER) {
        /* Correlate each burner voice without making duplicate broadband hiss. */
        variation = (gv89_s32)((seed >> 8) & 1023U) - 512;
        tuned.airflow_q15 = gsse89_clamp_q15(tuned.airflow_q15 + variation * 3);
        tuned.crackle_q15 = gsse89_clamp_q15(tuned.crackle_q15 - variation * 2);
        tuned.pressure_q15 = gsse89_clamp_q15(tuned.pressure_q15 + variation);
        tuned.brightness_q15 = gsse89_clamp_q15(tuned.brightness_q15 + variation * 2);
        active_flames = gsse89_active_flamethrowers_internal(ctx);
        if (active_flames != 0U) {
            tuned.gain_q15 = gsse89_mul_s16_q15(tuned.gain_q15, 24500);
            tuned.output_gain_q15 = gsse89_clamp_q15((tuned.output_gain_q15 * 27000) >> 15);
            tuned.airflow_q15 = gsse89_clamp_q15(tuned.airflow_q15 - 2600);
            tuned.brightness_q15 = gsse89_clamp_q15(tuned.brightness_q15 - 2200);
            tuned.crackle_q15 = gsse89_clamp_q15(tuned.crackle_q15 + 1800);
        }
    }
    event_class = tuned.role == GSSE89_FIRE_ROLE_AMBIENCE ?
                  GWV89_EVENT_AMBIENCE : GWV89_EVENT_EXPLOSION;
    gwv89_event_default(&desc, event_class);
    desc.gain_q15 = tuned.gain_q15;
    desc.screen_x_q15 = tuned.pan_q15;
    desc.distance_q15 = tuned.distance_q15;
    desc.occlusion_q15 = tuned.occlusion_q15;
    desc.focus_q15 = tuned.focus_q15;
    desc.priority_bias = tuned.priority_bias;
    desc.instance_key = tuned.instance_key;
    desc.instance_limit = tuned.instance_limit;
    desc.attack_ms = tuned.role == GSSE89_FIRE_ROLE_FLAMETHROWER ? 12U : 45U;
    desc.release_ms = tuned.release_ms;
    desc.override_virtual_behavior = 1U;
    desc.virtual_behavior = tuned.duration_ms == 0U ?
                            GV89_VIRTUAL_PAUSE : GV89_VIRTUAL_RESTART;
    if (tuned.duration_ms == 0U) desc.add_flags |= GV89_FLAG_LOOPING;
    result = gwv89_reserve_event(handler, &desc, &reservation, &resolved);
    if (result != GV89_OK) return result;
    voice = gsse89_find_fire_slot(ctx, handler);
    if (voice == 0) {
        gwv89_cancel_event(handler, &reservation);
        return GV89_NO_VOICE;
    }
    voice->params = tuned;
    voice->seed = seed;
    gfire89_init(&voice->synth, (gfire89_s32)ctx->sample_rate, seed);
    gfire89_set_preset(&voice->synth, tuned.preset_id);
    gfire89_set_controls(&voice->synth,
                         tuned.intensity_q15,
                         tuned.airflow_q15,
                         tuned.crackle_q15,
                         tuned.size_q15);
    gfire89_set_detail(&voice->synth,
                       tuned.pressure_q15,
                       tuned.drive_q15,
                       tuned.brightness_q15);
    gfire89_set_output_gain(&voice->synth, tuned.output_gain_q15);
    gfire89_set_gate(&voice->synth, 1);
    voice->age_frames = 0U;
    voice->gate_closed = 0U;
    if (tuned.duration_ms == 0U) {
        voice->gate_off_frame = 0U;
        voice->total_frames = 0U;
    } else {
        gv89_u32 release_frames;
        voice->gate_off_frame = gsse89_ms_to_frames_saturated(tuned.duration_ms,
                                                              ctx->sample_rate);
        release_frames = gsse89_ms_to_frames_saturated((gv89_u32)tuned.release_ms,
                                                       ctx->sample_rate);
        if (voice->gate_off_frame > 0xFFFFFFFFU - release_frames)
            voice->total_frames = 0xFFFFFFFFU;
        else
            voice->total_frames = voice->gate_off_frame + release_frames;
    }
    voice->in_use = 1U;
    voice->serial = ++ctx->serial_counter;
    provider.base.user = voice;
    provider.base.process_mono = gsse89_fire_process_cb;
    provider.base.is_active = gsse89_fire_active_cb;
    provider.base.stop = gsse89_fire_stop_cb;
    provider.advance_frames = 0;
    provider.restart = gsse89_fire_restart_cb;
    provider.estimated_level_q15 = gsse89_fire_level_cb;
    provider.physical_state_changed = 0;
    result = gwv89_commit_event_ex(handler, &reservation, &provider, &resolved,
                                   &voice->handle);
    if (result != GV89_OK) {
        voice->in_use = 0U;
        return result;
    }
    if (out_handle != 0) *out_handle = voice->handle;
    return GV89_OK;
}

void gsse89_stop_fire(gsse89_context *ctx,
                       gwv89_context *handler,
                       gv89_handle handle,
                       gv89_u16 release_ms)
{
    gv89_u16 i;
    if (ctx == 0 || handler == 0) return;
    for (i = 0U; i < ctx->fire_capacity; ++i) {
        if (ctx->fire_voices[i].handle.index == handle.index &&
            ctx->fire_voices[i].handle.generation == handle.generation) {
            gfire89_set_gate(&ctx->fire_voices[i].synth, 0);
            ctx->fire_voices[i].gate_closed = 1U;
            (void)gv89_stop(&handler->voices, handle, release_ms);
            return;
        }
    }
}

gv89_u16 gsse89_active_casings(const gsse89_context *ctx)
{
    gv89_u16 i;
    gv89_u16 count;
    if (ctx == 0) return 0U;
    count = 0U;
    for (i = 0U; i < ctx->casing_capacity; ++i)
        if (ctx->casing_voices[i].in_use) ++count;
    return count;
}

gv89_u16 gsse89_active_fires(const gsse89_context *ctx)
{
    gv89_u16 i;
    gv89_u16 count;
    if (ctx == 0) return 0U;
    count = 0U;
    for (i = 0U; i < ctx->fire_capacity; ++i)
        if (ctx->fire_voices[i].in_use) ++count;
    return count;
}

gv89_u32 gsse89_casing_voice_bytes(void)
{
    return (gv89_u32)sizeof(gsse89_casing_voice);
}

gv89_u32 gsse89_fire_voice_bytes(void)
{
    return (gv89_u32)sizeof(gsse89_fire_voice);
}

gv89_u32 gsse89_context_bytes(void)
{
    return (gv89_u32)sizeof(gsse89_context);
}
