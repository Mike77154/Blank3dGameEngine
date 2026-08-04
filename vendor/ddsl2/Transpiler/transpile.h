#ifndef DDSL_TRANSPILE_H
#define DDSL_TRANSPILE_H

#include "bytecode/bytecode.h"
#include "error/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ============================ Transpiler ============================
 * Genera un .c (C89) que embebe el bytecode y ejecuta vía ddsl_vm.
 * No escribe a archivos: el caller pasa un buffer de salida.
 */

typedef struct ddsl_transpile_opts {
    const char *func_name;     /* default: ddsl2_script_run */
    const char *prog_name;     /* default: ddsl2_script_prog */
} ddsl_transpile_opts;

int ddsl_transpile_bytecode_to_c(const ddsl_bc_program *bc,
                                 const ddsl_transpile_opts *opts,
                                 char *out,
                                 int out_cap,
                                 ddsl_error *err);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_TRANSPILE_H */
