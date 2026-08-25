#ifndef W3D89_ARENA_H
#define W3D89_ARENA_H

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned long  w3d_u32;
typedef signed long    w3d_i32;
typedef unsigned short w3d_u16;
typedef signed short   w3d_i16;
typedef unsigned char  w3d_u8;

typedef struct w3d_arena_s {
    w3d_u8 *base;
    w3d_u32 size;
    w3d_u32 used;
    w3d_u32 high_water;
    w3d_u32 failed;
} w3d_arena;

void w3d_arena_init(w3d_arena *a, void *memory, w3d_u32 size);
void *w3d_arena_push(w3d_arena *a, w3d_u32 size, w3d_u32 align);
void w3d_arena_reset(w3d_arena *a);
w3d_u32 w3d_arena_used(const w3d_arena *a);
w3d_u32 w3d_arena_high_water(const w3d_arena *a);

#ifdef __cplusplus
}
#endif

#endif
