#ifndef TIME_BINDING_FPI_H
#define TIME_BINDING_FPI_H

#include "value_resolver_fpi.h"

typedef FPI_U32 (*FPI_NowMsFn)(void* user);

typedef struct FPI_TimeBinding {
    FPI_Context* context;
    FPI_U32* timer_start;
    FPI_U32* etimer_start;
    FPI_NowMsFn now_ms;
    void* now_user;
    FPI_ValueResolver resolver;
    int ID_TIMERGREATER;
    int ID_TIMERSTART;
    int ID_ETIMERGREATER;
    int ID_ETIMERSTART;
} FPI_TimeBinding;

int fpi_time_binding_init(FPI_TimeBinding* timer, FPI_Context* context,
    FPI_U32* timer_start_ref, FPI_NowMsFn now_ms_fn, void* now_user);
int fpi_time_binding_init2(FPI_TimeBinding* timer, FPI_Context* context,
    FPI_U32* timer_start_ref, FPI_U32* etimer_start_ref,
    FPI_NowMsFn now_ms_fn, void* now_user);
int fpi_time_binding_init3(FPI_TimeBinding* timer, FPI_Context* context,
    FPI_U32* timer_start_ref, FPI_U32* etimer_start_ref,
    FPI_NowMsFn now_ms_fn, void* now_user,
    struct FPI_VarSystem* vars, FPI_InternalVarFn internal_fn, void* internal_user);
int fpi_eval_time_condition(FPI_TimeBinding* timer, int condition_id, const FPI_Value* value);
int fpi_exec_time_action(FPI_TimeBinding* timer, int action_id);

#endif /* TIME_BINDING_FPI_H */
