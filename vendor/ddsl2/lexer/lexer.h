#ifndef DDSL_LEXER_H
#define DDSL_LEXER_H

#include "token/token.h"
#include "error/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Tokeniza todo el source.
 * - Mantiene NEWLINE como token.
 * - Keywords son case-insensitive.
 * - 'then' también acepta: do, execute, =>, ->
 * - Comentarios: #, ; o // (hasta fin de línea)
 *
 * Devuelve 1 en éxito, 0 en error (ver err).
 */
int ddsl_lex_all(const char *source, ddsl_token_vec *out_tokens, ddsl_error *err);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_LEXER_H */
