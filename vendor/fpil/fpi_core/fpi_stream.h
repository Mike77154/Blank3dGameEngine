#ifndef FPI_STREAM_H
#define FPI_STREAM_H

#include "fpi_span.h"

typedef struct FPI_Stream {
    const char* text;
    FPI_U32 length;
    FPI_U32 position;
    int line;
    int column;
} FPI_Stream;

void fpi_stream_init(FPI_Stream* stream, const char* text);
char fpi_stream_peek(const FPI_Stream* stream);
char fpi_stream_peek_n(const FPI_Stream* stream, FPI_U32 n);
char fpi_stream_advance(FPI_Stream* stream);
int fpi_stream_eof(const FPI_Stream* stream);
FPI_Span fpi_stream_span_from(const FPI_Stream* stream, FPI_U32 start, int line, int column);

#endif /* FPI_STREAM_H */
