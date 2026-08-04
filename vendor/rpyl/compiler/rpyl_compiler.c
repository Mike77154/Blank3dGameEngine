#include "rpyl_compiler.h"

void rpyl_compiler_options_default(RpylCompilerOptions* options) {
    if (!options) return;
    options->validate_semantics = 1;
    options->build_ir = 1;
}

void rpyl_compiler_result_init(RpylCompilerResult* result) {
    if (!result) return;
    rpyl_semantics_report_init(&result->semantics);
    rpyl_ir_init(&result->ir);
    result->bytecode_words = 0u;
    result->string_count = 0u;
    result->label_count = 0u;
    result->define_count = 0u;
    result->had_ir = 0;
    result->success = 0;
}

int rpyl_compiler_compile_ast_ex(RpylBytecode* out, AstNode* root, const RpylCompilerOptions* options, RpylCompilerResult* result) {
    RpylCompilerOptions local_options;
    const RpylCompilerOptions* opt;
    int sem_ok;
    int bc_ok;

    if (!out || !root) return 0;
    if (options) opt = options;
    else {
        rpyl_compiler_options_default(&local_options);
        opt = &local_options;
    }
    if (result) rpyl_compiler_result_init(result);

    sem_ok = 1;
    if (opt->validate_semantics) {
        if (result) sem_ok = rpyl_semantics_validate_ex(root, &result->semantics);
        else sem_ok = rpyl_semantics_validate(root);
        if (!sem_ok) return 0;
    }

    if (opt->build_ir && result) {
        if (!rpyl_ir_from_ast(&result->ir, root)) return 0;
        result->had_ir = 1;
    }

    bc_ok = rpyl_bytecode_compile_ast(out, root);
    if (!bc_ok) return 0;

    if (result) {
        result->bytecode_words = out->code_count;
        result->string_count = out->string_count;
        result->label_count = out->label_count;
        result->define_count = out->define_count;
        result->success = 1;
    }
    return 1;
}

int rpyl_compiler_compile_ast(RpylBytecode* out, AstNode* root) {
    return rpyl_compiler_compile_ast_ex(out, root, (const RpylCompilerOptions*)0, (RpylCompilerResult*)0);
}
