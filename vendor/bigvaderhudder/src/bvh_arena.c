#include "bvh_arena.h"

void bvh_arena_reset(BVH_Arena *arena) {
    if (!arena) return;
    arena->used = 0UL;
    arena->bytes[0] = 0U;
}

int bvh_arena_put(BVH_Arena *arena, const char *s, unsigned long n, unsigned long *out_off) {
    unsigned long off;
    if (!arena) return 0;
    if (!s) {
        s = "";
        n = 0UL;
    }
    if (arena->used + n + 1UL > (unsigned long)BVH_ARENA_SIZE) return 0;
    off = arena->used;
    if (n) memcpy(arena->bytes + off, s, (size_t)n);
    arena->bytes[off + n] = 0U;
    arena->used += n + 1UL;
    if (out_off) *out_off = off;
    return 1;
}

const char *bvh_arena_at(const BVH_Arena *arena, unsigned long off) {
    if (!arena) return "";
    if (off >= arena->used) return "";
    return (const char *)(arena->bytes + off);
}
