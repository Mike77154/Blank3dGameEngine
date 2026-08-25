#ifndef CONDOR_EVACT89_H
#define CONDOR_EVACT89_H

/*
 * condor_evact89 v0.2 - context-based event/condition/action rule core.
 * ISO C89, fixed capacity, no heap, no float/double.
 *
 * Derived from the uploaded condor_evact_lib design, but removes global
 * singleton storage so a host can own multiple independent runtimes.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CEA89_MAX_LISTENERS
#define CEA89_MAX_LISTENERS 128
#endif
#ifndef CEA89_MAX_CONDITIONS
#define CEA89_MAX_CONDITIONS 128
#endif
#ifndef CEA89_MAX_ACTIONS
#define CEA89_MAX_ACTIONS 128
#endif
#ifndef CEA89_MAX_RULES
#define CEA89_MAX_RULES 256
#endif

#ifndef CEA89_VALUE_T
#define CEA89_VALUE_T int
#endif
#ifndef CEA89_ID_T
#define CEA89_ID_T unsigned long
#endif

typedef CEA89_VALUE_T cea89_value;
typedef CEA89_ID_T cea89_id;
typedef cea89_id cea89_listener_id;
typedef cea89_id cea89_condition_id;
typedef cea89_id cea89_action_id;
typedef cea89_id cea89_rule_id;

#define CEA89_ID_INVALID ((cea89_id)0)
#define CEA89_MATCH_ANY  ((cea89_value)-1)
#define CEA89_COND_ALWAYS ((cea89_condition_id)0)

#define CEA89_OK            0
#define CEA89_ERR_NULL     -1
#define CEA89_ERR_CAPACITY -2
#define CEA89_ERR_HANDLE   -3
#define CEA89_ERR_ARGUMENT -4

typedef struct cea89_event {
    cea89_value type;
    cea89_value code;
    unsigned long owner;     /* actor / gameverb subject identity */
    unsigned long instance;  /* Thing/VarRuntime owner identity */
    void *subject;           /* opaque native entity/object */
    void *data;              /* synchronous opaque payload */
} cea89_event;

typedef void (*cea89_listener_fn)(const cea89_event *event, void *user);
typedef int  (*cea89_condition_fn)(const cea89_event *event, void *user);
typedef void (*cea89_action_fn)(const cea89_event *event, void *user);

typedef struct cea89_listener_entry {
    cea89_value type;
    cea89_listener_fn fn;
    void *user;
} cea89_listener_entry;

typedef struct cea89_condition_entry {
    cea89_condition_fn fn;
    void *user;
} cea89_condition_entry;

typedef struct cea89_action_entry {
    cea89_action_fn fn;
    void *user;
} cea89_action_entry;

typedef struct cea89_rule_entry {
    cea89_value type;
    cea89_value code;
    cea89_condition_id condition;
    cea89_action_id action;
} cea89_rule_entry;

typedef struct cea89_context {
    cea89_listener_entry listeners[CEA89_MAX_LISTENERS];
    cea89_id listener_serials[CEA89_MAX_LISTENERS];
    cea89_condition_entry conditions[CEA89_MAX_CONDITIONS];
    cea89_id condition_serials[CEA89_MAX_CONDITIONS];
    cea89_action_entry actions[CEA89_MAX_ACTIONS];
    cea89_id action_serials[CEA89_MAX_ACTIONS];
    cea89_rule_entry rules[CEA89_MAX_RULES];
    cea89_id rule_serials[CEA89_MAX_RULES];
    unsigned long emit_count;
    unsigned long action_count;
    int last_result;
} cea89_context;

void cea89_init(cea89_context *ctx);
/* Clears active listeners/conditions/actions/rules but intentionally preserves
 * serial counters so stale handles stay stale after reset. */
void cea89_reset(cea89_context *ctx);

cea89_listener_id cea89_subscribe(cea89_context *ctx, cea89_value type,
                                  cea89_listener_fn fn, void *user);
cea89_listener_id cea89_subscribe_unique(cea89_context *ctx, cea89_value type,
                                         cea89_listener_fn fn, void *user);
int cea89_unsubscribe_id(cea89_context *ctx, cea89_listener_id id);
int cea89_unsubscribe(cea89_context *ctx, cea89_listener_fn fn, void *user);

cea89_condition_id cea89_condition_register(cea89_context *ctx,
                                             cea89_condition_fn fn,
                                             void *user);
int cea89_condition_unregister(cea89_context *ctx, cea89_condition_id id);
int cea89_condition_eval(cea89_context *ctx, cea89_condition_id id,
                         const cea89_event *event);

cea89_action_id cea89_action_register(cea89_context *ctx,
                                      cea89_action_fn fn, void *user);
int cea89_action_unregister(cea89_context *ctx, cea89_action_id id);
int cea89_action_exec(cea89_context *ctx, cea89_action_id id,
                      const cea89_event *event);

cea89_rule_id cea89_rule_register(cea89_context *ctx,
                                  cea89_value type, cea89_value code,
                                  cea89_condition_id condition,
                                  cea89_action_id action);
cea89_rule_id cea89_rule_register_unique(cea89_context *ctx,
                                         cea89_value type, cea89_value code,
                                         cea89_condition_id condition,
                                         cea89_action_id action);
int cea89_rule_unregister(cea89_context *ctx, cea89_rule_id id);
int cea89_process(cea89_context *ctx, const cea89_event *event);

/* Emits listeners and rules using snapshots taken before dispatch. New
 * listeners/rules registered during the event do not receive that event;
 * entries removed before their turn are skipped. Returns executed actions. */
int cea89_emit(cea89_context *ctx, const cea89_event *event);
int cea89_emit_values(cea89_context *ctx,
                      cea89_value type, cea89_value code,
                      unsigned long owner, unsigned long instance,
                      void *subject, void *data);

int cea89_listener_count(const cea89_context *ctx);
int cea89_condition_count(const cea89_context *ctx);
int cea89_action_count_registered(const cea89_context *ctx);
int cea89_rule_count(const cea89_context *ctx);

#ifdef __cplusplus
}
#endif

#endif
