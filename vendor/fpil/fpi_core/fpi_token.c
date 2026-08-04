#include "fpi_token.h"

const char* fpi_token_type_name(int type) {
    switch (type) {
        case FPI_TOK_EOF: return "EOF";
        case FPI_TOK_ERROR: return "ERROR";
        case FPI_TOK_EOL: return "EOL";
        case FPI_TOK_COLON: return "COLON";
        case FPI_TOK_COMMA: return "COMMA";
        case FPI_TOK_EQUAL: return "EQUAL";
        case FPI_TOK_IDENTIFIER: return "IDENTIFIER";
        default: return "UNKNOWN";
    }
}
