#ifndef FPI_REGISTRY_H
#define FPI_REGISTRY_H

#include "fpi_value.h"

typedef int (*FPI_BoundCondFn)(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value);
typedef void (*FPI_BoundActFn)(void* bind_user, void* run_user, int symbol_id, const FPI_Value* value);

typedef struct FPI_CondSymbol {
    FPI_U32 name_offset;
    FPI_U32 hash;
    int canonical_id;
    FPI_BoundCondFn fn;
    void* bind_user;
} FPI_CondSymbol;

typedef struct FPI_ActSymbol {
    FPI_U32 name_offset;
    FPI_U32 hash;
    int canonical_id;
    FPI_BoundActFn fn;
    void* bind_user;
} FPI_ActSymbol;

typedef struct FPI_Registry {
    char pool[FPI_REGISTRY_POOL_BYTES];
    FPI_U32 pool_used;
    FPI_CondSymbol conditions[FPI_MAX_COND_SYMBOLS];
    int condition_count;
    FPI_ActSymbol actions[FPI_MAX_ACT_SYMBOLS];
    int action_count;
} FPI_Registry;

void fpi_registry_init(FPI_Registry* registry);
int fpi_registry_register_cond(FPI_Registry* registry, const char* name, FPI_Error* error);
int fpi_registry_register_act(FPI_Registry* registry, const char* name, FPI_Error* error);
int fpi_registry_find_cond(const FPI_Registry* registry, const char* name);
int fpi_registry_find_act(const FPI_Registry* registry, const char* name);
const char* fpi_registry_cond_name(const FPI_Registry* registry, int id);
const char* fpi_registry_act_name(const FPI_Registry* registry, int id);
int fpi_registry_bind_cond(FPI_Registry* registry, const char* name, FPI_BoundCondFn fn, void* bind_user, FPI_Error* error);
int fpi_registry_bind_act(FPI_Registry* registry, const char* name, FPI_BoundActFn fn, void* bind_user, FPI_Error* error);
int fpi_registry_bind_cond_id(FPI_Registry* registry, int id, FPI_BoundCondFn fn, void* bind_user);
int fpi_registry_bind_act_id(FPI_Registry* registry, int id, FPI_BoundActFn fn, void* bind_user);
int fpi_registry_alias_cond(FPI_Registry* registry, const char* alias_name, const char* target_name, FPI_Error* error);
int fpi_registry_alias_act(FPI_Registry* registry, const char* alias_name, const char* target_name, FPI_Error* error);
int fpi_registry_canonical_cond(const FPI_Registry* registry, int id);
int fpi_registry_canonical_act(const FPI_Registry* registry, int id);
int fpi_registry_dispatch_cond(const FPI_Registry* registry, void* run_user, int id, const FPI_Value* value);
int fpi_registry_dispatch_act(const FPI_Registry* registry, void* run_user, int id, const FPI_Value* value);

#endif /* FPI_REGISTRY_H */
