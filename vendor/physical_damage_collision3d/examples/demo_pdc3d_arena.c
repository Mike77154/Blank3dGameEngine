#include <stdio.h>
#include "pdc3d.h"

int main(void)
{
    unsigned char mem[1024];
    pdc3d_arena arena;
    pdc3d_world w;
    void *scratch_a;
    void *scratch_b;

    pdc3d_arena_init(&arena, mem, (int)sizeof(mem));
    pdc3d_world_init_with_arena(&w, &arena);

    scratch_a = pdc3d_arena_alloc_bytes(w.arena, 128);
    scratch_b = pdc3d_arena_alloc_bytes(w.arena, 256);

    printf("arena scratch_a=%s scratch_b=%s used=%d remaining=%d overflow=%d\n",
           scratch_a != 0 ? "ok" : "no",
           scratch_b != 0 ? "ok" : "no",
           pdc3d_arena_used(&arena),
           pdc3d_arena_remaining(&arena),
           pdc3d_arena_overflowed(&arena));
    return 0;
}
