#include "alloc/alloc.h"

void *ddsl_alloc_block(ddsl_arena *arena, size_t bytes, size_t align) {
    return ddsl_arena_alloc(arena, bytes, align);
}

void *ddsl_alloc_zero_block(ddsl_arena *arena, size_t bytes, size_t align) {
    return ddsl_arena_alloc_zero(arena, bytes, align);
}
