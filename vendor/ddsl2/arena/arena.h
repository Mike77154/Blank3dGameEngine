#ifndef DDSL_ARENA_H
#define DDSL_ARENA_H

/* Arena allocator (bump) con memoria externa fija.
 *
 * Uso típico:
 *
 *   unsigned char mem[64 * 1024];
 *   ddsl_arena a;
 *   ddsl_arena_init(&a, mem, sizeof(mem));
 *   ... ddsl_arena_alloc(&a, ...)
 *   ddsl_arena_reset(&a);
 */

#include <stddef.h> /* size_t */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ddsl_arena {
    unsigned char *base;
    size_t cap;
    size_t used;
    int oom;
} ddsl_arena;

void ddsl_arena_init(ddsl_arena *a, void *mem, size_t bytes);
void ddsl_arena_reset(ddsl_arena *a);

/* Devuelve un puntero alineado. align=0 => alineación por defecto (sizeof(void*)). */
void *ddsl_arena_alloc(ddsl_arena *a, size_t size, size_t align);
void *ddsl_arena_alloc_zero(ddsl_arena *a, size_t size, size_t align);

/* "Marks" para backtracking: */
size_t ddsl_arena_mark(ddsl_arena *a);
void ddsl_arena_rewind(ddsl_arena *a, size_t mark);

int ddsl_arena_oom(const ddsl_arena *a);
size_t ddsl_arena_used(const ddsl_arena *a);
size_t ddsl_arena_cap(const ddsl_arena *a);

#ifdef __cplusplus
}
#endif

#endif /* DDSL_ARENA_H */
