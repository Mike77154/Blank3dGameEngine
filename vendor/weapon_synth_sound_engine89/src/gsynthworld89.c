#include "gsynthworld89.h"

static gv89_s16 gssw89_sat16(gv89_s32 value)
{
    if (value > 32767) return 32767;
    if (value < -32768) return -32768;
    return (gv89_s16)value;
}

static void gssw89_clear_handle(gv89_handle *handle)
{
    handle->index = GV89_INVALID_INDEX;
    handle->generation = 0U;
}

void gssw89_common_defaults(gssw89_common_params *params)
{
    if (params == 0) return;
    params->pan_q15 = 0;
    params->distance_q15 = 8000U;
    params->occlusion_q15 = 32767U;
    params->focus_q15 = 32767U;
    params->gain_q15 = 26000;
    params->priority_bias = 0;
    params->instance_key = 0U;
    params->instance_limit = 12U;
}

void gssw89_projectile_defaults(gssw89_projectile_params *params)
{
    if (params == 0) return;
    gssw89_common_defaults(&params->common);
    params->mode = WSOUNDPROJECTILE89_NEAR_MISS_SNAP;
    params->amplitude_q15 = 28500U;
    params->proximity_q15 = 26000U;
    params->common.priority_bias = 110;
    params->common.instance_limit = 24U;
}

void gssw89_impact_defaults(gssw89_impact_params *params)
{
    if (params == 0) return;
    gssw89_common_defaults(&params->common);
    params->material = WSOUNDIMPACT89_CONCRETE;
    params->energy_q15 = 27000U;
    params->size_q15 = 18000U;
    params->common.priority_bias = 95;
    params->common.instance_limit = 24U;
}

void gssw89_ricochet_defaults(gssw89_ricochet_params *params)
{
    if (params == 0) return;
    gssw89_common_defaults(&params->common);
    params->energy_q15 = 27000U;
    params->grazing_q15 = 25000U;
    params->roughness_q15 = 10000U;
    params->common.priority_bias = 120;
    params->common.instance_limit = 20U;
}

static int gssw89_handle_dead(const gwv89_context *handler, gv89_handle handle)
{
    if (handle.index == GV89_INVALID_INDEX) return 1;
    return !gv89_is_handle_active(&handler->voices, handle);
}

static void gssw89_zero_projectile(gssw89_projectile_voice *voice)
{
    gssw89_clear_handle(&voice->handle);
    voice->seed = 0U;
    voice->in_use = 0U;
}
static void gssw89_zero_impact(gssw89_impact_voice *voice)
{
    gssw89_clear_handle(&voice->handle);
    voice->seed = 0U;
    voice->in_use = 0U;
}
static void gssw89_zero_ricochet(gssw89_ricochet_voice *voice)
{
    gssw89_clear_handle(&voice->handle);
    voice->seed = 0U;
    voice->sample_rate = 0U;
    voice->in_use = 0U;
}

int gssw89_init(gssw89_context *ctx,
                 gssw89_projectile_voice *projectile_storage,
                 gv89_u16 projectile_capacity,
                 gssw89_impact_voice *impact_storage,
                 gv89_u16 impact_capacity,
                 gssw89_ricochet_voice *ricochet_storage,
                 gv89_u16 ricochet_capacity,
                 gv89_u32 sample_rate,
                 gv89_u32 seed,
                 const gssw89_memory *memory)
{
    gv89_u16 i;
    if (ctx == 0 || memory == 0 || sample_rate == 0U) return 0;
    if (projectile_capacity != 0U && projectile_storage == 0) return 0;
    if (impact_capacity != 0U && impact_storage == 0) return 0;
    if (ricochet_capacity != 0U && ricochet_storage == 0) return 0;
    if (memory->room_delay == 0 || memory->room_delay_samples == 0U) return 0;
    if (memory->prop_delay == 0 || memory->prop_delay_samples == 0U) return 0;
    ctx->projectile_voices = projectile_storage;
    ctx->projectile_capacity = projectile_capacity;
    ctx->impact_voices = impact_storage;
    ctx->impact_capacity = impact_capacity;
    ctx->ricochet_voices = ricochet_storage;
    ctx->ricochet_capacity = ricochet_capacity;
    ctx->sample_rate = sample_rate;
    ctx->world_wet_q15 = 12200;
    ctx->propagation_enabled = 0U;
    ctx->room_enabled = 1U;
    ctx->receiver_enabled = 0U;
    ctx->acoustic_enabled = 1U;
    for (i = 0U; i < projectile_capacity; ++i) gssw89_zero_projectile(&projectile_storage[i]);
    for (i = 0U; i < impact_capacity; ++i) gssw89_zero_impact(&impact_storage[i]);
    for (i = 0U; i < ricochet_capacity; ++i) gssw89_zero_ricochet(&ricochet_storage[i]);
    if (wsounddna89_init(&ctx->dna, seed + 1U) != WSOUND89_OK) return 0;
    if (wsoundreceiver89_init(&ctx->receiver, WSOUNDRECEIVER89_RIFLE_STEEL) != WSOUND89_OK) return 0;
    if (wsoundprop89_init(&ctx->propagation, sample_rate, memory->prop_delay,
                          memory->prop_delay_samples) != WSOUND89_OK) return 0;
    if (wsoundroom89_init(&ctx->room, sample_rate, WSOUNDROOM89_WAREHOUSE,
                          memory->room_delay, memory->room_delay_samples) != WSOUND89_OK) return 0;
    if (wsoundaction89_init(&ctx->action, sample_rate) != WSOUND89_OK) return 0;
    if (wsoundcombatbus89_init(&ctx->combat_bus) != WSOUND89_OK) return 0;
    if (wsounda89_init(&ctx->acoustic, sample_rate, memory->prop_delay,
                       memory->prop_delay_samples, memory->room_delay,
                       memory->room_delay_samples) != WSOUND89_OK) return 0;
    wsounda89_set_profile(&ctx->acoustic, WSOUNDA89_HYBRID);
    return 1;
}

static gssw89_projectile_voice *gssw89_find_projectile(gssw89_context *ctx,
                                                        const gwv89_context *handler)
{
    gv89_u16 i;
    for (i = 0U; i < ctx->projectile_capacity; ++i) {
        if (!ctx->projectile_voices[i].in_use) return &ctx->projectile_voices[i];
        if (gssw89_handle_dead(handler, ctx->projectile_voices[i].handle)) {
            ctx->projectile_voices[i].in_use = 0U;
            return &ctx->projectile_voices[i];
        }
    }
    return 0;
}
static gssw89_impact_voice *gssw89_find_impact(gssw89_context *ctx,
                                                const gwv89_context *handler)
{
    gv89_u16 i;
    for (i = 0U; i < ctx->impact_capacity; ++i) {
        if (!ctx->impact_voices[i].in_use) return &ctx->impact_voices[i];
        if (gssw89_handle_dead(handler, ctx->impact_voices[i].handle)) {
            ctx->impact_voices[i].in_use = 0U;
            return &ctx->impact_voices[i];
        }
    }
    return 0;
}
static gssw89_ricochet_voice *gssw89_find_ricochet(gssw89_context *ctx,
                                                    const gwv89_context *handler)
{
    gv89_u16 i;
    for (i = 0U; i < ctx->ricochet_capacity; ++i) {
        if (!ctx->ricochet_voices[i].in_use) return &ctx->ricochet_voices[i];
        if (gssw89_handle_dead(handler, ctx->ricochet_voices[i].handle)) {
            ctx->ricochet_voices[i].in_use = 0U;
            return &ctx->ricochet_voices[i];
        }
    }
    return 0;
}

static gv89_s16 gssw89_projectile_process(void *user)
{
    gssw89_projectile_voice *voice;
    voice = (gssw89_projectile_voice *)user;
    if (voice == 0 || !voice->in_use) return 0;
    if (!wsoundprojectile89_is_active(&voice->synth)) {
        voice->in_use = 0U;
        return 0;
    }
    return (gv89_s16)wsoundprojectile89_process_sample(&voice->synth);
}
static int gssw89_projectile_active(const void *user)
{
    const gssw89_projectile_voice *voice;
    voice = (const gssw89_projectile_voice *)user;
    return voice != 0 && voice->in_use && wsoundprojectile89_is_active(&voice->synth);
}
static void gssw89_projectile_stop(void *user)
{
    gssw89_projectile_voice *voice;
    voice = (gssw89_projectile_voice *)user;
    if (voice != 0) voice->in_use = 0U;
}
static void gssw89_projectile_restart(void *user)
{
    gssw89_projectile_voice *voice;
    voice = (gssw89_projectile_voice *)user;
    if (voice == 0) return;
    (void)wsoundprojectile89_init(&voice->synth, voice->synth.sample_rate, voice->seed);
    (void)wsoundprojectile89_trigger(&voice->synth, voice->params.mode,
                                      voice->params.amplitude_q15,
                                      voice->params.proximity_q15);
    voice->in_use = 1U;
}

static gv89_s16 gssw89_impact_process(void *user)
{
    gssw89_impact_voice *voice;
    voice = (gssw89_impact_voice *)user;
    if (voice == 0 || !voice->in_use) return 0;
    if (!wsoundimpact89_is_active(&voice->synth)) {
        voice->in_use = 0U;
        return 0;
    }
    return (gv89_s16)wsoundimpact89_process_sample(&voice->synth);
}
static int gssw89_impact_active(const void *user)
{
    const gssw89_impact_voice *voice;
    voice = (const gssw89_impact_voice *)user;
    return voice != 0 && voice->in_use && wsoundimpact89_is_active(&voice->synth);
}
static void gssw89_impact_stop(void *user)
{
    gssw89_impact_voice *voice;
    voice = (gssw89_impact_voice *)user;
    if (voice != 0) voice->in_use = 0U;
}
static void gssw89_impact_restart(void *user)
{
    gssw89_impact_voice *voice;
    voice = (gssw89_impact_voice *)user;
    if (voice == 0) return;
    (void)wsoundimpact89_init(&voice->synth, voice->seed);
    (void)wsoundimpact89_trigger(&voice->synth, voice->params.material,
                                  voice->params.energy_q15,
                                  voice->params.size_q15);
    voice->in_use = 1U;
}

static gv89_s16 gssw89_ricochet_process(void *user)
{
    gssw89_ricochet_voice *voice;
    voice = (gssw89_ricochet_voice *)user;
    if (voice == 0 || !voice->in_use) return 0;
    if (!wsoundricochet89_is_active(&voice->synth)) {
        voice->in_use = 0U;
        return 0;
    }
    return (gv89_s16)wsoundricochet89_process_sample(&voice->synth);
}
static int gssw89_ricochet_active(const void *user)
{
    const gssw89_ricochet_voice *voice;
    voice = (const gssw89_ricochet_voice *)user;
    return voice != 0 && voice->in_use && wsoundricochet89_is_active(&voice->synth);
}
static void gssw89_ricochet_stop(void *user)
{
    gssw89_ricochet_voice *voice;
    voice = (gssw89_ricochet_voice *)user;
    if (voice != 0) voice->in_use = 0U;
}
static void gssw89_ricochet_restart(void *user)
{
    gssw89_ricochet_voice *voice;
    voice = (gssw89_ricochet_voice *)user;
    if (voice == 0) return;
    (void)wsoundricochet89_init(&voice->synth, voice->seed);
    (void)wsoundricochet89_trigger(&voice->synth, voice->params.energy_q15,
                                    voice->params.grazing_q15,
                                    voice->params.roughness_q15,
                                    voice->sample_rate);
    voice->in_use = 1U;
}

static void gssw89_advance_generic(void *user, gv89_u32 frames,
                                   gv89_s16 (*process_fn)(void *))
{
    gv89_u32 i;
    for (i = 0U; i < frames; ++i) (void)process_fn(user);
}
static void gssw89_projectile_advance(void *user, gv89_u32 frames)
{ gssw89_advance_generic(user, frames, gssw89_projectile_process); }
static void gssw89_impact_advance(void *user, gv89_u32 frames)
{ gssw89_advance_generic(user, frames, gssw89_impact_process); }
static void gssw89_ricochet_advance(void *user, gv89_u32 frames)
{ gssw89_advance_generic(user, frames, gssw89_ricochet_process); }
static gv89_u16 gssw89_level(const void *user)
{ return user == 0 ? 0U : 27000U; }

static gv89_result gssw89_commit(gwv89_context *handler,
                                  gwv89_event_class event_class,
                                  const gssw89_common_params *common,
                                  void *user,
                                  gv89_process_mono_fn process,
                                  gv89_is_active_fn active,
                                  gv89_stop_fn stop,
                                  gv89_advance_frames_fn advance,
                                  gv89_restart_fn restart,
                                  gv89_handle *out_handle)
{
    gwv89_event_desc desc;
    gv89_provider_ex provider;
    gwv89_event_default(&desc, event_class);
    desc.gain_q15 = common->gain_q15;
    desc.screen_x_q15 = common->pan_q15;
    desc.distance_q15 = common->distance_q15;
    desc.occlusion_q15 = common->occlusion_q15;
    desc.focus_q15 = common->focus_q15;
    desc.priority_bias = common->priority_bias;
    desc.instance_key = common->instance_key;
    desc.instance_limit = common->instance_limit;
    provider.base.user = user;
    provider.base.process_mono = process;
    provider.base.is_active = active;
    provider.base.stop = stop;
    provider.advance_frames = advance;
    provider.restart = restart;
    provider.estimated_level_q15 = gssw89_level;
    provider.physical_state_changed = 0;
    return gwv89_play_ex(handler, &desc, &provider, out_handle);
}

gv89_result gssw89_play_projectile(gssw89_context *ctx,
                                    gwv89_context *handler,
                                    const gssw89_projectile_params *params,
                                    gv89_u32 seed,
                                    gv89_handle *out_handle)
{
    gssw89_projectile_voice *voice;
    gv89_result result;
    if (ctx == 0 || handler == 0 || params == 0 || out_handle == 0) return GV89_BAD_ARGUMENT;
    voice = gssw89_find_projectile(ctx, handler);
    if (voice == 0) return GV89_NO_VOICE;
    voice->params = *params;
    voice->seed = seed;
    if (wsoundprojectile89_init(&voice->synth, ctx->sample_rate, seed) != WSOUND89_OK) return GV89_BAD_ARGUMENT;
    if (wsoundprojectile89_trigger(&voice->synth, params->mode, params->amplitude_q15,
                                    params->proximity_q15) != WSOUND89_OK) return GV89_BAD_ARGUMENT;
    voice->in_use = 1U;
    result = gssw89_commit(handler, GWV89_EVENT_RICOCHET, &params->common,
                           voice, gssw89_projectile_process,
                           gssw89_projectile_active, gssw89_projectile_stop,
                           gssw89_projectile_advance, gssw89_projectile_restart,
                           out_handle);
    if (result != GV89_OK) voice->in_use = 0U;
    else voice->handle = *out_handle;
    return result;
}

gv89_result gssw89_play_impact(gssw89_context *ctx,
                                gwv89_context *handler,
                                const gssw89_impact_params *params,
                                gv89_u32 seed,
                                gv89_handle *out_handle)
{
    gssw89_impact_voice *voice;
    gv89_result result;
    if (ctx == 0 || handler == 0 || params == 0 || out_handle == 0) return GV89_BAD_ARGUMENT;
    voice = gssw89_find_impact(ctx, handler);
    if (voice == 0) return GV89_NO_VOICE;
    voice->params = *params;
    voice->seed = seed;
    if (wsoundimpact89_init(&voice->synth, seed) != WSOUND89_OK) return GV89_BAD_ARGUMENT;
    if (wsoundimpact89_trigger(&voice->synth, params->material, params->energy_q15,
                                params->size_q15) != WSOUND89_OK) return GV89_BAD_ARGUMENT;
    voice->in_use = 1U;
    result = gssw89_commit(handler, GWV89_EVENT_IMPACT, &params->common,
                           voice, gssw89_impact_process, gssw89_impact_active,
                           gssw89_impact_stop, gssw89_impact_advance,
                           gssw89_impact_restart, out_handle);
    if (result != GV89_OK) voice->in_use = 0U;
    else voice->handle = *out_handle;
    return result;
}

gv89_result gssw89_play_ricochet(gssw89_context *ctx,
                                  gwv89_context *handler,
                                  const gssw89_ricochet_params *params,
                                  gv89_u32 seed,
                                  gv89_handle *out_handle)
{
    gssw89_ricochet_voice *voice;
    gv89_result result;
    if (ctx == 0 || handler == 0 || params == 0 || out_handle == 0) return GV89_BAD_ARGUMENT;
    voice = gssw89_find_ricochet(ctx, handler);
    if (voice == 0) return GV89_NO_VOICE;
    voice->params = *params;
    voice->seed = seed;
    voice->sample_rate = ctx->sample_rate;
    if (wsoundricochet89_init(&voice->synth, seed) != WSOUND89_OK) return GV89_BAD_ARGUMENT;
    if (wsoundricochet89_trigger(&voice->synth, params->energy_q15,
                                  params->grazing_q15, params->roughness_q15,
                                  ctx->sample_rate) != WSOUND89_OK) return GV89_BAD_ARGUMENT;
    voice->in_use = 1U;
    result = gssw89_commit(handler, GWV89_EVENT_RICOCHET, &params->common,
                           voice, gssw89_ricochet_process,
                           gssw89_ricochet_active, gssw89_ricochet_stop,
                           gssw89_ricochet_advance, gssw89_ricochet_restart,
                           out_handle);
    if (result != GV89_OK) voice->in_use = 0U;
    else voice->handle = *out_handle;
    return result;
}

wsound89_result gssw89_set_room(gssw89_context *ctx,
                                wsoundroom89_preset preset,
                                wsound89_i16 *memory,
                                wsound89_u32 memory_samples)
{
    if (ctx == 0) return WSOUND89_EINVAL;
    return wsoundroom89_init(&ctx->room, ctx->sample_rate, preset, memory,
                             memory_samples);
}

wsound89_result gssw89_set_propagation(gssw89_context *ctx,
                                       wsound89_u32 distance_cm,
                                       wsound89_u32 speed_cm_s,
                                       wsound89_i16 muzzle_dot_q15,
                                       wsound89_u16 occlusion_q15)
{
    if (ctx == 0) return WSOUND89_EINVAL;
    return wsoundprop89_set_path(&ctx->propagation, distance_cm, speed_cm_s,
                                 muzzle_dot_q15, occlusion_q15);
}

void gssw89_enable_post(gssw89_context *ctx, int propagation,
                        int room, int receiver)
{
    if (ctx == 0) return;
    ctx->propagation_enabled = propagation ? 1U : 0U;
    ctx->room_enabled = room ? 1U : 0U;
    ctx->receiver_enabled = receiver ? 1U : 0U;
}
void gssw89_set_world_wet(gssw89_context *ctx, gv89_s16 wet_q15)
{ if (ctx != 0) ctx->world_wet_q15 = wet_q15; }

void gssw89_enable_acoustic(gssw89_context *ctx, int enabled)
{
    if (ctx == 0) return;
    ctx->acoustic_enabled = enabled ? 1U : 0U;
    wsounda89_set_enabled(&ctx->acoustic, enabled);
}

void gssw89_set_acoustic_profile(gssw89_context *ctx, wsounda89_profile profile)
{
    if (ctx != 0) wsounda89_set_profile(&ctx->acoustic, profile);
}

wsound89_result gssw89_set_acoustic_path(gssw89_context *ctx,
                                         const wsounda89_path_params *params)
{
    if (ctx == 0) return WSOUND89_EINVAL;
    return wsounda89_set_path(&ctx->acoustic, params);
}

wsound89_result gssw89_set_acoustic_material(gssw89_context *ctx,
                                             wsounda89_material material,
                                             wsound89_u16 thickness_q15)
{
    if (ctx == 0) return WSOUND89_EINVAL;
    return wsounda89_set_material(&ctx->acoustic, material, thickness_q15);
}

wsound89_result gssw89_set_acoustic_space(gssw89_context *ctx,
                                          wsounda89_space space)
{
    if (ctx == 0) return WSOUND89_EINVAL;
    return wsounda89_set_space(&ctx->acoustic, space);
}

wsound89_result gssw89_set_acoustic_portal(gssw89_context *ctx,
                                           const wsounda89_portal_params *params)
{
    if (ctx == 0) return WSOUND89_EINVAL;
    return wsounda89_set_portal(&ctx->acoustic, params);
}

void gssw89_trigger_pressure(gssw89_context *ctx,
                             wsounda89_pressure_kind kind,
                             wsound89_u16 energy_q15,
                             wsound89_i16 pan_q15,
                             wsound89_u32 seed)
{
    if (ctx != 0) wsounda89_trigger_pressure(&ctx->acoustic, kind, energy_q15, pan_q15, seed);
}

void gssw89_process_post_stereo(gssw89_context *ctx,
                                gv89_s16 in_left, gv89_s16 in_right,
                                gv89_s16 *out_left, gv89_s16 *out_right)
{
    wsound89_i16 mono;
    wsound89_i16 processed;
    wsound89_i16 room_l;
    wsound89_i16 room_r;
    gv89_s32 l;
    gv89_s32 r;
    if (out_left == 0 || out_right == 0) return;
    if (ctx == 0) { *out_left = in_left; *out_right = in_right; return; }
    if (ctx->acoustic_enabled) {
        wsounda89_process_stereo(&ctx->acoustic, in_left, in_right, out_left, out_right);
        return;
    }
    mono = (wsound89_i16)(((gv89_s32)in_left + (gv89_s32)in_right) / 2);
    processed = mono;
    if (ctx->receiver_enabled) processed = wsoundreceiver89_process_sample(&ctx->receiver, processed);
    if (ctx->propagation_enabled) processed = wsoundprop89_process_sample(&ctx->propagation, processed);
    if (ctx->room_enabled) wsoundroom89_process_sample(&ctx->room, processed, &room_l, &room_r);
    else { room_l = processed; room_r = processed; }
    l = (gv89_s32)in_left + (((gv89_s32)room_l * ctx->world_wet_q15) >> 15);
    r = (gv89_s32)in_right + (((gv89_s32)room_r * ctx->world_wet_q15) >> 15);
    *out_left = gssw89_sat16(l);
    *out_right = gssw89_sat16(r);
}

wsound89_result gssw89_action_trigger(gssw89_context *ctx,
                                      wsoundaction89_type type,
                                      wsound89_u32 speed_q16)
{ return ctx == 0 ? WSOUND89_EINVAL : wsoundaction89_trigger(&ctx->action, type, speed_q16); }
wsound89_result gssw89_action_advance(gssw89_context *ctx,
                                      wsound89_u32 frames,
                                      wsoundaction89_event *events,
                                      wsound89_u16 capacity,
                                      wsound89_u16 *written)
{ return ctx == 0 ? WSOUND89_EINVAL : wsoundaction89_advance(&ctx->action, frames, events, capacity, written); }
wsound89_result gssw89_dna_set_profile(gssw89_context *ctx,
                                       const wsounddna89_profile *profile)
{
    return ctx == 0 ? WSOUND89_EINVAL :
           wsounddna89_set_profile(&ctx->dna, profile);
}

wsound89_result gssw89_dna_set_mode(gssw89_context *ctx,
                                    wsounddna89_mode mode)
{
    return ctx == 0 ? WSOUND89_EINVAL :
           wsounddna89_set_mode(&ctx->dna, mode);
}

wsound89_result gssw89_dna_next_profiled(gssw89_context *ctx,
                                         wsounddna89_shot *out_shot)
{
    return ctx == 0 ? WSOUND89_EINVAL :
           wsounddna89_next_profiled(&ctx->dna, out_shot);
}

wsound89_result gssw89_dna_next(gssw89_context *ctx,
                                wsounddna89_class weapon_class,
                                wsound89_u16 base_energy_q15,
                                wsounddna89_shot *out_shot)
{ return ctx == 0 ? WSOUND89_EINVAL : wsounddna89_next(&ctx->dna, weapon_class, base_energy_q15, out_shot); }
void gssw89_set_receiver(gssw89_context *ctx, wsoundreceiver89_preset preset)
{ if (ctx != 0) (void)wsoundreceiver89_init(&ctx->receiver, preset); }
void gssw89_excite_receiver(gssw89_context *ctx, wsound89_i16 impulse)
{ if (ctx != 0) wsoundreceiver89_excite(&ctx->receiver, impulse); }

void gssw89_reset(gssw89_context *ctx, gwv89_context *handler)
{
    gv89_u16 i;
    if (ctx == 0) return;
    for (i = 0U; i < ctx->projectile_capacity; ++i) {
        if (ctx->projectile_voices[i].in_use && handler != 0)
            (void)gv89_stop(&handler->voices, ctx->projectile_voices[i].handle, 0U);
        gssw89_zero_projectile(&ctx->projectile_voices[i]);
    }
    for (i = 0U; i < ctx->impact_capacity; ++i) {
        if (ctx->impact_voices[i].in_use && handler != 0)
            (void)gv89_stop(&handler->voices, ctx->impact_voices[i].handle, 0U);
        gssw89_zero_impact(&ctx->impact_voices[i]);
    }
    for (i = 0U; i < ctx->ricochet_capacity; ++i) {
        if (ctx->ricochet_voices[i].in_use && handler != 0)
            (void)gv89_stop(&handler->voices, ctx->ricochet_voices[i].handle, 0U);
        gssw89_zero_ricochet(&ctx->ricochet_voices[i]);
    }
    wsoundprop89_reset(&ctx->propagation);
    wsoundroom89_reset(&ctx->room);
    wsoundreceiver89_reset(&ctx->receiver);
    wsounda89_reset(&ctx->acoustic);
}

#define GSSW89_COUNT_FN(name, field, capacity_field) \
gv89_u16 name(const gssw89_context *ctx) \
{ gv89_u16 i, count = 0U; if (ctx == 0) return 0U; \
  for (i = 0U; i < ctx->capacity_field; ++i) if (ctx->field[i].in_use) ++count; \
  return count; }
GSSW89_COUNT_FN(gssw89_active_projectiles, projectile_voices, projectile_capacity)
GSSW89_COUNT_FN(gssw89_active_impacts, impact_voices, impact_capacity)
GSSW89_COUNT_FN(gssw89_active_ricochets, ricochet_voices, ricochet_capacity)

gv89_u32 gssw89_context_bytes(void) { return (gv89_u32)sizeof(gssw89_context); }
gv89_u32 gssw89_projectile_voice_bytes(void) { return (gv89_u32)sizeof(gssw89_projectile_voice); }
gv89_u32 gssw89_impact_voice_bytes(void) { return (gv89_u32)sizeof(gssw89_impact_voice); }
gv89_u32 gssw89_ricochet_voice_bytes(void) { return (gv89_u32)sizeof(gssw89_ricochet_voice); }
