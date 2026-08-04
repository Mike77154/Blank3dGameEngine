#include "rpyl_arena.h"

#include "rpyl_config.h"

#include <string.h>

struct RpylArena {
    unsigned char* data;
    size_t capacity;
    size_t offset;
    int overflowed;
    int in_use;
    int external;
};

static RpylArena g_arenas[RPYL_MAX_ARENAS];
static unsigned char g_arena_storage[RPYL_MAX_ARENAS][RPYL_ARENA_DEFAULT_CAPACITY];

static int rpyl_align_up(size_t v, size_t align, size_t* out) {
    size_t remainder;
    size_t delta;
    size_t max_size;
    if (!out) return 0;
    if (align == 0u) {
        *out = v;
        return 1;
    }
    remainder = v % align;
    if (remainder == 0u) {
        *out = v;
        return 1;
    }
    delta = align - remainder;
    max_size = (size_t)-1;
    if (v > max_size - delta) return 0;
    *out = v + delta;
    return 1;
}

static RpylArena* claim_arena(void) {
    int i;
    for (i = 0; i < RPYL_MAX_ARENAS; i++) {
        if (!g_arenas[i].in_use) {
            memset(&g_arenas[i], 0, sizeof(g_arenas[i]));
            g_arenas[i].in_use = 1;
            return &g_arenas[i];
        }
    }
    return 0;
}

RpylArena* rpyl_arena_create(size_t initial_capacity) {
    RpylArena* a;
    int idx;

    if (initial_capacity == 0) initial_capacity = (size_t)RPYL_ARENA_DEFAULT_CAPACITY;
    if (initial_capacity > (size_t)RPYL_ARENA_DEFAULT_CAPACITY) return 0;

    a = claim_arena();
    if (!a) return 0;

    idx = (int)(a - g_arenas);
    a->data = g_arena_storage[idx];
    a->capacity = initial_capacity;
    a->offset = 0;
    a->overflowed = 0;
    a->external = 0;
    memset(a->data, 0, a->capacity);
    return a;
}

RpylArena* rpyl_arena_create_with_buffer(void* buffer, size_t capacity) {
    RpylArena* a;

    if (!buffer || capacity == 0) return 0;
    a = claim_arena();
    if (!a) return 0;

    a->data = (unsigned char*)buffer;
    a->capacity = capacity;
    a->offset = 0;
    a->overflowed = 0;
    a->external = 1;
    memset(a->data, 0, a->capacity);
    return a;
}

void rpyl_arena_destroy(RpylArena* arena) {
    if (!arena) return;
    if (!arena->external && arena->data) {
        memset(arena->data, 0, arena->capacity);
    }
    memset(arena, 0, sizeof(*arena));
}

void* rpyl_arena_alloc(RpylArena* arena, size_t size, size_t align) {
    size_t off;
    size_t new_off;
    void* p;

    if (!arena || !arena->data || size == 0) return 0;
    if (align == 0) align = sizeof(void*);

    if (!rpyl_align_up(arena->offset, align, &off)) {
        arena->overflowed = 1;
        return 0;
    }
    new_off = off + size;
    if (new_off < off || new_off > arena->capacity) {
        arena->overflowed = 1;
        return 0;
    }

    p = (void*)(arena->data + off);
    arena->offset = new_off;
    return p;
}

void* rpyl_arena_alloc_zero(RpylArena* arena, size_t size, size_t align) {
    void* p;
    p = rpyl_arena_alloc(arena, size, align);
    if (p) memset(p, 0, size);
    return p;
}

RpylArenaMark rpyl_arena_mark(RpylArena* arena) {
    RpylArenaMark m;
    m.block = 0;
    m.offset = 0;
    if (!arena) return m;
    m.block = (void*)arena;
    m.offset = arena->offset;
    return m;
}

void rpyl_arena_rewind(RpylArena* arena, RpylArenaMark mark) {
    if (!arena) return;
    if (!mark.block) {
        rpyl_arena_reset(arena);
        return;
    }
    if (mark.block != (void*)arena) return;
    if (mark.offset <= arena->capacity) arena->offset = mark.offset;
    else arena->offset = arena->capacity;
}

void rpyl_arena_reset(RpylArena* arena) {
    if (!arena) return;
    arena->offset = 0;
    arena->overflowed = 0;
    if (arena->data && arena->capacity > 0) memset(arena->data, 0, arena->capacity);
}

size_t rpyl_arena_used(const RpylArena* arena) {
    if (!arena) return 0;
    return arena->offset;
}

size_t rpyl_arena_capacity(const RpylArena* arena) {
    if (!arena) return 0;
    return arena->capacity;
}

int rpyl_arena_overflowed(const RpylArena* arena) {
    if (!arena) return 0;
    return arena->overflowed ? 1 : 0;
}
