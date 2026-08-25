#include "3d_contact_trigger89.h"
#include <string.h>

static void ct89_set_error(CT89_Context *ctx, int error_code)
{
    if (ctx) ctx->last_error = error_code;
}

static unsigned int ct89_next_generation(unsigned int generation)
{
    ++generation;
    if (generation >= CT89_TRIGGER_GENERATION_LIMIT) generation = 0U;
    return generation;
}

static CT89_Trigger ct89_pack_trigger(int index, unsigned int generation)
{
    unsigned long raw;
    raw = (unsigned long)generation * (unsigned long)CT89_MAX_TRIGGERS +
          (unsigned long)index;
    if (raw > (unsigned long)INT_MAX) return CT89_TRIGGER_INVALID;
    return (CT89_Trigger)raw;
}

static int ct89_trigger_index(CT89_Trigger trigger)
{
    if (trigger < 0) return -1;
    return (int)((unsigned long)trigger % (unsigned long)CT89_MAX_TRIGGERS);
}

static unsigned int ct89_trigger_generation(CT89_Trigger trigger)
{
    if (trigger < 0) return 0U;
    return (unsigned int)((unsigned long)trigger /
                          (unsigned long)CT89_MAX_TRIGGERS);
}

static CT89_TriggerSlot *ct89_slot(CT89_Context *ctx, CT89_Trigger trigger)
{
    int index;
    CT89_TriggerSlot *slot;
    if (!ctx || trigger < 0) return 0;
    index = ct89_trigger_index(trigger);
    if (index < 0 || index >= CT89_MAX_TRIGGERS) return 0;
    slot = &ctx->triggers[index];
    if (!slot->used) return 0;
    if (slot->generation != ct89_trigger_generation(trigger)) return 0;
    return slot;
}

static const CT89_TriggerSlot *ct89_slot_const(const CT89_Context *ctx,
                                                CT89_Trigger trigger)
{
    int index;
    const CT89_TriggerSlot *slot;
    if (!ctx || trigger < 0) return 0;
    index = ct89_trigger_index(trigger);
    if (index < 0 || index >= CT89_MAX_TRIGGERS) return 0;
    slot = &ctx->triggers[index];
    if (!slot->used) return 0;
    if (slot->generation != ct89_trigger_generation(trigger)) return 0;
    return slot;
}

static unsigned int ct89_relevant_relations(const CT89_TriggerSlot *slot,
                                              unsigned int relations)
{
    if (!slot) return CT89_RELATION_NONE;
    if (slot->desc.sensor_mode == CT89_SENSOR_TOUCH)
        return relations & CT89_RELATION_TOUCH;
    if (slot->desc.sensor_mode == CT89_SENSOR_PROXIMITY)
        return relations & CT89_RELATION_NEAR;
    if (slot->desc.sensor_mode == CT89_SENSOR_TOUCH_OR_PROXIMITY)
        return relations & (CT89_RELATION_TOUCH | CT89_RELATION_NEAR);
    return CT89_RELATION_NONE;
}

static CT89_PairSlot *ct89_find_pair(CT89_Context *ctx,
                                      CT89_Trigger trigger,
                                      CT89_Subject other)
{
    int i;
    if (!ctx || other == CT89_SUBJECT_INVALID) return 0;
    for (i = 0; i < CT89_MAX_PAIRS; ++i) {
        if (ctx->pairs[i].used &&
            ctx->pairs[i].trigger == trigger &&
            ctx->pairs[i].other == other)
            return &ctx->pairs[i];
    }
    return 0;
}

static const CT89_PairSlot *ct89_find_pair_const(const CT89_Context *ctx,
                                                  CT89_Trigger trigger,
                                                  CT89_Subject other)
{
    int i;
    if (!ctx || other == CT89_SUBJECT_INVALID) return 0;
    for (i = 0; i < CT89_MAX_PAIRS; ++i) {
        if (ctx->pairs[i].used &&
            ctx->pairs[i].trigger == trigger &&
            ctx->pairs[i].other == other)
            return &ctx->pairs[i];
    }
    return 0;
}

static CT89_PairSlot *ct89_alloc_pair(CT89_Context *ctx,
                                       CT89_Trigger trigger,
                                       CT89_Subject other)
{
    int i;
    CT89_PairSlot *pair;
    pair = ct89_find_pair(ctx, trigger, other);
    if (pair) return pair;
    for (i = 0; i < CT89_MAX_PAIRS; ++i) {
        if (!ctx->pairs[i].used) {
            memset(&ctx->pairs[i], 0, sizeof(ctx->pairs[i]));
            ctx->pairs[i].used = 1U;
            ctx->pairs[i].trigger = trigger;
            ctx->pairs[i].other = other;
            return &ctx->pairs[i];
        }
    }
    ctx->stats.pair_overflows += 1UL;
    ct89_set_error(ctx, CT89_ERR_PAIR_FULL);
    return 0;
}

static void ct89_remove_pairs_for_trigger(CT89_Context *ctx,
                                           CT89_Trigger trigger)
{
    int i;
    if (!ctx) return;
    for (i = 0; i < CT89_MAX_PAIRS; ++i) {
        if (ctx->pairs[i].used && ctx->pairs[i].trigger == trigger)
            memset(&ctx->pairs[i], 0, sizeof(ctx->pairs[i]));
    }
}

static int ct89_candidate_allowed(CT89_Context *ctx,
                                   const CT89_Probe *probe,
                                   const CT89_Candidate *candidate)
{
    if (!ctx || !probe || !candidate) return 0;
    if (candidate->subject == CT89_SUBJECT_INVALID) return 0;
    if (candidate->subject == probe->owner) return 0;
    if (probe->category_mask != 0UL &&
        (candidate->category_mask & probe->category_mask) == 0UL)
        return 0;
    if (ctx->filter_provider.accept)
        return ctx->filter_provider.accept(ctx->filter_provider.user,
                                           probe, candidate) ? 1 : 0;
    return 1;
}

static void ct89_emit(CT89_Context *ctx,
                       const CT89_TriggerSlot *slot,
                       CT89_Event *event)
{
    unsigned long bit;
    if (!ctx || !slot || !event) return;
    bit = CT89_EVENT_BIT(event->event_type);
    if ((slot->desc.notify_event_mask & bit) != 0UL &&
        ctx->event_provider.emit)
        ctx->event_provider.emit(ctx->event_provider.user, event);
}

static int ct89_destroy_subject(CT89_Context *ctx, CT89_Subject subject)
{
    int ok;
    if (!ctx || subject == CT89_SUBJECT_INVALID) return 0;
    ctx->stats.lifecycle_requests += 1UL;
    if (!ctx->lifecycle_provider.destroy_subject) {
        ctx->stats.lifecycle_failures += 1UL;
        return 0;
    }
    ok = ctx->lifecycle_provider.destroy_subject(
        ctx->lifecycle_provider.user, subject);
    if (!ok) ctx->stats.lifecycle_failures += 1UL;
    return ok ? 1 : 0;
}

static int ct89_consume(CT89_Context *ctx,
                         CT89_Trigger trigger,
                         CT89_TriggerSlot *slot,
                         CT89_Subject other)
{
    int ok_a;
    int ok_b;
    if (!ctx || !slot) return 0;
    switch (slot->desc.consume_policy) {
    case CT89_CONSUME_KEEP:
        return 1;
    case CT89_CONSUME_DISABLE_TRIGGER:
        slot->enabled = 0U;
        return 1;
    case CT89_CONSUME_DESTROY_OWNER:
        ok_a = ct89_destroy_subject(ctx, slot->desc.owner);
        if (ok_a) slot->enabled = 0U;
        return ok_a;
    case CT89_CONSUME_DESTROY_OTHER:
        return ct89_destroy_subject(ctx, other);
    case CT89_CONSUME_DESTROY_BOTH:
        ok_a = ct89_destroy_subject(ctx, other);
        ok_b = ct89_destroy_subject(ctx, slot->desc.owner);
        if (ok_b) slot->enabled = 0U;
        return ok_a && ok_b;
    default:
        (void)trigger;
        return 0;
    }
}

static int ct89_execute_action(CT89_Context *ctx,
                                CT89_Trigger trigger,
                                CT89_TriggerSlot *slot,
                                CT89_Event *event)
{
    unsigned long bit;
    int status;
    if (!ctx || !slot || !event) return CT89_ACTION_UNHANDLED;
    bit = CT89_EVENT_BIT(event->event_type);
    if ((slot->desc.action_event_mask & bit) == 0UL)
        return CT89_ACTION_UNHANDLED;
    if (slot->cooldown_left_ms != 0UL) {
        event->action_status = CT89_ACTION_DEFERRED;
        ctx->stats.actions_deferred += 1UL;
        return CT89_ACTION_DEFERRED;
    }
    if (slot->desc.max_activations != 0U &&
        slot->activation_count >= slot->desc.max_activations) {
        event->action_status = CT89_ACTION_REJECTED;
        ctx->stats.actions_rejected += 1UL;
        return CT89_ACTION_REJECTED;
    }
    if (!ctx->action_provider.execute)
        status = CT89_ACTION_UNHANDLED;
    else
        status = ctx->action_provider.execute(ctx->action_provider.user,
                                              event);
    event->action_status = status;
    if (status == CT89_ACTION_ACCEPTED) {
        ctx->stats.actions_accepted += 1UL;
        slot->activation_count += 1U;
        slot->cooldown_left_ms = slot->desc.cooldown_ms;
        (void)ct89_consume(ctx, trigger, slot, event->other);
        if (slot->desc.max_activations != 0U &&
            slot->activation_count >= slot->desc.max_activations)
            slot->enabled = 0U;
    } else if (status == CT89_ACTION_REJECTED) {
        ctx->stats.actions_rejected += 1UL;
    } else if (status == CT89_ACTION_DEFERRED) {
        ctx->stats.actions_deferred += 1UL;
    } else {
        ctx->stats.actions_unhandled += 1UL;
        status = CT89_ACTION_UNHANDLED;
        event->action_status = status;
    }
    return status;
}

static void ct89_build_event(const CT89_TriggerSlot *slot,
                              CT89_Trigger trigger,
                              CT89_Subject other,
                              int event_type,
                              unsigned int relations,
                              CT89_Event *event)
{
    memset(event, 0, sizeof(*event));
    event->trigger = trigger;
    event->owner = slot->desc.owner;
    event->other = other;
    event->event_type = event_type;
    event->relation_flags = relations;
    event->action_id = slot->desc.action_id;
    event->action_arg0 = slot->desc.action_arg0;
    event->action_arg1 = slot->desc.action_arg1;
    event->user_tag = slot->desc.user_tag;
    event->action_status = CT89_ACTION_UNHANDLED;
}

static void ct89_fire_event(CT89_Context *ctx,
                             CT89_Trigger trigger,
                             CT89_TriggerSlot *slot,
                             CT89_Subject other,
                             int event_type,
                             unsigned int relations)
{
    CT89_Event event;
    if (!ctx || !slot) return;
    ct89_build_event(slot, trigger, other, event_type, relations, &event);
    ct89_emit(ctx, slot, &event);
    if (slot->enabled)
        (void)ct89_execute_action(ctx, trigger, slot, &event);
}

static int ct89_gather_provider(CT89_Context *ctx,
                                 CT89_Trigger trigger,
                                 CT89_TriggerSlot *slot,
                                 const CT89_SensorProvider *provider,
                                 unsigned int relation_flag)
{
    CT89_Candidate candidates[CT89_MAX_GATHER];
    CT89_Probe probe;
    CT89_PairSlot *pair;
    int count;
    int i;
    if (!ctx || !slot || !provider || !provider->gather) return 1;
    memset(&probe, 0, sizeof(probe));
    probe.trigger = trigger;
    probe.owner = slot->desc.owner;
    probe.sensor_mode = slot->desc.sensor_mode;
    probe.radius_fx = slot->desc.radius_fx;
    probe.category_mask = slot->desc.category_mask;
    probe.user_tag = slot->desc.user_tag;
    memset(candidates, 0, sizeof(candidates));
    count = provider->gather(provider->user, &probe,
                             candidates, CT89_MAX_GATHER);
    if (count < 0) {
        ct89_set_error(ctx, CT89_ERR_PROVIDER);
        return 0;
    }
    if (count > CT89_MAX_GATHER) count = CT89_MAX_GATHER;
    for (i = 0; i < count; ++i) {
        ctx->stats.candidates += 1UL;
        if (!ct89_candidate_allowed(ctx, &probe, &candidates[i])) continue;
        pair = ct89_alloc_pair(ctx, trigger, candidates[i].subject);
        if (!pair) continue;
        pair->current_relations |= relation_flag;
        pair->category_mask |= candidates[i].category_mask;
        pair->distance_fx = candidates[i].distance_fx;
    }
    return 1;
}

void ct89_init(CT89_Context *ctx)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->initialized = 1;
    ctx->last_error = CT89_OK;
}

void ct89_reset(CT89_Context *ctx)
{
    CT89_SensorProvider contact_provider;
    CT89_SensorProvider proximity_provider;
    CT89_FilterProvider filter_provider;
    CT89_ActionProvider action_provider;
    CT89_EventProvider event_provider;
    CT89_LifecycleProvider lifecycle_provider;
    if (!ctx) return;
    contact_provider = ctx->contact_provider;
    proximity_provider = ctx->proximity_provider;
    filter_provider = ctx->filter_provider;
    action_provider = ctx->action_provider;
    event_provider = ctx->event_provider;
    lifecycle_provider = ctx->lifecycle_provider;
    ct89_init(ctx);
    ctx->contact_provider = contact_provider;
    ctx->proximity_provider = proximity_provider;
    ctx->filter_provider = filter_provider;
    ctx->action_provider = action_provider;
    ctx->event_provider = event_provider;
    ctx->lifecycle_provider = lifecycle_provider;
}

int ct89_last_error(const CT89_Context *ctx)
{
    return ctx ? ctx->last_error : CT89_ERR_ARGUMENT;
}

const char *ct89_error_string(int error_code)
{
    switch (error_code) {
    case CT89_OK: return "ok";
    case CT89_ERR_ARGUMENT: return "bad argument";
    case CT89_ERR_FULL: return "trigger table full";
    case CT89_ERR_BAD_TRIGGER: return "bad trigger";
    case CT89_ERR_STALE_TRIGGER: return "stale trigger";
    case CT89_ERR_PROVIDER: return "provider failure";
    case CT89_ERR_PAIR_FULL: return "pair table full";
    case CT89_ERR_NOT_ACTIVE: return "subject is not active in trigger";
    case CT89_ERR_DISABLED: return "trigger disabled";
    default: return "unknown";
    }
}

void ct89_sensor_provider_init(CT89_SensorProvider *provider)
{ if (provider) memset(provider, 0, sizeof(*provider)); }
void ct89_filter_provider_init(CT89_FilterProvider *provider)
{ if (provider) memset(provider, 0, sizeof(*provider)); }
void ct89_action_provider_init(CT89_ActionProvider *provider)
{ if (provider) memset(provider, 0, sizeof(*provider)); }
void ct89_event_provider_init(CT89_EventProvider *provider)
{ if (provider) memset(provider, 0, sizeof(*provider)); }
void ct89_lifecycle_provider_init(CT89_LifecycleProvider *provider)
{ if (provider) memset(provider, 0, sizeof(*provider)); }

void ct89_set_contact_provider(CT89_Context *ctx,
                               const CT89_SensorProvider *provider)
{
    if (!ctx) return;
    if (provider) ctx->contact_provider = *provider;
    else memset(&ctx->contact_provider, 0, sizeof(ctx->contact_provider));
}

void ct89_set_proximity_provider(CT89_Context *ctx,
                                 const CT89_SensorProvider *provider)
{
    if (!ctx) return;
    if (provider) ctx->proximity_provider = *provider;
    else memset(&ctx->proximity_provider, 0,
                sizeof(ctx->proximity_provider));
}

void ct89_set_filter_provider(CT89_Context *ctx,
                              const CT89_FilterProvider *provider)
{
    if (!ctx) return;
    if (provider) ctx->filter_provider = *provider;
    else memset(&ctx->filter_provider, 0, sizeof(ctx->filter_provider));
}

void ct89_set_action_provider(CT89_Context *ctx,
                              const CT89_ActionProvider *provider)
{
    if (!ctx) return;
    if (provider) ctx->action_provider = *provider;
    else memset(&ctx->action_provider, 0, sizeof(ctx->action_provider));
}

void ct89_set_event_provider(CT89_Context *ctx,
                             const CT89_EventProvider *provider)
{
    if (!ctx) return;
    if (provider) ctx->event_provider = *provider;
    else memset(&ctx->event_provider, 0, sizeof(ctx->event_provider));
}

void ct89_set_lifecycle_provider(CT89_Context *ctx,
                                 const CT89_LifecycleProvider *provider)
{
    if (!ctx) return;
    if (provider) ctx->lifecycle_provider = *provider;
    else memset(&ctx->lifecycle_provider, 0,
                sizeof(ctx->lifecycle_provider));
}

void ct89_trigger_desc_defaults(CT89_TriggerDesc *desc)
{
    if (!desc) return;
    memset(desc, 0, sizeof(*desc));
    desc->sensor_mode = CT89_SENSOR_TOUCH;
    desc->notify_event_mask = CT89_EVENT_MASK_ALL;
    desc->action_event_mask = CT89_EVENT_MASK_ENTER;
    desc->consume_policy = CT89_CONSUME_KEEP;
}

CT89_Trigger ct89_trigger_create(CT89_Context *ctx,
                                 const CT89_TriggerDesc *desc)
{
    int i;
    CT89_TriggerSlot *slot;
    CT89_Trigger trigger;
    if (!ctx || !ctx->initialized || !desc ||
        desc->owner == CT89_SUBJECT_INVALID) {
        ct89_set_error(ctx, CT89_ERR_ARGUMENT);
        return CT89_TRIGGER_INVALID;
    }
    if (desc->sensor_mode < CT89_SENSOR_TOUCH ||
        desc->sensor_mode > CT89_SENSOR_MANUAL) {
        ct89_set_error(ctx, CT89_ERR_ARGUMENT);
        return CT89_TRIGGER_INVALID;
    }
    if ((desc->sensor_mode == CT89_SENSOR_PROXIMITY ||
         desc->sensor_mode == CT89_SENSOR_TOUCH_OR_PROXIMITY) &&
        desc->radius_fx < 0L) {
        ct89_set_error(ctx, CT89_ERR_ARGUMENT);
        return CT89_TRIGGER_INVALID;
    }
    for (i = 0; i < CT89_MAX_TRIGGERS; ++i) {
        if (!ctx->triggers[i].used) {
            slot = &ctx->triggers[i];
            slot->used = 1U;
            slot->enabled = 1U;
            slot->desc = *desc;
            slot->cooldown_left_ms = 0UL;
            slot->activation_count = 0U;
            trigger = ct89_pack_trigger(i, slot->generation);
            if (trigger == CT89_TRIGGER_INVALID) {
                memset(slot, 0, sizeof(*slot));
                ct89_set_error(ctx, CT89_ERR_FULL);
                return CT89_TRIGGER_INVALID;
            }
            ct89_set_error(ctx, CT89_OK);
            return trigger;
        }
    }
    ct89_set_error(ctx, CT89_ERR_FULL);
    return CT89_TRIGGER_INVALID;
}

int ct89_trigger_destroy(CT89_Context *ctx, CT89_Trigger trigger)
{
    int index;
    unsigned int generation;
    CT89_TriggerSlot *slot;
    if (!ctx || trigger < 0) {
        ct89_set_error(ctx, CT89_ERR_BAD_TRIGGER);
        return 0;
    }
    index = ct89_trigger_index(trigger);
    if (index < 0 || index >= CT89_MAX_TRIGGERS) {
        ct89_set_error(ctx, CT89_ERR_BAD_TRIGGER);
        return 0;
    }
    slot = &ctx->triggers[index];
    if (!slot->used) {
        ct89_set_error(ctx, CT89_ERR_BAD_TRIGGER);
        return 0;
    }
    if (slot->generation != ct89_trigger_generation(trigger)) {
        ct89_set_error(ctx, CT89_ERR_STALE_TRIGGER);
        return 0;
    }
    generation = ct89_next_generation(slot->generation);
    ct89_remove_pairs_for_trigger(ctx, trigger);
    memset(slot, 0, sizeof(*slot));
    slot->generation = generation;
    ct89_set_error(ctx, CT89_OK);
    return 1;
}

int ct89_trigger_set_enabled(CT89_Context *ctx,
                             CT89_Trigger trigger, int enabled)
{
    CT89_TriggerSlot *slot;
    slot = ct89_slot(ctx, trigger);
    if (!slot) {
        ct89_set_error(ctx, CT89_ERR_BAD_TRIGGER);
        return 0;
    }
    slot->enabled = enabled ? 1U : 0U;
    if (!slot->enabled) ct89_remove_pairs_for_trigger(ctx, trigger);
    ct89_set_error(ctx, CT89_OK);
    return 1;
}

int ct89_trigger_is_enabled(const CT89_Context *ctx,
                            CT89_Trigger trigger)
{
    const CT89_TriggerSlot *slot;
    slot = ct89_slot_const(ctx, trigger);
    return slot && slot->enabled ? 1 : 0;
}

const CT89_TriggerDesc *ct89_trigger_desc(const CT89_Context *ctx,
                                          CT89_Trigger trigger)
{
    const CT89_TriggerSlot *slot;
    slot = ct89_slot_const(ctx, trigger);
    return slot ? &slot->desc : 0;
}

unsigned int ct89_trigger_activation_count(const CT89_Context *ctx,
                                           CT89_Trigger trigger)
{
    const CT89_TriggerSlot *slot;
    slot = ct89_slot_const(ctx, trigger);
    return slot ? slot->activation_count : 0U;
}

static void ct89_tick_cooldowns(CT89_Context *ctx, unsigned long dt_ms)
{
    int i;
    if (!ctx) return;
    for (i = 0; i < CT89_MAX_TRIGGERS; ++i) {
        if (!ctx->triggers[i].used) continue;
        if (ctx->triggers[i].cooldown_left_ms > dt_ms)
            ctx->triggers[i].cooldown_left_ms -= dt_ms;
        else
            ctx->triggers[i].cooldown_left_ms = 0UL;
    }
}

int ct89_step(CT89_Context *ctx, unsigned long dt_ms)
{
    int i;
    CT89_Trigger trigger;
    CT89_TriggerSlot *slot;
    CT89_PairSlot *pair;
    unsigned int previous_active;
    unsigned int current_active;
    unsigned int event_relations;
    if (!ctx || !ctx->initialized) return 0;
    ctx->last_error = CT89_OK;
    ctx->stats.steps += 1UL;
    ct89_tick_cooldowns(ctx, dt_ms);
    for (i = 0; i < CT89_MAX_PAIRS; ++i) {
        if (!ctx->pairs[i].used) continue;
        ctx->pairs[i].current_relations = CT89_RELATION_NONE;
        ctx->pairs[i].category_mask = 0UL;
        ctx->pairs[i].distance_fx = 0L;
    }
    for (i = 0; i < CT89_MAX_TRIGGERS; ++i) {
        slot = &ctx->triggers[i];
        if (!slot->used || !slot->enabled) continue;
        trigger = ct89_pack_trigger(i, slot->generation);
        if (slot->desc.sensor_mode == CT89_SENSOR_TOUCH ||
            slot->desc.sensor_mode == CT89_SENSOR_TOUCH_OR_PROXIMITY) {
            if (!ct89_gather_provider(ctx, trigger, slot,
                    &ctx->contact_provider, CT89_RELATION_TOUCH))
                return 0;
        }
        if (slot->desc.sensor_mode == CT89_SENSOR_PROXIMITY ||
            slot->desc.sensor_mode == CT89_SENSOR_TOUCH_OR_PROXIMITY) {
            if (!ct89_gather_provider(ctx, trigger, slot,
                    &ctx->proximity_provider, CT89_RELATION_NEAR))
                return 0;
        }
    }
    for (i = 0; i < CT89_MAX_PAIRS; ++i) {
        pair = &ctx->pairs[i];
        if (!pair->used) continue;
        slot = ct89_slot(ctx, pair->trigger);
        if (!slot || !slot->enabled) {
            memset(pair, 0, sizeof(*pair));
            continue;
        }
        previous_active = ct89_relevant_relations(slot,
                                                   pair->previous_relations);
        current_active = ct89_relevant_relations(slot,
                                                  pair->current_relations);
        if (!previous_active && current_active) {
            ctx->stats.enters += 1UL;
            ct89_fire_event(ctx, pair->trigger, slot, pair->other,
                            CT89_EVENT_ENTER, current_active);
        } else if (previous_active && current_active) {
            ctx->stats.stays += 1UL;
            ct89_fire_event(ctx, pair->trigger, slot, pair->other,
                            CT89_EVENT_STAY, current_active);
        } else if (previous_active && !current_active) {
            ctx->stats.exits += 1UL;
            event_relations = previous_active;
            ct89_fire_event(ctx, pair->trigger, slot, pair->other,
                            CT89_EVENT_EXIT, event_relations);
        }
        if (!pair->used) continue;
        if (!slot->enabled) {
            memset(pair, 0, sizeof(*pair));
            continue;
        }
        pair->previous_relations = pair->current_relations;
        if (!current_active && !previous_active)
            memset(pair, 0, sizeof(*pair));
        else if (!current_active && previous_active)
            memset(pair, 0, sizeof(*pair));
    }
    return 1;
}

int ct89_activate(CT89_Context *ctx,
                  CT89_Trigger trigger,
                  CT89_Subject activator)
{
    CT89_TriggerSlot *slot;
    CT89_PairSlot *pair;
    unsigned int relations;
    CT89_Event event;
    int status;
    if (!ctx || activator == CT89_SUBJECT_INVALID) {
        ct89_set_error(ctx, CT89_ERR_ARGUMENT);
        return CT89_ACTION_UNHANDLED;
    }
    slot = ct89_slot(ctx, trigger);
    if (!slot) {
        ct89_set_error(ctx, CT89_ERR_BAD_TRIGGER);
        return CT89_ACTION_UNHANDLED;
    }
    if (!slot->enabled) {
        ct89_set_error(ctx, CT89_ERR_DISABLED);
        return CT89_ACTION_UNHANDLED;
    }
    relations = CT89_RELATION_NONE;
    if (slot->desc.sensor_mode != CT89_SENSOR_MANUAL) {
        pair = ct89_find_pair(ctx, trigger, activator);
        if (!pair) {
            ct89_set_error(ctx, CT89_ERR_NOT_ACTIVE);
            return CT89_ACTION_UNHANDLED;
        }
        relations = ct89_relevant_relations(slot,
                                             pair->previous_relations);
        if (!relations) {
            ct89_set_error(ctx, CT89_ERR_NOT_ACTIVE);
            return CT89_ACTION_UNHANDLED;
        }
    }
    ctx->stats.activations += 1UL;
    ct89_build_event(slot, trigger, activator, CT89_EVENT_ACTIVATE,
                     relations, &event);
    ct89_emit(ctx, slot, &event);
    status = ct89_execute_action(ctx, trigger, slot, &event);
    ct89_set_error(ctx, CT89_OK);
    return status;
}

int ct89_subject_is_active(const CT89_Context *ctx,
                           CT89_Trigger trigger,
                           CT89_Subject subject)
{
    const CT89_TriggerSlot *slot;
    const CT89_PairSlot *pair;
    slot = ct89_slot_const(ctx, trigger);
    if (!slot || !slot->enabled) return 0;
    pair = ct89_find_pair_const(ctx, trigger, subject);
    if (!pair) return 0;
    return ct89_relevant_relations(slot, pair->previous_relations) ? 1 : 0;
}

unsigned int ct89_subject_relations(const CT89_Context *ctx,
                                    CT89_Trigger trigger,
                                    CT89_Subject subject)
{
    const CT89_TriggerSlot *slot;
    const CT89_PairSlot *pair;
    slot = ct89_slot_const(ctx, trigger);
    if (!slot || !slot->enabled) return CT89_RELATION_NONE;
    pair = ct89_find_pair_const(ctx, trigger, subject);
    if (!pair) return CT89_RELATION_NONE;
    return ct89_relevant_relations(slot, pair->previous_relations);
}

const CT89_Stats *ct89_stats(const CT89_Context *ctx)
{
    return ctx ? &ctx->stats : 0;
}

CT89_FX ct89_fx_from_int(int value)
{
    long v;
    v = (long)value;
    if (v > LONG_MAX / CT89_FX_ONE) return LONG_MAX;
    if (v < LONG_MIN / CT89_FX_ONE) return LONG_MIN;
    return (CT89_FX)(v * CT89_FX_ONE);
}

int ct89_fx_to_int(CT89_FX value)
{
    return (int)(value / CT89_FX_ONE);
}
