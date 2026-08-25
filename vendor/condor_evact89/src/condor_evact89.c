#include "condor_evact89.h"
#include <string.h>

static cea89_id cea89_serial_limit(cea89_id capacity)
{
    cea89_id max_id;
    if (capacity == (cea89_id)0) return (cea89_id)1;
    max_id = (cea89_id)~((cea89_id)0);
    max_id /= capacity;
    if (max_id == (cea89_id)0) max_id = (cea89_id)1;
    return max_id;
}

static cea89_id cea89_next_serial(cea89_id current, cea89_id capacity)
{
    cea89_id next;
    cea89_id limit;
    limit = cea89_serial_limit(capacity);
    next = current + (cea89_id)1;
    if (next == (cea89_id)0 || next > limit) next = (cea89_id)1;
    return next;
}

static cea89_id cea89_make_id(int slot, cea89_id serial, cea89_id capacity)
{
    return ((serial - (cea89_id)1) * capacity) + (cea89_id)(slot + 1);
}

static int cea89_decode_id(cea89_id id, cea89_id capacity,
                           int *slot, cea89_id *serial)
{
    cea89_id raw;
    if (!slot || !serial || id == CEA89_ID_INVALID || capacity == 0) return 0;
    raw = id - (cea89_id)1;
    *slot = (int)(raw % capacity);
    *serial = (raw / capacity) + (cea89_id)1;
    return 1;
}

void cea89_init(cea89_context *ctx)
{
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->last_result = CEA89_OK;
}

void cea89_reset(cea89_context *ctx)
{
    int i;
    if (!ctx) return;
    for (i = 0; i < CEA89_MAX_LISTENERS; ++i) {
        ctx->listeners[i].type = 0;
        ctx->listeners[i].fn = 0;
        ctx->listeners[i].user = 0;
    }
    for (i = 0; i < CEA89_MAX_CONDITIONS; ++i) {
        ctx->conditions[i].fn = 0;
        ctx->conditions[i].user = 0;
    }
    for (i = 0; i < CEA89_MAX_ACTIONS; ++i) {
        ctx->actions[i].fn = 0;
        ctx->actions[i].user = 0;
    }
    for (i = 0; i < CEA89_MAX_RULES; ++i) {
        ctx->rules[i].type = 0;
        ctx->rules[i].code = 0;
        ctx->rules[i].condition = CEA89_COND_ALWAYS;
        ctx->rules[i].action = CEA89_ID_INVALID;
    }
    ctx->emit_count = 0UL;
    ctx->action_count = 0UL;
    ctx->last_result = CEA89_OK;
}

cea89_listener_id cea89_subscribe(cea89_context *ctx, cea89_value type,
                                  cea89_listener_fn fn, void *user)
{
    int i;
    cea89_id serial;
    if (!ctx || !fn) return CEA89_ID_INVALID;
    for (i = 0; i < CEA89_MAX_LISTENERS; ++i) {
        if (!ctx->listeners[i].fn) {
            serial = cea89_next_serial(ctx->listener_serials[i],
                                       (cea89_id)CEA89_MAX_LISTENERS);
            ctx->listener_serials[i] = serial;
            ctx->listeners[i].type = type;
            ctx->listeners[i].fn = fn;
            ctx->listeners[i].user = user;
            return cea89_make_id(i, serial, (cea89_id)CEA89_MAX_LISTENERS);
        }
    }
    ctx->last_result = CEA89_ERR_CAPACITY;
    return CEA89_ID_INVALID;
}

cea89_listener_id cea89_subscribe_unique(cea89_context *ctx, cea89_value type,
                                         cea89_listener_fn fn, void *user)
{
    int i;
    if (!ctx || !fn) return CEA89_ID_INVALID;
    for (i = 0; i < CEA89_MAX_LISTENERS; ++i) {
        if (ctx->listeners[i].fn == fn && ctx->listeners[i].user == user &&
            ctx->listeners[i].type == type)
            return cea89_make_id(i, ctx->listener_serials[i],
                                 (cea89_id)CEA89_MAX_LISTENERS);
    }
    return cea89_subscribe(ctx, type, fn, user);
}

int cea89_unsubscribe_id(cea89_context *ctx, cea89_listener_id id)
{
    int slot;
    cea89_id serial;
    if (!ctx || !cea89_decode_id(id, (cea89_id)CEA89_MAX_LISTENERS,
                                  &slot, &serial)) return 0;
    if (!ctx->listeners[slot].fn || ctx->listener_serials[slot] != serial) return 0;
    ctx->listeners[slot].type = 0;
    ctx->listeners[slot].fn = 0;
    ctx->listeners[slot].user = 0;
    return 1;
}

int cea89_unsubscribe(cea89_context *ctx, cea89_listener_fn fn, void *user)
{
    int i;
    int removed;
    if (!ctx || !fn) return 0;
    removed = 0;
    for (i = 0; i < CEA89_MAX_LISTENERS; ++i) {
        if (ctx->listeners[i].fn == fn && ctx->listeners[i].user == user) {
            ctx->listeners[i].type = 0;
            ctx->listeners[i].fn = 0;
            ctx->listeners[i].user = 0;
            ++removed;
        }
    }
    return removed;
}

cea89_condition_id cea89_condition_register(cea89_context *ctx,
                                             cea89_condition_fn fn,
                                             void *user)
{
    int i;
    cea89_id serial;
    if (!ctx || !fn) return CEA89_ID_INVALID;
    for (i = 0; i < CEA89_MAX_CONDITIONS; ++i) {
        if (!ctx->conditions[i].fn) {
            serial = cea89_next_serial(ctx->condition_serials[i],
                                       (cea89_id)CEA89_MAX_CONDITIONS);
            ctx->condition_serials[i] = serial;
            ctx->conditions[i].fn = fn;
            ctx->conditions[i].user = user;
            return cea89_make_id(i, serial, (cea89_id)CEA89_MAX_CONDITIONS);
        }
    }
    ctx->last_result = CEA89_ERR_CAPACITY;
    return CEA89_ID_INVALID;
}

int cea89_condition_unregister(cea89_context *ctx, cea89_condition_id id)
{
    int slot;
    cea89_id serial;
    if (!ctx || !cea89_decode_id(id, (cea89_id)CEA89_MAX_CONDITIONS,
                                  &slot, &serial)) return 0;
    if (!ctx->conditions[slot].fn || ctx->condition_serials[slot] != serial) return 0;
    ctx->conditions[slot].fn = 0;
    ctx->conditions[slot].user = 0;
    return 1;
}

int cea89_condition_eval(cea89_context *ctx, cea89_condition_id id,
                         const cea89_event *event)
{
    int slot;
    cea89_id serial;
    cea89_condition_fn fn;
    void *user;
    if (!ctx || !event || id == CEA89_COND_ALWAYS) return id == CEA89_COND_ALWAYS;
    if (!cea89_decode_id(id, (cea89_id)CEA89_MAX_CONDITIONS,
                         &slot, &serial)) return 0;
    if (!ctx->conditions[slot].fn || ctx->condition_serials[slot] != serial) return 0;
    fn = ctx->conditions[slot].fn;
    user = ctx->conditions[slot].user;
    return fn(event, user) ? 1 : 0;
}

cea89_action_id cea89_action_register(cea89_context *ctx,
                                      cea89_action_fn fn, void *user)
{
    int i;
    cea89_id serial;
    if (!ctx || !fn) return CEA89_ID_INVALID;
    for (i = 0; i < CEA89_MAX_ACTIONS; ++i) {
        if (!ctx->actions[i].fn) {
            serial = cea89_next_serial(ctx->action_serials[i],
                                       (cea89_id)CEA89_MAX_ACTIONS);
            ctx->action_serials[i] = serial;
            ctx->actions[i].fn = fn;
            ctx->actions[i].user = user;
            return cea89_make_id(i, serial, (cea89_id)CEA89_MAX_ACTIONS);
        }
    }
    ctx->last_result = CEA89_ERR_CAPACITY;
    return CEA89_ID_INVALID;
}

int cea89_action_unregister(cea89_context *ctx, cea89_action_id id)
{
    int slot;
    cea89_id serial;
    if (!ctx || !cea89_decode_id(id, (cea89_id)CEA89_MAX_ACTIONS,
                                  &slot, &serial)) return 0;
    if (!ctx->actions[slot].fn || ctx->action_serials[slot] != serial) return 0;
    ctx->actions[slot].fn = 0;
    ctx->actions[slot].user = 0;
    return 1;
}

int cea89_action_exec(cea89_context *ctx, cea89_action_id id,
                      const cea89_event *event)
{
    int slot;
    cea89_id serial;
    cea89_action_fn fn;
    void *user;
    if (!ctx || !event || !cea89_decode_id(id, (cea89_id)CEA89_MAX_ACTIONS,
                                            &slot, &serial)) return 0;
    if (!ctx->actions[slot].fn || ctx->action_serials[slot] != serial) return 0;
    fn = ctx->actions[slot].fn;
    user = ctx->actions[slot].user;
    fn(event, user);
    ++ctx->action_count;
    return 1;
}

static int cea89_rule_matches(const cea89_rule_entry *rule,
                              const cea89_event *event)
{
    if (!rule || !event || rule->action == CEA89_ID_INVALID) return 0;
    if (rule->type != CEA89_MATCH_ANY && rule->type != event->type) return 0;
    if (rule->code != CEA89_MATCH_ANY && rule->code != event->code) return 0;
    return 1;
}

cea89_rule_id cea89_rule_register(cea89_context *ctx,
                                  cea89_value type, cea89_value code,
                                  cea89_condition_id condition,
                                  cea89_action_id action)
{
    int i;
    cea89_id serial;
    if (!ctx || action == CEA89_ID_INVALID) return CEA89_ID_INVALID;
    for (i = 0; i < CEA89_MAX_RULES; ++i) {
        if (ctx->rules[i].action == CEA89_ID_INVALID) {
            serial = cea89_next_serial(ctx->rule_serials[i],
                                       (cea89_id)CEA89_MAX_RULES);
            ctx->rule_serials[i] = serial;
            ctx->rules[i].type = type;
            ctx->rules[i].code = code;
            ctx->rules[i].condition = condition;
            ctx->rules[i].action = action;
            return cea89_make_id(i, serial, (cea89_id)CEA89_MAX_RULES);
        }
    }
    ctx->last_result = CEA89_ERR_CAPACITY;
    return CEA89_ID_INVALID;
}

cea89_rule_id cea89_rule_register_unique(cea89_context *ctx,
                                         cea89_value type, cea89_value code,
                                         cea89_condition_id condition,
                                         cea89_action_id action)
{
    int i;
    if (!ctx || action == CEA89_ID_INVALID) return CEA89_ID_INVALID;
    for (i = 0; i < CEA89_MAX_RULES; ++i) {
        if (ctx->rules[i].action != CEA89_ID_INVALID &&
            ctx->rules[i].type == type && ctx->rules[i].code == code &&
            ctx->rules[i].condition == condition && ctx->rules[i].action == action)
            return cea89_make_id(i, ctx->rule_serials[i],
                                 (cea89_id)CEA89_MAX_RULES);
    }
    return cea89_rule_register(ctx, type, code, condition, action);
}

int cea89_rule_unregister(cea89_context *ctx, cea89_rule_id id)
{
    int slot;
    cea89_id serial;
    if (!ctx || !cea89_decode_id(id, (cea89_id)CEA89_MAX_RULES,
                                  &slot, &serial)) return 0;
    if (ctx->rules[slot].action == CEA89_ID_INVALID ||
        ctx->rule_serials[slot] != serial) return 0;
    ctx->rules[slot].type = 0;
    ctx->rules[slot].code = 0;
    ctx->rules[slot].condition = CEA89_COND_ALWAYS;
    ctx->rules[slot].action = CEA89_ID_INVALID;
    return 1;
}

int cea89_process(cea89_context *ctx, const cea89_event *event)
{
    int i;
    int count;
    int slots[CEA89_MAX_RULES];
    cea89_id serials[CEA89_MAX_RULES];
    int slot;
    int executed;
    if (!ctx || !event) return 0;
    count = 0;
    executed = 0;
    for (i = 0; i < CEA89_MAX_RULES; ++i) {
        if (cea89_rule_matches(&ctx->rules[i], event)) {
            slots[count] = i;
            serials[count] = ctx->rule_serials[i];
            ++count;
        }
    }
    for (i = 0; i < count; ++i) {
        slot = slots[i];
        if (ctx->rules[slot].action == CEA89_ID_INVALID ||
            ctx->rule_serials[slot] != serials[i] ||
            !cea89_rule_matches(&ctx->rules[slot], event)) continue;
        if (ctx->rules[slot].condition != CEA89_COND_ALWAYS &&
            !cea89_condition_eval(ctx, ctx->rules[slot].condition, event)) continue;
        executed += cea89_action_exec(ctx, ctx->rules[slot].action, event);
    }
    return executed;
}

int cea89_emit(cea89_context *ctx, const cea89_event *event)
{
    int i;
    int lcount;
    int rcount;
    int lslots[CEA89_MAX_LISTENERS];
    cea89_id lserials[CEA89_MAX_LISTENERS];
    int rslots[CEA89_MAX_RULES];
    cea89_id rserials[CEA89_MAX_RULES];
    int slot;
    int executed;
    cea89_event local;
    cea89_listener_fn lfn;
    void *luser;
    if (!ctx || !event) return 0;
    local = *event;
    lcount = 0;
    rcount = 0;
    for (i = 0; i < CEA89_MAX_LISTENERS; ++i) {
        if (ctx->listeners[i].fn &&
            (ctx->listeners[i].type == CEA89_MATCH_ANY ||
             ctx->listeners[i].type == local.type)) {
            lslots[lcount] = i;
            lserials[lcount] = ctx->listener_serials[i];
            ++lcount;
        }
    }
    for (i = 0; i < CEA89_MAX_RULES; ++i) {
        if (cea89_rule_matches(&ctx->rules[i], &local)) {
            rslots[rcount] = i;
            rserials[rcount] = ctx->rule_serials[i];
            ++rcount;
        }
    }
    ++ctx->emit_count;
    for (i = 0; i < lcount; ++i) {
        slot = lslots[i];
        if (!ctx->listeners[slot].fn ||
            ctx->listener_serials[slot] != lserials[i]) continue;
        if (ctx->listeners[slot].type != CEA89_MATCH_ANY &&
            ctx->listeners[slot].type != local.type) continue;
        lfn = ctx->listeners[slot].fn;
        luser = ctx->listeners[slot].user;
        lfn(&local, luser);
    }
    executed = 0;
    for (i = 0; i < rcount; ++i) {
        slot = rslots[i];
        if (ctx->rules[slot].action == CEA89_ID_INVALID ||
            ctx->rule_serials[slot] != rserials[i] ||
            !cea89_rule_matches(&ctx->rules[slot], &local)) continue;
        if (ctx->rules[slot].condition != CEA89_COND_ALWAYS &&
            !cea89_condition_eval(ctx, ctx->rules[slot].condition, &local)) continue;
        executed += cea89_action_exec(ctx, ctx->rules[slot].action, &local);
    }
    return executed;
}

int cea89_emit_values(cea89_context *ctx,
                      cea89_value type, cea89_value code,
                      unsigned long owner, unsigned long instance,
                      void *subject, void *data)
{
    cea89_event event;
    if (!ctx) return 0;
    event.type = type;
    event.code = code;
    event.owner = owner;
    event.instance = instance;
    event.subject = subject;
    event.data = data;
    return cea89_emit(ctx, &event);
}

int cea89_listener_count(const cea89_context *ctx)
{
    int i, n;
    if (!ctx) return 0;
    n = 0;
    for (i = 0; i < CEA89_MAX_LISTENERS; ++i) if (ctx->listeners[i].fn) ++n;
    return n;
}
int cea89_condition_count(const cea89_context *ctx)
{
    int i, n;
    if (!ctx) return 0;
    n = 0;
    for (i = 0; i < CEA89_MAX_CONDITIONS; ++i) if (ctx->conditions[i].fn) ++n;
    return n;
}
int cea89_action_count_registered(const cea89_context *ctx)
{
    int i, n;
    if (!ctx) return 0;
    n = 0;
    for (i = 0; i < CEA89_MAX_ACTIONS; ++i) if (ctx->actions[i].fn) ++n;
    return n;
}
int cea89_rule_count(const cea89_context *ctx)
{
    int i, n;
    if (!ctx) return 0;
    n = 0;
    for (i = 0; i < CEA89_MAX_RULES; ++i) if (ctx->rules[i].action != CEA89_ID_INVALID) ++n;
    return n;
}
