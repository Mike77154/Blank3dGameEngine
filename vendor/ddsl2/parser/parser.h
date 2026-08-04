#ifndef DDSL_PARSER_H
#define DDSL_PARSER_H

#include "ast/ast.h"
#include "arena/arena.h"
#include "error/error.h"
#include "token/token.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Parsea tokens -> AST.
 * Devuelve 1 en éxito, 0 en error (ver err).
 */
/* Parsea tokens -> AST usando arena.
 * - out_prog apunta a memoria dentro de la arena.
 * - En error, la arena se revierte al punto inicial de parseo.
 */
int ddsl_parse_program(const ddsl_token_vec *tokens, ddsl_arena *arena, ddsl_program **out_prog, ddsl_error *err);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_PARSER_H */
