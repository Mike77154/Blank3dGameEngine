#include "activation_fpi.h"

static int activation_condition_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_UNUSED(run_user);
    return fpi_eval_activation_condition((FPI_Activation*)bind_user, symbol_id, value);
}

static void activation_action_cb(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value) {
    FPI_UNUSED(run_user);
    (void)fpi_exec_activation_action((FPI_Activation*)bind_user, symbol_id, value);
}

int fpi_activation_init(FPI_Activation* activation, FPI_Context* context,
    FPI_Fixed* activation_ref) {
    return fpi_activation_init2(activation, context, activation_ref, 0, 0, 0);
}

int fpi_activation_init2(FPI_Activation* activation, FPI_Context* context,
    FPI_Fixed* activation_ref, struct FPI_VarSystem* vars,
    FPI_InternalVarFn internal_fn, void* internal_user) {
    int rc;
    if (!activation || !context || !activation_ref) return FPI_ERR_ARGUMENT;
    activation->context = context;
    activation->activation_ref = activation_ref;
    fpi_value_resolver_init(&activation->resolver, context, vars, internal_fn, internal_user);
    rc = fpi_bind_cond(context, "activated", activation_condition_cb, activation);
    if (rc < 0) return rc;
    activation->ID_COND_ACTIVATED = rc;
    rc = fpi_bind_act(context, "activate", activation_action_cb, activation);
    if (rc < 0) return rc;
    activation->ID_ACT_ACTIVATE = rc;
    return FPI_OK;
}

int fpi_eval_activation_condition(FPI_Activation* activation, int condition_id,
    const FPI_Value* value) {
    FPI_Fixed rhs;
    if (!activation || condition_id != activation->ID_COND_ACTIVATED) return -1;
    if (!activation->activation_ref) return 0;
    if (!fpi_resolve_fixed_value(&activation->resolver, value, &rhs)) return 0;
    return *activation->activation_ref == rhs;
}

int fpi_exec_activation_action(FPI_Activation* activation, int action_id,
    const FPI_Value* value) {
    FPI_Fixed rhs;
    if (!activation || action_id != activation->ID_ACT_ACTIVATE) return 0;
    if (!fpi_resolve_fixed_value(&activation->resolver, value, &rhs)) return 0;
    if (activation->activation_ref) *activation->activation_ref = rhs;
    return 1;
}
