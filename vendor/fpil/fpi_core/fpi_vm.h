#ifndef FPI_VM_H
#define FPI_VM_H

#include "fpi_program.h"
#include "fpi_runtime.h"

int fpi_vm_run_tick_ex(void* user, const FPI_Program* program, const FPI_Bindings* bindings, const FPI_RunOptions* options, int* out_fired, FPI_Error* error);
int fpi_vm_run_tick(void* user, const FPI_Program* program, const FPI_Bindings* bindings, const FPI_RunOptions* options);

#endif /* FPI_VM_H */
