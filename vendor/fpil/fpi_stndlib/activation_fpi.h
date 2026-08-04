#ifndef ACTIVATION_FPI_H
#define ACTIVATION_FPI_H

#include "value_resolver_fpi.h"

typedef struct FPI_Activation {
    FPI_Context* context;
    FPI_Fixed* activation_ref;
    int ID_COND_ACTIVATED;
    int ID_ACT_ACTIVATE;
    FPI_ValueResolver resolver;
} FPI_Activation;

int fpi_activation_init(FPI_Activation* activation, FPI_Context* context,
    FPI_Fixed* activation_ref);
int fpi_activation_init2(FPI_Activation* activation, FPI_Context* context,
    FPI_Fixed* activation_ref, struct FPI_VarSystem* vars,
    FPI_InternalVarFn internal_fn, void* internal_user);
int fpi_eval_activation_condition(FPI_Activation* activation, int condition_id,
    const FPI_Value* value);
int fpi_exec_activation_action(FPI_Activation* activation, int action_id,
    const FPI_Value* value);

#endif /* ACTIVATION_FPI_H */
