#ifndef DDSL_SEMANTICS_H
#define DDSL_SEMANTICS_H

#include "ast/ast.h"
#include "error/error.h"

#ifndef DDSL_SEMANTICS_MAX_DEPTH
#define DDSL_SEMANTICS_MAX_DEPTH 256
#endif

#ifndef DDSL_SEMANTICS_MAX_NODES
#define DDSL_SEMANTICS_MAX_NODES 16384
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Valida invariantes del AST y límites que de otro modo serían truncados por
 * el store fijo. No reserva memoria y es seguro para C89.
 */
int ddsl_semantics_check_program(const ddsl_program *prog, ddsl_error *err);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_SEMANTICS_H */
