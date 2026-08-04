#include "gweaponvoice89.h"

static gv89_s16 gwv89_mul_q15(gv89_s32 a, gv89_s32 b)
{
    gv89_s32 value;
    value = (a * b) >> 15;
    if (value < 0) return 0;
    if (value > 32767) return 32767;
    return (gv89_s16)value;
}

static gv89_u16 gwv89_clamp_priority(gv89_s32 value)
{
    if (value < 0) return 0U;
    if (value > 65535) return 65535U;
    return (gv89_u16)value;
}

static gv89_s16 gwv89_clamp_pan(gv89_s32 value)
{
    if (value < -32767) return -32767;
    if (value > 32767) return 32767;
    return (gv89_s16)value;
}

static gv89_u16 gwv89_distance_gain(gv89_u16 distance_q15)
{
    gv89_s32 attenuation;
    attenuation = 32767 - (((gv89_s32)distance_q15 * 28672) >> 15);
    if (attenuation < 4095) attenuation = 4095;
    return (gv89_u16)attenuation;
}

static gv89_u16 gwv89_audibility(gv89_u16 distance_q15,
                                  gv89_u16 occlusion_q15,
                                  gv89_u16 focus_q15)
{
    gv89_s16 value;
    value = (gv89_s16)gwv89_distance_gain(distance_q15);
    value = gwv89_mul_q15(value, occlusion_q15);
    value = gwv89_mul_q15(value, focus_q15);
    return (gv89_u16)value;
}

void gwv89_event_default(gwv89_event_desc *desc,
                          gwv89_event_class event_class)
{
    if (desc == 0) return;
    desc->event_class = event_class;
    desc->required_capabilities = 0U;
    desc->gain_q15 = 32767;
    desc->screen_x_q15 = 0;
    desc->distance_q15 = 0U;
    desc->occlusion_q15 = 32767U;
    desc->focus_q15 = 32767U;
    desc->priority_bias = 0;
    desc->instance_key = 0U;
    desc->instance_limit = 0U;
    desc->attack_ms = 1U;
    desc->release_ms = 8U;
    desc->add_flags = 0U;
    desc->override_virtual_behavior = 0U;
    desc->virtual_behavior = GV89_VIRTUAL_ADVANCE;
    desc->allow_higher_priority_steal = 0U;
    desc->allow_protected_steal = 0U;
}

static void gwv89_set_default_class(gwv89_class_config *config,
                                     gv89_u16 group_id,
                                     gv89_u16 priority,
                                     gv89_u16 max_logical,
                                     gv89_u16 reserve,
                                     gv89_s16 gain_q15,
                                     gv89_u16 distance_penalty,
                                     gv89_u16 instance_limit,
                                     gv89_u16 protect_ms,
                                     gv89_u16 physical_ms,
                                     gv89_u32 timeout_ms,
                                     gv89_steal_policy steal_policy,
                                     gv89_virtual_behavior virtual_behavior,
                                     gv89_u32 flags,
                                     gv89_u16 bus_id)
{
    config->group_id = group_id;
    config->priority = priority;
    config->max_logical = max_logical;
    config->physical_reserve = reserve;
    config->gain_q15 = gain_q15;
    config->distance_priority_penalty = distance_penalty;
    config->default_instance_limit = instance_limit;
    config->steal_protect_ms = protect_ms;
    config->minimum_physical_ms = physical_ms;
    config->virtual_timeout_ms = timeout_ms;
    config->steal_policy = steal_policy;
    config->virtual_behavior = virtual_behavior;
    config->flags = flags;
    config->bus_id = bus_id;
}

static void gwv89_default_classes(gwv89_context *ctx)
{
    gwv89_set_default_class(&ctx->classes[GWV89_EVENT_REPORT],
        GWV89_GROUP_REPORT, 560U, 96U, 24U, 32767, 160U, 12U,
        24U, 24U, 1800U, GV89_STEAL_HYBRID, GV89_VIRTUAL_ADVANCE,
        GV89_FLAG_NONE, 1U);
    gwv89_set_default_class(&ctx->classes[GWV89_EVENT_MECHANISM],
        GWV89_GROUP_MECHANISM, 480U, 40U, 8U, 20800, 80U, 4U,
        50U, 40U, 2500U, GV89_STEAL_OLDEST, GV89_VIRTUAL_PAUSE,
        GV89_FLAG_NONE, 2U);
    gwv89_set_default_class(&ctx->classes[GWV89_EVENT_FOLEY],
        GWV89_GROUP_FOLEY, 260U, 48U, 2U, 18800, 180U, 8U,
        8U, 12U, 700U, GV89_STEAL_QUIETEST, GV89_VIRTUAL_KILL,
        GV89_FLAG_NONE, 3U);
    gwv89_set_default_class(&ctx->classes[GWV89_EVENT_CASING],
        GWV89_GROUP_CASING, 110U, 96U, 0U, 17200, 240U, 16U,
        0U, 0U, 500U, GV89_STEAL_QUIETEST, GV89_VIRTUAL_KILL,
        GV89_FLAG_NONE, 4U);
    gwv89_set_default_class(&ctx->classes[GWV89_EVENT_RICOCHET],
        GWV89_GROUP_RICOCHET, 330U, 64U, 4U, 25000, 200U, 8U,
        10U, 14U, 1200U, GV89_STEAL_HYBRID, GV89_VIRTUAL_ADVANCE,
        GV89_FLAG_NONE, 5U);
    gwv89_set_default_class(&ctx->classes[GWV89_EVENT_IMPACT],
        GWV89_GROUP_IMPACT, 400U, 96U, 12U, 28200, 180U, 12U,
        12U, 18U, 1500U, GV89_STEAL_HYBRID, GV89_VIRTUAL_ADVANCE,
        GV89_FLAG_NONE, 6U);
    gwv89_set_default_class(&ctx->classes[GWV89_EVENT_EXPLOSION],
        GWV89_GROUP_EXPLOSION, 650U, 24U, 8U, 32767, 100U, 6U,
        80U, 100U, 5000U, GV89_STEAL_PRIORITY_AUDIBILITY,
        GV89_VIRTUAL_CONTINUE, GV89_FLAG_NONE, 7U);
    gwv89_set_default_class(&ctx->classes[GWV89_EVENT_AMBIENCE],
        GWV89_GROUP_AMBIENCE, 80U, 32U, 0U, 16800, 120U, 4U,
        0U, 0U, 30000U, GV89_STEAL_QUIETEST, GV89_VIRTUAL_ADVANCE,
        GV89_FLAG_LOOPING, 8U);
}

static gv89_s16 gwv89_profile_gain(gwv89_mix_profile profile,
                                       gwv89_event_class event_class)
{
    static const gv89_s16 gains[GWV89_MIX_PROFILE_COUNT][GWV89_EVENT_CLASS_COUNT] = {
        { 32767, 17500, 15500, 14500, 22500, 25500, 32767, 15500 },
        { 32767, 20800, 18800, 17200, 25000, 28200, 32767, 16800 },
        { 32767, 22000, 19800, 17800, 27000, 30000, 32767, 16200 }
    };
    return gains[(int)profile][(int)event_class];
}

gv89_result gwv89_apply_mix_profile(gwv89_context *ctx,
                                      gwv89_mix_profile profile)
{
    gv89_u16 i;
    gv89_result result;
    gv89_s16 master_gain;
    gv89_s16 limiter_threshold;
    if (ctx == 0 || (int)profile < 0 || profile >= GWV89_MIX_PROFILE_COUNT)
        return GV89_BAD_ARGUMENT;
    for (i = 0U; i < GWV89_EVENT_CLASS_COUNT; ++i) {
        ctx->classes[i].gain_q15 = gwv89_profile_gain(
            profile, (gwv89_event_class)i);
        result = gv89_set_bus(&ctx->voices, ctx->classes[i].bus_id,
                              32767, 0, 0);
        if (result != GV89_OK) return result;
    }
    master_gain = profile == GWV89_MIX_REALISTIC ? 30000 :
                  (profile == GWV89_MIX_HYBRID ? 29200 : 28200);
    limiter_threshold = profile == GWV89_MIX_CINEMATIC ? 29600 : 30200;
    gv89_set_master(&ctx->voices, master_gain, limiter_threshold, 5U);
    return GV89_OK;
}

gv89_result gwv89_init_ex(gwv89_context *ctx,
                            gv89_voice *voice_storage,
                            gv89_u16 logical_capacity,
                            gv89_u16 physical_limit,
                            gv89_u32 sample_rate)
{
    gv89_u16 i;
    gv89_result result;
    if (ctx == 0) return GV89_BAD_ARGUMENT;
    result = gv89_init(&ctx->voices, voice_storage, logical_capacity,
                       sample_rate);
    if (result != GV89_OK) return result;
    result = gv89_set_physical_limit(&ctx->voices, physical_limit);
    if (result != GV89_OK) return result;
    gwv89_default_classes(ctx);
    for (i = 0U; i < GWV89_EVENT_CLASS_COUNT; ++i) {
        result = gv89_set_group_rule(&ctx->voices,
            ctx->classes[i].group_id,
            ctx->classes[i].max_logical,
            ctx->classes[i].physical_reserve,
            ctx->classes[i].steal_policy);
        if (result != GV89_OK) return result;
        result = gv89_set_bus(&ctx->voices, ctx->classes[i].bus_id,
                              32767, 0, 0);
        if (result != GV89_OK) return result;
    }
    return gwv89_apply_mix_profile(ctx, GWV89_MIX_HYBRID);
}

gv89_result gwv89_init(gwv89_context *ctx,
                         gv89_voice *voice_storage,
                         gv89_u16 logical_capacity,
                         gv89_u32 sample_rate)
{
    gv89_u16 physical_limit;
    physical_limit = logical_capacity;
    if (physical_limit > GV89_RECOMMENDED_PHYSICAL_VOICES) {
        physical_limit = GV89_RECOMMENDED_PHYSICAL_VOICES;
    }
    return gwv89_init_ex(ctx, voice_storage, logical_capacity,
                         physical_limit, sample_rate);
}

gv89_result gwv89_set_class(gwv89_context *ctx,
                              gwv89_event_class event_class,
                              const gwv89_class_config *config)
{
    gv89_result result;
    if (ctx == 0 || config == 0 || event_class < 0
        || event_class >= GWV89_EVENT_CLASS_COUNT
        || config->bus_id >= GV89_MAX_BUSES) return GV89_BAD_ARGUMENT;
    ctx->classes[event_class] = *config;
    result = gv89_set_group_rule(&ctx->voices, config->group_id,
                                 config->max_logical,
                                 config->physical_reserve,
                                 config->steal_policy);
    if (result != GV89_OK) return result;
    return gv89_set_bus(&ctx->voices, config->bus_id, 32767, 0, 0);
}

gv89_result gwv89_set_physical_limit(gwv89_context *ctx,
                                       gv89_u16 physical_limit)
{
    if (ctx == 0) return GV89_BAD_ARGUMENT;
    return gv89_set_physical_limit(&ctx->voices, physical_limit);
}

void gwv89_set_slot_capabilities(gwv89_context *ctx,
                                  gv89_u16 index,
                                  gv89_u32 capabilities)
{
    if (ctx == 0) return;
    gv89_set_slot_capabilities(&ctx->voices, index, capabilities);
}

void gwv89_begin_batch(gwv89_context *ctx)
{
    if (ctx == 0) return;
    gv89_begin_batch(&ctx->voices);
}

gv89_result gwv89_end_batch(gwv89_context *ctx)
{
    if (ctx == 0) return GV89_BAD_ARGUMENT;
    return gv89_end_batch(&ctx->voices);
}

gv89_result gwv89_reserve_event(gwv89_context *ctx,
                                  const gwv89_event_desc *desc,
                                  gv89_reservation *reservation,
                                  gv89_voice_params *resolved_params)
{
    const gwv89_class_config *class_config;
    gv89_request request;
    gv89_s32 priority;
    gv89_u16 penalty;
    gv89_s16 gain;
    if (ctx == 0 || desc == 0 || reservation == 0
        || resolved_params == 0 || desc->event_class < 0
        || desc->event_class >= GWV89_EVENT_CLASS_COUNT) {
        return GV89_BAD_ARGUMENT;
    }
    class_config = &ctx->classes[desc->event_class];
    gv89_request_default(&request);
    penalty = (gv89_u16)(((gv89_u32)desc->distance_q15
              * class_config->distance_priority_penalty) >> 15);
    priority = (gv89_s32)class_config->priority + desc->priority_bias
             - penalty;
    request.priority = gwv89_clamp_priority(priority);
    request.group_id = class_config->group_id;
    request.required_capabilities = desc->required_capabilities;
    request.steal_policy = class_config->steal_policy;
    request.virtual_behavior = desc->override_virtual_behavior
        ? desc->virtual_behavior : class_config->virtual_behavior;
    request.flags = class_config->flags | desc->add_flags;
    request.instance_key = desc->instance_key;
    request.instance_limit = desc->instance_limit > 0U
        ? desc->instance_limit : class_config->default_instance_limit;
    request.bus_id = class_config->bus_id;
    request.steal_protect_ms = class_config->steal_protect_ms;
    request.minimum_physical_ms = class_config->minimum_physical_ms;
    request.virtual_timeout_ms = class_config->virtual_timeout_ms;
    request.allow_higher_priority_steal = desc->allow_higher_priority_steal;
    request.allow_protected_steal = desc->allow_protected_steal;

    gain = gwv89_mul_q15(class_config->gain_q15, desc->gain_q15);
    resolved_params->gain_q15 = gain;
    resolved_params->pan_q15 = gwv89_clamp_pan(desc->screen_x_q15);
    resolved_params->audibility_q15 = gwv89_audibility(
        desc->distance_q15, desc->occlusion_q15, desc->focus_q15);
    resolved_params->attack_ms = desc->attack_ms;
    resolved_params->release_ms = desc->release_ms;
    return gv89_reserve(&ctx->voices, &request, reservation);
}

gv89_result gwv89_commit_event(gwv89_context *ctx,
                                 const gv89_reservation *reservation,
                                 const gv89_provider *provider,
                                 const gv89_voice_params *resolved_params,
                                 gv89_handle *handle)
{
    if (ctx == 0) return GV89_BAD_ARGUMENT;
    return gv89_commit(&ctx->voices, reservation, provider,
                       resolved_params, handle);
}

gv89_result gwv89_commit_event_ex(gwv89_context *ctx,
                                    const gv89_reservation *reservation,
                                    const gv89_provider_ex *provider,
                                    const gv89_voice_params *resolved_params,
                                    gv89_handle *handle)
{
    if (ctx == 0) return GV89_BAD_ARGUMENT;
    return gv89_commit_ex(&ctx->voices, reservation, provider,
                          resolved_params, handle);
}

gv89_result gwv89_play(gwv89_context *ctx,
                         const gwv89_event_desc *desc,
                         const gv89_provider *provider,
                         gv89_handle *handle)
{
    gv89_reservation reservation;
    gv89_voice_params params;
    gv89_result result;
    result = gwv89_reserve_event(ctx, desc, &reservation, &params);
    if (result != GV89_OK) return result;
    result = gwv89_commit_event(ctx, &reservation, provider, &params, handle);
    if (result != GV89_OK) gwv89_cancel_event(ctx, &reservation);
    return result;
}

gv89_result gwv89_play_ex(gwv89_context *ctx,
                            const gwv89_event_desc *desc,
                            const gv89_provider_ex *provider,
                            gv89_handle *handle)
{
    gv89_reservation reservation;
    gv89_voice_params params;
    gv89_result result;
    result = gwv89_reserve_event(ctx, desc, &reservation, &params);
    if (result != GV89_OK) return result;
    result = gwv89_commit_event_ex(ctx, &reservation, provider,
                                   &params, handle);
    if (result != GV89_OK) gwv89_cancel_event(ctx, &reservation);
    return result;
}

void gwv89_cancel_event(gwv89_context *ctx,
                         const gv89_reservation *reservation)
{
    if (ctx == 0) return;
    gv89_cancel_reservation(&ctx->voices, reservation);
}

gv89_result gwv89_set_event_spatial(gwv89_context *ctx,
                                      gv89_handle handle,
                                      gv89_s16 screen_x_q15,
                                      gv89_u16 distance_q15,
                                      gv89_u16 occlusion_q15,
                                      gv89_u16 focus_q15)
{
    gv89_voice *voice;
    gv89_u16 audibility;
    if (ctx == 0 || handle.index >= ctx->voices.capacity) {
        return GV89_BAD_ARGUMENT;
    }
    voice = &ctx->voices.voices[handle.index];
    if (!voice->active || voice->generation != handle.generation) {
        return GV89_STALE_HANDLE;
    }
    audibility = gwv89_audibility(distance_q15, occlusion_q15, focus_q15);
    voice->pan_q15 = gwv89_clamp_pan(screen_x_q15);
    voice->audibility_q15 = audibility;
    ctx->voices.rebalance_countdown = 0U;
    return GV89_OK;
}

void gwv89_process_stereo_sample(gwv89_context *ctx,
                                  gv89_s16 *left,
                                  gv89_s16 *right)
{
    if (ctx == 0) {
        if (left != 0) *left = 0;
        if (right != 0) *right = 0;
        return;
    }
    gv89_process_stereo_sample(&ctx->voices, left, right);
}

gv89_u32 gwv89_render_stereo(gwv89_context *ctx,
                               gv89_s16 *interleaved_stereo,
                               gv89_u32 frames,
                               int accumulate)
{
    if (ctx == 0) return 0U;
    return gv89_render_stereo(&ctx->voices, interleaved_stereo,
                              frames, accumulate);
}

void gwv89_get_stats(const gwv89_context *ctx, gv89_stats *stats)
{
    if (ctx == 0) return;
    gv89_get_stats(&ctx->voices, stats);
}

gv89_u16 gwv89_active_count(const gwv89_context *ctx)
{
    return ctx != 0 ? gv89_active_count(&ctx->voices) : 0U;
}

gv89_u16 gwv89_physical_count(const gwv89_context *ctx)
{
    return ctx != 0 ? gv89_physical_count(&ctx->voices) : 0U;
}

gv89_u16 gwv89_virtual_count(const gwv89_context *ctx)
{
    return ctx != 0 ? gv89_virtual_count(&ctx->voices) : 0U;
}

gv89_u32 gwv89_context_bytes(void)
{
    return (gv89_u32)sizeof(gwv89_context);
}
