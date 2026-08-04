#include "numsys.h"

#ifndef NULL
#define NULL ((void *)0)
#endif

static int ns_name_equal(const char *a, const char *b)
{
    int i;

    if (a == NULL || b == NULL) {
        return NS_FALSE;
    }

    i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) {
            return NS_FALSE;
        }
        i++;
    }

    if (a[i] == '\0' && b[i] == '\0') {
        return NS_TRUE;
    }

    return NS_FALSE;
}

static void ns_name_copy(char *dst, const char *src)
{
    int i;

    if (dst == NULL) {
        return;
    }

    if (src == NULL) {
        dst[0] = '\0';
        return;
    }

    i = 0;
    while (i < NS_NAME_MAX && src[i] != '\0') {
        dst[i] = src[i];
        i++;
    }

    dst[i] = '\0';
}

static int ns_valid_kind(int kind)
{
    if (kind == NS_KIND_FIXED) {
        return NS_TRUE;
    }
    if (kind == NS_KIND_INT) {
        return NS_TRUE;
    }
    if (kind == NS_KIND_BOOL) {
        return NS_TRUE;
    }
    return NS_FALSE;
}

static int ns_valid_scope(int scope)
{
    if (scope == NS_SCOPE_GLOBAL) {
        return NS_TRUE;
    }
    if (scope == NS_SCOPE_FRAME) {
        return NS_TRUE;
    }
    if (scope == NS_SCOPE_INSTANCE) {
        return NS_TRUE;
    }
    if (scope == NS_SCOPE_FAMILY) {
        return NS_TRUE;
    }
    if (scope == NS_SCOPE_LOCAL) {
        return NS_TRUE;
    }
    if (scope == NS_SCOPE_COMPONENT) {
        return NS_TRUE;
    }
    return NS_FALSE;
}

static int ns_valid_overflow(int overflow)
{
    if (overflow == NS_OVERFLOW_NONE) {
        return NS_TRUE;
    }
    if (overflow == NS_OVERFLOW_CLAMP) {
        return NS_TRUE;
    }
    if (overflow == NS_OVERFLOW_WRAP) {
        return NS_TRUE;
    }
    if (overflow == NS_OVERFLOW_BOUNCE) {
        return NS_TRUE;
    }
    if (overflow == NS_OVERFLOW_SPILL) {
        return NS_TRUE;
    }
    return NS_FALSE;
}

static int ns_valid_edge(int edge)
{
    if (edge == NS_EDGE_UP) {
        return NS_TRUE;
    }
    if (edge == NS_EDGE_DOWN) {
        return NS_TRUE;
    }
    if (edge == NS_EDGE_ANY) {
        return NS_TRUE;
    }
    return NS_FALSE;
}

static int ns_valid_mod_target(int target)
{
    if (target == NS_MOD_TARGET_VALUE) {
        return NS_TRUE;
    }
    if (target == NS_MOD_TARGET_MIN) {
        return NS_TRUE;
    }
    if (target == NS_MOD_TARGET_MAX) {
        return NS_TRUE;
    }
    if (target == NS_MOD_TARGET_REGEN) {
        return NS_TRUE;
    }
    if (target == NS_MOD_TARGET_DRAIN) {
        return NS_TRUE;
    }
    return NS_FALSE;
}

static int ns_valid_mod_mode(int mode)
{
    if (mode == NS_MOD_FLAT) {
        return NS_TRUE;
    }
    if (mode == NS_MOD_PERCENT_ADD) {
        return NS_TRUE;
    }
    if (mode == NS_MOD_PERCENT_MUL) {
        return NS_TRUE;
    }
    return NS_FALSE;
}

static int ns_valid_derived_mode(int mode)
{
    if (mode == NS_DERIVED_COPY) {
        return NS_TRUE;
    }
    if (mode == NS_DERIVED_SUM) {
        return NS_TRUE;
    }
    if (mode == NS_DERIVED_SUB) {
        return NS_TRUE;
    }
    if (mode == NS_DERIVED_PRODUCT) {
        return NS_TRUE;
    }
    if (mode == NS_DERIVED_RATIO) {
        return NS_TRUE;
    }
    if (mode == NS_DERIVED_MIN) {
        return NS_TRUE;
    }
    if (mode == NS_DERIVED_MAX) {
        return NS_TRUE;
    }
    if (mode == NS_DERIVED_PERCENT) {
        return NS_TRUE;
    }
    if (mode == NS_DERIVED_LERP) {
        return NS_TRUE;
    }
    return NS_FALSE;
}

static int ns_valid_bind_kind(int kind)
{
    if (kind == NS_BIND_BAR) {
        return NS_TRUE;
    }
    if (kind == NS_BIND_COUNTER) {
        return NS_TRUE;
    }
    if (kind == NS_BIND_ECG) {
        return NS_TRUE;
    }
    if (kind == NS_BIND_ICON) {
        return NS_TRUE;
    }
    if (kind == NS_BIND_DEBUG) {
        return NS_TRUE;
    }
    if (kind == NS_BIND_CUSTOM) {
        return NS_TRUE;
    }
    return NS_FALSE;
}

static int ns_valid_type_id(const NS_World *world, ns_id type_id)
{
    if (world == NULL) {
        return NS_FALSE;
    }
    if (type_id < 0 || type_id >= NS_MAX_TYPES) {
        return NS_FALSE;
    }
    if (world->types[type_id].used == NS_FALSE) {
        return NS_FALSE;
    }
    return NS_TRUE;
}

static int ns_valid_value_id(const NS_World *world, ns_id value_id)
{
    if (world == NULL) {
        return NS_FALSE;
    }
    if (value_id < 0 || value_id >= NS_MAX_VALUES) {
        return NS_FALSE;
    }
    if (world->values[value_id].used == NS_FALSE) {
        return NS_FALSE;
    }
    return NS_TRUE;
}

static int ns_valid_template_id(const NS_World *world, ns_id template_id)
{
    if (world == NULL) {
        return NS_FALSE;
    }
    if (template_id < 0 || template_id >= NS_MAX_TEMPLATES) {
        return NS_FALSE;
    }
    if (world->templates[template_id].used == NS_FALSE) {
        return NS_FALSE;
    }
    return NS_TRUE;
}

static int ns_valid_derived_id(const NS_World *world, ns_id derived_id)
{
    if (world == NULL) {
        return NS_FALSE;
    }
    if (derived_id < 0 || derived_id >= NS_MAX_DERIVED) {
        return NS_FALSE;
    }
    if (world->derived[derived_id].used == NS_FALSE) {
        return NS_FALSE;
    }
    return NS_TRUE;
}

static int ns_valid_binding_id(const NS_World *world, ns_id binding_id)
{
    if (world == NULL) {
        return NS_FALSE;
    }
    if (binding_id < 0 || binding_id >= NS_MAX_BINDINGS) {
        return NS_FALSE;
    }
    if (world->bindings[binding_id].used == NS_FALSE) {
        return NS_FALSE;
    }
    return NS_TRUE;
}

static ns_fx ns_min_fx(ns_fx a, ns_fx b)
{
    if (a < b) {
        return a;
    }
    return b;
}

static ns_fx ns_max_fx(ns_fx a, ns_fx b)
{
    if (a > b) {
        return a;
    }
    return b;
}

ns_fx ns_fx_mul(ns_fx a, ns_fx b)
{
    return (a * b) / NS_FX_ONE;
}

ns_fx ns_fx_div(ns_fx a, ns_fx b)
{
    if (b == NS_FX_ZERO) {
        return NS_FX_ZERO;
    }
    return (a * NS_FX_ONE) / b;
}

ns_fx ns_fx_clamp(ns_fx value, ns_fx min_value, ns_fx max_value)
{
    if (min_value > max_value) {
        return value;
    }
    if (value < min_value) {
        return min_value;
    }
    if (value > max_value) {
        return max_value;
    }
    return value;
}

ns_fx ns_fx_lerp(ns_fx a, ns_fx b, ns_fx t)
{
    return a + ns_fx_mul(b - a, t);
}

static ns_fx ns_quantize_value(int kind, ns_fx value)
{
    ns_fx whole;

    if (kind == NS_KIND_BOOL) {
        if (value <= NS_FX_ZERO) {
            return NS_FX_ZERO;
        }
        return NS_FX_ONE;
    }

    if (kind == NS_KIND_INT) {
        whole = value / NS_FX_ONE;
        return whole * NS_FX_ONE;
    }

    return value;
}

static ns_fx ns_apply_overflow_calc(const NS_Value *slot,
                                    ns_fx raw_value,
                                    int *out_spill_dir,
                                    ns_fx *out_spill_amount)
{
    ns_fx result;
    ns_fx range;
    ns_fx spill;
    int guard;

    if (out_spill_dir != NULL) {
        *out_spill_dir = 0;
    }
    if (out_spill_amount != NULL) {
        *out_spill_amount = NS_FX_ZERO;
    }

    if (slot == NULL) {
        return raw_value;
    }

    result = raw_value;

    if (slot->overflow == NS_OVERFLOW_NONE) {
        return result;
    }

    if (slot->min_value > slot->max_value) {
        return result;
    }

    if (slot->overflow == NS_OVERFLOW_CLAMP) {
        return ns_fx_clamp(result, slot->min_value, slot->max_value);
    }

    if (slot->overflow == NS_OVERFLOW_SPILL) {
        if (result > slot->max_value) {
            spill = result - slot->max_value;
            if (out_spill_dir != NULL) {
                *out_spill_dir = 1;
            }
            if (out_spill_amount != NULL) {
                *out_spill_amount = spill;
            }
            return slot->max_value;
        }
        if (result < slot->min_value) {
            spill = slot->min_value - result;
            if (out_spill_dir != NULL) {
                *out_spill_dir = -1;
            }
            if (out_spill_amount != NULL) {
                *out_spill_amount = spill;
            }
            return slot->min_value;
        }
        return result;
    }

    range = slot->max_value - slot->min_value;
    if (range <= NS_FX_ZERO) {
        return slot->min_value;
    }

    if (slot->overflow == NS_OVERFLOW_WRAP) {
        guard = 0;
        while (result > slot->max_value && guard < 128) {
            result = result - range;
            guard++;
        }
        while (result < slot->min_value && guard < 256) {
            result = result + range;
            guard++;
        }
        return ns_fx_clamp(result, slot->min_value, slot->max_value);
    }

    if (slot->overflow == NS_OVERFLOW_BOUNCE) {
        guard = 0;
        while ((result > slot->max_value || result < slot->min_value) && guard < 256) {
            if (result > slot->max_value) {
                result = slot->max_value - (result - slot->max_value);
            } else if (result < slot->min_value) {
                result = slot->min_value + (slot->min_value - result);
            }
            guard++;
        }
        return ns_fx_clamp(result, slot->min_value, slot->max_value);
    }

    return result;
}

static int ns_push_event(NS_World *world,
                         int event_type,
                         ns_id value_id,
                         ns_fx previous_value,
                         ns_fx value,
                         ns_fx threshold,
                         ns_id threshold_id,
                         int user_code)
{
    NS_Event *event_data;
    NS_Value *slot;

    if (world == NULL) {
        return NS_ERR_NULL;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    if (world->event_count >= NS_MAX_EVENTS) {
        return NS_ERR_FULL;
    }

    slot = &world->values[value_id];
    event_data = &world->events[world->event_count];
    event_data->event_type = event_type;
    event_data->value_id = value_id;
    event_data->type_id = slot->type_id;
    event_data->owner = slot->owner;
    event_data->previous_value = previous_value;
    event_data->value = value;
    event_data->threshold = threshold;
    event_data->threshold_id = threshold_id;
    event_data->user_code = user_code;

    world->event_count++;
    return NS_OK;
}

static int ns_emit_basic_events(NS_World *world, ns_id value_id, ns_fx previous_value, ns_fx value)
{
    NS_Value *slot;
    int result;
    int temp;

    if (world == NULL) {
        return NS_ERR_NULL;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    result = ns_push_event(world,
                           NS_EVENT_CHANGED,
                           value_id,
                           previous_value,
                           value,
                           NS_FX_ZERO,
                           NS_INVALID_ID,
                           0);

    if (value > previous_value) {
        temp = ns_push_event(world,
                             NS_EVENT_INCREASED,
                             value_id,
                             previous_value,
                             value,
                             NS_FX_ZERO,
                             NS_INVALID_ID,
                             0);
        if (result == NS_OK && temp != NS_OK) {
            result = temp;
        }
    } else if (value < previous_value) {
        temp = ns_push_event(world,
                             NS_EVENT_DECREASED,
                             value_id,
                             previous_value,
                             value,
                             NS_FX_ZERO,
                             NS_INVALID_ID,
                             0);
        if (result == NS_OK && temp != NS_OK) {
            result = temp;
        }
    }

    slot = &world->values[value_id];
    if (previous_value > slot->min_value && value <= slot->min_value) {
        temp = ns_push_event(world,
                             NS_EVENT_EMPTY,
                             value_id,
                             previous_value,
                             value,
                             slot->min_value,
                             NS_INVALID_ID,
                             0);
        if (result == NS_OK && temp != NS_OK) {
            result = temp;
        }
    }

    if (previous_value < slot->max_value && value >= slot->max_value) {
        temp = ns_push_event(world,
                             NS_EVENT_FULL,
                             value_id,
                             previous_value,
                             value,
                             slot->max_value,
                             NS_INVALID_ID,
                             0);
        if (result == NS_OK && temp != NS_OK) {
            result = temp;
        }
    }

    return result;
}

static int ns_threshold_crossed(const NS_Threshold *threshold, ns_fx previous_value, ns_fx value)
{
    if (threshold == NULL) {
        return NS_FALSE;
    }

    if (threshold->edge == NS_EDGE_UP) {
        if (previous_value < threshold->threshold && value >= threshold->threshold) {
            return NS_TRUE;
        }
        return NS_FALSE;
    }

    if (threshold->edge == NS_EDGE_DOWN) {
        if (previous_value > threshold->threshold && value <= threshold->threshold) {
            return NS_TRUE;
        }
        return NS_FALSE;
    }

    if (threshold->edge == NS_EDGE_ANY) {
        if (previous_value < threshold->threshold && value >= threshold->threshold) {
            return NS_TRUE;
        }
        if (previous_value > threshold->threshold && value <= threshold->threshold) {
            return NS_TRUE;
        }
        return NS_FALSE;
    }

    return NS_FALSE;
}

static int ns_check_thresholds(NS_World *world, ns_id value_id, ns_fx previous_value, ns_fx value)
{
    int i;
    int result;
    int temp;
    NS_Threshold *threshold;

    if (world == NULL) {
        return NS_ERR_NULL;
    }

    result = NS_OK;
    i = 0;
    while (i < NS_MAX_THRESHOLDS) {
        threshold = &world->thresholds[i];
        if (threshold->used == NS_TRUE && threshold->value_id == value_id) {
            if (threshold->once == NS_FALSE || threshold->fired == NS_FALSE) {
                if (ns_threshold_crossed(threshold, previous_value, value) == NS_TRUE) {
                    temp = ns_push_event(world,
                                         NS_EVENT_THRESHOLD,
                                         value_id,
                                         previous_value,
                                         value,
                                         threshold->threshold,
                                         i,
                                         threshold->user_code);
                    if (result == NS_OK && temp != NS_OK) {
                        result = temp;
                    }
                    threshold->fired = NS_TRUE;
                }
            }
        }
        i++;
    }

    return result;
}

static ns_fx ns_apply_modifiers(const NS_World *world, ns_id value_id, int target, ns_fx base_value)
{
    int i;
    ns_fx value;
    ns_fx percent_add;
    const NS_Modifier *mod;

    value = base_value;
    percent_add = NS_FX_ZERO;

    i = 0;
    while (i < NS_MAX_MODIFIERS) {
        mod = &world->modifiers[i];
        if (mod->used == NS_TRUE && mod->value_id == value_id && mod->target == target) {
            if (mod->mode == NS_MOD_FLAT) {
                value = value + mod->amount;
            }
        }
        i++;
    }

    i = 0;
    while (i < NS_MAX_MODIFIERS) {
        mod = &world->modifiers[i];
        if (mod->used == NS_TRUE && mod->value_id == value_id && mod->target == target) {
            if (mod->mode == NS_MOD_PERCENT_ADD) {
                percent_add = percent_add + mod->amount;
            }
        }
        i++;
    }

    if (percent_add != NS_FX_ZERO) {
        value = value + ns_fx_mul(value, percent_add);
    }

    i = 0;
    while (i < NS_MAX_MODIFIERS) {
        mod = &world->modifiers[i];
        if (mod->used == NS_TRUE && mod->value_id == value_id && mod->target == target) {
            if (mod->mode == NS_MOD_PERCENT_MUL) {
                value = ns_fx_mul(value, NS_FX_ONE + mod->amount);
            }
        }
        i++;
    }

    return value;
}

static int ns_recalculate_internal(NS_World *world, ns_id value_id, int emit_events)
{
    NS_Value *slot;
    ns_fx previous_value;
    ns_fx raw_value;
    ns_fx new_value;
    ns_fx spill_amount;
    int spill_dir;
    ns_id spill_target;
    int result;
    int temp;

    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    slot = &world->values[value_id];
    previous_value = slot->value;

    slot->min_value = ns_apply_modifiers(world, value_id, NS_MOD_TARGET_MIN, slot->base_min_value);
    slot->max_value = ns_apply_modifiers(world, value_id, NS_MOD_TARGET_MAX, slot->base_max_value);
    slot->regen_per_tick = ns_apply_modifiers(world, value_id, NS_MOD_TARGET_REGEN, slot->base_regen_per_tick);
    slot->drain_per_tick = ns_apply_modifiers(world, value_id, NS_MOD_TARGET_DRAIN, slot->base_drain_per_tick);

    if (slot->min_value > slot->max_value) {
        slot->max_value = slot->min_value;
    }

    raw_value = ns_apply_modifiers(world, value_id, NS_MOD_TARGET_VALUE, slot->base_value);
    raw_value = ns_quantize_value(slot->kind, raw_value);
    new_value = ns_apply_overflow_calc(slot, raw_value, &spill_dir, &spill_amount);

    slot->previous_value = previous_value;
    slot->value = new_value;
    slot->dirty = NS_TRUE;

    result = NS_OK;
    if (new_value != previous_value) {
        slot->changed = NS_TRUE;
        if (emit_events == NS_TRUE) {
            result = ns_emit_basic_events(world, value_id, previous_value, new_value);
            temp = ns_check_thresholds(world, value_id, previous_value, new_value);
            if (result == NS_OK && temp != NS_OK) {
                result = temp;
            }
        }
    }

    spill_target = slot->spill_target_value_id;
    if (slot->overflow == NS_OVERFLOW_SPILL && spill_amount > NS_FX_ZERO) {
        if (spill_target != value_id && ns_valid_value_id(world, spill_target) == NS_TRUE) {
            if (spill_dir > 0) {
                temp = ns_add_by_id(world, spill_target, spill_amount);
            } else {
                temp = ns_sub_by_id(world, spill_target, spill_amount);
            }
            if (result == NS_OK && temp != NS_OK) {
                result = temp;
            }
        }
    }

    return result;
}

static int ns_commit_base_value(NS_World *world, ns_id value_id, ns_fx raw_base_value, int emit_reset)
{
    NS_Value *slot;
    int result;
    int temp;

    if (world == NULL) {
        return NS_ERR_NULL;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    slot = &world->values[value_id];
    if ((slot->flags & NS_FLAG_LOCKED) != 0) {
        return NS_ERR_LOCKED;
    }

    slot->base_value = ns_quantize_value(slot->kind, raw_base_value);
    result = ns_recalculate_internal(world, value_id, NS_TRUE);

    if (emit_reset == NS_TRUE) {
        temp = ns_push_event(world,
                             NS_EVENT_RESET,
                             value_id,
                             slot->previous_value,
                             slot->value,
                             NS_FX_ZERO,
                             NS_INVALID_ID,
                             0);
        if (result == NS_OK && temp != NS_OK) {
            result = temp;
        }
    }

    return result;
}

void ns_init(NS_World *world)
{
    int i;

    if (world == NULL) {
        return;
    }

    i = 0;
    while (i < NS_MAX_TYPES) {
        world->types[i].used = NS_FALSE;
        world->types[i].name[0] = '\0';
        world->types[i].scope = 0;
        world->types[i].kind = 0;
        world->types[i].overflow = 0;
        world->types[i].initial_value = NS_FX_ZERO;
        world->types[i].min_value = NS_FX_ZERO;
        world->types[i].max_value = NS_FX_ZERO;
        world->types[i].regen_per_tick = NS_FX_ZERO;
        world->types[i].drain_per_tick = NS_FX_ZERO;
        world->types[i].flags = 0;
        i++;
    }

    i = 0;
    while (i < NS_MAX_VALUES) {
        world->values[i].used = NS_FALSE;
        world->values[i].owner = 0;
        world->values[i].type_id = NS_INVALID_ID;
        world->values[i].base_value = NS_FX_ZERO;
        world->values[i].value = NS_FX_ZERO;
        world->values[i].previous_value = NS_FX_ZERO;
        world->values[i].initial_value = NS_FX_ZERO;
        world->values[i].base_min_value = NS_FX_ZERO;
        world->values[i].base_max_value = NS_FX_ZERO;
        world->values[i].min_value = NS_FX_ZERO;
        world->values[i].max_value = NS_FX_ZERO;
        world->values[i].base_regen_per_tick = NS_FX_ZERO;
        world->values[i].base_drain_per_tick = NS_FX_ZERO;
        world->values[i].regen_per_tick = NS_FX_ZERO;
        world->values[i].drain_per_tick = NS_FX_ZERO;
        world->values[i].overflow = NS_OVERFLOW_NONE;
        world->values[i].kind = NS_KIND_FIXED;
        world->values[i].flags = 0;
        world->values[i].tag = 0;
        world->values[i].changed = NS_FALSE;
        world->values[i].dirty = NS_FALSE;
        world->values[i].spill_target_value_id = NS_INVALID_ID;
        i++;
    }

    i = 0;
    while (i < NS_MAX_THRESHOLDS) {
        world->thresholds[i].used = NS_FALSE;
        world->thresholds[i].value_id = NS_INVALID_ID;
        world->thresholds[i].threshold = NS_FX_ZERO;
        world->thresholds[i].edge = NS_EDGE_ANY;
        world->thresholds[i].once = NS_FALSE;
        world->thresholds[i].fired = NS_FALSE;
        world->thresholds[i].user_code = 0;
        i++;
    }

    i = 0;
    while (i < NS_MAX_EVENTS) {
        world->events[i].event_type = 0;
        world->events[i].value_id = NS_INVALID_ID;
        world->events[i].type_id = NS_INVALID_ID;
        world->events[i].owner = 0;
        world->events[i].previous_value = NS_FX_ZERO;
        world->events[i].value = NS_FX_ZERO;
        world->events[i].threshold = NS_FX_ZERO;
        world->events[i].threshold_id = NS_INVALID_ID;
        world->events[i].user_code = 0;
        i++;
    }

    i = 0;
    while (i < NS_MAX_TEMPLATES) {
        world->templates[i].used = NS_FALSE;
        world->templates[i].name[0] = '\0';
        world->templates[i].flags = 0;
        i++;
    }

    i = 0;
    while (i < NS_MAX_TEMPLATE_ITEMS) {
        world->template_items[i].used = NS_FALSE;
        world->template_items[i].template_id = NS_INVALID_ID;
        world->template_items[i].type_id = NS_INVALID_ID;
        i++;
    }

    i = 0;
    while (i < NS_MAX_MODIFIERS) {
        world->modifiers[i].used = NS_FALSE;
        world->modifiers[i].value_id = NS_INVALID_ID;
        world->modifiers[i].code = 0;
        world->modifiers[i].target = 0;
        world->modifiers[i].mode = 0;
        world->modifiers[i].amount = NS_FX_ZERO;
        world->modifiers[i].duration_ticks = NS_MOD_FOREVER;
        world->modifiers[i].priority = 0;
        world->modifiers[i].flags = 0;
        i++;
    }

    i = 0;
    while (i < NS_MAX_DERIVED) {
        world->derived[i].used = NS_FALSE;
        world->derived[i].output_value_id = NS_INVALID_ID;
        world->derived[i].a_value_id = NS_INVALID_ID;
        world->derived[i].b_value_id = NS_INVALID_ID;
        world->derived[i].mode = 0;
        world->derived[i].scale = NS_FX_ONE;
        world->derived[i].offset = NS_FX_ZERO;
        world->derived[i].flags = 0;
        i++;
    }

    i = 0;
    while (i < NS_MAX_BINDINGS) {
        world->bindings[i].used = NS_FALSE;
        world->bindings[i].value_id = NS_INVALID_ID;
        world->bindings[i].kind = 0;
        world->bindings[i].user_code = 0;
        world->bindings[i].channel = 0;
        world->bindings[i].flags = 0;
        i++;
    }

    world->event_count = 0;
    world->callback = NULL;
    world->callback_user_data = NULL;
}

ns_id ns_define_type(NS_World *world,
                     const char *name,
                     int scope,
                     int kind,
                     ns_fx initial_value,
                     ns_fx min_value,
                     ns_fx max_value,
                     int overflow,
                     int flags)
{
    int i;
    NS_TypeDef *type_def;
    NS_Value temp_slot;
    ns_fx prepared_initial;

    if (world == NULL || name == NULL) {
        return NS_ERR_NULL;
    }
    if (name[0] == '\0') {
        return NS_ERR_BAD_ARG;
    }
    if (ns_valid_scope(scope) == NS_FALSE) {
        return NS_ERR_BAD_ARG;
    }
    if (ns_valid_kind(kind) == NS_FALSE) {
        return NS_ERR_BAD_ARG;
    }
    if (ns_valid_overflow(overflow) == NS_FALSE) {
        return NS_ERR_BAD_ARG;
    }
    if (min_value > max_value) {
        return NS_ERR_RANGE;
    }

    i = ns_find_type(world, name);
    if (i >= 0) {
        return i;
    }

    i = 0;
    while (i < NS_MAX_TYPES) {
        if (world->types[i].used == NS_FALSE) {
            type_def = &world->types[i];
            type_def->used = NS_TRUE;
            ns_name_copy(type_def->name, name);
            type_def->scope = scope;
            type_def->kind = kind;
            type_def->overflow = overflow;
            type_def->initial_value = initial_value;
            type_def->min_value = min_value;
            type_def->max_value = max_value;
            type_def->regen_per_tick = NS_FX_ZERO;
            type_def->drain_per_tick = NS_FX_ZERO;
            type_def->flags = flags;

            temp_slot.used = NS_TRUE;
            temp_slot.owner = NS_OWNER_GLOBAL;
            temp_slot.type_id = i;
            temp_slot.base_value = initial_value;
            temp_slot.value = initial_value;
            temp_slot.previous_value = initial_value;
            temp_slot.initial_value = initial_value;
            temp_slot.base_min_value = min_value;
            temp_slot.base_max_value = max_value;
            temp_slot.min_value = min_value;
            temp_slot.max_value = max_value;
            temp_slot.base_regen_per_tick = NS_FX_ZERO;
            temp_slot.base_drain_per_tick = NS_FX_ZERO;
            temp_slot.regen_per_tick = NS_FX_ZERO;
            temp_slot.drain_per_tick = NS_FX_ZERO;
            temp_slot.overflow = overflow;
            temp_slot.kind = kind;
            temp_slot.flags = flags;
            temp_slot.tag = 0;
            temp_slot.changed = NS_FALSE;
            temp_slot.dirty = NS_FALSE;
            temp_slot.spill_target_value_id = NS_INVALID_ID;

            prepared_initial = ns_quantize_value(kind, initial_value);
            prepared_initial = ns_apply_overflow_calc(&temp_slot, prepared_initial, NULL, NULL);
            type_def->initial_value = prepared_initial;
            return i;
        }
        i++;
    }

    return NS_ERR_FULL;
}

int ns_set_type_rates(NS_World *world,
                      ns_id type_id,
                      ns_fx regen_per_tick,
                      ns_fx drain_per_tick)
{
    if (ns_valid_type_id(world, type_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    world->types[type_id].regen_per_tick = regen_per_tick;
    world->types[type_id].drain_per_tick = drain_per_tick;
    return NS_OK;
}

int ns_define_pack(NS_World *world, const NS_PackItem *items, int count)
{
    int i;
    ns_id type_id;
    int result;

    if (world == NULL || items == NULL) {
        return NS_ERR_NULL;
    }

    result = NS_OK;
    i = 0;
    while (i < count) {
        if (items[i].name == NULL) {
            return result;
        }
        type_id = ns_define_type(world,
                                 items[i].name,
                                 items[i].scope,
                                 items[i].kind,
                                 items[i].initial_value,
                                 items[i].min_value,
                                 items[i].max_value,
                                 items[i].overflow,
                                 items[i].flags);
        if (type_id < 0) {
            return type_id;
        }
        result = ns_set_type_rates(world,
                                   type_id,
                                   items[i].regen_per_tick,
                                   items[i].drain_per_tick);
        if (result != NS_OK) {
            return result;
        }
        i++;
    }

    return result;
}

ns_id ns_find_type(const NS_World *world, const char *name)
{
    int i;

    if (world == NULL || name == NULL) {
        return NS_ERR_NULL;
    }

    i = 0;
    while (i < NS_MAX_TYPES) {
        if (world->types[i].used == NS_TRUE) {
            if (ns_name_equal(world->types[i].name, name) == NS_TRUE) {
                return i;
            }
        }
        i++;
    }

    return NS_ERR_NOT_FOUND;
}

const char *ns_type_name(const NS_World *world, ns_id type_id)
{
    if (ns_valid_type_id(world, type_id) == NS_FALSE) {
        return NULL;
    }
    return world->types[type_id].name;
}

ns_id ns_attach(NS_World *world, ns_owner owner, const char *type_name)
{
    ns_id type_id;

    if (world == NULL || type_name == NULL) {
        return NS_ERR_NULL;
    }

    type_id = ns_find_type(world, type_name);
    if (type_id < 0) {
        return type_id;
    }

    return ns_attach_type(world, owner, type_id);
}

ns_id ns_attach_type(NS_World *world, ns_owner owner, ns_id type_id)
{
    int i;
    NS_TypeDef *type_def;
    NS_Value *slot;

    if (ns_valid_type_id(world, type_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    i = 0;
    while (i < NS_MAX_VALUES) {
        if (world->values[i].used == NS_TRUE) {
            if (world->values[i].owner == owner && world->values[i].type_id == type_id) {
                return i;
            }
        }
        i++;
    }

    i = 0;
    while (i < NS_MAX_VALUES) {
        if (world->values[i].used == NS_FALSE) {
            type_def = &world->types[type_id];
            slot = &world->values[i];
            slot->used = NS_TRUE;
            slot->owner = owner;
            slot->type_id = type_id;
            slot->base_value = type_def->initial_value;
            slot->value = type_def->initial_value;
            slot->previous_value = type_def->initial_value;
            slot->initial_value = type_def->initial_value;
            slot->base_min_value = type_def->min_value;
            slot->base_max_value = type_def->max_value;
            slot->min_value = type_def->min_value;
            slot->max_value = type_def->max_value;
            slot->base_regen_per_tick = type_def->regen_per_tick;
            slot->base_drain_per_tick = type_def->drain_per_tick;
            slot->regen_per_tick = type_def->regen_per_tick;
            slot->drain_per_tick = type_def->drain_per_tick;
            slot->overflow = type_def->overflow;
            slot->kind = type_def->kind;
            slot->flags = type_def->flags;
            slot->tag = 0;
            slot->changed = NS_FALSE;
            slot->dirty = NS_FALSE;
            slot->spill_target_value_id = NS_INVALID_ID;
            ns_recalculate_internal(world, i, NS_FALSE);
            slot->changed = NS_FALSE;
            slot->dirty = NS_FALSE;
            return i;
        }
        i++;
    }

    return NS_ERR_FULL;
}

int ns_attach_pack(NS_World *world, ns_owner owner, const char * const *type_names, int count)
{
    int i;
    ns_id value_id;

    if (world == NULL || type_names == NULL) {
        return NS_ERR_NULL;
    }

    i = 0;
    while (i < count) {
        if (type_names[i] == NULL) {
            return NS_OK;
        }
        value_id = ns_attach(world, owner, type_names[i]);
        if (value_id < 0) {
            return value_id;
        }
        i++;
    }

    return NS_OK;
}

int ns_attach_type_pack(NS_World *world, ns_owner owner, const ns_id *type_ids, int count)
{
    int i;
    ns_id value_id;

    if (world == NULL || type_ids == NULL) {
        return NS_ERR_NULL;
    }

    i = 0;
    while (i < count) {
        if (type_ids[i] == NS_INVALID_ID) {
            return NS_OK;
        }
        value_id = ns_attach_type(world, owner, type_ids[i]);
        if (value_id < 0) {
            return value_id;
        }
        i++;
    }

    return NS_OK;
}

ns_id ns_find_value_by_type(const NS_World *world, ns_owner owner, ns_id type_id)
{
    int i;

    if (ns_valid_type_id(world, type_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    i = 0;
    while (i < NS_MAX_VALUES) {
        if (world->values[i].used == NS_TRUE) {
            if (world->values[i].owner == owner && world->values[i].type_id == type_id) {
                return i;
            }
        }
        i++;
    }

    return NS_ERR_NOT_FOUND;
}

ns_id ns_find_value(const NS_World *world, ns_owner owner, const char *type_name)
{
    ns_id type_id;

    if (world == NULL || type_name == NULL) {
        return NS_ERR_NULL;
    }

    type_id = ns_find_type(world, type_name);
    if (type_id < 0) {
        return type_id;
    }

    return ns_find_value_by_type(world, owner, type_id);
}

int ns_get_by_id(const NS_World *world, ns_id value_id, ns_fx *out_value)
{
    if (out_value == NULL) {
        return NS_ERR_NULL;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    *out_value = world->values[value_id].value;
    return NS_OK;
}

int ns_get_base_by_id(const NS_World *world, ns_id value_id, ns_fx *out_value)
{
    if (out_value == NULL) {
        return NS_ERR_NULL;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    *out_value = world->values[value_id].base_value;
    return NS_OK;
}

ns_fx ns_get_or(const NS_World *world, ns_owner owner, const char *type_name, ns_fx fallback_value)
{
    ns_id value_id;

    value_id = ns_find_value(world, owner, type_name);
    if (value_id < 0) {
        return fallback_value;
    }

    return world->values[value_id].value;
}

int ns_set_by_id(NS_World *world, ns_id value_id, ns_fx new_value)
{
    return ns_commit_base_value(world, value_id, new_value, NS_FALSE);
}

int ns_set_base_by_id(NS_World *world, ns_id value_id, ns_fx new_base_value)
{
    return ns_commit_base_value(world, value_id, new_base_value, NS_FALSE);
}

int ns_add_by_id(NS_World *world, ns_id value_id, ns_fx amount)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    return ns_commit_base_value(world, value_id, world->values[value_id].base_value + amount, NS_FALSE);
}

int ns_sub_by_id(NS_World *world, ns_id value_id, ns_fx amount)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    return ns_commit_base_value(world, value_id, world->values[value_id].base_value - amount, NS_FALSE);
}

int ns_reset_by_id(NS_World *world, ns_id value_id)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    return ns_commit_base_value(world, value_id, world->values[value_id].initial_value, NS_TRUE);
}

int ns_set(NS_World *world, ns_owner owner, const char *type_name, ns_fx new_value)
{
    ns_id value_id;

    value_id = ns_find_value(world, owner, type_name);
    if (value_id < 0) {
        return value_id;
    }

    return ns_set_by_id(world, value_id, new_value);
}

int ns_add(NS_World *world, ns_owner owner, const char *type_name, ns_fx amount)
{
    ns_id value_id;

    value_id = ns_find_value(world, owner, type_name);
    if (value_id < 0) {
        return value_id;
    }

    return ns_add_by_id(world, value_id, amount);
}

int ns_sub(NS_World *world, ns_owner owner, const char *type_name, ns_fx amount)
{
    ns_id value_id;

    value_id = ns_find_value(world, owner, type_name);
    if (value_id < 0) {
        return value_id;
    }

    return ns_sub_by_id(world, value_id, amount);
}

int ns_reset(NS_World *world, ns_owner owner, const char *type_name)
{
    ns_id value_id;

    value_id = ns_find_value(world, owner, type_name);
    if (value_id < 0) {
        return value_id;
    }

    return ns_reset_by_id(world, value_id);
}

int ns_set_bounds_by_id(NS_World *world, ns_id value_id, ns_fx min_value, ns_fx max_value)
{
    NS_Value *slot;

    if (min_value > max_value) {
        return NS_ERR_RANGE;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    slot = &world->values[value_id];
    slot->base_min_value = min_value;
    slot->base_max_value = max_value;
    return ns_recalculate_internal(world, value_id, NS_TRUE);
}

int ns_set_rates_by_id(NS_World *world, ns_id value_id, ns_fx regen_per_tick, ns_fx drain_per_tick)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    world->values[value_id].base_regen_per_tick = regen_per_tick;
    world->values[value_id].base_drain_per_tick = drain_per_tick;
    return ns_recalculate_internal(world, value_id, NS_TRUE);
}

int ns_set_overflow_by_id(NS_World *world, ns_id value_id, int overflow)
{
    if (ns_valid_overflow(overflow) == NS_FALSE) {
        return NS_ERR_BAD_ARG;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    world->values[value_id].overflow = overflow;
    return ns_recalculate_internal(world, value_id, NS_TRUE);
}

int ns_set_spill_target_by_id(NS_World *world, ns_id value_id, ns_id target_value_id)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    if (target_value_id != NS_INVALID_ID) {
        if (ns_valid_value_id(world, target_value_id) == NS_FALSE) {
            return NS_ERR_BAD_ID;
        }
    }
    world->values[value_id].spill_target_value_id = target_value_id;
    return NS_OK;
}

int ns_set_tag_by_id(NS_World *world, ns_id value_id, int tag)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    world->values[value_id].tag = tag;
    world->values[value_id].flags = world->values[value_id].flags | NS_FLAG_TAGGED;
    return NS_OK;
}

int ns_get_tag_by_id(const NS_World *world, ns_id value_id, int *out_tag)
{
    if (out_tag == NULL) {
        return NS_ERR_NULL;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    *out_tag = world->values[value_id].tag;
    return NS_OK;
}

int ns_set_flags_by_id(NS_World *world, ns_id value_id, int flags)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    world->values[value_id].flags = flags;
    return NS_OK;
}

int ns_add_flags_by_id(NS_World *world, ns_id value_id, int flags)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    world->values[value_id].flags = world->values[value_id].flags | flags;
    return NS_OK;
}

int ns_clear_flags_by_id(NS_World *world, ns_id value_id, int flags)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    world->values[value_id].flags = world->values[value_id].flags & (~flags);
    return NS_OK;
}

int ns_get_flags_by_id(const NS_World *world, ns_id value_id, int *out_flags)
{
    if (out_flags == NULL) {
        return NS_ERR_NULL;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    *out_flags = world->values[value_id].flags;
    return NS_OK;
}

int ns_was_changed_by_id(const NS_World *world, ns_id value_id)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_FALSE;
    }
    return world->values[value_id].changed;
}

int ns_is_dirty_by_id(const NS_World *world, ns_id value_id)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_FALSE;
    }
    return world->values[value_id].dirty;
}

int ns_clear_changed_by_id(NS_World *world, ns_id value_id)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    world->values[value_id].changed = NS_FALSE;
    world->values[value_id].dirty = NS_FALSE;
    return NS_OK;
}

int ns_clear_all_changed(NS_World *world)
{
    int i;

    if (world == NULL) {
        return NS_ERR_NULL;
    }

    i = 0;
    while (i < NS_MAX_VALUES) {
        if (world->values[i].used == NS_TRUE) {
            world->values[i].changed = NS_FALSE;
            world->values[i].dirty = NS_FALSE;
        }
        i++;
    }
    return NS_OK;
}

int ns_compare_by_id(const NS_World *world, ns_id value_id, int cmp, ns_fx rhs)
{
    ns_fx lhs;

    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_FALSE;
    }

    lhs = world->values[value_id].value;

    if (cmp == NS_CMP_EQ) {
        return lhs == rhs;
    }
    if (cmp == NS_CMP_NE) {
        return lhs != rhs;
    }
    if (cmp == NS_CMP_LT) {
        return lhs < rhs;
    }
    if (cmp == NS_CMP_LTE) {
        return lhs <= rhs;
    }
    if (cmp == NS_CMP_GT) {
        return lhs > rhs;
    }
    if (cmp == NS_CMP_GTE) {
        return lhs >= rhs;
    }

    return NS_FALSE;
}

int ns_compare(const NS_World *world, ns_owner owner, const char *type_name, int cmp, ns_fx rhs)
{
    ns_id value_id;

    value_id = ns_find_value(world, owner, type_name);
    if (value_id < 0) {
        return NS_FALSE;
    }

    return ns_compare_by_id(world, value_id, cmp, rhs);
}

int ns_is_empty_by_id(const NS_World *world, ns_id value_id)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_FALSE;
    }
    return world->values[value_id].value <= world->values[value_id].min_value;
}

int ns_is_full_by_id(const NS_World *world, ns_id value_id)
{
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_FALSE;
    }
    return world->values[value_id].value >= world->values[value_id].max_value;
}

int ns_is_empty(const NS_World *world, ns_owner owner, const char *type_name)
{
    ns_id value_id;

    value_id = ns_find_value(world, owner, type_name);
    if (value_id < 0) {
        return NS_FALSE;
    }
    return ns_is_empty_by_id(world, value_id);
}

int ns_is_full(const NS_World *world, ns_owner owner, const char *type_name)
{
    ns_id value_id;

    value_id = ns_find_value(world, owner, type_name);
    if (value_id < 0) {
        return NS_FALSE;
    }
    return ns_is_full_by_id(world, value_id);
}

int ns_percent_by_id(const NS_World *world, ns_id value_id, ns_fx *out_percent)
{
    const NS_Value *slot;
    ns_fx range;
    ns_fx offset;

    if (out_percent == NULL) {
        return NS_ERR_NULL;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    slot = &world->values[value_id];
    range = slot->max_value - slot->min_value;
    if (range <= NS_FX_ZERO) {
        *out_percent = NS_FX_ZERO;
        return NS_ERR_RANGE;
    }

    offset = slot->value - slot->min_value;
    if (offset < NS_FX_ZERO) {
        offset = NS_FX_ZERO;
    }
    if (offset > range) {
        offset = range;
    }

    *out_percent = ns_fx_div(offset, range);
    return NS_OK;
}

int ns_percent(const NS_World *world, ns_owner owner, const char *type_name, ns_fx *out_percent)
{
    ns_id value_id;

    value_id = ns_find_value(world, owner, type_name);
    if (value_id < 0) {
        return value_id;
    }
    return ns_percent_by_id(world, value_id, out_percent);
}

int ns_transfer_by_id(NS_World *world,
                      ns_id from_value_id,
                      ns_id to_value_id,
                      ns_fx requested_amount,
                      ns_fx *out_moved_amount)
{
    NS_Value *from_slot;
    NS_Value *to_slot;
    ns_fx available;
    ns_fx space;
    ns_fx moved;
    int result;
    int temp;

    if (requested_amount < NS_FX_ZERO) {
        return NS_ERR_BAD_ARG;
    }
    if (ns_valid_value_id(world, from_value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    if (ns_valid_value_id(world, to_value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    from_slot = &world->values[from_value_id];
    to_slot = &world->values[to_value_id];
    available = from_slot->value - from_slot->min_value;
    space = to_slot->max_value - to_slot->value;

    if (available < NS_FX_ZERO) {
        available = NS_FX_ZERO;
    }
    if (space < NS_FX_ZERO) {
        space = NS_FX_ZERO;
    }

    moved = requested_amount;
    moved = ns_min_fx(moved, available);
    moved = ns_min_fx(moved, space);

    if (out_moved_amount != NULL) {
        *out_moved_amount = moved;
    }

    if (moved <= NS_FX_ZERO) {
        return NS_OK;
    }

    result = ns_sub_by_id(world, from_value_id, moved);
    temp = ns_add_by_id(world, to_value_id, moved);
    if (result == NS_OK && temp != NS_OK) {
        result = temp;
    }
    return result;
}

int ns_sub_spill_by_id(NS_World *world,
                       ns_id primary_value_id,
                       ns_id secondary_value_id,
                       ns_fx amount,
                       ns_fx *out_spill_amount)
{
    NS_Value *primary_slot;
    ns_fx absorbable;
    ns_fx absorbed;
    ns_fx spill;
    int result;
    int temp;

    if (amount < NS_FX_ZERO) {
        return NS_ERR_BAD_ARG;
    }
    if (ns_valid_value_id(world, primary_value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    if (ns_valid_value_id(world, secondary_value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    primary_slot = &world->values[primary_value_id];
    absorbable = primary_slot->value - primary_slot->min_value;
    if (absorbable < NS_FX_ZERO) {
        absorbable = NS_FX_ZERO;
    }

    absorbed = ns_min_fx(amount, absorbable);
    spill = amount - absorbed;

    if (out_spill_amount != NULL) {
        *out_spill_amount = spill;
    }

    result = NS_OK;
    if (absorbed > NS_FX_ZERO) {
        result = ns_sub_by_id(world, primary_value_id, absorbed);
    }
    if (spill > NS_FX_ZERO) {
        temp = ns_sub_by_id(world, secondary_value_id, spill);
        if (result == NS_OK && temp != NS_OK) {
            result = temp;
        }
    }

    return result;
}

static int ns_tick_modifiers(NS_World *world)
{
    int i;
    int result;
    int temp;
    ns_id value_id;
    int code;

    result = NS_OK;
    i = 0;
    while (i < NS_MAX_MODIFIERS) {
        if (world->modifiers[i].used == NS_TRUE) {
            if (world->modifiers[i].duration_ticks > 0) {
                world->modifiers[i].duration_ticks--;
                if (world->modifiers[i].duration_ticks == 0) {
                    value_id = world->modifiers[i].value_id;
                    code = world->modifiers[i].code;
                    world->modifiers[i].used = NS_FALSE;
                    world->modifiers[i].value_id = NS_INVALID_ID;
                    temp = ns_recalculate_internal(world, value_id, NS_TRUE);
                    if (result == NS_OK && temp != NS_OK) {
                        result = temp;
                    }
                    temp = ns_push_event(world,
                                         NS_EVENT_MODIFIER_EXPIRED,
                                         value_id,
                                         world->values[value_id].previous_value,
                                         world->values[value_id].value,
                                         NS_FX_ZERO,
                                         i,
                                         code);
                    if (result == NS_OK && temp != NS_OK) {
                        result = temp;
                    }
                }
            }
        }
        i++;
    }

    return result;
}

int ns_tick(NS_World *world)
{
    int i;
    int result;
    int temp;
    ns_fx delta;

    if (world == NULL) {
        return NS_ERR_NULL;
    }

    result = ns_tick_modifiers(world);

    i = 0;
    while (i < NS_MAX_VALUES) {
        if (world->values[i].used == NS_TRUE) {
            delta = world->values[i].regen_per_tick - world->values[i].drain_per_tick;
            if (delta != NS_FX_ZERO) {
                temp = ns_add_by_id(world, i, delta);
                if (result == NS_OK && temp != NS_OK) {
                    result = temp;
                }
            }
        }
        i++;
    }

    temp = ns_update_all_derived(world);
    if (result == NS_OK && temp != NS_OK) {
        result = temp;
    }

    return result;
}

ns_id ns_add_threshold(NS_World *world,
                       ns_id value_id,
                       ns_fx threshold,
                       int edge,
                       int once,
                       int user_code)
{
    int i;
    NS_Threshold *slot;

    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    if (ns_valid_edge(edge) == NS_FALSE) {
        return NS_ERR_BAD_ARG;
    }

    i = 0;
    while (i < NS_MAX_THRESHOLDS) {
        if (world->thresholds[i].used == NS_FALSE) {
            slot = &world->thresholds[i];
            slot->used = NS_TRUE;
            slot->value_id = value_id;
            slot->threshold = threshold;
            slot->edge = edge;
            if (once != NS_FALSE) {
                slot->once = NS_TRUE;
            } else {
                slot->once = NS_FALSE;
            }
            slot->fired = NS_FALSE;
            slot->user_code = user_code;
            return i;
        }
        i++;
    }

    return NS_ERR_FULL;
}

int ns_clear_thresholds_for_value(NS_World *world, ns_id value_id)
{
    int i;

    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    i = 0;
    while (i < NS_MAX_THRESHOLDS) {
        if (world->thresholds[i].used == NS_TRUE && world->thresholds[i].value_id == value_id) {
            world->thresholds[i].used = NS_FALSE;
            world->thresholds[i].value_id = NS_INVALID_ID;
            world->thresholds[i].threshold = NS_FX_ZERO;
            world->thresholds[i].edge = NS_EDGE_ANY;
            world->thresholds[i].once = NS_FALSE;
            world->thresholds[i].fired = NS_FALSE;
            world->thresholds[i].user_code = 0;
        }
        i++;
    }

    return NS_OK;
}

ns_id ns_define_template(NS_World *world, const char *name, int flags)
{
    int i;

    if (world == NULL || name == NULL) {
        return NS_ERR_NULL;
    }
    if (name[0] == '\0') {
        return NS_ERR_BAD_ARG;
    }

    i = ns_find_template(world, name);
    if (i >= 0) {
        return i;
    }

    i = 0;
    while (i < NS_MAX_TEMPLATES) {
        if (world->templates[i].used == NS_FALSE) {
            world->templates[i].used = NS_TRUE;
            ns_name_copy(world->templates[i].name, name);
            world->templates[i].flags = flags;
            return i;
        }
        i++;
    }

    return NS_ERR_FULL;
}

ns_id ns_find_template(const NS_World *world, const char *name)
{
    int i;

    if (world == NULL || name == NULL) {
        return NS_ERR_NULL;
    }

    i = 0;
    while (i < NS_MAX_TEMPLATES) {
        if (world->templates[i].used == NS_TRUE) {
            if (ns_name_equal(world->templates[i].name, name) == NS_TRUE) {
                return i;
            }
        }
        i++;
    }

    return NS_ERR_NOT_FOUND;
}

int ns_template_add_type(NS_World *world, ns_id template_id, ns_id type_id)
{
    int i;

    if (ns_valid_template_id(world, template_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    if (ns_valid_type_id(world, type_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    i = 0;
    while (i < NS_MAX_TEMPLATE_ITEMS) {
        if (world->template_items[i].used == NS_TRUE) {
            if (world->template_items[i].template_id == template_id && world->template_items[i].type_id == type_id) {
                return NS_OK;
            }
        }
        i++;
    }

    i = 0;
    while (i < NS_MAX_TEMPLATE_ITEMS) {
        if (world->template_items[i].used == NS_FALSE) {
            world->template_items[i].used = NS_TRUE;
            world->template_items[i].template_id = template_id;
            world->template_items[i].type_id = type_id;
            return NS_OK;
        }
        i++;
    }

    return NS_ERR_FULL;
}

int ns_template_add(NS_World *world, const char *template_name, const char *type_name)
{
    ns_id template_id;
    ns_id type_id;

    if (world == NULL || template_name == NULL || type_name == NULL) {
        return NS_ERR_NULL;
    }

    template_id = ns_find_template(world, template_name);
    if (template_id < 0) {
        return template_id;
    }
    type_id = ns_find_type(world, type_name);
    if (type_id < 0) {
        return type_id;
    }
    return ns_template_add_type(world, template_id, type_id);
}

int ns_attach_template(NS_World *world, ns_owner owner, ns_id template_id)
{
    int i;
    ns_id value_id;

    if (ns_valid_template_id(world, template_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    i = 0;
    while (i < NS_MAX_TEMPLATE_ITEMS) {
        if (world->template_items[i].used == NS_TRUE && world->template_items[i].template_id == template_id) {
            value_id = ns_attach_type(world, owner, world->template_items[i].type_id);
            if (value_id < 0) {
                return value_id;
            }
        }
        i++;
    }

    return NS_OK;
}

int ns_attach_template_name(NS_World *world, ns_owner owner, const char *template_name)
{
    ns_id template_id;

    template_id = ns_find_template(world, template_name);
    if (template_id < 0) {
        return template_id;
    }
    return ns_attach_template(world, owner, template_id);
}

int ns_clear_template(NS_World *world, ns_id template_id)
{
    int i;

    if (ns_valid_template_id(world, template_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    i = 0;
    while (i < NS_MAX_TEMPLATE_ITEMS) {
        if (world->template_items[i].used == NS_TRUE && world->template_items[i].template_id == template_id) {
            world->template_items[i].used = NS_FALSE;
            world->template_items[i].template_id = NS_INVALID_ID;
            world->template_items[i].type_id = NS_INVALID_ID;
        }
        i++;
    }

    world->templates[template_id].used = NS_FALSE;
    world->templates[template_id].name[0] = '\0';
    world->templates[template_id].flags = 0;
    return NS_OK;
}

ns_id ns_add_modifier_by_id(NS_World *world,
                            ns_id value_id,
                            int code,
                            int target,
                            int mode,
                            ns_fx amount,
                            int duration_ticks,
                            int priority,
                            int flags)
{
    int i;
    NS_Modifier *slot;
    int temp;

    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    if (ns_valid_mod_target(target) == NS_FALSE) {
        return NS_ERR_BAD_ARG;
    }
    if (ns_valid_mod_mode(mode) == NS_FALSE) {
        return NS_ERR_BAD_ARG;
    }

    i = 0;
    while (i < NS_MAX_MODIFIERS) {
        if (world->modifiers[i].used == NS_TRUE) {
            if (world->modifiers[i].value_id == value_id && world->modifiers[i].code == code && world->modifiers[i].target == target) {
                world->modifiers[i].mode = mode;
                world->modifiers[i].amount = amount;
                world->modifiers[i].duration_ticks = duration_ticks;
                world->modifiers[i].priority = priority;
                world->modifiers[i].flags = flags;
                temp = ns_recalculate_internal(world, value_id, NS_TRUE);
                if (temp != NS_OK) {
                    return temp;
                }
                ns_push_event(world,
                              NS_EVENT_MODIFIER_ADDED,
                              value_id,
                              world->values[value_id].previous_value,
                              world->values[value_id].value,
                              NS_FX_ZERO,
                              i,
                              code);
                return i;
            }
        }
        i++;
    }

    i = 0;
    while (i < NS_MAX_MODIFIERS) {
        if (world->modifiers[i].used == NS_FALSE) {
            slot = &world->modifiers[i];
            slot->used = NS_TRUE;
            slot->value_id = value_id;
            slot->code = code;
            slot->target = target;
            slot->mode = mode;
            slot->amount = amount;
            slot->duration_ticks = duration_ticks;
            slot->priority = priority;
            slot->flags = flags;
            temp = ns_recalculate_internal(world, value_id, NS_TRUE);
            if (temp != NS_OK) {
                return temp;
            }
            ns_push_event(world,
                          NS_EVENT_MODIFIER_ADDED,
                          value_id,
                          world->values[value_id].previous_value,
                          world->values[value_id].value,
                          NS_FX_ZERO,
                          i,
                          code);
            return i;
        }
        i++;
    }

    return NS_ERR_FULL;
}

int ns_remove_modifier_by_code(NS_World *world, ns_id value_id, int code)
{
    int i;
    int removed;
    int result;
    int temp;

    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    removed = 0;
    result = NS_OK;
    i = 0;
    while (i < NS_MAX_MODIFIERS) {
        if (world->modifiers[i].used == NS_TRUE) {
            if (world->modifiers[i].value_id == value_id && world->modifiers[i].code == code) {
                world->modifiers[i].used = NS_FALSE;
                world->modifiers[i].value_id = NS_INVALID_ID;
                removed++;
            }
        }
        i++;
    }

    if (removed > 0) {
        result = ns_recalculate_internal(world, value_id, NS_TRUE);
        temp = ns_push_event(world,
                             NS_EVENT_MODIFIER_REMOVED,
                             value_id,
                             world->values[value_id].previous_value,
                             world->values[value_id].value,
                             NS_FX_ZERO,
                             NS_INVALID_ID,
                             code);
        if (result == NS_OK && temp != NS_OK) {
            result = temp;
        }
    }

    return result;
}

int ns_clear_modifiers_by_value(NS_World *world, ns_id value_id)
{
    int i;
    int changed;

    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    changed = NS_FALSE;
    i = 0;
    while (i < NS_MAX_MODIFIERS) {
        if (world->modifiers[i].used == NS_TRUE && world->modifiers[i].value_id == value_id) {
            world->modifiers[i].used = NS_FALSE;
            world->modifiers[i].value_id = NS_INVALID_ID;
            changed = NS_TRUE;
        }
        i++;
    }

    if (changed == NS_TRUE) {
        return ns_recalculate_internal(world, value_id, NS_TRUE);
    }
    return NS_OK;
}

int ns_count_modifiers_by_value(const NS_World *world, ns_id value_id)
{
    int i;
    int count;

    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    count = 0;
    i = 0;
    while (i < NS_MAX_MODIFIERS) {
        if (world->modifiers[i].used == NS_TRUE && world->modifiers[i].value_id == value_id) {
            count++;
        }
        i++;
    }
    return count;
}

int ns_recalculate_by_id(NS_World *world, ns_id value_id)
{
    return ns_recalculate_internal(world, value_id, NS_TRUE);
}

ns_id ns_add_derived(NS_World *world,
                     ns_id output_value_id,
                     ns_id a_value_id,
                     ns_id b_value_id,
                     int mode,
                     ns_fx scale,
                     ns_fx offset,
                     int flags)
{
    int i;
    NS_Derived *slot;

    if (ns_valid_value_id(world, output_value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    if (ns_valid_value_id(world, a_value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    if (mode != NS_DERIVED_COPY && mode != NS_DERIVED_PERCENT) {
        if (ns_valid_value_id(world, b_value_id) == NS_FALSE) {
            return NS_ERR_BAD_ID;
        }
    }
    if (ns_valid_derived_mode(mode) == NS_FALSE) {
        return NS_ERR_BAD_ARG;
    }

    i = 0;
    while (i < NS_MAX_DERIVED) {
        if (world->derived[i].used == NS_FALSE) {
            slot = &world->derived[i];
            slot->used = NS_TRUE;
            slot->output_value_id = output_value_id;
            slot->a_value_id = a_value_id;
            slot->b_value_id = b_value_id;
            slot->mode = mode;
            slot->scale = scale;
            slot->offset = offset;
            slot->flags = flags;
            ns_update_derived(world, i);
            return i;
        }
        i++;
    }

    return NS_ERR_FULL;
}

int ns_update_derived(NS_World *world, ns_id derived_id)
{
    NS_Derived *d;
    ns_fx a;
    ns_fx b;
    ns_fx result;
    int temp;

    if (ns_valid_derived_id(world, derived_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    d = &world->derived[derived_id];
    if (ns_valid_value_id(world, d->output_value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    if (ns_valid_value_id(world, d->a_value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    a = world->values[d->a_value_id].value;
    b = NS_FX_ZERO;
    if (d->b_value_id != NS_INVALID_ID && ns_valid_value_id(world, d->b_value_id) == NS_TRUE) {
        b = world->values[d->b_value_id].value;
    }

    result = a;
    if (d->mode == NS_DERIVED_COPY) {
        result = a;
        result = ns_fx_mul(result, d->scale) + d->offset;
    } else if (d->mode == NS_DERIVED_SUM) {
        result = a + b;
        result = ns_fx_mul(result, d->scale) + d->offset;
    } else if (d->mode == NS_DERIVED_SUB) {
        result = a - b;
        result = ns_fx_mul(result, d->scale) + d->offset;
    } else if (d->mode == NS_DERIVED_PRODUCT) {
        result = ns_fx_mul(a, b);
        result = ns_fx_mul(result, d->scale) + d->offset;
    } else if (d->mode == NS_DERIVED_RATIO) {
        result = ns_fx_div(a, b);
        result = ns_fx_mul(result, d->scale) + d->offset;
    } else if (d->mode == NS_DERIVED_MIN) {
        result = ns_min_fx(a, b);
        result = ns_fx_mul(result, d->scale) + d->offset;
    } else if (d->mode == NS_DERIVED_MAX) {
        result = ns_max_fx(a, b);
        result = ns_fx_mul(result, d->scale) + d->offset;
    } else if (d->mode == NS_DERIVED_PERCENT) {
        temp = ns_percent_by_id(world, d->a_value_id, &result);
        if (temp != NS_OK) {
            return temp;
        }
        result = ns_fx_mul(result, d->scale) + d->offset;
    } else if (d->mode == NS_DERIVED_LERP) {
        result = ns_fx_lerp(a, b, d->scale) + d->offset;
    } else {
        return NS_ERR_BAD_ARG;
    }

    temp = ns_set_by_id(world, d->output_value_id, result);
    if (temp != NS_OK) {
        return temp;
    }
    return ns_push_event(world,
                         NS_EVENT_DERIVED_UPDATED,
                         d->output_value_id,
                         world->values[d->output_value_id].previous_value,
                         world->values[d->output_value_id].value,
                         NS_FX_ZERO,
                         derived_id,
                         d->mode);
}

int ns_update_all_derived(NS_World *world)
{
    int i;
    int result;
    int temp;

    if (world == NULL) {
        return NS_ERR_NULL;
    }

    result = NS_OK;
    i = 0;
    while (i < NS_MAX_DERIVED) {
        if (world->derived[i].used == NS_TRUE) {
            temp = ns_update_derived(world, i);
            if (result == NS_OK && temp != NS_OK) {
                result = temp;
            }
        }
        i++;
    }
    return result;
}

int ns_clear_derived_for_value(NS_World *world, ns_id value_id)
{
    int i;

    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    i = 0;
    while (i < NS_MAX_DERIVED) {
        if (world->derived[i].used == NS_TRUE) {
            if (world->derived[i].output_value_id == value_id || world->derived[i].a_value_id == value_id || world->derived[i].b_value_id == value_id) {
                world->derived[i].used = NS_FALSE;
                world->derived[i].output_value_id = NS_INVALID_ID;
                world->derived[i].a_value_id = NS_INVALID_ID;
                world->derived[i].b_value_id = NS_INVALID_ID;
            }
        }
        i++;
    }
    return NS_OK;
}

void ns_query_all(NS_Query *query)
{
    if (query == NULL) {
        return;
    }
    query->use_owner = NS_FALSE;
    query->owner = 0;
    query->use_type = NS_FALSE;
    query->type_id = NS_INVALID_ID;
    query->use_cmp = NS_FALSE;
    query->cmp = NS_CMP_EQ;
    query->rhs = NS_FX_ZERO;
    query->use_tag = NS_FALSE;
    query->tag = 0;
    query->flags_all = 0;
    query->flags_any = 0;
}

static int ns_query_matches(const NS_World *world, const NS_Query *query, ns_id value_id)
{
    const NS_Value *slot;

    if (query == NULL) {
        return NS_TRUE;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_FALSE;
    }

    slot = &world->values[value_id];

    if (query->use_owner == NS_TRUE && slot->owner != query->owner) {
        return NS_FALSE;
    }
    if (query->use_type == NS_TRUE && slot->type_id != query->type_id) {
        return NS_FALSE;
    }
    if (query->use_cmp == NS_TRUE) {
        if (ns_compare_by_id(world, value_id, query->cmp, query->rhs) == NS_FALSE) {
            return NS_FALSE;
        }
    }
    if (query->use_tag == NS_TRUE && slot->tag != query->tag) {
        return NS_FALSE;
    }
    if (query->flags_all != 0) {
        if ((slot->flags & query->flags_all) != query->flags_all) {
            return NS_FALSE;
        }
    }
    if (query->flags_any != 0) {
        if ((slot->flags & query->flags_any) == 0) {
            return NS_FALSE;
        }
    }

    return NS_TRUE;
}

int ns_query_next(const NS_World *world, const NS_Query *query, ns_id start_after, ns_id *out_value_id)
{
    int i;

    if (out_value_id == NULL) {
        return NS_ERR_NULL;
    }
    if (world == NULL) {
        return NS_ERR_NULL;
    }

    i = start_after + 1;
    if (i < 0) {
        i = 0;
    }
    while (i < NS_MAX_VALUES) {
        if (world->values[i].used == NS_TRUE) {
            if (ns_query_matches(world, query, i) == NS_TRUE) {
                *out_value_id = i;
                return NS_TRUE;
            }
        }
        i++;
    }

    *out_value_id = NS_INVALID_ID;
    return NS_FALSE;
}

int ns_query_count(const NS_World *world, const NS_Query *query, int *out_count)
{
    ns_id cursor;
    int count;
    int polled;

    if (out_count == NULL) {
        return NS_ERR_NULL;
    }
    if (world == NULL) {
        return NS_ERR_NULL;
    }

    count = 0;
    cursor = NS_INVALID_ID;
    polled = ns_query_next(world, query, cursor, &cursor);
    while (polled == NS_TRUE) {
        count++;
        polled = ns_query_next(world, query, cursor, &cursor);
    }
    *out_count = count;
    return NS_OK;
}

int ns_query_add(NS_World *world, const NS_Query *query, ns_fx amount, int *out_affected)
{
    ns_id cursor;
    int affected;
    int polled;
    int result;
    int temp;

    if (world == NULL) {
        return NS_ERR_NULL;
    }

    affected = 0;
    result = NS_OK;
    cursor = NS_INVALID_ID;
    polled = ns_query_next(world, query, cursor, &cursor);
    while (polled == NS_TRUE) {
        temp = ns_add_by_id(world, cursor, amount);
        if (result == NS_OK && temp != NS_OK) {
            result = temp;
        }
        affected++;
        polled = ns_query_next(world, query, cursor, &cursor);
    }

    if (out_affected != NULL) {
        *out_affected = affected;
    }
    return result;
}

int ns_query_sub(NS_World *world, const NS_Query *query, ns_fx amount, int *out_affected)
{
    ns_id cursor;
    int affected;
    int polled;
    int result;
    int temp;

    if (world == NULL) {
        return NS_ERR_NULL;
    }

    affected = 0;
    result = NS_OK;
    cursor = NS_INVALID_ID;
    polled = ns_query_next(world, query, cursor, &cursor);
    while (polled == NS_TRUE) {
        temp = ns_sub_by_id(world, cursor, amount);
        if (result == NS_OK && temp != NS_OK) {
            result = temp;
        }
        affected++;
        polled = ns_query_next(world, query, cursor, &cursor);
    }

    if (out_affected != NULL) {
        *out_affected = affected;
    }
    return result;
}

int ns_query_set(NS_World *world, const NS_Query *query, ns_fx value, int *out_affected)
{
    ns_id cursor;
    int affected;
    int polled;
    int result;
    int temp;

    if (world == NULL) {
        return NS_ERR_NULL;
    }

    affected = 0;
    result = NS_OK;
    cursor = NS_INVALID_ID;
    polled = ns_query_next(world, query, cursor, &cursor);
    while (polled == NS_TRUE) {
        temp = ns_set_by_id(world, cursor, value);
        if (result == NS_OK && temp != NS_OK) {
            result = temp;
        }
        affected++;
        polled = ns_query_next(world, query, cursor, &cursor);
    }

    if (out_affected != NULL) {
        *out_affected = affected;
    }
    return result;
}

ns_id ns_bind_value(NS_World *world, ns_id value_id, int kind, int user_code, int channel, int flags)
{
    int i;

    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    if (ns_valid_bind_kind(kind) == NS_FALSE) {
        return NS_ERR_BAD_ARG;
    }

    i = 0;
    while (i < NS_MAX_BINDINGS) {
        if (world->bindings[i].used == NS_FALSE) {
            world->bindings[i].used = NS_TRUE;
            world->bindings[i].value_id = value_id;
            world->bindings[i].kind = kind;
            world->bindings[i].user_code = user_code;
            world->bindings[i].channel = channel;
            world->bindings[i].flags = flags;
            world->values[value_id].flags = world->values[value_id].flags | NS_FLAG_HUD;
            return i;
        }
        i++;
    }
    return NS_ERR_FULL;
}

int ns_get_binding(const NS_World *world, ns_id binding_id, NS_Binding *out_binding)
{
    if (out_binding == NULL) {
        return NS_ERR_NULL;
    }
    if (ns_valid_binding_id(world, binding_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }
    *out_binding = world->bindings[binding_id];
    return NS_OK;
}

int ns_clear_bindings_for_value(NS_World *world, ns_id value_id)
{
    int i;

    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    i = 0;
    while (i < NS_MAX_BINDINGS) {
        if (world->bindings[i].used == NS_TRUE && world->bindings[i].value_id == value_id) {
            world->bindings[i].used = NS_FALSE;
            world->bindings[i].value_id = NS_INVALID_ID;
        }
        i++;
    }
    return NS_OK;
}

int ns_binding_count(const NS_World *world, ns_id value_id, int *out_count)
{
    int i;
    int count;

    if (out_count == NULL) {
        return NS_ERR_NULL;
    }
    if (ns_valid_value_id(world, value_id) == NS_FALSE) {
        return NS_ERR_BAD_ID;
    }

    count = 0;
    i = 0;
    while (i < NS_MAX_BINDINGS) {
        if (world->bindings[i].used == NS_TRUE && world->bindings[i].value_id == value_id) {
            count++;
        }
        i++;
    }
    *out_count = count;
    return NS_OK;
}

int ns_export_values(const NS_World *world, NS_SnapshotValue *out_values, int max_out, int *out_count)
{
    int i;
    int count;
    const NS_Value *slot;

    if (world == NULL || out_count == NULL) {
        return NS_ERR_NULL;
    }
    if (max_out < 0) {
        return NS_ERR_BAD_ARG;
    }

    count = 0;
    i = 0;
    while (i < NS_MAX_VALUES) {
        if (world->values[i].used == NS_TRUE) {
            if (out_values != NULL && count < max_out) {
                slot = &world->values[i];
                out_values[count].used = NS_TRUE;
                out_values[count].owner = slot->owner;
                ns_name_copy(out_values[count].type_name, world->types[slot->type_id].name);
                out_values[count].base_value = slot->base_value;
                out_values[count].initial_value = slot->initial_value;
                out_values[count].base_min_value = slot->base_min_value;
                out_values[count].base_max_value = slot->base_max_value;
                out_values[count].base_regen_per_tick = slot->base_regen_per_tick;
                out_values[count].base_drain_per_tick = slot->base_drain_per_tick;
                out_values[count].overflow = slot->overflow;
                out_values[count].kind = slot->kind;
                out_values[count].flags = slot->flags;
                out_values[count].tag = slot->tag;
            }
            count++;
        }
        i++;
    }

    *out_count = count;
    if (out_values != NULL && count > max_out) {
        return NS_ERR_FULL;
    }
    return NS_OK;
}

int ns_import_values(NS_World *world, const NS_SnapshotValue *values, int count)
{
    int i;
    ns_id type_id;
    ns_id value_id;
    NS_Value *slot;

    if (world == NULL || values == NULL) {
        return NS_ERR_NULL;
    }
    if (count < 0) {
        return NS_ERR_BAD_ARG;
    }

    i = 0;
    while (i < count) {
        if (values[i].used == NS_TRUE) {
            type_id = ns_find_type(world, values[i].type_name);
            if (type_id < 0) {
                type_id = ns_define_type(world,
                                         values[i].type_name,
                                         NS_SCOPE_INSTANCE,
                                         values[i].kind,
                                         values[i].initial_value,
                                         values[i].base_min_value,
                                         values[i].base_max_value,
                                         values[i].overflow,
                                         values[i].flags);
                if (type_id < 0) {
                    return type_id;
                }
            }
            value_id = ns_attach_type(world, values[i].owner, type_id);
            if (value_id < 0) {
                return value_id;
            }
            slot = &world->values[value_id];
            slot->base_value = values[i].base_value;
            slot->initial_value = values[i].initial_value;
            slot->base_min_value = values[i].base_min_value;
            slot->base_max_value = values[i].base_max_value;
            slot->base_regen_per_tick = values[i].base_regen_per_tick;
            slot->base_drain_per_tick = values[i].base_drain_per_tick;
            slot->overflow = values[i].overflow;
            slot->kind = values[i].kind;
            slot->flags = values[i].flags;
            slot->tag = values[i].tag;
            ns_recalculate_internal(world, value_id, NS_FALSE);
            slot->changed = NS_FALSE;
            slot->dirty = NS_FALSE;
        }
        i++;
    }

    return NS_OK;
}

int ns_save_values(const NS_World *world, NS_WriteProc write_proc, void *user_data)
{
    NS_SnapshotValue item;
    int i;
    int count;
    int result;

    if (world == NULL || write_proc == NULL) {
        return NS_ERR_NULL;
    }

    count = 0;
    i = 0;
    while (i < NS_MAX_VALUES) {
        if (world->values[i].used == NS_TRUE) {
            count++;
        }
        i++;
    }

    result = write_proc(user_data, &count, (unsigned int)sizeof(count));
    if (result != NS_OK) {
        return result;
    }

    i = 0;
    while (i < NS_MAX_VALUES) {
        if (world->values[i].used == NS_TRUE) {
            item.used = NS_TRUE;
            item.owner = world->values[i].owner;
            ns_name_copy(item.type_name, world->types[world->values[i].type_id].name);
            item.base_value = world->values[i].base_value;
            item.initial_value = world->values[i].initial_value;
            item.base_min_value = world->values[i].base_min_value;
            item.base_max_value = world->values[i].base_max_value;
            item.base_regen_per_tick = world->values[i].base_regen_per_tick;
            item.base_drain_per_tick = world->values[i].base_drain_per_tick;
            item.overflow = world->values[i].overflow;
            item.kind = world->values[i].kind;
            item.flags = world->values[i].flags;
            item.tag = world->values[i].tag;
            result = write_proc(user_data, &item, (unsigned int)sizeof(item));
            if (result != NS_OK) {
                return result;
            }
        }
        i++;
    }

    return NS_OK;
}

int ns_load_values(NS_World *world, NS_ReadProc read_proc, void *user_data, int count)
{
    int i;
    int result;
    NS_SnapshotValue item;

    if (world == NULL || read_proc == NULL) {
        return NS_ERR_NULL;
    }
    if (count < 0) {
        return NS_ERR_BAD_ARG;
    }

    i = 0;
    while (i < count) {
        result = read_proc(user_data, &item, (unsigned int)sizeof(item));
        if (result != NS_OK) {
            return result;
        }
        result = ns_import_values(world, &item, 1);
        if (result != NS_OK) {
            return result;
        }
        i++;
    }

    return NS_OK;
}

int ns_clear_owner(NS_World *world, ns_owner owner)
{
    int i;

    if (world == NULL) {
        return NS_ERR_NULL;
    }

    i = 0;
    while (i < NS_MAX_VALUES) {
        if (world->values[i].used == NS_TRUE && world->values[i].owner == owner) {
            ns_clear_thresholds_for_value(world, i);
            ns_clear_modifiers_by_value(world, i);
            ns_clear_derived_for_value(world, i);
            ns_clear_bindings_for_value(world, i);
            world->values[i].used = NS_FALSE;
            world->values[i].type_id = NS_INVALID_ID;
        }
        i++;
    }
    return NS_OK;
}

int ns_count_values(const NS_World *world, int *out_count)
{
    int i;
    int count;

    if (world == NULL || out_count == NULL) {
        return NS_ERR_NULL;
    }

    count = 0;
    i = 0;
    while (i < NS_MAX_VALUES) {
        if (world->values[i].used == NS_TRUE) {
            count++;
        }
        i++;
    }
    *out_count = count;
    return NS_OK;
}

void ns_set_callback(NS_World *world, NS_EventProc callback, void *user_data)
{
    if (world == NULL) {
        return;
    }
    world->callback = callback;
    world->callback_user_data = user_data;
}

int ns_poll_event(NS_World *world, NS_Event *out_event)
{
    int i;

    if (world == NULL) {
        return NS_ERR_NULL;
    }
    if (world->event_count <= 0) {
        return NS_FALSE;
    }

    if (out_event != NULL) {
        *out_event = world->events[0];
    }

    i = 1;
    while (i < world->event_count) {
        world->events[i - 1] = world->events[i];
        i++;
    }

    world->event_count--;
    if (world->event_count >= 0) {
        world->events[world->event_count].event_type = 0;
        world->events[world->event_count].value_id = NS_INVALID_ID;
        world->events[world->event_count].type_id = NS_INVALID_ID;
        world->events[world->event_count].owner = 0;
        world->events[world->event_count].previous_value = NS_FX_ZERO;
        world->events[world->event_count].value = NS_FX_ZERO;
        world->events[world->event_count].threshold = NS_FX_ZERO;
        world->events[world->event_count].threshold_id = NS_INVALID_ID;
        world->events[world->event_count].user_code = 0;
    }

    return NS_TRUE;
}

int ns_dispatch_events(NS_World *world)
{
    NS_Event event_data;
    int polled;
    int dispatched;

    if (world == NULL) {
        return NS_ERR_NULL;
    }
    if (world->callback == NULL) {
        ns_clear_events(world);
        return NS_OK;
    }

    dispatched = 0;
    polled = ns_poll_event(world, &event_data);
    while (polled == NS_TRUE) {
        world->callback(world, &event_data, world->callback_user_data);
        dispatched++;
        polled = ns_poll_event(world, &event_data);
    }

    return dispatched;
}

void ns_clear_events(NS_World *world)
{
    int i;

    if (world == NULL) {
        return;
    }

    i = 0;
    while (i < NS_MAX_EVENTS) {
        world->events[i].event_type = 0;
        world->events[i].value_id = NS_INVALID_ID;
        world->events[i].type_id = NS_INVALID_ID;
        world->events[i].owner = 0;
        world->events[i].previous_value = NS_FX_ZERO;
        world->events[i].value = NS_FX_ZERO;
        world->events[i].threshold = NS_FX_ZERO;
        world->events[i].threshold_id = NS_INVALID_ID;
        world->events[i].user_code = 0;
        i++;
    }

    world->event_count = 0;
}
