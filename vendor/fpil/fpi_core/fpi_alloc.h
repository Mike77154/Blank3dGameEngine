#ifndef FPI_ALLOC_H
#define FPI_ALLOC_H

#include "fpi_types.h"

typedef void* (*FPI_AllocFn)(void* user, FPI_U32 bytes, FPI_U32 alignment);

typedef struct FPI_Allocator {
    FPI_AllocFn alloc;
    void* user;
} FPI_Allocator;

void* fpi_alloc(FPI_Allocator* allocator, FPI_U32 bytes, FPI_U32 alignment);

#endif /* FPI_ALLOC_H */
