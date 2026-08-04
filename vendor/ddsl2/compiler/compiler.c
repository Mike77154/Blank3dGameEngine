#include "compiler/compiler.h"

int ddsl_compiler_source_to_ir(ddsl_arena *arena, const char *source, ddsl_ir_program **out_ir, ddsl_error *err) {
    return ddsl_compile_source_to_ir(arena, source, out_ir, err);
}

int ddsl_compiler_source_to_bytecode(ddsl_arena *arena, const char *source, ddsl_bc_program **out_bc, ddsl_error *err) {
    return ddsl_compile_source_to_bytecode(arena, source, out_bc, err);
}
