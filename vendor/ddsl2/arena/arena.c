#include "arena/arena.h"

#include <string.h> /* memset */

static size_t ddsl_align_up(size_t x, size_t a) {
    size_t m;
    if (a == 0) return x;
    m = a - 1;
    return (x + m) & ~m;
}

void ddsl_arena_init(ddsl_arena *a, void *mem, size_t bytes) {
    if (!a) return;
    a->base = (unsigned char *)mem;
    a->cap = bytes;
    a->used = 0;
    a->oom = 0;
}

void ddsl_arena_reset(ddsl_arena *a) {
    if (!a) return;
    a->used = 0;
    a->oom = 0;
}

void *ddsl_arena_alloc(ddsl_arena *a, size_t size, size_t align) {
    size_t at;
    size_t a2;
    if (!a || !a->base || a->cap == 0) return NULL;
    if (align == 0) align = sizeof(void *);
    a2 = align;

    at = ddsl_align_up(a->used, a2);
    if (size > a->cap) { a->oom = 1; return NULL; }
    if (at > a->cap - size) { a->oom = 1; return NULL; }

    a->used = at + size;
    return (void *)(a->base + at);
}

void *ddsl_arena_alloc_zero(ddsl_arena *a, size_t size, size_t align) {
    void *p;
    p = ddsl_arena_alloc(a, size, align);
    if (p && size) memset(p, 0, size);
    return p;
}

size_t ddsl_arena_mark(ddsl_arena *a) {
    if (!a) return 0;
    return a->used;
}

void ddsl_arena_rewind(ddsl_arena *a, size_t mark) {
    if (!a) return;
    if (mark > a->cap) mark = a->cap;
    a->used = mark;
    a->oom = 0;
}

int ddsl_arena_oom(const ddsl_arena *a) {
    if (!a) return 1;
    return a->oom ? 1 : 0;
}

size_t ddsl_arena_used(const ddsl_arena *a) {
    if (!a) return 0;
    return a->used;
}

size_t ddsl_arena_cap(const ddsl_arena *a) {
    if (!a) return 0;
    return a->cap;
}
