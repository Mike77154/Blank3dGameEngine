#ifndef DDSL_RUNTIME_H
#define DDSL_RUNTIME_H

#include "store/store.h"
#include "value/value.h"
#include "ast/ast.h"
#include "arena/arena.h"
#include "error/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Callback opcional: se llama cada vez que una acción escribe algo.
 * - key_norm: key normalizada tipo snake_case
 * - value: el valor (num/bool/str)
 * Devuelve 1 para continuar, 0 para abortar ejecución.
 */
#ifndef DDSL_EMIT_FN_DEFINED
#define DDSL_EMIT_FN_DEFINED
typedef int (*ddsl_emit_fn)(void *user, const char *key_norm, ddsl_value value);
#endif

typedef struct ddsl_runtime {
    ddsl_store *store;
    ddsl_emit_fn emit;
    void *emit_user;
    ddsl_arena *arena; /* memoria temporal para lexer/parser/compilación */
} ddsl_runtime;

void ddsl_runtime_init(ddsl_runtime *rt, ddsl_store *store);
void ddsl_runtime_set_emit(ddsl_runtime *rt, ddsl_emit_fn fn, void *user);
void ddsl_runtime_set_arena(ddsl_runtime *rt, ddsl_arena *arena);

/* Ejecuta un AST ya parseado. */
int ddsl_runtime_run(ddsl_runtime *rt, const ddsl_program *prog, ddsl_error *err);

/* Atajo: compila (lexer+parser) y ejecuta. */
int ddsl_exec_source(ddsl_runtime *rt, const char *source, ddsl_error *err);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_RUNTIME_H */
