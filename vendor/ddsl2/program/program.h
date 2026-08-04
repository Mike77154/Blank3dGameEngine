#ifndef DDSL_PROGRAM_H
#define DDSL_PROGRAM_H

#include "arena/arena.h"
#include "bytecode/bytecode.h"
#include "runtime/runtime.h"
#include "VM/vm.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ddsl_program_bundle {
    ddsl_program *ast;
    ddsl_ir_program *ir;
    ddsl_bc_program *bc;
} ddsl_program_bundle;

void ddsl_program_bundle_init(ddsl_program_bundle *b);
int ddsl_program_compile(ddsl_arena *arena, const char *source, ddsl_program_bundle *out, ddsl_error *err);
int ddsl_program_run_vm(ddsl_program_bundle *b, ddsl_vm *vm, ddsl_error *err);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_PROGRAM_H */
