#ifndef FPI_LEXER_H
#define FPI_LEXER_H

#include "fpi_stream.h"
#include "fpi_token.h"
#include "fpi_error.h"

typedef struct FPI_Lexer {
    FPI_Stream stream;
    FPI_Error* error;
} FPI_Lexer;

void fpi_lexer_init(FPI_Lexer* lexer, const char* source, FPI_Error* error);
FPI_Token fpi_lexer_next(FPI_Lexer* lexer);

/* Legacy names. */
typedef FPI_Lexer Lexer;
typedef FPI_Token Token;
void lexer_init(Lexer* lexer, const char* source);
Token lexer_next(Lexer* lexer);
void lexer_advance(Lexer* lexer);

#endif /* FPI_LEXER_H */
