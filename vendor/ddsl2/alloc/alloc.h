#ifndef DDSL_ALLOC_H
#define DDSL_ALLOC_H

#include <stddef.h>
#include "arena/arena.h"

#ifdef __cplusplus
extern "C" {
#endif

void *ddsl_alloc_block(ddsl_arena *arena, size_t bytes, size_t align);
void *ddsl_alloc_zero_block(ddsl_arena *arena, size_t bytes, size_t align);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_ALLOC_H */
