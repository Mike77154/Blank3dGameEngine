#ifndef FPI_RUNTIME_H
#define FPI_RUNTIME_H

#include "fpi_ast.h"

typedef int (*FPI_EvalCondFn)(void* user, int condition_id, const FPI_Value* value);
typedef void (*FPI_ExecActFn)(void* user, int action_id, const FPI_Value* value);

typedef struct FPI_Bindings {
    FPI_EvalCondFn eval_cond;
    FPI_ExecActFn exec_act;
} FPI_Bindings;

int fpi_runtime_tick(void* user, const FPI_AST* ast, const FPI_Bindings* bindings, const FPI_RunOptions* options, int* out_fired, FPI_Error* error);
int fpi_run_tick(void* user, const AST_Script* script, const FPI_Bindings* bindings, const FPI_RunOptions* options);

#endif /* FPI_RUNTIME_H */
