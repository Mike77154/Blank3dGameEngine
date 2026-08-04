#include "program/program.h"

void ddsl_program_bundle_init(ddsl_program_bundle *b) {
    if (!b) return;
    b->ast = 0;
    b->ir = 0;
    b->bc = 0;
}

int ddsl_program_compile(ddsl_arena *arena, const char *source, ddsl_program_bundle *out, ddsl_error *err) {
    if (err) ddsl_error_clear(err);
    if (!arena || !source || !out) {
        if (err) ddsl_error_set(err, 0, 0, 0, "program: argumentos inválidos");
        return 0;
    }
    ddsl_program_bundle_init(out);
    if (!ddsl_compile_source_to_ir(arena, source, &out->ir, err)) return 0;
    if (!ddsl_bytecode_from_ir(arena, out->ir, &out->bc, err)) return 0;
    return 1;
}

int ddsl_program_run_vm(ddsl_program_bundle *b, ddsl_vm *vm, ddsl_error *err) {
    if (!b || !b->bc || !vm) {
        if (err) ddsl_error_set(err, 0, 0, 0, "program: bytecode inválido");
        return 0;
    }
    return ddsl_vm_run(vm, b->bc, err);
}
