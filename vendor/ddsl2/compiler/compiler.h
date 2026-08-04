#ifndef DDSL_COMPILER_H
#define DDSL_COMPILER_H

#include "arena/arena.h"
#include "IR/ir.h"
#include "bytecode/bytecode.h"
#include "error/error.h"

#ifdef __cplusplus
extern "C" {
#endif

int ddsl_compiler_source_to_ir(ddsl_arena *arena, const char *source, ddsl_ir_program **out_ir, ddsl_error *err);
int ddsl_compiler_source_to_bytecode(ddsl_arena *arena, const char *source, ddsl_bc_program **out_bc, ddsl_error *err);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_COMPILER_H */
