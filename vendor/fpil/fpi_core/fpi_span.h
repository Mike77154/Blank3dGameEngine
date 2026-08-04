#ifndef FPI_SPAN_H
#define FPI_SPAN_H

#include "fpi_types.h"

typedef struct FPI_Span {
    FPI_U32 offset;
    FPI_U32 length;
    FPI_U16 line;
    FPI_U16 column;
} FPI_Span;

void fpi_span_clear(FPI_Span* span);
FPI_Span fpi_span_make(FPI_U32 offset, FPI_U32 length, int line, int column);

#endif /* FPI_SPAN_H */
