#include "fpi_stream.h"

void fpi_stream_init(FPI_Stream* stream, const char* text) {
    FPI_U32 n;
    if (!stream) return;
    stream->text = text ? text : "";
    n = 0UL;
    while (stream->text[n]) n++;
    stream->length = n;
    stream->position = 0UL;
    stream->line = 1;
    stream->column = 1;
}

char fpi_stream_peek(const FPI_Stream* stream) {
    if (!stream || stream->position >= stream->length) return '\0';
    return stream->text[stream->position];
}

char fpi_stream_peek_n(const FPI_Stream* stream, FPI_U32 n) {
    FPI_U32 p;
    if (!stream) return '\0';
    p = stream->position + n;
    if (p < stream->position || p >= stream->length) return '\0';
    return stream->text[p];
}

char fpi_stream_advance(FPI_Stream* stream) {
    char c;
    if (!stream || stream->position >= stream->length) return '\0';
    c = stream->text[stream->position++];
    if (c == '\r') {
        if (stream->position < stream->length && stream->text[stream->position] == '\n') stream->position++;
        stream->line++;
        stream->column = 1;
        return '\n';
    }
    if (c == '\n') {
        stream->line++;
        stream->column = 1;
    } else {
        stream->column++;
    }
    return c;
}

int fpi_stream_eof(const FPI_Stream* stream) {
    return !stream || stream->position >= stream->length;
}

FPI_Span fpi_stream_span_from(const FPI_Stream* stream, FPI_U32 start, int line, int column) {
    FPI_U32 length;
    if (!stream || stream->position < start) length = 0UL;
    else length = stream->position - start;
    return fpi_span_make(start, length, line, column);
}
