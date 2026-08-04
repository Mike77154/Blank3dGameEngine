#include "gvoice89.h"

static gv89_s16 gv89_sat16(gv89_s32 x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return (gv89_s16)x;
}

static gv89_s32 gv89_mul_q15(gv89_s32 a, gv89_s32 b)
{
    return (a * b) >> 15;
}

static gv89_u32 gv89_abs32(gv89_s32 x)
{
    if (x < 0) return (gv89_u32)(-x);
    return (gv89_u32)x;
}

static gv89_u32 gv89_ms_to_frames(gv89_u32 rate, gv89_u32 ms)
{
    if (ms == 0U) return 0U;
    return (rate * ms + 999U) / 1000U;
}

static gv89_s16 gv89_clamp_gain(gv89_s32 x)
{
    if (x < 0) return 0;
    if (x > 32767) return 32767;
    return (gv89_s16)x;
}

static gv89_u16 gv89_clamp_uq15(gv89_s32 x)
{
    if (x < 0) return 0U;
    if (x > 32767) return 32767U;
    return (gv89_u16)x;
}

static gv89_s16 gv89_clamp_pan(gv89_s32 x)
{
    if (x < -32767) return -32767;
    if (x > 32767) return 32767;
    return (gv89_s16)x;
}

static gv89_u16 gv89_clamp_priority(gv89_s32 x)
{
    if (x < 0) return 0U;
    if (x > 65535) return 65535U;
    return (gv89_u16)x;
}

static void gv89_clear_basic_provider(gv89_provider *provider)
{
    provider->user = 0;
    provider->process_mono = 0;
    provider->is_active = 0;
    provider->stop = 0;
}

static void gv89_clear_provider_ex(gv89_provider_ex *provider)
{
    gv89_clear_basic_provider(&provider->base);
    provider->advance_frames = 0;
    provider->restart = 0;
    provider->estimated_level_q15 = 0;
    provider->physical_state_changed = 0;
}

void gv89_provider_ex_from_basic(gv89_provider_ex *out_provider,
                                  const gv89_provider *basic)
{
    if (out_provider == 0) return;
    gv89_clear_provider_ex(out_provider);
    if (basic != 0) out_provider->base = *basic;
}

void gv89_request_default(gv89_request *request)
{
    if (request == 0) return;
    request->priority = 256U;
    request->group_id = 0U;
    request->required_capabilities = 0U;
    request->steal_policy = GV89_STEAL_PRIORITY_AUDIBILITY;
    request->virtual_behavior = GV89_VIRTUAL_ADVANCE;
    request->flags = GV89_FLAG_NONE;
    request->instance_key = 0U;
    request->instance_limit = 0U;
    request->bus_id = 0U;
    request->steal_protect_ms = 12U;
    request->minimum_physical_ms = 18U;
    request->virtual_timeout_ms = 0U;
    request->allow_higher_priority_steal = 0U;
    request->allow_protected_steal = 0U;
}

void gv89_voice_params_default(gv89_voice_params *params)
{
    if (params == 0) return;
    params->gain_q15 = 32767;
    params->pan_q15 = 0;
    params->audibility_q15 = 32767U;
    params->attack_ms = 1U;
    params->release_ms = 8U;
}

static void gv89_clear_voice(gv89_voice *voice)
{
    gv89_u32 capabilities;
    gv89_u16 generation;
    gv89_s16 tail_l;
    gv89_s16 tail_r;
    gv89_u16 tail_frames;
    gv89_u16 tail_pos;
    capabilities = voice->capabilities;
    generation = voice->generation;
    tail_l = voice->tail_l;
    tail_r = voice->tail_r;
    tail_frames = voice->tail_frames;
    tail_pos = voice->tail_pos;
    gv89_clear_provider_ex(&voice->provider);
    voice->capabilities = capabilities;
    voice->flags = 0U;
    voice->instance_key = 0U;
    voice->age_frames = 0U;
    voice->virtual_age_frames = 0U;
    voice->physical_age_frames = 0U;
    voice->serial = 0U;
    voice->last_abs = 0U;
    voice->attack_frames = 0U;
    voice->attack_pos = 0U;
    voice->release_frames = 0U;
    voice->release_pos = 0U;
    voice->steal_protect_frames = 0U;
    voice->minimum_physical_frames = 0U;
    voice->virtual_timeout_frames = 0U;
    voice->generation = generation;
    voice->priority = 0U;
    voice->group_id = 0U;
    voice->bus_id = 0U;
    voice->audibility_q15 = 32767U;
    voice->gain_q15 = 32767;
    voice->pan_q15 = 0;
    voice->last_l = 0;
    voice->last_r = 0;
    voice->tail_l = tail_l;
    voice->tail_r = tail_r;
    voice->tail_frames = tail_frames;
    voice->tail_pos = tail_pos;
    voice->virtual_behavior = GV89_VIRTUAL_ADVANCE;
    voice->active = 0U;
    voice->physical = 0U;
    voice->releasing = 0U;
    voice->reserved = 0U;
    voice->selected = 0U;
    voice->needs_restart = 0U;
}

static void gv89_zero_stats(gv89_stats *stats)
{
    stats->starts = 0U;
    stats->natural_ends = 0U;
    stats->steals = 0U;
    stats->group_steals = 0U;
    stats->instance_steals = 0U;
    stats->rejects = 0U;
    stats->protected_rejects = 0U;
    stats->peak_logical = 0U;
    stats->peak_active = 0U;
    stats->peak_physical = 0U;
    stats->promotions = 0U;
    stats->demotions = 0U;
    stats->virtual_kills = 0U;
    stats->virtual_timeouts = 0U;
    stats->forced_virtuals = 0U;
    stats->rebalance_passes = 0U;
    stats->limiter_hits = 0U;
}

static void gv89_rebalance_internal(gv89_context *ctx);

gv89_result gv89_init(gv89_context *ctx,
                        gv89_voice *voice_storage,
                        gv89_u16 capacity,
                        gv89_u32 sample_rate)
{
    gv89_u16 i;
    gv89_u16 default_physical;
    if (ctx == 0 || voice_storage == 0 || capacity == 0U
        || sample_rate == 0U) return GV89_BAD_ARGUMENT;
    ctx->voices = voice_storage;
    ctx->capacity = capacity;
    ctx->active_count = 0U;
    default_physical = capacity;
    if (default_physical > GV89_RECOMMENDED_PHYSICAL_VOICES) {
        default_physical = GV89_RECOMMENDED_PHYSICAL_VOICES;
    }
    if (default_physical > GV89_MAX_PHYSICAL_VOICES) {
        default_physical = GV89_MAX_PHYSICAL_VOICES;
    }
    ctx->physical_limit = default_physical;
    ctx->physical_count = 0U;
    for (i = 0U; i < GV89_MAX_PHYSICAL_VOICES; ++i) {
        ctx->physical_indices[i] = GV89_INVALID_INDEX;
    }
    for (i = 0U; i < GV89_MAX_TAIL_VOICES; ++i) {
        ctx->tail_indices[i] = GV89_INVALID_INDEX;
    }
    ctx->tail_count = 0U;
    ctx->sample_rate = sample_rate;
    ctx->serial_counter = 1U;
    ctx->rebalance_interval_frames = 128U;
    ctx->rebalance_countdown = 0U;
    ctx->physical_hysteresis_q15 = 1024U;
    ctx->inaudible_threshold_q15 = 96U;
    ctx->group_rule_count = 0U;
    ctx->master_gain_q15 = 30000;
    ctx->limiter_threshold = 30000;
    ctx->limiter_gain_q15 = 32767;
    ctx->limiter_release_shift = 9U;
    ctx->anti_click_frames = 32U;
    ctx->batch_depth = 0U;
    ctx->rebalance_pending = 0U;
    gv89_zero_stats(&ctx->stats);
    for (i = 0U; i < GV89_MAX_GROUP_RULES; ++i) {
        ctx->groups[i].group_id = 0U;
        ctx->groups[i].max_logical = 0U;
        ctx->groups[i].physical_reserve = 0U;
        ctx->groups[i].steal_policy = GV89_STEAL_PRIORITY_AUDIBILITY;
        ctx->groups[i].used = 0U;
    }
    for (i = 0U; i < GV89_MAX_BUSES; ++i) {
        ctx->buses[i].gain_q15 = 32767;
        ctx->buses[i].priority_bias = 0;
        ctx->buses[i].mute = 0U;
    }
    for (i = 0U; i < capacity; ++i) {
        voice_storage[i].capabilities = GV89_CAP_ANY;
        voice_storage[i].generation = 1U;
        voice_storage[i].tail_l = 0;
        voice_storage[i].tail_r = 0;
        voice_storage[i].tail_frames = 0U;
        voice_storage[i].tail_pos = 0U;
        gv89_clear_voice(&voice_storage[i]);
    }
    return GV89_OK;
}

gv89_result gv89_set_physical_limit(gv89_context *ctx, gv89_u16 limit)
{
    if (ctx == 0 || limit > GV89_MAX_PHYSICAL_VOICES) {
        return GV89_BAD_ARGUMENT;
    }
    if (limit > ctx->capacity) limit = ctx->capacity;
    ctx->physical_limit = limit;
    gv89_rebalance_internal(ctx);
    return GV89_OK;
}

void gv89_set_virtualization(gv89_context *ctx,
                              gv89_u16 inaudible_threshold_q15,
                              gv89_u16 physical_hysteresis_q15,
                              gv89_u16 rebalance_interval_frames)
{
    if (ctx == 0) return;
    ctx->inaudible_threshold_q15 = gv89_clamp_uq15(inaudible_threshold_q15);
    ctx->physical_hysteresis_q15 = gv89_clamp_uq15(physical_hysteresis_q15);
    if (rebalance_interval_frames == 0U) rebalance_interval_frames = 1U;
    ctx->rebalance_interval_frames = rebalance_interval_frames;
    ctx->rebalance_countdown = 0U;
}

void gv89_set_slot_capabilities(gv89_context *ctx,
                                 gv89_u16 index,
                                 gv89_u32 capabilities)
{
    if (ctx == 0 || index >= ctx->capacity) return;
    ctx->voices[index].capabilities = capabilities;
}

static gv89_group_rule *gv89_find_group_rule(gv89_context *ctx,
                                              gv89_u16 group_id)
{
    gv89_u16 i;
    for (i = 0U; i < GV89_MAX_GROUP_RULES; ++i) {
        if (ctx->groups[i].used && ctx->groups[i].group_id == group_id) {
            return &ctx->groups[i];
        }
    }
    return 0;
}

gv89_result gv89_set_group_rule(gv89_context *ctx,
                                  gv89_u16 group_id,
                                  gv89_u16 max_logical,
                                  gv89_u16 physical_reserve,
                                  gv89_steal_policy policy)
{
    gv89_group_rule *rule;
    gv89_u16 i;
    if (ctx == 0) return GV89_BAD_ARGUMENT;
    rule = gv89_find_group_rule(ctx, group_id);
    if (rule == 0) {
        for (i = 0U; i < GV89_MAX_GROUP_RULES; ++i) {
            if (!ctx->groups[i].used) {
                rule = &ctx->groups[i];
                rule->used = 1U;
                rule->group_id = group_id;
                ctx->group_rule_count++;
                break;
            }
        }
    }
    if (rule == 0) return GV89_BAD_ARGUMENT;
    if (physical_reserve > GV89_MAX_PHYSICAL_VOICES) {
        physical_reserve = GV89_MAX_PHYSICAL_VOICES;
    }
    rule->max_logical = max_logical;
    rule->physical_reserve = physical_reserve;
    rule->steal_policy = policy;
    ctx->rebalance_countdown = 0U;
    return GV89_OK;
}

gv89_result gv89_set_group_limit(gv89_context *ctx,
                                  gv89_u16 group_id,
                                  gv89_u16 max_voices,
                                  gv89_steal_policy policy)
{
    gv89_group_rule *rule;
    gv89_u16 reserve;
    if (ctx == 0) return GV89_BAD_ARGUMENT;
    rule = gv89_find_group_rule(ctx, group_id);
    reserve = rule != 0 ? rule->physical_reserve : 0U;
    return gv89_set_group_rule(ctx, group_id, max_voices, reserve, policy);
}

gv89_result gv89_set_bus(gv89_context *ctx,
                           gv89_u16 bus_id,
                           gv89_s16 gain_q15,
                           gv89_s16 priority_bias,
                           int mute)
{
    if (ctx == 0 || bus_id >= GV89_MAX_BUSES) return GV89_BAD_ARGUMENT;
    ctx->buses[bus_id].gain_q15 = gv89_clamp_gain(gain_q15);
    ctx->buses[bus_id].priority_bias = priority_bias;
    ctx->buses[bus_id].mute = mute ? 1U : 0U;
    ctx->rebalance_countdown = 0U;
    return GV89_OK;
}

void gv89_set_master(gv89_context *ctx,
                      gv89_s16 master_gain_q15,
                      gv89_s16 limiter_threshold,
                      gv89_u8 limiter_release_shift)
{
    if (ctx == 0) return;
    ctx->master_gain_q15 = gv89_clamp_gain(master_gain_q15);
    if (limiter_threshold < 1024) limiter_threshold = 1024;
    ctx->limiter_threshold = limiter_threshold;
    if (limiter_release_shift < 4U) limiter_release_shift = 4U;
    if (limiter_release_shift > 15U) limiter_release_shift = 15U;
    ctx->limiter_release_shift = limiter_release_shift;
}

static int gv89_caps_match(const gv89_voice *voice, gv89_u32 required)
{
    if (required == 0U) return 1;
    return (voice->capabilities & required) == required;
}

static gv89_u16 gv89_group_active_count(const gv89_context *ctx,
                                         gv89_u16 group_id)
{
    gv89_u16 i;
    gv89_u16 count;
    count = 0U;
    for (i = 0U; i < ctx->capacity; ++i) {
        if ((ctx->voices[i].active || ctx->voices[i].reserved)
            && ctx->voices[i].group_id == group_id) count++;
    }
    return count;
}

static gv89_u16 gv89_instance_active_count(const gv89_context *ctx,
                                            gv89_u32 instance_key)
{
    gv89_u16 i;
    gv89_u16 count;
    if (instance_key == 0U) return 0U;
    count = 0U;
    for (i = 0U; i < ctx->capacity; ++i) {
        if ((ctx->voices[i].active || ctx->voices[i].reserved)
            && ctx->voices[i].instance_key == instance_key) count++;
    }
    return count;
}

static gv89_u16 gv89_voice_priority_effective(const gv89_context *ctx,
                                               const gv89_voice *voice)
{
    gv89_s32 value;
    value = voice->priority;
    if (voice->bus_id < GV89_MAX_BUSES) {
        value += ctx->buses[voice->bus_id].priority_bias;
    }
    return gv89_clamp_priority(value);
}

static gv89_u16 gv89_voice_audibility(const gv89_context *ctx,
                                       const gv89_voice *voice)
{
    gv89_s32 level;
    gv89_s32 gain;
    gv89_s32 value;
    const gv89_bus *bus;
    if (!voice->active) return 0U;
    bus = &ctx->buses[voice->bus_id < GV89_MAX_BUSES ? voice->bus_id : 0U];
    if (bus->mute) return 0U;
    if (voice->provider.estimated_level_q15 != 0) {
        level = voice->provider.estimated_level_q15(voice->provider.base.user);
    } else if (voice->age_frames == 0U) {
        level = 32767;
    } else {
        level = voice->last_abs > 32767U ? 32767 : (gv89_s32)voice->last_abs;
    }
    gain = gv89_mul_q15(voice->gain_q15, bus->gain_q15);
    value = gv89_mul_q15(level, gain);
    value = gv89_mul_q15(value, voice->audibility_q15);
    return gv89_clamp_uq15(value);
}

static int gv89_candidate_protected(const gv89_voice *voice,
                                    const gv89_request *request)
{
    if (voice->flags & GV89_FLAG_NEVER_STEAL) return 1;
    if (voice->age_frames < voice->steal_protect_frames
        && !request->allow_protected_steal
        && !(request->flags & GV89_FLAG_ALLOW_PROTECTED_STEAL)) return 1;
    return 0;
}

static int gv89_better_victim(const gv89_context *ctx,
                               const gv89_voice *candidate,
                               const gv89_voice *best,
                               gv89_steal_policy policy)
{
    gv89_u16 cp;
    gv89_u16 bp;
    gv89_u16 ca;
    gv89_u16 ba;
    if (best == 0) return 1;
    cp = gv89_voice_priority_effective(ctx, candidate);
    bp = gv89_voice_priority_effective(ctx, best);
    ca = gv89_voice_audibility(ctx, candidate);
    ba = gv89_voice_audibility(ctx, best);
    if (policy == GV89_STEAL_OLDEST) {
        if (candidate->age_frames != best->age_frames) {
            return candidate->age_frames > best->age_frames;
        }
        return candidate->serial < best->serial;
    }
    if (policy == GV89_STEAL_NEWEST) {
        if (candidate->age_frames != best->age_frames) {
            return candidate->age_frames < best->age_frames;
        }
        return candidate->serial > best->serial;
    }
    if (policy == GV89_STEAL_QUIETEST) {
        if (ca != ba) return ca < ba;
        if (cp != bp) return cp < bp;
        return candidate->age_frames > best->age_frames;
    }
    if (policy == GV89_STEAL_FURTHEST) {
        if (candidate->audibility_q15 != best->audibility_q15) {
            return candidate->audibility_q15 < best->audibility_q15;
        }
        if (cp != bp) return cp < bp;
        return candidate->age_frames > best->age_frames;
    }
    if (policy == GV89_STEAL_HYBRID) {
        gv89_u32 cscore;
        gv89_u32 bscore;
        cscore = ((gv89_u32)cp << 12) + ca;
        bscore = ((gv89_u32)bp << 12) + ba;
        if (cscore != bscore) return cscore < bscore;
        return candidate->age_frames > best->age_frames;
    }
    if (cp != bp) return cp < bp;
    if (ca != ba) return ca < ba;
    return candidate->age_frames > best->age_frames;
}

static gv89_u16 gv89_select_victim(gv89_context *ctx,
                                    const gv89_request *request,
                                    int restrict_group,
                                    int restrict_instance,
                                    gv89_steal_policy policy,
                                    int *had_protected)
{
    gv89_u16 i;
    gv89_voice *best;
    gv89_voice *voice;
    gv89_u16 request_priority;
    best = 0;
    if (had_protected != 0) *had_protected = 0;
    if (policy == GV89_STEAL_REJECT) return GV89_INVALID_INDEX;
    request_priority = request->priority;
    for (i = 0U; i < ctx->capacity; ++i) {
        voice = &ctx->voices[i];
        if (!voice->active || voice->reserved) continue;
        if (!gv89_caps_match(voice, request->required_capabilities)) continue;
        if (restrict_group && voice->group_id != request->group_id) continue;
        if (restrict_instance && voice->instance_key != request->instance_key) {
            continue;
        }
        if (!request->allow_higher_priority_steal
            && gv89_voice_priority_effective(ctx, voice) > request_priority) {
            continue;
        }
        if (gv89_candidate_protected(voice, request)) {
            if (had_protected != 0) *had_protected = 1;
            continue;
        }
        if (gv89_better_victim(ctx, voice, best, policy)) best = voice;
    }
    if (best == 0) return GV89_INVALID_INDEX;
    return (gv89_u16)(best - ctx->voices);
}

static void gv89_capture_tail(gv89_context *ctx, gv89_voice *voice)
{
    gv89_u16 index;
    gv89_u16 i;
    if (ctx->anti_click_frames == 0U) return;
    if (voice->last_l == 0 && voice->last_r == 0) return;
    voice->tail_l = voice->last_l;
    voice->tail_r = voice->last_r;
    voice->tail_frames = ctx->anti_click_frames;
    voice->tail_pos = 0U;
    index = (gv89_u16)(voice - ctx->voices);
    for (i = 0U; i < ctx->tail_count; ++i) {
        if (ctx->tail_indices[i] == index) return;
    }
    if (ctx->tail_count < GV89_MAX_TAIL_VOICES) {
        ctx->tail_indices[ctx->tail_count++] = index;
    }
}


static void gv89_deactivate(gv89_context *ctx,
                            gv89_voice *voice,
                            int stolen,
                            int natural)
{
    gv89_u32 capabilities;
    gv89_u16 generation;
    if (voice->physical && voice->provider.physical_state_changed != 0) {
        voice->provider.physical_state_changed(voice->provider.base.user, 0);
    }
    if (voice->provider.base.stop != 0) {
        voice->provider.base.stop(voice->provider.base.user);
    }
    gv89_capture_tail(ctx, voice);
    capabilities = voice->capabilities;
    generation = voice->generation;
    if (voice->active && ctx->active_count > 0U) ctx->active_count--;
    gv89_clear_voice(voice);
    voice->capabilities = capabilities;
    voice->generation = generation;
    if (stolen) ctx->stats.steals++;
    if (natural) ctx->stats.natural_ends++;
    ctx->rebalance_countdown = 0U;
}

gv89_result gv89_reserve(gv89_context *ctx,
                           const gv89_request *request,
                           gv89_reservation *reservation)
{
    gv89_u16 i;
    gv89_u16 selected;
    gv89_group_rule *rule;
    gv89_voice *voice;
    int protected_block;
    if (ctx == 0 || request == 0 || reservation == 0
        || request->bus_id >= GV89_MAX_BUSES) return GV89_BAD_ARGUMENT;
    reservation->index = GV89_INVALID_INDEX;
    reservation->generation = 0U;
    reservation->valid = 0U;
    selected = GV89_INVALID_INDEX;
    protected_block = 0;

    rule = gv89_find_group_rule(ctx, request->group_id);
    if (rule != 0 && rule->max_logical > 0U
        && gv89_group_active_count(ctx, request->group_id)
           >= rule->max_logical) {
        selected = gv89_select_victim(ctx, request, 1, 0,
                                       rule->steal_policy,
                                       &protected_block);
        if (selected == GV89_INVALID_INDEX) {
            ctx->stats.rejects++;
            if (protected_block) ctx->stats.protected_rejects++;
            return GV89_GROUP_LIMIT;
        }
        gv89_deactivate(ctx, &ctx->voices[selected], 1, 0);
        ctx->stats.group_steals++;
    } else if (request->instance_key != 0U && request->instance_limit > 0U
               && gv89_instance_active_count(ctx, request->instance_key)
                  >= request->instance_limit) {
        selected = gv89_select_victim(ctx, request, 0, 1,
                                       request->steal_policy,
                                       &protected_block);
        if (selected == GV89_INVALID_INDEX) {
            ctx->stats.rejects++;
            if (protected_block) ctx->stats.protected_rejects++;
            return GV89_INSTANCE_LIMIT;
        }
        gv89_deactivate(ctx, &ctx->voices[selected], 1, 0);
        ctx->stats.instance_steals++;
    } else {
        for (i = 0U; i < ctx->capacity; ++i) {
            voice = &ctx->voices[i];
            if (!voice->active && !voice->reserved
                && gv89_caps_match(voice, request->required_capabilities)) {
                selected = i;
                break;
            }
        }
        if (selected == GV89_INVALID_INDEX) {
            selected = gv89_select_victim(ctx, request, 0, 0,
                                           request->steal_policy,
                                           &protected_block);
            if (selected == GV89_INVALID_INDEX) {
                ctx->stats.rejects++;
                if (protected_block) ctx->stats.protected_rejects++;
                return GV89_NO_VOICE;
            }
            gv89_deactivate(ctx, &ctx->voices[selected], 1, 0);
        }
    }

    voice = &ctx->voices[selected];
    voice->generation++;
    if (voice->generation == 0U) voice->generation = 1U;
    voice->priority = request->priority;
    voice->group_id = request->group_id;
    voice->bus_id = request->bus_id;
    voice->flags = request->flags;
    voice->instance_key = request->instance_key;
    voice->virtual_behavior = request->virtual_behavior;
    voice->steal_protect_frames = gv89_ms_to_frames(ctx->sample_rate,
                                              request->steal_protect_ms);
    voice->minimum_physical_frames = gv89_ms_to_frames(ctx->sample_rate,
                                              request->minimum_physical_ms);
    voice->virtual_timeout_frames = gv89_ms_to_frames(ctx->sample_rate,
                                              request->virtual_timeout_ms);
    voice->reserved = 1U;
    reservation->index = selected;
    reservation->generation = voice->generation;
    reservation->valid = 1U;
    return GV89_OK;
}

static gv89_result gv89_commit_common(gv89_context *ctx,
                                       const gv89_reservation *reservation,
                                       const gv89_provider_ex *provider,
                                       const gv89_voice_params *params,
                                       gv89_handle *handle)
{
    gv89_voice *voice;
    gv89_voice_params defaults;
    const gv89_voice_params *p;
    if (ctx == 0 || reservation == 0 || provider == 0
        || provider->base.process_mono == 0 || !reservation->valid
        || reservation->index >= ctx->capacity) return GV89_BAD_ARGUMENT;
    voice = &ctx->voices[reservation->index];
    if (!voice->reserved || voice->generation != reservation->generation) {
        return GV89_NOT_RESERVED;
    }
    if (params == 0) {
        gv89_voice_params_default(&defaults);
        p = &defaults;
    } else {
        p = params;
    }
    voice->provider = *provider;
    voice->gain_q15 = gv89_clamp_gain(p->gain_q15);
    voice->pan_q15 = gv89_clamp_pan(p->pan_q15);
    voice->audibility_q15 = gv89_clamp_uq15(p->audibility_q15);
    voice->attack_frames = gv89_ms_to_frames(ctx->sample_rate, p->attack_ms);
    voice->attack_pos = 0U;
    voice->release_frames = gv89_ms_to_frames(ctx->sample_rate, p->release_ms);
    voice->release_pos = 0U;
    voice->age_frames = 0U;
    voice->virtual_age_frames = 0U;
    voice->physical_age_frames = 0U;
    voice->last_abs = 0U;
    voice->serial = ctx->serial_counter++;
    voice->active = 1U;
    voice->physical = 0U;
    voice->releasing = 0U;
    voice->reserved = 0U;
    voice->needs_restart = 0U;
    ctx->active_count++;
    ctx->stats.starts++;
    if ((gv89_u32)ctx->active_count > ctx->stats.peak_logical) {
        ctx->stats.peak_logical = ctx->active_count;
        ctx->stats.peak_active = ctx->active_count;
    }
    if (ctx->batch_depth > 0U) {
        ctx->rebalance_pending = 1U;
        ctx->rebalance_countdown = 0U;
    } else {
        gv89_rebalance_internal(ctx);
    }
    if (ctx->batch_depth == 0U && !voice->active) return GV89_NO_VOICE;
    if ((voice->flags & GV89_FLAG_NEVER_VIRTUAL) && !voice->physical) {
        ctx->stats.forced_virtuals++;
        gv89_deactivate(ctx, voice, 0, 0);
        return GV89_PHYSICAL_LIMIT;
    }
    if (handle != 0) {
        handle->index = reservation->index;
        handle->generation = reservation->generation;
    }
    return GV89_OK;
}

gv89_result gv89_commit(gv89_context *ctx,
                          const gv89_reservation *reservation,
                          const gv89_provider *provider,
                          const gv89_voice_params *params,
                          gv89_handle *handle)
{
    gv89_provider_ex extended;
    if (provider == 0) return GV89_BAD_ARGUMENT;
    gv89_provider_ex_from_basic(&extended, provider);
    return gv89_commit_common(ctx, reservation, &extended, params, handle);
}

gv89_result gv89_commit_ex(gv89_context *ctx,
                             const gv89_reservation *reservation,
                             const gv89_provider_ex *provider,
                             const gv89_voice_params *params,
                             gv89_handle *handle)
{
    return gv89_commit_common(ctx, reservation, provider, params, handle);
}

void gv89_cancel_reservation(gv89_context *ctx,
                              const gv89_reservation *reservation)
{
    gv89_voice *voice;
    if (ctx == 0 || reservation == 0 || !reservation->valid
        || reservation->index >= ctx->capacity) return;
    voice = &ctx->voices[reservation->index];
    if (voice->reserved && voice->generation == reservation->generation) {
        voice->reserved = 0U;
        voice->priority = 0U;
        voice->group_id = 0U;
        voice->instance_key = 0U;
        voice->flags = 0U;
    }
}

gv89_result gv89_start(gv89_context *ctx,
                         const gv89_request *request,
                         const gv89_provider *provider,
                         const gv89_voice_params *params,
                         gv89_handle *handle)
{
    gv89_reservation reservation;
    gv89_result result;
    result = gv89_reserve(ctx, request, &reservation);
    if (result != GV89_OK) return result;
    result = gv89_commit(ctx, &reservation, provider, params, handle);
    if (result != GV89_OK) gv89_cancel_reservation(ctx, &reservation);
    return result;
}

gv89_result gv89_start_ex(gv89_context *ctx,
                            const gv89_request *request,
                            const gv89_provider_ex *provider,
                            const gv89_voice_params *params,
                            gv89_handle *handle)
{
    gv89_reservation reservation;
    gv89_result result;
    result = gv89_reserve(ctx, request, &reservation);
    if (result != GV89_OK) return result;
    result = gv89_commit_ex(ctx, &reservation, provider, params, handle);
    if (result != GV89_OK) gv89_cancel_reservation(ctx, &reservation);
    return result;
}

static gv89_voice *gv89_from_handle(gv89_context *ctx, gv89_handle handle)
{
    gv89_voice *voice;
    if (ctx == 0 || handle.index >= ctx->capacity) return 0;
    voice = &ctx->voices[handle.index];
    if (!voice->active || voice->generation != handle.generation) return 0;
    return voice;
}

gv89_result gv89_stop(gv89_context *ctx,
                        gv89_handle handle,
                        gv89_u16 release_ms)
{
    gv89_voice *voice;
    voice = gv89_from_handle(ctx, handle);
    if (voice == 0) return GV89_STALE_HANDLE;
    if (release_ms > 0U) {
        voice->release_frames = gv89_ms_to_frames(ctx->sample_rate, release_ms);
    }
    if (voice->release_frames == 0U) {
        gv89_deactivate(ctx, voice, 0, 0);
    } else {
        voice->releasing = 1U;
        voice->release_pos = 0U;
    }
    return GV89_OK;
}

void gv89_stop_group(gv89_context *ctx,
                      gv89_u16 group_id,
                      gv89_u16 release_ms)
{
    gv89_u16 i;
    gv89_handle handle;
    if (ctx == 0) return;
    for (i = 0U; i < ctx->capacity; ++i) {
        if (ctx->voices[i].active && ctx->voices[i].group_id == group_id) {
            handle.index = i;
            handle.generation = ctx->voices[i].generation;
            gv89_stop(ctx, handle, release_ms);
        }
    }
}

void gv89_stop_all(gv89_context *ctx, gv89_u16 release_ms)
{
    gv89_u16 i;
    gv89_handle handle;
    if (ctx == 0) return;
    for (i = 0U; i < ctx->capacity; ++i) {
        if (ctx->voices[i].active) {
            handle.index = i;
            handle.generation = ctx->voices[i].generation;
            gv89_stop(ctx, handle, release_ms);
        }
    }
}

gv89_result gv89_set_voice_mix(gv89_context *ctx,
                                 gv89_handle handle,
                                 gv89_s16 gain_q15,
                                 gv89_s16 pan_q15)
{
    gv89_voice *voice;
    voice = gv89_from_handle(ctx, handle);
    if (voice == 0) return GV89_STALE_HANDLE;
    voice->gain_q15 = gv89_clamp_gain(gain_q15);
    voice->pan_q15 = gv89_clamp_pan(pan_q15);
    ctx->rebalance_countdown = 0U;
    return GV89_OK;
}

gv89_result gv89_set_voice_audibility(gv89_context *ctx,
                                        gv89_handle handle,
                                        gv89_u16 audibility_q15)
{
    gv89_voice *voice;
    voice = gv89_from_handle(ctx, handle);
    if (voice == 0) return GV89_STALE_HANDLE;
    voice->audibility_q15 = gv89_clamp_uq15(audibility_q15);
    ctx->rebalance_countdown = 0U;
    return GV89_OK;
}

gv89_result gv89_set_voice_priority(gv89_context *ctx,
                                      gv89_handle handle,
                                      gv89_u16 priority)
{
    gv89_voice *voice;
    voice = gv89_from_handle(ctx, handle);
    if (voice == 0) return GV89_STALE_HANDLE;
    voice->priority = priority;
    ctx->rebalance_countdown = 0U;
    return GV89_OK;
}

static int gv89_voice_eligible_physical(const gv89_context *ctx,
                                        const gv89_voice *voice)
{
    gv89_u32 audible;
    if (!voice->active || voice->reserved || voice->selected) return 0;
    if (voice->flags & (GV89_FLAG_CRITICAL | GV89_FLAG_NEVER_VIRTUAL)) return 1;
    audible = gv89_voice_audibility(ctx, voice);
    if (voice->physical) audible += ctx->physical_hysteresis_q15;
    return audible >= ctx->inaudible_threshold_q15;
}

static int gv89_better_physical(const gv89_context *ctx,
                                 const gv89_voice *candidate,
                                 const gv89_voice *best)
{
    gv89_u16 cp;
    gv89_u16 bp;
    gv89_u32 ca;
    gv89_u32 ba;
    gv89_u8 cf;
    gv89_u8 bf;
    gv89_u8 ch;
    gv89_u8 bh;
    if (best == 0) return 1;
    cf = (candidate->flags & GV89_FLAG_NEVER_VIRTUAL) ? 2U
       : ((candidate->flags & GV89_FLAG_CRITICAL) ? 1U : 0U);
    bf = (best->flags & GV89_FLAG_NEVER_VIRTUAL) ? 2U
       : ((best->flags & GV89_FLAG_CRITICAL) ? 1U : 0U);
    if (cf != bf) return cf > bf;
    ch = candidate->physical
       && candidate->physical_age_frames < candidate->minimum_physical_frames;
    bh = best->physical
       && best->physical_age_frames < best->minimum_physical_frames;
    if (ch != bh) return ch > bh;
    cp = gv89_voice_priority_effective(ctx, candidate);
    bp = gv89_voice_priority_effective(ctx, best);
    if (cp != bp) return cp > bp;
    ca = gv89_voice_audibility(ctx, candidate);
    ba = gv89_voice_audibility(ctx, best);
    if (candidate->physical) ca += ctx->physical_hysteresis_q15;
    if (best->physical) ba += ctx->physical_hysteresis_q15;
    if (ca != ba) return ca > ba;
    if (candidate->physical != best->physical) return candidate->physical;
    return candidate->serial < best->serial;
}

static gv89_voice *gv89_select_best_physical(gv89_context *ctx,
                                              int restrict_group,
                                              gv89_u16 group_id)
{
    gv89_u16 i;
    gv89_voice *best;
    gv89_voice *voice;
    best = 0;
    for (i = 0U; i < ctx->capacity; ++i) {
        voice = &ctx->voices[i];
        if (!gv89_voice_eligible_physical(ctx, voice)) continue;
        if (restrict_group && voice->group_id != group_id) continue;
        if (gv89_better_physical(ctx, voice, best)) best = voice;
    }
    return best;
}

static void gv89_apply_virtual_transition(gv89_context *ctx,
                                          gv89_voice *voice,
                                          gv89_u8 old_physical)
{
    if (voice->selected) {
        voice->physical = 1U;
        voice->virtual_age_frames = 0U;
        if (!old_physical) {
            voice->physical_age_frames = 0U;
            ctx->stats.promotions++;
            if (voice->provider.physical_state_changed != 0) {
                voice->provider.physical_state_changed(
                    voice->provider.base.user, 1);
            }
            if (voice->needs_restart && voice->provider.restart != 0) {
                voice->provider.restart(voice->provider.base.user);
            }
            voice->needs_restart = 0U;
        }
    } else {
        voice->physical = 0U;
        voice->physical_age_frames = 0U;
        if (old_physical) {
            ctx->stats.demotions++;
            if (voice->provider.physical_state_changed != 0) {
                voice->provider.physical_state_changed(
                    voice->provider.base.user, 0);
            }
            voice->virtual_age_frames = 0U;
            if (voice->virtual_behavior == GV89_VIRTUAL_RESTART) {
                voice->needs_restart = 1U;
            }
        }
    }
}

static void gv89_rebalance_internal(gv89_context *ctx)
{
    gv89_u16 i;
    gv89_u16 j;
    gv89_u16 selected_count;
    gv89_u16 reserve_count;
    gv89_voice *voice;
    gv89_voice *best;
    const gv89_group_rule *rule;
    if (ctx == 0) return;
    ctx->stats.rebalance_passes++;
    for (i = 0U; i < ctx->capacity; ++i) ctx->voices[i].selected = 0U;
    selected_count = 0U;

    for (i = 0U; i < GV89_MAX_GROUP_RULES
         && selected_count < ctx->physical_limit; ++i) {
        rule = &ctx->groups[i];
        if (!rule->used || rule->physical_reserve == 0U) continue;
        reserve_count = rule->physical_reserve;
        for (j = 0U; j < reserve_count
             && selected_count < ctx->physical_limit; ++j) {
            best = gv89_select_best_physical(ctx, 1, rule->group_id);
            if (best == 0) break;
            best->selected = 1U;
            selected_count++;
        }
    }
    while (selected_count < ctx->physical_limit) {
        best = gv89_select_best_physical(ctx, 0, 0U);
        if (best == 0) break;
        best->selected = 1U;
        selected_count++;
    }

    for (i = 0U; i < ctx->capacity; ++i) {
        voice = &ctx->voices[i];
        if (!voice->active) continue;
        if (!voice->selected && voice->virtual_behavior == GV89_VIRTUAL_KILL) {
            ctx->stats.virtual_kills++;
            gv89_deactivate(ctx, voice, 0, 0);
            continue;
        }
        gv89_apply_virtual_transition(ctx, voice, voice->physical);
    }

    ctx->physical_count = 0U;
    for (i = 0U; i < GV89_MAX_PHYSICAL_VOICES; ++i) {
        ctx->physical_indices[i] = GV89_INVALID_INDEX;
    }
    for (i = 0U; i < ctx->capacity
         && ctx->physical_count < ctx->physical_limit; ++i) {
        if (ctx->voices[i].active && ctx->voices[i].physical) {
            ctx->physical_indices[ctx->physical_count++] = i;
        }
    }
    if ((gv89_u32)ctx->physical_count > ctx->stats.peak_physical) {
        ctx->stats.peak_physical = ctx->physical_count;
    }
    ctx->rebalance_countdown = ctx->rebalance_interval_frames;
    ctx->rebalance_pending = 0U;
}

void gv89_begin_batch(gv89_context *ctx)
{
    if (ctx == 0) return;
    if (ctx->batch_depth < 65535U) ctx->batch_depth++;
}

gv89_result gv89_end_batch(gv89_context *ctx)
{
    if (ctx == 0 || ctx->batch_depth == 0U) return GV89_BAD_ARGUMENT;
    ctx->batch_depth--;
    if (ctx->batch_depth == 0U && ctx->rebalance_pending) {
        gv89_rebalance_internal(ctx);
    }
    return GV89_OK;
}

void gv89_force_rebalance(gv89_context *ctx)
{
    if (ctx == 0) return;
    gv89_rebalance_internal(ctx);
}

static gv89_s32 gv89_envelope_q15(const gv89_voice *voice)
{
    gv89_s32 env;
    if (voice->releasing) {
        if (voice->release_frames == 0U) return 0;
        if (voice->release_pos >= voice->release_frames) return 0;
        env = 32767 - (gv89_s32)((voice->release_pos * 32767U)
              / voice->release_frames);
        return env;
    }
    if (voice->attack_frames > 0U && voice->attack_pos < voice->attack_frames) {
        env = (gv89_s32)((voice->attack_pos * 32767U)
              / voice->attack_frames);
        return env;
    }
    return 32767;
}

static void gv89_mix_tail(gv89_voice *voice, gv89_s32 *left, gv89_s32 *right)
{
    gv89_s32 env;
    if (voice->tail_frames == 0U || voice->tail_pos >= voice->tail_frames) return;
    env = 32767 - (gv89_s32)(((gv89_u32)voice->tail_pos * 32767U)
          / (gv89_u32)voice->tail_frames);
    *left += gv89_mul_q15(voice->tail_l, env);
    *right += gv89_mul_q15(voice->tail_r, env);
    voice->tail_pos++;
    if (voice->tail_pos >= voice->tail_frames) {
        voice->tail_frames = 0U;
        voice->tail_pos = 0U;
        voice->tail_l = 0;
        voice->tail_r = 0;
    }
}

static void gv89_pan_gains(gv89_s16 gain,
                            gv89_s16 pan,
                            gv89_s32 *left_gain,
                            gv89_s32 *right_gain)
{
    if (pan < 0) {
        *left_gain = gain;
        *right_gain = gv89_mul_q15(gain, 32767 + pan);
    } else {
        *left_gain = gv89_mul_q15(gain, 32767 - pan);
        *right_gain = gain;
    }
}

static int gv89_provider_active(const gv89_voice *voice)
{
    if (voice->provider.base.is_active == 0) return 1;
    return voice->provider.base.is_active(voice->provider.base.user);
}

static void gv89_advance_envelope(gv89_context *ctx,
                                  gv89_voice *voice,
                                  gv89_u32 frames)
{
    if (!voice->releasing && voice->attack_pos < voice->attack_frames) {
        gv89_u32 left;
        left = voice->attack_frames - voice->attack_pos;
        voice->attack_pos += frames < left ? frames : left;
    }
    if (voice->releasing) {
        gv89_u32 left;
        if (voice->release_pos >= voice->release_frames) {
            gv89_deactivate(ctx, voice, 0, 0);
            return;
        }
        left = voice->release_frames - voice->release_pos;
        voice->release_pos += frames < left ? frames : left;
        if (voice->release_pos >= voice->release_frames) {
            gv89_deactivate(ctx, voice, 0, 0);
        }
    }
}

static void gv89_step_virtual_voice(gv89_context *ctx,
                                     gv89_voice *voice,
                                     gv89_u32 frames)
{
    gv89_u32 i;
    gv89_s16 sample;
    gv89_u32 peak;
    if (!voice->active || voice->physical || voice->reserved) return;
    voice->virtual_age_frames += frames;
    voice->age_frames += frames;
    if (voice->virtual_timeout_frames > 0U
        && voice->virtual_age_frames >= voice->virtual_timeout_frames) {
        ctx->stats.virtual_timeouts++;
        gv89_deactivate(ctx, voice, 0, 0);
        return;
    }
    if (voice->virtual_behavior == GV89_VIRTUAL_CONTINUE) {
        peak = 0U;
        for (i = 0U; i < frames && voice->active; ++i) {
            if (!gv89_provider_active(voice)) {
                gv89_deactivate(ctx, voice, 0, 1);
                break;
            }
            sample = voice->provider.base.process_mono(voice->provider.base.user);
            if (gv89_abs32(sample) > peak) peak = gv89_abs32(sample);
        }
        voice->last_abs = peak;
        if (voice->active && !gv89_provider_active(voice)) {
            gv89_deactivate(ctx, voice, 0, 1);
            return;
        }
        if (voice->active) gv89_advance_envelope(ctx, voice, frames);
    } else if (voice->virtual_behavior == GV89_VIRTUAL_ADVANCE) {
        if (voice->provider.advance_frames != 0) {
            voice->provider.advance_frames(voice->provider.base.user, frames);
        } else {
            for (i = 0U; i < frames && voice->active; ++i) {
                if (!gv89_provider_active(voice)) break;
                voice->provider.base.process_mono(voice->provider.base.user);
            }
        }
        if (!gv89_provider_active(voice)) {
            gv89_deactivate(ctx, voice, 0, 1);
            return;
        }
        if (voice->provider.estimated_level_q15 != 0) {
            voice->last_abs = voice->provider.estimated_level_q15(
                                             voice->provider.base.user);
        }
        gv89_advance_envelope(ctx, voice, frames);
    } else if (voice->releasing) {
        gv89_advance_envelope(ctx, voice, frames);
    }
}

static void gv89_step_virtuals(gv89_context *ctx, gv89_u32 frames)
{
    gv89_u16 i;
    for (i = 0U; i < ctx->capacity; ++i) {
        gv89_step_virtual_voice(ctx, &ctx->voices[i], frames);
    }
}

static void gv89_process_tails(gv89_context *ctx,
                                gv89_s32 *mix_l,
                                gv89_s32 *mix_r)
{
    gv89_u16 i;
    gv89_u16 index;
    gv89_voice *voice;
    i = 0U;
    while (i < ctx->tail_count) {
        index = ctx->tail_indices[i];
        if (index == GV89_INVALID_INDEX || index >= ctx->capacity) {
            ctx->tail_indices[i] = ctx->tail_indices[ctx->tail_count - 1U];
            ctx->tail_count--;
            continue;
        }
        voice = &ctx->voices[index];
        gv89_mix_tail(voice, mix_l, mix_r);
        if (voice->tail_frames == 0U) {
            ctx->tail_indices[i] = ctx->tail_indices[ctx->tail_count - 1U];
            ctx->tail_indices[ctx->tail_count - 1U] = GV89_INVALID_INDEX;
            ctx->tail_count--;
            continue;
        }
        i++;
    }
}

static void gv89_process_physical_sample(gv89_context *ctx,
                                          gv89_s16 *left,
                                          gv89_s16 *right)
{
    gv89_u16 p;
    gv89_u16 index;
    gv89_voice *voice;
    const gv89_bus *bus;
    gv89_s32 mix_l;
    gv89_s32 mix_r;
    gv89_s32 sample;
    gv89_s32 env;
    gv89_s32 gain;
    gv89_s32 gain_l;
    gv89_s32 gain_r;
    gv89_s32 out_l;
    gv89_s32 out_r;
    gv89_u32 peak;
    gv89_s32 target_gain;
    mix_l = 0;
    mix_r = 0;
    gv89_process_tails(ctx, &mix_l, &mix_r);
    for (p = 0U; p < ctx->physical_count; ++p) {
        index = ctx->physical_indices[p];
        if (index == GV89_INVALID_INDEX || index >= ctx->capacity) continue;
        voice = &ctx->voices[index];
        if (!voice->active || !voice->physical || voice->reserved) continue;
        if (!gv89_provider_active(voice)) {
            gv89_deactivate(ctx, voice, 0, 1);
            continue;
        }
        sample = voice->provider.base.process_mono(voice->provider.base.user);
        env = gv89_envelope_q15(voice);
        bus = &ctx->buses[voice->bus_id < GV89_MAX_BUSES ? voice->bus_id : 0U];
        gain = bus->mute ? 0 : gv89_mul_q15(voice->gain_q15, bus->gain_q15);
        gain = gv89_mul_q15(gain, env);
        gv89_pan_gains((gv89_s16)gain, voice->pan_q15, &gain_l, &gain_r);
        out_l = gv89_mul_q15(sample, gain_l);
        out_r = gv89_mul_q15(sample, gain_r);
        mix_l += out_l;
        mix_r += out_r;
        voice->last_l = gv89_sat16(out_l);
        voice->last_r = gv89_sat16(out_r);
        voice->last_abs = gv89_abs32(out_l) > gv89_abs32(out_r)
                        ? gv89_abs32(out_l) : gv89_abs32(out_r);
        voice->age_frames++;
        voice->physical_age_frames++;
        if (!voice->releasing && voice->attack_pos < voice->attack_frames) {
            voice->attack_pos++;
        }
        if (voice->releasing) {
            voice->release_pos++;
            if (voice->release_pos >= voice->release_frames) {
                gv89_deactivate(ctx, voice, 0, 0);
                continue;
            }
        }
        if (voice->active && !gv89_provider_active(voice)) {
            gv89_deactivate(ctx, voice, 0, 1);
        }
    }
    mix_l = gv89_mul_q15(mix_l, ctx->master_gain_q15);
    mix_r = gv89_mul_q15(mix_r, ctx->master_gain_q15);
    peak = gv89_abs32(mix_l);
    if (gv89_abs32(mix_r) > peak) peak = gv89_abs32(mix_r);
    if (peak > (gv89_u32)ctx->limiter_threshold) {
        target_gain = ((gv89_s32)ctx->limiter_threshold << 15)
                    / (gv89_s32)peak;
        if (target_gain < ctx->limiter_gain_q15) {
            ctx->limiter_gain_q15 = gv89_clamp_gain(target_gain);
        }
        ctx->stats.limiter_hits++;
    } else if (ctx->limiter_gain_q15 < 32767) {
        ctx->limiter_gain_q15 = (gv89_s16)(ctx->limiter_gain_q15
            + ((32767 - ctx->limiter_gain_q15)
               >> ctx->limiter_release_shift));
        if (ctx->limiter_gain_q15 > 32760) ctx->limiter_gain_q15 = 32767;
    }
    mix_l = gv89_mul_q15(mix_l, ctx->limiter_gain_q15);
    mix_r = gv89_mul_q15(mix_r, ctx->limiter_gain_q15);
    *left = gv89_sat16(mix_l);
    *right = gv89_sat16(mix_r);
}

void gv89_process_stereo_sample(gv89_context *ctx,
                                 gv89_s16 *left,
                                 gv89_s16 *right)
{
    if (left == 0 || right == 0) return;
    *left = 0;
    *right = 0;
    if (ctx == 0) return;
    if (ctx->rebalance_countdown == 0U) gv89_rebalance_internal(ctx);
    gv89_step_virtuals(ctx, 1U);
    gv89_process_physical_sample(ctx, left, right);
    if (ctx->rebalance_countdown > 0U) ctx->rebalance_countdown--;
}

gv89_u32 gv89_render_stereo(gv89_context *ctx,
                              gv89_s16 *interleaved_stereo,
                              gv89_u32 frames,
                              int accumulate)
{
    gv89_u32 done;
    gv89_u32 chunk;
    gv89_u32 i;
    gv89_s16 left;
    gv89_s16 right;
    gv89_s32 mixed;
    if (ctx == 0 || interleaved_stereo == 0) return 0U;
    done = 0U;
    while (done < frames) {
        if (ctx->rebalance_countdown == 0U) gv89_rebalance_internal(ctx);
        chunk = frames - done;
        if (chunk > ctx->rebalance_countdown) chunk = ctx->rebalance_countdown;
        if (chunk == 0U) chunk = 1U;
        gv89_step_virtuals(ctx, chunk);
        for (i = 0U; i < chunk; ++i) {
            gv89_process_physical_sample(ctx, &left, &right);
            if (accumulate) {
                mixed = (gv89_s32)interleaved_stereo[(done + i) * 2U] + left;
                interleaved_stereo[(done + i) * 2U] = gv89_sat16(mixed);
                mixed = (gv89_s32)interleaved_stereo[(done + i) * 2U + 1U]
                      + right;
                interleaved_stereo[(done + i) * 2U + 1U] = gv89_sat16(mixed);
            } else {
                interleaved_stereo[(done + i) * 2U] = left;
                interleaved_stereo[(done + i) * 2U + 1U] = right;
            }
        }
        done += chunk;
        if (ctx->rebalance_countdown >= chunk) {
            ctx->rebalance_countdown -= chunk;
        } else {
            ctx->rebalance_countdown = 0U;
        }
    }
    return frames;
}

int gv89_is_handle_active(const gv89_context *ctx, gv89_handle handle)
{
    const gv89_voice *voice;
    if (ctx == 0 || handle.index >= ctx->capacity) return 0;
    voice = &ctx->voices[handle.index];
    return voice->active && voice->generation == handle.generation;
}

gv89_u8 gv89_voice_state(const gv89_context *ctx, gv89_handle handle)
{
    const gv89_voice *voice;
    if (ctx == 0 || handle.index >= ctx->capacity) return GV89_VOICE_INACTIVE;
    voice = &ctx->voices[handle.index];
    if (voice->generation != handle.generation) return GV89_VOICE_INACTIVE;
    if (voice->reserved) return GV89_VOICE_RESERVED;
    if (!voice->active) return GV89_VOICE_INACTIVE;
    return voice->physical ? GV89_VOICE_PHYSICAL : GV89_VOICE_VIRTUAL;
}

gv89_u16 gv89_active_count(const gv89_context *ctx)
{
    return ctx != 0 ? ctx->active_count : 0U;
}

gv89_u16 gv89_physical_count(const gv89_context *ctx)
{
    gv89_u16 i;
    gv89_u16 count;
    if (ctx == 0) return 0U;
    count = 0U;
    for (i = 0U; i < ctx->capacity; ++i) {
        if (ctx->voices[i].active && ctx->voices[i].physical) count++;
    }
    return count;
}

gv89_u16 gv89_virtual_count(const gv89_context *ctx)
{
    gv89_u16 physical;
    if (ctx == 0) return 0U;
    physical = gv89_physical_count(ctx);
    if (ctx->active_count < physical) return 0U;
    return (gv89_u16)(ctx->active_count - physical);
}

void gv89_get_stats(const gv89_context *ctx, gv89_stats *stats)
{
    if (ctx == 0 || stats == 0) return;
    *stats = ctx->stats;
}

gv89_u32 gv89_context_bytes(void)
{
    return (gv89_u32)sizeof(gv89_context);
}

gv89_u32 gv89_voice_bytes(void)
{
    return (gv89_u32)sizeof(gv89_voice);
}
