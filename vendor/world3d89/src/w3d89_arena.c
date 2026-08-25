#include "w3d89_arena.h"

static w3d_u32 w3d_align_forward_u32(w3d_u32 value, w3d_u32 align)
{
    w3d_u32 mask;
    if (align == 0UL) return value;
    mask = align - 1UL;
    return (value + mask) & ~mask;
}

void w3d_arena_init(w3d_arena *a, void *memory, w3d_u32 size)
{
    if (!a) return;
    a->base = (w3d_u8*)memory;
    a->size = size;
    a->used = 0UL;
    a->high_water = 0UL;
    a->failed = 0UL;
}

void *w3d_arena_push(w3d_arena *a, w3d_u32 size, w3d_u32 align)
{
    w3d_u32 at;
    w3d_u32 next;
    void *ptr;
    if (!a || !a->base) return 0;
    if (align == 0UL) align = 4UL;
    at = w3d_align_forward_u32(a->used, align);
    next = at + size;
    if (next < at || next > a->size) {
        a->failed = 1UL;
        return 0;
    }
    ptr = (void*)(a->base + at);
    a->used = next;
    if (a->used > a->high_water) a->high_water = a->used;
    return ptr;
}

void w3d_arena_reset(w3d_arena *a)
{
    if (!a) return;
    a->used = 0UL;
    a->failed = 0UL;
}

w3d_u32 w3d_arena_used(const w3d_arena *a)
{
    if (!a) return 0UL;
    return a->used;
}

w3d_u32 w3d_arena_high_water(const w3d_arena *a)
{
    if (!a) return 0UL;
    return a->high_water;
}
