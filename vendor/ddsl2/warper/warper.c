#include "warper/warper.h"
#include "VM/vm.h"

int ddsl_warper_exec_to_store(ddsl_arena *arena, const char *source, ddsl_store *store, ddsl_error *err) {
    ddsl_vm vm;
    if (!arena || !source || !store) {
        if (err) ddsl_error_set(err, 0, 0, 0, "warper: argumentos inválidos");
        return 0;
    }
    ddsl_vm_init(&vm, store);
    return ddsl_vm_exec_source(&vm, arena, source, err);
}
