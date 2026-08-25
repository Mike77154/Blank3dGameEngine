#include "pdc3d_arena.h"

static int pdc3d_align_up(int v, int align)
{
    int r;
    if (align <= 1) {
        return v;
    }
    r = v % align;
    if (r == 0) {
        return v;
    }
    return v + (align - r);
}

void pdc3d_arena_init(pdc3d_arena *a, void *mem, int capacity)
{
    if (a == 0) {
        return;
    }
    a->mem = (unsigned char *)mem;
    a->capacity = capacity;
    a->offset = 0;
    a->high_water = 0;
    a->overflowed = 0;
    if (mem == 0 || capacity < 0) {
        a->capacity = 0;
        a->overflowed = 1;
    }
}

void pdc3d_arena_reset(pdc3d_arena *a)
{
    if (a == 0) {
        return;
    }
    a->offset = 0;
    a->overflowed = 0;
}

void *pdc3d_arena_alloc_bytes(pdc3d_arena *a, int byte_count)
{
    int start;
    int end;
    int i;
    unsigned char *p;
    if (a == 0 || byte_count < 0 || a->mem == 0) {
        if (a != 0) {
            a->overflowed = 1;
        }
        return 0;
    }
    start = pdc3d_align_up(a->offset, PDC3D_ARENA_ALIGN);
    end = start + byte_count;
    if (end < start || end > a->capacity) {
        a->overflowed = 1;
        return 0;
    }
    p = a->mem + start;
    for (i = 0; i < byte_count; ++i) {
        p[i] = 0;
    }
    a->offset = end;
    if (a->offset > a->high_water) {
        a->high_water = a->offset;
    }
    return (void *)p;
}

int pdc3d_arena_used(const pdc3d_arena *a)
{
    if (a == 0) {
        return 0;
    }
    return a->offset;
}

int pdc3d_arena_remaining(const pdc3d_arena *a)
{
    if (a == 0 || a->capacity < a->offset) {
        return 0;
    }
    return a->capacity - a->offset;
}

int pdc3d_arena_overflowed(const pdc3d_arena *a)
{
    if (a == 0) {
        return 1;
    }
    return a->overflowed;
}
