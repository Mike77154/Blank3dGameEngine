#ifndef RPYL_ERROR_H
#define RPYL_ERROR_H

#include <stddef.h>
#include "rpyl_span.h"

#define RPYL_ERROR_MESSAGE_MAX 128
#ifndef RPYL_ERROR_MAX_DIAGNOSTICS
#define RPYL_ERROR_MAX_DIAGNOSTICS 32
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum RpylErrorCode {
    RPYL_ERROR_NONE = 0,
    RPYL_ERROR_OVERFLOW = 1,
    RPYL_ERROR_PARSE = 2,
    RPYL_ERROR_RUNTIME = 3,
    RPYL_ERROR_SEMANTIC = 4,
    RPYL_ERROR_IO = 5,
    RPYL_ERROR_BAD_ARGUMENT = 6,
    RPYL_ERROR_INTERNAL = 7
} RpylErrorCode;

typedef enum RpylErrorSeverity {
    RPYL_ERROR_SEVERITY_INFO = 0,
    RPYL_ERROR_SEVERITY_WARNING = 1,
    RPYL_ERROR_SEVERITY_ERROR = 2
} RpylErrorSeverity;

typedef struct RpylError {
    RpylErrorCode code;
    unsigned long line;
    unsigned long column;
    char message[RPYL_ERROR_MESSAGE_MAX];
} RpylError;

typedef struct RpylDiagnostic {
    RpylErrorCode code;
    RpylErrorSeverity severity;
    RpylSpan span;
    char message[RPYL_ERROR_MESSAGE_MAX];
} RpylDiagnostic;

typedef struct RpylErrorLog {
    RpylDiagnostic entries[RPYL_ERROR_MAX_DIAGNOSTICS];
    size_t count;
    size_t dropped;
    int has_error;
} RpylErrorLog;

const char* rpyl_error_code_name(RpylErrorCode code);
void rpyl_error_clear(RpylError* e);
void rpyl_error_set(RpylError* e, RpylErrorCode code, unsigned long line, unsigned long column, const char* message);

void rpyl_error_log_init(RpylErrorLog* log);
int rpyl_error_log_push(RpylErrorLog* log, RpylErrorCode code, RpylErrorSeverity severity, RpylSpan span, const char* message);
size_t rpyl_error_log_count(const RpylErrorLog* log);
const RpylDiagnostic* rpyl_error_log_at(const RpylErrorLog* log, size_t index);
int rpyl_error_log_has_error(const RpylErrorLog* log);

#ifdef __cplusplus
}
#endif

#endif
