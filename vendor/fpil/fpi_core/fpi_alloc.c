#include "fpi_alloc.h"

void* fpi_alloc(FPI_Allocator* allocator, FPI_U32 bytes, FPI_U32 alignment) {
    if (!allocator || !allocator->alloc || bytes == 0UL) return 0;
    return allocator->alloc(allocator->user, bytes, alignment);
}
