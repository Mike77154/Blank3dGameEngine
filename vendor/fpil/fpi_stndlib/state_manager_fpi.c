#include "state_manager_fpi.h"

int fpi_state_manager_init(FPI_StateManager* manager, FPI_Context* context,
    FPI_Fixed* state_ref, int* transitioned_ref) {
    return fpi_state_manager_init2(manager, context, state_ref, transitioned_ref, 0, 0, 0);
}

int fpi_state_manager_init2(FPI_StateManager* manager, FPI_Context* context,
    FPI_Fixed* state_ref, int* transitioned_ref,
    struct FPI_VarSystem* vars, FPI_InternalVarFn internal_fn, void* internal_user) {
    if (!manager) return FPI_ERR_ARGUMENT;
    fpi_state_words_default(&manager->words);
    fpi_value_resolver_init(&manager->resolver, context, vars, internal_fn, internal_user);
    return fpi_state_builtin_init(&manager->builtin, context, state_ref,
        transitioned_ref, &manager->words);
}

int fpi_state_manager_init_named(FPI_StateManager* manager, FPI_Context* context,
    FPI_Fixed* state_ref, int* transitioned_ref, const char* base_name,
    struct FPI_VarSystem* vars, FPI_InternalVarFn internal_fn, void* internal_user) {
    int rc;
    if (!manager || !base_name) return FPI_ERR_ARGUMENT;
    rc = fpi_state_words_from_base(&manager->words, base_name);
    if (rc != FPI_OK) return rc;
    fpi_value_resolver_init(&manager->resolver, context, vars, internal_fn, internal_user);
    return fpi_state_builtin_init(&manager->builtin, context, state_ref,
        transitioned_ref, &manager->words);
}

void fpi_state_manager_begin_tick(FPI_StateManager* manager) {
    if (manager) fpi_state_builtin_begin_tick(&manager->builtin);
}
