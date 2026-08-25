#include "sm3d_arena.h"

void sm3d_arena_init(SM3D_Arena *arena, void *memory, long capacity)
{
    if (arena == 0) return;
    arena->base = (unsigned char *)memory;
    arena->capacity = capacity;
    arena->used = 0;
    arena->high_water = 0;
    arena->error = 0;
}

void sm3d_arena_clear(SM3D_Arena *arena)
{
    if (arena == 0) return;
    arena->used = 0;
    arena->error = 0;
}

void *sm3d_arena_alloc(SM3D_Arena *arena, long size, long align)
{
    long at;
    long mask;
    long aligned;
    void *p;

    if (arena == 0) return 0;
    if (arena->base == 0) return 0;
    if (size <= 0) return 0;
    if (align <= 0) align = 1;

    mask = align - 1;
    at = arena->used;
    aligned = (at + mask) & ~mask;

    if (aligned + size > arena->capacity) {
        arena->error = 1;
        return 0;
    }

    p = (void *)(arena->base + aligned);
    arena->used = aligned + size;
    if (arena->used > arena->high_water) arena->high_water = arena->used;
    return p;
}

long sm3d_arena_remaining(const SM3D_Arena *arena)
{
    if (arena == 0) return 0;
    if (arena->capacity < arena->used) return 0;
    return arena->capacity - arena->used;
}
