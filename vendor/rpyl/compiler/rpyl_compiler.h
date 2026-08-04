#ifndef RPYL_COMPILER_H
#define RPYL_COMPILER_H

#include "rpyl_bytecode.h"
#include "rpyl_semantics.h"
#include "rpyl_ir.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RpylCompilerOptions {
    int validate_semantics;
    int build_ir;
} RpylCompilerOptions;

typedef struct RpylCompilerResult {
    RpylSemanticsReport semantics;
    RpylIrProgram ir;
    size_t bytecode_words;
    size_t string_count;
    size_t label_count;
    size_t define_count;
    int had_ir;
    int success;
} RpylCompilerResult;

void rpyl_compiler_options_default(RpylCompilerOptions* options);
void rpyl_compiler_result_init(RpylCompilerResult* result);
int rpyl_compiler_compile_ast_ex(RpylBytecode* out, AstNode* root, const RpylCompilerOptions* options, RpylCompilerResult* result);
int rpyl_compiler_compile_ast(RpylBytecode* out, AstNode* root);

#ifdef __cplusplus
}
#endif

#endif
