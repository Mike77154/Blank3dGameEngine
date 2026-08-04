#include "fpi_error.h"
#include "fpi_util.h"

void fpi_error_clear(FPI_Error* error) {
    if (!error) return;
    error->code = FPI_OK;
    fpi_span_clear(&error->span);
    error->message[0] = '\0';
}

void fpi_error_set(FPI_Error* error, int code, FPI_Span span, const char* message) {
    if (!error) return;
    error->code = code;
    error->span = span;
    fpi_str_copy(error->message, FPI_ERROR_MESSAGE_MAX + 1, message ? message : fpi_result_name(code));
}

const char* fpi_result_name(int code) {
    switch (code) {
        case FPI_OK: return "ok";
        case FPI_ERR_ARGUMENT: return "invalid argument";
        case FPI_ERR_IO: return "I/O error";
        case FPI_ERR_FILE_TOO_LARGE: return "file too large";
        case FPI_ERR_IDENTIFIER_TOO_LONG: return "identifier too long";
        case FPI_ERR_VALUE_TOO_LONG: return "value too long";
        case FPI_ERR_TOO_MANY_RULES: return "too many rules";
        case FPI_ERR_TOO_MANY_TERMS: return "too many terms";
        case FPI_ERR_TOO_MANY_TERMS_IN_RULE: return "too many terms in rule";
        case FPI_ERR_SYMBOL_TABLE_FULL: return "symbol table full";
        case FPI_ERR_STRING_POOL_FULL: return "string pool full";
        case FPI_ERR_POLYSYM_FULL: return "polysym table full";
        case FPI_ERR_SYNTAX: return "syntax error";
        case FPI_ERR_UNTERMINATED_STRING: return "unterminated string";
        case FPI_ERR_INVALID_ESCAPE: return "invalid escape";
        case FPI_ERR_INVALID_NUMBER: return "invalid number";
        case FPI_ERR_NUMBER_OVERFLOW: return "number overflow";
        case FPI_ERR_SEMANTIC: return "semantic error";
        case FPI_ERR_ARENA_EXHAUSTED: return "arena exhausted";
        case FPI_ERR_PROGRAM_INVALID: return "invalid program";
        case FPI_ERR_VM_BOUNDS: return "VM bounds error";
        case FPI_ERR_UNKNOWN_OPCODE: return "unknown opcode";
        case FPI_ERR_UNBOUND_SYMBOL: return "unbound symbol";
        case FPI_ERR_GENERATION_MISMATCH: return "program generation mismatch";
        default: return "unknown error";
    }
}
