#ifndef BLANK3D_CONDOR_H
#define BLANK3D_CONDOR_H

#include "condor_evact89.h"
#include "gameverbs89.h"
#include "blank3d_variables.h"
#include "blank3d_input.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef B3D_CONDOR_MAX_GAMEVERB_CONDITIONS
#define B3D_CONDOR_MAX_GAMEVERB_CONDITIONS 64
#endif
#ifndef B3D_CONDOR_MAX_GAMEVERB_ACTIONS
#define B3D_CONDOR_MAX_GAMEVERB_ACTIONS 64
#endif
#ifndef B3D_CONDOR_MAX_VAR_CONDITIONS
#define B3D_CONDOR_MAX_VAR_CONDITIONS 64
#endif
#ifndef B3D_CONDOR_MAX_VAR_ACTIONS
#define B3D_CONDOR_MAX_VAR_ACTIONS 64
#endif
#ifndef B3D_CONDOR_NAME_CAP
#define B3D_CONDOR_NAME_CAP 64
#endif

#define B3D_CEA_EVENT_INPUT_PRESS       1
#define B3D_CEA_EVENT_INPUT_HOLD        2
#define B3D_CEA_EVENT_INPUT_RELEASE     3
#define B3D_CEA_EVENT_THING_SPAWN      10
#define B3D_CEA_EVENT_THING_DESTROY    11
#define B3D_CEA_EVENT_ACTOR_DAMAGE     20
#define B3D_CEA_EVENT_ACTOR_DEATH      21
#define B3D_CEA_EVENT_GFO_BEGIN        30
#define B3D_CEA_EVENT_GFO_END          31
#define B3D_CEA_EVENT_CUSTOM         1000
#define B3D_CEA_MOUSE_CODE_BASE       256

#define B3D_CEA_CMP_EQ 0
#define B3D_CEA_CMP_NE 1
#define B3D_CEA_CMP_LT 2
#define B3D_CEA_CMP_LE 3
#define B3D_CEA_CMP_GT 4
#define B3D_CEA_CMP_GE 5

#define B3D_CEA_VAR_SET 1
#define B3D_CEA_VAR_ADD 2
#define B3D_CEA_VAR_SUB 3

typedef struct Blank3DCondorVerbConditionTag {
    int used;
    char name[B3D_CONDOR_NAME_CAP];
    cea89_condition_id id;
    void *owner_ctx;
} Blank3DCondorVerbCondition;

typedef struct Blank3DCondorVerbActionTag {
    int used;
    char name[B3D_CONDOR_NAME_CAP];
    cea89_action_id id;
    void *owner_ctx;
} Blank3DCondorVerbAction;

typedef struct Blank3DCondorVarConditionTag {
    int used;
    vr89_scope scope;
    unsigned long fixed_owner;
    char name[B3D_CONDOR_NAME_CAP];
    int compare;
    long value_q16;
    cea89_condition_id id;
    void *owner_ctx;
} Blank3DCondorVarCondition;

typedef struct Blank3DCondorVarActionTag {
    int used;
    vr89_scope scope;
    unsigned long fixed_owner;
    char name[B3D_CONDOR_NAME_CAP];
    int operation;
    vm89_value value;
    cea89_action_id id;
    void *owner_ctx;
} Blank3DCondorVarAction;

typedef struct Blank3DCondorTag {
    int initialized;
    cea89_context core;
    gverb89_registry *gameverbs;
    Blank3DVariables *variables;
    Blank3DCondorVerbCondition verb_conditions[B3D_CONDOR_MAX_GAMEVERB_CONDITIONS];
    Blank3DCondorVerbAction verb_actions[B3D_CONDOR_MAX_GAMEVERB_ACTIONS];
    Blank3DCondorVarCondition var_conditions[B3D_CONDOR_MAX_VAR_CONDITIONS];
    Blank3DCondorVarAction var_actions[B3D_CONDOR_MAX_VAR_ACTIONS];
    unsigned long input_events;
    unsigned long thing_events;
    unsigned long actor_events;
    char status[192];
} Blank3DCondor;

void blank3d_condor_init(Blank3DCondor *condor,
                         gverb89_registry *gameverbs,
                         Blank3DVariables *variables);
void blank3d_condor_reset_rules(Blank3DCondor *condor);

cea89_condition_id blank3d_condor_condition_gameverb(Blank3DCondor *condor,
                                                       const char *name);
cea89_action_id blank3d_condor_action_gameverb(Blank3DCondor *condor,
                                                const char *name);
cea89_condition_id blank3d_condor_condition_var_q16(Blank3DCondor *condor,
                                                     vr89_scope scope,
                                                     unsigned long fixed_owner,
                                                     const char *name,
                                                     int compare,
                                                     long value_q16);
cea89_action_id blank3d_condor_action_var_q16(Blank3DCondor *condor,
                                               vr89_scope scope,
                                               unsigned long fixed_owner,
                                               const char *name,
                                               int operation,
                                               long value_q16);
cea89_action_id blank3d_condor_action_var_bool(Blank3DCondor *condor,
                                                vr89_scope scope,
                                                unsigned long fixed_owner,
                                                const char *name,
                                                int value);
cea89_rule_id blank3d_condor_rule(Blank3DCondor *condor,
                                  int event_type, int event_code,
                                  cea89_condition_id condition,
                                  cea89_action_id action);
cea89_rule_id blank3d_condor_rule_gameverbs(Blank3DCondor *condor,
                                            int event_type, int event_code,
                                            const char *condition_name,
                                            const char *action_name);

int blank3d_condor_emit(Blank3DCondor *condor,
                        int event_type, int event_code,
                        unsigned long owner, unsigned long instance,
                        void *subject, void *data);
void blank3d_condor_emit_input(Blank3DCondor *condor,
                               const Blank3DInput *input,
                               unsigned long owner,
                               unsigned long instance,
                               void *subject);
void blank3d_condor_emit_thing(Blank3DCondor *condor,
                               int event_type, unsigned long thing_owner,
                               int actor_id, unsigned long entity_id,
                               void *subject);
void blank3d_condor_emit_actor(Blank3DCondor *condor,
                               int event_type, int actor_id,
                               unsigned long thing_owner,
                               int source_actor, void *subject, void *data);
int blank3d_condor_event_type_from_name(const char *name, int *out_type);
const char *blank3d_condor_event_type_name(int event_type);
const char *blank3d_condor_status(const Blank3DCondor *condor);

#ifdef __cplusplus
}
#endif
#endif
