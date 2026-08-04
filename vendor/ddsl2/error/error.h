#ifndef DDSL_ERROR_H
#define DDSL_ERROR_H

/* Error simple con posición (línea/columna) y mensaje.
 * Diseñado para ser fácil de usar desde C89.
 */

#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ddsl_error {
    int line;   /* 1-based */
    int col;    /* 1-based */
    int offset; /* 0-based byte offset */
    char message[256];
} ddsl_error;

void ddsl_error_clear(ddsl_error *err);
int ddsl_error_set(ddsl_error *err, int line, int col, int offset, const char *msg);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_ERROR_H */
