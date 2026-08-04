#ifndef FPI_POLYSYM_H
#define FPI_POLYSYM_H

#include "fpi_value.h"

typedef struct FPI_PolySymbol {
    FPI_U32 name_offset;
    FPI_U32 hash;
    FPI_Fixed value;
} FPI_PolySymbol;

typedef struct FPI_PolySym {
    char pool[FPI_POLYSYM_POOL_BYTES];
    FPI_U32 pool_used;
    FPI_PolySymbol entries[FPI_MAX_POLYSYMS];
    int count;
} FPI_PolySym;

void fpi_polysym_init(FPI_PolySym* table);
int fpi_polysym_define(FPI_PolySym* table, const char* name, FPI_Fixed value, FPI_Error* error);
int fpi_polysym_resolve(const FPI_PolySym* table, const char* name, FPI_Fixed* out_value);
const char* fpi_polysym_name(const FPI_PolySym* table, int index);

#endif /* FPI_POLYSYM_H */
