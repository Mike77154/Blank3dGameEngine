#ifndef FPI_API_H
#define FPI_API_H

#include "fpi_compiler.h"
#include "fpi_io.h"
#include "fpi_polysym.h"
#include "fpi_semantics.h"
#include "fpi_store.h"
#include "fpi_vm.h"

typedef struct FPI_Context {
    FPI_Registry registries[2];
    FPI_AST ast_banks[2];
    FPI_PolySym polysym;
    FPI_Store store;
    FPI_Error last_error;
    FPI_RunOptions run_options;
    FPI_U32 generation;
    int active_bank;
} FPI_Context;

void fpi_context_init(FPI_Context* context);
FPI_U32 fpi_context_size(void);
FPI_Context* fpi_create(void);
void fpi_destroy(FPI_Context* context);
int fpi_context_load_text(FPI_Context* context, const char* source_text);
int fpi_context_load_buffered(FPI_Context* context, const char* filename, char* buffer, FPI_U32 buffer_capacity);
int fpi_context_load_file(FPI_Context* context, const char* filename);
const FPI_Error* fpi_context_error(const FPI_Context* context);
const FPI_AST* fpi_context_ast(const FPI_Context* context);
const FPI_Registry* fpi_context_registry(const FPI_Context* context);
FPI_Registry* fpi_context_registry_mut(FPI_Context* context);
FPI_U32 fpi_context_generation(const FPI_Context* context);

void fpi_set_run_options(FPI_Context* context, const FPI_RunOptions* options);
void fpi_get_run_options(const FPI_Context* context, FPI_RunOptions* out_options);

int fpi_bind_cond(FPI_Context* context, const char* name, FPI_BoundCondFn fn, void* bind_user);
int fpi_bind_act(FPI_Context* context, const char* name, FPI_BoundActFn fn, void* bind_user);
int fpi_bind_cond_id(FPI_Context* context, int id, FPI_BoundCondFn fn, void* bind_user);
int fpi_bind_act_id(FPI_Context* context, int id, FPI_BoundActFn fn, void* bind_user);
int fpi_alias_cond(FPI_Context* context, const char* alias_name, const char* target_name);
int fpi_alias_act(FPI_Context* context, const char* alias_name, const char* target_name);
void fpi_clear_bindings(FPI_Context* context);
int fpi_get_cond_id(FPI_Context* context, const char* name);
int fpi_get_act_id(FPI_Context* context, const char* name);
const char* fpi_get_cond_name(const FPI_Context* context, int id);
const char* fpi_get_act_name(const FPI_Context* context, int id);

int fpi_define_value_symbol(FPI_Context* context, const char* name, FPI_Fixed value);
int fpi_resolve_value_fixed(const FPI_Context* context, const FPI_Value* value, FPI_Fixed* out_value);
int fpi_store_value(FPI_Context* context, const char* name, FPI_Fixed value);
int fpi_fetch_value(const FPI_Context* context, const char* name, FPI_Fixed* out_value);

int fpi_tick_ex(FPI_Context* context, void* user, FPI_EvalCondFn eval_cond, FPI_ExecActFn exec_act, const FPI_RunOptions* options);
void fpi_tick(FPI_Context* context, void* user, FPI_EvalCondFn eval_cond, FPI_ExecActFn exec_act);
int fpi_tick_bound_ex(FPI_Context* context, void* user, const FPI_RunOptions* options);
int fpi_tick_bound(FPI_Context* context, void* user);

int fpi_compile(FPI_Context* context, FPI_Arena* arena, FPI_Program* out_program);
int fpi_tick_vm(FPI_Context* context, const FPI_Program* program, void* user, FPI_EvalCondFn eval_cond, FPI_ExecActFn exec_act, const FPI_RunOptions* options);
int fpi_tick_vm_bound(FPI_Context* context, const FPI_Program* program, void* user, const FPI_RunOptions* options);

int fpi_value_parse_ints_ws(const FPI_Value* value, int* out_values, int capacity);
int fpi_value_parse_fixed_ws(const FPI_Context* context, const FPI_Value* value, FPI_Fixed* out_values, int capacity);

/* Compatibility lifecycle and load names. */
void fpi_init_context(FPI_Context* context);
int fpi_load_script_text(FPI_Context* context, const char* source_text);
int fpi_load_script_buffered(FPI_Context* context, const char* filename, char* buffer, long buffer_capacity);
int fpi_load_script(FPI_Context* context, const char* filename);

#endif /* FPI_API_H */
