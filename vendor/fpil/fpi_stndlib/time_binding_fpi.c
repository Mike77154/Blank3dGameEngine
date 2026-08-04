#include "time_binding_fpi.h"

static int time_condition_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_UNUSED(run_user);
    return fpi_eval_time_condition((FPI_TimeBinding*)bind_user, symbol_id, value);
}

static void time_action_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_UNUSED(run_user);
    FPI_UNUSED(value);
    (void)fpi_exec_time_action((FPI_TimeBinding*)bind_user, symbol_id);
}

int fpi_time_binding_init(FPI_TimeBinding* timer, FPI_Context* context,
    FPI_U32* timer_start_ref, FPI_NowMsFn now_ms_fn, void* now_user) {
    return fpi_time_binding_init3(timer, context, timer_start_ref, 0,
        now_ms_fn, now_user, 0, 0, 0);
}

int fpi_time_binding_init2(FPI_TimeBinding* timer, FPI_Context* context,
    FPI_U32* timer_start_ref, FPI_U32* etimer_start_ref,
    FPI_NowMsFn now_ms_fn, void* now_user) {
    return fpi_time_binding_init3(timer, context, timer_start_ref, etimer_start_ref,
        now_ms_fn, now_user, 0, 0, 0);
}

int fpi_time_binding_init3(FPI_TimeBinding* timer, FPI_Context* context,
    FPI_U32* timer_start_ref, FPI_U32* etimer_start_ref,
    FPI_NowMsFn now_ms_fn, void* now_user,
    struct FPI_VarSystem* vars, FPI_InternalVarFn internal_fn, void* internal_user) {
    int rc;
    if (!timer || !context || !now_ms_fn) return FPI_ERR_ARGUMENT;
    timer->context = context;
    timer->timer_start = timer_start_ref;
    timer->etimer_start = etimer_start_ref;
    timer->now_ms = now_ms_fn;
    timer->now_user = now_user;
    fpi_value_resolver_init(&timer->resolver, context, vars, internal_fn, internal_user);
    rc = fpi_bind_cond(context, "timergreater", time_condition_cb, timer);
    if (rc < 0) return rc;
    timer->ID_TIMERGREATER = rc;
    rc = fpi_bind_cond(context, "etimergreater", time_condition_cb, timer);
    if (rc < 0) return rc;
    timer->ID_ETIMERGREATER = rc;
    rc = fpi_bind_act(context, "timerstart", time_action_cb, timer);
    if (rc < 0) return rc;
    timer->ID_TIMERSTART = rc;
    rc = fpi_bind_act(context, "etimerstart", time_action_cb, timer);
    if (rc < 0) return rc;
    timer->ID_ETIMERSTART = rc;
    return FPI_OK;
}

int fpi_eval_time_condition(FPI_TimeBinding* timer, int condition_id, const FPI_Value* value) {
    FPI_Fixed fixed_threshold;
    long threshold;
    FPI_U32 now;
    FPI_U32 start;
    if (!timer || !timer->now_ms) return -1;
    if (condition_id != timer->ID_TIMERGREATER && condition_id != timer->ID_ETIMERGREATER)
        return -1;
    if (!fpi_resolve_fixed_value(&timer->resolver, value, &fixed_threshold)) return 0;
    threshold = fpi_fixed_to_int(fixed_threshold);
    if (threshold < 0L) return 0;
    now = timer->now_ms(timer->now_user);
    if (condition_id == timer->ID_TIMERGREATER) {
        if (!timer->timer_start) return 0;
        start = *timer->timer_start;
    } else {
        if (!timer->etimer_start) return 0;
        start = *timer->etimer_start;
    }
    return (FPI_U32)(now - start) >= (FPI_U32)threshold;
}

int fpi_exec_time_action(FPI_TimeBinding* timer, int action_id) {
    FPI_U32 now;
    if (!timer || !timer->now_ms) return 0;
    now = timer->now_ms(timer->now_user);
    if (action_id == timer->ID_TIMERSTART) {
        if (timer->timer_start) *timer->timer_start = now;
        return 1;
    }
    if (action_id == timer->ID_ETIMERSTART) {
        if (timer->etimer_start) *timer->etimer_start = now;
        return 1;
    }
    return 0;
}
