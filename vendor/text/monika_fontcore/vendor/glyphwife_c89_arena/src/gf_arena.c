#include "gf_arena.h"

void gf_arena_init(GF_Arena *a) { a->used = 0; }
void gf_arena_reset(GF_Arena *a) { a->used = 0; }
long gf_arena_used(const GF_Arena *a) { return a->used; }
long gf_arena_remaining(const GF_Arena *a) { return GF_ARENA_SIZE - a->used; }
void gf_arena_pop(GF_Arena *a, long mark) { if (mark >= 0 && mark <= a->used) a->used = mark; }

void *gf_arena_push(GF_Arena *a, long size, long align) {
    long p;
    long mask;
    if (align <= 0) align = 1;
    mask = align - 1;
    p = (a->used + mask) & ~mask;
    if (p + size > GF_ARENA_SIZE) return (void*)0;
    a->used = p + size;
    return (void*)(a->data + p);
}

void *gf_arena_push_zero(GF_Arena *a, long size, long align) {
    unsigned char *p;
    long i;
    p = (unsigned char*)gf_arena_push(a, size, align);
    if (!p) return (void*)0;
    for (i = 0; i < size; ++i) p[i] = 0;
    return (void*)p;
}
