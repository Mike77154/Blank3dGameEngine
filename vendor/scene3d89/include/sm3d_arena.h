#ifndef SM3D_ARENA_H
#define SM3D_ARENA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct SM3D_Arena {
    unsigned char *base;
    long capacity;
    long used;
    long high_water;
    int error;
} SM3D_Arena;

void sm3d_arena_init(SM3D_Arena *arena, void *memory, long capacity);
void sm3d_arena_clear(SM3D_Arena *arena);
void *sm3d_arena_alloc(SM3D_Arena *arena, long size, long align);
long sm3d_arena_remaining(const SM3D_Arena *arena);

#ifdef __cplusplus
}
#endif

#endif
