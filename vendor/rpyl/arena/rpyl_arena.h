#ifndef RPYL_ARENA_H
#define RPYL_ARENA_H

/*
    rpyl_arena.h

    Bounded C89 arena region.

    The refactored arena never grows beyond its provided storage. It is either backed
    by caller-provided memory or by one fixed internal slot from the arena pool.
*/

#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct RpylArena RpylArena;

typedef struct {
    void* block;
    size_t offset;
} RpylArenaMark;

RpylArena* rpyl_arena_create(size_t initial_capacity);
RpylArena* rpyl_arena_create_with_buffer(void* buffer, size_t capacity);
void rpyl_arena_destroy(RpylArena* arena);

void* rpyl_arena_alloc(RpylArena* arena, size_t size, size_t align);
void* rpyl_arena_alloc_zero(RpylArena* arena, size_t size, size_t align);

RpylArenaMark rpyl_arena_mark(RpylArena* arena);
void rpyl_arena_rewind(RpylArena* arena, RpylArenaMark mark);
void rpyl_arena_reset(RpylArena* arena);

size_t rpyl_arena_used(const RpylArena* arena);
size_t rpyl_arena_capacity(const RpylArena* arena);
int rpyl_arena_overflowed(const RpylArena* arena);

#ifdef __cplusplus
}
#endif

#endif /* RPYL_ARENA_H */
