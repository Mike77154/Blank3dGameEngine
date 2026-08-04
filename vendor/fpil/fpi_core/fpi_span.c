#include "fpi_span.h"

void fpi_span_clear(FPI_Span* span) {
    if (!span) return;
    span->offset = 0UL;
    span->length = 0UL;
    span->line = 0;
    span->column = 0;
}

FPI_Span fpi_span_make(FPI_U32 offset, FPI_U32 length, int line, int column) {
    FPI_Span span;
    span.offset = offset;
    span.length = length;
    span.line = (FPI_U16)((line < 0) ? 0 : line);
    span.column = (FPI_U16)((column < 0) ? 0 : column);
    return span;
}
