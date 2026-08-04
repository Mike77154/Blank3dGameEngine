#ifndef VALUE_RESOLVER_FPI_H
#define VALUE_RESOLVER_FPI_H

#include "fpi.h"

struct FPI_VarSystem;

typedef int (*FPI_InternalVarFn)(void* user, const char* name, FPI_Fixed* out_value);

typedef struct FPI_ValueResolver {
    FPI_Context* context;
    struct FPI_VarSystem* vars;
    FPI_InternalVarFn internal_fn;
    void* internal_user;
} FPI_ValueResolver;

void fpi_value_resolver_init(FPI_ValueResolver* resolver, FPI_Context* context, struct FPI_VarSystem* vars, FPI_InternalVarFn internal_fn, void* internal_user);
int fpi_split_2tokens_ws(const char* text, char* token1, int token1_capacity, char* token2, int token2_capacity);
int fpi_resolve_fixed_token(const FPI_ValueResolver* resolver, const char* token, FPI_Fixed* out_value);
int fpi_resolve_int_token(const FPI_ValueResolver* resolver, const char* token, int* out_value);
int fpi_resolve_fixed_value(const FPI_ValueResolver* resolver, const FPI_Value* value, FPI_Fixed* out_value);
int fpi_resolve_int_value(const FPI_ValueResolver* resolver, const FPI_Value* value, int* out_value);

#endif /* VALUE_RESOLVER_FPI_H */
