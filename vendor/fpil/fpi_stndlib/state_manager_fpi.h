#ifndef STATE_MANAGER_FPI_H
#define STATE_MANAGER_FPI_H

#include "fpi_builtins.h"
#include "value_resolver_fpi.h"

typedef struct FPI_StateManager {
    FPI_StateBuiltin builtin;
    FPI_StateWords words;
    FPI_ValueResolver resolver;
} FPI_StateManager;

int fpi_state_manager_init(FPI_StateManager* manager, FPI_Context* context,
    FPI_Fixed* state_ref, int* transitioned_ref);
int fpi_state_manager_init2(FPI_StateManager* manager, FPI_Context* context,
    FPI_Fixed* state_ref, int* transitioned_ref,
    struct FPI_VarSystem* vars, FPI_InternalVarFn internal_fn, void* internal_user);
int fpi_state_manager_init_named(FPI_StateManager* manager, FPI_Context* context,
    FPI_Fixed* state_ref, int* transitioned_ref, const char* base_name,
    struct FPI_VarSystem* vars, FPI_InternalVarFn internal_fn, void* internal_user);
void fpi_state_manager_begin_tick(FPI_StateManager* manager);

#endif /* STATE_MANAGER_FPI_H */
