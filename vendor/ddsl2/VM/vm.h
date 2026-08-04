#ifndef DDSL_VM_H
#define DDSL_VM_H

#include "bytecode/bytecode.h"
#include "error/error.h"
#include "store/store.h"
#include "value/value.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Callback compatible con runtime.h.
 * Se declara con un guard para evitar re-definición si incluyes runtime.h.
 */
#ifndef DDSL_EMIT_FN_DEFINED
#define DDSL_EMIT_FN_DEFINED
typedef int (*ddsl_emit_fn)(void *user, const char *key_norm, ddsl_value value);
#endif

#ifndef DDSL_VM_STACK_MAX
#define DDSL_VM_STACK_MAX 128
#endif

typedef struct ddsl_vm {
    ddsl_store *store;
    ddsl_emit_fn emit;
    void *emit_user;

    ddsl_value stack[DDSL_VM_STACK_MAX];
    int sp;
} ddsl_vm;

void ddsl_vm_init(ddsl_vm *vm, ddsl_store *store);
void ddsl_vm_set_emit(ddsl_vm *vm, ddsl_emit_fn fn, void *user);

int ddsl_vm_run(ddsl_vm *vm, const ddsl_bc_program *prog, ddsl_error *err);

/* Atajo: compila (lexer+parser+IR+bytecode) y ejecuta.
 * La memoria usada vive en arena y se revierte al final.
 */
int ddsl_vm_exec_source(ddsl_vm *vm, ddsl_arena *arena, const char *source, ddsl_error *err);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_VM_H */
