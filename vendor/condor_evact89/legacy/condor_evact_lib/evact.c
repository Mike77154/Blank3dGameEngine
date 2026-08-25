/* evact.c - C89, reglas estáticas */

#include "evact.h"

#ifndef EVACT_MAX_RULES
#define EVACT_MAX_RULES 16
#endif

typedef struct {
    evcore_value_t type;
    evcore_value_t code;
    condcore_id    cond_id;
    actcore_id     act_id;
} evact_rule_entry;

static evact_rule_entry evact_rules[EVACT_MAX_RULES];
static evact_rule_id    evact_serials[EVACT_MAX_RULES];

static evact_rule_id evact_serial_limit(void)
{
    evact_rule_id max_id;

    max_id = (evact_rule_id)~((evact_rule_id)0);
    max_id = max_id / (evact_rule_id)EVACT_MAX_RULES;
    if (max_id == (evact_rule_id)0) {
        max_id = (evact_rule_id)1;
    }
    return max_id;
}

static evact_rule_id evact_make_id(int slot, evact_rule_id serial)
{
    return (evact_rule_id)(((serial - (evact_rule_id)1) *
                            (evact_rule_id)EVACT_MAX_RULES) +
                           (evact_rule_id)(slot + 1));
}

static int evact_decode_id(evact_rule_id id, int *slot, evact_rule_id *serial)
{
    evact_rule_id raw;

    if (id == EVACT_RULE_INVALID) {
        return 0;
    }

    raw = id - (evact_rule_id)1;
    *slot = (int)(raw % (evact_rule_id)EVACT_MAX_RULES);
    *serial = (raw / (evact_rule_id)EVACT_MAX_RULES) + (evact_rule_id)1;
    return 1;
}

static int evact_rule_matches(const evact_rule_entry *rule,
                              const evcore_event_t *evt)
{
    if (rule->act_id == ACTCORE_ID_INVALID) {
        return 0;
    }
    if (rule->type != EVCORE_MATCH_ANY && rule->type != evt->type) {
        return 0;
    }
    if (rule->code != EVCORE_MATCH_ANY && rule->code != evt->code) {
        return 0;
    }
    return 1;
}

static evact_rule_id evact_fill_slot(int slot,
                                     evcore_value_t type,
                                     evcore_value_t code,
                                     condcore_id cond_id,
                                     actcore_id act_id)
{
    evact_rule_id limit;
    evact_rule_id serial;

    limit = evact_serial_limit();
    serial = evact_serials[slot] + (evact_rule_id)1;
    if (serial == (evact_rule_id)0 || serial > limit) {
        serial = (evact_rule_id)1;
    }

    evact_serials[slot] = serial;
    evact_rules[slot].type = type;
    evact_rules[slot].code = code;
    evact_rules[slot].cond_id = cond_id;
    evact_rules[slot].act_id = act_id;
    return evact_make_id(slot, serial);
}

evact_rule_id evact_rule_register(evcore_value_t type,
                                  evcore_value_t code,
                                  condcore_id cond_id,
                                  actcore_id act_id)
{
    int i;

    if (act_id == ACTCORE_ID_INVALID) {
        return EVACT_RULE_INVALID;
    }

    for (i = 0; i < EVACT_MAX_RULES; ++i) {
        if (evact_rules[i].act_id == ACTCORE_ID_INVALID) {
            return evact_fill_slot(i, type, code, cond_id, act_id);
        }
    }

    return EVACT_RULE_INVALID;
}

evact_rule_id evact_rule_register_unique(evcore_value_t type,
                                         evcore_value_t code,
                                         condcore_id cond_id,
                                         actcore_id act_id)
{
    int i;

    if (act_id == ACTCORE_ID_INVALID) {
        return EVACT_RULE_INVALID;
    }

    for (i = 0; i < EVACT_MAX_RULES; ++i) {
        if (evact_rules[i].act_id != ACTCORE_ID_INVALID &&
            evact_rules[i].type == type &&
            evact_rules[i].code == code &&
            evact_rules[i].cond_id == cond_id &&
            evact_rules[i].act_id == act_id) {
            return evact_make_id(i, evact_serials[i]);
        }
    }

    return evact_rule_register(type, code, cond_id, act_id);
}

int evact_rule_unregister(evact_rule_id id)
{
    int slot;
    evact_rule_id serial;

    if (!evact_decode_id(id, &slot, &serial)) {
        return 0;
    }

    if (evact_rules[slot].act_id == ACTCORE_ID_INVALID) {
        return 0;
    }

    if (evact_serials[slot] != serial) {
        return 0;
    }

    evact_rules[slot].type = (evcore_value_t)0;
    evact_rules[slot].code = (evcore_value_t)0;
    evact_rules[slot].cond_id = CONDCORE_ID_INVALID;
    evact_rules[slot].act_id = ACTCORE_ID_INVALID;
    return 1;
}

int evact_process(const evcore_event_t *evt)
{
    int i;
    int snapshot_count;
    int snapshot_slots[EVACT_MAX_RULES];
    evact_rule_id snapshot_serials[EVACT_MAX_RULES];
    int slot;
    evcore_event_t local_evt;
    int executed;

    if (evt == 0) {
        return 0;
    }

    local_evt = *evt;
    snapshot_count = 0;
    executed = 0;

    for (i = 0; i < EVACT_MAX_RULES; ++i) {
        if (evact_rules[i].act_id != ACTCORE_ID_INVALID &&
            evact_rule_matches(&evact_rules[i], &local_evt)) {
            snapshot_slots[snapshot_count] = i;
            snapshot_serials[snapshot_count] = evact_serials[i];
            ++snapshot_count;
        }
    }

    for (i = 0; i < snapshot_count; ++i) {
        slot = snapshot_slots[i];

        if (evact_rules[slot].act_id == ACTCORE_ID_INVALID) {
            continue;
        }
        if (evact_serials[slot] != snapshot_serials[i]) {
            continue;
        }
        if (!evact_rule_matches(&evact_rules[slot], &local_evt)) {
            continue;
        }
        if (evact_rules[slot].cond_id != EVACT_COND_ALWAYS &&
            !condcore_eval(evact_rules[slot].cond_id, &local_evt)) {
            continue;
        }

        executed += actcore_exec(evact_rules[slot].act_id, &local_evt);
    }

    return executed;
}

void evact_listener(const evcore_event_t *evt, void *user)
{
    (void)user;
    evact_process(evt);
}

evcore_sub_id evact_attach_all(void)
{
    return evcore_subscribe_unique(EVCORE_MATCH_ANY, evact_listener, 0);
}

void evact_reset(void)
{
    int i;

    for (i = 0; i < EVACT_MAX_RULES; ++i) {
        evact_rules[i].type = (evcore_value_t)0;
        evact_rules[i].code = (evcore_value_t)0;
        evact_rules[i].cond_id = CONDCORE_ID_INVALID;
        evact_rules[i].act_id = ACTCORE_ID_INVALID;
    }
}
