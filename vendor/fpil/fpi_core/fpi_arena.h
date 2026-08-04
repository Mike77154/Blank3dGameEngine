#ifndef FPI_ARENA_H
#define FPI_ARENA_H

#include "fpi_alloc.h"

typedef struct FPI_Arena {
    unsigned char* memory;
    FPI_U32 capacity;
    FPI_U32 used;
    int failed;
} FPI_Arena;

void fpi_arena_init(FPI_Arena* arena, void* memory, FPI_U32 capacity);
void fpi_arena_reset(FPI_Arena* arena);
void* fpi_arena_alloc(FPI_Arena* arena, FPI_U32 bytes, FPI_U32 alignment);
void* fpi_arena_allocator_fn(void* user, FPI_U32 bytes, FPI_U32 alignment);
FPI_Allocator fpi_arena_allocator(FPI_Arena* arena);
FPI_U32 fpi_arena_remaining(const FPI_Arena* arena);

#endif /* FPI_ARENA_H */
