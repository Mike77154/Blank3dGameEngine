#include "blank3d_condor.h"
#include <string.h>
#include <stdio.h>

static void b3d_cea_status(Blank3DCondor *c, const char *text)
{
    if (!c) return;
    if (!text) text = "";
    strncpy(c->status, text, sizeof(c->status) - 1U);
    c->status[sizeof(c->status) - 1U] = '\0';
}

static int b3d_copy_name(char *dst, unsigned int cap, const char *src)
{
    unsigned int n;
    if (!dst || cap == 0U || !src || !*src) return 0;
    n = (unsigned int)strlen(src);
    if (n + 1U > cap) return 0;
    memcpy(dst, src, n + 1U);
    return 1;
}

static int b3d_vm_q16(const vm89_value *v, long *out)
{
    if (!v || !out) return 0;
    if (v->type == VM89_VALUE_FIXED) { *out = v->fixed_q16; return 1; }
    if (v->type == VM89_VALUE_BOOL) {
        *out = v->boolean ? 65536L : 0L;
        return 1;
    }
    return 0;
}

static int b3d_condor_gameverb_condition_cb(const cea89_event *event, void *user)
{
    Blank3DCondorVerbCondition *d;
    Blank3DCondor *c;
    gverb89_call call;
    gverb89_result out;
    int r;
    d = (Blank3DCondorVerbCondition *)user;
    if (!d || !d->owner_ctx || !event) return 0;
    c = (Blank3DCondor *)d->owner_ctx;
    if (!c->gameverbs) return 0;
    memset(&call, 0, sizeof(call));
    memset(&out, 0, sizeof(out));
    call.owner = event->owner;
    call.subject = event->subject;
    call.name = d->name;
    r = gverb89_query(c->gameverbs, &call, &out);
    return r == GVERB89_HANDLED && out.truth;
}

static void b3d_condor_gameverb_action_cb(const cea89_event *event, void *user)
{
    Blank3DCondorVerbAction *d;
    Blank3DCondor *c;
    gverb89_call call;
    d = (Blank3DCondorVerbAction *)user;
    if (!d || !d->owner_ctx || !event) return;
    c = (Blank3DCondor *)d->owner_ctx;
    if (!c->gameverbs) return;
    memset(&call, 0, sizeof(call));
    call.owner = event->owner;
    call.subject = event->subject;
    call.name = d->name;
    (void)gverb89_perform(c->gameverbs, &call);
}

static int b3d_condor_var_condition_cb(const cea89_event *event, void *user)
{
    Blank3DCondorVarCondition *d;
    Blank3DCondor *c;
    vm89_value value;
    unsigned long owner;
    long actual;
    d = (Blank3DCondorVarCondition *)user;
    if (!d || !d->owner_ctx || !event) return 0;
    c = (Blank3DCondor *)d->owner_ctx;
    if (!c->variables) return 0;
    owner = d->fixed_owner ? d->fixed_owner : event->instance;
    memset(&value, 0, sizeof(value));
    if (!blank3d_variables_get(c->variables, d->scope, owner,
                               d->name, &value)) return 0;
    if (!b3d_vm_q16(&value, &actual)) return 0;
    if (d->compare == B3D_CEA_CMP_EQ) return actual == d->value_q16;
    if (d->compare == B3D_CEA_CMP_NE) return actual != d->value_q16;
    if (d->compare == B3D_CEA_CMP_LT) return actual < d->value_q16;
    if (d->compare == B3D_CEA_CMP_LE) return actual <= d->value_q16;
    if (d->compare == B3D_CEA_CMP_GT) return actual > d->value_q16;
    if (d->compare == B3D_CEA_CMP_GE) return actual >= d->value_q16;
    return 0;
}

static void b3d_condor_var_action_cb(const cea89_event *event, void *user)
{
    Blank3DCondorVarAction *d;
    Blank3DCondor *c;
    unsigned long owner;
    int r;
    d = (Blank3DCondorVarAction *)user;
    if (!d || !d->owner_ctx || !event) return;
    c = (Blank3DCondor *)d->owner_ctx;
    if (!c->variables) return;
    owner = d->fixed_owner ? d->fixed_owner : event->instance;
    if (d->operation == B3D_CEA_VAR_SET)
        r = vr89_set(&c->variables->runtime, d->scope, owner, d->name, &d->value);
    else if (d->operation == B3D_CEA_VAR_ADD)
        r = vr89_add(&c->variables->runtime, d->scope, owner, d->name, &d->value);
    else if (d->operation == B3D_CEA_VAR_SUB)
        r = vr89_sub(&c->variables->runtime, d->scope, owner, d->name, &d->value);
    else r = VR89_ERR_BAD_TYPE;
    if (r != VR89_OK) b3d_cea_status(c, vr89_result_string(r));
}

void blank3d_condor_init(Blank3DCondor *condor,
                         gverb89_registry *gameverbs,
                         Blank3DVariables *variables)
{
    if (!condor) return;
    memset(condor, 0, sizeof(*condor));
    cea89_init(&condor->core);
    condor->gameverbs = gameverbs;
    condor->variables = variables;
    condor->initialized = 1;
    b3d_cea_status(condor, "Condor EvAct89 online: Event -> Condition -> Action");
}

void blank3d_condor_reset_rules(Blank3DCondor *condor)
{
    if (!condor) return;
    cea89_reset(&condor->core);
    memset(condor->verb_conditions, 0, sizeof(condor->verb_conditions));
    memset(condor->verb_actions, 0, sizeof(condor->verb_actions));
    memset(condor->var_conditions, 0, sizeof(condor->var_conditions));
    memset(condor->var_actions, 0, sizeof(condor->var_actions));
    b3d_cea_status(condor, "Condor rules reset; serials preserved");
}

cea89_condition_id blank3d_condor_condition_gameverb(Blank3DCondor *condor,
                                                       const char *name)
{
    int i;
    Blank3DCondorVerbCondition *d;
    if (!condor || !condor->initialized || !name || !*name) return CEA89_ID_INVALID;
    for (i = 0; i < B3D_CONDOR_MAX_GAMEVERB_CONDITIONS; ++i)
        if (condor->verb_conditions[i].used &&
            strcmp(condor->verb_conditions[i].name, name) == 0)
            return condor->verb_conditions[i].id;
    for (i = 0; i < B3D_CONDOR_MAX_GAMEVERB_CONDITIONS; ++i) {
        d = &condor->verb_conditions[i];
        if (d->used) continue;
        memset(d, 0, sizeof(*d));
        if (!b3d_copy_name(d->name, sizeof(d->name), name)) return CEA89_ID_INVALID;
        d->owner_ctx = condor;
        d->id = cea89_condition_register(&condor->core,
                                         b3d_condor_gameverb_condition_cb, d);
        if (!d->id) { memset(d, 0, sizeof(*d)); return CEA89_ID_INVALID; }
        d->used = 1;
        return d->id;
    }
    b3d_cea_status(condor, "Condor gameverb condition descriptor capacity exhausted");
    return CEA89_ID_INVALID;
}

cea89_action_id blank3d_condor_action_gameverb(Blank3DCondor *condor,
                                                const char *name)
{
    int i;
    Blank3DCondorVerbAction *d;
    if (!condor || !condor->initialized || !name || !*name) return CEA89_ID_INVALID;
    for (i = 0; i < B3D_CONDOR_MAX_GAMEVERB_ACTIONS; ++i)
        if (condor->verb_actions[i].used &&
            strcmp(condor->verb_actions[i].name, name) == 0)
            return condor->verb_actions[i].id;
    for (i = 0; i < B3D_CONDOR_MAX_GAMEVERB_ACTIONS; ++i) {
        d = &condor->verb_actions[i];
        if (d->used) continue;
        memset(d, 0, sizeof(*d));
        if (!b3d_copy_name(d->name, sizeof(d->name), name)) return CEA89_ID_INVALID;
        d->owner_ctx = condor;
        d->id = cea89_action_register(&condor->core,
                                      b3d_condor_gameverb_action_cb, d);
        if (!d->id) { memset(d, 0, sizeof(*d)); return CEA89_ID_INVALID; }
        d->used = 1;
        return d->id;
    }
    b3d_cea_status(condor, "Condor gameverb action descriptor capacity exhausted");
    return CEA89_ID_INVALID;
}

cea89_condition_id blank3d_condor_condition_var_q16(Blank3DCondor *condor,
                                                     vr89_scope scope,
                                                     unsigned long fixed_owner,
                                                     const char *name,
                                                     int compare,
                                                     long value_q16)
{
    int i;
    Blank3DCondorVarCondition *d;
    if (!condor || !condor->initialized || !name || !*name) return CEA89_ID_INVALID;
    for (i = 0; i < B3D_CONDOR_MAX_VAR_CONDITIONS; ++i) {
        d = &condor->var_conditions[i];
        if (d->used) continue;
        memset(d, 0, sizeof(*d));
        if (!b3d_copy_name(d->name, sizeof(d->name), name)) return CEA89_ID_INVALID;
        d->scope = scope; d->fixed_owner = fixed_owner;
        d->compare = compare; d->value_q16 = value_q16; d->owner_ctx = condor;
        d->id = cea89_condition_register(&condor->core,
                                         b3d_condor_var_condition_cb, d);
        if (!d->id) { memset(d, 0, sizeof(*d)); return CEA89_ID_INVALID; }
        d->used = 1;
        return d->id;
    }
    b3d_cea_status(condor, "Condor variable condition descriptor capacity exhausted");
    return CEA89_ID_INVALID;
}

static cea89_action_id b3d_condor_action_var(Blank3DCondor *condor,
                                              vr89_scope scope,
                                              unsigned long fixed_owner,
                                              const char *name,
                                              int operation,
                                              const vm89_value *value)
{
    int i;
    Blank3DCondorVarAction *d;
    if (!condor || !condor->initialized || !name || !*name || !value)
        return CEA89_ID_INVALID;
    for (i = 0; i < B3D_CONDOR_MAX_VAR_ACTIONS; ++i) {
        d = &condor->var_actions[i];
        if (d->used) continue;
        memset(d, 0, sizeof(*d));
        if (!b3d_copy_name(d->name, sizeof(d->name), name)) return CEA89_ID_INVALID;
        d->scope = scope; d->fixed_owner = fixed_owner;
        d->operation = operation; d->value = *value; d->owner_ctx = condor;
        d->id = cea89_action_register(&condor->core,
                                      b3d_condor_var_action_cb, d);
        if (!d->id) { memset(d, 0, sizeof(*d)); return CEA89_ID_INVALID; }
        d->used = 1;
        return d->id;
    }
    b3d_cea_status(condor, "Condor variable action descriptor capacity exhausted");
    return CEA89_ID_INVALID;
}

cea89_action_id blank3d_condor_action_var_q16(Blank3DCondor *condor,
                                               vr89_scope scope,
                                               unsigned long fixed_owner,
                                               const char *name,
                                               int operation,
                                               long value_q16)
{
    vm89_value value;
    vm89_value_fixed_raw(&value, value_q16);
    return b3d_condor_action_var(condor, scope, fixed_owner,
                                 name, operation, &value);
}

cea89_action_id blank3d_condor_action_var_bool(Blank3DCondor *condor,
                                                vr89_scope scope,
                                                unsigned long fixed_owner,
                                                const char *name,
                                                int value_bool)
{
    vm89_value value;
    vm89_value_bool(&value, value_bool);
    return b3d_condor_action_var(condor, scope, fixed_owner,
                                 name, B3D_CEA_VAR_SET, &value);
}

cea89_rule_id blank3d_condor_rule(Blank3DCondor *condor,
                                  int event_type, int event_code,
                                  cea89_condition_id condition,
                                  cea89_action_id action)
{
    if (!condor || !condor->initialized) return CEA89_ID_INVALID;
    return cea89_rule_register_unique(&condor->core,
                                      (cea89_value)event_type,
                                      (cea89_value)event_code,
                                      condition, action);
}

cea89_rule_id blank3d_condor_rule_gameverbs(Blank3DCondor *condor,
                                            int event_type, int event_code,
                                            const char *condition_name,
                                            const char *action_name)
{
    cea89_condition_id c;
    cea89_action_id a;
    if (!condor || !action_name || !*action_name) return CEA89_ID_INVALID;
    c = CEA89_COND_ALWAYS;
    if (condition_name && *condition_name) {
        c = blank3d_condor_condition_gameverb(condor, condition_name);
        if (!c) return CEA89_ID_INVALID;
    }
    a = blank3d_condor_action_gameverb(condor, action_name);
    if (!a) return CEA89_ID_INVALID;
    return blank3d_condor_rule(condor, event_type, event_code, c, a);
}

int blank3d_condor_emit(Blank3DCondor *condor,
                        int event_type, int event_code,
                        unsigned long owner, unsigned long instance,
                        void *subject, void *data)
{
    if (!condor || !condor->initialized) return 0;
    return cea89_emit_values(&condor->core,
                             (cea89_value)event_type,
                             (cea89_value)event_code,
                             owner, instance, subject, data);
}

void blank3d_condor_emit_input(Blank3DCondor *condor,
                               const Blank3DInput *input,
                               unsigned long owner,
                               unsigned long instance,
                               void *subject)
{
    int usage;
    int bank;
    int bit;
    int i;
    const InputScanner *s;
    if (!condor || !condor->initialized || !input) return;
    for (usage = 0; usage < 256; ++usage) {
        bank = usage / B3D_INPUT_BANK_WIDTH;
        bit = usage % B3D_INPUT_BANK_WIDTH;
        s = &input->key_banks[bank];
        if (input_button_pressed(s, bit)) {
            (void)blank3d_condor_emit(condor, B3D_CEA_EVENT_INPUT_PRESS,
                usage, owner, instance, subject, 0); ++condor->input_events;
        } else if (input_button_hold(s, bit)) {
            (void)blank3d_condor_emit(condor, B3D_CEA_EVENT_INPUT_HOLD,
                usage, owner, instance, subject, 0); ++condor->input_events;
        }
        if (input_button_released(s, bit)) {
            (void)blank3d_condor_emit(condor, B3D_CEA_EVENT_INPUT_RELEASE,
                usage, owner, instance, subject, 0); ++condor->input_events;
        }
    }
    for (i = 0; i < 5; ++i) {
        if (input->mouse_pressed[i]) {
            (void)blank3d_condor_emit(condor, B3D_CEA_EVENT_INPUT_PRESS,
                B3D_CEA_MOUSE_CODE_BASE + i, owner, instance, subject, 0);
            ++condor->input_events;
        } else if (input->mouse_current[i]) {
            (void)blank3d_condor_emit(condor, B3D_CEA_EVENT_INPUT_HOLD,
                B3D_CEA_MOUSE_CODE_BASE + i, owner, instance, subject, 0);
            ++condor->input_events;
        }
        if (input->mouse_released[i]) {
            (void)blank3d_condor_emit(condor, B3D_CEA_EVENT_INPUT_RELEASE,
                B3D_CEA_MOUSE_CODE_BASE + i, owner, instance, subject, 0);
            ++condor->input_events;
        }
    }
}

void blank3d_condor_emit_thing(Blank3DCondor *condor,
                               int event_type, unsigned long thing_owner,
                               int actor_id, unsigned long entity_id,
                               void *subject)
{
    unsigned long actor_owner;
    actor_owner = actor_id > 0 ? (unsigned long)actor_id : thing_owner;
    (void)blank3d_condor_emit(condor, event_type, (int)entity_id,
                              actor_owner, thing_owner, subject, 0);
    if (condor) ++condor->thing_events;
}

void blank3d_condor_emit_actor(Blank3DCondor *condor,
                               int event_type, int actor_id,
                               unsigned long thing_owner,
                               int source_actor, void *subject, void *data)
{
    (void)blank3d_condor_emit(condor, event_type, source_actor,
                              (unsigned long)actor_id, thing_owner,
                              subject, data);
    if (condor) ++condor->actor_events;
}


int blank3d_condor_event_type_from_name(const char *name, int *out_type)
{
    struct B3DCondorEventName { const char *name; int type; };
    static const struct B3DCondorEventName names[] = {
        {"input_press", B3D_CEA_EVENT_INPUT_PRESS},
        {"input_hold", B3D_CEA_EVENT_INPUT_HOLD},
        {"input_release", B3D_CEA_EVENT_INPUT_RELEASE},
        {"thing_spawn", B3D_CEA_EVENT_THING_SPAWN},
        {"thing_destroy", B3D_CEA_EVENT_THING_DESTROY},
        {"actor_damage", B3D_CEA_EVENT_ACTOR_DAMAGE},
        {"actor_death", B3D_CEA_EVENT_ACTOR_DEATH},
        {"gfo_begin", B3D_CEA_EVENT_GFO_BEGIN},
        {"gfo_end", B3D_CEA_EVENT_GFO_END},
        {"custom", B3D_CEA_EVENT_CUSTOM}
    };
    unsigned int i;
    if (!name || !out_type) return 0;
    for (i = 0U; i < (unsigned int)(sizeof(names) / sizeof(names[0])); ++i) {
        if (strcmp(name, names[i].name) == 0) {
            *out_type = names[i].type;
            return 1;
        }
    }
    return 0;
}

const char *blank3d_condor_event_type_name(int event_type)
{
    if (event_type == B3D_CEA_EVENT_INPUT_PRESS) return "input_press";
    if (event_type == B3D_CEA_EVENT_INPUT_HOLD) return "input_hold";
    if (event_type == B3D_CEA_EVENT_INPUT_RELEASE) return "input_release";
    if (event_type == B3D_CEA_EVENT_THING_SPAWN) return "thing_spawn";
    if (event_type == B3D_CEA_EVENT_THING_DESTROY) return "thing_destroy";
    if (event_type == B3D_CEA_EVENT_ACTOR_DAMAGE) return "actor_damage";
    if (event_type == B3D_CEA_EVENT_ACTOR_DEATH) return "actor_death";
    if (event_type == B3D_CEA_EVENT_GFO_BEGIN) return "gfo_begin";
    if (event_type == B3D_CEA_EVENT_GFO_END) return "gfo_end";
    if (event_type == B3D_CEA_EVENT_CUSTOM) return "custom";
    return "unknown";
}

const char *blank3d_condor_status(const Blank3DCondor *condor)
{
    return condor ? condor->status : "Condor unavailable";
}
