#ifndef FPI_TOKEN_H
#define FPI_TOKEN_H

#include "fpi_span.h"

typedef enum FPI_TokenType {
    FPI_TOK_EOF = 0,
    FPI_TOK_ERROR,
    FPI_TOK_EOL,
    FPI_TOK_COLON,
    FPI_TOK_COMMA,
    FPI_TOK_EQUAL,
    FPI_TOK_IDENTIFIER
} FPI_TokenType;

typedef struct FPI_Token {
    int type;
    FPI_Span span;
    char text[FPI_IDENT_MAX + 1];
} FPI_Token;

const char* fpi_token_type_name(int type);

#endif /* FPI_TOKEN_H */
