#include "fpi_arena.h"

void fpi_arena_init(FPI_Arena* arena, void* memory, FPI_U32 capacity) {
    if (!arena) return;
    arena->memory = (unsigned char*)memory;
    arena->capacity = memory ? capacity : 0UL;
    arena->used = 0UL;
    arena->failed = 0;
}

void fpi_arena_reset(FPI_Arena* arena) {
    if (!arena) return;
    arena->used = 0UL;
    arena->failed = 0;
}

void* fpi_arena_alloc(FPI_Arena* arena, FPI_U32 bytes, FPI_U32 alignment) {
    FPI_U32 mask;
    FPI_U32 start;
    if (!arena || !arena->memory || bytes == 0UL) return 0;
    if (alignment == 0UL) alignment = 1UL;
    if ((alignment & (alignment - 1UL)) != 0UL) { arena->failed = 1; return 0; }
    mask = alignment - 1UL;
    start = (arena->used + mask) & ~mask;
    if (start > arena->capacity || bytes > arena->capacity - start) {
        arena->failed = 1;
        return 0;
    }
    arena->used = start + bytes;
    return arena->memory + start;
}

void* fpi_arena_allocator_fn(void* user, FPI_U32 bytes, FPI_U32 alignment) {
    return fpi_arena_alloc((FPI_Arena*)user, bytes, alignment);
}

FPI_Allocator fpi_arena_allocator(FPI_Arena* arena) {
    FPI_Allocator allocator;
    allocator.alloc = fpi_arena_allocator_fn;
    allocator.user = arena;
    return allocator;
}

FPI_U32 fpi_arena_remaining(const FPI_Arena* arena) {
    if (!arena || arena->used >= arena->capacity) return 0UL;
    return arena->capacity - arena->used;
}
