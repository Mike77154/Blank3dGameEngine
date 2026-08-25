#ifndef CONTACT_TRIGGER89_H
#define CONTACT_TRIGGER89_H

#include <limits.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CT89_MAX_TRIGGERS
#define CT89_MAX_TRIGGERS 128
#endif
#ifndef CT89_MAX_PAIRS
#define CT89_MAX_PAIRS 512
#endif
#ifndef CT89_MAX_GATHER
#define CT89_MAX_GATHER 64
#endif

#if CT89_MAX_TRIGGERS <= 0
#error "CT89_MAX_TRIGGERS must be positive"
#endif
#if CT89_MAX_PAIRS <= 0
#error "CT89_MAX_PAIRS must be positive"
#endif
#if CT89_MAX_GATHER <= 0
#error "CT89_MAX_GATHER must be positive"
#endif

/* Q16.16 fixed point. */
typedef signed long CT89_FX;
#define CT89_FX_ONE ((CT89_FX)65536L)

typedef unsigned long CT89_Subject;
#define CT89_SUBJECT_INVALID 0UL

typedef int CT89_Trigger;
#define CT89_TRIGGER_INVALID (-1)

#define CT89_TRIGGER_GENERATION_LIMIT \
    ((unsigned int)(((unsigned long)INT_MAX / \
    (unsigned long)CT89_MAX_TRIGGERS) + 1UL))

enum {
    CT89_OK = 0,
    CT89_ERR_ARGUMENT = -1,
    CT89_ERR_FULL = -2,
    CT89_ERR_BAD_TRIGGER = -3,
    CT89_ERR_STALE_TRIGGER = -4,
    CT89_ERR_PROVIDER = -5,
    CT89_ERR_PAIR_FULL = -6,
    CT89_ERR_NOT_ACTIVE = -7,
    CT89_ERR_DISABLED = -8
};

enum {
    CT89_SENSOR_TOUCH = 1,
    CT89_SENSOR_PROXIMITY = 2,
    CT89_SENSOR_TOUCH_OR_PROXIMITY = 3,
    CT89_SENSOR_MANUAL = 4
};

enum {
    CT89_RELATION_NONE = 0,
    CT89_RELATION_TOUCH = 1,
    CT89_RELATION_NEAR = 2
};

enum {
    CT89_EVENT_NONE = 0,
    CT89_EVENT_ENTER = 1,
    CT89_EVENT_STAY = 2,
    CT89_EVENT_EXIT = 3,
    CT89_EVENT_ACTIVATE = 4
};

#define CT89_EVENT_BIT(event_id) (1UL << (unsigned long)(event_id))
#define CT89_EVENT_MASK_ENTER CT89_EVENT_BIT(CT89_EVENT_ENTER)
#define CT89_EVENT_MASK_STAY CT89_EVENT_BIT(CT89_EVENT_STAY)
#define CT89_EVENT_MASK_EXIT CT89_EVENT_BIT(CT89_EVENT_EXIT)
#define CT89_EVENT_MASK_ACTIVATE CT89_EVENT_BIT(CT89_EVENT_ACTIVATE)
#define CT89_EVENT_MASK_ALL \
    (CT89_EVENT_MASK_ENTER | CT89_EVENT_MASK_STAY | \
     CT89_EVENT_MASK_EXIT | CT89_EVENT_MASK_ACTIVATE)

enum {
    CT89_CONSUME_KEEP = 0,
    CT89_CONSUME_DISABLE_TRIGGER = 1,
    CT89_CONSUME_DESTROY_OWNER = 2,
    CT89_CONSUME_DESTROY_OTHER = 3,
    CT89_CONSUME_DESTROY_BOTH = 4
};

enum {
    CT89_ACTION_UNHANDLED = 0,
    CT89_ACTION_ACCEPTED = 1,
    CT89_ACTION_REJECTED = 2,
    CT89_ACTION_DEFERRED = 3
};

typedef struct CT89_CandidateTag {
    CT89_Subject subject;
    unsigned long category_mask;
    CT89_FX distance_fx;
} CT89_Candidate;

typedef struct CT89_ProbeTag {
    CT89_Trigger trigger;
    CT89_Subject owner;
    int sensor_mode;
    CT89_FX radius_fx;
    unsigned long category_mask;
    unsigned long user_tag;
} CT89_Probe;

typedef struct CT89_TriggerDescTag {
    CT89_Subject owner;
    int sensor_mode;
    CT89_FX radius_fx;
    unsigned long category_mask;
    unsigned long notify_event_mask;
    unsigned long action_event_mask;
    int consume_policy;
    int action_id;
    long action_arg0;
    long action_arg1;
    unsigned long user_tag;
    unsigned long cooldown_ms;
    unsigned int max_activations;
} CT89_TriggerDesc;

typedef struct CT89_EventTag {
    CT89_Trigger trigger;
    CT89_Subject owner;
    CT89_Subject other;
    int event_type;
    unsigned int relation_flags;
    int action_id;
    long action_arg0;
    long action_arg1;
    unsigned long user_tag;
    int action_status;
} CT89_Event;

typedef int (*CT89_GatherFn)(void *user,
                              const CT89_Probe *probe,
                              CT89_Candidate *out_candidates,
                              int max_candidates);

typedef int (*CT89_FilterFn)(void *user,
                              const CT89_Probe *probe,
                              const CT89_Candidate *candidate);

typedef int (*CT89_ActionFn)(void *user,
                              const CT89_Event *event);

typedef void (*CT89_EventFn)(void *user,
                              const CT89_Event *event);

typedef int (*CT89_DestroyFn)(void *user,
                               CT89_Subject subject);

typedef struct CT89_SensorProviderTag {
    void *user;
    CT89_GatherFn gather;
} CT89_SensorProvider;

typedef struct CT89_FilterProviderTag {
    void *user;
    CT89_FilterFn accept;
} CT89_FilterProvider;

typedef struct CT89_ActionProviderTag {
    void *user;
    CT89_ActionFn execute;
} CT89_ActionProvider;

typedef struct CT89_EventProviderTag {
    void *user;
    CT89_EventFn emit;
} CT89_EventProvider;

typedef struct CT89_LifecycleProviderTag {
    void *user;
    CT89_DestroyFn destroy_subject;
} CT89_LifecycleProvider;

typedef struct CT89_TriggerSlotTag {
    unsigned char used;
    unsigned char enabled;
    unsigned int generation;
    CT89_TriggerDesc desc;
    unsigned long cooldown_left_ms;
    unsigned int activation_count;
} CT89_TriggerSlot;

typedef struct CT89_PairSlotTag {
    unsigned char used;
    CT89_Trigger trigger;
    CT89_Subject other;
    unsigned int previous_relations;
    unsigned int current_relations;
    unsigned long category_mask;
    CT89_FX distance_fx;
} CT89_PairSlot;

typedef struct CT89_StatsTag {
    unsigned long steps;
    unsigned long candidates;
    unsigned long enters;
    unsigned long stays;
    unsigned long exits;
    unsigned long activations;
    unsigned long actions_accepted;
    unsigned long actions_rejected;
    unsigned long actions_deferred;
    unsigned long actions_unhandled;
    unsigned long lifecycle_requests;
    unsigned long lifecycle_failures;
    unsigned long pair_overflows;
} CT89_Stats;

typedef struct CT89_ContextTag {
    int initialized;
    int last_error;
    CT89_TriggerSlot triggers[CT89_MAX_TRIGGERS];
    CT89_PairSlot pairs[CT89_MAX_PAIRS];
    CT89_SensorProvider contact_provider;
    CT89_SensorProvider proximity_provider;
    CT89_FilterProvider filter_provider;
    CT89_ActionProvider action_provider;
    CT89_EventProvider event_provider;
    CT89_LifecycleProvider lifecycle_provider;
    CT89_Stats stats;
} CT89_Context;

void ct89_init(CT89_Context *ctx);
void ct89_reset(CT89_Context *ctx);
int ct89_last_error(const CT89_Context *ctx);
const char *ct89_error_string(int error_code);

void ct89_sensor_provider_init(CT89_SensorProvider *provider);
void ct89_filter_provider_init(CT89_FilterProvider *provider);
void ct89_action_provider_init(CT89_ActionProvider *provider);
void ct89_event_provider_init(CT89_EventProvider *provider);
void ct89_lifecycle_provider_init(CT89_LifecycleProvider *provider);

void ct89_set_contact_provider(CT89_Context *ctx,
                               const CT89_SensorProvider *provider);
void ct89_set_proximity_provider(CT89_Context *ctx,
                                 const CT89_SensorProvider *provider);
void ct89_set_filter_provider(CT89_Context *ctx,
                              const CT89_FilterProvider *provider);
void ct89_set_action_provider(CT89_Context *ctx,
                              const CT89_ActionProvider *provider);
void ct89_set_event_provider(CT89_Context *ctx,
                             const CT89_EventProvider *provider);
void ct89_set_lifecycle_provider(CT89_Context *ctx,
                                 const CT89_LifecycleProvider *provider);

void ct89_trigger_desc_defaults(CT89_TriggerDesc *desc);
CT89_Trigger ct89_trigger_create(CT89_Context *ctx,
                                 const CT89_TriggerDesc *desc);
int ct89_trigger_destroy(CT89_Context *ctx, CT89_Trigger trigger);
int ct89_trigger_set_enabled(CT89_Context *ctx,
                             CT89_Trigger trigger, int enabled);
int ct89_trigger_is_enabled(const CT89_Context *ctx,
                            CT89_Trigger trigger);
const CT89_TriggerDesc *ct89_trigger_desc(const CT89_Context *ctx,
                                          CT89_Trigger trigger);
unsigned int ct89_trigger_activation_count(const CT89_Context *ctx,
                                           CT89_Trigger trigger);

int ct89_step(CT89_Context *ctx, unsigned long dt_ms);
int ct89_activate(CT89_Context *ctx,
                  CT89_Trigger trigger,
                  CT89_Subject activator);
int ct89_subject_is_active(const CT89_Context *ctx,
                           CT89_Trigger trigger,
                           CT89_Subject subject);
unsigned int ct89_subject_relations(const CT89_Context *ctx,
                                    CT89_Trigger trigger,
                                    CT89_Subject subject);
const CT89_Stats *ct89_stats(const CT89_Context *ctx);

CT89_FX ct89_fx_from_int(int value);
int ct89_fx_to_int(CT89_FX value);

#ifdef __cplusplus
}
#endif

#endif
