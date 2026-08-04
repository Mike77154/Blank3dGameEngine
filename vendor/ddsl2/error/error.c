#include "error/error.h"

#include <string.h>

void ddsl_error_clear(ddsl_error *err) {
    if (!err) return;
    err->line = 0;
    err->col = 0;
    err->offset = 0;
    err->message[0] = '\0';
}

int ddsl_error_set(ddsl_error *err, int line, int col, int offset, const char *msg) {
    if (!err) return 0;
    err->line = line;
    err->col = col;
    err->offset = offset;
    if (!msg) msg = "";
    strncpy(err->message, msg, sizeof(err->message) - 1);
    err->message[sizeof(err->message) - 1] = '\0';
    return 1;
}
