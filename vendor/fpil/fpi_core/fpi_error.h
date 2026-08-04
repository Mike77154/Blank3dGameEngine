#ifndef FPI_ERROR_H
#define FPI_ERROR_H

#include "fpi_span.h"

typedef enum FPI_Result {
    FPI_OK = 0,
    FPI_ERR_ARGUMENT = -1,
    FPI_ERR_IO = -2,
    FPI_ERR_FILE_TOO_LARGE = -3,
    FPI_ERR_IDENTIFIER_TOO_LONG = -4,
    FPI_ERR_VALUE_TOO_LONG = -5,
    FPI_ERR_TOO_MANY_RULES = -6,
    FPI_ERR_TOO_MANY_TERMS = -7,
    FPI_ERR_TOO_MANY_TERMS_IN_RULE = -8,
    FPI_ERR_SYMBOL_TABLE_FULL = -9,
    FPI_ERR_STRING_POOL_FULL = -10,
    FPI_ERR_POLYSYM_FULL = -11,
    FPI_ERR_SYNTAX = -12,
    FPI_ERR_UNTERMINATED_STRING = -13,
    FPI_ERR_INVALID_ESCAPE = -14,
    FPI_ERR_INVALID_NUMBER = -15,
    FPI_ERR_NUMBER_OVERFLOW = -16,
    FPI_ERR_SEMANTIC = -17,
    FPI_ERR_ARENA_EXHAUSTED = -18,
    FPI_ERR_PROGRAM_INVALID = -19,
    FPI_ERR_VM_BOUNDS = -20,
    FPI_ERR_UNKNOWN_OPCODE = -21,
    FPI_ERR_UNBOUND_SYMBOL = -22,
    FPI_ERR_GENERATION_MISMATCH = -23
} FPI_Result;

typedef struct FPI_Error {
    int code;
    FPI_Span span;
    char message[FPI_ERROR_MESSAGE_MAX + 1];
} FPI_Error;

void fpi_error_clear(FPI_Error* error);
void fpi_error_set(FPI_Error* error, int code, FPI_Span span, const char* message);
const char* fpi_result_name(int code);

#endif /* FPI_ERROR_H */
