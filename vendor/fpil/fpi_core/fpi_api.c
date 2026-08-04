#include "fpi_api.h"
#include "fpi_parser.h"
#include "fpi_util.h"

static FPI_Context fpi_static_contexts[FPI_STATIC_CONTEXTS];
static int fpi_static_used[FPI_STATIC_CONTEXTS];
static char fpi_static_file_buffer[FPI_STATIC_FILE_BYTES];

static FPI_Registry* active_registry_mut(FPI_Context* context) {
    if (!context) return 0;
    return &context->registries[context->active_bank];
}

static const FPI_Registry* active_registry(const FPI_Context* context) {
    if (!context) return 0;
    return &context->registries[context->active_bank];
}


static const FPI_AST* active_ast(const FPI_Context* context) {
    if (!context) return 0;
    return &context->ast_banks[context->active_bank];
}

void fpi_context_init(FPI_Context* context) {
    if (!context) return;
    fpi_registry_init(&context->registries[0]);
    fpi_registry_init(&context->registries[1]);
    fpi_ast_init(&context->ast_banks[0]);
    fpi_ast_init(&context->ast_banks[1]);
    fpi_polysym_init(&context->polysym);
    fpi_store_init(&context->store);
    fpi_error_clear(&context->last_error);
    context->run_options.stop_on_first_match = 0;
    context->run_options.exec_mode = FPI_EXEC_SEQUENTIAL_IMMEDIATE;
    context->generation = 0UL;
    context->active_bank = 0;
}

void fpi_init_context(FPI_Context* context) { fpi_context_init(context); }

FPI_U32 fpi_context_size(void) { return (FPI_U32)sizeof(FPI_Context); }

FPI_Context* fpi_create(void) {
    int i;
    for (i = 0; i < FPI_STATIC_CONTEXTS; i++) {
        if (!fpi_static_used[i]) {
            fpi_static_used[i] = 1;
            fpi_context_init(&fpi_static_contexts[i]);
            return &fpi_static_contexts[i];
        }
    }
    return 0;
}

void fpi_destroy(FPI_Context* context) {
    int i;
    if (!context) return;
    for (i = 0; i < FPI_STATIC_CONTEXTS; i++) {
        if (context == &fpi_static_contexts[i]) {
            fpi_context_init(context);
            fpi_static_used[i] = 0;
            return;
        }
    }
    fpi_context_init(context);
}

int fpi_context_load_text(FPI_Context* context, const char* source_text) {
    int staging;
    int rc;
    FPI_Parser parser;
    if (!context || !source_text) return FPI_ERR_ARGUMENT;
    fpi_error_clear(&context->last_error);
    staging = 1 - context->active_bank;
    context->registries[staging] = context->registries[context->active_bank];
    fpi_ast_init(&context->ast_banks[staging]);
    fpi_parser_init(&parser, source_text, &context->registries[staging], &context->ast_banks[staging], &context->last_error);
    rc = fpi_parse_script(&parser);
    if (rc != FPI_OK) return rc;
    rc = fpi_semantics_validate(&context->ast_banks[staging], &context->registries[staging], &context->last_error);
    if (rc != FPI_OK) return rc;
    context->active_bank = staging;
    context->generation++;
    return FPI_OK;
}

int fpi_context_load_buffered(FPI_Context* context, const char* filename, char* buffer, FPI_U32 buffer_capacity) {
    FPI_U32 length;
    int rc;
    if (!context) return FPI_ERR_ARGUMENT;
    fpi_error_clear(&context->last_error);
    rc = fpi_io_read_file(filename, buffer, buffer_capacity, &length, &context->last_error);
    FPI_UNUSED(length);
    if (rc != FPI_OK) return rc;
    return fpi_context_load_text(context, buffer);
}

int fpi_context_load_file(FPI_Context* context, const char* filename) {
    return fpi_context_load_buffered(context, filename, fpi_static_file_buffer, FPI_STATIC_FILE_BYTES);
}

int fpi_load_script_text(FPI_Context* context, const char* source_text) {
    return fpi_context_load_text(context, source_text) == FPI_OK ? 0 : -1;
}

int fpi_load_script_buffered(FPI_Context* context, const char* filename, char* buffer, long buffer_capacity) {
    if (buffer_capacity <= 0L) return -1;
    return fpi_context_load_buffered(context, filename, buffer, (FPI_U32)buffer_capacity) == FPI_OK ? 0 : -1;
}

int fpi_load_script(FPI_Context* context, const char* filename) {
    return fpi_context_load_file(context, filename) == FPI_OK ? 0 : -1;
}

const FPI_Error* fpi_context_error(const FPI_Context* context) { return context ? &context->last_error : 0; }
const FPI_AST* fpi_context_ast(const FPI_Context* context) { return active_ast(context); }
const FPI_Registry* fpi_context_registry(const FPI_Context* context) { return active_registry(context); }
FPI_Registry* fpi_context_registry_mut(FPI_Context* context) { return active_registry_mut(context); }
FPI_U32 fpi_context_generation(const FPI_Context* context) { return context ? context->generation : 0UL; }

void fpi_set_run_options(FPI_Context* context, const FPI_RunOptions* options) {
    if (!context) return;
    if (!options) {
        context->run_options.stop_on_first_match = 0;
        context->run_options.exec_mode = FPI_EXEC_SEQUENTIAL_IMMEDIATE;
    } else context->run_options = *options;
}

void fpi_get_run_options(const FPI_Context* context, FPI_RunOptions* out_options) {
    if (context && out_options) *out_options = context->run_options;
}

int fpi_bind_cond(FPI_Context* context, const char* name, FPI_BoundCondFn fn, void* bind_user) {
    if (!context) return FPI_ERR_ARGUMENT;
    return fpi_registry_bind_cond(active_registry_mut(context), name, fn, bind_user, &context->last_error);
}

int fpi_bind_act(FPI_Context* context, const char* name, FPI_BoundActFn fn, void* bind_user) {
    if (!context) return FPI_ERR_ARGUMENT;
    return fpi_registry_bind_act(active_registry_mut(context), name, fn, bind_user, &context->last_error);
}

int fpi_bind_cond_id(FPI_Context* context, int id, FPI_BoundCondFn fn, void* bind_user) {
    return context ? fpi_registry_bind_cond_id(active_registry_mut(context), id, fn, bind_user) : FPI_ERR_ARGUMENT;
}

int fpi_bind_act_id(FPI_Context* context, int id, FPI_BoundActFn fn, void* bind_user) {
    return context ? fpi_registry_bind_act_id(active_registry_mut(context), id, fn, bind_user) : FPI_ERR_ARGUMENT;
}

int fpi_alias_cond(FPI_Context* context, const char* alias_name, const char* target_name) {
    if (!context) return FPI_ERR_ARGUMENT;
    return fpi_registry_alias_cond(active_registry_mut(context), alias_name, target_name, &context->last_error);
}

int fpi_alias_act(FPI_Context* context, const char* alias_name, const char* target_name) {
    if (!context) return FPI_ERR_ARGUMENT;
    return fpi_registry_alias_act(active_registry_mut(context), alias_name, target_name, &context->last_error);
}

void fpi_clear_bindings(FPI_Context* context) {
    FPI_Registry* registry;
    int i;
    if (!context) return;
    registry = active_registry_mut(context);
    for (i = 0; i < registry->condition_count; i++) {
        registry->conditions[i].fn = 0;
        registry->conditions[i].bind_user = 0;
    }
    for (i = 0; i < registry->action_count; i++) {
        registry->actions[i].fn = 0;
        registry->actions[i].bind_user = 0;
    }
}

int fpi_get_cond_id(FPI_Context* context, const char* name) {
    if (!context) return FPI_ERR_ARGUMENT;
    return fpi_registry_register_cond(active_registry_mut(context), name, &context->last_error);
}

int fpi_get_act_id(FPI_Context* context, const char* name) {
    if (!context) return FPI_ERR_ARGUMENT;
    return fpi_registry_register_act(active_registry_mut(context), name, &context->last_error);
}

const char* fpi_get_cond_name(const FPI_Context* context, int id) { return fpi_registry_cond_name(active_registry(context), id); }
const char* fpi_get_act_name(const FPI_Context* context, int id) { return fpi_registry_act_name(active_registry(context), id); }

int fpi_define_value_symbol(FPI_Context* context, const char* name, FPI_Fixed value) {
    if (!context) return FPI_ERR_ARGUMENT;
    return fpi_polysym_define(&context->polysym, name, value, &context->last_error);
}

int fpi_store_value(FPI_Context* context, const char* name, FPI_Fixed value) {
    if (!context) return FPI_ERR_ARGUMENT;
    return fpi_store_set(&context->store, name, value, &context->last_error);
}

int fpi_fetch_value(const FPI_Context* context, const char* name, FPI_Fixed* out_value) {
    return context ? fpi_store_get(&context->store, name, out_value) : 0;
}

int fpi_resolve_value_fixed(const FPI_Context* context, const FPI_Value* value, FPI_Fixed* out_value) {
    if (!context || !value || !value->has || !out_value) return 0;
    if (value->kind == FPI_VALUE_FIXED) {
        *out_value = value->fixed;
        return 1;
    }
    if (value->s && value->s[0] == '%') return fpi_store_get(&context->store, value->s + 1, out_value);
    if (fpi_polysym_resolve(&context->polysym, value->s, out_value)) return 1;
    return fpi_store_get(&context->store, value->s, out_value);
}

typedef struct FPI_BoundDispatch {
    FPI_Context* context;
    void* run_user;
} FPI_BoundDispatch;

static int bound_eval(void* user, int id, const FPI_Value* value) {
    FPI_BoundDispatch* dispatch;
    dispatch = (FPI_BoundDispatch*)user;
    if (!dispatch || !dispatch->context) return 0;
    return fpi_registry_dispatch_cond(active_registry(dispatch->context), dispatch->run_user, id, value);
}

static void bound_exec(void* user, int id, const FPI_Value* value) {
    FPI_BoundDispatch* dispatch;
    int rc;
    dispatch = (FPI_BoundDispatch*)user;
    if (!dispatch || !dispatch->context) return;
    rc = fpi_registry_dispatch_act(active_registry(dispatch->context),
                                   dispatch->run_user, id, value);
    if (rc < 0) {
        fpi_error_set(&dispatch->context->last_error, rc,
                      fpi_span_make(0UL, 0UL, 0, 0),
                      "action dispatch failed");
    }
}

int fpi_tick_ex(FPI_Context* context, void* user, FPI_EvalCondFn eval_cond, FPI_ExecActFn exec_act, const FPI_RunOptions* options) {
    FPI_Bindings bindings;
    int fired;
    int rc;
    if (!context) return 0;
    fpi_error_clear(&context->last_error);
    bindings.eval_cond = eval_cond;
    bindings.exec_act = exec_act;
    rc = fpi_runtime_tick(user, active_ast(context), &bindings,
                          options ? options : &context->run_options,
                          &fired, &context->last_error);
    if (rc != FPI_OK || context->last_error.code != FPI_OK) return 0;
    return fired;
}

void fpi_tick(FPI_Context* context, void* user, FPI_EvalCondFn eval_cond, FPI_ExecActFn exec_act) {
    (void)fpi_tick_ex(context, user, eval_cond, exec_act, 0);
}

int fpi_tick_bound_ex(FPI_Context* context, void* user, const FPI_RunOptions* options) {
    FPI_BoundDispatch dispatch;
    if (!context) return 0;
    dispatch.context = context;
    dispatch.run_user = user;
    return fpi_tick_ex(context, &dispatch, bound_eval, bound_exec, options);
}

int fpi_tick_bound(FPI_Context* context, void* user) { return fpi_tick_bound_ex(context, user, 0); }

int fpi_compile(FPI_Context* context, FPI_Arena* arena, FPI_Program* out_program) {
    if (!context) return FPI_ERR_ARGUMENT;
    fpi_error_clear(&context->last_error);
    return fpi_compile_ast(active_ast(context), context->generation, arena, out_program, &context->last_error);
}

int fpi_tick_vm(FPI_Context* context, const FPI_Program* program, void* user, FPI_EvalCondFn eval_cond, FPI_ExecActFn exec_act, const FPI_RunOptions* options) {
    FPI_Bindings bindings;
    int fired;
    int rc;
    if (!context || !program) return 0;
    fpi_error_clear(&context->last_error);
    if (program->source_generation != context->generation) {
        fpi_error_set(&context->last_error, FPI_ERR_GENERATION_MISMATCH, fpi_span_make(0UL, 0UL, 0, 0), "bytecode was compiled from a different script generation");
        return 0;
    }
    bindings.eval_cond = eval_cond;
    bindings.exec_act = exec_act;
    rc = fpi_vm_run_tick_ex(user, program, &bindings,
                            options ? options : &context->run_options,
                            &fired, &context->last_error);
    if (rc != FPI_OK || context->last_error.code != FPI_OK) return 0;
    return fired;
}

int fpi_tick_vm_bound(FPI_Context* context, const FPI_Program* program, void* user, const FPI_RunOptions* options) {
    FPI_BoundDispatch dispatch;
    if (!context) return 0;
    dispatch.context = context;
    dispatch.run_user = user;
    return fpi_tick_vm(context, program, &dispatch, bound_eval, bound_exec, options);
}

static int parse_one_int(const char** cursor, int* out_value) {
    const char* s;
    int sign;
    int value;
    int digits;
    s = *cursor;
    while (*s == ' ' || *s == '\t') s++;
    sign = 1;
    if (*s == '-' || *s == '+') { if (*s == '-') sign = -1; s++; }
    value = 0;
    digits = 0;
    while (fpi_ascii_is_digit(*s)) {
        value = value * 10 + (*s - '0');
        s++;
        digits++;
    }
    if (!digits || (*s && *s != ' ' && *s != '\t')) return 0;
    *out_value = sign * value;
    *cursor = s;
    return 1;
}

int fpi_value_parse_ints_ws(const FPI_Value* value, int* out_values, int capacity) {
    const char* cursor;
    int count;
    if (!value || !value->has || !out_values || capacity <= 0) return 0;
    cursor = value->s;
    count = 0;
    while (*cursor && count < capacity) {
        if (!parse_one_int(&cursor, &out_values[count])) break;
        count++;
        while (*cursor == ' ' || *cursor == '\t') cursor++;
    }
    return count;
}

int fpi_value_parse_fixed_ws(const FPI_Context* context, const FPI_Value* value, FPI_Fixed* out_values, int capacity) {
    const char* cursor;
    char token[FPI_IDENT_MAX + 32];
    int len;
    int count;
    FPI_Value single;
    if (!context || !value || !value->has || !out_values || capacity <= 0) return 0;
    cursor = value->s;
    count = 0;
    while (*cursor && count < capacity) {
        while (*cursor == ' ' || *cursor == '\t') cursor++;
        len = 0;
        while (*cursor && *cursor != ' ' && *cursor != '\t') {
            if (len >= (int)sizeof(token) - 1) return count;
            token[len++] = *cursor++;
        }
        token[len] = '\0';
        fpi_value_clear(&single);
        single.has = 1;
        single.s = token;
        single.len = len;
        if (fpi_fixed_parse(token, &single.fixed) == FPI_OK) single.kind = FPI_VALUE_FIXED;
        else single.kind = FPI_VALUE_TEXT;
        if (!fpi_resolve_value_fixed(context, &single, &out_values[count])) return count;
        count++;
    }
    return count;
}
