#include "rpyl_error.h"
#include "rpyl_common.h"

const char* rpyl_error_code_name(RpylErrorCode code) {
    switch (code) {
    case RPYL_ERROR_NONE: return "none";
    case RPYL_ERROR_OVERFLOW: return "overflow";
    case RPYL_ERROR_PARSE: return "parse";
    case RPYL_ERROR_RUNTIME: return "runtime";
    case RPYL_ERROR_SEMANTIC: return "semantic";
    case RPYL_ERROR_IO: return "io";
    case RPYL_ERROR_BAD_ARGUMENT: return "bad_argument";
    case RPYL_ERROR_INTERNAL: return "internal";
    default: return "unknown";
    }
}

void rpyl_error_clear(RpylError* e) {
    if (e != 0) {
        e->code = RPYL_ERROR_NONE;
        e->line = 0UL;
        e->column = 0UL;
        e->message[0] = '\0';
    }
}

void rpyl_error_set(RpylError* e, RpylErrorCode code, unsigned long line, unsigned long column, const char* message) {
    if (e == 0) return;
    e->code = code;
    e->line = line;
    e->column = column;
    (void)rpyl_common_copy(e->message, sizeof(e->message), message ? message : "");
}

void rpyl_error_log_init(RpylErrorLog* log) {
    size_t i;
    if (!log) return;
    log->count = 0u;
    log->dropped = 0u;
    log->has_error = 0;
    for (i = 0u; i < RPYL_ERROR_MAX_DIAGNOSTICS; i++) {
        log->entries[i].code = RPYL_ERROR_NONE;
        log->entries[i].severity = RPYL_ERROR_SEVERITY_INFO;
        log->entries[i].span = rpyl_span_zero();
        log->entries[i].message[0] = '\0';
    }
}

int rpyl_error_log_push(RpylErrorLog* log, RpylErrorCode code, RpylErrorSeverity severity, RpylSpan span, const char* message) {
    RpylDiagnostic* d;
    if (!log) return 0;
    if (severity == RPYL_ERROR_SEVERITY_ERROR) log->has_error = 1;
    if (log->count >= (size_t)RPYL_ERROR_MAX_DIAGNOSTICS) {
        log->dropped++;
        return 0;
    }
    d = &log->entries[log->count];
    d->code = code;
    d->severity = severity;
    d->span = span;
    (void)rpyl_common_copy(d->message, sizeof(d->message), message ? message : "");
    log->count++;
    return 1;
}

size_t rpyl_error_log_count(const RpylErrorLog* log) {
    if (!log) return 0u;
    return log->count;
}

const RpylDiagnostic* rpyl_error_log_at(const RpylErrorLog* log, size_t index) {
    if (!log) return 0;
    if (index >= log->count) return 0;
    return &log->entries[index];
}

int rpyl_error_log_has_error(const RpylErrorLog* log) {
    if (!log) return 0;
    return log->has_error ? 1 : 0;
}
