#include "fpi_lexer.h"
#include "fpi_util.h"

static FPI_Token token_make(int type, FPI_U32 offset, FPI_U32 length, int line, int column) {
    FPI_Token token;
    token.type = type;
    token.span = fpi_span_make(offset, length, line, column);
    token.text[0] = '\0';
    return token;
}

void fpi_lexer_init(FPI_Lexer* lexer, const char* source, FPI_Error* error) {
    if (!lexer) return;
    fpi_stream_init(&lexer->stream, source);
    lexer->error = error;
}

static void lexer_skip_horizontal(FPI_Lexer* lexer) {
    char c;
    c = fpi_stream_peek(&lexer->stream);
    while (c == ' ' || c == '\t') {
        fpi_stream_advance(&lexer->stream);
        c = fpi_stream_peek(&lexer->stream);
    }
}

FPI_Token fpi_lexer_next(FPI_Lexer* lexer) {
    FPI_U32 start;
    int line;
    int column;
    char c;
    FPI_Token token;
    int len;
    FPI_Span span;

    if (!lexer) return token_make(FPI_TOK_EOF, 0UL, 0UL, 0, 0);
    for (;;) {
        lexer_skip_horizontal(lexer);
        start = lexer->stream.position;
        line = lexer->stream.line;
        column = lexer->stream.column;
        c = fpi_stream_peek(&lexer->stream);
        if (c == '\0') return token_make(FPI_TOK_EOF, start, 0UL, line, column);
        if (c == ';') {
            while ((c = fpi_stream_peek(&lexer->stream)) != '\0' && c != '\r' && c != '\n')
                fpi_stream_advance(&lexer->stream);
            continue;
        }
        if (c == '\r' || c == '\n') {
            fpi_stream_advance(&lexer->stream);
            return token_make(FPI_TOK_EOL, start, lexer->stream.position - start, line, column);
        }
        if (c == ':') {
            fpi_stream_advance(&lexer->stream);
            return token_make(FPI_TOK_COLON, start, 1UL, line, column);
        }
        if (c == ',') {
            fpi_stream_advance(&lexer->stream);
            return token_make(FPI_TOK_COMMA, start, 1UL, line, column);
        }
        if (c == '=') {
            fpi_stream_advance(&lexer->stream);
            return token_make(FPI_TOK_EQUAL, start, 1UL, line, column);
        }
        if (fpi_ascii_is_ident_start(c)) {
            token = token_make(FPI_TOK_IDENTIFIER, start, 0UL, line, column);
            len = 0;
            while (fpi_ascii_is_ident_continue(fpi_stream_peek(&lexer->stream))) {
                if (len >= FPI_IDENT_MAX) {
                    while (fpi_ascii_is_ident_continue(fpi_stream_peek(&lexer->stream)))
                        fpi_stream_advance(&lexer->stream);
                    span = fpi_stream_span_from(&lexer->stream, start, line, column);
                    fpi_error_set(lexer->error, FPI_ERR_IDENTIFIER_TOO_LONG, span, "identifier exceeds FPI_IDENT_MAX");
                    return token_make(FPI_TOK_ERROR, start, span.length, line, column);
                }
                token.text[len++] = fpi_ascii_tolower(fpi_stream_advance(&lexer->stream));
            }
            token.text[len] = '\0';
            token.span.length = lexer->stream.position - start;
            return token;
        }
        fpi_stream_advance(&lexer->stream);
        span = fpi_span_make(start, 1UL, line, column);
        fpi_error_set(lexer->error, FPI_ERR_SYNTAX, span, "unexpected character outside a RHS value");
        return token_make(FPI_TOK_ERROR, start, 1UL, line, column);
    }
}

void lexer_init(Lexer* lexer, const char* source) { fpi_lexer_init(lexer, source, 0); }
Token lexer_next(Lexer* lexer) { return fpi_lexer_next(lexer); }
void lexer_advance(Lexer* lexer) { if (lexer) (void)fpi_stream_advance(&lexer->stream); }
